#!/usr/bin/env python3
"""按 JSONL 数据集批量调用大模型 chat/completions API 并解析 ALT/ASC 输出。

每个 JSONL 行形如 ``{"messages":[system, user, assistant]}`，脚本会：

1. 解析 messages，剥离 assistant 后构造请求体（仅保留 system + user）。
2. 从 system content 末尾提取嵌入的 TaskSpec JSON，落盘为 ``task.taskSpec.json``。
3. 取 user content 作为 query，落盘为 ``query.txt``。
4. 用 .env.toml 中指定段位的模型配置调用 chat/completions，失败时按指数退避重试。
5. 解析响应中的 ``<alt>...</alt>`` 与 ``<asc>...</asc>``，落盘为 ``card.alt.txt`` / ``card.asc.txt``。
6. 在 ``-o`` 指定的输出目录下为每个 case 创建 ``case-NNN/`` 子目录。

支持：并发、可配置重试、--debug 落盘原始 request/response、强制重跑、中文帮助。
"""

from __future__ import annotations

import argparse
import asyncio
import json
import re
import sys
import time
import tomllib
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any


if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")


DEFAULT_JOBS = 10
DEFAULT_RETRY = 3
DEFAULT_RETRY_BASE_SECONDS = 2.0
DEFAULT_REQUEST_TIMEOUT = 180.0
DEFAULT_PROGRESS_INTERVAL = 5.0
ENV_TOML_NAME = ".env.toml"

ALT_FILE_NAME = "card.alt.txt"
ASC_FILE_NAME = "card.asc.txt"
QUERY_FILE_NAME = "query.txt"
TASKSPEC_FILE_NAME = "task.taskSpec.json"
REQUEST_FILE_NAME = "request.json"
RESPONSE_FILE_NAME = "response.json"
SYSTEM_RAW_FILE_NAME = "system.raw.txt"
LOG_FILE_NAME = "run_alt_batch.log"
SUMMARY_FILE_NAME = "run_alt_batch_summary.json"

REQUIRED_OUTPUTS = (QUERY_FILE_NAME, TASKSPEC_FILE_NAME, ALT_FILE_NAME, ASC_FILE_NAME)
SKIP_IF_EXIST = (ALT_FILE_NAME, ASC_FILE_NAME)

RESERVED_EXTRA_KEYS = {"messages", "model"}
TOML_RESERVED_KEYS = {"api_url", "api_key", "model"}

THINK_RE = re.compile(r"<think>.*?</think>", re.DOTALL)
ALT_RE = re.compile(r"<alt>(.*?)</alt>", re.DOTALL)
ASC_RE = re.compile(r"<asc>(.*?)</asc>", re.DOTALL)
FENCE_RE = re.compile(r"```(?:[A-Za-z0-9_+\-]*)\s*|\s*```", re.MULTILINE)


LOG_PATH: Path | None = None


def now() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def log(message: str) -> None:
    line = f"[{now()}] {message}"
    enc = sys.stdout.encoding or "utf-8"
    try:
        sys.stdout.write(line + "\n")
        sys.stdout.flush()
    except UnicodeEncodeError:
        sys.stdout.write(
            (line + "\n").encode(enc, errors="replace").decode(enc, errors="replace")
        )
        sys.stdout.flush()
    if LOG_PATH is not None:
        try:
            with LOG_PATH.open("a", encoding="utf-8") as handle:
                handle.write(line + "\n")
        except OSError:
            pass


@dataclass(frozen=True)
class CaseItem:
    line_no: int
    index: int
    query: str
    system_content: str
    messages: list[dict]

    @property
    def directory_name(self) -> str:
        return f"case-{self.index:03d}"


@dataclass
class CaseResult:
    case: CaseItem
    ok: bool
    action: str
    elapsed: float
    out_dir: Path
    error: str = ""
    retries: int = 0


@dataclass(frozen=True)
class EnvConfig:
    label: str
    api_url: str
    api_key: str
    model: str
    base_url: str
    extra_body: dict[str, Any]


class _RetryableError(Exception):
    """触发重试的异常。"""


