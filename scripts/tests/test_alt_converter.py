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
    def test_composite_text_expression_resolves_all_scalar_references(self) -> None:
        data_model = {
            "data": {
                "calendar": {
                    "items": [{"dtStart": "14:00", "dtEnd": "15:00"}]
                }
            }
        }
        sample, references, error = converter.resolve_text_expression(
            "{{ ${/data/calendar/items/0/dtStart} + '-' + ${/data/calendar/items/0/dtEnd} }}",
            data_model,
        )
        self.assertIsNone(error)
        self.assertEqual(sample, "14:00-15:00")
        self.assertEqual(
            references,
            [
                "/data/calendar/items/0/dtStart",
                "/data/calendar/items/0/dtEnd",
            ],
        )

    def test_composite_text_expression_uses_real_sample_for_font_adaptation(self) -> None:
        spec = task_spec()
        spec["dataModelSchema"] = {
            "type": "object",
            "properties": {
                "data": {
                    "type": "object",
                    "properties": {
                        "calendar": {
                            "type": "object",
                            "properties": {
                                "items": {
                                    "type": "array",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "dtStart": {"type": "string", "sampleValue": "14:00"},
                                            "dtEnd": {"type": "string", "sampleValue": "15:00"},
                                        },
                                    },
                                },
                            },
                        },
                    },
                }
            },
        }
        document = converter.AltDocument(
            converter.AltNode(
                "Column",
                "root",
                {"card": "2x2", "theme": "neutral-light"},
                [converter.AltNode("Text", "schedule_time", {"role": "metric"})],
            )
        )
        expression = (
            "{{ ${/data/calendar/items/0/dtStart} + '-' + "
            "${/data/calendar/items/0/dtEnd} }}"
        )
        asc = {"schedule_time": {"expr": expression}}

        converter.validate_auto_asc(document, spec, asc)
        compiled, issues = converter.auto_layout_document(document, spec, asc)
        self.assertFalse([issue for issue in issues if issue.severity == "error"])
        self.assertFalse(
            [issue for issue in issues if "semantic fallback" in issue.message]
        )
        node = next(
            item for item in converter.alt_nodes(compiled) if item.node_id == "schedule_time"
        )
        self.assertEqual(node.attrs["font"], "16/500")
        self.assertGreaterEqual(converter.parse_box(node.attrs["box"])[0], 113)

        base_text, _ = converter.alt_to_dsl(
            compiled, spec, strict_layout=False, validate_content=False
        )
        dsl_text = converter.apply_asc_to_dsl(base_text, document, spec, asc)
        _, update, _ = converter.parse_jsonl_messages(dsl_text)
        output = next(
            item
            for item in update["updateComponents"]["components"]
            if item["id"] == "schedule_time"
        )
        self.assertEqual(output["content"], expression)

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

        root = components["root"]
        self.assertEqual(
            root["styles"]["linearGradient"],
            converter.THEME_CONFIG["themes"]["neutral-light"]["surface"]["gradient"],
        )
        self.assertEqual(root["styles"]["borderWidth"], 1)

        button = components["action_button"]
        styles = button["styles"]
        self.assertNotIn("padding", styles)
        self.assertEqual(
            styles["linearGradient"],
            converter.THEME_CONFIG["themes"]["neutral-light"]["action"]["primaryGradient"],
        )
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

    def test_2x4_checkbox_uses_compact_automatic_outer_height(self) -> None:
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
        spec["dataModelSchema"]["properties"]["battery"]["properties"]["enabled"] = {
            "type": "boolean",
            "description": "省电状态",
            "sampleValue": True,
        }
        asc = {
            "title": {"text": "省电设置"},
            "power_toggle": {"label": "降低刷新率", "bind": "/battery/enabled"},
            "action_button": {"label": "保存设置", "event": 0},
        }

        compiled, issues = converter.auto_layout_document(document, spec, asc)
        issues += converter.validate_layout(compiled, "2x4")
        self.assertFalse([issue for issue in issues if issue.severity == "error"])
        checkbox = next(
            node for node in converter.alt_nodes(compiled) if node.node_id == "power_toggle"
        )
        _, height = converter.parse_box(checkbox.attrs["box"])
        self.assertEqual(height, 22)
        self.assertNotIn("font", checkbox.attrs)

    def test_auto_checkbox_resolves_dynamic_label_and_boolean_state(self) -> None:
        spec = task_spec(size="2x4")
        spec["dataModelSchema"]["properties"]["battery"]["properties"].update(
            {
                "enabled": {
                    "type": "boolean",
                    "description": "省电状态",
                    "sampleValue": True,
                },
                "label": {
                    "type": "string",
                    "description": "设置名称",
                    "sampleValue": "省电模式",
                },
            }
        )
        root = converter.AltNode(
            "Column", "root", {"card": "2x4", "theme": "neutral-light"}
        )
        root.children = [
            converter.AltNode("Checkbox", "power_toggle", {"role": "selection"})
        ]
        document = converter.AltDocument(root)
        asc = {
            "power_toggle": {
                "label": "{{ ${/battery/label} }}",
                "bind": "/battery/enabled",
            }
        }
        converter.validate_auto_asc(document, spec, asc)
        compiled, issues = converter.auto_layout_document(document, spec, asc)
        self.assertFalse([issue for issue in issues if issue.severity == "error"])
        checkbox = next(
            node for node in converter.alt_nodes(compiled) if node.node_id == "power_toggle"
        )
        self.assertEqual(converter.parse_box(checkbox.attrs["box"])[1], 22)
        self.assertNotIn("semantic fallback", " ".join(issue.message for issue in issues))

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

    def test_nested_named_groups_are_exposed_as_bindable_fields(self) -> None:
        spec = task_spec()
        spec["dataModelSchema"] = {
            "guard": {
                "title": {
                    "type": "string",
                    "description": "卡片标题",
                    "sampleValue": "防沉迷",
                }
            }
        }

        fields = request_builder.case_context(spec)["bindableFields"]

        self.assertEqual([field["path"] for field in fields], ["/guard/title"])

    def test_2x4_row_rounding_preserves_integer_width_budget(self) -> None:
        document = converter.AltDocument(
            converter.AltNode(
                "Column",
                "root",
                {"card": "2x4", "theme": "focus-dark"},
            )
        )
        body = converter.AltNode("Row", "body", {})
        left = converter.AltNode("Column", "left_group", {})
        left.children = [
            converter.AltNode("Text", "title", {"role": "title"}),
            converter.AltNode("Column", "items", {}),
        ]
        left.children[1].children = [
            converter.AltNode("Text", "item_one", {"role": "selection"}),
            converter.AltNode("Text", "item_two", {"role": "selection"}),
            converter.AltNode("Text", "item_three", {"role": "selection"}),
        ]
        right = converter.AltNode("Column", "right_group", {})
        right.children = [
            converter.AltNode("Text", "warning", {"role": "warning"}),
            converter.AltNode("Text", "count", {"role": "primary"}),
            converter.AltNode("Button", "open_app", {"role": "action"}),
        ]
        body.children = [left, right]
        document.root.children = [body]
        spec = task_spec(size="2x4")
        asc = {
            "title": {"text": "请选择项目"},
            "item_one": {"text": "项目一"},
            "item_two": {"text": "项目二"},
            "item_three": {"text": "项目三"},
            "warning": {"text": "危险提示"},
            "count": {"text": "数量变化"},
            "open_app": {"label": "打开应用", "event": 0},
        }

        compiled, issues = converter.auto_layout_document(document, spec, asc)
        issues += converter.validate_layout(compiled, "2x4")

        self.assertFalse(
            [issue for issue in issues if issue.severity == "error"],
            [issue.message for issue in issues],
        )
        row = next(node for node in converter.alt_nodes(compiled) if node.node_id == "body")
        child_widths = [converter.parse_box(child.attrs["box"])[0] for child in row.children]
        self.assertLessEqual(sum(child_widths) + int(row.attrs["gap"]), 276)

    def test_warning_role_uses_theme_warning_color(self) -> None:
        document = converter.AltDocument(
            converter.AltNode(
                "Column",
                "root",
                {"card": "2x2", "theme": "focus-dark"},
            )
        )
        document.root.children = [
            converter.AltNode("Text", "warning", {"role": "warning"}),
            converter.AltNode("Button", "action_button", {"role": "action"}),
        ]
        spec = task_spec()
        asc = {
            "warning": {"text": "超时风险"},
            "action_button": {"label": "去设置", "event": 0},
        }

        compiled, issues = converter.auto_layout_document(document, spec, asc)
        issues += converter.validate_layout(compiled, "2x2")

        self.assertFalse([issue for issue in issues if issue.severity == "error"])
        warning = next(node for node in converter.alt_nodes(compiled) if node.node_id == "warning")
        self.assertEqual(
            warning.attrs["fg"],
            converter.THEME_CONFIG["themes"]["focus-dark"]["status"]["warning"],
        )

    def test_button_data_model_binding_compiles_to_dynamic_label(self) -> None:
        document = converter.AltDocument(
            converter.AltNode(
                "Column",
                "root",
                {"card": "2x2", "theme": "neutral-light"},
            )
        )
        document.root.children = [
            converter.AltNode("Button", "action_button", {"role": "action"})
        ]
        asc = {"action_button": {"bind": "/battery/levelText", "event": 0}}
        converter.validate_auto_asc(document, task_spec(), asc)
        compiled, issues = converter.auto_layout_document(document, task_spec(), asc)
        issues += converter.validate_layout(compiled, "2x2")
        self.assertFalse([issue for issue in issues if issue.severity == "error"])
        base_text, _ = converter.alt_to_dsl(
            compiled, task_spec(), strict_layout=False, validate_content=False
        )
        dsl_text = converter.apply_asc_to_dsl(base_text, document, task_spec(), asc)
        _, update, _ = converter.parse_jsonl_messages(dsl_text)
        button = next(
            item for item in update["updateComponents"]["components"] if item["id"] == "action_button"
        )
        self.assertEqual(button["label"], "{{ ${/battery/levelText} }}")

    def test_auto_text_defaults_to_center_alignment(self) -> None:
        top, styles = converter.apply_alt_styles(
            converter.AltNode("Text", "title", {"role": "title"})
        )
        self.assertEqual(top["id"], "title")
        self.assertEqual(styles["textAlign"], "center")

        _, explicit_styles = converter.apply_alt_styles(
            converter.AltNode("Text", "title", {"text": "start"})
        )
        self.assertEqual(explicit_styles["textAlign"], "start")

    def test_static_semantic_text_is_repaired_to_data_model_binding(self) -> None:
        document = automatic_document()
        asc = {
            "battery_icon": {"asset": 0},
            "title": {"text": "电量状态"},
            "battery_value": {"text": "鍓╀綑 20%"},
            "action_button": {"label": "绔嬪嵆鐪佺數", "event": 0},
        }
        normalized, notes = converter.normalize_auto_bindings(document, task_spec(), asc)
        self.assertIn("battery_value", normalized)
        self.assertEqual(normalized["battery_value"]["bind"], "/battery/levelText")
        self.assertNotIn("text", normalized["battery_value"])
        self.assertTrue(any("battery_value" in note for note in notes))

    def test_excess_children_are_grouped_and_long_protected_text_is_downgraded(self) -> None:
        document = automatic_document()
        document.root.children.insert(
            1, converter.AltNode("Text", "warning", {"role": "status"})
        )
        asc = {
            "battery_icon": {"asset": 0},
            "title": {"text": "标题"},
            "battery_value": {"text": "剩余 20%"},
            "warning": {"text": "这是一个很长的状态说明，需要动态展示"},
            "action_button": {"label": "打开", "event": 0},
        }
        repaired, structure_notes = converter.repair_auto_structure(document)
        role_notes = converter.repair_long_auto_text_roles(repaired, task_spec(), asc)
        self.assertLessEqual(len(repaired.root.children), 3)
        self.assertTrue(structure_notes)
        self.assertEqual(
            next(node for node in converter.alt_nodes(repaired) if node.node_id == "warning").attrs["role"],
            "support",
        )
        self.assertTrue(role_notes)

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
                loaded,
                loaded["userQuery"],
                request_builder.format_assistant_answer(alt_text, asc_text),
            )

            system = request["messages"][0]["content"]
            user = request["messages"][1]["content"]
            assistant = request["messages"][2]["content"]
            self.assertEqual(request["taskSpec"], spec)
            self.assertEqual(user, spec["userQuery"])

            # TASK_CONTEXT contains metadata only; raw candidate objects stay
            # outside the model prompt.
            task_marker = "TASK_CONTEXT（任务元数据，不含原始事件对象和素材路径）："
            task_start = system.index(task_marker) + len(task_marker)
            task_context, _ = json.JSONDecoder().raw_decode(
                system[system.index("{", task_start) :]
            )
            self.assertEqual(task_context["cardSize"], "2x2")
            self.assertNotIn(spec["userQuery"], system)
            self.assertNotIn(spec["assetCandidates"][0]["src"], system)
            self.assertNotIn('"intentName"', system)

            policy_marker = "POLICY_CONTEXT（由 profile 和自动转换能力唯一生成）："
            policy_start = system.index(policy_marker) + len(policy_marker)
            policy, _ = json.JSONDecoder().raw_decode(
                system[system.index("{", policy_start) :]
            )
            self.assertEqual(policy["card"]["orientation"], "square")
            self.assertEqual(policy["hardProtocol"]["rootComponent"], "Column")
            self.assertFalse(policy["hardProtocol"]["rootRowAllowed"])
            self.assertEqual(policy["axes"]["Row"], "horizontal")
            self.assertFalse(policy["limits"]["maxCheckbox"])
            self.assertFalse(policy["limits"]["maxLists"])

            # REFERENCE_CONTEXT is the only model-facing reference index.
            context = request_builder.case_context(loaded)
            fields = context["bindableFields"]
            self.assertEqual(len(fields), 1)
            self.assertEqual(fields[0]["path"], "/battery/levelText")
            self.assertEqual(fields[0]["description"], "当前剩余电量文案")
            self.assertNotIn("args", context["events"][0])
            self.assertNotIn("format", context["assets"][0])
            self.assertIn("REFERENCE_CONTEXT（唯一合法的绑定、素材和事件索引入口）：", system)
            self.assertIn('"bindableFields": [', system)
            self.assertIn("neutral-light", system)
            self.assertIn("严格正例", system)
            self.assertIn('"orientation": "square"', system)
            self.assertIn("Row 横向分配空间", system)
            self.assertNotIn("Checkbox：label", system)
            self.assertNotIn("maxVisibleListItems", system)
            for unresolved in (
                "{{TASK_CONTEXT_JSON}}",
                "{{POLICY_CONTEXT_JSON}}",
                "{{REFERENCE_CONTEXT_JSON}}",
            ):
                self.assertNotIn(unresolved, system)
            self.assertNotIn("<think>", assistant)
            self.assertTrue(assistant.startswith("<alt>\n"))
            self.assertIn("<alt>\nColumn root card=2x2 theme=neutral-light", assistant)
            self.assertIn("<asc>\nImage battery_icon asset=0", assistant)

    def test_2x4_policy_uses_horizontal_groups_without_changing_root_protocol(self) -> None:
        spec = task_spec(size="2x4")
        request = request_builder.build_request(spec, "展示横向设备状态卡片", "")
        system = request["messages"][0]["content"]
        marker = "POLICY_CONTEXT（由 profile 和自动转换能力唯一生成）："
        start = system.index(marker) + len(marker)
        policy, _ = json.JSONDecoder().raw_decode(system[system.index("{", start) :])

        self.assertEqual(policy["card"]["orientation"], "landscape")
        self.assertEqual(policy["card"]["canvas"], {"width": 300.0, "height": 140.0})
        self.assertEqual(policy["hardProtocol"]["rootComponent"], "Column")
        self.assertTrue(policy["axes"]["rowChildrenMayBeContainers"])
        self.assertIn("Row", policy["hardProtocol"]["allowedComponents"])
        self.assertIn("Column", policy["hardProtocol"]["allowedComponents"])
        self.assertIn("selection cards", policy["shapeStrategy"])
        self.assertIn("Column primary_group", system)
        self.assertEqual(policy["textRules"]["maxLines"]["status"], 1)
        self.assertIn("status", policy["textRules"]["protectedSingleLineRoles"])
        self.assertTrue(policy["capabilities"]["selection"]["interactive"])
        self.assertEqual(policy["capabilities"]["selection"]["mode"], "checkbox")
        self.assertIn("Checkbox", policy["hardProtocol"]["allowedComponents"])
        self.assertNotIn("Checkbox", policy["hardProtocol"]["forbiddenComponents"])
        self.assertEqual(policy["limits"]["maxCheckbox"], 3)

    def test_case_context_excludes_null_and_non_scalar_bindings(self) -> None:
        spec = task_spec()
        spec["presentationSlots"] = {
            "battery_icon": {
                "source": "resources/base/media/battery_leaf_fill.svg",
                "assetCandidate": 0,
            }
        }
        spec["dataModelSchema"]["properties"]["pending"] = {
            "type": "string",
            "description": "尚未提供的状态文案",
            "sampleValue": None,
        }
        spec["dataModelSchema"]["properties"]["summary"] = {
            "type": "object",
            "description": "不允许整体绑定的摘要对象",
            "sampleValue": {"text": "剩余 20%"},
        }

        context = request_builder.case_context(spec)

        self.assertEqual(
            [field["path"] for field in context["bindableFields"]],
            ["/battery/levelText"],
        )
        self.assertTrue(
            all("bindable" not in field for field in context["bindableFields"])
        )
        self.assertEqual(
            context["presentationSlots"], {"battery_icon": {"assetCandidate": 0}}
        )
        self.assertNotIn("source", json.dumps(context, ensure_ascii=False))

    def test_reference_context_exposes_selection_fact_relations(self) -> None:
        spec = task_spec(size="2x4")
        spec["userQuery"] = "搞个能勾选的几项"
        spec["dataModelSchema"]["properties"]["guard"] = {
            "item1Label": {"type": "string", "description": "标签", "sampleValue": "第一项"},
            "item1Selected": {"type": "boolean", "description": "状态", "sampleValue": True},
            "selectedCount": {"type": "number", "description": "已选", "sampleValue": 1},
            "totalCount": {"type": "number", "description": "总数", "sampleValue": 2},
        }
        context = request_builder.reference_context(spec)
        selection = next(
            relation for relation in context["factRelations"] if relation["kind"] == "selection"
        )
        self.assertEqual(selection["selectedCountPath"], "/guard/selectedCount")
        self.assertEqual(selection["totalCountPath"], "/guard/totalCount")
        self.assertEqual(request_builder.presentation_requirements(spec)["selection"]["mode"], "checkbox")
        self.assertNotIn("sample", selection)

    def test_task_spec_rejects_out_of_range_presentation_slot_reference(self) -> None:
        spec = task_spec()
        spec["presentationSlots"] = {"missing_asset": {"assetCandidate": 1}}

        with self.assertRaisesRegex(ValueError, "must reference an existing candidate"):
            request_builder.validate_task_spec(spec)

    def test_training_request_with_disable_label_and_missing_files_writes_empty_assistant(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            case = Path(temporary)
            spec = task_spec()
            task_path = case / "task.taskSpec.json"
            output_path = case / "task.request.json"
            task_path.write_text(
                json.dumps(spec, ensure_ascii=False), encoding="utf-8"
            )

            argv = [
                "taskspec_to_alt_chat_completions.py",
                str(task_path),
                "-o",
                str(output_path),
                "--disable_label",
            ]
            with mock.patch.object(sys, "argv", argv):
                result = request_builder.main()

            self.assertEqual(result, 0)
            request = json.loads(output_path.read_text(encoding="utf-8"))
            assistant = request["messages"][2]
            self.assertEqual(assistant["role"], "assistant")
            self.assertEqual(assistant["content"], "")


if __name__ == "__main__":
    unittest.main()
