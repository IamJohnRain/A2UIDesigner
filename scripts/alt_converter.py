#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import copy
import json
import logging
import math
import re
import sys
import unicodedata
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable


if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")


GENUI_PROFILE = "genui@0.7.0-alpha.7"
CONFIG_DIR = Path(__file__).resolve().parent / "config"
THEMES_PATH = CONFIG_DIR / "alt-themes.json"
LAYOUT_PROFILE_PATH = CONFIG_DIR / "alt-layout-profile.json"
TUNING_PATH = CONFIG_DIR / "alt-tuning.json"
DEFAULT_DSL_NAME = "card.dsl.jsonl"
DEFAULT_ALT_NAME = "card.alt.txt"
DEFAULT_ASC_NAME = "card.asc.txt"
DEFAULT_LAYOUT_REPORT_NAME = "card.layout-report.txt"
DEFAULT_TASKSPEC_NAME = "task.taskSpec.json"
SURFACE_ID = "surface_card"
CATALOG_ID = "ohos.a2ui.extended.catalog"
LAYOUT_TOP_LEVEL_FIELDS = {
    "id", "component", "children", "styles", "itemMargin", "space", "wrap", "vertical"
}
AUTO_ASC_FIELDS = {
    "Text": {"text", "bind", "expr"},
    "Image": {"asset"},
    "Progress": {"value", "total"},
    "Button": {"label", "bind", "event"},
    "Checkbox": {"label", "value", "group", "bind", "select", "event"},
}
SIMPLE_BINDING_PATTERN = re.compile(r"^\{\{\s*\$\{([^}]+)\}\s*\}\}$")

COMPONENTS = {
    "Text",
    "Image",
    "Divider",
    "Progress",
    "Button",
    "Checkbox",
    "Row",
    "Column",
    "List",
    "Stack",
}
CONTAINERS = {"Row", "Column", "List", "Stack"}
VIRTUAL_COMPONENTS = {"Repeat"}
RING_PROGRESS_TYPES = {"ring", "eclipse", "scaleRing"}
ID_PATTERN = re.compile(r"^[A-Za-z_][A-Za-z0-9_-]*$")
NUMBER_PATTERN = re.compile(r"^-?(?:\d+(?:\.\d*)?|\.\d+)(?:vp|fp|px|%)?$")

MAIN_FROM_DSL = {
    "start": "start",
    "center": "center",
    "end": "end",
    "spaceBetween": "between",
    "spaceAround": "around",
    "spaceEvenly": "evenly",
}
MAIN_TO_DSL = {value: key for key, value in MAIN_FROM_DSL.items()}

ATTRIBUTE_ORDER = [
    "card",
    "theme",
    "box",
    "size",
    "pad",
    "margin",
    "gap",
    "main",
    "cross",
    "align",
    "wrap",
    "grow",
    "shrink",
    "visible",
    "direction",
    "scroll",
    "axis",
    "len",
    "stroke",
    "font",
    "chars",
    "lines",
    "overflow",
    "text",
    "fit",
    "fill",
    "ratio",
    "type",
    "color",
    "shape",
    "selected",
    "unselected",
    "mark",
    "radius",
    "bg",
    "fg",
    "gradient",
    "border",
    "shadow",
    "style",
    "clip",
    "role",
    "protect",
]

COMMON_NODE_ATTRS = {
    "card",
    "theme",
    "box",
    "pad",
    "margin",
    "grow",
    "shrink",
    "radius",
    "bg",
    "gradient",
    "border",
    "shadow",
    "style",
    "clip",
    "role",
    "protect",
}
COMPONENT_NODE_ATTRS = {
    "Text": {"font", "lines", "overflow", "text", "fg"},
    "Image": {"size", "fit", "ratio", "fill"},
    "Divider": {"axis", "len", "stroke", "color"},
    "Progress": {"size", "type", "color"},
    "Button": {"font", "chars", "fg"},
    "Checkbox": {"shape", "selected", "unselected", "mark"},
    "Row": {"gap", "main", "cross", "wrap"},
    "Column": {"gap", "main", "cross"},
    "List": {"gap", "direction", "scroll"},
    "Stack": {"align"},
    "Repeat": {"visible", "role"},
}

class ConversionError(ValueError):
    pass


@dataclass
class AltNode:
    component: str
    node_id: str
    attrs: dict[str, Any] = field(default_factory=dict)
    children: list["AltNode"] = field(default_factory=list)
    line_number: int = 0


@dataclass
class AltDocument:
    root: AltNode


@dataclass
class ValidationIssue:
    severity: str
    node_id: str
    message: str


def configure_logging() -> logging.Logger:
    logger = logging.getLogger("alt_converter")
    logger.handlers.clear()
    logger.setLevel(logging.INFO)
    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(
        logging.Formatter(
            "[%(asctime)s] [%(levelname)s] %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S",
        )
    )
    logger.addHandler(handler)
    logger.propagate = False
    return logger


LOGGER = configure_logging()