class _PermanentError(Exception):
    """不应触发重试的异常（请求格式错误、4xx 客户端错误等）。"""


class _InFlight:
    def __init__(self) -> None:
        self.lock = asyncio.Lock()
        self.in_flight: dict[str, float] = {}
        self.finished = 0
        self.total = 0
        self.jobs = 0

    def configure(self, total: int, jobs: int) -> None:
        self.total = total
        self.jobs = jobs

    async def begin(self, name: str) -> float:
        async with self.lock:
            started = time.perf_counter()
            self.in_flight[name] = started
            return started

    async def end(self, name: str) -> None:
        async with self.lock:
            self.in_flight.pop(name, None)
            self.finished += 1

    async def snapshot(self) -> str:
        async with self.lock:
            names = sorted(self.in_flight.keys())
            return (
                f"in_flight({len(names)}/{self.jobs}) "
                f"finished={self.finished}/{self.total} "
                + ", ".join(names)
            )


def find_env_toml(override: str | None) -> Path:
    if override:
        path = Path(override).expanduser().resolve()
        if not path.is_file():
            raise FileNotFoundError(f"--env-toml 指向的文件不存在: {path}")
        return path
    cwd = Path.cwd()
    candidates: list[Path] = []
    try:
        candidates.append((cwd / ENV_TOML_NAME).resolve())
    except OSError:
        pass
    parent = cwd.parent
    try:
        candidates.append((parent / ENV_TOML_NAME).resolve())
    except OSError:
        pass
    seen: set[Path] = set()
    for candidate in candidates:
        if candidate in seen:
            continue
        seen.add(candidate)
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(
        f"未找到 {ENV_TOML_NAME}, 请通过 --env-toml 指定路径, 或在 cwd/父目录放置该文件"
    )


def load_env_config(path: Path, label: str) -> EnvConfig:
    try:
        raw = tomllib.loads(path.read_text(encoding="utf-8-sig"))
    except OSError as exc:
        raise FileNotFoundError(f"无法读取 {path}: {exc}") from exc
    if label not in raw:
        raise ValueError(f"{path} 中找不到 toml 段 [{label}], 可用段: {sorted(raw.keys())}")
    section = raw[label]
    if not isinstance(section, dict):
        raise ValueError(f"{path} 中 [{label}] 不是 toml table")
    api_url = section.get("api_url")
    api_key = section.get("api_key")
    model = section.get("model")
    if not isinstance(api_url, str) or not api_url.strip():
        raise ValueError(f"{path} 中 [{label}].api_url 缺失或非字符串")
    if not isinstance(api_key, str) or not api_key.strip():
        raise ValueError(f"{path} 中 [{label}].api_key 缺失或非字符串")
    if not isinstance(model, str) or not model.strip():
        raise ValueError(f"{path} 中 [{label}].model 缺失或非字符串")
    base_url = api_url
    suffix = "/chat/completions"
    if base_url.endswith(suffix):
        base_url = base_url[: -len(suffix)]
    extra_body: dict[str, Any] = {
        k: v for k, v in section.items() if k not in TOML_RESERVED_KEYS
    }
    for reserved in RESERVED_EXTRA_KEYS:
        if reserved in extra_body:
            raise ValueError(
                f"{path} 中 [{label}] 含保留键 {reserved!r}, 不允许覆盖 (api_url/api_key/model 之外仅 messages/model 禁止)"
            )
    return EnvConfig(
        label=label,
        api_url=api_url,
        api_key=api_key,
        model=model,
        base_url=base_url,
        extra_body=extra_body,
    )


def extract_message_text(messages: list[Any], role: str) -> str:
    for msg in messages:
        if isinstance(msg, dict) and msg.get("role") == role:
            content = msg.get("content", "")
            if isinstance(content, str):
                return content
    return ""


def strip_assistant(messages: list[dict]) -> list[dict]:
    stripped: list[dict] = []
    for msg in messages:
        if not isinstance(msg, dict):
            continue
        if msg.get("role") == "assistant":
            continue
        role = msg.get("role")
        content = msg.get("content", "")
        if role not in {"system", "user"}:
            continue
        if not isinstance(content, str):
            continue
        stripped.append({"role": role, "content": content})
    return stripped


