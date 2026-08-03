from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

import alt_converter as converter
import taskspec_to_alt_chat_completions as request_builder


def task_spec(size: str = "2x2", asset_src: str = "resources/base/media/battery_leaf_fill.svg") -> dict:
    return {
        "userQuery": "显示低电状态并提供省电操作",
        "size": size,
        "dataModelSchema": {
            "type": "object",
            "properties": {
                "battery": {
                    "type": "object",
                    "properties": {
                        "levelText": {
                            "type": "string",
                            "description": "当前剩余电量文案",
                            "sampleValue": "剩余 20%",
                        }
                    },
                }
            },
        },
        "eventCandidates": [
            {
                "call": "clickToIntent",
                "args": {
                    "intentName": "SetSettingSwitch",
                    "params": {
                        "appBundleName": "com.huawei.hmos.settings",
                        "itemName": "battery_saving_mode",
                        "switchFlag": 0,
                    },
                },
            }
        ],
        "assetCandidates": [
            {"src": asset_src, "description": "电池与绿叶省电图标"}
        ],
    }


def automatic_document() -> converter.AltDocument:
    root = converter.AltNode("Column", "root", {"card": "2x2", "theme": "neutral-light"})
    header = converter.AltNode("Row", "header", {})
    header.children = [
        converter.AltNode("Image", "battery_icon", {"role": "asset"}),
        converter.AltNode("Text", "title", {"role": "title"}),
    ]
    root.children = [
        header,
        converter.AltNode("Text", "battery_value", {"role": "primary"}),
        converter.AltNode("Button", "action_button", {"role": "action"}),
    ]
    return converter.AltDocument(root)


