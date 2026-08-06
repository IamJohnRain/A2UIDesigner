(() => {
  const embeddedFontFamily = 'A2UI HarmonyOS Sans SC';
  const embeddedFontFaces = [
    ['Thin', 100],
    ['Light', 300],
    ['Regular', 400],
    ['Medium', 500],
    ['Medium', 600],
    ['Bold', 700],
    ['Black', 800],
    ['Black', 900]
  ];
  const fontStyle = document.createElement('style');
  fontStyle.dataset.genuiEmbeddedFonts = 'HarmonyOS Sans SC';
  fontStyle.textContent = `${embeddedFontFaces.map(([name, weight]) => `@font-face{font-family:"${embeddedFontFamily}";src:url("references/fonts/HarmonyOS_Sans_SC_${name}.ttf") format("truetype");font-style:normal;font-weight:${weight};font-display:block}`).join('')}[data-genui-list-scroll-bar]{scrollbar-width:none;-ms-overflow-style:none}[data-genui-list-scroll-bar]::-webkit-scrollbar{display:none;width:0;height:0}`;
  document.head.appendChild(fontStyle);

  // Versioned compatibility profile shared by the Designer and the CLI.
  // Values in this object mirror references/genui and its ArkUI defaults.
  const compatibilityProfile = Object.freeze({
    id: 'genui-arkui-2026-07',
    apiVersion: 12,
    theme: 'light',
    Button: Object.freeze({
      padding: Object.freeze({ top: 8, right: 12, bottom: 8, left: 12 }),
      fontSize: 16,
      fontWeight: 500,
      fontColor: '#FF0A59F7',
      backgroundColor: '#0C000000'
    }),
    Text: Object.freeze({ fontSize: 16, fontWeight: 400, fontColor: '#E5000000', maxLines: 2147483647 }),
    Checkbox: Object.freeze({
      height: 48,
      controlSize: 20,
      controlMargin: 2,
      labelSpacing: 12,
      labelFontSize: 16,
      selectedColor: '#FF317AF7',
      unSelectedColor: '#33FFFFFF',
      markColor: '#FFFFFFFF',
      markSize: 20,
      markStrokeWidth: 2,
      shape: 'circle'
    }),
    Row: Object.freeze({ itemMargin: 16, alignItems: 'center', justifyContent: 'start', wrap: false }),
    Column: Object.freeze({ itemMargin: 8, alignItems: 'start', justifyContent: 'start' }),
    List: Object.freeze({ space: 0, listDirection: 'vertical', scrollBar: 'auto' }),
    Image: Object.freeze({ aspectRatio: 1, objectFit: 'cover' }),
    Divider: Object.freeze({ strokeWidth: 1, vertical: false, color: '#33000000', lineCap: 'butt' }),
    Progress: Object.freeze({
      value: 0,
      total: 100,
      type: 'linear',
      color: '#FF0A59F7',
      trackColor: '#19000000',
      linearStrokeWidth: 4
    })
  });
  const defaults = compatibilityProfile;

  function cssColor(value) {
    if (!value) return '';
    if (/^#[0-9a-f]{8}$/i.test(value)) return `#${value.slice(3)}${value.slice(1, 3)}`;
    return value;
  }

  function edge(value) {
    if (typeof value === 'number') return `${value}px`;
    if (Array.isArray(value)) {
      const [top = 0, right = top, bottom = top, left = right] = value;
      return `${top}px ${right}px ${bottom}px ${left}px`;
    }
    if (!value || typeof value !== 'object') return '';
    const all = value.all ?? 0;
    const vertical = value.vertical ?? all;
    const horizontal = value.horizontal ?? all;
    return `${value.top ?? vertical}px ${value.right ?? horizontal}px ${value.bottom ?? vertical}px ${value.left ?? horizontal}px`;
  }

  function fontWeight(value, fallback) {
    if (typeof value === 'number') return value;
    return ({ lighter: 300, normal: 400, regular: 400, medium: 500, bold: 700, bolder: 800 }[value] ?? fallback);
  }

  function flexAlign(value, fallback = 'flex-start') {
    return ({ start: 'flex-start', top: 'flex-start', left: 'flex-start', end: 'flex-end', bottom: 'flex-end',
      right: 'flex-end', center: 'center', spaceBetween: 'space-between', spaceAround: 'space-around',
      spaceEvenly: 'space-evenly' }[value] || fallback);
  }

  function stackAlign(value) {
    return ({ topStart: 'start', top: 'start center', topEnd: 'start end', start: 'center start', center: 'center',
      end: 'center end', bottomStart: 'end start', bottom: 'end center', bottomEnd: 'end end' }[value] || 'center');
  }

  function gradient(value) {
    if (!value?.colors) return '';
    const directions = { RightBottom: '135deg', LeftBottom: '45deg', RightTop: '225deg', LeftTop: '315deg',
      Right: '90deg', Left: '270deg', Bottom: '180deg', Top: '0deg' };
    const colors = value.colors;
    const stops = Array.isArray(value.stops) ? value.stops : [];
    const repeating = value.repeating === true;
    const angle = typeof value.angle === 'number' ? `${value.angle}deg` : (directions[value.direction] || '135deg');
    const parts = colors.map((item, index) => {
      let color;
      let stop;
      if (Array.isArray(item) && item.length >= 2) {
        color = cssColor(item[0]);
        stop = Number(item[1]);
      } else if (item && typeof item === 'object') {
        color = cssColor(item.color);
        stop = item.stop !== undefined ? Number(item.stop) : item.position !== undefined ? Number(item.position) : NaN;
      } else {
        color = cssColor(item);
        stop = NaN;
      }
      if (typeof stops[index] === 'number') stop = stops[index];
      if (!Number.isFinite(stop)) stop = colors.length <= 1 ? 0 : index / (colors.length - 1);
      return `${color} ${stop * 100}%`;
    });
    return `${repeating ? 'repeating-linear-gradient' : 'linear-gradient'}(${angle},${parts.join(',')})`;
  }

  function resolved(value, context) {
    return context.evalBinding(value, context.local);
  }

  // ---------------------------------------------------------------- dynamic
  // GenUI resolves every attribute through one recursive pipeline
  // (data/DynamicValueResolver.cpp): full {{ ... }} expressions, ${...}
  // template strings, {"path": ...} bindings and {"call": ..., "args": ...}
  // native function calls. Objects and arrays are walked recursively.

  const MAX_DYNAMIC_RESOLVE_DEPTH = 16;

  function valueToText(value) {
    if (value == null) return '';
    if (typeof value === 'string' || typeof value === 'number' || typeof value === 'boolean') return String(value);
    return JSON.stringify(value);
  }

  function isPathBinding(value) {
    if (!value || typeof value !== 'object' || Array.isArray(value)) return false;
    const keys = Object.keys(value);
    return keys.length === 1 && keys[0] === 'path';
  }

  function evaluateExpression(raw, getPath, local) {
    let expression = raw.trim().slice(2, -2).trim();
    // JSON-pointer placeholders are rewritten to the absolute data-model
    // variable in native expressions; mirror that with literal substitution.
    expression = expression.replace(/\$\{([^}]+)\}/g, (_, path) => JSON.stringify(getPath(path, local)));
    // GenUI recovers $__dataModel["/a/b"] JSON-pointer bracket keys.
    expression = expression.replace(/\$__dataModel\s*\[\s*(['"])(\/[^'"]*)\1\s*\]/g, (_, __, path) =>
      JSON.stringify(getPath(path, local)));
    const size = input => Array.isArray(input) ? input.length : 0;
    const dataModel = getPath('/', local) ?? {};
    try { return Function('size', '$__dataModel', `"use strict";return (${expression})`)(size, dataModel); }
    catch { return undefined; }
  }

  function resolveTemplateString(raw, getPath, local) {
    let resolved = '';
    let touched = false;
    let index = 0;
    while (index < raw.length) {
      if (raw[index] === '\\' && raw[index + 1] === '$' && raw[index + 2] === '{') {
        resolved += '${';
        index += 3;
        touched = true;
        continue;
      }
      if (raw[index] === '$' && raw[index + 1] === '{') {
        const close = raw.indexOf('}', index + 2);
        if (close === -1) {
          resolved += raw[index];
          index += 1;
          continue;
        }
        const expr = raw.slice(index + 2, close);
        if (expr[0] === '/') {
          resolved += valueToText(getPath(expr, local));
          touched = true;
          index = close + 1;
          continue;
        }
        // Plain template strings only resolve JSON-pointer placeholders.
        resolved += raw[index];
        index += 1;
        continue;
      }
      resolved += raw[index];
      index += 1;
    }
    return touched ? resolved : raw;
  }

  // ------------------------------------------------------ native functions
  // Registry mirrors functions/NativeFunctionRegistry.cpp (new in 8a2c9fc).
  const EMAIL_PATTERN = /^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$/;

  function parseDecimals(value) {
    if (value === undefined) return 2;
    if (typeof value !== 'number' || !Number.isFinite(value) || Math.abs(value) > 20) return null;
    return Math.floor(Math.abs(value));
  }

  function parseGrouping(value) {
    if (value === undefined) return false;
    if (typeof value !== 'boolean') return null;
    return value;
  }

  function roundTo(value, decimals) {
    const factor = Math.pow(10, decimals);
    const epsilon = value >= 0 ? 1e-9 : -1e-9;
    return Math.round(value * factor + epsilon) / factor;
  }

  function formatDecimal(value, decimals) {
    const rounded = roundTo(value, decimals);
    const negative = rounded < 0 || Object.is(rounded, -0);
    const digits = Math.abs(rounded).toFixed(decimals);
    return (negative ? '-' : '') + digits;
  }

  function formatWithGrouping(value, decimals) {
    const formatted = formatDecimal(Math.abs(value), decimals);
    const dot = formatted.indexOf('.');
    const intPart = dot === -1 ? formatted : formatted.slice(0, dot);
    const decPart = dot === -1 ? '' : formatted.slice(dot);
    let grouped = '';
    for (let i = intPart.length - 1, count = 0; i >= 0; i -= 1, count += 1) {
      if (count > 0 && count % 3 === 0) grouped = ',' + grouped;
      grouped = intPart[i] + grouped;
    }
    return (value < 0 ? '-' : '') + grouped + decPart;
  }

  const MONTH_NAMES = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August',
    'September', 'October', 'November', 'December'];
  const MONTH_ABBREVS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
  const DAY_NAMES = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];
  const DAY_ABBREVS = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

  function dayOfWeek(year, month, day) {
    if (month < 3) {
      month += 12;
      year -= 1;
    }
    const k = year % 100;
    const j = Math.floor(year / 100);
    const h = (day + Math.floor((13 * (month + 1)) / 5) + k + Math.floor(k / 4) + Math.floor(j / 4) + 5 * j) % 7;
    return (h + 6) % 7;
  }

  function padInt(value, width) {
    let text = String(Math.abs(value));
    while (text.length < width) text = '0' + text;
    return text;
  }

  function formatDateValue(value, format) {
    if (typeof value !== 'string' || typeof format !== 'string' || value === '' || format === '') return '';
    if (value.length < 10 || value[4] !== '-' || value[7] !== '-') return '';
    const year = Number(value.slice(0, 4));
    const month = Number(value.slice(5, 7));
    const day = Number(value.slice(8, 10));
    if (!Number.isFinite(year) || !Number.isFinite(month) || !Number.isFinite(day)) return '';
    let hour = 0;
    let minute = 0;
    let second = 0;
    if (value.length > 10 && value[10] === 'T') {
      if (value.length >= 19) {
        hour = Number(value.slice(11, 13));
        minute = Number(value.slice(14, 16));
        second = Number(value.slice(17, 19));
      } else if (value.length >= 16) {
        hour = Number(value.slice(11, 13));
        minute = Number(value.slice(14, 16));
      }
    }
    const parts = { year, month, day, hour, minute, second };
    let result = '';
    let index = 0;
    while (index < format.length) {
      const token = format[index];
      let run = 1;
      while (index + run < format.length && format[index + run] === token) run += 1;
      if (token === 'y') result += run >= 3 ? padInt(parts.year, 4) : padInt(parts.year % 100, 2);
      else if (token === 'M') {
        const valid = parts.month >= 1 && parts.month <= 12;
        result += run >= 4 ? (valid ? MONTH_NAMES[parts.month - 1] : '')
          : run === 3 ? (valid ? MONTH_ABBREVS[parts.month - 1] : '')
            : run === 2 ? padInt(parts.month, 2) : String(parts.month);
      } else if (token === 'd') result += run >= 2 ? padInt(parts.day, 2) : String(parts.day);
      else if (token === 'E') {
        const weekday = dayOfWeek(parts.year, parts.month, parts.day);
        result += run >= 4 ? DAY_NAMES[weekday] : DAY_ABBREVS[weekday];
      } else if (token === 'H') result += run >= 2 ? padInt(parts.hour, 2) : String(parts.hour);
      else if (token === 'h') {
        let hour12 = parts.hour % 12;
        if (hour12 === 0) hour12 = 12;
        result += run >= 2 ? padInt(hour12, 2) : String(hour12);
      } else if (token === 'm') result += run >= 2 ? padInt(parts.minute, 2) : String(parts.minute);
      else if (token === 's') result += run >= 2 ? padInt(parts.second, 2) : String(parts.second);
      else if (token === 'a') result += parts.hour < 12 ? 'AM' : 'PM';
      else result += token.repeat(run);
      index += run;
    }
    return result;
  }

  function findMatchingBrace(text, start) {
    let depth = 0;
    for (let i = start; i < text.length; i += 1) {
      if (text[i] === '{') depth += 1;
      else if (text[i] === '}') {
        depth -= 1;
        if (depth === 0) return i;
      }
    }
    return -1;
  }

  function parseLooseJsonToken(token) {
    if (token === '') return '';
    if (token === 'true') return true;
    if (token === 'false') return false;
    if (token === 'null') return null;
    if (token.trim() !== '' && Number.isFinite(Number(token))) return Number(token);
    return token;
  }

  function parseSingleFormatArg(argsPart, start, getPath, local) {
    if (argsPart[start] === "'") {
      const end = argsPart.indexOf("'", start + 1);
      if (end === -1) return null;
      return { value: argsPart.slice(start + 1, end), next: end + 1 };
    }
    if (argsPart[start] === '$' && argsPart[start + 1] === '{') {
      const innerClose = findMatchingBrace(argsPart, start + 1);
      if (innerClose === -1) return null;
      const resolved = resolveFormatTemplate('${' + argsPart.slice(start + 2, innerClose) + '}', getPath, local);
      return { value: parseLooseJsonToken(resolved), next: innerClose + 1 };
    }
    let end = argsPart.indexOf(',', start);
    if (end === -1) end = argsPart.length;
    const raw = argsPart.slice(start, end).replace(/\s+$/, '');
    return { value: parseLooseJsonToken(raw), next: end };
  }

  function parseFormatCallArgs(argsPart, getPath, local) {
    const args = {};
    let pos = 0;
    while (pos < argsPart.length) {
      while (pos < argsPart.length && argsPart[pos] === ' ') pos += 1;
      if (pos >= argsPart.length) break;
      const colon = argsPart.indexOf(':', pos);
      if (colon === -1) return null;
      const name = argsPart.slice(pos, colon).replace(/\s+$/, '');
      if (name === '') return null;
      let start = colon + 1;
      while (start < argsPart.length && argsPart[start] === ' ') start += 1;
      if (start >= argsPart.length) return null;
      const parsed = parseSingleFormatArg(argsPart, start, getPath, local);
      if (!parsed) return null;
      pos = parsed.next;
      if (pos < argsPart.length && argsPart[pos] === ',') pos += 1;
      args[name] = parsed.value;
    }
    return args;
  }

  function resolveFormatDataPath(expr, getPath, local) {
    const path = expr && expr[0] === '/' ? expr : '/' + expr;
    return valueToText(getPath(path, local));
  }

  function resolveFormatFunctionCall(funcName, argsPart, getPath, local) {
    const fn = nativeFunctions[funcName];
    if (!fn) return '';
    const args = parseFormatCallArgs(argsPart, getPath, local);
    if (!args) return '';
    const result = fn(args, getPath, local);
    return result == null ? '' : valueToText(result);
  }

  function resolveFormatTemplate(template, getPath, local) {
    let result = '';
    let index = 0;
    while (index < template.length) {
      if (template[index] === '\\' && template[index + 1] === '$' && template[index + 2] === '{') {
        result += '${';
        index += 3;
        continue;
      }
      if (template[index] !== '$' || template[index + 1] !== '{') {
        result += template[index];
        index += 1;
        continue;
      }
      const close = findMatchingBrace(template, index + 1);
      if (close < 0) {
        result += template[index];
        index += 1;
        continue;
      }
      const expr = template.slice(index + 2, close);
      const paren = expr.indexOf('(');
      let resolved;
      if (paren !== -1 && expr[expr.length - 1] === ')') {
        resolved = resolveFormatFunctionCall(expr.slice(0, paren), expr.slice(paren + 1, expr.length - 1),
          getPath, local);
      } else {
        resolved = resolveFormatDataPath(expr, getPath, local);
      }
      result += resolved;
      index = close + 1;
    }
    return result;
  }

  // CLDR plural rule families (NativePluralizeFunction.cpp).
  function pluralOperands(count) {
    const integerValue = Math.trunc(count);
    const isDecimal = count % 1 !== 0;
    return { count, integerValue, isDecimal };
  }

  const pluralOneOther = o => o.isDecimal ? 'other' : o.integerValue === 1 ? 'one' : 'other';
  const pluralFrench = o => {
    if (o.integerValue === 0 || o.integerValue === 1) return 'one';
    if (o.isDecimal) {
      const intPart = Math.trunc(o.count);
      const frac = o.count - intPart;
      if (frac > 0 && (intPart === 0 || intPart === 1)) return 'one';
    }
    return 'other';
  };
  const pluralOneFewOther = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue === 1) return 'one';
    if (o.integerValue >= 2 && o.integerValue <= 4) return 'few';
    return 'other';
  };
  const pluralOneFewManyOther = o => {
    if (o.isDecimal) return 'other';
    const mod10 = o.integerValue % 10;
    const mod100 = o.integerValue % 100;
    if (mod10 === 1 && mod100 !== 11) return 'one';
    if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) return 'few';
    if (mod10 === 0 || (mod10 >= 5 && mod10 <= 9) || (mod100 >= 11 && mod100 <= 14)) return 'many';
    return 'other';
  };
  const pluralPolish = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue === 1) return 'one';
    const mod10 = o.integerValue % 10;
    const mod100 = o.integerValue % 100;
    if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) return 'few';
    return 'many';
  };
  const pluralLithuanian = o => {
    if (o.isDecimal) return 'other';
    const mod10 = o.integerValue % 10;
    const mod100 = o.integerValue % 100;
    if (mod10 === 1 && mod100 !== 11) return 'one';
    if (mod10 >= 2 && mod10 <= 9 && (mod100 < 12 || mod100 > 19)) return 'few';
    return 'other';
  };
  const pluralLatvian = o => {
    if (o.isDecimal) return 'zero';
    if (o.integerValue % 10 === 0) return 'zero';
    const mod100 = o.integerValue % 100;
    if (mod100 >= 11 && mod100 <= 19) return 'zero';
    if (o.integerValue % 10 === 1 && mod100 !== 11) return 'one';
    return 'other';
  };
  const pluralBreton = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue === 1) return 'one';
    if (o.integerValue === 2) return 'two';
    if (o.integerValue === 3) return 'few';
    if (o.integerValue === 6) return 'many';
    return 'other';
  };
  const pluralMacedonian = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue % 10 === 1 && o.integerValue !== 11) return 'one';
    return 'other';
  };
  const pluralIcelandic = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue % 10 === 1 && o.integerValue % 100 !== 11) return 'one';
    return 'other';
  };
  const pluralArabic = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue === 0) return 'zero';
    if (o.integerValue === 1) return 'one';
    if (o.integerValue === 2) return 'two';
    const mod100 = o.integerValue % 100;
    if (mod100 >= 3 && mod100 <= 10) return 'few';
    if (mod100 >= 11 && mod100 <= 99) return 'many';
    return 'other';
  };
  const pluralHebrew = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue === 1) return 'one';
    if (o.integerValue === 2) return 'two';
    if (o.integerValue === 0) return 'many';
    if (o.integerValue >= 10 && o.integerValue <= 20) return 'many';
    if (o.integerValue > 20 && o.integerValue % 10 === 0) return 'many';
    return 'other';
  };
  const pluralWelsh = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue === 0) return 'zero';
    if (o.integerValue === 1) return 'one';
    if (o.integerValue === 2) return 'two';
    if (o.integerValue === 3) return 'few';
    if (o.integerValue === 6) return 'many';
    return 'other';
  };
  const pluralIrish = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue === 1) return 'one';
    if (o.integerValue === 2) return 'two';
    if (o.integerValue >= 3 && o.integerValue <= 6) return 'few';
    if (o.integerValue >= 7 && o.integerValue <= 10) return 'many';
    return 'other';
  };
  const pluralSlovenian = o => {
    if (o.isDecimal) return 'other';
    const mod100 = o.integerValue % 100;
    if (mod100 === 1) return 'one';
    if (mod100 === 2) return 'two';
    if (mod100 === 3 || mod100 === 4) return 'few';
    return 'other';
  };
  const pluralMaltese = o => {
    if (o.isDecimal) return 'other';
    if (o.integerValue === 1) return 'one';
    if (o.integerValue === 0 || (o.integerValue % 100 >= 2 && o.integerValue % 100 <= 10)) return 'few';
    if (o.integerValue % 100 >= 11 && o.integerValue % 100 <= 19) return 'many';
    return 'other';
  };
  const PLURAL_RULE_MAPPINGS = {
    ar: pluralArabic, ru: pluralOneFewManyOther, uk: pluralOneFewManyOther, be: pluralOneFewManyOther,
    bs: pluralOneFewManyOther, hr: pluralOneFewManyOther, sr: pluralOneFewManyOther, pl: pluralPolish,
    cs: pluralOneFewOther, sk: pluralOneFewOther, lt: pluralLithuanian, lv: pluralLatvian,
    fr: pluralFrench, pt: pluralFrench, cy: pluralWelsh, ga: pluralIrish, br: pluralBreton,
    he: pluralHebrew, is: pluralIcelandic, mk: pluralMacedonian, sl: pluralSlovenian, mt: pluralMaltese
  };

  function pluralCategory(count, locale) {
    const language = String(locale || '').split('-')[0];
    const rule = PLURAL_RULE_MAPPINGS[language] || pluralOneOther;
    return rule(pluralOperands(count));
  }

  function nativeFormatString(args, getPath, local) {
    if (!args || typeof args !== 'object') return '';
    const value = args.value;
    if (typeof value !== 'string') return '';
    return resolveFormatTemplate(value, getPath, local);
  }

  function nativeFormatNumber(args) {
    if (!args || typeof args !== 'object') return '';
    const value = args.value;
    if (typeof value !== 'number') return '';
    const decimals = parseDecimals(args.decimals);
    const grouping = parseGrouping(args.grouping);
    if (decimals === null || grouping === null) return '';
    return grouping ? formatWithGrouping(value, decimals) : formatDecimal(value, decimals);
  }

  function nativeFormatCurrency(args) {
    if (!args || typeof args !== 'object') return '';
    const value = args.value;
    const currency = args.currency;
    if (typeof value !== 'number' || typeof currency !== 'string' || currency === '') return '';
    const decimals = parseDecimals(args.decimals);
    const grouping = parseGrouping(args.grouping);
    if (decimals === null || grouping === null) return '';
    const formatted = grouping ? formatWithGrouping(value, decimals) : formatDecimal(value, decimals);
    return currency + ' ' + formatted;
  }

  function nativeFormatDate(args) {
    if (!args || typeof args !== 'object') return '';
    return formatDateValue(args.value, args.format);
  }

  function nativePluralize(args) {
    if (!args || typeof args !== 'object') return '';
    const value = args.value;
    if (typeof value !== 'number') return '';
    for (const key of ['zero', 'one', 'two', 'few', 'many', 'other']) {
      if (args[key] !== undefined && typeof args[key] !== 'string') return '';
    }
    const locale = (typeof navigator !== 'undefined' && navigator.language) || 'en';
    const category = pluralCategory(value, locale);
    const matched = args[category];
    if (typeof matched === 'string' && matched !== '') return matched;
    return typeof args.other === 'string' ? args.other : '';
  }

  function nativeRequired(args) {
    if (!args || typeof args !== 'object') return false;
    const value = args.value;
    if (value === undefined || value === null) return false;
    if (typeof value === 'string') return value !== '';
    if (Array.isArray(value)) return value.length > 0;
    if (typeof value === 'object') return Object.keys(value).length > 0;
    return true;
  }

  function nativeRegex(args) {
    if (!args || typeof args !== 'object') return false;
    const value = args.value;
    const pattern = args.pattern;
    if (typeof value !== 'string' || typeof pattern !== 'string' || pattern === '') return false;
    try { return new RegExp('^(?:' + pattern + ')$').test(value); }
    catch { return false; }
  }

  function nativeLength(args) {
    if (!args || typeof args !== 'object') return false;
    const value = args.value;
    if (typeof value !== 'string') return false;
    const length = value.length;
    const hasMin = args.min !== undefined;
    const hasMax = args.max !== undefined;
    if (!hasMin && !hasMax) return false;
    if (hasMin) {
      if (typeof args.min !== 'number' || !Number.isFinite(args.min) ||
        args.min < -2147483648 || args.min > 2147483647) return false;
      if (length < Math.trunc(args.min)) return false;
    }
    if (hasMax) {
      if (typeof args.max !== 'number' || !Number.isFinite(args.max) ||
        args.max < -2147483648 || args.max > 2147483647) return false;
      if (length > Math.trunc(args.max)) return false;
    }
    return true;
  }

  function nativeNumeric(args) {
    if (!args || typeof args !== 'object') return false;
    const value = args.value;
    if (typeof value !== 'number' || !Number.isFinite(value)) return false;
    const hasMin = args.min !== undefined;
    const hasMax = args.max !== undefined;
    if (!hasMin && !hasMax) return false;
    if (hasMin) {
      if (typeof args.min !== 'number' || !Number.isFinite(args.min)) return false;
      if (value < args.min) return false;
    }
    if (hasMax) {
      if (typeof args.max !== 'number' || !Number.isFinite(args.max)) return false;
      if (value > args.max) return false;
    }
    return true;
  }

  function nativeEmail(args) {
    if (!args || typeof args !== 'object') return false;
    const value = args.value;
    if (typeof value !== 'string' || value === '') return false;
    return EMAIL_PATTERN.test(value);
  }

  function nativeAnd(args) {
    if (!args || typeof args !== 'object') return false;
    const values = args.values;
    if (!Array.isArray(values) || values.length < 2) return false;
    return values.every(item => item === true);
  }

  function nativeOr(args) {
    if (!args || typeof args !== 'object') return false;
    const values = args.values;
    if (!Array.isArray(values) || values.length < 2) return false;
    return values.some(item => item === true);
  }

  function nativeNot(args) {
    if (!args || typeof args !== 'object') return true;
    const value = args.value;
    if (typeof value !== 'boolean') return true;
    return !value;
  }

  const nativeFunctions = Object.freeze({
    required: nativeRequired,
    regex: nativeRegex,
    length: nativeLength,
    numeric: nativeNumeric,
    email: nativeEmail,
    formatString: nativeFormatString,
    formatNumber: nativeFormatNumber,
    formatCurrency: nativeFormatCurrency,
    formatDate: nativeFormatDate,
    pluralize: nativePluralize,
    and: nativeAnd,
    or: nativeOr,
    not: nativeNot
  });

  function resolveFunctionCall(node, getPath, local, depth) {
    const fn = nativeFunctions[node.call];
    if (!fn) return undefined;
    const args = resolveDynamicValue(node.args, getPath, local, depth + 1) ?? {};
    return fn(args, getPath, local);
  }

  function resolveDynamicValue(value, getPath, local, depth) {
    if (depth > MAX_DYNAMIC_RESOLVE_DEPTH) return undefined;
    if (typeof value === 'string') {
      const trimmed = value.trim();
      if (/^\{\{[\s\S]*\}\}$/.test(trimmed)) return evaluateExpression(value, getPath, local);
      if (value.includes('${')) return resolveTemplateString(value, getPath, local);
      return value;
    }
    if (Array.isArray(value)) return value.map(item => resolveDynamicValue(item, getPath, local, depth + 1));
    if (value && typeof value === 'object') {
      if (Object.prototype.hasOwnProperty.call(value, 'call')) return resolveFunctionCall(value, getPath, local, depth);
      if (isPathBinding(value)) return getPath(value.path, local);
      const resolvedObject = {};
      Object.keys(value).forEach(key => {
        resolvedObject[key] = resolveDynamicValue(value[key], getPath, local, depth + 1);
      });
      return resolvedObject;
    }
    return value;
  }

  function evaluateBinding(value, getPath, local) {
    return resolveDynamicValue(value, getPath, local, 0);
  }

  function applyComponentDefaults(element, component) {
    const style = element.style;
    if (component.component === 'Button') {
      const preset = defaults.Button;
      style.padding = edge(preset.padding);
      style.fontSize = `${preset.fontSize}px`;
      style.fontWeight = preset.fontWeight;
      style.color = cssColor(preset.fontColor);
      style.backgroundColor = cssColor(preset.backgroundColor);
    } else if (component.component === 'Text') {
      const preset = defaults.Text;
      style.fontSize = `${preset.fontSize}px`;
      style.fontWeight = preset.fontWeight;
      style.color = cssColor(preset.fontColor);
    } else if (component.component === 'Checkbox') {
      style.height = `${defaults.Checkbox.height}px`;
    } else if (component.component === 'Image') {
      style.aspectRatio = String(defaults.Image.aspectRatio);
    }
  }

  function applyCommonStyles(element, component, context) {
    const styles = resolved(component.styles, context) || {};
    const style = element.style;
    const dimension = value => typeof value === 'number' ? `${value}px` : String(value);
    // ArkUI nodes do not shrink unless flexShrink is explicitly applied.
    style.flexShrink = '0';
    applyComponentDefaults(element, component);
    if (styles.width != null) style.width = dimension(styles.width);
    if (styles.height != null) style.height = dimension(styles.height);
    if (styles.padding != null) style.padding = edge(styles.padding);
    if (styles.margin != null) style.margin = edge(styles.margin);
    if (styles.borderRadius != null) style.borderRadius = typeof styles.borderRadius === 'number'
      ? `${styles.borderRadius}px` : edge(styles.borderRadius);
    if (styles.clip) style.overflow = 'hidden';
    if (styles.visibility) {
      const visibility = String(resolved(styles.visibility, context)).toLowerCase();
      if (visibility === 'none') style.display = 'none';
      else if (visibility === 'hidden') style.visibility = 'hidden';
    }
    if (styles.backgroundColor) style.backgroundColor = cssColor(resolved(styles.backgroundColor, context));
    if (styles.linearGradient) style.backgroundImage = gradient(styles.linearGradient);
    else if (styles.backgroundImage) {
      const source = resolved(styles.backgroundImage, context);
      style.backgroundImage = `url("${context.previewAssetPath(source)}")`;
      style.backgroundSize = 'cover';
      style.backgroundPosition = 'center';
    }
    if (styles.constraintSize) {
      const constraintSize = resolved(styles.constraintSize, context) || {};
      if (constraintSize.minWidth != null) style.minWidth = dimension(constraintSize.minWidth);
      if (constraintSize.maxWidth != null) style.maxWidth = dimension(constraintSize.maxWidth);
      if (constraintSize.minHeight != null) style.minHeight = dimension(constraintSize.minHeight);
      if (constraintSize.maxHeight != null) style.maxHeight = dimension(constraintSize.maxHeight);
    }
    if (styles.backgroundImageSize && style.backgroundImage) {
      const backgroundImageSize = resolved(styles.backgroundImageSize, context) || {};
      const sizeDimension = value => typeof value === 'number' ? `${value}px` : String(value);
      if (backgroundImageSize.width != null && backgroundImageSize.height != null) {
        style.backgroundSize = `${sizeDimension(backgroundImageSize.width)} ${sizeDimension(backgroundImageSize.height)}`;
      } else if (backgroundImageSize.width != null) {
        style.backgroundSize = sizeDimension(backgroundImageSize.width);
      } else if (backgroundImageSize.height != null) {
        style.backgroundSize = `auto ${sizeDimension(backgroundImageSize.height)}`;
      }
    }
    if (styles.fontSize != null) style.fontSize = `${styles.fontSize}px`;
    if (styles.fontWeight != null) style.fontWeight = fontWeight(styles.fontWeight, 400);
    if (styles.fontColor) style.color = cssColor(resolved(styles.fontColor, context));
    if (styles.opacity != null) style.opacity = styles.opacity;
    if (styles.borderWidth) style.border = `${styles.borderWidth}px solid ${cssColor(resolved(styles.borderColor || '#FF000000', context))}`;
    if (styles.shadow) {
      const shadow = styles.shadow;
      style.boxShadow = `${shadow.offsetX || 0}px ${shadow.offsetY || 0}px ${shadow.radius || 0}px ${cssColor(resolved(shadow.color || '#FF000000', context))}`;
    }
    if (styles.aspectRatio != null) {
      const aspectRatio = Number(resolved(styles.aspectRatio, context));
      if (Number.isFinite(aspectRatio) && aspectRatio > 0) style.aspectRatio = String(aspectRatio);
    }
    if (styles.flexGrow != null) style.flexGrow = styles.flexGrow;
    if (Number(styles.layoutWeight) > 0) {
      style.flexGrow = String(styles.layoutWeight);
      style.flexBasis = '0px';
    }
    if (styles.flexShrink != null) style.flexShrink = styles.flexShrink;
  }

  function configureContainer(element, component, context) {
    const styles = resolved(component.styles, context) || {};
    const style = element.style;
    if (component.component === 'Row' || component.component === 'Column' || component.component === 'List') {
      const isRow = component.component === 'Row';
      const isList = component.component === 'List';
      const justifyContent = resolved(styles.justifyContent ?? component.justifyContent, context);
      const alignItems = resolved(styles.alignItems ?? component.alignItems, context);
      const distributed = ['spaceAround', 'spaceBetween', 'spaceEvenly'].includes(justifyContent);
      const direction = isList
        ? (resolved(styles.listDirection, context) || defaults.List.listDirection)
        : (isRow ? 'horizontal' : 'vertical');
      const defaultMargin = isRow ? defaults.Row.itemMargin : defaults.Column.itemMargin;
      const spacing = resolved(isList ? (component.space ?? defaults.List.space) : (component.itemMargin ?? defaultMargin), context);
      const wrap = resolved(component.wrap, context);
      style.display = 'flex';
      style.flexDirection = direction === 'horizontal' ? 'row' : 'column';
      style.gap = `${distributed ? 0 : Math.max(0, Number(spacing) || 0)}px`;
      style.justifyContent = flexAlign(justifyContent, 'flex-start');
      style.alignItems = flexAlign(alignItems, isRow ? 'center' : 'flex-start');
      if (wrap === true || wrap === 'wrap') style.flexWrap = 'wrap';
      if (isList) {
        const scrollBar = resolved(styles.scrollBar, context) || defaults.List.scrollBar;
        style.overflowX = direction === 'horizontal' ? (scrollBar === 'off' ? 'hidden' : 'auto') : 'hidden';
        style.overflowY = direction === 'vertical' ? (scrollBar === 'off' ? 'hidden' : 'auto') : 'hidden';
        // Chromium's overlay scrollbar is absent in headless screenshots. ArkUI
        // still paints an indicator for an overflowing List, so finalize() adds
        // a non-scrolling visual indicator after layout is known.
        element.dataset.genuiListScrollBar = scrollBar;
        element.dataset.genuiListDirection = direction;
        style.position = 'relative';
        if (scrollBar === 'off') style.scrollbarWidth = 'none';
      }
    } else if (component.component === 'Stack') {
      style.position = 'relative';
      style.display = 'grid';
      style.placeItems = stackAlign(styles.alignContent);
      [...element.children].forEach(child => { child.style.gridArea = '1 / 1'; });
    }
  }

  function configureText(element, component, context) {
    const styles = resolved(component.styles, context) || {};
    const maxLines = Number.isFinite(Number(styles.maxLines)) ? Math.max(0, Number(styles.maxLines)) : defaults.Text.maxLines;
    const overflow = styles.textOverflow === 'ellipsis' ? 'ellipsis' : 'clip';
    element.textContent = resolved(component.content ?? component.text, context) ?? '';
    element.dataset.genuiTextContent = element.textContent;
    element.style.display = 'flex';
    element.style.alignItems = 'center';
    if (maxLines === 1) {
      element.style.whiteSpace = 'nowrap';
      element.style.overflow = 'hidden';
      element.style.textOverflow = overflow;
      if (overflow === 'clip') element.dataset.genuiGlyphClip = 'true';
    } else if (maxLines < defaults.Text.maxLines) {
      element.style.overflow = 'hidden';
      element.style.display = '-webkit-box';
      element.style.webkitBoxOrient = 'vertical';
      element.style.webkitLineClamp = String(maxLines);
    } else {
      element.style.whiteSpace = 'pre-wrap';
    }
    element.style.textAlign = ({ start: 'left', center: 'center', end: 'right' }[styles.textAlign] || 'left');
    element.style.justifyContent = flexAlign(styles.textAlign, 'flex-start');
    element.style.overflowWrap = styles.wordBreak === 'breakAll' ? 'anywhere' : 'break-word';
    if (styles.wordBreak === 'breakAll') element.style.wordBreak = 'break-all';
    if (styles.minFontSize || styles.maxFontSize) element.dataset.genuiAutoFit = 'true';
  }

  function configureButton(element, component, context) {
    const label = resolved(component.label ?? component.text, context) ?? '';
    const text = document.createElement('span');
    text.className = 'button-label';
    text.textContent = label;
    text.style.whiteSpace = 'nowrap';
    text.style.flex = 'none';
    element.style.display = 'flex';
    element.style.alignItems = 'center';
    element.style.justifyContent = 'center';
    element.style.minWidth = '0';
    element.style.overflow = 'hidden';
    element.appendChild(text);
    if (component.styles?.minFontSize || component.styles?.maxFontSize) element.dataset.genuiAutoFit = 'true';
  }

  function configureImage(element, component, context) {
    const styles = resolved(component.styles, context) || {};
    const source = resolved(component.src, context);
    const previewSource = context.previewAssetPath(source);
    const objectFit = styles.objectFit || defaults.Image.objectFit;
    const fillColor = resolved(styles.fillColor, context);
    const useSvgFill = typeof fillColor === 'string' && /\.svg(?:$|[?#])/i.test(String(source || ''));
    const image = document.createElement(useSvgFill ? 'span' : 'img');
    image.style.width = '100%';
    image.style.height = '100%';
    image.style.display = 'block';
    if (useSvgFill) {
      const maskSize = objectFit === 'fill' ? '100% 100%'
        : objectFit === 'cover' ? 'cover'
          : objectFit === 'none' ? 'auto' : 'contain';
      image.setAttribute('role', 'img');
      image.setAttribute('aria-label', component.id);
      image.style.backgroundColor = cssColor(fillColor);
      image.style.maskImage = `url("${previewSource}")`;
      image.style.webkitMaskImage = `url("${previewSource}")`;
      image.style.maskRepeat = 'no-repeat';
      image.style.webkitMaskRepeat = 'no-repeat';
      image.style.maskPosition = 'center';
      image.style.webkitMaskPosition = 'center';
      image.style.maskSize = maskSize;
      image.style.webkitMaskSize = maskSize;
    } else {
      image.alt = component.id;
      image.style.objectFit = ({ scaleDown: 'scale-down' }[objectFit] || objectFit);
    }
    const showPlaceholder = () => {
      image.style.display = 'none';
      element.classList.add('image-placeholder');
      element.title = source || '';
      element.style.display = 'grid';
      element.style.placeItems = 'center';
      element.style.color = '#8c94a5';
      element.style.background = 'repeating-linear-gradient(45deg,#eef0f4,#eef0f4 4px,#e5e8ee 4px,#e5e8ee 8px)';
      if (!element.querySelector('.asset-missing')) {
        const mark = document.createElement('span');
        mark.className = 'asset-missing';
        mark.textContent = '▧';
        element.appendChild(mark);
      }
    };
    if (previewSource && useSvgFill) {
      const probe = new Image();
      probe.onerror = showPlaceholder;
      probe.src = previewSource;
    } else if (previewSource) {
      image.src = previewSource;
      image.onerror = showPlaceholder;
    } else showPlaceholder();
    element.appendChild(image);
  }

  function configureProgress(element, component, context) {
    const styles = resolved(component.styles, context) || {};
    const value = Number(resolved(component.value, context));
    const total = Number(resolved(component.total, context));
    const safeTotal = Number.isFinite(total) && total > 0 ? total : defaults.Progress.total;
    const safeValue = Number.isFinite(value) ? Math.max(0, Math.min(value, safeTotal)) : defaults.Progress.value;
    const percent = safeValue / safeTotal * 100;
    const type = String(styles.type || defaults.Progress.type).toLowerCase();
    const color = cssColor(resolved(styles.color, context) || defaults.Progress.color);
    const track = cssColor(resolved(styles.backgroundColor, context) || defaults.Progress.trackColor);
    const strokeWidth = Number(resolved(styles.strokeWidth, context));
    const safeStrokeWidth = Number.isFinite(strokeWidth) && strokeWidth > 0 ? strokeWidth
      : (type === 'ring' || type === 'scalering' ? 8 : defaults.Progress.linearStrokeWidth);
    if (type === 'ring' || type === 'scalering') {
      const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
      const background = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      const foreground = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      svg.classList.add('genui-progress-ring');
      svg.dataset.ratio = String(safeValue / safeTotal);
      svg.dataset.progressType = type;
      svg.setAttribute('viewBox', '0 0 100 100');
      svg.setAttribute('preserveAspectRatio', 'xMidYMid meet');
      svg.style.cssText = 'display:block;width:100%;height:100%;overflow:visible';
      [background, foreground].forEach(circle => {
        circle.setAttribute('cx', '50');
        circle.setAttribute('cy', '50');
        circle.setAttribute('r', '46');
        circle.setAttribute('fill', 'none');
        circle.setAttribute('stroke-width', String(safeStrokeWidth));
      });
      background.setAttribute('stroke', track);
      foreground.setAttribute('stroke', color);
      foreground.setAttribute('stroke-linecap', 'round');
      foreground.setAttribute('pathLength', '100');
      foreground.setAttribute('stroke-dasharray', `${percent} 100`);
      foreground.setAttribute('transform', 'rotate(-90 50 50)');
      svg.append(background, foreground);
      element.style.background = 'transparent';
      element.appendChild(svg);
    } else if (type === 'eclipse') {
      element.style.borderRadius = '50%';
      element.style.background = `conic-gradient(${color} ${percent}%,${track} 0)`;
    } else {
      const primitive = document.createElement('span');
      const fill = document.createElement('i');
      primitive.className = 'genui-progress-track';
      primitive.dataset.ratio = String(safeValue / safeTotal);
      primitive.dataset.progressType = type;
      primitive.dataset.strokeWidth = String(safeStrokeWidth);
      primitive.style.cssText = 'position:absolute;left:0;right:0;top:50%;transform:translateY(-50%);overflow:hidden';
      primitive.style.background = track;
      fill.style.cssText = `display:block;height:100%;background:${color}`;
      primitive.appendChild(fill);
      element.style.position = 'relative';
      element.style.overflow = 'hidden';
      element.style.background = 'transparent';
      element.appendChild(primitive);
    }
  }

  function configureDivider(element, component, context) {
    const styles = resolved(component.styles, context) || {};
    const vertical = styles.vertical ?? defaults.Divider.vertical;
    const strokeWidth = styles.strokeWidth ?? defaults.Divider.strokeWidth;
    const primitive = document.createElement('span');
    primitive.className = 'genui-divider-stroke';
    primitive.dataset.vertical = String(vertical);
    primitive.dataset.strokeWidth = String(Math.max(0, Number(strokeWidth) || defaults.Divider.strokeWidth));
    primitive.dataset.lineCap = String(styles.lineCap || defaults.Divider.lineCap).toLowerCase();
    primitive.style.cssText = 'position:absolute;display:block';
    primitive.style.background = cssColor(resolved(styles.color, context) || defaults.Divider.color);
    element.style.position = 'relative';
    element.style.background = 'transparent';
    if (vertical && styles.width == null) element.style.width = `${strokeWidth}px`;
    if (!vertical && styles.height == null) element.style.height = `${strokeWidth}px`;
    element.appendChild(primitive);
  }

  function configureCheckbox(element, component, context) {
    const preset = defaults.Checkbox;
    const styles = resolved(component.styles, context) || {};
    const mark = resolved(component.mark || styles.mark, context) || {};
    const selected = Boolean(resolved(component.select, context));
    const control = document.createElement('span');
    element.setAttribute('role', 'checkbox');
    element.setAttribute('aria-checked', String(selected));
    element.style.display = 'flex';
    element.style.alignItems = 'center';
    control.className = 'checkbox-visual';
    control.style.width = `${preset.controlSize}px`;
    control.style.height = `${preset.controlSize}px`;
    control.style.margin = `${preset.controlMargin}px`;
    control.style.flex = 'none';
    control.style.display = 'grid';
    control.style.placeItems = 'center';
    control.style.boxSizing = 'border-box';
    control.style.borderRadius = (component.shape || styles.shape || preset.shape) === 'circle' ? '50%' : '5px';
    if (selected) {
      control.style.backgroundColor = cssColor(resolved(component.selectedColor ?? styles.selectedColor, context) || preset.selectedColor);
      const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
      const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      svg.setAttribute('width', Number(mark.size) || preset.markSize);
      svg.setAttribute('height', Number(mark.size) || preset.markSize);
      svg.setAttribute('viewBox', '0 0 20 20');
      svg.setAttribute('aria-hidden', 'true');
      path.setAttribute('d', 'M5 9.8 L8.8 13.6 L15.2 6.6');
      path.setAttribute('fill', 'none');
      path.setAttribute('stroke', cssColor(resolved(mark.strokeColor, context) || preset.markColor));
      path.setAttribute('stroke-width', String(Number(mark.strokeWidth) || preset.markStrokeWidth));
      path.setAttribute('stroke-linecap', 'round');
      path.setAttribute('stroke-linejoin', 'round');
      path.style.filter = 'drop-shadow(0 0 0.5px rgba(0,0,0,.25))';
      svg.appendChild(path);
      control.appendChild(svg);
    } else {
      control.style.backgroundColor = 'transparent';
      const unselectedColor = component.unselectedColor ?? component.unSelectedColor
        ?? styles.unselectedColor ?? styles.unSelectedColor;
      control.style.border = `1px solid ${cssColor(resolved(unselectedColor, context) || preset.unSelectedColor)}`;
    }
    element.appendChild(control);
    const label = resolved(component.label, context);
    if (label != null && label !== '') {
      const text = document.createElement('span');
      text.className = 'checkbox-label';
      text.textContent = label;
      text.style.minWidth = '0';
      text.style.flex = '1';
      text.style.marginLeft = `${preset.labelSpacing}px`;
      text.style.fontSize = `${preset.labelFontSize}px`;
      text.style.fontWeight = 400;
      text.style.color = cssColor('#E5000000');
      text.style.overflow = 'hidden';
      text.style.textOverflow = 'ellipsis';
      text.style.whiteSpace = 'nowrap';
      text.dataset.genuiCheckboxLabel = 'true';
      text.dataset.genuiCheckboxText = label;
      element.appendChild(text);
    }
  }

  function renderLeaf(element, component, context) {
    if (component.component === 'Text') configureText(element, component, context);
    else if (component.component === 'Button') configureButton(element, component, context);
    else if (component.component === 'Image') configureImage(element, component, context);
    else if (component.component === 'Progress') configureProgress(element, component, context);
    else if (component.component === 'Divider') configureDivider(element, component, context);
    else if (component.component === 'Checkbox') configureCheckbox(element, component, context);
  }

  function fitAdaptiveText(root) {
    root.style.fontFamily = `"${embeddedFontFamily}","HarmonyOS Sans SC","HarmonyOS Sans","Noto Sans CJK SC","Noto Sans SC","Microsoft YaHei",sans-serif`;
    root.querySelectorAll('[data-genui-auto-fit="true"]').forEach(element => {
      const styles = element.__genuiComponent?.styles || {};
      const target = element.querySelector('.button-label') || element;
      const computed = getComputedStyle(target);
      const maximum = Number(styles.maxFontSize) || Number.parseFloat(computed.fontSize);
      const minimum = Math.min(maximum, Number(styles.minFontSize) || maximum);
      target.style.fontSize = `${maximum}px`;
      for (let size = maximum; size >= minimum; size -= 0.5) {
        target.style.fontSize = `${size}px`;
        if (element.scrollWidth <= element.clientWidth && element.scrollHeight <= element.clientHeight) break;
      }
    });
    root.querySelectorAll('[data-genui-glyph-clip="true"]').forEach(element => {
      const original = element.dataset.genuiTextContent || '';
      const available = element.clientWidth;
      if (!original || available <= 0) return;
      const segments = typeof Intl.Segmenter === 'function'
        ? [...new Intl.Segmenter(undefined, { granularity: 'grapheme' }).segment(original)].map(item => item.segment)
        : [...original];
      const widthOf = text => {
        element.textContent = text;
        const textNode = element.firstChild;
        if (!textNode) return 0;
        const range = document.createRange();
        range.selectNodeContents(textNode);
        // Range bounds include the Designer canvas transform (for example its
        // 200% zoom), while clientWidth remains in layout pixels. Convert the
        // visual measurement back to layout pixels before comparing them.
        const elementWidth = element.getBoundingClientRect().width;
        const scaleX = element.clientWidth > 0 && elementWidth > 0
          ? elementWidth / element.clientWidth : 1;
        const width = range.getBoundingClientRect().width / scaleX;
        range.detach();
        return width;
      };
      if (widthOf(original) <= available) {
        element.textContent = original;
        return;
      }
      let visible = '';
      for (const segment of segments) {
        const candidate = visible + segment;
        if (widthOf(candidate) > available) break;
        visible = candidate;
      }
      element.textContent = visible;
    });
    root.querySelectorAll('[data-genui-checkbox-label="true"]').forEach(element => {
      const original = element.dataset.genuiCheckboxText || '';
      const available = Math.max(0, element.clientWidth - 4);
      if (!original || available <= 0) return;
      const widthOf = text => {
        element.textContent = text;
        const range = document.createRange();
        range.selectNodeContents(element);
        const elementWidth = element.getBoundingClientRect().width;
        const scaleX = element.clientWidth > 0 && elementWidth > 0
          ? elementWidth / element.clientWidth : 1;
        const width = range.getBoundingClientRect().width / scaleX;
        range.detach();
        return width;
      };
      if (widthOf(original) <= available) {
        element.textContent = original;
        return;
      }
      const ellipsis = '...';
      let visible = '';
      for (const segment of [...original]) {
        if (widthOf(visible + segment + ellipsis) > available) break;
        visible += segment;
      }
      element.textContent = visible + ellipsis;
    });
    root.querySelectorAll('[data-genui-list-scroll-bar]').forEach(list => {
      list.querySelector('.genui-list-scroll-indicator')?.remove();
      const mode = list.dataset.genuiListScrollBar;
      const vertical = list.dataset.genuiListDirection !== 'horizontal';
      const viewport = vertical ? list.clientHeight : list.clientWidth;
      const content = vertical ? list.scrollHeight : list.scrollWidth;
      if (mode === 'off' || viewport <= 0 || (mode === 'auto' && content <= viewport)) return;
      const indicator = document.createElement('span');
      const length = Math.min(viewport, Math.max(48, viewport * viewport / Math.max(content, 1)));
      indicator.className = 'genui-list-scroll-indicator';
      indicator.style.cssText = `position:absolute;display:block;pointer-events:none;background:${cssColor('#FF0C0C0C')};z-index:1;${vertical
        ? `top:0;right:4px;width:4px;height:${length}px;border-radius:2px`
        : `left:0;bottom:4px;height:4px;width:${length}px;border-radius:2px`}`;
      list.appendChild(indicator);
    });
    root.querySelectorAll('.genui-progress-track').forEach(track => {
      const host = track.parentElement;
      const type = track.dataset.progressType;
      const strokeWidth = Number(track.dataset.strokeWidth) || defaults.Progress.linearStrokeWidth;
      const thickness = type === 'capsule'
        ? host.clientHeight
        : Math.min(strokeWidth, host.clientHeight);
      const radius = thickness / 2;
      const ratio = Math.max(0, Math.min(1, Number(track.dataset.ratio) || 0));
      track.style.height = `${thickness}px`;
      track.style.borderRadius = `${radius}px`;
      const fill = track.firstElementChild;
      fill.style.borderRadius = `${radius}px`;
      // ArkUI measures rounded linear progress along the centre line.
      fill.style.width = `${ratio <= 0 ? 0 : Math.min(host.clientWidth,
        2 * radius + Math.max(0, host.clientWidth - 2 * radius) * ratio)}px`;
    });
    root.querySelectorAll('.genui-divider-stroke').forEach(stroke => {
      const host = stroke.parentElement;
      const vertical = stroke.dataset.vertical === 'true';
      const crossSize = vertical ? host.clientWidth : host.clientHeight;
      const thickness = Math.min(crossSize, Math.max(0, Number(stroke.dataset.strokeWidth) || 0));
      const round = stroke.dataset.lineCap === 'round';
      stroke.style.borderRadius = round ? `${thickness / 2}px` : '0';
      if (vertical) {
        stroke.style.width = `${thickness}px`;
        stroke.style.height = '100%';
        stroke.style.left = '50%';
        stroke.style.top = '0';
        stroke.style.transform = 'translateX(-50%)';
      } else {
        stroke.style.width = '100%';
        stroke.style.height = `${thickness}px`;
        stroke.style.left = '0';
        stroke.style.top = '50%';
        stroke.style.transform = 'translateY(-50%)';
      }
    });
  }

  function apply(element, component, context) {
    element.__genuiComponent = component;
    applyCommonStyles(element, component, context);
    configureContainer(element, component, context);
  }

  window.GenUIRenderer = Object.freeze({
    compatibilityProfile,
    embeddedFontFamily,
    defaults,
    cssColor,
    evaluateBinding,
    nativeFunctions,
    apply,
    renderLeaf,
    finalize: fitAdaptiveText
  });
})();