def extract_taskspec_from_system(system_content: str) -> dict[str, Any]:
    """扫描 system content 中所有顶层 JSON 对象, 挑出含 userQuery/size 的最后一个。

    顶层对象的判断: raw_decode 顺序扫描, 每解析成功一次就跳到 ``end``, 因此不会
    把嵌套在已解析对象里的字典再拆出来。
    """
    decoder = json.JSONDecoder()
    candidates: list[dict[str, Any]] = []
    i = 0
    n = len(system_content)
    while i < n:
        ch = system_content[i]
        if ch != "{":
            i += 1
            continue
        try:
            obj, end = decoder.raw_decode(system_content, i)
        except json.JSONDecodeError:
            i += 1
            continue
        if isinstance(obj, dict) and "userQuery" in obj and "size" in obj:
            candidates.append(obj)
        i = max(end, i + 1)
    if not candidates:
        raise ValueError("在 system content 中未找到含 userQuery/size 的 TaskSpec JSON 对象")
    return candidates[-1]


def load_cases(
    jsonl_path: Path,
    start: int | None,
    end: int | None,
    limit: int | None,
) -> list[CaseItem]:
    cases: list[CaseItem] = []
    with jsonl_path.open("r", encoding="utf-8-sig") as handle:
        for line_no, raw_line in enumerate(handle, start=1):
            line = raw_line.strip()
            if not line:
                continue
            try:
                payload = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(f"{jsonl_path}:{line_no} 不是合法 JSON: {exc}") from exc
            if not isinstance(payload, dict):
                raise ValueError(f"{jsonl_path}:{line_no} 必须是 JSON object")
            messages = payload.get("messages")
            if not isinstance(messages, list) or not messages:
                raise ValueError(f"{jsonl_path}:{line_no} 缺少 messages 数组")
            system_content = extract_message_text(messages, "system")
            user_content = extract_message_text(messages, "user").strip()
            if not system_content:
                raise ValueError(f"{jsonl_path}:{line_no} 缺少 system content")
            if not user_content:
                raise ValueError(f"{jsonl_path}:{line_no} 缺少 user content")
            stripped = strip_assistant(messages)
            if not any(m["role"] == "system" for m in stripped):
                raise ValueError(f"{jsonl_path}:{line_no} 缺少 system 消息")
            if not any(m["role"] == "user" for m in stripped):
                raise ValueError(f"{jsonl_path}:{line_no} 缺少 user 消息")
            cases.append(
                CaseItem(
                    line_no=line_no,
                    index=len(cases) + 1,
                    query=user_content,
                    system_content=system_content,
                    messages=stripped,
                )
            )
    if start is not None or end is not None or (limit is not None and limit > 0):
        selected: list[CaseItem] = []
        for case in cases:
            if start is not None and case.line_no < start:
                continue
            if end is not None and case.line_no > end:
                continue
            selected.append(case)
            if limit is not None and limit > 0 and len(selected) >= limit:
                break
        cases = selected
    return cases


def _extract_assistant_text(raw: dict[str, Any]) -> str:
    choices = raw.get("choices")
    if not isinstance(choices, list) or not choices:
        return ""
    choice = choices[0]
    if not isinstance(choice, dict):
        return ""
    parts: list[str] = []
    message = choice.get("message")
    if isinstance(message, dict):
        for field in ("reasoning_content", "content"):
            value = message.get(field)
            if isinstance(value, str) and value.strip():
                parts.append(value)
    if not parts:
        text = choice.get("text")
        if isinstance(text, str) and text.strip():
            parts.append(text)
    return "\n".join(parts)


def parse_alt_asc(text: str) -> tuple[str, str]:
    """从响应文本中提取 (alt, asc)。"""
    cleaned = FENCE_RE.sub("", text)
    cleaned = THINK_RE.sub("", cleaned)
    alt_match = ALT_RE.search(cleaned)
    asc_match = ASC_RE.search(cleaned)
    if not alt_match or not asc_match:
        raise _RetryableError(
            f"响应缺少 <alt> 或 <asc> 段 (alt_found={bool(alt_match)} asc_found={bool(asc_match)})"
        )
    alt = alt_match.group(1).strip()
    asc = asc_match.group(1).strip()
    if not alt or not asc:
        raise _RetryableError("<alt> 或 <asc> 段内容为空")
    return alt, asc