def load_json_config(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ConversionError(f"required configuration file does not exist: {path}")
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ConversionError(f"cannot load configuration {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ConversionError(f"configuration root must be an object: {path}")
    return value


THEME_CONFIG = load_json_config(THEMES_PATH)
LAYOUT_PROFILE = load_json_config(LAYOUT_PROFILE_PATH)
TUNING = load_json_config(TUNING_PATH)


def tuning_value(path: str, default: Any) -> Any:
    """Read a dotted path from the tuning configuration with a fallback."""
    node: Any = TUNING
    for part in path.split("."):
        if not isinstance(node, dict) or part not in node:
            return default
        node = node[part]
    return node


# Derived tuning constants. Defaults mirror the values previously hard-coded
# in this module so a missing key cannot silently change behavior.
EPSILON = float(tuning_value("tolerance.comparisonEpsilon", 0.01))
TEXT_WIDTH_SAFETY = float(tuning_value("text.widthSafety", 1.10))
BUTTON_WIDTH_SAFETY = float(tuning_value("text.buttonWidthSafety", 1.08))
LINE_HEIGHT_PADDING = float(tuning_value("text.lineHeightPadding", 4.0))
TEXT_MINIMUM_WIDTH_CHARS = float(tuning_value("text.minimumWidthChars", 2.0))
LONG_TEXT_UNITS_THRESHOLD = float(tuning_value("text.longTextUnitsThreshold", 6.0))
TEXT_UNIT_WEIGHTS = tuning_value("text.unitWeights", {}) or {}
BUTTON_CHARS_RESERVE = float(tuning_value("button.charsHorizontalReserve", 24.0))
BUTTON_LABEL_FONT_FALLBACK = float(tuning_value("button.labelFontFallback", 16.0))
COLUMN_CENTER_MAIN_RATIO = float(tuning_value("column.centerMainRatio", 0.7))
COLUMN_BOTTOM_ACTION_ANCHOR_GAP = float(tuning_value("column.bottomActionAnchorGap", 18.0))
COLUMN_COMPACT_GAP_MINIMUM = float(tuning_value("column.compactGapMinimum", 2.0))
PRIMARY_FONT_BANDS = tuning_value(
    "typography.primaryFontBands",
    [
        {"aboveUnits": 12, "fontSize": 16},
        {"aboveUnits": 8, "fontSize": 18},
        {"aboveUnits": 3, "fontSize": 20},
    ],
)
FONT_ADAPTATION_DEFAULT_MIN_SIZE = int(tuning_value("fontAdaptation.defaultMinSize", 10))
FONT_ADAPTATION_DEFAULT_STEP = int(tuning_value("fontAdaptation.defaultStep", 2))
FONT_ADAPTATION_ABSOLUTE_MIN_SIZE = float(tuning_value("fontAdaptation.absoluteMinimumSize", 8))
THEME_LUMINANCE_WEIGHTS = tuning_value(
    "themeInference.luminanceWeights", [0.2126, 0.7152, 0.0722]
)
THEME_DARK_LUMINANCE_THRESHOLD = float(tuning_value("themeInference.darkLuminanceThreshold", 0.42))
THEME_AMBIENT_CHANNEL_SPREAD = float(tuning_value("themeInference.ambientChannelSpread", 12.0))
THEME_AMBIENT_MIN_CHANNEL = float(tuning_value("themeInference.ambientMinChannel", 238.0))
AUTO_THEMES = set(THEME_CONFIG.get("themes", {}))
GRADIENT_DIRECTIONS = {
    "RightBottom",
    "LeftBottom",
    "RightTop",
    "LeftTop",
    "Right",
    "Left",
    "Bottom",
    "Top",
}


def validate_configuration() -> None:
    required_themes = {"neutral-light", "ambient-light", "focus-dark"}
    missing_themes = sorted(required_themes - AUTO_THEMES)
    if missing_themes:
        raise ConversionError("theme configuration is missing: " + ", ".join(missing_themes))
    default_theme = THEME_CONFIG.get("defaultTheme")
    if default_theme not in AUTO_THEMES:
        raise ConversionError("theme configuration defaultTheme must reference a declared theme")

    required_colors = {
        "surface": {"root", "panel"},
        "text": {"primary", "secondary", "tertiary", "onAccent"},
        "icon": {"primary", "secondary", "onAccent"},
        "action": {"primaryBackground", "primaryText", "secondaryBackground", "secondaryText"},
        "progress": {"fill", "track"},
        "selection": {"selected", "unselected", "mark"},
        "status": {"success", "warning", "error"},
    }
    themes = THEME_CONFIG.get("themes", {})
    for theme_name, theme in themes.items():
        if not isinstance(theme, dict) or theme.get("mode") not in {"light", "dark"}:
            raise ConversionError(f"theme {theme_name!r} must declare mode=light or mode=dark")
        for section_name, field_names in required_colors.items():
            section = theme.get(section_name)
            if not isinstance(section, dict):
                raise ConversionError(f"theme {theme_name!r} is missing section {section_name!r}")
            for field_name in field_names:
                color = section.get(field_name)
                if not isinstance(color, str) or not re.fullmatch(r"#[0-9A-Fa-f]{6}(?:[0-9A-Fa-f]{2})?", color):
                    raise ConversionError(
                        f"theme {theme_name!r} {section_name}.{field_name} must be #RRGGBB or #AARRGGBB"
                    )
        divider = theme.get("divider")
        if not isinstance(divider, str) or not re.fullmatch(r"#[0-9A-Fa-f]{6}(?:[0-9A-Fa-f]{2})?", divider):
            raise ConversionError(f"theme {theme_name!r} divider must be #RRGGBB or #AARRGGBB")
        surface = theme["surface"]
        for field_name in ("gradient", "border"):
            if field_name not in surface:
                raise ConversionError(f"theme {theme_name!r} surface is missing {field_name!r}")
        validate_theme_gradient(surface["gradient"], f"theme {theme_name!r} surface.gradient")
        border = surface["border"]
        if not isinstance(border, str) or not re.fullmatch(
            r"(?:\d+(?:\.\d+)?)(?:vp|fp|px)?/#[0-9A-Fa-f]{6}(?:[0-9A-Fa-f]{2})?",
            border,
        ):
            raise ConversionError(f"theme {theme_name!r} surface.border must be WIDTH/#COLOR")
        action = theme["action"]
        if "primaryGradient" not in action:
            raise ConversionError(f"theme {theme_name!r} action is missing 'primaryGradient'")
        validate_theme_gradient(
            action["primaryGradient"], f"theme {theme_name!r} action.primaryGradient"
        )

    for size, expected in {"2x2": (140, 140), "2x4": (300, 140)}.items():
        canvas = LAYOUT_PROFILE.get("canvas", {}).get(size)
        if not isinstance(canvas, dict) or (canvas.get("width"), canvas.get("height")) != expected:
            raise ConversionError(f"layout profile canvas {size} must be {expected[0]}x{expected[1]}")


def validate_theme_gradient(value: Any, label: str) -> None:
    if not isinstance(value, dict):
        raise ConversionError(f"{label} must be an object")
    if value.get("direction") not in GRADIENT_DIRECTIONS:
        raise ConversionError(f"{label}.direction must be a supported gradient direction")
    colors = value.get("colors")
    if not isinstance(colors, list) or len(colors) < 2:
        raise ConversionError(f"{label}.colors must contain at least two stops")
    previous_stop = -1.0
    for index, stop in enumerate(colors):
        if not (
            isinstance(stop, list)
            and len(stop) == 2
            and isinstance(stop[0], str)
            and re.fullmatch(r"#[0-9A-Fa-f]{6}(?:[0-9A-Fa-f]{2})?", stop[0])
            and isinstance(stop[1], (int, float))
            and 0 <= float(stop[1]) <= 1
            and float(stop[1]) >= previous_stop
        ):
            raise ConversionError(f"{label}.colors[{index}] must be an ordered [#COLOR, 0..1] stop")
        previous_stop = float(stop[1])


validate_configuration()


def compact_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def format_number(value: float | int) -> str:
    number = float(value)
    if math.isfinite(number) and number.is_integer():
        return str(int(number))
    return format(number, ".6g")


def is_dynamic(value: Any) -> bool:
    if isinstance(value, str):
        stripped = value.strip()
        return stripped.startswith("{{") and stripped.endswith("}}")
    if isinstance(value, dict):
        return any(is_dynamic(child) for child in value.values())
    if isinstance(value, list):
        return any(is_dynamic(child) for child in value)
    return False


def encode_alt_value(value: Any) -> str:
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        return format_number(value)
    if isinstance(value, (dict, list)):
        return compact_json(value)
    text = str(value)
    if not text or any(character.isspace() for character in text):
        return json.dumps(text, ensure_ascii=False)
    return text


def parse_alt_value(value: str) -> Any:
    if value == "true":
        return True
    if value == "false":
        return False
    if value == "null":
        return None
    if value.startswith(("{", "[", '"')):
        try:
            return json.loads(value)
        except json.JSONDecodeError as exc:
            raise ConversionError(f"invalid JSON attribute value {value!r}: {exc}") from exc
    if re.fullmatch(r"-?(?:\d+(?:\.\d*)?|\.\d+)", value):
        number = float(value)
        return int(number) if number.is_integer() else number
    return value


def split_alt_tokens(text: str) -> list[str]:
    tokens: list[str] = []
    current: list[str] = []
    quote = False
    escaped = False
    depth = 0
    for character in text:
        if quote:
            current.append(character)
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                quote = False
            continue
        if character == '"':
            quote = True
            current.append(character)
            continue
        if character in "{[(":
            depth += 1
            current.append(character)
            continue
        if character in "}])":
            depth -= 1
            if depth < 0:
                raise ConversionError("unbalanced ALT attribute delimiters")
            current.append(character)
            continue
        if character.isspace() and depth == 0:
            if current:
                tokens.append("".join(current))
                current.clear()
            continue
        current.append(character)
    if quote or depth != 0:
        raise ConversionError("unterminated ALT quote or attribute value")
    if current:
        tokens.append("".join(current))
    return tokens


def normalize_edge(value: Any) -> str | None:
    if value is None or is_dynamic(value):
        return None
    if isinstance(value, (int, float, str)):
        return encode_alt_value(value)
    if isinstance(value, list):
        if len(value) == 1:
            return encode_alt_value(value[0])
        if len(value) == 2:
            return f"{encode_alt_value(value[0])}/{encode_alt_value(value[1])}"
        if len(value) == 4:
            if all(side == value[0] for side in value):
                return encode_alt_value(value[0])
            if value[0] == value[2] and value[1] == value[3]:
                return f"{encode_alt_value(value[0])}/{encode_alt_value(value[1])}"
            return "/".join(encode_alt_value(side) for side in value)
        return encode_alt_value(value)
    if not isinstance(value, dict):
        return encode_alt_value(value)
    if "all" in value:
        return encode_alt_value(value["all"])
    if "vertical" in value or "horizontal" in value:
        vertical = value.get("vertical", 0)
        horizontal = value.get("horizontal", 0)
        return f"{encode_alt_value(vertical)}/{encode_alt_value(horizontal)}"
    sides = [value.get(name, 0) for name in ("top", "right", "bottom", "left")]
    if all(side == sides[0] for side in sides):
        return encode_alt_value(sides[0])
    if sides[0] == sides[2] and sides[1] == sides[3]:
        return f"{encode_alt_value(sides[0])}/{encode_alt_value(sides[1])}"
    return "/".join(encode_alt_value(side) for side in sides)


def parse_edge(value: Any) -> Any:
    if isinstance(value, (int, float, dict)):
        return value
    if isinstance(value, list):
        if len(value) == 1:
            return value[0]
        if len(value) == 2:
            return {"vertical": value[0], "horizontal": value[1]}
        if len(value) == 4:
            return dict(zip(("top", "right", "bottom", "left"), value))
        raise ConversionError(f"edge array must have 1, 2, or 4 items: {value!r}")
    text = str(value)
    parts = text.split("/")
    parsed = [parse_dimension_token(part) for part in parts]
    if any(part is None for part in parsed):
        raise ConversionError(f"invalid edge value: {value!r}")
    if len(parsed) == 1:
        return parsed[0]
    if len(parsed) == 2:
        return [parsed[0], parsed[1], parsed[0], parsed[1]]
    if len(parsed) == 4:
        return parsed
    raise ConversionError(f"edge value must have 1, 2, or 4 parts: {value!r}")


def parse_dimension_token(value: Any) -> int | float | str | None:
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        return value
    text = str(value).strip()
    if text == "auto":
        return None
    if not NUMBER_PATTERN.fullmatch(text):
        return None
    suffix = next((unit for unit in ("vp", "fp", "px", "%") if text.endswith(unit)), "")
    raw = text[: -len(suffix)] if suffix else text
    number = float(raw)
    if suffix:
        return f"{format_number(number)}{suffix}"
    return int(number) if number.is_integer() else number


def numeric_dimension(value: Any) -> float | None:
    parsed = parse_dimension_token(value)
    if isinstance(parsed, (int, float)):
        return float(parsed)
    if isinstance(parsed, str) and parsed.endswith(("vp", "fp", "px")):
        try:
            return float(parsed[:-2])
        except ValueError:
            return None
    return None


def parse_box(value: Any) -> tuple[Any | None, Any | None]:
    text = str(value)
    if "x" not in text:
        raise ConversionError(f"box must use WIDTHxHEIGHT: {value!r}")
    width_text, height_text = text.rsplit("x", 1)
    width = parse_dimension_token(width_text)
    height = parse_dimension_token(height_text)
    if width_text != "auto" and width is None:
        raise ConversionError(f"invalid box width: {width_text!r}")
    if height_text != "auto" and height is None:
        raise ConversionError(f"invalid box height: {height_text!r}")
    return width, height


def edge_numbers(value: Any) -> tuple[float, float, float, float]:
    if value is None:
        return 0.0, 0.0, 0.0, 0.0
    parsed = parse_edge(value)
    if isinstance(parsed, (int, float, str)):
        number = numeric_dimension(parsed) or 0.0
        return number, number, number, number
    if isinstance(parsed, dict):
        if "vertical" in parsed or "horizontal" in parsed:
            vertical = numeric_dimension(parsed.get("vertical", 0)) or 0.0
            horizontal = numeric_dimension(parsed.get("horizontal", 0)) or 0.0
            return vertical, horizontal, vertical, horizontal
        return tuple(
            numeric_dimension(parsed.get(side, 0)) or 0.0
            for side in ("top", "right", "bottom", "left")
        )  # type: ignore[return-value]
    return 0.0, 0.0, 0.0, 0.0


def node_role(node_id: str, component: str) -> str:
    lowered = node_id.lower()
    if node_id == "root":
        return "shell"
    if component == "Button" or any(token in lowered for token in ("action", "button", "cta")):
        return "action"
    if component == "Image":
        return "asset"
    if component == "Checkbox":
        return "selection"
    if component == "Divider":
        return "separator"
    if component == "Progress":
        return "metric"
    if any(token in lowered for token in ("title", "heading")):
        return "title"
    if any(token in lowered for token in ("primary", "hero", "main_value")):
        return "primary"
    if any(token in lowered for token in ("warning", "alert", "risk")):
        return "warning"
    if any(token in lowered for token in ("error", "failure")):
        return "error"
    if any(token in lowered for token in ("status", "badge")):
        return "status"
    if any(token in lowered for token in ("support", "caption", "subtitle", "hint", "meta", "time", "date")):
        return "support"
    if component in CONTAINERS:
        return "group"
    return component.lower()


def should_protect(node_id: str, component: str, role: str) -> bool:
    if component == "Button" or role == "title" or role in configured_single_line_roles():
        return True
    lowered = node_id.lower()
    return any(token in lowered for token in ("time", "date", "price", "count", "total", "value"))


def add_static_attr(
    attrs: dict[str, Any],
    key: str,
    value: Any,
    diagnostics: list[str],
    source_name: str,
) -> None:
    if value is None:
        return
    if is_dynamic(value):
        diagnostics.append(f"dropped dynamic layout style {source_name}")
        return
    attrs[key] = value


def box_from_styles(styles: dict[str, Any], diagnostics: list[str]) -> tuple[Any | None, Any | None]:
    width = styles.get("width")
    height = styles.get("height")
    if is_dynamic(width):
        diagnostics.append("dropped dynamic layout style styles.width")
        width = None
    if is_dynamic(height):
        diagnostics.append("dropped dynamic layout style styles.height")
        height = None
    return width, height


def extract_alt_attrs(component: dict[str, Any], diagnostics: list[str]) -> dict[str, Any]:
    component_type = str(component.get("component", ""))
    node_id = str(component.get("id", ""))
    styles_value = component.get("styles", {})
    styles = styles_value if isinstance(styles_value, dict) else {}
    attrs: dict[str, Any] = {}
    width, height = box_from_styles(styles, diagnostics)

    if component_type == "Divider":
        vertical = bool(styles.get("vertical", component.get("vertical", False)))
        attrs["axis"] = "v" if vertical else "h"
        length = height if vertical else width
        thickness = styles.get("strokeWidth")
        if thickness is None:
            thickness = width if vertical else height
        if length is not None:
            attrs["len"] = length
        if thickness is not None and not is_dynamic(thickness):
            attrs["stroke"] = thickness
    elif component_type in {"Image", "Progress"} and width is not None and width == height:
        attrs["size"] = width
    elif width is not None or height is not None:
        attrs["box"] = f"{encode_alt_value(width if width is not None else 'auto')}x{encode_alt_value(height if height is not None else 'auto')}"

    padding = normalize_edge(styles.get("padding"))
    margin = normalize_edge(styles.get("margin"))
    if padding is not None:
        attrs["pad"] = padding
    if margin is not None:
        attrs["margin"] = margin

    gap = component.get("space") if component_type == "List" else component.get("itemMargin")
    add_static_attr(attrs, "gap", gap, diagnostics, "space" if component_type == "List" else "itemMargin")

    if "justifyContent" in styles:
        main = MAIN_FROM_DSL.get(str(styles["justifyContent"]), str(styles["justifyContent"]))
        add_static_attr(attrs, "main", main, diagnostics, "styles.justifyContent")
    add_static_attr(attrs, "cross", styles.get("alignItems"), diagnostics, "styles.alignItems")
    add_static_attr(attrs, "align", styles.get("alignContent"), diagnostics, "styles.alignContent")
    add_static_attr(attrs, "wrap", component.get("wrap"), diagnostics, "wrap")
    add_static_attr(attrs, "grow", styles.get("layoutWeight"), diagnostics, "styles.layoutWeight")
    add_static_attr(attrs, "shrink", styles.get("flexShrink"), diagnostics, "styles.flexShrink")

    if component_type in {"Text", "Button"}:
        font_size = styles.get("fontSize")
        font_weight = styles.get("fontWeight")
        if not is_dynamic(font_size) and not is_dynamic(font_weight):
            if font_size is not None and font_weight is not None:
                attrs["font"] = f"{encode_alt_value(font_size)}/{encode_alt_value(font_weight)}"
            elif font_size is not None:
                attrs["font"] = font_size
            elif component_type == "Button":
                attrs["font"] = "16/500"
        elif font_size is not None or font_weight is not None:
            diagnostics.append("dropped dynamic font style")
            if component_type == "Button":
                attrs["font"] = "16/500"

        if component_type == "Button":
            capacity_font = numeric_dimension(font_size) if not is_dynamic(font_size) else None
            capacity_font = capacity_font or BUTTON_LABEL_FONT_FALLBACK
            capacity_width = numeric_dimension(width)
            if capacity_width is not None:
                attrs["chars"] = max(
                    0,
                    int(math.floor((capacity_width - BUTTON_CHARS_RESERVE) / capacity_font)),
                )

    if component_type == "Text":
        add_static_attr(attrs, "lines", styles.get("maxLines"), diagnostics, "styles.maxLines")
        add_static_attr(attrs, "overflow", styles.get("textOverflow"), diagnostics, "styles.textOverflow")
        add_static_attr(attrs, "text", styles.get("textAlign"), diagnostics, "styles.textAlign")
    elif component_type == "Image":
        add_static_attr(attrs, "fit", styles.get("objectFit"), diagnostics, "styles.objectFit")
        add_static_attr(attrs, "ratio", styles.get("aspectRatio"), diagnostics, "styles.aspectRatio")
        add_static_attr(attrs, "fill", styles.get("fillColor"), diagnostics, "styles.fillColor")
    elif component_type == "Progress":
        add_static_attr(attrs, "type", styles.get("type"), diagnostics, "styles.type")
        add_static_attr(attrs, "color", styles.get("color"), diagnostics, "styles.color")
    elif component_type == "Divider":
        add_static_attr(attrs, "color", styles.get("color"), diagnostics, "styles.color")
    elif component_type == "Checkbox":
        add_static_attr(attrs, "shape", styles.get("shape"), diagnostics, "styles.shape")
        add_static_attr(attrs, "selected", styles.get("selectedColor"), diagnostics, "styles.selectedColor")
        unselected = styles.get("unselectedColor", styles.get("unSelectedColor"))
        add_static_attr(attrs, "unselected", unselected, diagnostics, "styles.unselectedColor")
        mark = styles.get("mark")
        if isinstance(mark, dict) and not is_dynamic(mark):
            mark_size = mark.get("size", 20)
            mark_width = mark.get("strokeWidth", 2)
            mark_color = mark.get("strokeColor", "#FFFFFFFF")
            attrs["mark"] = "/".join(
                encode_alt_value(item) for item in (mark_size, mark_width, mark_color)
            )
    elif component_type == "List":
        add_static_attr(attrs, "direction", styles.get("listDirection"), diagnostics, "styles.listDirection")
        add_static_attr(attrs, "scroll", styles.get("scrollBar"), diagnostics, "styles.scrollBar")

    add_static_attr(attrs, "radius", styles.get("borderRadius"), diagnostics, "styles.borderRadius")
    add_static_attr(attrs, "bg", styles.get("backgroundColor"), diagnostics, "styles.backgroundColor")
    if component_type in {"Text", "Button"}:
        add_static_attr(attrs, "fg", styles.get("fontColor"), diagnostics, "styles.fontColor")
    add_static_attr(attrs, "gradient", styles.get("linearGradient"), diagnostics, "styles.linearGradient")
    border_width = styles.get("borderWidth")
    border_color = styles.get("borderColor")
    if border_width is not None and not is_dynamic(border_width) and not is_dynamic(border_color):
        attrs["border"] = (
            f"{encode_alt_value(border_width)}/{encode_alt_value(border_color)}"
            if border_color is not None
            else encode_alt_value(border_width)
        )
    add_static_attr(attrs, "shadow", styles.get("shadow"), diagnostics, "styles.shadow")
    if styles.get("clip") is True:
        attrs["clip"] = True

    role = node_role(node_id, component_type)
    attrs["role"] = role
    if should_protect(node_id, component_type, role):
        attrs["protect"] = True
    _, represented_styles = apply_alt_styles(AltNode(component_type, node_id, copy.deepcopy(attrs)))
    extra_styles: dict[str, Any] = {}
    for key, value in styles.items():
        represented = represented_styles.get(key)
        if key in {"padding", "margin"} and key in represented_styles:
            if normalize_edge(value) == normalize_edge(represented):
                continue
        elif key in represented_styles and represented == value:
            continue
        extra_styles[key] = copy.deepcopy(value)
    if extra_styles:
        attrs["style"] = extra_styles
    return attrs


def parse_jsonl_messages(text: str, source: str = "DSL") -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    if len(lines) != 3:
        raise ConversionError(f"GenUI DSL must contain exactly 3 non-empty JSONL lines, got {len(lines)}")
    messages: list[dict[str, Any]] = []
    for index, line in enumerate(lines, 1):
        try:
            value = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ConversionError(f"invalid JSON on {source} line {index}: {exc}") from exc
        if not isinstance(value, dict):
            raise ConversionError(f"DSL line {index} must be a JSON object")
        messages.append(value)
    expected = ("createSurface", "updateComponents", "updateDataModel")
    for index, key in enumerate(expected):
        if key not in messages[index]:
            raise ConversionError(f"DSL line {index + 1} must contain {key}")
    return messages[0], messages[1], messages[2]


def read_jsonl_messages(path: Path) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    if not path.is_file():
        raise ConversionError(f"DSL file does not exist: {path}")
    return parse_jsonl_messages(path.read_text(encoding="utf-8-sig"), str(path))


def validate_restored_content(document: AltDocument, dsl_text: str) -> None:
    nodes: dict[str, AltNode] = {}

    def collect(node: AltNode) -> None:
        if node.component != "Repeat":
            nodes[node.node_id] = node
        for child in node.children:
            collect(child)

    collect(document.root)
    _, update_message, _ = parse_jsonl_messages(dsl_text, "ASC-restored DSL")
    components = update_message.get("updateComponents", {}).get("components", [])
    for component in components:
        if not isinstance(component, dict):
            continue
        node = nodes.get(str(component.get("id", "")))
        if node is None or node.component != "Button":
            continue
        label = component.get("label", "")
        if not isinstance(label, str) or is_dynamic(label):
            raise ConversionError(
                f"{node.node_id}: model-generated Button label must be a bounded static string"
            )
        chars = node.attrs.get("chars")
        if not isinstance(chars, int) or len(label) > chars:
            raise ConversionError(
                f"{node.node_id}: Button label {label!r} contains {len(label)} characters "
                f"but ALT chars={chars!r}"
            )
        width, _ = node_box(node, intrinsic=False)
        required_width = (
            estimated_text_width(label, node_font_size(node, BUTTON_LABEL_FONT_FALLBACK))
            + BUTTON_CHARS_RESERVE
        )
        if width is None or width + EPSILON < required_width:
            raise ConversionError(
                f"{node.node_id}: Button width cannot contain label {label!r}; "
                f"requires at least {required_width:g}vp"
            )


def size_from_surface(create_message: dict[str, Any]) -> str:
    surface = create_message.get("createSurface")
    if not isinstance(surface, dict):
        raise ConversionError("createSurface must be an object")
    width = surface.get("width")
    height = surface.get("height")
    if width == 140 and height == 140:
        return "2x2"
    if width == 300 and height == 140:
        return "2x4"
    raise ConversionError(f"unsupported surface size: {width}x{height}")


def pointer_get(root: Any, pointer: str) -> Any:
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


def dsl_to_alt(path: Path, task_spec: dict[str, Any] | None) -> tuple[AltDocument, str, list[str], int]:
    create_message, update_message, data_message = read_jsonl_messages(path)
    size = size_from_surface(create_message)
    if task_spec is not None and task_spec.get("size") not in {None, size}:
        raise ConversionError(
            f"TaskSpec.size {task_spec.get('size')!r} does not match DSL surface size {size!r}"
        )
    update = update_message.get("updateComponents")
    if not isinstance(update, dict):
        raise ConversionError("updateComponents must be an object")
    root_id = update.get("root")
    components_value = update.get("components")
    if not isinstance(root_id, str) or not root_id:
        raise ConversionError("updateComponents.root must be a non-empty string")
    if not isinstance(components_value, list):
        raise ConversionError("updateComponents.components must be an array")

    component_map: dict[str, dict[str, Any]] = {}
    for component in components_value:
        if not isinstance(component, dict):
            raise ConversionError("every component must be an object")
        node_id = component.get("id")
        component_type = component.get("component")
        if not isinstance(node_id, str) or not ID_PATTERN.fullmatch(node_id):
            raise ConversionError(f"invalid component id: {node_id!r}")
        if component_type not in COMPONENTS:
            raise ConversionError(f"unsupported component {component_type!r} on node {node_id!r}")
        if node_id in component_map:
            raise ConversionError(f"duplicate component id: {node_id}")
        component_map[node_id] = component
    if root_id not in component_map:
        raise ConversionError(f"root component does not exist: {root_id}")

    diagnostics: list[str] = []
    visiting: set[str] = set()
    visited: set[str] = set()
    parent_of: dict[str, str] = {}
    data_model = data_message.get("updateDataModel", {}).get("value", {})

    def build(node_id: str, parent_id: str | None = None) -> AltNode:
        if node_id in visiting:
            raise ConversionError(f"component cycle detected at {node_id}")
        if parent_id is not None and node_id in parent_of and parent_of[node_id] != parent_id:
            raise ConversionError(
                f"component {node_id} has multiple parents: {parent_of[node_id]} and {parent_id}"
            )
        if node_id not in component_map:
            raise ConversionError(f"component reference does not exist: {node_id}")
        if parent_id is not None:
            parent_of[node_id] = parent_id
        visiting.add(node_id)
        source = component_map[node_id]
        node = AltNode(
            component=str(source["component"]),
            node_id=node_id,
            attrs=extract_alt_attrs(source, diagnostics),
        )
        children = source.get("children")
        if isinstance(children, list):
            for child_id in children:
                if not isinstance(child_id, str):
                    raise ConversionError(f"{node_id}.children must contain component IDs")
                node.children.append(build(child_id, node_id))
        elif isinstance(children, dict):
            template_id = children.get("componentId")
            collection_path = children.get("path")
            if not isinstance(template_id, str) or not isinstance(collection_path, str):
                raise ConversionError(f"{node_id}.children template must contain componentId and path")
            collection = pointer_get(data_model, collection_path)
            visible = len(collection) if isinstance(collection, list) else 1
            repeat_id = f"{node_id}_items"
            repeat = AltNode(
                component="Repeat",
                node_id=repeat_id,
                attrs={"visible": max(1, visible), "role": "collection"},
            )
            repeat.children.append(build(template_id, repeat_id))
            node.children.append(repeat)
        elif children is not None:
            raise ConversionError(f"{node_id}.children must be an array or template object")
        visiting.remove(node_id)
        visited.add(node_id)
        return node

    root = build(root_id)
    orphan_count = len(set(component_map) - visited)
    return AltDocument(root), size, diagnostics, orphan_count


def alt_nodes(document: AltDocument) -> list[AltNode]:
    nodes: list[AltNode] = []

    def visit(node: AltNode) -> None:
        if node.component != "Repeat":
            nodes.append(node)
        for child in node.children:
            visit(child)

    visit(document.root)
    return nodes


def binding_path(value: Any) -> str | None:
    if not isinstance(value, str):
        return None
    match = SIMPLE_BINDING_PATTERN.fullmatch(value.strip())
    return match.group(1) if match else None


def split_text_expression_terms(body: str) -> list[str] | None:
    """Split a text expression on top-level + operators.

    Automatic ALT expressions intentionally support a small, deterministic
    subset: scalar JSON-pointer references, quoted string literals, and
    concatenation.  Keeping the parser here explicit avoids evaluating
    arbitrary code while still allowing composite runtime facts such as a
    start/end time range.
    """
    terms: list[str] = []
    current: list[str] = []
    quote: str | None = None
    escaped = False
    for character in body:
        if quote is not None:
            current.append(character)
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == quote:
                quote = None
            continue
        if character in {"'", '"'}:
            quote = character
            current.append(character)
        elif character == "+":
            term = "".join(current).strip()
            if not term:
                return None
            terms.append(term)
            current.clear()
        else:
            current.append(character)
    if quote is not None:
        return None
    term = "".join(current).strip()
    if not term:
        return None
    terms.append(term)
    return terms


def resolve_text_expression(
    expression: str,
    data_model: Any,
) -> tuple[str | None, list[str], str | None]:
    """Resolve the safe automatic Text expression subset against sample data.

    The returned text is only a layout sample.  The original expression is
    retained in the generated DSL so runtime updates remain data-driven.
    """
    if not is_dynamic(expression):
        return None, [], "expression must be wrapped in {{ ... }}"
    stripped = expression.strip()
    if not (stripped.startswith("{{") and stripped.endswith("}}")):
        return None, [], "expression must be wrapped in {{ ... }}"
    body = stripped[2:-2].strip()
    terms = split_text_expression_terms(body)
    if not terms:
        return None, [], "expression must contain at least one term"

    parts: list[str] = []
    references: list[str] = []
    for term in terms:
        pointer_match = re.fullmatch(r"\$\{([^{}]+)\}", term)
        if pointer_match:
            pointer = pointer_match.group(1).strip()
            if not pointer.startswith("/"):
                return None, references, f"expression reference {pointer!r} is not a JSON Pointer"
            value = pointer_get(data_model, pointer)
            if value is None or isinstance(value, (dict, list)):
                return None, references, f"expression reference {pointer!r} has no scalar sampleValue"
            if isinstance(value, bool):
                parts.append("true" if value else "false")
            elif isinstance(value, (int, float)) and not isinstance(value, bool):
                parts.append(format_number(value))
            else:
                parts.append(str(value))
            references.append(pointer)
            continue
        if len(term) >= 2 and term[0] == term[-1] and term[0] in {"'", '"'}:
            try:
                value = ast.literal_eval(term)
            except (SyntaxError, ValueError) as exc:
                return None, references, f"invalid string literal {term!r}: {exc}"
            if not isinstance(value, str):
                return None, references, f"expression literal {term!r} must be a string"
            parts.append(value)
            continue
        return None, references, f"unsupported Text expression term {term!r}"
    return "".join(parts), references, None


def semantic_attrs(
    component: dict[str, Any],
    task_spec: dict[str, Any],
) -> dict[str, Any]:
    component_type = str(component.get("component", ""))
    semantic = {
        key: copy.deepcopy(value)
        for key, value in component.items()
        if key not in LAYOUT_TOP_LEVEL_FIELDS
    }
    attrs: dict[str, Any] = {}

    content = semantic.pop("content", None)
    if content is not None:
        path = binding_path(content)
        if path is not None:
            attrs["bind"] = path
        elif is_dynamic(content):
            attrs["expr"] = content
        else:
            attrs["text"] = content

    src = semantic.pop("src", None)
    if src is not None:
        matched_asset = None
        for index, candidate in enumerate(task_spec.get("assetCandidates", [])):
            if not isinstance(candidate, dict):
                continue
            bind_to = candidate.get("bindTo")
            if src == candidate.get("src") or (
                isinstance(bind_to, str) and binding_path(src) == bind_to
            ):
                matched_asset = index
                break
        if matched_asset is not None:
            attrs["asset"] = matched_asset
        else:
            path = binding_path(src)
            if path is not None:
                attrs["bind"] = path
            elif is_dynamic(src):
                attrs["expr"] = src
            else:
                attrs["src"] = src

    on_click = semantic.pop("onClick", None)
    if on_click is not None:
        matched_event = None
        for index, candidate in enumerate(task_spec.get("eventCandidates", [])):
            if on_click == [candidate]:
                matched_event = index
                break
        attrs["event" if matched_event is not None else "onClick"] = (
            matched_event if matched_event is not None else on_click
        )

    select = semantic.pop("select", None)
    if select is not None:
        path = binding_path(select)
        attrs["bind" if path is not None else "select"] = path if path is not None else select

    for key in ("value", "total"):
        if key not in semantic:
            continue
        value = semantic.pop(key)
        path = binding_path(value) if component_type == "Progress" else None
        attrs[key] = path if path is not None else value

    attrs.update(semantic)
    return attrs


def build_asc(
    original: tuple[dict[str, Any], dict[str, Any], dict[str, Any]],
    document: AltDocument,
    task_spec: dict[str, Any],
) -> str:
    update = original[1].get("updateComponents", {})
    components = update.get("components", []) if isinstance(update, dict) else []
    component_map = {
        component["id"]: component
        for component in components
        if isinstance(component, dict) and isinstance(component.get("id"), str)
    }
    lines: list[str] = []
    for node in alt_nodes(document):
        component = component_map[node.node_id]
        attrs = semantic_attrs(component, task_spec)
        if not attrs:
            continue
        tokens = [node.component, node.node_id]
        for key, value in attrs.items():
            tokens.append(f"{key}={encode_alt_value(value)}")
        lines.append(" ".join(tokens))
    return "\n".join(lines).rstrip() + "\n"


def parse_asc_text(text: str, document: AltDocument) -> dict[str, dict[str, Any]]:
    nodes = alt_nodes(document)
    node_map = {node.node_id: node for node in nodes}
    node_order = {node.node_id: index for index, node in enumerate(nodes)}
    result: dict[str, dict[str, Any]] = {}
    previous = -1
    for line_number, line in enumerate(text.splitlines(), 1):
        if not line.strip():
            continue
        tokens = split_alt_tokens(line.strip())
        if len(tokens) < 3:
            raise ConversionError(f"ASC line {line_number} must contain component, id, and properties")
        component_type, node_id = tokens[:2]
        node = node_map.get(node_id)
        if node is None or node.component != component_type:
            raise ConversionError(f"ASC line {line_number} does not map to the matching ALT node")
        if node_id in result or node_order[node_id] <= previous:
            raise ConversionError("ASC nodes must be unique and follow ALT preorder")
        attrs: dict[str, Any] = {}
        for token in tokens[2:]:
            if "=" not in token:
                raise ConversionError(f"ASC line {line_number} contains invalid property {token!r}")
            key, raw_value = token.split("=", 1)
            if key in {"styles", "style", "id", "component", "children"}:
                raise ConversionError(f"ASC cannot contain layout/style field {key!r}")
            attrs[key] = parse_alt_value(raw_value)
        result[node_id] = attrs
        previous = node_order[node_id]
    return result


def parse_asc(path: Path, document: AltDocument) -> dict[str, dict[str, Any]]:
    if not path.is_file():
        raise ConversionError(f"ASC file does not exist: {path}")
    return parse_asc_text(path.read_text(encoding="utf-8-sig"), document)


def validate_auto_asc(
    document: AltDocument,
    task_spec: dict[str, Any],
    asc: dict[str, dict[str, Any]],
) -> None:
    events = task_spec.get("eventCandidates", [])
    assets = task_spec.get("assetCandidates", [])
    data_model = sample_data_model(task_spec["dataModelSchema"])

    def require_scalar_pointer(node_id: str, key: str, pointer: Any) -> None:
        if not isinstance(pointer, str) or not pointer.startswith("/"):
            raise ConversionError(f"{node_id}: auto ASC {key} must be a JSON Pointer")
        try:
            value = pointer_get(data_model, pointer)
        except ConversionError:
            value = None
        if isinstance(value, (dict, list)) or value is None:
            raise ConversionError(
                f"{node_id}: auto ASC {key} must reference a scalar TaskSpec sampleValue, not {pointer!r}"
            )

    for node in alt_nodes(document):
        attrs = asc.get(node.node_id, {})
        allowed = AUTO_ASC_FIELDS.get(node.component, set())
        unknown = sorted(set(attrs) - allowed)
        if unknown:
            raise ConversionError(
                f"{node.node_id}: auto ASC {node.component} does not support field(s): "
                + ", ".join(unknown)
            )
        if node.component == "Text" and not any(key in attrs for key in ("text", "bind", "expr")):
            raise ConversionError(f"{node.node_id}: auto ASC Text requires text, bind, or expr")
        if node.component == "Image" and "asset" not in attrs:
            raise ConversionError(f"{node.node_id}: auto ASC Image requires asset=N")
        if node.component == "Progress" and not all(key in attrs for key in ("value", "total")):
            raise ConversionError(f"{node.node_id}: auto ASC Progress requires value and total")
        if node.component == "Button" and not all(key in attrs for key in ("event",)):
            raise ConversionError(f"{node.node_id}: auto ASC Button requires event=N")
        if node.component == "Button" and not any(key in attrs for key in ("label", "bind")):
            raise ConversionError(f"{node.node_id}: auto ASC Button requires label or bind and event=N")
        if node.component == "Button" and "label" in attrs and "bind" in attrs:
            raise ConversionError(f"{node.node_id}: auto ASC Button cannot contain both label and bind")
        if node.component == "Button":
            label = attrs.get("label")
            if label is not None and (not isinstance(label, str) or label.startswith("/") or is_dynamic(label)):
                raise ConversionError(
                    f"{node.node_id}: auto ASC Button label must be a short static string, not a binding"
                )

        for key in ("text", "label", "value", "group"):
            if key in attrs and not isinstance(attrs[key], str):
                raise ConversionError(f"{node.node_id}: auto ASC {key} must be a string")
        for key in ("bind",):
            if key in attrs and (not isinstance(attrs[key], str) or not attrs[key].startswith("/")):
                raise ConversionError(f"{node.node_id}: auto ASC {key} must be a JSON Pointer")
        if node.component == "Text" and "bind" in attrs:
            require_scalar_pointer(node.node_id, "bind", attrs["bind"])
        if node.component == "Button" and "bind" in attrs:
            require_scalar_pointer(node.node_id, "bind", attrs["bind"])
        if node.component == "Checkbox" and "bind" in attrs:
            require_scalar_pointer(node.node_id, "bind", attrs["bind"])
            try:
                selected_sample = pointer_get(data_model, attrs["bind"])
            except ConversionError:
                selected_sample = None
            if not isinstance(selected_sample, bool):
                raise ConversionError(
                    f"{node.node_id}: auto ASC Checkbox bind must reference a boolean selected field"
                )
        if node.component == "Checkbox":
            if "bind" not in attrs or "label" not in attrs:
                raise ConversionError(
                    f"{node.node_id}: auto ASC Checkbox requires label and bind=/booleanPath"
                )
            label = attrs["label"]
            if not isinstance(label, str):
                raise ConversionError(f"{node.node_id}: auto ASC Checkbox label must be a string")
            if is_dynamic(label):
                _, label_refs, label_error = resolve_text_expression(label, data_model)
                if label_error:
                    raise ConversionError(
                        f"{node.node_id}: auto ASC Checkbox label cannot be measured safely: {label_error}"
                    )
                for pointer in label_refs:
                    require_scalar_pointer(node.node_id, "label", pointer)
        if node.component == "Progress":
            for key in ("value", "total"):
                if not isinstance(attrs[key], str) or not attrs[key].startswith("/"):
                    raise ConversionError(f"{node.node_id}: auto ASC {key} must be a JSON Pointer")
                require_scalar_pointer(node.node_id, key, attrs[key])
        if "expr" in attrs:
            if not is_dynamic(attrs["expr"]):
                raise ConversionError(f"{node.node_id}: auto ASC expr must be a complete {{ ... }} expression")
            _, expression_refs, expression_error = resolve_text_expression(
                attrs["expr"], data_model
            )
            if expression_error:
                raise ConversionError(
                    f"{node.node_id}: auto ASC expr cannot be measured safely: {expression_error}"
                )
            for pointer in expression_refs:
                require_scalar_pointer(node.node_id, "expr", pointer)
        if "asset" in attrs:
            index = attrs["asset"]
            if not isinstance(index, int) or index < 0 or index >= len(assets):
                raise ConversionError(f"{node.node_id}: auto ASC asset must reference a valid assetCandidate")
            candidate = assets[index]
            source = candidate.get("src") if isinstance(candidate, dict) else None
            if not isinstance(source, str) or not source.lower().endswith(".svg"):
                raise ConversionError(f"{node.node_id}: auto ASC asset must reference a local SVG")
        if "event" in attrs:
            index = attrs["event"]
            if not isinstance(index, int) or index < 0 or index >= len(events):
                raise ConversionError(f"{node.node_id}: auto ASC event must reference a valid eventCandidate")


def apply_asc_to_dsl(
    base_text: str,
    document: AltDocument,
    task_spec: dict[str, Any],
    asc: dict[str, dict[str, Any]],
) -> str:
    messages = list(parse_jsonl_messages(base_text, "generated base DSL"))
    components = messages[1]["updateComponents"]["components"]
    for component in components:
        node_id = component["id"]
        for key in list(component):
            if key not in LAYOUT_TOP_LEVEL_FIELDS:
                del component[key]
        attrs = asc.get(node_id, {})
        component_type = component["component"]
        for key, value in attrs.items():
            if key == "text":
                component["content"] = value
            elif key == "bind":
                target = (
                    "select"
                    if component_type == "Checkbox"
                    else "src"
                    if component_type == "Image"
                    else "label"
                    if component_type == "Button"
                    else "content"
                )
                component[target] = f"{{{{ ${{{value}}} }}}}"
            elif key == "expr":
                target = "src" if component_type == "Image" else "content"
                component[target] = value
            elif key == "asset":
                candidates = task_spec.get("assetCandidates", [])
                if not isinstance(value, int) or value < 0 or value >= len(candidates):
                    raise ConversionError(f"{node_id}: invalid ASC asset index {value!r}")
                candidate = candidates[value]
                source = candidate.get("src") if isinstance(candidate, dict) else None
                if is_auto_layout(document) and (not isinstance(source, str) or not source.lower().endswith(".svg")):
                    raise ConversionError(f"{node_id}: auto ALT assets must use a local .svg source")
                bind_to = candidate.get("bindTo") if isinstance(candidate, dict) else None
                component["src"] = f"{{{{ ${{{bind_to}}} }}}}" if isinstance(bind_to, str) else source
            elif key == "event":
                candidates = task_spec.get("eventCandidates", [])
                if not isinstance(value, int) or value < 0 or value >= len(candidates):
                    raise ConversionError(f"{node_id}: invalid ASC event index {value!r}")
                component["onClick"] = [copy.deepcopy(candidates[value])]
            elif key == "onClick":
                component["onClick"] = value
            elif key in {"value", "total"} and component_type == "Progress" and isinstance(value, str) and value.startswith("/"):
                component[key] = f"{{{{ ${{{value}}} }}}}"
            elif key == "select":
                component["select"] = value
            else:
                if component_type == "Image" and key == "src" and is_auto_layout(document):
                    if not isinstance(value, str) or not value.lower().endswith(".svg"):
                        raise ConversionError(f"{node_id}: auto ALT Image.src must be a local .svg path")
                component[key] = value
    return "\n".join(compact_json(message) for message in messages) + "\n"


def ordered_attrs(attrs: dict[str, Any]) -> Iterable[tuple[str, Any]]:
    emitted: set[str] = set()
    for key in ATTRIBUTE_ORDER:
        if key in attrs:
            emitted.add(key)
            yield key, attrs[key]
    for key in sorted(set(attrs) - emitted):
        yield key, attrs[key]


def serialize_alt(document: AltDocument) -> str:
    lines: list[str] = []

    def emit(node: AltNode, depth: int) -> None:
        tokens = [node.component, node.node_id]
        for key, value in ordered_attrs(node.attrs):
            if key in {"clip", "protect"} and value is True:
                tokens.append(key)
            elif value is not None:
                tokens.append(f"{key}={encode_alt_value(value)}")
        lines.append("  " * depth + " ".join(tokens))
        for child in node.children:
            emit(child, depth + 1)

    emit(document.root, 0)
    return "\n".join(lines).rstrip() + "\n"


def parse_alt(path: Path) -> AltDocument:
    if not path.is_file():
        raise ConversionError(f"ALT file does not exist: {path}")
    raw_lines = path.read_text(encoding="utf-8-sig").splitlines()
    content = [(index, line) for index, line in enumerate(raw_lines, 1) if line.strip()]
    if not content:
        raise ConversionError("ALT file is empty")
    roots: list[AltNode] = []
    stack: list[AltNode] = []
    seen_ids: set[str] = set()
    for line_number, line in content:
        if "\t" in line[: len(line) - len(line.lstrip())]:
            raise ConversionError(f"line {line_number}: ALT indentation cannot contain tabs")
        leading = len(line) - len(line.lstrip(" "))
        if leading % 2 != 0:
            raise ConversionError(f"line {line_number}: indentation must use two spaces per level")
        depth = leading // 2
        tokens = split_alt_tokens(line.strip())
        if len(tokens) < 2:
            raise ConversionError(f"line {line_number}: node must contain component and id")
        component, node_id = tokens[0], tokens[1]
        if component not in COMPONENTS | VIRTUAL_COMPONENTS:
            raise ConversionError(f"line {line_number}: unsupported component {component!r}")
        if not ID_PATTERN.fullmatch(node_id):
            raise ConversionError(f"line {line_number}: invalid node id {node_id!r}")
        if node_id in seen_ids:
            raise ConversionError(f"line {line_number}: duplicate node id {node_id!r}")
        attrs: dict[str, Any] = {}
        for token in tokens[2:]:
            if "=" in token:
                key, raw_value = token.split("=", 1)
                if not key:
                    raise ConversionError(f"line {line_number}: empty attribute name")
                attrs[key] = parse_alt_value(raw_value)
            elif token in {"clip", "protect"}:
                attrs[token] = True
            else:
                raise ConversionError(f"line {line_number}: unknown flag {token!r}")
        if component == "Repeat":
            allowed_attrs = COMPONENT_NODE_ATTRS[component]
        elif component == "Divider":
            allowed_attrs = (COMMON_NODE_ATTRS - {"box"}) | COMPONENT_NODE_ATTRS[component]
        else:
            allowed_attrs = COMMON_NODE_ATTRS | COMPONENT_NODE_ATTRS[component]
        unknown_attrs = sorted(set(attrs) - allowed_attrs)
        if unknown_attrs:
            raise ConversionError(
                f"line {line_number}: {component} does not support ALT attribute(s): "
                + ", ".join(unknown_attrs)
            )
        node = AltNode(component, node_id, attrs, line_number=line_number)
        seen_ids.add(node_id)
        if depth == 0:
            roots.append(node)
            stack = [node]
        else:
            if depth > len(stack):
                raise ConversionError(f"line {line_number}: indentation jumps over a parent level")
            stack = stack[:depth]
            if not stack:
                raise ConversionError(f"line {line_number}: node has no parent")
            stack[-1].children.append(node)
            stack.append(node)
    if len(roots) != 1:
        raise ConversionError(f"ALT must contain exactly one root node, got {len(roots)}")
    return AltDocument(roots[0])


def is_auto_layout(document: AltDocument) -> bool:
    return "card" in document.root.attrs or "theme" in document.root.attrs


def theme_values(theme_name: str) -> dict[str, Any]:
    themes = THEME_CONFIG.get("themes", {})
    theme = themes.get(theme_name) if isinstance(themes, dict) else None
    if not isinstance(theme, dict):
        raise ConversionError(f"unknown ALT theme {theme_name!r}")
    return theme


def checkbox_layout_profile(*, automatic: bool) -> dict[str, Any]:
    """Return the explicit Checkbox profile for legacy or automatic output."""
    components = TUNING.get("components", {})
    key = "autoCheckbox" if automatic else "checkbox"
    value = components.get(key, {}) if isinstance(components, dict) else {}
    return value if isinstance(value, dict) else {}


def infer_theme_name(document: AltDocument) -> str:
    attrs = document.root.attrs
    background = attrs.get("bg")
    if not isinstance(background, str):
        style = attrs.get("style")
        if isinstance(style, dict):
            background = style.get("backgroundColor")
    if not isinstance(background, str) or not re.fullmatch(r"#[0-9A-Fa-f]{6}(?:[0-9A-Fa-f]{2})?", background):
        return str(THEME_CONFIG.get("defaultTheme", "neutral-light"))
    raw = background[1:]
    if len(raw) == 8:
        red, green, blue = int(raw[2:4], 16), int(raw[4:6], 16), int(raw[6:8], 16)
    else:
        red, green, blue = int(raw[0:2], 16), int(raw[2:4], 16), int(raw[4:6], 16)
    luminance = (
        float(THEME_LUMINANCE_WEIGHTS[0]) * red
        + float(THEME_LUMINANCE_WEIGHTS[1]) * green
        + float(THEME_LUMINANCE_WEIGHTS[2]) * blue
    ) / 255.0
    if luminance < THEME_DARK_LUMINANCE_THRESHOLD:
        return "focus-dark"
    channel_spread = max(red, green, blue) - min(red, green, blue)
    if (
        channel_spread >= THEME_AMBIENT_CHANNEL_SPREAD
        or min(red, green, blue) < THEME_AMBIENT_MIN_CHANNEL
    ):
        return "ambient-light"
    return "neutral-light"


def simplify_to_auto(document: AltDocument, size: str) -> AltDocument:
    result = copy.deepcopy(document)

    def visit(node: AltNode, root: bool = False) -> None:
        role = node.attrs.get("role")
        if node.component == "Stack":
            node.component = "Column"
        node.attrs = {}
        if root:
            node.attrs["card"] = size
            node.attrs["theme"] = infer_theme_name(document)
        if isinstance(role, str) and role:
            node.attrs["role"] = role
        for child in node.children:
            visit(child)

    visit(result.root, True)
    return result


def validate_auto_protocol(document: AltDocument, task_size: str | None = None) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    root = document.root
    size = root.attrs.get("card")
    theme = root.attrs.get("theme")
    if size not in {"2x2", "2x4"}:
        issues.append(ValidationIssue("error", root.node_id, "auto ALT root must declare card=2x2 or card=2x4"))
        return issues
    if task_size is not None and size != task_size:
        issues.append(
            ValidationIssue("error", root.node_id, f"ALT card={size} does not match TaskSpec.size={task_size}")
        )
    if theme not in AUTO_THEMES:
        issues.append(
            ValidationIssue(
                "error",
                root.node_id,
                "auto ALT theme must be one of: " + ", ".join(sorted(AUTO_THEMES)),
            )
        )
    if root.component != "Column":
        issues.append(ValidationIssue("error", root.node_id, "auto ALT root must be Column"))

    limits_value = LAYOUT_PROFILE.get("limits", {}).get(str(size), {})
    limits = limits_value if isinstance(limits_value, dict) else {}
    nodes = alt_nodes(document)
    counters = {
        "maxNodes": len(nodes),
        "maxText": sum(node.component == "Text" for node in nodes),
        "maxButtons": sum(node.component == "Button" for node in nodes),
        "maxImages": sum(node.component == "Image" for node in nodes),
        "maxProgress": sum(node.component == "Progress" for node in nodes),
        "maxCheckbox": sum(node.component == "Checkbox" for node in nodes),
        "maxLists": sum(node.component == "List" for node in nodes),
    }
    labels = {
        "maxNodes": "nodes",
        "maxText": "Text nodes",
        "maxButtons": "Button nodes",
        "maxImages": "Image nodes",
        "maxProgress": "Progress nodes",
        "maxCheckbox": "Checkbox nodes",
        "maxLists": "List nodes",
    }
    for key, actual in counters.items():
        maximum = limits.get(key)
        if isinstance(maximum, int) and actual > maximum:
            issues.append(
                ValidationIssue("error", root.node_id, f"auto ALT has {actual} {labels[key]}; {size} allows {maximum}")
            )

    primary_count = 0

    def visit(node: AltNode, depth: int, is_root: bool = False) -> None:
        nonlocal primary_count
        allowed = {"card", "theme", "role"} if is_root else {"role"}
        forbidden = sorted(set(node.attrs) - allowed)
        if forbidden:
            issues.append(
                ValidationIssue(
                    "error",
                    node.node_id,
                    "auto ALT cannot contain inferred attribute(s): " + ", ".join(forbidden),
                )
            )
        if not is_root and any(key in node.attrs for key in ("card", "theme")):
            issues.append(ValidationIssue("error", node.node_id, "card/theme are allowed only on the root node"))
        role = node.attrs.get("role")
        if node.component not in CONTAINERS | VIRTUAL_COMPONENTS and not isinstance(role, str):
            issues.append(ValidationIssue("error", node.node_id, "auto ALT leaf nodes must declare role"))
        if role == "primary":
            primary_count += 1
        max_depth = limits.get("maxDepth")
        if isinstance(max_depth, int) and depth > max_depth:
            issues.append(
                ValidationIssue("error", node.node_id, f"auto ALT depth {depth} exceeds {size} limit {max_depth}")
            )
        if node.component in CONTAINERS and not node.children:
            issues.append(ValidationIssue("error", node.node_id, "auto ALT containers cannot be empty"))
        if node.component in {"Row", "Column", "Stack"} and len(node.children) > 3:
            issues.append(
                ValidationIssue("error", node.node_id, "auto ALT containers can have at most three direct children")
            )
        if node.component == "Stack":
            issues.append(
                ValidationIssue("error", node.node_id, "Stack is compiler-reserved and cannot appear in auto ALT input")
            )
        for child in node.children:
            visit(child, depth + 1)

    visit(root, 1, True)
    if primary_count > 1:
        issues.append(ValidationIssue("error", root.node_id, "auto ALT allows only one role=primary node"))
    return issues


def node_box(
    node: AltNode,
    intrinsic: bool = True,
    checkbox_min_height: float = 48.0,
    checkbox_min_width: float = 36.0,
) -> tuple[float | None, float | None]:
    if "size" in node.attrs:
        size = numeric_dimension(node.attrs["size"])
        return size, size
    if "box" in node.attrs:
        width_value, height_value = parse_box(node.attrs["box"])
        width = numeric_dimension(width_value)
        height = numeric_dimension(height_value)
    else:
        width = height = None
    if intrinsic and node.component == "Checkbox":
        height = max(height or 0.0, checkbox_min_height)
        width = max(width or 0.0, checkbox_min_width)
    return width, height


def validate_layout(document: AltDocument, size: str) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    automatic = is_auto_layout(document)
    checkbox_profile = checkbox_layout_profile(automatic=automatic)
    checkbox_min_height = float(
        checkbox_profile.get("outerHeight", 48 if not automatic else 22)
    )
    checkbox_min_width = float(
        checkbox_profile.get(
            "minimumWidth",
            float(checkbox_profile.get("controlSize", 20))
            + float(checkbox_profile.get("controlMargin", 2)) * 2
            + float(checkbox_profile.get("labelGap", 12)),
        )
    )
    expected = (140.0, 140.0) if size == "2x2" else (300.0, 140.0)
    root_width, root_height = node_box(document.root, intrinsic=False)
    if document.root.component not in {"Row", "Column", "Stack"}:
        issues.append(
            ValidationIssue("error", document.root.node_id, "root must be Row, Column, or Stack")
        )
    if root_width is None or root_height is None:
        issues.append(ValidationIssue("error", document.root.node_id, "root must declare a numeric box"))
    elif (root_width, root_height) != expected:
        issues.append(
            ValidationIssue(
                "error",
                document.root.node_id,
                f"root box {root_width:g}x{root_height:g} does not match {size} canvas {expected[0]:g}x{expected[1]:g}",
            )
        )
    if "pad" not in document.root.attrs:
        issues.append(ValidationIssue("error", document.root.node_id, "root must declare pad=12"))
    if "radius" not in document.root.attrs:
        issues.append(ValidationIssue("error", document.root.node_id, "root must declare radius"))
    if document.root.attrs.get("clip") is not True:
        issues.append(ValidationIssue("error", document.root.node_id, "root must declare clip"))
    if not any(key in document.root.attrs for key in ("bg", "gradient")):
        issues.append(ValidationIssue("error", document.root.node_id, "root must declare bg or gradient"))

    def visit(node: AltNode, parent: AltNode | None = None) -> None:
        width, height = node_box(node, intrinsic=False)
        if node.children and node.component not in CONTAINERS | VIRTUAL_COMPONENTS:
            issues.append(
                ValidationIssue("error", node.node_id, f"{node.component} cannot contain ALT child nodes")
            )
        if node.component == "Checkbox":
            if height is not None and height < checkbox_min_height:
                issues.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        f"Checkbox outer height {height:g} is smaller than intrinsic {checkbox_min_height:g}vp",
                    )
                )
            if width is not None and width < checkbox_min_width:
                issues.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        f"Checkbox width {width:g} cannot contain the fixed control and label gap minimum {checkbox_min_width:g}vp",
                    )
                )
            if any(key in node.attrs for key in ("font", "lines", "overflow")):
                issues.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        "Checkbox cannot control its internal label font, lines, or overflow through ALT",
                    )
                )
            if "mark" in node.attrs:
                try:
                    mark_size = numeric_dimension(str(node.attrs["mark"]).split("/", 1)[0])
                except (TypeError, ValueError):
                    mark_size = None
                if mark_size is not None and mark_size > 20:
                    issues.append(
                        ValidationIssue(
                            "error",
                            node.node_id,
                            f"Checkbox mark size {mark_size:g} exceeds the fixed 20vp control",
                        )
                    )

        if node.component == "Progress" and str(node.attrs.get("type", "linear")) in RING_PROGRESS_TYPES:
            if width is not None and height is not None and not math.isclose(width, height):
                issues.append(
                    ValidationIssue("error", node.node_id, "ring-like Progress must have equal width and height")
                )

        if node.component == "Button":
            font_size = BUTTON_LABEL_FONT_FALLBACK
            if "font" in node.attrs:
                font_size = numeric_dimension(str(node.attrs["font"]).split("/", 1)[0]) or font_size
            else:
                issues.append(ValidationIssue("error", node.node_id, "Button must declare font=SIZE/WEIGHT"))
            chars = node.attrs.get("chars")
            if not isinstance(chars, int) or isinstance(chars, bool) or chars < 1:
                issues.append(ValidationIssue("error", node.node_id, "Button chars must be an integer of at least 1"))
            if width is not None and isinstance(chars, int):
                capacity = max(0, int(math.floor((width - BUTTON_CHARS_RESERVE) / font_size)))
                if chars > capacity:
                    issues.append(
                        ValidationIssue(
                            "error",
                            node.node_id,
                            f"Button chars={chars} exceeds box/font capacity {capacity}",
                        )
                    )
            minimum = max(32.0, font_size + 16.0)
            if height is not None and height < minimum:
                issues.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        f"Button height {height:g} is smaller than the {minimum:g}vp intrinsic minimum",
                    )
                )

        main = str(node.attrs.get("main", ""))
        if main in {"between", "around", "evenly"} and "gap" in node.attrs:
            issues.append(
                ValidationIssue(
                    "error",
                    node.node_id,
                    f"main={main} cannot be combined with a fixed gap",
                )
            )

        if node.component in {"Row", "Column", "Stack"} and node.children:
            parent_width, parent_height = node_box(
                node,
                checkbox_min_height=checkbox_min_height,
                checkbox_min_width=checkbox_min_width,
            )
            top, right, bottom, left = edge_numbers(node.attrs.get("pad"))
            inner_width = None if parent_width is None else max(0.0, parent_width - left - right)
            inner_height = None if parent_height is None else max(0.0, parent_height - top - bottom)
            child_boxes = [
                node_box(
                    child,
                    checkbox_min_height=checkbox_min_height,
                    checkbox_min_width=checkbox_min_width,
                )
                for child in node.children
            ]
            child_margins = [edge_numbers(child.attrs.get("margin")) for child in node.children]
            gap = numeric_dimension(node.attrs.get("gap", 0)) or 0.0
            if node.component == "Row" and inner_width is not None and all(box[0] is not None for box in child_boxes):
                required = sum(
                    (box[0] or 0.0) + margin[1] + margin[3]
                    for box, margin in zip(child_boxes, child_margins)
                ) + gap * max(0, len(child_boxes) - 1)
                if required > inner_width + EPSILON:
                    issues.append(
                        ValidationIssue(
                            "error",
                            node.node_id,
                            f"Row requires {required:g}vp horizontally but only {inner_width:g}vp is available",
                        )
                    )
            if node.component == "Row" and inner_height is not None and all(box[1] is not None for box in child_boxes):
                required = max(
                    (box[1] or 0.0) + margin[0] + margin[2]
                    for box, margin in zip(child_boxes, child_margins)
                )
                if required > inner_height + EPSILON:
                    issues.append(
                        ValidationIssue(
                            "error",
                            node.node_id,
                            f"Row requires {required:g}vp vertically but only {inner_height:g}vp is available",
                        )
                    )
            if node.component == "Column" and inner_height is not None and all(box[1] is not None for box in child_boxes):
                required = sum(
                    (box[1] or 0.0) + margin[0] + margin[2]
                    for box, margin in zip(child_boxes, child_margins)
                ) + gap * max(0, len(child_boxes) - 1)
                if required > inner_height + EPSILON:
                    issues.append(
                        ValidationIssue(
                            "error",
                            node.node_id,
                            f"Column requires {required:g}vp vertically but only {inner_height:g}vp is available",
                        )
                    )
            if node.component == "Column" and inner_width is not None and all(box[0] is not None for box in child_boxes):
                required = max(
                    (box[0] or 0.0) + margin[1] + margin[3]
                    for box, margin in zip(child_boxes, child_margins)
                )
                if required > inner_width + EPSILON:
                    issues.append(
                        ValidationIssue(
                            "error",
                            node.node_id,
                            f"Column requires {required:g}vp horizontally but only {inner_width:g}vp is available",
                        )
                    )
            if node.component == "Stack":
                for child, box, margin in zip(node.children, child_boxes, child_margins):
                    child_width = None if box[0] is None else box[0] + margin[1] + margin[3]
                    child_height = None if box[1] is None else box[1] + margin[0] + margin[2]
                    if inner_width is not None and child_width is not None and child_width > inner_width + EPSILON:
                        issues.append(
                            ValidationIssue(
                                "error",
                                child.node_id,
                                f"Stack child width {child_width:g} exceeds {node.node_id} inner width {inner_width:g}",
                            )
                        )
                    if inner_height is not None and child_height is not None and child_height > inner_height + EPSILON:
                        issues.append(
                            ValidationIssue(
                                "error",
                                child.node_id,
                                f"Stack child height {child_height:g} exceeds {node.node_id} inner height {inner_height:g}",
                            )
                        )
        if node.component == "Repeat":
            if parent is None or parent.component not in {"Row", "Column", "List"}:
                issues.append(
                    ValidationIssue("error", node.node_id, "Repeat parent must be Row, Column, or List")
                )
            if len(node.children) != 1:
                issues.append(
                    ValidationIssue("error", node.node_id, "Repeat must contain exactly one template component")
                )
            visible = node.attrs.get("visible", 1)
            if not isinstance(visible, (int, float)) or visible < 1:
                issues.append(ValidationIssue("error", node.node_id, "Repeat.visible must be at least 1"))
        for child in node.children:
            visit(child, node)

    visit(document.root)
    return issues


