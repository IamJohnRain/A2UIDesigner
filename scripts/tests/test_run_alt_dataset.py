from __future__ import annotations

import argparse
import asyncio
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

import run_alt_dataset as runner


def make_spec() -> dict:
    return {
        "userQuery": "显示低电状态并提供省电操作",
        "size": "2x2",
        "dataModelSchema": {
            "type": "object",
            "properties": {
                "battery": {
                    "type": "object",
                    "properties": {
                        "levelText": {
                            "type": "string",
                            "description": "剩余电量",
                            "sampleValue": "剩余 20%",
                        }
                    },
                }
            },
        },
        "eventCandidates": [
            {"call": "clickToIntent", "args": {"intentName": "SetSettingSwitch"}}
        ],
        "assetCandidates": [
            {
                "src": "resources/base/media/battery_leaf_fill.svg",
                "description": "电池省电图标",
            }
        ],
    }


def new_style_system(spec: dict) -> str:
    context = {"fields": [], "events": [], "assets": []}
    return (
        "你是 A2UI 卡片结构规划模型。\n"
        "TaskSpec（原样嵌入，禁止改写、删除或推导任何字段）：\n"
        + json.dumps(spec, ensure_ascii=False, indent=2)
        + "\n\n协议约束：\n- 主题白名单：neutral-light\n\n"
        "本卡上下文 CASE_CONTEXT（脚本按本 TaskSpec 推导）：\n"
        + json.dumps(context, ensure_ascii=False, indent=2)
    )


def make_args(force: bool) -> argparse.Namespace:
    return argparse.Namespace(
        force=force,
        debug=False,
        retry=0,
        retry_base_seconds=0.0,
        request_timeout=1.0,
    )


def make_case(index: int = 1) -> runner.CaseItem:
    spec = make_spec()
    system = new_style_system(spec)
    return runner.CaseItem(
        line_no=1,
        index=index,
        query="显示低电状态并提供省电操作",
        system_content=system,
        messages=[
            {"role": "system", "content": system},
            {"role": "user", "content": "显示低电状态并提供省电操作"},
        ],
    )


async def _fake_call_model(env, messages, request_timeout):
    return (
        '<alt>\nColumn root card=2x2 theme=neutral-light\n</alt>\n'
        '<asc>\nText title text=低电模式\n</asc>',
        {"choices": [{"message": {"content": "ok"}}]},
    )


async def _run_case(case, args, output_dir):
    env = runner.EnvConfig(
        label="test",
        api_url="http://127.0.0.1:4000/v1/chat/completions",
        api_key="k",
        model="m",
        base_url="http://127.0.0.1:4000",
        extra_body={},
    )
    sem = asyncio.Semaphore(1)
    tracker = runner._InFlight()
    tracker.configure(1, 1)
    return await runner.run_case(case, args, env, output_dir, sem, tracker)


class ExtractTaskSpecTest(unittest.TestCase):
    def test_new_style_marker_extracts_original_spec(self) -> None:
        spec = make_spec()
        extracted = runner.extract_taskspec_from_system(new_style_system(spec))
        self.assertEqual(extracted, spec)

    def test_marker_wins_over_later_taskspec_like_json(self) -> None:
        spec = make_spec()
        later = dict(spec)
        later["userQuery"] = "后面的假 TaskSpec"
        system = (
            "TaskSpec（原样嵌入，禁止改写、删除或推导任何字段）：\n"
            + json.dumps(spec, ensure_ascii=False)
            + "\n\n其它内容：\n"
            + json.dumps(later, ensure_ascii=False)
        )
        extracted = runner.extract_taskspec_from_system(system)
        self.assertEqual(extracted["userQuery"], spec["userQuery"])

    def test_legacy_fallback_takes_last_full_taskspec(self) -> None:
        spec = make_spec()
        other = {"userQuery": "旧格式", "size": "2x4"}
        system = json.dumps(other, ensure_ascii=False) + "\n正文\n" + json.dumps(
            spec, ensure_ascii=False
        )
        extracted = runner.extract_taskspec_from_system(system)
        self.assertEqual(extracted, spec)

    def test_missing_taskspec_raises(self) -> None:
        with self.assertRaises(ValueError):
            runner.extract_taskspec_from_system("没有任何 JSON")

    def test_incomplete_planning_spec_is_rejected(self) -> None:
        system = json.dumps(
            {"userQuery": "q", "size": "2x2", "themes": [], "fields": []},
            ensure_ascii=False,
        )
        with self.assertRaises(ValueError):
            runner.extract_taskspec_from_system(system)


class RunCaseTaskSpecTest(unittest.TestCase):
    def test_run_case_preserves_existing_taskspec(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output_dir = Path(temporary)
            case = make_case()
            case_dir = output_dir / case.directory_name
            case_dir.mkdir()
            sentinel = {"sentinel": True}
            (case_dir / runner.TASKSPEC_FILE_NAME).write_text(
                json.dumps(sentinel, ensure_ascii=False), encoding="utf-8"
            )
            with mock.patch.object(runner, "call_model", new=_fake_call_model):
                result = asyncio.run(_run_case(case, make_args(force=False), output_dir))
            self.assertTrue(result.ok, result.error)
            saved = json.loads(
                (case_dir / runner.TASKSPEC_FILE_NAME).read_text(encoding="utf-8")
            )
            self.assertEqual(saved, sentinel)
            self.assertTrue((case_dir / runner.ALT_FILE_NAME).is_file())
            self.assertTrue((case_dir / runner.ASC_FILE_NAME).is_file())

    def test_run_case_force_overwrites_taskspec(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output_dir = Path(temporary)
            case = make_case()
            case_dir = output_dir / case.directory_name
            case_dir.mkdir()
            sentinel = {"sentinel": True}
            (case_dir / runner.TASKSPEC_FILE_NAME).write_text(
                json.dumps(sentinel, ensure_ascii=False), encoding="utf-8"
            )
            with mock.patch.object(runner, "call_model", new=_fake_call_model):
                result = asyncio.run(_run_case(case, make_args(force=True), output_dir))
            self.assertTrue(result.ok, result.error)
            saved = json.loads(
                (case_dir / runner.TASKSPEC_FILE_NAME).read_text(encoding="utf-8")
            )
            self.assertEqual(saved, make_spec())


if __name__ == "__main__":
    unittest.main()