async def call_model(
    env: EnvConfig,
    messages: list[dict],
    request_timeout: float,
) -> tuple[str, dict[str, Any]]:
    """调用 chat/completions, 返回 (assistant_text, raw_response_dict)。"""
    from openai import (
        APIConnectionError,
        APIError,
        APITimeoutError,
        AsyncOpenAI,
        InternalServerError,
        RateLimitError,
    )

    client = AsyncOpenAI(
        api_key=env.api_key,
        base_url=env.base_url,
        timeout=request_timeout,
        max_retries=0,
    )
    try:
        resp = await client.chat.completions.create(
            model=env.model,
            messages=messages,
            **env.extra_body,
        )
    except (APIConnectionError, APITimeoutError, RateLimitError) as exc:
        raise _RetryableError(
            f"API 网络/限流异常 ({type(exc).__name__}): {exc}"
        ) from exc
    except InternalServerError as exc:
        raise _RetryableError(
            f"API 服务端错误 ({type(exc).__name__}): {exc}"
        ) from exc
    except APIError as exc:
        status = getattr(exc, "status_code", None)
        if isinstance(status, int) and 400 <= status < 500 and status != 429:
            raise _PermanentError(
                f"API 客户端错误 (status={status}, {type(exc).__name__}): {exc}"
            ) from exc
        raise _RetryableError(
            f"API 错误 ({type(exc).__name__}, status={status}): {exc}"
        ) from exc
    except (ValueError, TypeError, KeyError) as exc:
        raise _PermanentError(
            f"请求参数错误 ({type(exc).__name__}): {exc}"
        ) from exc
    except Exception as exc:
        raise _RetryableError(
            f"API 调用未知异常 ({type(exc).__name__}): {exc}"
        ) from exc

    try:
        raw = resp.model_dump()
    except Exception as exc:
        raise _RetryableError(f"API 响应序列化失败: {exc}") from exc

    text = _extract_assistant_text(raw)
    if not text:
        raise _RetryableError("API 响应中未找到 assistant content / reasoning_content")
    return text, raw


def _is_case_skippable(case_dir: Path) -> bool:
    if not case_dir.is_dir():
        return False
    return all((case_dir / name).is_file() for name in SKIP_IF_EXIST)


def _write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8", errors="strict")


async def progress_printer(tracker: _InFlight, interval: float, stop_event: asyncio.Event) -> None:
    try:
        while not stop_event.is_set():
            try:
                await asyncio.wait_for(stop_event.wait(), timeout=interval)
            except asyncio.TimeoutError:
                pass
            snapshot = await tracker.snapshot()
            log(f"PROGRESS {snapshot}")
    except asyncio.CancelledError:
        raise


