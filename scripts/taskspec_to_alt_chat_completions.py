#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
import unicodedata
from pathlib import Path
from typing import Any

from alt_converter import (
    auto_layout_document,
    is_auto_layout,
    parse_alt,
    parse_asc,
    validate_auto_asc,
    validate_auto_protocol,
    validate_layout,
)


if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")


DEFAULT_ALT_NAME = "card.alt.txt"
DEFAULT_ASC_NAME = "card.asc.txt"
DEFAULT_REJECTED_ALT_NAME = "card.MiniMax-M3.alt.txt"
DEFAULT_REJECTED_ASC_NAME = "card.MiniMax-M3.asc.txt"
CONFIG_DIR = Path(__file__).resolve().parent / "config"
LAYOUT_PROFILE = json.loads((CONFIG_DIR / "alt-layout-profile.json").read_text(encoding="utf-8"))
THEME_CONFIG = json.loads((CONFIG_DIR / "alt-themes.json").read_text(encoding="utf-8"))
REQUIRED_TOP_LEVEL = {
    "userQuery",
    "size",
    "eventCandidates",
    "dataModelSchema",
    "assetCandidates",
}
ALLOWED_TOP_LEVEL = REQUIRED_TOP_LEVEL | {"presentationSlots"}
FORBIDDEN_EVENT_FIELDS = {
    "description",
    "eventRef",
    "id",
    "label",
    "note",
    "onClick",
    "required",
}


def build_static_context(size: str) -> str:
    """Build protocol limits for the TaskSpec's selected card size."""
    themes = ", ".join(THEME_CONFIG.get("themes", {}).keys())
    limits = dict(LAYOUT_PROFILE.get("limits", {}).get(size, {}))
    # Automatic training output deliberately excludes components whose intrinsic
    # geometry or collection template cannot be inferred safely from semantics.
    limits["maxCheckbox"] = 0
    limits["maxLists"] = 0
    limits.pop("maxVisibleListItems", None)
    lines = [f"- 主题白名单（theme 只能取以下名称）：{themes}"]
    lines.append(f"- {size} 容量预算（Checkbox/List 一律禁用）：")
    detail = ", ".join(f"{key}={value}" for key, value in limits.items())
    lines.append(f"  - {detail}")
    return "\n".join(lines)