def layout_issue_code(message: str) -> str:
    lowered = message.lower()
    if "requires" in lowered and "available" in lowered:
        return "container_overflow"
    if "stack child" in lowered and "exceeds" in lowered:
        return "stack_child_overflow"
    if "checkbox" in lowered and ("intrinsic" in lowered or "fixed" in lowered):
        return "checkbox_intrinsic_overflow"
    if "button" in lowered and "capacity" in lowered:
        return "button_text_overflow"
    if "button height" in lowered and "intrinsic minimum" in lowered:
        return "button_intrinsic_overflow"
    if "progress" in lowered and "equal width and height" in lowered:
        return "progress_aspect_mismatch"
    if "cannot be combined with a fixed gap" in lowered:
        return "conflicting_spacing"
    if lowered.startswith("root "):
        return "invalid_root_layout"
    if "cannot contain alt child nodes" in lowered:
        return "invalid_child_layout"
    if lowered.startswith("repeat"):
        return "invalid_repeat_layout"
    return "layout_constraint"


def build_layout_report(
    case_name: str,
    size: str,
    alt_name: str,
    dsl_name: str,
    document: AltDocument,
    issues: list[ValidationIssue],
) -> str:
    components = {node.node_id: node.component for node in alt_nodes(document)}
    counts = {"error": 0, "warning": 0}
    entries: list[dict[str, Any]] = []
    for issue in issues:
        counts[issue.severity] = counts.get(issue.severity, 0) + 1
        entries.append(
            {
                "code": layout_issue_code(issue.message),
                "severity": issue.severity,
                "nodeId": issue.node_id,
                "component": components.get(issue.node_id),
                "message": issue.message,
            }
        )
    report = {
        "case": case_name,
        "size": size,
        "sourceAlt": alt_name,
        "generatedDsl": dsl_name,
        "status": "issues" if issues else "pass",
        "summary": {
            "total": len(issues),
            "errors": counts.get("error", 0),
            "warnings": counts.get("warning", 0),
        },
        "issues": entries,
    }
    return json.dumps(report, ensure_ascii=False, indent=2) + "\n"