async def run_case(
    case: CaseItem,
    args: argparse.Namespace,
    env: EnvConfig,
    output_dir: Path,
    sem: asyncio.Semaphore,
    tracker: _InFlight,
) -> CaseResult:
    case_dir = output_dir / case.directory_name
    async with sem:
        await tracker.begin(case.directory_name)
        started_at = time.perf_counter()
        try:
            if not args.force and _is_case_skippable(case_dir):
                elapsed = time.perf_counter() - started_at
                log(
                    f"SKIP  line={case.line_no} case={case.directory_name} "
                    f"reason=output-exists elapsed={elapsed:.1f}s"
                )
                return CaseResult(
                    case=case, ok=True, action="skipped", elapsed=elapsed, out_dir=case_dir
                )

            log(
                f"PREP  line={case.line_no} case={case.directory_name} mkdir={case_dir}"
            )
            case_dir.mkdir(parents=True, exist_ok=True)

            try:
                taskspec = extract_taskspec_from_system(case.system_content)
            except Exception as exc:
                elapsed = time.perf_counter() - started_at
                if args.debug:
                    _write_text(
                        case_dir / SYSTEM_RAW_FILE_NAME, case.system_content + "\n"
                    )
                log(
                    f"FAIL  line={case.line_no} case={case.directory_name} "
                    f"stage=parse-taskspec error={exc} elapsed={elapsed:.1f}s"
                )
                return CaseResult(
                    case=case,
                    ok=False,
                    action="failed-parse",
                    elapsed=elapsed,
                    out_dir=case_dir,
                    error=str(exc),
                )

            _write_text(case_dir / QUERY_FILE_NAME, case.query + "\n")
            _write_text(
                case_dir / TASKSPEC_FILE_NAME,
                json.dumps(taskspec, ensure_ascii=False, indent=2) + "\n",
            )

            last_error = ""
            retries_used = 0
            alt_text = ""
            asc_text = ""
            raw_response: dict[str, Any] = {}
            max_attempts = args.retry + 1
            for attempt in range(1, max_attempts + 1):
                if attempt > 1:
                    retries_used = attempt - 1
                    wait_seconds = args.retry_base_seconds * (2 ** (attempt - 2))
                    log(
                        f"RETRY line={case.line_no} case={case.directory_name} "
                        f"attempt={attempt}/{max_attempts} wait={wait_seconds:.1f}s "
                        f"last_error={last_error}"
                    )
                    await asyncio.sleep(wait_seconds)
                log(
                    f"CALL  line={case.line_no} case={case.directory_name} "
                    f"model={env.model} attempt={attempt}/{max_attempts}"
                )
                try:
                    text, raw_response = await call_model(
                        env, case.messages, args.request_timeout
                    )
                    alt_text, asc_text = parse_alt_asc(text)
                    last_error = ""
                    break
                except _PermanentError as exc:
                    elapsed = time.perf_counter() - started_at
                    log(
                        f"FAIL  line={case.line_no} case={case.directory_name} "
                        f"stage=api permanent error={exc} elapsed={elapsed:.1f}s"
                    )
                    return CaseResult(
                        case=case,
                        ok=False,
                        action="failed-api",
                        elapsed=elapsed,
                        out_dir=case_dir,
                        error=str(exc),
                        retries=retries_used,
                    )
                except _RetryableError as exc:
                    last_error = str(exc)
                    log(
                        f"WARN  line={case.line_no} case={case.directory_name} "
                        f"stage=api attempt={attempt}/{max_attempts} error={exc}"
                    )
                    continue

            if last_error:
                elapsed = time.perf_counter() - started_at
                log(
                    f"FAIL  line={case.line_no} case={case.directory_name} "
                    f"stage=api retries_exhausted error={last_error} elapsed={elapsed:.1f}s"
                )
                return CaseResult(
                    case=case,
                    ok=False,
                    action="failed-api",
                    elapsed=elapsed,
                    out_dir=case_dir,
                    error=last_error,
                    retries=retries_used,
                )

            _write_text(case_dir / ALT_FILE_NAME, alt_text + "\n")
            _write_text(case_dir / ASC_FILE_NAME, asc_text + "\n")

            if args.debug:
                request_body = {
                    "model": env.model,
                    "messages": case.messages,
                    **env.extra_body,
                }
                _write_text(
                    case_dir / REQUEST_FILE_NAME,
                    json.dumps(request_body, ensure_ascii=False, indent=2) + "\n",
                )
                _write_text(
                    case_dir / RESPONSE_FILE_NAME,
                    json.dumps(raw_response, ensure_ascii=False, indent=2) + "\n",
                )

            elapsed = time.perf_counter() - started_at
            log(
                f"DONE  line={case.line_no} case={case.directory_name} "
                f"alt_chars={len(alt_text)} asc_chars={len(asc_text)} "
                f"retries={retries_used} elapsed={elapsed:.1f}s"
            )
            return CaseResult(
                case=case,
                ok=True,
                action="generated",
                elapsed=elapsed,
                out_dir=case_dir,
                retries=retries_used,
            )
        finally:
            await tracker.end(case.directory_name)