SYSTEM_PROMPT = """你是 A2UI 卡片结构规划模型。根据 TaskSpec、协议约束与本卡上下文 CASE_CONTEXT 同时生成自动布局 ALT 与 ASC；不要生成 GenUI DSL、CardSpec、颜色、尺寸或解释。

TaskSpec 是本卡的完整事实源：阅读其中的用户需求、数据字段、素材和事件以规划卡片。CASE_CONTEXT 是脚本从同一 TaskSpec 计算出的安全引用索引；生成 bind、asset、event 或 Progress 时必须遵守它。

输出契约：
- 只输出一个 <alt>...</alt> 和紧随其后的一个 <asc>...</asc>，不使用 Markdown 围栏，不输出思考、标题、解释或日志。
- TaskSpec 是输入数据而非输出对象；不要输出完整 TaskSpec JSON、dataModelSchema、素材 src/bindTo 或完整事件对象。
- ALT 直接从根节点开始，每层两个空格，每行一个节点，不输出 @alt 头。
- 根节点必须是 Column，且必须写 card=2x2|2x4 theme=THEME；card 必须等于 TaskSpec.size，THEME 必须来自主题白名单。自动布局不使用根 Row。
- 普通节点语法只能是 Component id 或 Component id role=ROLE；除根节点 card/theme 外，ALT 唯一允许的属性是 role。
- 禁止输出 box、size、padding、margin、gap、font、chars、lines、overflow、fit、ratio、type、颜色、背景、圆角、边框、阴影、clip、protect、style 或任何其他样式字段。它们全部由编译器根据文本和原生 profile 推断。
- 自动生成只允许 Text、Image、Divider、Progress、Button、Row、Column。禁止 Stack、Checkbox、List 和 Repeat；Checkbox 的原生尺寸不可控，List 的集合模板暂不进入训练输出。
- 节点 ID 必须唯一并匹配 [A-Za-z_][A-Za-z0-9_-]*；优先使用 presentationSlots 的 key。
- 所有叶节点必须声明 role。推荐 role：title、primary、status、metric、support、meta、action、asset、selection、separator、item。
- 只允许一个 role=primary；包括根节点在内的每个 Row/Column 最多三个直接子节点；禁止空容器和未挂载节点。
- 2x2 的根节点固定采用 Column；Button 必须是某个 Column 的直接子节点，不能放进 Row。Row 只放最多两个短 Text 或一个 Image 加一个短 Text。
- primary 只承载短数值、短状态或不超过 4 个中文等价单位的文本；较长的动态字段使用 status 或 support。2x2 title 不超过 6 个中文等价单位，2x4 title 不超过 10 个。
- Button 标签必须短：2x2 不超过 4 个中文等价单位，2x4 不超过 6 个；放不下时改用更短的动作词，不要生成长标签、第二行或缩小字体。

结构和内容预算：
- 只服务一个对象或主问题，同一事实只由一个节点主承载。
- 节点、组件和可见文本的数量上限以“协议约束”中的当前卡片容量预算为准。
- 只展示状态使用 Text，表达动作使用 Button；不要使用 Checkbox 或 List。
- 长说明、第三层元信息、重复标签、第二主任务和额外动作不进入卡片。
- 字段摘要中的 units 已由脚本计算。优先保留标题、主值/状态和必要动作，再保留一个短 support；放不下时删除 meta、次要 asset 和弱 support。

主题和 SVG：
- 模型只选择主题名，禁止输出任何具体颜色。编译器会为卡片根容器和主按钮应用该主题的多停靠渐变；通过主题匹配场景气质，不要用额外节点或样式模拟背景装饰。
- neutral-light 是默认通用主题；ambient-light 用于天气、环境、健康或设备；focus-dark 仅用于睡眠、专注、音乐或明确夜间场景。
- Image 只能通过 ASC asset=N 引用 CASE_CONTEXT.assets 中的本地 SVG；禁止 PNG、网络图、base64、emoji、src 路径和素材颜色。
- SVG 尺寸、objectFit 和 fillColor 全部由编译器按主题和角色生成。

ASC：
- 使用 Component node_id key=value 单行语法，只列有语义补充的节点，并严格遵循 ALT 前序顺序。ASC 属性按空白分隔；任何包含空格、制表符或换行的静态字符串，必须作为一个 JSON 双引号字符串 token，并按 JSON 规则转义。
- Text：text=静态文案、bind=/路径，复杂表达式才用 expr。bind 必须逐字符等于 CASE_CONTEXT.bindableFields 中的一个 path；绝不能绑定对象、数组、父路径、猜测路径或路径前缀。含空格的文案必须写成例如 text="今日已用 42 分钟"。
- Image：只用 asset=N。
- Progress：value=/路径、total=/路径。
- Button：label=短文案 event=N；含空格的标签必须写成例如 label="Open calendar" event=0。Checkbox：label、value、group、bind、event=N。
- event=N 和 asset=N 只引用 CASE_CONTEXT 中的索引，不复制事件、素材路径或颜色。
- Progress 的 value 和 total 也必须逐字符等于 CASE_CONTEXT.bindableFields 中的标量叶子 path；缺少两个合法数值字段时不要生成 Progress。
- ASC 禁止 id、component、children、styles、尺寸、字体、颜色、圆角、间距、DataModel 和操作符。

示例：
<alt>
Column root card=2x2 theme=neutral-light
  Row header
    Image battery_icon role=asset
    Text title role=title
  Text battery_value role=primary
  Button action_button role=action
</alt>
<asc>
Image battery_icon asset=0
Text title text=低电模式
Text battery_value bind=/battery/levelText
Button action_button label=立即省电 event=0
</asc>

TaskSpec（完整嵌入，作为本卡事实源，不是输出段）：
{{TASK_SPEC_JSON}}

协议约束（脚本按当前卡片尺寸与配置生成）：
{{STATIC_CONTEXT}}

本卡上下文 CASE_CONTEXT（脚本从 TaskSpec 推导的安全引用索引）：
{{CASE_CONTEXT_JSON}}
"""


