(() => {
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
    return `linear-gradient(${directions[value.direction] || '135deg'},${value.colors.map(item =>
      `${cssColor(item[0])} ${Number(item[1]) * 100}%`).join(',')})`;
  }

  function resolved(value, context) {
    return context.evalBinding(value, context.local);
  }

  function evaluateBinding(value, getPath, local) {
    if (typeof value !== 'string' || !/^\{\{[\s\S]*\}\}$/.test(value.trim())) return value;
    let expression = value.trim().slice(2, -2).trim();
    expression = expression.replace(/\$\{([^}]+)\}/g, (_, path) => JSON.stringify(getPath(path, local)));
    const size = input => Array.isArray(input) ? input.length : 0;
    try { return Function('size', `"use strict";return (${expression})`)(size); }
    catch { return value; }
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
    const styles = component.styles || {};
    const style = element.style;
    // ArkUI nodes do not shrink unless flexShrink is explicitly applied.
    style.flexShrink = '0';
    applyComponentDefaults(element, component);
    if (styles.width != null) style.width = `${styles.width}px`;
    if (styles.height != null) style.height = `${styles.height}px`;
    if (styles.padding != null) style.padding = edge(styles.padding);
    if (styles.margin != null) style.margin = edge(styles.margin);
    if (styles.borderRadius != null) style.borderRadius = typeof styles.borderRadius === 'number'
      ? `${styles.borderRadius}px` : edge(styles.borderRadius);
    if (styles.clip) style.overflow = 'hidden';
    if (styles.backgroundColor) style.backgroundColor = cssColor(resolved(styles.backgroundColor, context));
    if (styles.linearGradient) style.backgroundImage = gradient(styles.linearGradient);
    else if (styles.backgroundImage) {
      const source = resolved(styles.backgroundImage, context);
      style.backgroundImage = `url("${context.previewAssetPath(source)}")`;
      style.backgroundSize = 'cover';
      style.backgroundPosition = 'center';
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
    if (styles.flexGrow != null) style.flexGrow = styles.flexGrow;
    if (Number(styles.layoutWeight) > 0) {
      style.flexGrow = String(styles.layoutWeight);
      style.flexBasis = '0px';
    }
    if (styles.flexShrink != null) style.flexShrink = styles.flexShrink;
  }

  function configureContainer(element, component, context) {
    const styles = component.styles || {};
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
    const styles = component.styles || {};
    const maxLines = Number.isFinite(Number(styles.maxLines)) ? Math.max(0, Number(styles.maxLines)) : defaults.Text.maxLines;
    const overflow = styles.textOverflow === 'ellipsis' ? 'ellipsis' : 'clip';
    element.textContent = resolved(component.content, context) ?? '';
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
    const image = document.createElement('img');
    const source = resolved(component.src, context);
    const previewSource = context.previewAssetPath(source);
    image.alt = component.id;
    image.style.width = '100%';
    image.style.height = '100%';
    image.style.display = 'block';
    image.style.objectFit = ({ scaleDown: 'scale-down' }[component.styles?.objectFit] || component.styles?.objectFit || defaults.Image.objectFit);
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
    if (previewSource) { image.src = previewSource; image.onerror = showPlaceholder; } else showPlaceholder();
    element.appendChild(image);
  }

  function configureProgress(element, component, context) {
    const styles = component.styles || {};
    const value = Number(resolved(component.value, context));
    const total = Number(resolved(component.total, context));
    const safeTotal = Number.isFinite(total) && total > 0 ? total : defaults.Progress.total;
    const safeValue = Number.isFinite(value) ? Math.max(0, Math.min(value, safeTotal)) : defaults.Progress.value;
    const percent = safeValue / safeTotal * 100;
    const type = String(styles.type || defaults.Progress.type).toLowerCase();
    const color = cssColor(resolved(styles.color, context) || defaults.Progress.color);
    const track = cssColor(resolved(styles.backgroundColor, context) || defaults.Progress.trackColor);
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
        circle.setAttribute('stroke-width', '8');
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
    const styles = component.styles || {};
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
    const styles = component.styles || {};
    const mark = component.mark || styles.mark || {};
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
    root.style.fontFamily = '"HarmonyOS Sans SC","HarmonyOS Sans","Noto Sans CJK SC","Noto Sans SC","Microsoft YaHei",sans-serif';
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
        const width = range.getBoundingClientRect().width;
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
    root.querySelectorAll('.genui-progress-track').forEach(track => {
      const host = track.parentElement;
      const type = track.dataset.progressType;
      const thickness = type === 'capsule'
        ? host.clientHeight
        : Math.min(defaults.Progress.linearStrokeWidth, host.clientHeight);
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
    defaults,
    cssColor,
    evaluateBinding,
    apply,
    renderLeaf,
    finalize: fitAdaptiveText
  });
})();