class AutomaticAltTest(unittest.TestCase):
    def test_auto_layout_compiles_button_and_theme_colored_svg(self) -> None:
        document = automatic_document()
        spec = task_spec()
        asc = {
            "battery_icon": {"asset": 0},
            "title": {"text": "低电模式"},
            "battery_value": {"bind": "/battery/levelText"},
            "action_button": {"label": "立即省电", "event": 0},
        }

        compiled, issues = converter.auto_layout_document(document, spec, asc)
        issues += converter.validate_layout(compiled, "2x2")
        self.assertFalse([issue for issue in issues if issue.severity == "error"])
        base_text, _ = converter.alt_to_dsl(
            compiled,
            spec,
            strict_layout=False,
            validate_content=False,
        )
        dsl_text = converter.apply_asc_to_dsl(base_text, document, spec, asc)
        _, update, _ = converter.parse_jsonl_messages(dsl_text)
        components = {
            item["id"]: item for item in update["updateComponents"]["components"]
        }

        image = components["battery_icon"]
        self.assertEqual(image["src"], spec["assetCandidates"][0]["src"])
        self.assertEqual(image["styles"]["objectFit"], "contain")
        self.assertEqual(
            image["styles"]["fillColor"],
            converter.THEME_CONFIG["themes"]["neutral-light"]["icon"]["primary"],
        )

        button = components["action_button"]
        styles = button["styles"]
        self.assertNotIn("padding", styles)
        self.assertGreaterEqual(styles["height"], 32)
        required_width = converter.estimated_text_width(
            button["label"], styles["fontSize"], styles["fontWeight"], 1.08
        ) + 28
        self.assertGreaterEqual(styles["width"], required_width)

    def test_compiled_svg_fill_color_passes_full_skill_validator(self) -> None:
        spec = task_spec()
        asc = {
            "battery_icon": {"asset": 0},
            "title": {"text": "低电模式"},
            "battery_value": {"bind": "/battery/levelText"},
            "action_button": {"label": "立即省电", "event": 0},
        }
        validator = (
            SCRIPTS_DIR.parent
            / ".agents"
            / "skills"
            / "harmony-card-generation-datamodel-first"
            / "scripts"
            / "validate_card.py"
        )
        with tempfile.TemporaryDirectory() as temporary:
            case = Path(temporary)
            cardspec_path = case / "card.cardspec.json"
            cardspec_path.write_text(
                json.dumps(
                    {
                        "title": "低电模式",
                        "description": "显示低电状态并提供省电操作",
                        "suggestSize": "2x2",
                    },
                    ensure_ascii=False,
                ),
                encoding="utf-8",
            )
            for theme_name in sorted(converter.AUTO_THEMES):
                with self.subTest(theme=theme_name):
                    document = automatic_document()
                    document.root.attrs["theme"] = theme_name
                    compiled, issues = converter.auto_layout_document(document, spec, asc)
                    issues += converter.validate_layout(compiled, "2x2")
                    self.assertFalse(
                        [issue for issue in issues if issue.severity == "error"]
                    )
                    base_text, _ = converter.alt_to_dsl(
                        compiled,
                        spec,
                        strict_layout=False,
                        validate_content=False,
                    )
                    dsl_text = converter.apply_asc_to_dsl(
                        base_text, document, spec, asc
                    )
                    dsl_path = case / f"card.{theme_name}.dsl.jsonl"
                    dsl_path.write_text(dsl_text, encoding="utf-8")
                    result = subprocess.run(
                        [
                            sys.executable,
                            "-B",
                            str(validator),
                            "--dsl",
                            str(dsl_path),
                            "--cardspec",
                            str(cardspec_path),
                            "--stage",
                            "all",
                        ],
                        cwd=SCRIPTS_DIR.parent,
                        capture_output=True,
                        text=True,
                        encoding="utf-8",
                        errors="replace",
                        check=False,
                    )
                    self.assertEqual(
                        result.returncode, 0, result.stdout + result.stderr
                    )

    def test_2x2_rejects_checkbox(self) -> None:
        root = converter.AltNode(
            "Column", "root", {"card": "2x2", "theme": "neutral-light"}
        )
        root.children = [
            converter.AltNode("Checkbox", "power_toggle", {"role": "selection"})
        ]
        issues = converter.validate_auto_protocol(converter.AltDocument(root), "2x2")
        self.assertTrue(
            any("Checkbox nodes" in issue.message and issue.severity == "error" for issue in issues)
        )

    def test_2x4_checkbox_uses_fixed_native_outer_height(self) -> None:
        root = converter.AltNode(
            "Column", "root", {"card": "2x4", "theme": "ambient-light"}
        )
        root.children = [
            converter.AltNode("Text", "title", {"role": "title"}),
            converter.AltNode("Checkbox", "power_toggle", {"role": "selection"}),
            converter.AltNode("Button", "action_button", {"role": "action"}),
        ]
        document = converter.AltDocument(root)
        spec = task_spec(size="2x4")
        asc = {
            "title": {"text": "省电设置"},
            "power_toggle": {"label": "降低刷新率", "select": True},
            "action_button": {"label": "保存设置", "event": 0},
        }

        compiled, issues = converter.auto_layout_document(document, spec, asc)
        issues += converter.validate_layout(compiled, "2x4")
        self.assertFalse([issue for issue in issues if issue.severity == "error"])
        checkbox = next(
            node for node in converter.alt_nodes(compiled) if node.node_id == "power_toggle"
        )
        _, height = converter.parse_box(checkbox.attrs["box"])
        self.assertEqual(height, 48)
        self.assertNotIn("font", checkbox.attrs)

    def test_auto_layout_rejects_png_asset(self) -> None:
        document = automatic_document()
        spec = task_spec(asset_src="resources/base/media/icon_electricity.png")
        asc = {
            "battery_icon": {"asset": 0},
            "title": {"text": "低电模式"},
            "battery_value": {"text": "剩余 20%"},
            "action_button": {"label": "立即省电", "event": 0},
        }
        _, issues = converter.auto_layout_document(document, spec, asc)
        self.assertTrue(
            any("local .svg" in issue.message and issue.severity == "error" for issue in issues)
        )

    def test_visible_text_capacity_is_a_hard_error(self) -> None:
        document = automatic_document()
        spec = task_spec()
        asc = {
            "battery_icon": {"asset": 0},
            "title": {"text": "这是一个明显超过二乘二卡片容量限制的超长标题文本"},
            "battery_value": {"text": "这是另一个无法安全排版的超长主状态文本"},
            "action_button": {"label": "执行一个非常非常长的操作", "event": 0},
        }
        _, issues = converter.auto_layout_document(document, spec, asc)
        self.assertTrue(
            any("visible text budget" in issue.message and issue.severity == "error" for issue in issues)
        )

    def test_legacy_geometry_document_remains_readable_but_is_not_auto(self) -> None:
        root = converter.AltNode(
            "Column",
            "root",
            {
                "box": "140x140",
                "pad": 12,
                "radius": 18,
                "clip": True,
                "bg": "#FFFFFFFF",
            },
        )
        root.children = [
            converter.AltNode(
                "Text",
                "title",
                {"box": "116x20", "font": "14/600", "lines": 1, "role": "title"},
            )
        ]
        document = converter.AltDocument(root)
        self.assertFalse(converter.is_auto_layout(document))

        migrated = converter.simplify_to_auto(document, "2x2")
        self.assertTrue(converter.is_auto_layout(migrated))
        self.assertEqual(migrated.root.attrs["theme"], "neutral-light")
        self.assertEqual(migrated.root.children[0].attrs, {"role": "title"})

    def test_cli_writes_report_and_preserves_dsl_when_auto_layout_is_infeasible(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            dataset = Path(temporary)
            case = dataset / "Case-overflow"
            case.mkdir()
            spec = task_spec()
            (case / "task.taskSpec.json").write_text(
                json.dumps(spec, ensure_ascii=False), encoding="utf-8"
            )
            (case / "card.alt.txt").write_text(
                converter.serialize_alt(automatic_document()), encoding="utf-8"
            )
            (case / "card.asc.txt").write_text(
                "\n".join(
                    [
                        "Image battery_icon asset=0",
                        "Text title text=这是一个明显超过二乘二卡片容量限制的超长标题文本",
                        "Text battery_value text=这是另一个无法安全排版的超长主状态文本",
                        "Button action_button label=执行一个非常非常长的操作 event=0",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            dsl_path = case / "card.output.dsl.jsonl"
            dsl_path.write_text("sentinel\n", encoding="utf-8")

            argv = [
                "alt_converter.py",
                "-o",
                str(dataset),
                "--mode",
                "t2d",
                "--dsl_name",
                dsl_path.name,
            ]
            with mock.patch.object(sys, "argv", argv):
                result = converter.main()

            self.assertEqual(result, 1)
            self.assertEqual(dsl_path.read_text(encoding="utf-8"), "sentinel\n")
            report = json.loads(
                (case / "card.layout-report.txt").read_text(encoding="utf-8")
            )
            self.assertGreater(report["summary"]["errors"], 0)

    def test_cli_compiles_feasible_auto_case_and_writes_pass_report(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            dataset = Path(temporary)
            case = dataset / "Case-valid"
            case.mkdir()
            spec = task_spec()
            (case / "task.taskSpec.json").write_text(
                json.dumps(spec, ensure_ascii=False), encoding="utf-8"
            )
            (case / "card.alt.txt").write_text(
                converter.serialize_alt(automatic_document()), encoding="utf-8"
            )
            (case / "card.asc.txt").write_text(
                "\n".join(
                    [
                        "Image battery_icon asset=0",
                        "Text title text=低电模式",
                        "Text battery_value bind=/battery/levelText",
                        "Button action_button label=立即省电 event=0",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            argv = ["alt_converter.py", "-o", str(dataset), "--mode", "t2d"]
            with mock.patch.object(sys, "argv", argv):
                result = converter.main()

            self.assertEqual(result, 0)
            _, update, _ = converter.read_jsonl_messages(case / "card.dsl.jsonl")
            image = next(
                item
                for item in update["updateComponents"]["components"]
                if item["id"] == "battery_icon"
            )
            self.assertEqual(image["styles"]["fillColor"], "#E5000000")
            report = json.loads(
                (case / "card.layout-report.txt").read_text(encoding="utf-8")
            )
            self.assertEqual(report["status"], "pass")
            self.assertEqual(report["summary"]["errors"], 0)

    def test_training_request_uses_planning_spec_and_feasible_alt_asc(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            case = Path(temporary)
            spec = task_spec()
            task_path = case / "task.taskSpec.json"
            alt_path = case / "card.alt.txt"
            asc_path = case / "card.asc.txt"
            task_path.write_text(json.dumps(spec, ensure_ascii=False), encoding="utf-8")
            alt_path.write_text(
                converter.serialize_alt(automatic_document()), encoding="utf-8"
            )
            asc_path.write_text(
                "\n".join(
                    [
                        "Image battery_icon asset=0",
                        "Text title text=低电模式",
                        "Text battery_value bind=/battery/levelText",
                        "Button action_button label=立即省电 event=0",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            loaded = request_builder.load_task_spec(task_path)
            alt_text, document = request_builder.read_alt(alt_path, loaded)
            asc_text = request_builder.read_asc(asc_path, document, loaded)
            request = request_builder.build_request(
                loaded, loaded["userQuery"], alt_text, asc_text
            )

            system = request["messages"][0]["content"]
            assistant = request["messages"][2]["content"]
            self.assertIn('"themes": [', system)
            self.assertNotIn(spec["assetCandidates"][0]["src"], system)
            self.assertNotIn("<think>", assistant)
            self.assertTrue(assistant.startswith("<alt>\n"))
            self.assertIn("<alt>\nColumn root card=2x2 theme=neutral-light", assistant)
            self.assertIn("<asc>\nImage battery_icon asset=0", assistant)


if __name__ == "__main__":
    unittest.main()