def load_task_spec(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ConversionError(f"TaskSpec file does not exist: {path}")
    try:
        value = json.loads(path.read_text(encoding="utf-8-sig"))
    except json.JSONDecodeError as exc:
        raise ConversionError(f"invalid TaskSpec JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise ConversionError("TaskSpec must be a JSON object")
    if value.get("size") not in {"2x2", "2x4"}:
        raise ConversionError("TaskSpec.size must be 2x2 or 2x4")
    if not isinstance(value.get("dataModelSchema"), dict):
        raise ConversionError("TaskSpec.dataModelSchema must be an object")
    if not isinstance(value.get("eventCandidates", []), list):
        raise ConversionError("TaskSpec.eventCandidates must be an array")
    if not isinstance(value.get("assetCandidates", []), list):
        raise ConversionError("TaskSpec.assetCandidates must be an array")
    slots = value.get("presentationSlots")
    if slots is not None and not isinstance(slots, dict):
        raise ConversionError("TaskSpec.presentationSlots must be an object when present")
    return value


def sample_data_model(schema: Any) -> Any:
    if isinstance(schema, dict):
        if "sampleValue" in schema:
            return schema["sampleValue"]
        if schema.get("type") == "array" and isinstance(schema.get("items"), dict):
            return [sample_data_model(schema["items"])]
        if isinstance(schema.get("properties"), dict):
            return sample_data_model(schema["properties"])
        result: dict[str, Any] = {}
        for key, child in schema.items():
            if key in {"type", "description"} and "type" in schema:
                continue
            result[key] = sample_data_model(child)
        return result
    if isinstance(schema, list):
        return [sample_data_model(item) for item in schema]
    return schema


def escape_pointer_token(token: str) -> str:
    return token.replace("~", "~0").replace("/", "~1")


def flatten_values(value: Any, pointer: str = "") -> list[tuple[str, Any]]:
    result: list[tuple[str, Any]] = []
    if isinstance(value, dict):
        for key, child in value.items():
            result.extend(flatten_values(child, pointer + "/" + escape_pointer_token(str(key))))
    elif isinstance(value, list):
        if value:
            result.extend(flatten_values(value[0], pointer + "/0"))
        else:
            result.append((pointer, value))
    else:
        result.append((pointer or "/", value))
    return result


def flatten_collections(value: Any, pointer: str = "") -> list[str]:
    result: list[str] = []
    if isinstance(value, dict):
        for key, child in value.items():
            result.extend(flatten_collections(child, pointer + "/" + escape_pointer_token(str(key))))
    elif isinstance(value, list):
        result.append(pointer or "/")
        if value:
            result.extend(flatten_collections(value[0], pointer + "/0"))
    return result


def identifier_tokens(value: str) -> set[str]:
    expanded = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", value)
    return {token.lower() for token in re.split(r"[^A-Za-z0-9]+", expanded) if token}


def find_best_value(
    node: AltNode,
    values: list[tuple[str, Any]],
    expected: str,
    used_paths: set[str],
) -> str | None:
    node_tokens = identifier_tokens(node.node_id)
    ignored = {"text", "label", "value", "item", "row", "column", "image", "icon", "button"}
    node_tokens -= ignored
    best: tuple[int, str] | None = None
    for pointer, value in values:
        if pointer in used_paths:
            continue
        if expected == "bool" and not isinstance(value, bool):
            continue
        if expected == "number" and (isinstance(value, bool) or not isinstance(value, (int, float))):
            continue
        if expected == "text" and isinstance(value, (dict, list, bool)):
            continue
        path_tokens = identifier_tokens(pointer)
        leaf = pointer.rsplit("/", 1)[-1].lower()
        score = len(node_tokens & path_tokens) * 20
        if leaf in node_tokens:
            score += 50
        if node.node_id.lower().endswith(leaf):
            score += 20
        if best is None or score > best[0]:
            best = (score, pointer)
    if best is None or best[0] < 20:
        return None
    used_paths.add(best[1])
    return best[1]


ROLE_BINDING_TOKENS = {
    "title": {"title", "name", "heading", "subject"},
    "primary": {
        "primary",
        "value",
        "level",
        "score",
        "count",
        "total",
        "amount",
        "number",
        "metric",
        "load",
        "temperature",
        "duration",
        "time",
        "reminder",
    },
    "status": {
        "status",
        "state",
        "condition",
        "mode",
        "current",
        "connected",
        "loading",
        "risk",
        "alert",
    },
    "warning": {"warning", "risk", "alert", "danger", "notice", "level"},
    "error": {"error", "failure", "exception", "invalid"},
    "metric": {
        "metric",
        "value",
        "count",
        "total",
        "score",
        "progress",
        "level",
        "temperature",
        "duration",
        "time",
    },
    "support": {
        "support",
        "caption",
        "subtitle",
        "summary",
        "description",
        "detail",
        "meta",
        "location",
        "time",
        "date",
        "reminder",
    },
    "meta": {"meta", "description", "detail", "time", "date", "location"},
    "action": {"action", "label", "button", "command", "entry"},
    "item": {"item", "label", "option", "selection"},
    "selection": {"item", "label", "option", "selected", "selection"},
}


def binding_tokens(value: str) -> set[str]:
    """Tokenize field paths and also retain the alphabetic stem of item1-like names."""
    tokens = identifier_tokens(value)
    tokens.update(re.sub(r"\d+$", "", token) for token in list(tokens))
    return {token for token in tokens if token}


def flatten_schema_fields(
    schema: Any, pointer: str = ""
) -> list[tuple[str, Any, str]]:
    """Flatten scalar schema samples together with descriptions for binding inference."""
    fields: list[tuple[str, Any, str]] = []
    if not isinstance(schema, dict):
        return fields
    properties = schema.get("properties")
    if isinstance(properties, dict):
        for key, child in properties.items():
            escaped = escape_pointer_token(str(key))
            fields.extend(flatten_schema_fields(child, pointer + "/" + escaped))
        return fields
    if schema.get("type") == "array" and isinstance(schema.get("items"), dict):
        fields.extend(flatten_schema_fields(schema["items"], pointer + "/0"))
        return fields
    sample = schema.get("sampleValue")
    if "sampleValue" in schema:
        if sample is not None and not isinstance(sample, (dict, list)):
            description = schema.get("description")
            fields.append((pointer or "/", sample, description if isinstance(description, str) else ""))
        return fields
    for key, child in schema.items():
        if key in {"type", "description", "maxLength"}:
            continue
        escaped = escape_pointer_token(str(key))
        fields.extend(flatten_schema_fields(child, pointer + "/" + escaped))
    return fields


def infer_binding_path(
    node: AltNode,
    component: str,
    static_value: Any,
    task_spec: dict[str, Any],
    used_paths: set[str],
) -> str | None:
    """Infer a safe scalar binding when a model emitted a semantic literal.

    Exact sample matches are authoritative.  Otherwise use only node/role/path
    semantics and require a non-trivial score; unrelated UI copy remains static.
    """
    if not isinstance(static_value, (str, int, float)) or isinstance(static_value, bool):
        return None
    role = auto_role(node)
    node_tokens = binding_tokens(node.node_id)
    ignored = {"text", "label", "value", "item", "row", "column", "image", "icon", "button"}
    node_tokens -= ignored
    role_tokens = ROLE_BINDING_TOKENS.get(role, set())
    candidates: list[tuple[int, str]] = []
    for pointer, sample, description in flatten_schema_fields(task_spec["dataModelSchema"]):
        if pointer in used_paths or isinstance(sample, bool):
            continue
        if component == "Button" and not isinstance(sample, (str, int, float)):
            continue
        if component == "Button":
            field_tokens = binding_tokens(pointer + " " + description)
            conflict_tokens = {
                "load",
                "score",
                "value",
                "count",
                "total",
                "status",
                "state",
                "alert",
                "warning",
                "risk",
                "condition",
                "progress",
                "temperature",
                "current",
            }
            action_tokens = {
                "action",
                "button",
                "command",
                "event",
                "entry",
                "launch",
                "open",
                "navigate",
                "setting",
                "live",
                "app",
            }
            if field_tokens & conflict_tokens and not field_tokens & action_tokens:
                continue
        if component == "Text" and isinstance(sample, (dict, list)):
            continue
        score = 0
        if sample == static_value or str(sample) == str(static_value):
            score += 1000
        field_tokens = binding_tokens(pointer + " " + description)
        score += len(node_tokens & field_tokens) * 50
        score += len(role_tokens & field_tokens) * 30
        leaf = pointer.rsplit("/", 1)[-1].lower()
        if leaf in node_tokens:
            score += 60
        candidates.append((score, pointer))
    if not candidates:
        return None
    candidates.sort(key=lambda item: (-item[0], item[1]))
    best_score, best_path = candidates[0]
    second_score = candidates[1][0] if len(candidates) > 1 else -1
    if best_score >= 1000:
        return best_path
    if best_score < 30:
        return None
    if best_score < 60 and best_score == second_score:
        return None
    if best_score - second_score < 10 and best_score < 80:
        return None
    return best_path


def normalize_auto_bindings(
    document: AltDocument,
    task_spec: dict[str, Any],
    asc: dict[str, dict[str, Any]],
) -> tuple[dict[str, dict[str, Any]], list[str]]:
    """Prefer DataModel scalar bindings while retaining legitimate fixed copy."""
    result = copy.deepcopy(asc)
    used_paths = {
        value.get("bind")
        for value in result.values()
        if isinstance(value, dict) and isinstance(value.get("bind"), str)
    }
    notes: list[str] = []
    for node in alt_nodes(document):
        if node.component not in {"Text", "Button"}:
            continue
        attrs = result.get(node.node_id)
        if not isinstance(attrs, dict) or "expr" in attrs:
            continue
        if node.component == "Button" and isinstance(attrs.get("bind"), str):
            field_tokens = binding_tokens(attrs["bind"])
            conflict_tokens = {
                "load", "score", "value", "count", "total", "status", "state",
                "alert", "warning", "risk", "condition", "progress", "temperature", "current",
            }
            action_tokens = {
                "action", "button", "command", "event", "entry", "launch", "open",
                "navigate", "setting", "live", "app",
            }
            if field_tokens & conflict_tokens and not field_tokens & action_tokens:
                rejected_path = attrs["bind"]
                attrs.pop("bind", None)
                attrs["label"] = humanize_id(node.node_id, "Button")
                notes.append(f"{node.node_id}: rejected non-action Button binding {rejected_path!r}")
            else:
                used_paths.add(attrs["bind"])
                continue
        if "bind" in attrs:
            used_paths.add(attrs["bind"])
            continue
        key = "label" if node.component == "Button" else "text"
        if key not in attrs:
            continue
        path = infer_binding_path(node, node.component, attrs[key], task_spec, used_paths)
        if path is None:
            continue
        attrs.pop(key, None)
        attrs["bind"] = path
        used_paths.add(path)
        notes.append(f"{node.node_id}: inferred bind={path} from static {key}")
    return result, notes


def serialize_asc(document: AltDocument, asc: dict[str, dict[str, Any]]) -> str:
    """Serialize normalized semantic attributes in ALT preorder."""
    lines: list[str] = []
    for node in alt_nodes(document):
        attrs = asc.get(node.node_id)
        if not isinstance(attrs, dict) or not attrs:
            continue
        tokens = [node.component, node.node_id]
        for key, value in attrs.items():
            tokens.append(f"{key}={encode_alt_value(value)}")
        lines.append(" ".join(tokens))
    return "\n".join(lines).rstrip() + "\n"


def repair_auto_structure(document: AltDocument) -> tuple[AltDocument, list[str]]:
    """Group excess siblings into semantic containers without changing leaf meaning."""
    result = copy.deepcopy(document)
    used_ids = {node.node_id for node in alt_nodes(result)}
    notes: list[str] = []
    sequence = 0

    def new_group_id(node_id: str) -> str:
        nonlocal sequence
        while True:
            sequence += 1
            candidate = f"{node_id}_group_{sequence}"
            if candidate not in used_ids:
                used_ids.add(candidate)
                return candidate

    def visit(node: AltNode) -> None:
        for child in list(node.children):
            visit(child)
        if node.component not in {"Row", "Column"}:
            return
        while len(node.children) > 3:
            reserve_tail = node.component == "Column" and (
                node.children[-1].component == "Button"
                or node.children[-1].component in CONTAINERS | VIRTUAL_COMPONENTS
            )
            middle_end = -1 if reserve_tail else None
            middle = node.children[1:middle_end]
            if not middle:
                break
            wrapper = AltNode(
                "Column" if node.component == "Column" else "Row",
                new_group_id(node.node_id),
                {},
                middle,
            )
            node.children = node.children[:1] + [wrapper] + node.children[-1:] if reserve_tail else node.children[:1] + [wrapper]
            notes.append(f"{node.node_id}: grouped {len(middle)} excess children under {wrapper.node_id}")
            visit(wrapper)

    visit(result.root)
    return result, notes


def repair_long_auto_text_roles(
    document: AltDocument,
    task_spec: dict[str, Any],
    asc: dict[str, dict[str, Any]],
) -> list[str]:
    """Downgrade long protected facts to support before they cause clipping."""
    size = str(task_spec.get("size"))
    canvas = LAYOUT_PROFILE.get("canvas", {}).get(size, {})
    padding = float(canvas.get("padding", 12)) if isinstance(canvas, dict) else 12.0
    width = float(canvas.get("width", 140 if size == "2x2" else 300)) if isinstance(canvas, dict) else 140.0
    available = max(1.0, width - padding * 2)
    data_model = sample_data_model(task_spec["dataModelSchema"])
    density = "compact" if size == "2x2" else "regular"
    notes: list[str] = []
    for node in alt_nodes(document):
        if node.component != "Text" or auto_role(node) not in configured_single_line_roles():
            continue
        role = auto_role(node)
        content = auto_semantic_text(node, asc, data_model, [])
        font_size, font_weight = auto_font(role, content, density)
        candidate_sizes = text_font_candidates(role, content, density)
        smallest_size = candidate_sizes[-1] if candidate_sizes else font_size
        required = estimated_text_width(content, smallest_size, font_weight, TEXT_WIDTH_SAFETY)
        if required > available + EPSILON:
            old_role = auto_role(node)
            node.attrs["role"] = "support"
            notes.append(f"{node.node_id}: downgraded long role={old_role} text to support")
    return notes


def repair_missing_auto_roles(document: AltDocument) -> list[str]:
    """Fill the deterministic semantic role when a model omits it."""
    notes: list[str] = []
    for node in alt_nodes(document):
        if node.component in CONTAINERS | VIRTUAL_COMPONENTS or isinstance(node.attrs.get("role"), str):
            continue
        role = node_role(node.node_id, node.component)
        node.attrs["role"] = role
        notes.append(f"{node.node_id}: inferred missing role={role}")
    return notes


def repair_auto_depth(document: AltDocument, size: str) -> list[str]:
    """Flatten same-axis nested groups when they only add unsafe depth."""
    limits_value = LAYOUT_PROFILE.get("limits", {}).get(size, {})
    limits = limits_value if isinstance(limits_value, dict) else {}
    max_depth = limits.get("maxDepth")
    if not isinstance(max_depth, int):
        return []
    notes: list[str] = []

    def depths(node: AltNode, depth: int = 1) -> list[tuple[AltNode, int]]:
        result = [(node, depth)]
        for child in node.children:
            result.extend(depths(child, depth + 1))
        return result

    while True:
        over_depth = [(node, depth) for node, depth in depths(document.root) if depth > max_depth]
        if not over_depth:
            break
        target_depth = max(depth for _, depth in over_depth)
        candidate: tuple[AltNode, AltNode] | None = None

        def find(node: AltNode, depth: int = 1) -> None:
            nonlocal candidate
            if candidate is not None:
                return
            for child in node.children:
                if child.component == node.component and any(
                    child_depth >= target_depth
                    for _, child_depth in depths(child, depth + 1)
                ):
                    candidate = (node, child)
                    return
                find(child, depth + 1)

        find(document.root)
        if candidate is None:
            break
        parent, child = candidate
        index = parent.children.index(child)
        parent.children[index:index + 1] = child.children
        notes.append(f"{child.node_id}: flattened same-axis group to satisfy {size} maxDepth={max_depth}")
    return notes


def repair_narrow_text_row(document: AltDocument, task_spec: dict[str, Any], asc: dict[str, dict[str, Any]]) -> list[str]:
    """Use a vertical group when a 2x2 all-text header cannot fit horizontally."""
    size = str(task_spec.get("size"))
    if size != "2x2":
        return []
    canvas = LAYOUT_PROFILE.get("canvas", {}).get(size, {})
    width = float(canvas.get("width", 140)) if isinstance(canvas, dict) else 140.0
    padding = float(canvas.get("padding", 12)) if isinstance(canvas, dict) else 12.0
    available = max(1.0, width - padding * 2)
    data_model = sample_data_model(task_spec["dataModelSchema"])
    notes: list[str] = []
    for node in alt_nodes(document):
        if node.component != "Row" or len(node.children) < 2 or not all(child.component == "Text" for child in node.children):
            continue
        total = 0.0
        for child in node.children:
            content = auto_semantic_text(child, asc, data_model, [])
            role = auto_role(child)
            font_size, font_weight = auto_font(role, content, "compact")
            total += estimated_text_width(content, font_size, font_weight, TEXT_WIDTH_SAFETY)
        total += auto_gap(node, 1, "compact") * max(0, len(node.children) - 1)
        if total > available + EPSILON:
            node.component = "Column"
            notes.append(f"{node.node_id}: changed narrow all-text Row to Column")
    return notes


def prune_auto_low_priority_leaf(document: AltDocument, size: str) -> str | None:
    """Remove one low-priority leaf only after the automatic layout is infeasible."""
    if size not in {"2x2", "2x4"}:
        return None
    parent_map: dict[str, AltNode] = {}

    def visit(node: AltNode) -> None:
        for child in node.children:
            parent_map[child.node_id] = node
            visit(child)

    visit(document.root)
    candidates: list[tuple[int, AltNode]] = []
    for node in alt_nodes(document):
        parent = parent_map.get(node.node_id)
        if parent is None or len(parent.children) <= 1:
            continue
        role = auto_role(node)
        if node.component == "Image" and parent is not None and parent.component != "Row":
            candidates.append((0, node))
        elif node.component == "Progress" and role == "metric":
            candidates.append((1, node))
        elif size == "2x4" and node.component == "Text" and role == "selection":
            candidates.append((1, node))
        elif node.component == "Text" and role in {"meta", "support", "selection"}:
            candidates.append((2, node))
    if not candidates:
        return None
    _, target = sorted(candidates, key=lambda item: (item[0], item[1].node_id))[0]
    parent = parent_map[target.node_id]
    parent.children.remove(target)
    return target.node_id


def source_expression(source: Any, repeat_context: bool = False) -> Any:
    if not isinstance(source, str) or not source:
        return None
    if source.startswith("{{"):
        return source
    if source.startswith("/"):
        return f"{{{{ ${{{source}}} }}}}"
    if repeat_context:
        return f"{{{{ ${{{source}}} }}}}"
    return source


def humanize_id(node_id: str, component: str) -> str:
    lowered = node_id.lower()
    if component == "Button":
        return "查看"
    labels = {
        "title": "标题",
        "status": "状态",
        "time": "时间",
        "date": "日期",
        "value": "数值",
        "support": "信息",
        "caption": "说明",
    }
    for token, label in labels.items():
        if token in lowered:
            return label
    cleaned = re.sub(
        r"_(text|label|value|caption|button|image|icon|row|column|panel|group)$",
        "",
        node_id,
        flags=re.IGNORECASE,
    )
    return cleaned.replace("_", " ") or "信息"


def text_units(text: str) -> float:
    units = 0.0
    for character in text:
        if character.isspace():
            units += float(TEXT_UNIT_WEIGHTS.get("space", 0.35))
        elif unicodedata.east_asian_width(character) in {"W", "F"}:
            units += float(TEXT_UNIT_WEIGHTS.get("cjk", 1.0))
        elif character.isupper():
            units += float(TEXT_UNIT_WEIGHTS.get("upper", 0.68))
        elif character.islower():
            units += float(TEXT_UNIT_WEIGHTS.get("lower", 0.56))
        elif character.isdigit():
            units += float(TEXT_UNIT_WEIGHTS.get("digit", 0.62))
        else:
            units += float(TEXT_UNIT_WEIGHTS.get("other", 0.45))
    return units


def estimated_text_width(text: str, font_size: float, font_weight: int = 400, safety: float = 1.0) -> float:
    weight_factor = 1.0 + max(0, font_weight - 400) / float(tuning_value("text.weightFactorDivisor", 5000.0))
    return text_units(text) * font_size * weight_factor * safety


def node_font_size(node: AltNode, fallback: float) -> float:
    if "font" not in node.attrs:
        return fallback
    return numeric_dimension(str(node.attrs["font"]).split("/", 1)[0]) or fallback


@dataclass
class AutoMeasure:
    preferred_width: float
    preferred_height: float
    minimum_width: float
    minimum_height: float
    font_size: float = 0.0
    font_weight: int = 400
    max_lines: int = 1
    content: str = ""
    variant: str = ""


def auto_role(node: AltNode) -> str:
    role = node.attrs.get("role")
    return role if isinstance(role, str) and role else node_role(node.node_id, node.component)


def configured_single_line_roles() -> set[str]:
    rules = LAYOUT_PROFILE.get("textRules", {})
    roles = rules.get("protectedSingleLineRoles", []) if isinstance(rules, dict) else []
    return {role for role in roles if isinstance(role, str)}


def configured_text_max_lines(role: str, content: str) -> int:
    rules = LAYOUT_PROFILE.get("textRules", {})
    max_lines = rules.get("maxLines", {}) if isinstance(rules, dict) else {}
    configured = (
        max_lines.get(role, max_lines.get("default", 1))
        if isinstance(max_lines, dict)
        else 1
    )
    try:
        configured = max(1, int(configured))
    except (TypeError, ValueError):
        configured = 1
    return (
        configured
        if configured > 1 and text_units(content) > LONG_TEXT_UNITS_THRESHOLD
        else 1
    )


def text_font_candidates(role: str, content: str, density: str) -> list[int]:
    """Return descending readable font sizes for automatic text fitting."""
    preferred, _ = auto_font(role, content, density)
    rules = LAYOUT_PROFILE.get("textRules", {})
    adaptation = rules.get("fontAdaptation", {}) if isinstance(rules, dict) else {}
    config = adaptation.get(role, {}) if isinstance(adaptation, dict) else {}
    if not isinstance(config, dict):
        config = {}
    try:
        minimum = int(
            config.get(
                "minSize",
                max(FONT_ADAPTATION_DEFAULT_MIN_SIZE, preferred - 4),
            )
        )
    except (TypeError, ValueError):
        minimum = max(FONT_ADAPTATION_DEFAULT_MIN_SIZE, preferred - 4)
    try:
        step = max(1, int(config.get("step", FONT_ADAPTATION_DEFAULT_STEP)))
    except (TypeError, ValueError):
        step = FONT_ADAPTATION_DEFAULT_STEP
    minimum = min(preferred, max(FONT_ADAPTATION_ABSOLUTE_MIN_SIZE, minimum))
    candidates = list(range(preferred, minimum - 1, -step))
    if not candidates or candidates[-1] != minimum:
        candidates.append(minimum)
    return candidates


def fit_text_font(
    role: str,
    content: str,
    density: str,
    available_width: float,
) -> tuple[int, int]:
    """Choose the largest configured font that keeps a Text on one line."""
    preferred, font_weight = auto_font(role, content, density)
    for font_size in text_font_candidates(role, content, density):
        required = estimated_text_width(content, font_size, font_weight, TEXT_WIDTH_SAFETY)
        if required <= available_width + EPSILON:
            return font_size, font_weight
    return text_font_candidates(role, content, density)[-1], font_weight


def auto_semantic_text(
    node: AltNode,
    asc: dict[str, dict[str, Any]],
    data_model: Any,
    diagnostics: list[ValidationIssue],
) -> str:
    attrs = asc.get(node.node_id, {})
    key = "label" if node.component in {"Button", "Checkbox"} else "text"
    value = attrs.get(key)
    if isinstance(value, (str, int, float)):
        if isinstance(value, str) and is_dynamic(value):
            resolved, _, expression_error = resolve_text_expression(value, data_model)
            if resolved is not None:
                return resolved
            diagnostics.append(
                ValidationIssue(
                    "warning",
                    node.node_id,
                    (
                        "dynamic label/text cannot be measured safely; "
                        f"layout uses a semantic fallback ({expression_error})"
                        if expression_error
                        else "dynamic label/text has no statically measurable upper bound; layout uses a semantic fallback"
                    ),
                )
            )
            return humanize_id(node.node_id, node.component)
        return str(value)
    pointer = attrs.get("bind")
    if isinstance(pointer, str) and pointer.startswith("/"):
        try:
            resolved = pointer_get(data_model, pointer)
        except ConversionError:
            resolved = None
        if isinstance(resolved, (str, int, float, bool)):
            return str(resolved)
        diagnostics.append(
            ValidationIssue(
                "warning",
                node.node_id,
                f"dynamic text binding {pointer!r} has no scalar sampleValue; layout uses a semantic fallback",
            )
        )
    expression = attrs.get("expr")
    if isinstance(expression, str):
        resolved, _, expression_error = resolve_text_expression(expression, data_model)
        if resolved is not None:
            return resolved
        diagnostics.append(
            ValidationIssue(
                "warning",
                node.node_id,
                (
                    "complex dynamic text cannot be measured safely; "
                    f"layout uses a semantic fallback ({expression_error})"
                    if expression_error
                    else "complex dynamic text has no statically measurable upper bound; layout uses a semantic fallback"
                ),
            )
        )
    return humanize_id(node.node_id, node.component)


def auto_asset_src(
    node: AltNode,
    asc: dict[str, dict[str, Any]],
    task_spec: dict[str, Any],
    data_model: Any,
) -> str | None:
    attrs = asc.get(node.node_id, {})
    asset_index = attrs.get("asset")
    candidates = task_spec.get("assetCandidates", [])
    if isinstance(asset_index, int) and 0 <= asset_index < len(candidates):
        candidate = candidates[asset_index]
        if isinstance(candidate, dict) and isinstance(candidate.get("src"), str):
            return candidate["src"]
    src = attrs.get("src")
    if isinstance(src, str):
        return src
    pointer = attrs.get("bind")
    if isinstance(pointer, str) and pointer.startswith("/"):
        try:
            value = pointer_get(data_model, pointer)
        except ConversionError:
            value = None
        if isinstance(value, str):
            return value
    return None


def auto_font(role: str, content: str, density: str) -> tuple[int, int]:
    typography_value = LAYOUT_PROFILE.get("typography", {}).get(density, {})
    typography = typography_value if isinstance(typography_value, dict) else {}
    value = typography.get(role, typography.get("default", [14, 400]))
    if not (isinstance(value, list) and len(value) == 2):
        value = [14, 400]
    font_size, font_weight = int(value[0]), int(value[1])
    units = text_units(content)
    if role == "primary":
        banded = False
        for band in PRIMARY_FONT_BANDS:
            if units > float(band["aboveUnits"]):
                font_size = int(band["fontSize"])
                banded = True
                break
        if not banded and (
            ":" in content
            or any(token in content for token in ("分钟", "小时", "天"))
        ):
            # A compact time/duration is still a primary-sized fact, but the
            # 32fp hero treatment would consume 36vp of vertical space in a
            # 2x2 status group.  Keep the semantic role while using the safe
            # metric size; the model is still instructed to prefer metric or
            # support for time fields.
            font_size = 20
    return font_size, font_weight


def auto_gap(node: AltNode, depth: int, density: str) -> int:
    spacing = LAYOUT_PROFILE.get("spacing", {})
    if density == "compact" or len(node.children) >= 3:
        return int(spacing.get("denseGap", 4))
    if depth == 0:
        return int(spacing.get("rootGap", 8))
    return int(spacing.get("nestedGap", 6))


def checkbox_leaf_group(node: AltNode) -> bool:
    """True when a Row/Column directly holds >=2 Checkbox leaf children."""
    children = node.children
    if len(children) < 2:
        return False
    return all(child.component == "Checkbox" for child in children)


def rounded_dimension(value: float) -> int:
    return max(1, int(math.ceil(value - 1e-9)))


def allocate_axis(preferred: list[float], minimum: list[float], available: float) -> list[float]:
    if not preferred:
        return []
    preferred_total = sum(preferred)
    minimum_total = sum(minimum)
    if available >= preferred_total:
        result = list(preferred)
        result[-1] += available - preferred_total
        return result
    if available <= minimum_total:
        if minimum_total <= 0:
            return [available / len(minimum)] * len(minimum)
        scale = max(0.0, available) / minimum_total
        return [value * scale for value in minimum]
    shrink_needed = preferred_total - available
    capacity = [max(0.0, pref - floor) for pref, floor in zip(preferred, minimum)]
    capacity_total = sum(capacity)
    if capacity_total <= 0:
        return list(minimum)
    return [
        pref - shrink_needed * room / capacity_total
        for pref, room in zip(preferred, capacity)
    ]


def round_axis_dimensions(
    values: list[float], minimum: list[float], available: float
) -> list[int]:
    """Round sibling dimensions while preserving the parent's integer budget.

    Automatic layout is measured in floats but exported boxes use integer vp.
    Independently applying ceil() to every Row child can therefore add one vp
    per child and make an otherwise feasible row fail validation.  Start by
    rounding up, then take the excess from children that have slack above their
    own rounded minimum.  The final fallback keeps the parent budget intact
    when the minimums themselves cannot all be represented as integers.
    """
    if not values:
        return []
    target = max(len(values), int(math.floor(available + 1e-9)))
    rounded = [max(1, rounded_dimension(value)) for value in values]
    minimum_rounded = [max(1, rounded_dimension(value)) for value in minimum]
    excess = sum(rounded) - target
    while excess > 0:
        candidates = [
            index
            for index, value in enumerate(rounded)
            if value > minimum_rounded[index]
        ]
        if not candidates:
            candidates = [index for index, value in enumerate(rounded) if value > 1]
        if not candidates:
            break
        index = max(candidates, key=lambda item: rounded[item] - minimum_rounded[item])
        rounded[index] -= 1
        excess -= 1
    if sum(rounded) < target:
        # Preserve the preferred distribution's last-child fill behavior.
        rounded[-1] += target - sum(rounded)
    return rounded


def auto_layout_document(
    document: AltDocument,
    task_spec: dict[str, Any],
    asc: dict[str, dict[str, Any]],
) -> tuple[AltDocument, list[ValidationIssue]]:
    protocol_issues = validate_auto_protocol(document, str(task_spec.get("size")))
    errors = [issue for issue in protocol_issues if issue.severity == "error"]
    if errors:
        detail = "; ".join(f"{issue.node_id}: {issue.message}" for issue in errors)
        raise ConversionError(f"auto ALT protocol validation failed: {detail}")

    result = copy.deepcopy(document)
    size = str(result.root.attrs["card"])
    theme_name = str(result.root.attrs["theme"])
    theme = theme_values(theme_name)
    canvas_value = LAYOUT_PROFILE.get("canvas", {}).get(size, {})
    canvas = canvas_value if isinstance(canvas_value, dict) else {}
    width = float(canvas.get("width", 140 if size == "2x2" else 300))
    height = float(canvas.get("height", 140))
    root_padding = float(canvas.get("padding", 12))
    data_model = sample_data_model(task_spec["dataModelSchema"])
    diagnostics = list(protocol_issues)
    semantic_text: dict[str, str] = {}
    visible_units = 0.0
    for node in alt_nodes(result):
        if node.component in {"Text", "Button", "Checkbox"}:
            value = auto_semantic_text(node, asc, data_model, diagnostics)
            semantic_text[node.node_id] = value
            visible_units += text_units(value)
        if node.component == "Image":
            src = auto_asset_src(node, asc, task_spec, data_model)
            if not isinstance(src, str) or not src.lower().endswith(".svg"):
                diagnostics.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        "auto ALT Image must resolve to a local .svg assetCandidate",
                    )
                )

    limits_value = LAYOUT_PROFILE.get("limits", {}).get(size, {})
    limits = limits_value if isinstance(limits_value, dict) else {}
    max_units = float(limits.get("maxVisibleTextUnits", 32 if size == "2x2" else 72))
    if visible_units > max_units:
        diagnostics.append(
            ValidationIssue(
                "error",
                result.root.node_id,
                f"visible text budget is {visible_units:.1f} units; {size} allows {max_units:g}",
            )
        )
    density = "compact" if (
        size == "2x2"
        or len(alt_nodes(result))
        > (
            int(tuning_value("density.compactNodeThreshold2x2", 7))
            if size == "2x2"
            else int(tuning_value("density.compactNodeThreshold2x4", 13))
        )
        or visible_units > max_units * float(tuning_value("density.compactUnitsRatio", 0.72))
    ) else "regular"

    components = TUNING.get("components", {})
    button_profile = components.get("autoButton", components.get("button", {}))
    checkbox_profile = components.get("autoCheckbox", components.get("checkbox", {}))
    image_profile = components.get("image", {})
    progress_profile = components.get("progress", {})
    divider_profile = components.get("divider", {})
    measures: dict[str, AutoMeasure] = {}

    def measure(node: AltNode, depth: int = 0) -> AutoMeasure:
        role = auto_role(node)
        if node.component == "Text":
            content = semantic_text.get(node.node_id, "信息")
            font_size, font_weight = auto_font(role, content, density)
            maximum_lines = configured_text_max_lines(role, content)
            preferred_width = estimated_text_width(content, font_size, font_weight, TEXT_WIDTH_SAFETY)
            line_height = font_size + LINE_HEIGHT_PADDING
            candidate_sizes = text_font_candidates(role, content, density)
            minimum_font_size = candidate_sizes[-1] if candidate_sizes else font_size
            minimum_width = max(
                minimum_font_size * TEXT_MINIMUM_WIDTH_CHARS,
                estimated_text_width(content, minimum_font_size, font_weight, TEXT_WIDTH_SAFETY)
                / maximum_lines,
            )
            value = AutoMeasure(
                preferred_width,
                line_height,
                minimum_width,
                (minimum_font_size + LINE_HEIGHT_PADDING) * maximum_lines,
                font_size,
                font_weight,
                maximum_lines,
                content,
            )
        elif node.component == "Button":
            content = semantic_text.get(node.node_id, "查看")
            font_size, font_weight = auto_font("action", content, density)
            horizontal = float(button_profile.get("paddingHorizontal", 12)) * 2
            safety = float(button_profile.get("widthSafety", 4))
            preferred_width = max(
                float(button_profile.get("minimumWidth", 48)),
                estimated_text_width(content, font_size, font_weight, BUTTON_WIDTH_SAFETY) + horizontal + safety,
            )
            preferred_height = max(
                float(button_profile.get("minimumHeight", 32)),
                font_size + LINE_HEIGHT_PADDING + float(button_profile.get("paddingVertical", 8)) * 2,
            )
            value = AutoMeasure(
                preferred_width,
                preferred_height,
                preferred_width,
                preferred_height,
                font_size,
                font_weight,
                1,
                content,
            )
        elif node.component == "Checkbox":
            content = semantic_text.get(node.node_id, "选择")
            font_size = float(checkbox_profile.get("labelFontSize", 16))
            fixed = (
                float(checkbox_profile.get("controlSize", 20))
                + float(checkbox_profile.get("controlMargin", 2)) * 2
                + float(checkbox_profile.get("labelGap", 12))
            )
            preferred_width = fixed + estimated_text_width(content, font_size, 400, TEXT_WIDTH_SAFETY)
            preferred_height = float(checkbox_profile.get("outerHeight", 48))
            value = AutoMeasure(
                preferred_width,
                preferred_height,
                preferred_width,
                preferred_height,
                font_size,
                400,
                1,
                content,
            )
        elif node.component == "Image":
            if role == "primary":
                image_size = float(image_profile.get("primary2x2" if size == "2x2" else "primary2x4", 48))
            else:
                image_size = float(image_profile.get(role, image_profile.get("asset", 24)))
            value = AutoMeasure(image_size, image_size, image_size, image_size, variant="svg")
        elif node.component == "Progress":
            if role == "primary":
                ring_size = float(progress_profile.get("ring2x2" if size == "2x2" else "ring2x4", 56))
                value = AutoMeasure(ring_size, ring_size, ring_size, ring_size, variant="ring")
            else:
                linear_width = float(progress_profile.get("linearMinimumWidth", 64))
                linear_height = float(progress_profile.get("linearHeight", 8))
                value = AutoMeasure(linear_width, linear_height, linear_width, linear_height, variant="linear")
        elif node.component == "Divider":
            value = AutoMeasure(1, 1, 1, 1, variant="divider")
        elif node.component == "Repeat":
            child_measure = measure(node.children[0], depth + 1) if node.children else AutoMeasure(1, 1, 1, 1)
            visible = int(limits.get("maxVisibleListItems", 1))
            gap = auto_gap(node, depth, density)
            value = AutoMeasure(
                child_measure.preferred_width,
                child_measure.preferred_height * visible + gap * max(0, visible - 1),
                child_measure.minimum_width,
                child_measure.minimum_height * visible + gap * max(0, visible - 1),
                variant=str(visible),
            )
        else:
            child_measures = [measure(child, depth + 1) for child in node.children]
            gap = auto_gap(node, depth, density)
            if node.component == "Row":
                preferred_width = sum(item.preferred_width for item in child_measures) + gap * max(0, len(child_measures) - 1)
                preferred_height = max((item.preferred_height for item in child_measures), default=1)
                minimum_width = sum(item.minimum_width for item in child_measures) + gap * max(0, len(child_measures) - 1)
                minimum_height = max((item.minimum_height for item in child_measures), default=1)
            else:
                preferred_width = max((item.preferred_width for item in child_measures), default=1)
                preferred_height = sum(item.preferred_height for item in child_measures) + gap * max(0, len(child_measures) - 1)
                minimum_width = max((item.minimum_width for item in child_measures), default=1)
                minimum_height = sum(item.minimum_height for item in child_measures) + gap * max(0, len(child_measures) - 1)
            value = AutoMeasure(preferred_width, preferred_height, minimum_width, minimum_height)
        measures[node.node_id] = value
        return value

    measure(result.root)

    def set_leaf_layout(node: AltNode, available_width: float, available_height: float, parent: AltNode | None) -> None:
        measure_value = measures[node.node_id]
        role = auto_role(node)
        if node.component == "Text":
            fitted_font_size, fitted_font_weight = fit_text_font(
                role,
                measure_value.content,
                density,
                available_width,
            )
            measure_value.font_size = fitted_font_size
            measure_value.font_weight = fitted_font_weight
            measured_width = estimated_text_width(
                measure_value.content,
                measure_value.font_size,
                measure_value.font_weight,
                TEXT_WIDTH_SAFETY,
            )
            width_value = max(1.0, min(available_width, max(measure_value.minimum_width, measured_width)))
            required_lines = max(1, int(math.ceil(measured_width / max(width_value, 1.0))))
            if required_lines > measure_value.max_lines:
                diagnostics.append(
                    ValidationIssue(
                        "error"
                        if role == "title" or role in configured_single_line_roles()
                        else "warning",
                        node.node_id,
                        f"text {measure_value.content!r} needs {required_lines} lines but role={role} allows {measure_value.max_lines}",
                    )
                )
            lines = min(required_lines, measure_value.max_lines)
            height_value = (measure_value.font_size + LINE_HEIGHT_PADDING) * lines
            if height_value > available_height + EPSILON:
                diagnostics.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        f"text requires {height_value:g}vp vertically but only {available_height:g}vp is available",
                    )
                )
            node.attrs.update(
                {
                    "box": f"{rounded_dimension(width_value)}x{rounded_dimension(height_value)}",
                    "font": f"{int(measure_value.font_size)}/{measure_value.font_weight}",
                    "lines": lines,
                    "overflow": "none",
                    "fg": (
                        theme["status"][role]
                        if role in {"warning", "error"}
                        else theme["text"][
                            "primary"
                            if role in {"title", "primary", "status", "metric"}
                            else "secondary"
                            if role == "support"
                            else "tertiary"
                        ]
                    ),
                }
            )
            if should_protect(node.node_id, node.component, role):
                node.attrs["protect"] = True
        elif node.component == "Button":
            width_value = measure_value.preferred_width
            height_value = measure_value.preferred_height
            if width_value > available_width + EPSILON or height_value > available_height + EPSILON:
                diagnostics.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        f"Button label {measure_value.content!r} needs {width_value:g}x{height_value:g}vp but only {available_width:g}x{available_height:g}vp is available",
                    )
                )
            if parent is not None and parent.component == "Column":
                width_value = available_width
            else:
                width_value = min(width_value, available_width)
            height_value = min(height_value, available_height)
            chars = max(
                1,
                int(math.floor((width_value - BUTTON_CHARS_RESERVE) / max(measure_value.font_size, 1.0))),
            )
            node.attrs.update(
                {
                    "box": f"{rounded_dimension(width_value)}x{rounded_dimension(height_value)}",
                    "font": f"{int(measure_value.font_size)}/{measure_value.font_weight}",
                    "chars": chars,
                    "radius": rounded_dimension(height_value / 2),
                    "bg": theme["action"]["primaryBackground"],
                    "gradient": copy.deepcopy(theme["action"]["primaryGradient"]),
                    "fg": theme["action"]["primaryText"],
                    "protect": True,
                }
            )
        elif node.component == "Checkbox":
            width_value = min(measure_value.preferred_width, available_width)
            height_value = measure_value.preferred_height
            if measure_value.preferred_width > available_width + EPSILON or height_value > available_height + EPSILON:
                diagnostics.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        f"Checkbox label {measure_value.content!r} cannot fit the available {available_width:g}x{available_height:g}vp slot",
                    )
                )
            node.attrs.update(
                {
                    "box": f"{rounded_dimension(width_value)}x{rounded_dimension(height_value)}",
                    "shape": "rounded_square",
                    "selected": theme["selection"]["selected"],
                    "unselected": theme["selection"]["unselected"],
                    "mark": f"20/2/{theme['selection']['mark']}",
                    "protect": True,
                }
            )
        elif node.component == "Image":
            image_size = min(measure_value.preferred_width, available_width, available_height)
            if image_size + EPSILON < measure_value.minimum_width:
                diagnostics.append(
                    ValidationIssue("error", node.node_id, f"SVG icon requires {measure_value.minimum_width:g}vp square")
                )
            node.attrs.update(
                {
                    "size": rounded_dimension(image_size),
                    "fit": "contain",
                    "fill": theme["icon"]["primary" if role in {"primary", "asset", "title"} else "secondary"],
                }
            )
        elif node.component == "Progress":
            if measure_value.variant == "ring":
                progress_size = min(measure_value.preferred_width, available_width, available_height)
                node.attrs.update(
                    {"size": rounded_dimension(progress_size), "type": "ring", "color": theme["progress"]["fill"]}
                )
                if progress_size + EPSILON < measure_value.minimum_width:
                    diagnostics.append(
                        ValidationIssue("error", node.node_id, f"ring Progress requires {measure_value.minimum_width:g}vp square")
                    )
            else:
                width_value = max(1.0, available_width)
                height_value = float(progress_profile.get("linearHeight", 8))
                node.attrs.update(
                    {
                        "box": f"{rounded_dimension(width_value)}x{rounded_dimension(height_value)}",
                        "type": "linear",
                        "color": theme["progress"]["fill"],
                    }
                )
        elif node.component == "Divider":
            vertical = parent is not None and parent.component == "Row"
            node.attrs.update(
                {
                    "axis": "v" if vertical else "h",
                    "len": rounded_dimension(available_height if vertical else available_width),
                    "stroke": divider_profile.get("stroke", "1px"),
                    "color": theme["divider"],
                }
            )

    def layout(node: AltNode, available_width: float, available_height: float, depth: int, parent: AltNode | None = None) -> None:
        if node.component not in CONTAINERS | VIRTUAL_COMPONENTS:
            set_leaf_layout(node, available_width, available_height, parent)
            return
        if node.component == "Repeat":
            visible = int(measures[node.node_id].variant or 1)
            node.attrs["visible"] = visible
            if node.children:
                item_height = max(1.0, (available_height - auto_gap(node, depth, density) * max(0, visible - 1)) / visible)
                layout(node.children[0], available_width, item_height, depth + 1, parent)
            return

        root_node = node is result.root
        own_width = width if root_node else available_width
        own_height = height if root_node else available_height
        content_width = max(1.0, own_width - root_padding * 2) if root_node else max(1.0, own_width)
        content_height = max(1.0, own_height - root_padding * 2) if root_node else max(1.0, own_height)
        node.attrs["box"] = f"{rounded_dimension(own_width)}x{rounded_dimension(own_height)}"
        gap = auto_gap(node, depth, density)
        children = node.children
        if not children:
            return
        child_measures = [measures[child.node_id] for child in children]
        gap_total = gap * max(0, len(children) - 1)
        if node.component == "Row":
            child_widths = allocate_axis(
                [item.preferred_width for item in child_measures],
                [item.minimum_width for item in child_measures],
                max(1.0, content_width - gap_total),
            )
            child_widths = round_axis_dimensions(
                child_widths,
                [item.minimum_width for item in child_measures],
                max(1.0, content_width - gap_total),
            )
            minimum_width_total = sum(item.minimum_width for item in child_measures)
            if (
                minimum_width_total + gap_total > content_width + EPSILON
                and gap > COLUMN_COMPACT_GAP_MINIMUM
            ):
                compact_gap = max(COLUMN_COMPACT_GAP_MINIMUM, gap - COLUMN_COMPACT_GAP_MINIMUM)
                compact_gap_total = compact_gap * max(0, len(children) - 1)
                if minimum_width_total + compact_gap_total <= content_width + EPSILON:
                    gap = compact_gap
                    gap_total = compact_gap_total
                    child_widths = allocate_axis(
                        [item.preferred_width for item in child_measures],
                        [item.minimum_width for item in child_measures],
                        max(1.0, content_width - gap_total),
                    )
                    child_widths = round_axis_dimensions(
                        child_widths,
                        [item.minimum_width for item in child_measures],
                        max(1.0, content_width - gap_total),
                    )
            if minimum_width_total + gap_total > content_width + EPSILON:
                diagnostics.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        f"auto Row minimum width exceeds the available {content_width:g}vp",
                    )
                )
            centered_compact_header = (
                size == "2x2"
                and parent is result.root
                and len(children) <= 2
                and all(child.component in {"Image", "Text"} for child in children)
                and all(auto_role(child) in {"asset", "title", "status", "support", "meta"} for child in children)
            )
            if checkbox_leaf_group(node):
                node.attrs.update({"main": "between", "cross": "start"})
                node.attrs.pop("gap", None)
            else:
                node.attrs.update(
                    {"gap": gap, "main": "center" if centered_compact_header else "start", "cross": "center"}
                )
            for child, child_width in zip(children, child_widths):
                child_height = content_height if child.component in CONTAINERS | VIRTUAL_COMPONENTS else min(
                    content_height, measures[child.node_id].preferred_height
                )
                layout(child, max(1.0, child_width), max(1.0, child_height), depth + 1, node)
        else:
            preferred_heights: list[float] = []
            minimum_heights: list[float] = []
            for child, item in zip(children, child_measures):
                if child.component == "Text":
                    fitted_size, _ = fit_text_font(
                        auto_role(child), item.content, density, content_width
                    )
                    fitted_width = estimated_text_width(
                        item.content, fitted_size, item.font_weight, TEXT_WIDTH_SAFETY
                    )
                    lines = min(
                        item.max_lines,
                        max(1, int(math.ceil(fitted_width / content_width))),
                    )
                    text_height = (fitted_size + 4) * lines
                    preferred_heights.append(text_height)
                    minimum_heights.append(text_height)
                else:
                    preferred_heights.append(item.preferred_height)
                    minimum_heights.append(item.minimum_height)
            child_heights = allocate_axis(
                preferred_heights,
                minimum_heights,
                max(1.0, content_height - gap_total),
            )
            child_heights = round_axis_dimensions(
                child_heights,
                minimum_heights,
                max(1.0, content_height - gap_total),
            )
            minimum_height_total = sum(minimum_heights)
            if (
                minimum_height_total + gap_total > content_height + EPSILON
                and gap > COLUMN_COMPACT_GAP_MINIMUM
            ):
                compact_gap = max(COLUMN_COMPACT_GAP_MINIMUM, gap - COLUMN_COMPACT_GAP_MINIMUM)
                compact_gap_total = compact_gap * max(0, len(children) - 1)
                if minimum_height_total + compact_gap_total <= content_height + EPSILON:
                    gap = compact_gap
                    gap_total = compact_gap_total
                    child_heights = allocate_axis(
                        preferred_heights,
                        minimum_heights,
                        max(1.0, content_height - gap_total),
                    )
                    child_heights = round_axis_dimensions(
                        child_heights,
                        minimum_heights,
                        max(1.0, content_height - gap_total),
                    )
            if minimum_height_total + gap_total > content_height + EPSILON:
                diagnostics.append(
                    ValidationIssue(
                        "error",
                        node.node_id,
                        f"auto Column minimum height exceeds the available {content_height:g}vp",
                    )
                )
            has_bottom_action = children[-1].component == "Button"
            anchor_action = has_bottom_action and len(children) >= 3
            if checkbox_leaf_group(node):
                node.attrs.update({"main": "between", "cross": "start"})
                node.attrs.pop("gap", None)
            elif root_node and node.component == "Column":
                node.attrs.update({"main": "between", "cross": "center"})
                node.attrs.pop("gap", None)
            elif (
                sum(preferred_heights) + gap_total
                < content_height - COLUMN_BOTTOM_ACTION_ANCHOR_GAP
                and anchor_action
            ):
                node.attrs.update({"main": "between", "cross": "center"})
                node.attrs.pop("gap", None)
            else:
                node.attrs.update(
                    {
                        "gap": gap,
                        "main": (
                            "center"
                            if sum(preferred_heights) < content_height * COLUMN_CENTER_MAIN_RATIO
                            else "start"
                        ),
                        "cross": "center",
                    }
                )
            for child, child_height in zip(children, child_heights):
                if child.component in CONTAINERS | VIRTUAL_COMPONENTS or child.component in {
                    "Text",
                    "Progress",
                    "Divider",
                    "Button",
                }:
                    child_width = content_width
                else:
                    child_width = min(content_width, measures[child.node_id].preferred_width)
                layout(child, max(1.0, child_width), max(1.0, child_height), depth + 1, node)

    root_role = result.root.attrs.get("role")
    result.root.attrs = {
        "card": size,
        "theme": theme_name,
        **({"role": root_role} if isinstance(root_role, str) else {}),
    }
    layout(result.root, width, height, 0)
    result.root.attrs.update(
        {
            "pad": rounded_dimension(root_padding),
            "radius": int(canvas.get("radius", 18 if size == "2x2" else 22)),
            "clip": True,
            "bg": theme["surface"]["root"],
            "gradient": copy.deepcopy(theme["surface"]["gradient"]),
            "border": theme["surface"]["border"],
        }
    )
    return result, diagnostics