def write_summary(output_dir: Path, results: list[CaseResult]) -> None:
    summary: dict[str, Any] = {
        "generatedAt": datetime.now().isoformat(timespec="seconds"),
        "total": len(results),
        "ok": sum(1 for r in results if r.ok),
        "skipped": sum(1 for r in results if r.action == "skipped"),
        "generated": sum(1 for r in results if r.action == "generated"),
        "failed": len(results) - sum(1 for r in results if r.ok),
        "failedParse": sum(1 for r in results if r.action == "failed-parse"),
        "failedApi": sum(1 for r in results if r.action == "failed-api"),
        "results": [
            {
                "line": r.case.line_no,
                "case": r.case.directory_name,
                "ok": r.ok,
                "action": r.action,
                "elapsedSeconds": round(r.elapsed, 3),
                "retries": r.retries,
                "outputDir": str(r.out_dir),
                "error": r.error,
            }
            for r in results
        ],
    }
    (output_dir / SUMMARY_FILE_NAME).write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="run_alt_dataset.py",
        description="按 JSONL 数据集批量调用大模型 chat/completions API, 解析并落盘 ALT/ASC。",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""调用示例:
  # 用 glm 段跑 test.alt.jsonl, 10 并发
  python scripts/run_alt_dataset.py -i datasets/test.alt.jsonl -o datasets/case-glm-test -m glm

  # 用 doubao 跑前 10 条, 5 并发
  python scripts/run_alt_dataset.py -i datasets/test.alt.jsonl -o tmp/run -m doubao -j 5 --limit 10

  # debug 模式: 保存原始 request/response 便于排查
  python scripts/run_alt_dataset.py -i datasets/test.alt.jsonl -o tmp/run -m glm --debug

  # 强制重跑已存在的 case-001 ~ case-005
  python scripts/run_alt_dataset.py -i datasets/test.alt.jsonl -o tmp/run -m glm --force

  # 自定义重试次数和请求超时
  python scripts/run_alt_dataset.py -i datasets/test.alt.jsonl -o tmp/run -m glm -r 5 --request-timeout 300

  # 通过 --env-toml 指定其他 toml 文件
  python scripts/run_alt_dataset.py -i datasets/test.alt.jsonl -o tmp/run -m glm --env-toml configs/api.toml