def load_task_spec(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ValueError(f"input path must be a TaskSpec JSON file: {path}")
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(data, dict):
        raise ValueError("TaskSpec must be a JSON object")
    validate_task_spec(data)
    return data


def validate_task_spec(spec: dict[str, Any]) -> None:
    extra = sorted(set(spec) - ALLOWED_TOP_LEVEL)
    if extra:
        raise ValueError(
            "TaskSpec contains unsupported top-level field(s): "
            + ", ".join(repr(key) for key in extra)
        )
    missing = sorted(REQUIRED_TOP_LEVEL - set(spec))
    if missing:
        raise ValueError(
            "TaskSpec must contain top-level field(s): "
            + ", ".join(repr(key) for key in missing)
        )
    if not isinstance(spec["userQuery"], str) or not spec["userQuery"].strip():
        raise ValueError("TaskSpec.userQuery must be a non-empty string")
    if spec["size"] not in {"2x2", "2x4"}:
        raise ValueError("TaskSpec.size must be '2x2' or '2x4'")
    if not isinstance(spec["dataModelSchema"], dict):
        raise ValueError("TaskSpec.dataModelSchema must be an object")
    validate_data_model_schema(spec["dataModelSchema"], "/dataModelSchema")
    validate_event_candidates(spec["eventCandidates"])
    validate_asset_candidates(spec["assetCandidates"], spec["dataModelSchema"])
    slots = spec.get("presentationSlots")
    if slots is not None:
        validate_presentation_slots(
            slots,
            asset_count=len(spec["assetCandidates"]),
            event_count=len(spec["eventCandidates"]),
        )


def validate_data_model_schema(value: Any, location: str) -> None:
    if not isinstance(value, dict):
        raise ValueError(f"{location} must be an object")
    if "sampleValue" in value:
        for key in ("type", "description", "sampleValue"):
            if key not in value:
                raise ValueError(f"{location} field schema must contain {key!r}")
        if not isinstance(value["type"], str) or not value["type"].strip():
            raise ValueError(f"{location}.type must be a non-empty string")
        if not isinstance(value["description"], str) or not value["description"].strip():
            raise ValueError(f"{location}.description must be a non-empty string")
        return
    if value.get("type") == "array" and isinstance(value.get("items"), dict):
        validate_data_model_schema(value["items"], f"{location}/items")
        return
    if value.get("type") == "object" and isinstance(value.get("properties"), dict):
        validate_data_model_schema(value["properties"], f"{location}/properties")
        return
    if is_field_schema(value):
        raise ValueError(f"{location} field schema must contain 'sampleValue'")
    for key, child in value.items():
        if key in {"properties", "items"} and isinstance(child, dict):
            validate_data_model_schema(child, f"{location}/{key}")
        elif isinstance(child, dict):
            validate_data_model_schema(child, f"{location}/{key}")


def is_field_schema(value: dict[str, Any]) -> bool:
    if "sampleValue" in value:
        return True
    return "type" in value and (
        "description" in value or "properties" in value or "items" in value
    )


def validate_event_candidates(value: Any) -> None:
    if not isinstance(value, list):
        raise ValueError("TaskSpec.eventCandidates must be an array")
    for index, candidate in enumerate(value):
        location = f"TaskSpec.eventCandidates[{index}]"
        if not isinstance(candidate, dict):
            raise ValueError(f"{location} must be an object")
        forbidden = sorted(FORBIDDEN_EVENT_FIELDS & set(candidate))
        if forbidden:
            raise ValueError(f"{location} contains removed field(s): {', '.join(forbidden)}")
        call = candidate.get("call")
        if not isinstance(call, str) or not call.strip():
            raise ValueError(f"{location}.call must be a non-empty string")
        args = candidate.get("args", {})
        if args is not None and not isinstance(args, dict):
            raise ValueError(f"{location}.args must be an object when present")


def validate_asset_candidates(value: Any, data_model_schema: dict[str, Any]) -> None:
    if not isinstance(value, list):
        raise ValueError("TaskSpec.assetCandidates must be an array")
    for index, asset in enumerate(value):
        location = f"TaskSpec.assetCandidates[{index}]"
        if not isinstance(asset, dict):
            raise ValueError(f"{location} must be an object")
        src = asset.get("src")
        if not isinstance(src, str) or not src.strip():
            raise ValueError(f"{location}.src must be a non-empty string")
        if not src.lower().endswith(".svg"):
            raise ValueError(f"{location}.src must reference a local .svg asset")
        description = asset.get("description")
        if not isinstance(description, str) or not description.strip():
            raise ValueError(f"{location}.description must be a non-empty string")
        bind_to = asset.get("bindTo")
        if bind_to is not None:
            if not isinstance(bind_to, str) or not bind_to.startswith("/"):
                raise ValueError(f"{location}.bindTo must be a JSON Pointer")
            if read_schema_sample(data_model_schema, bind_to) != src:
                raise ValueError(f"{location}.bindTo must resolve to the same src in dataModelSchema")


def validate_presentation_slots(
    value: Any, *, asset_count: int, event_count: int
) -> None:
    if not isinstance(value, dict):
        raise ValueError("TaskSpec.presentationSlots must be an object")
    for slot_id, slot in value.items():
        if not isinstance(slot_id, str) or not slot_id:
            raise ValueError("presentationSlots keys must be non-empty strings")
        if not isinstance(slot, dict):
            raise ValueError(f"presentationSlots.{slot_id} must be an object")
        source = slot.get("source")
        if source is not None and not isinstance(source, str):
            raise ValueError(f"presentationSlots.{slot_id}.source must be a string")
        for index_key in ("assetCandidate", "eventCandidate"):
            index = slot.get(index_key)
            if index is not None and (not isinstance(index, int) or index < 0):
                raise ValueError(f"presentationSlots.{slot_id}.{index_key} must be a non-negative integer")
            if index is not None:
                limit = asset_count if index_key == "assetCandidate" else event_count
                if index >= limit:
                    raise ValueError(
                        f"presentationSlots.{slot_id}.{index_key} must reference an existing candidate"
                    )


def read_schema_sample(schema: Any, pointer: str) -> Any:
    target = read_pointer(schema, pointer)
    if isinstance(target, dict) and "sampleValue" in target:
        return target["sampleValue"]
    return target


def read_pointer(root: Any, pointer: str) -> Any:
    if pointer in {"", "/"}:
        return root
    if not isinstance(pointer, str) or not pointer.startswith("/"):
        return None
    current = root
    for raw_part in pointer.strip("/").split("/"):
        part = raw_part.replace("~1", "/").replace("~0", "~")
        if isinstance(current, dict) and part in current:
            current = current[part]
        elif isinstance(current, list) and part.isdigit() and int(part) < len(current):
            current = current[int(part)]
        else:
            return None
    return current


def equivalent_text_units(value: str) -> float:
    units = 0.0
    for character in value:
        if character.isspace():
            units += 0.35
        elif unicodedata.east_asian_width(character) in {"W", "F"}:
            units += 1.0
        elif character.isupper():
            units += 0.68
        elif character.islower():
            units += 0.56
        elif character.isdigit():
            units += 0.62
        else:
            units += 0.45
    return round(units, 1)


def schema_field_summaries(schema: Any, pointer: str = "") -> list[dict[str, Any]]:
    """Return only scalar TaskSpec sample values that the compiler can bind."""
    fields: list[dict[str, Any]] = []
    if not isinstance(schema, dict):
        return fields
    properties = schema.get("properties")
    if isinstance(properties, dict):
        for key, child in properties.items():
            escaped = str(key).replace("~", "~0").replace("/", "~1")
            fields.extend(schema_field_summaries(child, pointer + "/" + escaped))
        return fields
    if schema.get("type") == "array" and isinstance(schema.get("items"), dict):
        fields.extend(schema_field_summaries(schema["items"], pointer + "/0"))
        return fields
    if "sampleValue" not in schema and pointer == "":
        for key, child in schema.items():
            if key in {"type", "description"}:
                continue
            escaped = str(key).replace("~", "~0").replace("/", "~1")
            fields.extend(schema_field_summaries(child, pointer + "/" + escaped))
        return fields
    if "sampleValue" not in schema:
        return fields
    sample = schema["sampleValue"]
    if sample is None or isinstance(sample, (dict, list)):
        return fields
    entry = {
        "path": pointer or "/",
        "type": schema.get("type", type(sample).__name__),
        "sample": sample,
    }
    description = schema.get("description")
    if isinstance(description, str) and description.strip():
        entry["description"] = description
    if isinstance(sample, str):
        entry["units"] = equivalent_text_units(sample)
    if isinstance(schema.get("maxLength"), int):
        entry["maxLength"] = schema["maxLength"]
    fields.append(entry)
    return fields


def case_context(spec: dict[str, Any]) -> dict[str, Any]:
    """Per-case safe reference indexes derived from the complete TaskSpec."""
    events = []
    for index, candidate in enumerate(spec.get("eventCandidates", [])):
        if not isinstance(candidate, dict):
            continue
        event: dict[str, Any] = {"index": index, "call": candidate.get("call")}
        events.append(event)
    assets = [
        {"index": index, "description": candidate.get("description")}
        for index, candidate in enumerate(spec.get("assetCandidates", []))
        if isinstance(candidate, dict)
    ]
    return {
        "bindableFields": schema_field_summaries(spec["dataModelSchema"]),
        "events": events,
        "assets": assets,
        **({"presentationSlots": spec["presentationSlots"]} if "presentationSlots" in spec else {}),
    }


def canonical_task_spec_json(spec: dict[str, Any]) -> str:
    """Embed the original TaskSpec verbatim, preserving every field."""
    return json.dumps(spec, ensure_ascii=False, indent=2)


def case_context_json(spec: dict[str, Any]) -> str:
    """Derived per-case context (safe bindings, events, assets, and slots)."""
    return json.dumps(case_context(spec), ensure_ascii=False, indent=2)


def read_user_query(path: Path | None, spec: dict[str, Any]) -> str:
    if path is None:
        return str(spec["userQuery"]).strip()
    if not path.is_file():
        raise ValueError(f"user query path must be a text file: {path}")
    text = path.read_text(encoding="utf-8-sig").strip()
    return text or str(spec["userQuery"]).strip()


def read_alt(path: Path, spec: dict[str, Any], *, validate_reference: bool = True):
    if not path.is_file():
        raise ValueError(f"ALT answer file does not exist: {path}")
    text = path.read_text(encoding="utf-8-sig").strip()
    if not text:
        raise ValueError(f"ALT answer file is empty: {path}")
    if text.startswith("```"):
        raise ValueError("ALT answer must be raw ALT text without a Markdown code fence")
    first_line = next((line.strip() for line in text.splitlines() if line.strip()), "")
    if first_line.startswith("@alt"):
        raise ValueError("ALT answer must start directly with the root node, without an @alt header")
    first_tokens = first_line.split()
    if len(first_tokens) < 2:
        raise ValueError("ALT answer must start with a root component and node ID")
    document = parse_alt(path)
    if not is_auto_layout(document):
        raise ValueError("ALT answer must use the automatic-layout protocol with root card/theme")
    if validate_reference:
        issues = validate_auto_protocol(document, str(spec["size"]))
        errors = [issue for issue in issues if issue.severity == "error"]
        if errors:
            detail = "; ".join(f"{issue.node_id}: {issue.message}" for issue in errors)
            raise ValueError(f"invalid automatic-layout ALT: {detail}")
    return text, document


def read_asc(path: Path, document, spec: dict[str, Any], *, validate_reference: bool = True) -> str:
    if not path.is_file():
        raise ValueError(f"ASC answer file does not exist: {path}")
    text = path.read_text(encoding="utf-8-sig").strip()
    parsed = parse_asc(path, document)
    if validate_reference:
        validate_auto_asc(document, spec, parsed)
        compiled, issues = auto_layout_document(document, spec, parsed)
        issues += validate_layout(compiled, str(spec["size"]))
        errors = [issue for issue in issues if issue.severity == "error"]
        if errors:
            detail = "; ".join(f"{issue.node_id}: {issue.message}" for issue in errors)
            raise ValueError(f"ALT+ASC assistant answer is not auto-layout feasible: {detail}")
    return text


def format_assistant_answer(alt_answer: str, asc_answer: str) -> str:
    return f"<alt>\n{alt_answer}\n</alt>\n<asc>\n{asc_answer}\n</asc>"


def build_request(
    spec: dict[str, Any],
    user_query: str,
    assistant_content: str,
    rejected_response: str | None = None,
) -> dict[str, Any]:
    task_spec_json = canonical_task_spec_json(spec)
    context_json = case_context_json(spec)
    static_context = build_static_context(str(spec["size"]))
    messages: list[dict[str, str]] = [
        {
            "role": "system",
            "content": (
                SYSTEM_PROMPT.replace("{{TASK_SPEC_JSON}}", task_spec_json)
                .replace("{{STATIC_CONTEXT}}", static_context)
                .replace("{{CASE_CONTEXT_JSON}}", context_json)
            ),
        },
        {
            "role": "user",
            "content": user_query,
        },
        {
            "role": "assistant",
            "content": assistant_content,
        },
    ]
    request: dict[str, Any] = {"messages": messages}
    if rejected_response is not None:
        request["rejected_response"] = rejected_response
    return request


def write_json(path: Path, payload: dict[str, Any]) -> None:
    text = json.dumps(payload, ensure_ascii=False)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build an ALT+ASC chat-completions training request from TaskSpec."
    )
    parser.add_argument("input_path", type=Path, help="Path to task.taskSpec.json.")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Write the generated request JSON to this file. Defaults to stdout.",
    )
    parser.add_argument(
        "-q",
        "--query",
        type=Path,
        help="Optional original user query text file. Defaults to TaskSpec.userQuery.",
    )
    parser.add_argument(
        "-a",
        "--alt",
        type=Path,
        help=f"ALT assistant answer. Defaults to the sibling {DEFAULT_ALT_NAME}.",
    )
    parser.add_argument(
        "-s",
        "--asc",
        type=Path,
        help=f"ASC assistant answer. Defaults to the sibling {DEFAULT_ASC_NAME}.",
    )
    parser.add_argument(
        "--hfrl",
        action="store_true",
        help="Add a top-level rejected_response for HFRL training.",
    )
    parser.add_argument(
        "--allow-layout-issues",
        action="store_true",
        help=(
            "Allow legacy/known-bad ALT+ASC files as the reference Assistant sample. "
            "Generated model answers remain strictly validated by the downstream converter."
        ),
    )
    parser.add_argument(
        "--disable_label",
        "--disable-label",
        action="store_true",
        help=(
            "Do not require the ALT/ASC label. If either answer file is missing, "
            "still write the request with an empty Assistant message."
        ),
    )
    parser.add_argument(
        "--rejected-alt",
        type=Path,
        help=f"Rejected ALT answer for --hfrl. Defaults to sibling {DEFAULT_REJECTED_ALT_NAME}.",
    )
    parser.add_argument(
        "--rejected-asc",
        type=Path,
        help=f"Rejected ASC answer for --hfrl. Defaults to sibling {DEFAULT_REJECTED_ASC_NAME}.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        spec = load_task_spec(args.input_path)
        user_query = read_user_query(args.query, spec)
        validate_reference = not args.allow_layout_issues

        assistant_content = ""
        if args.disable_label:
            alt_path = args.alt or args.input_path.parent / DEFAULT_ALT_NAME
            asc_path = args.asc or args.input_path.parent / DEFAULT_ASC_NAME
            if alt_path.is_file() and asc_path.is_file():
                alt_answer, alt_document = read_alt(
                    alt_path, spec, validate_reference=validate_reference
                )
                asc_answer = read_asc(
                    asc_path, alt_document, spec, validate_reference=validate_reference
                )
                assistant_content = format_assistant_answer(alt_answer, asc_answer)
            else:
                print(
                    f"note: label disabled and {alt_path.name} or {asc_path.name} "
                    "is missing; Assistant content will be empty",
                    file=sys.stderr,
                )
        else:
            alt_path = args.alt or args.input_path.parent / DEFAULT_ALT_NAME
            alt_answer, alt_document = read_alt(
                alt_path, spec, validate_reference=validate_reference
            )
            asc_path = args.asc or args.input_path.parent / DEFAULT_ASC_NAME
            asc_answer = read_asc(
                asc_path, alt_document, spec, validate_reference=validate_reference
            )
            assistant_content = format_assistant_answer(alt_answer, asc_answer)

        rejected_response: str | None = None
        output_path = args.output
        if args.hfrl:
            rejected_path = args.rejected_alt or args.input_path.parent / DEFAULT_REJECTED_ALT_NAME
            rejected_asc_path = args.rejected_asc or args.input_path.parent / DEFAULT_REJECTED_ASC_NAME
            if rejected_path.is_file() and rejected_asc_path.is_file():
                rejected_alt, rejected_document = read_alt(
                    rejected_path, spec, validate_reference=validate_reference
                )
                rejected_asc = read_asc(
                    rejected_asc_path,
                    rejected_document,
                    spec,
                    validate_reference=validate_reference,
                )
                rejected_response = format_assistant_answer(rejected_alt, rejected_asc)
            elif args.disable_label:
                print(
                    f"note: label disabled and {rejected_path.name} or {rejected_asc_path.name} "
                    "is missing; rejected_response will be omitted",
                    file=sys.stderr,
                )
            else:
                print(
                    f"skip: rejected ALT/ASC file does not exist: {rejected_path}, {rejected_asc_path}",
                    file=sys.stderr,
                )
                return 0
            if output_path is None:
                output_path = args.input_path.parent / "task.request.hfrl.json"

        request = build_request(spec, user_query, assistant_content, rejected_response)
        if output_path is not None:
            write_json(output_path, request)
        else:
            print(json.dumps(request, ensure_ascii=False, indent=2))
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