class SemanticResolver:
    def __init__(self, task_spec: dict[str, Any]):
        self.task_spec = task_spec
        self.data_model = sample_data_model(task_spec["dataModelSchema"])
        self.values = flatten_values(self.data_model)
        self.collections = flatten_collections(self.data_model)
        self.slots = task_spec.get("presentationSlots", {})
        self.events = task_spec.get("eventCandidates", [])
        self.assets = task_spec.get("assetCandidates", [])
        self.used_paths: set[str] = set()
        self.warnings: list[str] = []

    def slot(self, node_id: str) -> dict[str, Any]:
        value = self.slots.get(node_id, {}) if isinstance(self.slots, dict) else {}
        return value if isinstance(value, dict) else {}

    def text(self, node: AltNode, repeat_context: bool) -> Any:
        slot = self.slot(node.node_id)
        if "value" in slot:
            return slot["value"]
        if "source" in slot:
            return source_expression(slot["source"], repeat_context)
        generic_static_ids = {"title", "title_text", "header_title", "card_title"}
        pointer = None
        if node.node_id.lower() not in generic_static_ids:
            pointer = find_best_value(node, self.values, "text", self.used_paths)
        if pointer:
            self.warnings.append(f"{node.node_id}: matched Text to schema path {pointer} by node ID")
            return source_expression(pointer)
        fallback = humanize_id(node.node_id, node.component)
        self.warnings.append(f"{node.node_id}: no presentation slot; using fallback text {fallback!r}")
        return fallback

    def image(self, node: AltNode) -> str:
        slot = self.slot(node.node_id)
        if isinstance(slot.get("src"), str):
            return slot["src"]
        asset_index = slot.get("assetCandidate")
        if isinstance(asset_index, int) and 0 <= asset_index < len(self.assets):
            source = self.assets[asset_index].get("src")
            if isinstance(source, str) and source:
                return source
        node_tokens = identifier_tokens(node.node_id)
        best: tuple[int, str] | None = None
        for asset in self.assets:
            if not isinstance(asset, dict) or not isinstance(asset.get("src"), str):
                continue
            haystack = str(asset.get("src", "")) + " " + str(asset.get("description", ""))
            score = len(node_tokens & identifier_tokens(haystack)) * 20
            candidate = (score, asset["src"])
            if best is None or candidate[0] > best[0]:
                best = candidate
        if best is None:
            raise ConversionError(f"{node.node_id}: Image has no matching TaskSpec assetCandidate")
        self.warnings.append(f"{node.node_id}: selected asset {best[1]!r} without presentation slot")
        return best[1]

    def progress(self, node: AltNode, repeat_context: bool) -> tuple[Any, Any]:
        slot = self.slot(node.node_id)
        source = slot.get("source")
        if source is None:
            source = find_best_value(node, self.values, "number", self.used_paths)
            if source:
                self.warnings.append(f"{node.node_id}: matched Progress to schema path {source} by node ID")
        value = source_expression(source, repeat_context) if source is not None else 0
        total_source = slot.get("totalSource")
        total = source_expression(total_source, repeat_context) if total_source is not None else slot.get("total", 100)
        return value, total

    def checkbox(self, node: AltNode, repeat_context: bool) -> tuple[str, Any]:
        slot = self.slot(node.node_id)
        label = str(slot.get("label", ""))
        source = slot.get("source")
        if source is None:
            source = find_best_value(node, self.values, "bool", self.used_paths)
            if source:
                self.warnings.append(f"{node.node_id}: matched Checkbox to schema path {source} by node ID")
        selected = source_expression(source, repeat_context) if source is not None else False
        return label, selected

    def event(self, node: AltNode, allow_fallback: bool) -> list[dict[str, Any]] | None:
        slot = self.slot(node.node_id)
        index = slot.get("eventCandidate")
        if isinstance(index, int) and 0 <= index < len(self.events):
            candidate = self.events[index]
        elif allow_fallback and self.events:
            candidate = self.events[0]
            self.warnings.append(f"{node.node_id}: selected first eventCandidate without presentation slot")
        else:
            return None
        if not isinstance(candidate, dict) or not isinstance(candidate.get("call"), str):
            return None
        event: dict[str, Any] = {"call": candidate["call"]}
        if isinstance(candidate.get("args"), dict):
            event["args"] = candidate["args"]
        return [event]

    def repeat_path(self, node: AltNode) -> str:
        slot = self.slot(node.node_id)
        source = slot.get("source")
        if isinstance(source, str) and source.startswith("/"):
            return source
        if self.collections:
            path = self.collections[0]
            self.warnings.append(f"{node.node_id}: selected collection {path} without presentation slot")
            return path
        raise ConversionError(f"{node.node_id}: Repeat requires a collection presentation slot")