""",
    )
    parser.add_argument("-i", "--input", required=True, type=Path, help="JSONL 数据集路径, 必填。")
    parser.add_argument(
        "-o", "--output-dir", required=True, type=Path, help="输出根目录, 必填。"
    )
    parser.add_argument(
        "-m",
        "--model-label",
        required=True,
        help=".env.toml 中的 toml 段名 (例如 glm / doubao / minimax)。",
    )
    parser.add_argument(
        "-j", "--jobs", type=int, default=DEFAULT_JOBS, help=f"并发 case 数, 默认 {DEFAULT_JOBS}。"
    )
    parser.add_argument(
        "-r",
        "--retry",
        type=int,
        default=DEFAULT_RETRY,
        help=f"单 case 最大重试次数 (不含首次), 默认 {DEFAULT_RETRY}。",
    )
    parser.add_argument(
        "--retry-base-seconds",
        type=float,
        default=DEFAULT_RETRY_BASE_SECONDS,
        help=f"指数退避基数秒 (实际等待=base*2^(attempt-2)), 默认 {DEFAULT_RETRY_BASE_SECONDS}。",
    )
    parser.add_argument(
        "--request-timeout",
        type=float,
        default=DEFAULT_REQUEST_TIMEOUT,
        help=f"单次 HTTP 请求超时秒, 默认 {DEFAULT_REQUEST_TIMEOUT}。",
    )
    parser.add_argument(
        "--env-toml",
        type=Path,
        help="覆盖 .env.toml 路径, 默认依次查找 cwd/.env.toml -> 父目录/.env.toml。",
    )
    parser.add_argument("--start", type=int, help="1-based 起始行号 (含)。")
    parser.add_argument("--end", type=int, help="1-based 结束行号 (含)。")
    parser.add_argument("--limit", type=int, help="最多处理的 case 数。")
    parser.add_argument(
        "--force",
        action="store_true",
        help="已存在完整输出的 case 目录仍强制重跑, 不跳过。",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="保存原始 request.json / response.json; 解析 TaskSpec 失败时保留 system.raw.txt。",
    )
    parser.add_argument(
        "--progress-interval",
        type=float,
        default=DEFAULT_PROGRESS_INTERVAL,
        help=f"PROGRESS 日志打印间隔秒, 默认 {DEFAULT_PROGRESS_INTERVAL}。",
    )
    return parser.parse_args(argv)


async def async_main(args: argparse.Namespace) -> int:
    global LOG_PATH

    if args.jobs < 1:
        raise ValueError("--jobs 必须 >= 1")
    if args.retry < 0:
        raise ValueError("--retry 必须 >= 0")
    if args.request_timeout <= 0:
        raise ValueError("--request-timeout 必须 > 0")
    if args.retry_base_seconds < 0:
        raise ValueError("--retry-base-seconds 必须 >= 0")
    if args.start is not None and args.start < 1:
        raise ValueError("--start 必须 >= 1")
    if args.end is not None and args.end < 1:
        raise ValueError("--end 必须 >= 1")
    if args.limit is not None and args.limit < 1:
        raise ValueError("--limit 必须 >= 1")

    input_path = args.input.resolve()
    output_dir = args.output_dir.resolve()
    if not input_path.is_file():
        raise FileNotFoundError(f"输入 JSONL 不存在: {input_path}")
    output_dir.mkdir(parents=True, exist_ok=True)

    env_path = find_env_toml(str(args.env_toml) if args.env_toml else None)
    env = load_env_config(env_path, args.model_label)

    LOG_PATH = output_dir / LOG_FILE_NAME
    LOG_PATH.write_text("", encoding="utf-8")

    cases = load_cases(input_path, args.start, args.end, args.limit)
    if not cases:
        log(
            f"未选择任何 case。 input={input_path} start={args.start} end={args.end} "
            f"limit={args.limit}"
        )
        return 0

    log(
        f"Batch start input={input_path} output={output_dir} model={env.model} "
        f"base_url={env.base_url} env_toml={env_path} cases={len(cases)} jobs={args.jobs} "
        f"retry={args.retry} force={args.force} debug={args.debug}"
    )
    if env.extra_body:
        log(f"extra_body keys={sorted(env.extra_body.keys())}")

    tracker = _InFlight()
    tracker.configure(len(cases), args.jobs)
    sem = asyncio.Semaphore(args.jobs)
    stop_event = asyncio.Event()
    progress_task: asyncio.Task[None] | None = None
    if len(cases) > 1:
        progress_task = asyncio.create_task(
            progress_printer(tracker, args.progress_interval, stop_event)
        )

    try:
        results = await asyncio.gather(
            *(
                run_case(case, args, env, output_dir, sem, tracker)
                for case in cases
            )
        )
    finally:
        stop_event.set()
        if progress_task is not None:
            try:
                await progress_task
            except asyncio.CancelledError:
                pass
        final_snapshot = await tracker.snapshot()
        log(f"PROGRESS-FINAL {final_snapshot}")

    results_list = list(results)
    write_summary(output_dir, results_list)

    ok_count = sum(1 for r in results_list if r.ok)
    skip_count = sum(1 for r in results_list if r.action == "skipped")
    gen_count = sum(1 for r in results_list if r.action == "generated")
    fail_parse = sum(1 for r in results_list if r.action == "failed-parse")
    fail_api = sum(1 for r in results_list if r.action == "failed-api")
    fail_total = len(results_list) - ok_count
    log(
        f"Batch finished total={len(results_list)} ok={ok_count} skipped={skip_count} "
        f"generated={gen_count} failed-parse={fail_parse} failed-api={fail_api} "
        f"summary={output_dir / SUMMARY_FILE_NAME}"
    )
    return 0 if fail_total == 0 else 1


def main(argv: list[str] | None = None) -> int:
    try:
        args = parse_args(argv)
        return asyncio.run(async_main(args))
    except KeyboardInterrupt:
        log("Interrupted.")
        return 130
    except Exception as exc:
        log(f"ERROR {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
