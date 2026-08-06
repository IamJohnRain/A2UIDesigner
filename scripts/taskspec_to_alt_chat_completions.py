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
TUNING = json.loads((CONFIG_DIR / "alt-tuning.json").read_text(encoding="utf-8"))
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


BASE_AUTO_COMPONENTS = [
    "Text",
    "Image",
    "Divider",
    "Progress",
    "Button",
    "Row",
    "Column",
]
LEGACY_ONLY_COMPONENTS = ["Stack", "List", "Repeat"]


def automatic_components(size: str) -> tuple[list[str], list[str]]:
    """Return the component boundary for the automatic protocol."""
    active = list(BASE_AUTO_COMPONENTS)
    if size == "2x4":
        active.append("Checkbox")
    inactive = list(LEGACY_ONLY_COMPONENTS)
    if size == "2x2":
        inactive.append("Checkbox")
    return active, inactive


def build_policy_context(size: str) -> str:
    """Build the single machine-generated policy object shown to the model."""
    canvas_value = LAYOUT_PROFILE.get("canvas", {}).get(size, {})
    canvas = canvas_value if isinstance(canvas_value, dict) else {}
    width = float(canvas.get("width", 140 if size == "2x2" else 300))
    height = float(canvas.get("height", 140))
    padding = float(canvas.get("padding", 12))
    spacing = LAYOUT_PROFILE.get("spacing", {})
    profile_text_rules = LAYOUT_PROFILE.get("textRules", {})
    text_rules = dict(profile_text_rules) if isinstance(profile_text_rules, dict) else {}
    components = TUNING.get("components", LAYOUT_PROFILE.get("components", {}))
    button = components.get("button", {}) if isinstance(components, dict) else {}

    profile_limits = LAYOUT_PROFILE.get("limits", {}).get(size, {})
    limits = dict(profile_limits) if isinstance(profile_limits, dict) else {}
    active_components, inactive_components = automatic_components(size)
    if size == "2x2":
        limits["maxCheckbox"] = 0
    limits["maxLists"] = 0
    limits.pop("maxVisibleListItems", None)

    if size == "2x2":
        shape_strategy = (
            "采用紧凑的纵向结构。root Column 最多三个直接子节点；Row 主要用于短图标/标题头部。"
            "当存在标题/图标、两个事实和一个动作时，使用 Row header、Column status_group、"
            "Button 三个 root 子节点，把两个事实放进 status_group。不要为两个天气事实再增加嵌套 Row；"
            "status_group 最多放两个 Text；第三个事实必须合并为一个短 support 或删除。"
        )
    else:
        shape_strategy = (
            "保留协议要求的 Column root 作为卡片外壳。存在两个独立信息组时，优先在 root "
            "附近使用一个 Row，并让 Row 放置左右两个 Column 分组。只有一个有意义的信息组时，"
            "不要强行增加 Row。"
        )

    if size == "2x4":
        shape_strategy = (
            "2x4 selection cards use Column root -> Row main_row + Row footer. "
            "main_row contains a summary Column and a compact selection Column; "
            "footer contains warning Text and Button. "
            "Use this template when TASK_CONTEXT.interactionRequirements.selection.requested is true. "
            "Otherwise use two horizontal information groups without stretching sparse groups to full height."
        )

    content_width = max(1.0, width - padding * 2)
    content_height = max(1.0, height - padding * 2)

    policy = {
        "profile": LAYOUT_PROFILE.get("profile"),
        "card": {
            "size": size,
            "orientation": "landscape" if width > height else "square",
            "canvas": {"width": width, "height": height},
            "rootPadding": padding,
            "contentArea": {
                "width": content_width,
                "height": content_height,
            },
        },
        "hardProtocol": {
            "rootComponent": "Column",
            "rootRowAllowed": False,
            "maxDirectChildren": 3,
            "allowedComponents": active_components,
            "forbiddenComponents": inactive_components,
            "buttonParentComponents": ["Column", "Row"],
        },
        "axes": {
            "Row": "horizontal",
            "Column": "vertical",
            "rowChildrenMayBeContainers": True,
        },
        "limits": limits,
        "layoutFacts": {
            "rootGap": spacing.get("rootGap", 8),
            "nestedGap": spacing.get("nestedGap", 6),
            "denseGap": spacing.get("denseGap", 4),
            "buttonMinimumHeight": button.get("minimumHeight", 32),
            "columnMinimumsIncludeChildrenAndGaps": True,
            "visibleTextBudgetIsNotALayoutGuarantee": True,
        },
        "textRules": {
            **text_rules,
            "singleLineContentWidth": content_width,
            "policy": "单行受保护文本必须在可用宽度内完整显示；放不下时缩短或删除低优先级事实，不改用长 status。",
        },
        "capabilities": {
            "selection": {
                "available": size == "2x4",
                "interactive": size == "2x4",
                "mode": "checkbox" if size == "2x4" else "summary",
                "fallback": "Text selection summary only",
                "reason": (
                    "2x2 has no compact Checkbox budget"
                    if size == "2x2"
                    else "Use Checkbox select binding for boolean state; add an event only when a matching candidate exists"
                ),
            },
            "semanticStatusRoles": ["status", "warning", "error"],
        },
        "dataBinding": {
            "priority": "Bind runtime facts to scalar REFERENCE_CONTEXT.bindableFields whenever semantically possible",
            "requiredForRoles": [
                "title",
                "primary",
                "status",
                "warning",
                "error",
                "metric",
                "support",
                "meta",
                "item",
                "selection",
            ],
            "staticTextAllowedFor": [
                "fixed UI copy with no matching scalar field",
                "short Button label with no matching scalar action field",
            ],
            "sampleValuesAreNotCopy": True,
            "oneBindingPerFact": True,
        },
        "shapeStrategy": shape_strategy,
        "themes": list(THEME_CONFIG.get("themes", {}).keys()),
    }
    return json.dumps(policy, ensure_ascii=False, indent=2)