def apply_alt_styles(node: AltNode) -> tuple[dict[str, Any], dict[str, Any]]:
    top: dict[str, Any] = {"id": node.node_id, "component": node.component}
    styles: dict[str, Any] = {}
    attrs = node.attrs

    if "size" in attrs:
        size = parse_dimension_token(attrs["size"])
        if size is None:
            raise ConversionError(f"{node.node_id}: invalid size")
        styles["width"] = size
        styles["height"] = size
    elif "box" in attrs:
        width, height = parse_box(attrs["box"])
        if width is not None:
            styles["width"] = width
        if height is not None:
            styles["height"] = height

    if "pad" in attrs:
        styles["padding"] = parse_edge(attrs["pad"])
    if "margin" in attrs:
        styles["margin"] = parse_edge(attrs["margin"])
    if "gap" in attrs:
        if node.component == "List":
            top["space"] = attrs["gap"]
        elif node.component in {"Row", "Column"}:
            top["itemMargin"] = attrs["gap"]
    if "main" in attrs:
        styles["justifyContent"] = MAIN_TO_DSL.get(str(attrs["main"]), attrs["main"])
    if "cross" in attrs:
        styles["alignItems"] = attrs["cross"]
    if "align" in attrs:
        styles["alignContent"] = attrs["align"]
    if "wrap" in attrs:
        top["wrap"] = attrs["wrap"]
    if "grow" in attrs:
        styles["layoutWeight"] = attrs["grow"]
    if "shrink" in attrs:
        styles["flexShrink"] = attrs["shrink"]

    if node.component in {"Text", "Button"} and "font" in attrs:
        parts = str(attrs["font"]).split("/", 1)
        font_size = parse_dimension_token(parts[0])
        if font_size is None:
            raise ConversionError(f"{node.node_id}: invalid font size")
        styles["fontSize"] = font_size
        if len(parts) == 2:
            weight = parse_dimension_token(parts[1])
            styles["fontWeight"] = weight if weight is not None else parts[1]
    if node.component == "Text":
        # Text alignment is a rendering default, not a model-facing layout
        # decision.  Preserve an explicit ALT value, otherwise center every
        # Text box so sparse automatic trees do not inherit renderer defaults.
        styles.setdefault("textAlign", "center")
        if "lines" in attrs:
            styles["maxLines"] = attrs["lines"]
        if "overflow" in attrs:
            styles["textOverflow"] = attrs["overflow"]
        if "text" in attrs:
            styles["textAlign"] = attrs["text"]
    elif node.component == "Image":
        if "fit" in attrs:
            styles["objectFit"] = attrs["fit"]
        if "ratio" in attrs:
            styles["aspectRatio"] = attrs["ratio"]
        if "fill" in attrs:
            styles["fillColor"] = attrs["fill"]
    elif node.component == "Progress":
        if "type" in attrs:
            styles["type"] = attrs["type"]
        if "color" in attrs:
            styles["color"] = attrs["color"]
    elif node.component == "Divider":
        vertical = str(attrs.get("axis", "h")) == "v"
        styles["vertical"] = vertical
        if "len" in attrs:
            styles["height" if vertical else "width"] = attrs["len"]
        if "stroke" in attrs:
            styles["strokeWidth"] = attrs["stroke"]
        if "color" in attrs:
            styles["color"] = attrs["color"]
    elif node.component == "Checkbox":
        if "shape" in attrs:
            styles["shape"] = attrs["shape"]
        if "selected" in attrs:
            styles["selectedColor"] = attrs["selected"]
        if "unselected" in attrs:
            styles["unSelectedColor"] = attrs["unselected"]
        if "mark" in attrs:
            parts = str(attrs["mark"]).split("/")
            if len(parts) != 3:
                raise ConversionError(f"{node.node_id}: mark must use size/strokeWidth/strokeColor")
            mark_size = parse_dimension_token(parts[0])
            mark_width = parse_dimension_token(parts[1])
            styles["mark"] = {
                "size": mark_size if mark_size is not None else parts[0],
                "strokeWidth": mark_width if mark_width is not None else parts[1],
                "strokeColor": parts[2],
            }
    elif node.component == "List":
        if "direction" in attrs:
            styles["listDirection"] = attrs["direction"]
        if "scroll" in attrs:
            styles["scrollBar"] = attrs["scroll"]

    if "radius" in attrs:
        styles["borderRadius"] = attrs["radius"]
    if "bg" in attrs:
        styles["backgroundColor"] = attrs["bg"]
    if "fg" in attrs and node.component in {"Text", "Button"}:
        styles["fontColor"] = attrs["fg"]
    if "gradient" in attrs:
        if not isinstance(attrs["gradient"], dict):
            raise ConversionError(f"{node.node_id}: gradient must be a JSON object")
        styles["linearGradient"] = attrs["gradient"]
    if "border" in attrs:
        parts = str(attrs["border"]).split("/", 1)
        width = parse_dimension_token(parts[0])
        styles["borderWidth"] = width if width is not None else parts[0]
        if len(parts) == 2:
            styles["borderColor"] = parts[1]
    if "shadow" in attrs:
        styles["shadow"] = attrs["shadow"]
    if attrs.get("clip") is True:
        styles["clip"] = True
    if "style" in attrs:
        if not isinstance(attrs["style"], dict):
            raise ConversionError(f"{node.node_id}: style must be a JSON object")
        styles.update(copy.deepcopy(attrs["style"]))
    return top, styles