def build_static_context(size: str) -> str:
    """Backward-compatible name for the generated policy context."""
    return build_policy_context(size)


SYSTEM_PROMPT = """你是 A2UI 自动布局卡片规划模型。根据下面分层上下文生成一份自动 ALT 和 ASC。

上下文优先级：
1. OUTPUT_CONTRACT 和 POLICY_CONTEXT 是硬规则；
2. TASK_CONTEXT 与 REFERENCE_CONTEXT 是本卡事实和合法引用；
3. SHAPE_STRATEGY 是在不违反硬规则时采用的布局偏好；
4. 用户消息只提供本卡业务意图；不要把任何上下文对象复制到输出中。

OUTPUT_CONTRACT：
- 只输出一个 <alt>...</alt>，后面紧跟一个 <asc>...</asc>；不输出 Markdown 围栏、标题、解释、日志或思考过程。
- ALT 从唯一根节点开始，每层两个空格，每行一个节点，不输出 @alt 头。
- 根节点必须是 Column，并写 card=2x2|2x4 theme=THEME；card 必须等于 POLICY_CONTEXT.card.size，theme 必须来自 POLICY_CONTEXT.themes。
- 普通 ALT 节点只能写 Component id 或 Component id role=ROLE；除 root 的 card/theme 外，不能写任何属性。
- 节点 ID 必须唯一并匹配 [A-Za-z_][A-Za-z0-9_-]*；叶节点必须声明 role；容器可以省略 role。
- 包括根节点在内，每个 Row/Column 最多三个直接子节点；禁止空容器和未挂载节点；全卡最多一个 role=primary。2x4 允许 Button 作为 footer Row 的直接子节点；Button 仍必须有 event=N。
- 只使用 POLICY_CONTEXT.hardProtocol.allowedComponents。不要输出颜色、背景、渐变、尺寸、字号、字重、间距、圆角、边框、阴影、padding、margin、gap、box、size、font、chars、lines、overflow、fit、ratio、type、clip 或 protect。
- 不要输出 POLICY_CONTEXT.hardProtocol.forbiddenComponents；它们只属于旧 DSL 兼容能力，不属于自动训练输出。

布局语义：
- Row 横向分配空间，Column 纵向分配空间；root Column 只是卡片外壳，不代表所有内容都必须纵向堆叠。
- Row 可以包含语义分组 Column；Column 用于组内纵向排列。Button 可以是 Column 或 footer Row 的直接子节点，不能放在更深的文本分组中。
- 2x2 和 2x4 的具体组织方式只按 POLICY_CONTEXT.shapeStrategy 执行，不要从 2x2 示例推断 2x4 结构。
- 所有 Column 的子节点最小高度和间距都要落在 POLICY_CONTEXT.card.contentArea.height 内；可见文本数量预算不是布局可行性的保证。空间不足时删除低优先级的 meta、次要 asset、弱 support、重复事实或额外动作。
- 每个分组 Column 也必须最多三个直接子节点；2x4 左右分组若超过三个事实，合并为一个短 support 或删除低优先级事实，不能继续增加子节点。
- 2x2 的 Row 默认只用于 Image+短标题或两个已确认能并排的一行文本；不要把标题和 metric/status 机械地放进同一个 Row。
- 2x2 的总节点上限包含 root、Row 和 Column 容器；不要为了两个相关事实再增加嵌套 Row。标题/图标、两个事实和动作优先压缩成 `root Column -> Row header + Column status_group + Button`。
- 在 `Row header + Column status_group + Button` 结构中，`status_group` 最多两个 Text；时间、天气温度和天气现象同时出现时只保留主事实，或将次要天气事实合并成一个短 support。

语义选择：
- 只服务一个对象或主问题，同一事实只由一个节点主承载。
- 只展示状态使用 Text，表达动作使用 Button。若 `TASK_CONTEXT.interactionRequirements.selection.mode=checkbox`，使用 Checkbox 表达逐项状态；Checkbox 的 `bind` 指向布尔 selectedPath，`label` 使用对应 labelPath 的动态表达式。若 mode=summary，才使用一个非交互 selection Text。
- `primary`、`status`、`warning`、`error`、`metric` 和 `action` 都是受保护单行角色，不能承载长说明或用“事实 A · 事实 B”拼接多个事实。
- `start/end`、`min/max`、`current/target` 等成对标量如果共同表达一个区间或比较事实，属于一个复合事实；必须由一个 Text 承载，并用 `expr` 同时引用两个字段，不要拆成两个 sibling metric，也不要只保留其中一个字段。
- `warning` 表示需要强调的危险/警告文案，`error` 表示错误文案；只能使用语义角色，不输出具体颜色。
- 绑定字段的 sample 和 units 是布局测量依据，不是可以忽略的描述；sample 的 units 大于 6 时视为长文本，不能绑定到 `primary`、`status`、`warning`、`error` 或 `metric`，应改用 `support` 或删除。
- 时间、日期和 `HH:MM` 字段默认使用 `metric` 或 `support`，不要使用 `primary` 的大号英雄样式；只有极短的纯数字主值才使用 `primary`。
- 复合时间范围如 `14:00-15:00` 仍是一个短时间事实；选择 `metric` 还是 `support` 要按编译器对最终 sample 的测量结果决定，不能因为两个字段存在就增加容器或牺牲结束时间。
- 例如 sample 为“今日已用 42 分钟”的动态字段必须使用 `support`，不能放入 `primary`；不要因为字段描述写着“主数值”就忽略 sample 的 units。
- `primary` 只用于短数值或短状态，不用于会议标题、完整说明或长动态文案。优先保留标题、唯一主值/主状态和必要动作，再保留一个短 support。较长动态字段优先使用 `support`；如果用户要求一行，必须缩短或删除低优先级字段。
- 2x2 title 最多 6 个中文等价单位，2x4 title 最多 10 个；Button 标签分别最多 4/6 个中文等价单位。放不下时缩短文案，不生成第二行或样式参数。

主题和素材：
- 只能选择 POLICY_CONTEXT.themes 中的主题名；不要输出具体颜色或渐变。
- Image 只能通过 ASC 的 asset=N 引用 REFERENCE_CONTEXT.assets 中的索引，不输出 src、bindTo、PNG、网络图、base64、emoji 或素材颜色。
- 编译器会根据主题、角色和 profile 推断所有具体样式。

数据绑定优先级：
- REFERENCE_CONTEXT.bindableFields 是运行时数据的唯一合法来源。先按字段 path、description、sample 和节点 role 为每个事实型 Text 选择最匹配的标量字段。
- 只要 Text 表达的是标题、数值、状态、警告、错误、指标、support、meta、item 或 selection 等运行时事实，并且存在语义匹配字段，就必须写 bind=/REFERENCE_CONTEXT 中的精确 path；不要把 sampleValue 复制成 text=。
- text= 只允许固定 UI 文案，或 REFERENCE_CONTEXT 中没有匹配字段的文案。示例中的静态文字和绑定路径只展示语法，不能照抄到当前卡片。
- Button 的运行时动作文案若有匹配标量字段，使用 bind=/path；没有匹配字段时才使用短静态 label=。Button 永远必须有 event=N。
- 一个语义事实只由一个主承载节点表达；复合事实可以由一个节点引用多个标量字段。必须遵循 REFERENCE_CONTEXT.factRelations，例如 captionValue 必须同时呈现 caption/value，selection 的 selectedCount/totalCount 必须合并为一个摘要，或由 Checkbox 节点分别承载 item label/selected。无法由当前能力表达的字段才可以省略，并且不能为了“用尽”而绑定错误字段或对象/数组路径。

ASC 语法：
- 每行使用 Component node_id key=value，只列有语义补充的节点，并严格遵循 ALT 前序顺序。
- 属性按空白分隔。任何包含空格、制表符或换行的静态 text/label 必须是一个 JSON 双引号字符串，并按 JSON 规则转义，例如 text="今日已用 42 分钟"、label="打开设置"。
- Text 只能使用 text=静态文案、bind=/路径或完整 expr；运行时事实优先使用 bind，且 bind 必须逐字符等于 REFERENCE_CONTEXT.bindableFields 中的标量叶子 path。
- Text 的 `expr` 只使用完整 `{{ ... }}` 表达式；自动协议支持标量 `${/path}`、单引号/双引号字符串和 `+` 拼接，例如 `expr="{{ '已勾选 ' + ${/guard/selectedCount} + '/' + ${/guard/totalCount} + ' 项' }}"`。Checkbox 的动态 `label` 也使用同一表达式语法。每个 `${/path}` 都必须来自 REFERENCE_CONTEXT 的标量叶子字段。
- Checkbox 使用 `label=静态文案或动态表达式` 与 `bind=/booleanPath`；label 必须对应 REFERENCE_CONTEXT.factRelations 中的 labelPath，bind 必须对应 selectedPath；只有存在匹配事件时才写 event=N。
- Image 只能使用 asset=N；Button 使用 label=短静态文案或 bind=/标量路径，并且必须同时写 event=N；Progress 才能使用 value=/路径和 total=/路径。
- bind、value、total 不得指向对象、数组、父路径或猜测路径；asset=N 和 event=N 只能使用 REFERENCE_CONTEXT 中的索引。
- ASC 不复制完整事件、素材路径、DataModel、样式、尺寸或颜色。

严格正例（仅用于展示尺寸对应的结构，不是本卡事实）：
2x2：
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

2x2 两个事实和动作：
<alt>
Column root card=2x2 theme=neutral-light
  Row header
    Image alert_icon role=asset
    Text title role=title
  Column status_group
    Text primary_value role=primary
    Text warning role=warning
  Button action_button role=action
</alt>
<asc>
Image alert_icon asset=0
Text title text=防沉迷
Text primary_value bind=/antiAddiction/primaryText
Text warning bind=/antiAddiction/riskText
Button action_button label=去设置 event=0
</asc>

2x4：
<alt>
Column root card=2x4 theme=ambient-light
  Row body
    Column primary_group
      Text title role=title
      Text primary_value role=primary
    Column secondary_group
      Text status role=status
      Button action_button role=action
</alt>
<asc>
Text title text=设备状态
Text primary_value bind=/device/statusText
Text status bind=/device/statusCaption
Button action_button label="打开设置" event=0
</asc>

2x4 逐项选择：当 TASK_CONTEXT 要求 selection.mode=checkbox 且 REFERENCE_CONTEXT.factRelations 提供 item label/selected 配对时：
<alt>
Column root card=2x4 theme=ambient-light
  Row main_row
    Column summary_group
      Text title role=title
      Text primary_caption role=support
      Text primary_value role=metric
    Column selection_group
      Checkbox item_1 role=selection
      Checkbox item_2 role=selection
      Checkbox item_3 role=selection
  Row footer
    Text warning role=warning
    Button action_button role=action
</alt>
<asc>
Text title bind=/REFERENCE_CONTEXT/titlePath
Text primary_caption bind=/REFERENCE_CONTEXT/captionPath
Text primary_value bind=/REFERENCE_CONTEXT/valuePath
Checkbox item_1 label="{{ ${/REFERENCE_CONTEXT/item1LabelPath} }}" bind=/REFERENCE_CONTEXT/item1SelectedPath
Checkbox item_2 label="{{ ${/REFERENCE_CONTEXT/item2LabelPath} }}" bind=/REFERENCE_CONTEXT/item2SelectedPath
Checkbox item_3 label="{{ ${/REFERENCE_CONTEXT/item3LabelPath} }}" bind=/REFERENCE_CONTEXT/item3SelectedPath
Text warning bind=/REFERENCE_CONTEXT/warningPath
Button action_button bind=/REFERENCE_CONTEXT/actionLabelPath event=0
</asc>

TASK_CONTEXT（任务元数据，不含原始事件对象和素材路径）：
{{TASK_CONTEXT_JSON}}

POLICY_CONTEXT（由 profile 和自动转换能力唯一生成）：
{{POLICY_CONTEXT_JSON}}

REFERENCE_CONTEXT（唯一合法的绑定、素材和事件索引入口）：
{{REFERENCE_CONTEXT_JSON}}
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
    if "sampleValue" not in schema:
        # TaskSpec schemas commonly use named grouping objects without a JSON
        # Schema `type`/`properties` wrapper (for example `{ "guard": {
        # ... } }`).  Continue walking those groups at every depth; stopping
        # once pointer is non-empty silently removed all real bindable fields
        # from REFERENCE_CONTEXT.
        for key, child in schema.items():
            if key in {"type", "description", "maxLength"}:
                continue
            escaped = str(key).replace("~", "~0").replace("/", "~1")
            fields.extend(schema_field_summaries(child, pointer + "/" + escaped))
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


def _field_parent(path: str) -> str:
    return path.rsplit("/", 1)[0] if "/" in path[1:] else ""


def semantic_field_relations(spec: dict[str, Any]) -> list[dict[str, Any]]:
    """Describe relationships between scalar fields without exposing samples twice."""
    fields = schema_field_summaries(spec["dataModelSchema"])
    by_path = {str(field["path"]): field for field in fields}
    relations: list[dict[str, Any]] = []

    value_fields = {
        path.rsplit("/", 1)[-1]: path
        for path in by_path
        if path.rsplit("/", 1)[-1].endswith("Value")
    }
    for leaf, value_path in value_fields.items():
        caption_path = value_path[: -len("Value")] + "Caption"
        if caption_path in by_path:
            relations.append(
                {
                    "kind": "captionValue",
                    "captionPath": caption_path,
                    "valuePath": value_path,
                    "rendering": "adjacent fields or one composite Text; do not show the value without its caption",
                }
            )

    selection_items: list[dict[str, str]] = []
    for path, field in by_path.items():
        leaf = path.rsplit("/", 1)[-1]
        selected_suffix = next(
            (suffix for suffix in ("Selected", "Checked", "Done") if leaf.endswith(suffix)),
            None,
        )
        if selected_suffix is None or field.get("type") != "boolean":
            continue
        base = leaf[: -len(selected_suffix)]
        parent = _field_parent(path)
        label_candidates = [
            parent + "/" + base + "Label",
            parent + "/" + base + "Title",
        ]
        label_path = next((candidate for candidate in label_candidates if candidate in by_path), None)
        if label_path is not None:
            selection_items.append(
                {"labelPath": label_path, "selectedPath": path}
            )

    selected_count = next(
        (
            path
            for path in by_path
            if path.rsplit("/", 1)[-1] == "selectedCount"
        ),
        None,
    )
    total_count = next(
        (
            path
            for path in by_path
            if path.rsplit("/", 1)[-1] == "totalCount"
        ),
        None,
    )
    if selection_items or (selected_count and total_count):
        relation: dict[str, Any] = {
            "kind": "selection",
            "rendering": "one Checkbox per item when selection mode is checkbox; otherwise one summary Text",
        }
        if selection_items:
            relation["items"] = selection_items
        if selected_count:
            relation["selectedCountPath"] = selected_count
        if total_count:
            relation["totalCountPath"] = total_count
        relations.append(relation)
    return relations


def presentation_requirements(spec: dict[str, Any]) -> dict[str, Any]:
    relations = semantic_field_relations(spec)
    selection = next(
        (relation for relation in relations if relation.get("kind") == "selection"),
        None,
    )
    query = str(spec.get("userQuery", "")).lower()
    selection_requested = bool(
        selection
        and any(
            token in query
            for token in ("勾选", "选择", "选中", "复选", "checkbox", "check", "select", "toggle", "待办")
        )
    )
    if selection_requested and spec.get("size") == "2x4":
        mode = "checkbox"
    elif selection is not None:
        mode = "summary"
    else:
        mode = "none"
    item_count = len(selection.get("items", [])) if selection else 0
    return {
        "selection": {
            "requested": selection_requested,
            "mode": mode,
            "itemCount": item_count,
        }
    }


def case_context(spec: dict[str, Any]) -> dict[str, Any]:
    """Return the only model-visible indexes for bindings, assets, and events."""
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
    safe_slots: dict[str, dict[str, int]] = {}
    slots = spec.get("presentationSlots")
    if isinstance(slots, dict):
        for slot_id, slot in slots.items():
            if not isinstance(slot_id, str) or not isinstance(slot, dict):
                continue
            safe_slot: dict[str, int] = {}
            for key in ("assetCandidate", "eventCandidate"):
                value = slot.get(key)
                if isinstance(value, int) and not isinstance(value, bool):
                    safe_slot[key] = value
            if safe_slot:
                safe_slots[slot_id] = safe_slot
    context = {
        "bindableFields": schema_field_summaries(spec["dataModelSchema"]),
        "events": events,
        "assets": assets,
    }
    relations = semantic_field_relations(spec)
    if relations:
        context["factRelations"] = relations
    if safe_slots:
        context["presentationSlots"] = safe_slots
    return context


def task_context(spec: dict[str, Any]) -> dict[str, Any]:
    """Return task metadata without duplicating the user intent or raw candidates."""
    return {
        "cardSize": spec["size"],
        "eventCandidateCount": len(spec.get("eventCandidates", [])),
        "assetCandidateCount": len(spec.get("assetCandidates", [])),
        "presentationSlotIds": sorted(spec.get("presentationSlots", {}).keys())
        if isinstance(spec.get("presentationSlots"), dict)
        else [],
        "interactionRequirements": presentation_requirements(spec),
    }


def reference_context(spec: dict[str, Any]) -> dict[str, Any]:
    """Descriptive alias for the model-facing safe reference context."""
    return case_context(spec)


def canonical_task_spec_json(spec: dict[str, Any]) -> str:
    """Serialize the full TaskSpec for internal callers, never for the model prompt."""
    return json.dumps(spec, ensure_ascii=False, indent=2)


def task_context_json(spec: dict[str, Any]) -> str:
    """Serialize the model-facing task metadata context."""
    return json.dumps(task_context(spec), ensure_ascii=False, indent=2)


def case_context_json(spec: dict[str, Any]) -> str:
    """Serialize the model-facing reference context."""
    return json.dumps(reference_context(spec), ensure_ascii=False, indent=2)


def reference_context_json(spec: dict[str, Any]) -> str:
    """Serialize the model-facing reference context under its explicit name."""
    return case_context_json(spec)


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
    task_facts_json = task_context_json(spec)
    policy_context = build_policy_context(str(spec["size"]))
    references_json = reference_context_json(spec)
    messages: list[dict[str, str]] = [
        {
            "role": "system",
            "content": (
                SYSTEM_PROMPT.replace("{{TASK_CONTEXT_JSON}}", task_facts_json)
                .replace("{{POLICY_CONTEXT_JSON}}", policy_context)
                .replace("{{REFERENCE_CONTEXT_JSON}}", references_json)
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
    # Keep the full TaskSpec as local dataset metadata. The batch runner uses
    # it to materialize task.taskSpec.json, while the API receives only
    # `messages`; this keeps the model-facing context layered and minimal.
    request: dict[str, Any] = {"taskSpec": spec, "messages": messages}
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