def alt_to_dsl(
    document: AltDocument,
    task_spec: dict[str, Any],
    strict_layout: bool = True,
    validate_content: bool = True,
) -> tuple[str, list[str]]:
    size = task_spec.get("size")
    if size not in {"2x2", "2x4"}:
        raise ConversionError(f"TaskSpec.size must be 2x2 or 2x4, got {size!r}")
    issues = validate_layout(document, size)
    errors = [issue for issue in issues if issue.severity == "error"]
    if strict_layout and errors:
        detail = "; ".join(f"{issue.node_id}: {issue.message}" for issue in errors)
        raise ConversionError(f"ALT layout validation failed: {detail}")

    resolver = SemanticResolver(task_spec)
    if not strict_layout:
        resolver.warnings.extend(
            f"[{issue.node_id}] {issue.message}" for issue in errors
        )
    components: list[dict[str, Any]] = []

    def compile_node(node: AltNode, repeat_context: bool = False) -> None:
        if node.component == "Repeat":
            for child in node.children:
                compile_node(child, True)
            return
        top, styles = apply_alt_styles(node)
        repeat_children = [child for child in node.children if child.component == "Repeat"]
        direct_children = [child for child in node.children if child.component != "Repeat"]
        if repeat_children and direct_children:
            raise ConversionError(f"{node.node_id}: container cannot mix Repeat and direct children")
        if len(repeat_children) > 1:
            raise ConversionError(f"{node.node_id}: container can contain only one Repeat")
        if repeat_children:
            repeat = repeat_children[0]
            if len(repeat.children) != 1:
                raise ConversionError(f"{repeat.node_id}: Repeat must contain exactly one template")
            top["children"] = {
                "componentId": repeat.children[0].node_id,
                "path": resolver.repeat_path(repeat),
            }
        elif node.component in CONTAINERS:
            top["children"] = [child.node_id for child in direct_children]

        if node.component == "Text":
            top["content"] = resolver.text(node, repeat_context)
        elif node.component == "Image":
            top["src"] = resolver.image(node)
        elif node.component == "Progress":
            top["value"], top["total"] = resolver.progress(node, repeat_context)
        elif node.component == "Button":
            slot = resolver.slot(node.node_id)
            top["label"] = str(slot.get("label", humanize_id(node.node_id, "Button")))
            button_width, _ = node_box(node, intrinsic=False)
            chars = node.attrs.get("chars")
            if validate_content and isinstance(chars, int) and not is_dynamic(top["label"]):
                if len(top["label"]) > chars:
                    raise ConversionError(
                        f"{node.node_id}: Button label {top['label']!r} contains {len(top['label'])} "
                        f"characters but ALT chars={chars}"
                    )
            if validate_content and button_width is not None:
                required_width = (
                    estimated_text_width(top["label"], node_font_size(node, BUTTON_LABEL_FONT_FALLBACK))
                    + BUTTON_CHARS_RESERVE
                )
                if button_width + EPSILON < required_width:
                    raise ConversionError(
                        f"{node.node_id}: Button width {button_width:g} cannot contain label {top['label']!r}; "
                        f"requires at least {required_width:g}vp"
                    )
            event = resolver.event(node, allow_fallback=True)
            if event:
                top["onClick"] = event
        elif node.component == "Checkbox":
            top["label"], top["select"] = resolver.checkbox(node, repeat_context)
            checkbox_width, _ = node_box(node, intrinsic=False)
            if validate_content and node.attrs.get("protect") is True and checkbox_width is not None and top["label"]:
                checkbox_profile = checkbox_layout_profile(automatic=is_auto_layout(document))
                label_font_size = float(checkbox_profile.get("labelFontSize", 16))
                fixed_width = (
                    float(checkbox_profile.get("controlSize", 20))
                    + float(checkbox_profile.get("controlMargin", 2)) * 2
                    + float(checkbox_profile.get("labelGap", 12))
                )
                required_width = estimated_text_width(top["label"], label_font_size) + fixed_width
                if checkbox_width + EPSILON < required_width:
                    raise ConversionError(
                        f"{node.node_id}: Checkbox width {checkbox_width:g} cannot contain protected label "
                        f"{top['label']!r}; requires at least {required_width:g}vp"
                    )
            slot = resolver.slot(node.node_id)
            if isinstance(slot.get("group"), str):
                top["group"] = slot["group"]
            event = resolver.event(node, allow_fallback=False)
            if event:
                top["onClick"] = event
        else:
            event = resolver.event(node, allow_fallback=False)
            if event:
                top["onClick"] = event

        if styles:
            top["styles"] = styles
        components.append(top)
        for child in node.children:
            compile_node(child, repeat_context)

    compile_node(document.root)

    width, height = (140, 140) if size == "2x2" else (300, 140)
    radius = 18 if size == "2x2" else 22
    root_component = next((item for item in components if item["id"] == document.root.node_id), None)
    if root_component is None:
        raise ConversionError("compiled root component is missing")
    root_styles = root_component.setdefault("styles", {})
    root_styles["width"] = width
    root_styles["height"] = height
    root_styles.setdefault("padding", 12)
    root_styles.setdefault("borderRadius", radius)
    root_styles.setdefault("clip", True)
    if not any(key in root_styles for key in ("backgroundColor", "linearGradient", "backgroundImage")):
        root_styles["backgroundColor"] = "#FFFFFFFF"
        resolver.warnings.append(f"{document.root.node_id}: root had no surface background; added #FFFFFFFF")

    create = {
        "version": "v0.9",
        "createSurface": {
            "surfaceId": SURFACE_ID,
            "catalogId": CATALOG_ID,
            "width": width,
            "height": height,
        },
    }
    update = {
        "version": "v0.9",
        "updateComponents": {
            "surfaceId": SURFACE_ID,
            "root": document.root.node_id,
            "components": components,
        },
    }
    data = {
        "version": "v0.9",
        "updateDataModel": {
            "surfaceId": SURFACE_ID,
            "path": "/",
            "value": resolver.data_model,
        },
    }
    text = "\n".join(compact_json(message) for message in (create, update, data)) + "\n"
    return text, resolver.warnings


def atomic_write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(text, encoding="utf-8")
    temporary.replace(path)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Batch-convert A2UI GenUI DSL and ALT+ASC files in dataset Case directories."
    )
    parser.add_argument(
        "-o",
        required=True,
        type=Path,
        metavar="DATASET_DIR",
        help="Dataset directory whose immediate child directories are Cases.",
    )
    parser.add_argument(
        "--dsl_name",
        default=DEFAULT_DSL_NAME,
        help=f"DSL filename inside each Case. Default: {DEFAULT_DSL_NAME}.",
    )
    parser.add_argument(
        "--alt_name",
        default=DEFAULT_ALT_NAME,
        help=f"ALT filename inside each Case. Default: {DEFAULT_ALT_NAME}.",
    )
    parser.add_argument(
        "--asc_name",
        default=DEFAULT_ASC_NAME,
        help=f"ASC semantic companion filename inside each Case. Default: {DEFAULT_ASC_NAME}.",
    )
    parser.add_argument(
        "--layout_report_name",
        default=DEFAULT_LAYOUT_REPORT_NAME,
        help=(
            "JSON layout report filename written inside each Case during t2d. "
            f"Default: {DEFAULT_LAYOUT_REPORT_NAME}."
        ),
    )
    parser.add_argument(
        "--taskspec_name",
        default=DEFAULT_TASKSPEC_NAME,
        help=f"TaskSpec filename inside each Case. Default: {DEFAULT_TASKSPEC_NAME}.",
    )
    parser.add_argument(
        "--mode",
        required=True,
        choices=("d2t", "t2d"),
        help="d2t converts DSL to ALT; t2d converts ALT plus TaskSpec to DSL.",
    )
    parser.add_argument(
        "--legacy_alt",
        action="store_true",
        help=(
            "During d2t, emit the geometry-preserving ALT v0.1 form instead of the default "
            "automatic-layout ALT. t2d always auto-detects both forms."
        ),
    )
    parser.add_argument(
        "--allow_layout_issues",
        "--allow-layout-issues",
        dest="allow_layout_issues",
        action="store_true",
        help=(
            "During d2t, keep an automatic ALT even when the source structure is not "
            "currently feasible; emit diagnostics for later model/evaluation use. "
            "t2d still refuses hard layout errors."
        ),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    dataset_dir = args.o.resolve()
    if not dataset_dir.is_dir():
        LOGGER.error("dataset directory does not exist: %s", dataset_dir)
        return 2
    cases = sorted((path for path in dataset_dir.iterdir() if path.is_dir()), key=lambda path: path.name.lower())
    if not cases:
        LOGGER.error("dataset directory has no Case subdirectories: %s", dataset_dir)
        return 2

    successes = 0
    failures = 0
    LOGGER.info(
        "START mode=%s dataset=%s cases=%d profile=%s",
        args.mode,
        dataset_dir,
        len(cases),
        GENUI_PROFILE,
    )
    for index, case_dir in enumerate(cases, 1):
        prefix = f"[{index}/{len(cases)}] [{case_dir.name}]"
        LOGGER.info("%s START", prefix)
        try:
            dsl_path = case_dir / args.dsl_name
            alt_path = case_dir / args.alt_name
            asc_path = case_dir / args.asc_name
            layout_report_path = case_dir / args.layout_report_name
            task_path = case_dir / args.taskspec_name
            if args.mode == "d2t":
                task_spec = load_task_spec(task_path)
                document, size, diagnostics, orphan_count = dsl_to_alt(dsl_path, task_spec)
                issues = validate_layout(document, size)
                for diagnostic in diagnostics:
                    LOGGER.warning("%s %s", prefix, diagnostic)
                for issue in issues:
                    LOGGER.warning("%s [%s] %s", prefix, issue.node_id, issue.message)
                if orphan_count:
                    raise ConversionError(
                        f"canonical DSL cannot contain {orphan_count} unmounted component(s)"
                    )
                base_text, base_warnings = alt_to_dsl(
                    document,
                    task_spec,
                    strict_layout=False,
                    validate_content=False,
                )
                for warning in base_warnings:
                    LOGGER.warning("%s ASC base: %s", prefix, warning)
                output_document = document if args.legacy_alt else simplify_to_auto(document, size)
                asc_text = build_asc(
                    read_jsonl_messages(dsl_path),
                    output_document,
                    task_spec,
                )
                if not args.legacy_alt:
                    protocol_issues = validate_auto_protocol(output_document, task_spec["size"])
                    for issue in protocol_issues:
                        LOGGER.warning("%s auto ALT [%s] %s", prefix, issue.node_id, issue.message)
                    protocol_errors = [issue for issue in protocol_issues if issue.severity == "error"]
                    if protocol_errors and not args.allow_layout_issues:
                        detail = "; ".join(
                            f"{issue.node_id}: {issue.message}" for issue in protocol_errors
                        )
                        raise ConversionError(f"migrated automatic ALT is invalid: {detail}")
                    migration_issues: list[ValidationIssue] = []
                    if args.allow_layout_issues:
                        LOGGER.warning(
                            "%s keeping automatic ALT with diagnostics; strict t2d remains unchanged",
                            prefix,
                        )
                    else:
                        migrated_asc = parse_asc_text(asc_text, output_document)
                        validate_auto_asc(output_document, task_spec, migrated_asc)
                        compiled_migration, migration_issues = auto_layout_document(
                            output_document,
                            task_spec,
                            migrated_asc,
                        )
                        migration_issues += validate_layout(compiled_migration, task_spec["size"])
                        migration_errors = [issue for issue in migration_issues if issue.severity == "error"]
                        if migration_errors:
                            detail = "; ".join(
                                f"{issue.node_id}: {issue.message}" for issue in migration_errors
                            )
                            raise ConversionError(f"migrated automatic ALT is infeasible: {detail}")
                atomic_write(alt_path, serialize_alt(output_document))
                atomic_write(asc_path, asc_text)
                if not args.legacy_alt and args.allow_layout_issues:
                    atomic_write(
                        layout_report_path,
                        build_layout_report(
                            case_dir.name,
                            task_spec["size"],
                            alt_path.name,
                            dsl_path.name,
                            output_document,
                            protocol_issues + migration_issues,
                        ),
                    )
                LOGGER.info(
                    "%s DONE dsl=%s alt=%s asc=%s protocol=%s nodes=%d",
                    prefix,
                    dsl_path.name,
                    alt_path.name,
                    asc_path.name,
                    "legacy" if args.legacy_alt else "auto",
                    len([line for line in asc_text.splitlines() if line.strip()]),
                )
            else:
                task_spec = load_task_spec(task_path)
                document = parse_alt(alt_path)
                structure_notes: list[str] = []
                binding_notes: list[str] = []
                role_notes: list[str] = []
                depth_notes: list[str] = []
                missing_role_notes: list[str] = []
                narrow_row_notes: list[str] = []
                auto_alt_changed = False
                auto_asc_changed = False
                if is_auto_layout(document):
                    document, structure_notes = repair_auto_structure(document)
                    depth_notes = repair_auto_depth(document, str(task_spec.get("size")))
                    if depth_notes:
                        document, extra_structure_notes = repair_auto_structure(document)
                        structure_notes.extend(extra_structure_notes)
                    missing_role_notes = repair_missing_auto_roles(document)
                    for note in structure_notes:
                        LOGGER.info("%s repair: %s", prefix, note)
                    for note in depth_notes + missing_role_notes:
                        LOGGER.info("%s repair: %s", prefix, note)
                    auto_alt_changed = bool(structure_notes or depth_notes or missing_role_notes)
                asc = parse_asc(asc_path, document)
                compiled_document = document
                auto_issues: list[ValidationIssue] = []
                if is_auto_layout(document):
                    asc, binding_notes = normalize_auto_bindings(document, task_spec, asc)
                    role_notes = repair_long_auto_text_roles(document, task_spec, asc)
                    narrow_row_notes = repair_narrow_text_row(document, task_spec, asc)
                    for note in binding_notes + role_notes:
                        LOGGER.info("%s semantic repair: %s", prefix, note)
                    for note in narrow_row_notes:
                        LOGGER.info("%s layout repair: %s", prefix, note)
                    auto_alt_changed = auto_alt_changed or bool(role_notes or narrow_row_notes)
                    auto_asc_changed = bool(binding_notes)
                    validate_auto_asc(document, task_spec, asc)
                    protocol_issues = validate_auto_protocol(document, str(task_spec.get("size")))
                    protocol_errors = [issue for issue in protocol_issues if issue.severity == "error"]
                    if protocol_errors:
                        atomic_write(
                            layout_report_path,
                            build_layout_report(
                                case_dir.name,
                                task_spec["size"],
                                alt_path.name,
                                dsl_path.name,
                                document,
                                protocol_issues,
                            ),
                        )
                        detail = "; ".join(
                            f"{issue.node_id}: {issue.message}" for issue in protocol_errors
                        )
                        raise ConversionError(f"automatic ALT protocol is invalid: {detail}")
                    compiled_document, auto_issues = auto_layout_document(document, task_spec, asc)
                    for _ in range(4):
                        hard_auto_issues = [
                            issue for issue in auto_issues if issue.severity == "error"
                        ]
                        if not hard_auto_issues:
                            break
                        removed_id = prune_auto_low_priority_leaf(
                            document, str(task_spec.get("size"))
                        )
                        if removed_id is None:
                            break
                        LOGGER.info(
                            "%s layout repair: removed low-priority node %s",
                            prefix,
                            removed_id,
                        )
                        asc.pop(removed_id, None)
                        auto_alt_changed = True
                        auto_asc_changed = True
                        validate_auto_asc(document, task_spec, asc)
                        compiled_document, auto_issues = auto_layout_document(document, task_spec, asc)
                issues = auto_issues + validate_layout(compiled_document, task_spec["size"])
                base_text, warnings = alt_to_dsl(
                    compiled_document,
                    task_spec,
                    strict_layout=False,
                    validate_content=False,
                )
                dsl_text = apply_asc_to_dsl(base_text, document, task_spec, asc)
                if not is_auto_layout(document):
                    for warning in warnings:
                        LOGGER.warning("%s %s", prefix, warning)
                atomic_write(
                    layout_report_path,
                    build_layout_report(
                        case_dir.name,
                        task_spec["size"],
                        alt_path.name,
                        dsl_path.name,
                        compiled_document,
                        issues,
                    ),
                )
                hard_errors = [issue for issue in issues if issue.severity == "error"]
                if is_auto_layout(document) and hard_errors:
                    detail = "; ".join(f"{issue.node_id}: {issue.message}" for issue in hard_errors)
                    raise ConversionError(f"automatic layout is infeasible: {detail}")
                if is_auto_layout(document):
                    if auto_alt_changed:
                        atomic_write(alt_path, serialize_alt(document))
                    if auto_asc_changed:
                        atomic_write(asc_path, serialize_asc(document, asc))
                atomic_write(dsl_path, dsl_text)
                LOGGER.info(
                    "%s DONE alt=%s asc=%s dsl=%s layout_report=%s issues=%d",
                    prefix,
                    alt_path.name,
                    asc_path.name,
                    dsl_path.name,
                    layout_report_path.name,
                    len(issues),
                )
            successes += 1
        except Exception as exc:
            failures += 1
            LOGGER.error("%s FAILED %s", prefix, exc)
    LOGGER.info("SUMMARY mode=%s total=%d success=%d failed=%d", args.mode, len(cases), successes, failures)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
