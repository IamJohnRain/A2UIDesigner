# ALT 协议前端嵌入实施方案 —— 方案 A（Pyodide/WASM）

> 版本：v1.4 ｜ 日期：2026-08-06 ｜ 状态：已实现（M1–M3）
> 变更 v1.1：目标 Python 脚本由 `scripts/alt_converter.py` 替换为独立单卡转换器 `scripts/alt_to_dsl_converter.py`（仅 t2d 方向、单卡 CLI）
> 变更 v1.2：脚本新增 `--theme` / `--width` / `--height`（缺省保留默认）；`parse_alt_text` / `compile_t2d` / `browser_convert` 已落地；ALT 面板设计新增主题下拉 + 宽高输入（占位符即默认值）
> 变更 v1.3：Pyodide core 包改为**严格懒加载**（页面打开零下载，仅首次点击「编译并渲染」时下载）；加载期间显示原因提示 + 真实下载进度条，失败可重试
> 变更 v1.5：运行时改为外部仓库托管——core 0.26.4（含 `__tzset_js` 补丁）已迁至 `IamJohnRain/a2ui-pyodide`（GitHub Pages 主源 + raw 兜底），主仓库删除 `vendor/pyodide/`，部署产物恢复 <1MB；`app.js` 以 `PYODIDE_BASES` 多源加载；补丁可复现（`patch-tzset.py`）。
> 关联文档：[ALT 协议](a2ui-layout-tree-protocol.md) ｜ [转换器深度分析](alt-converter-deep-analysis.md) ｜ [调参指南](alt-tuning-parameters.md)

## 1. 方案定位

**一句话**：将 `TaskSpec + ALT + ASC → DSL` 的确定性编译能力整体嵌入前端编辑器，用户在第一栏新增的「ALT 生成」页签中输入三份文本，点击编译即在画布渲染出卡片。

**技术路线**：Pyodide（CPython 编译为 WebAssembly）在浏览器主线程直接运行独立转换器 `scripts/alt_to_dsl_converter.py`，**零移植、行为 100% 与 CLI 一致**。可部署于 GitHub Pages 纯静态托管。

| 维度 | 结论 |
| --- | --- |
| 可行性 | 可行。Pyodide 官方明确支持 GitHub Pages 类静态托管 |
| 一致性 | 与原 CLI 100% 一致（跑的是同一份 CPython 源码） |
| 交付周期 | 2–4 天（转换器适配 1–2 天 + UI 0.5–1 天 + 回归 0.5–1 天） |
| 定位 | **快速验证 POC**；终局为方案 B（JS 移植），本方案提供平滑迁移路径 |

---

## 2. 背景与现状

### 2.1 目标功能

| 项 | 内容 |
| --- | --- |
| 位置 | 左栏 `source-panel` 新增第三个页签「ALT 生成」（现有：源码 / 布局） |
| 输入 | TaskSpec（JSON）、card.alt.txt、card.asc.txt 三个 textarea |
| 动作 | 点击「编译并渲染」 |
| 输出 | 生成三行 JSONL DSL → 写入现有源码栏 → 复用现有渲染管线在画布展示 |
| 辅助 | 展示编译错误 / 警告（布局报告摘要）；首次使用下载转换引擎时显示原因提示 + 下载进度条（严格懒加载） |

### 2.2 现状约束（已核实）

- `scripts/alt_to_dsl_converter.py`（4173 行）为**纯 Python 标准库**，无第三方依赖；
- 核心编译逻辑（`SemanticResolver` / `auto_layout_document` / `apply_alt_styles` / `alt_to_dsl`）是**纯内存计算**，无文件系统依赖；
- 文件 IO 仅存在于边界：`parse_alt(Path)` / `parse_asc(Path)` / `atomic_write` / `main()`；
- `parse_alt_text(text)` / `parse_asc_text(text, doc)` 均为文本入口（已实现）；
- 脚本支持可选 `--theme` / `--width` / `--height`（缺省保留 ALT/TaskSpec/配置默认值），并内置 `compile_t2d` / `browser_convert` 文本入口（含主题与宽高覆盖）；
- 配置：`scripts/config/alt-themes.json`（16KB）+ `alt-layout-profile.json`（2.2KB）+ `alt-tuning.json`（2.4KB），通过 `Path(__file__).parent / "config"` 加载，浏览器需注入（共 3 个文件，`alt-tuning.json` 易遗漏）；
- 前端渲染管线 `renderInput → parseJsonl → loadParsed → renderAll` 可直接复用；
- `previewAssetPath` 已映射 `resources/base/media/*.svg → references/media/`（仓库 85 个 SVG），生成 DSL 可正常出图。

---

## 3. 技术原理：为什么浏览器能跑 Python

| 角色 | 说明 |
| --- | --- |
| GitHub Pages | 纯静态托管，只发文件，**不执行任何代码** |
| Pyodide | 官方 CPython 经 Emscripten 交叉编译为 WASM（`pyodide.asm.wasm` ≈ 6.3MB），含完整标准库（`python_stdlib.zip` ≈ 6MB） |
| 浏览器 | 内置 WebAssembly 引擎（V8 / SpiderMonkey），加载并执行 wasm 中的 CPython |

**关键前提（已核实）**：跨域隔离头（COOP/COEP）只对 worker / `SharedArrayBuffer` 模式必需；本场景是毫秒级纯 CPU 计算，**主线程 `loadPyodide()` 即可**，GitHub Pages 无法设置自定义响应头也不受影响。

---

## 4. 目标架构

```
┌─ GitHub Pages 仓库（master 根）─────────────────────────────┐
│  index.html / app.js / genui-renderer.js  （现有，零改动）    │
│  a2ui-pyodide 外部仓库托管（core ~13MB 原始 / ~7MB 压缩）     │
│  scripts/config/*.json （fetch 注入）                        │
│  scripts/alt_to_dsl_converter.py （fetch 源码文本运行）        │
│  .nojekyll （阻止 Jekyll 处理二进制）                        │
└──────────────────────────────────────────────────────────────┘
        │ 静态托管（.wasm MIME 已支持）
        ▼
┌─ 浏览器 ─────────────────────────────────────────────────────┐
│  loadPyodide({ indexURL: PYODIDE_BASES[i] })  ← 懒加载       │
│  runPythonAsync(alt_to_dsl_converter.py 源码)                │
│  主线程执行 browser_convert(taskSpecJson, altText, ascText)  │
│  → 返回 { dslText, warnings, issues }                        │
│  dslText 写入 #dslInput → renderInput() → 画布渲染            │
└──────────────────────────────────────────────────────────────┘
```

---

## 5. 详细实施方案

### 5.1 转换器浏览器适配（`scripts/alt_to_dsl_converter.py` 轻改造）

原则：**CLI 行为完全不变**，只新增浏览器入口与配置注入能力。

**改造点 1（已实现）：配置注入（替代 `__file__` 路径）**

现状（顶层模块加载）：

```python
CONFIG_DIR = Path(__file__).resolve().parent / "config"
THEMES_PATH = CONFIG_DIR / "alt-themes.json"
LAYOUT_PROFILE_PATH = CONFIG_DIR / "alt-layout-profile.json"
TUNING_PATH = CONFIG_DIR / "alt-tuning.json"
THEME_CONFIG = load_json_config(THEMES_PATH)
LAYOUT_PROFILE = load_json_config(LAYOUT_PROFILE_PATH)
TUNING = load_json_config(TUNING_PATH)
```

改为「文件加载 + 可覆盖」双轨：

```python
_THEME_OVERRIDE: dict | None = None
_LAYOUT_OVERRIDE: dict | None = None
_TUNING_OVERRIDE: dict | None = None

def configure_runtime(
    themes_json: str | None = None,
    profile_json: str | None = None,
    tuning_json: str | None = None,
) -> None:
    """浏览器注入配置；CLI 不调用时保持文件加载行为。"""
    global _THEME_OVERRIDE, _LAYOUT_OVERRIDE, _TUNING_OVERRIDE
    _THEME_OVERRIDE = json.loads(themes_json) if themes_json else None
    _LAYOUT_OVERRIDE = json.loads(profile_json) if profile_json else None
    _TUNING_OVERRIDE = json.loads(tuning_json) if tuning_json else None
```

所有引用 `THEME_CONFIG` / `LAYOUT_PROFILE` / `TUNING` 的地方改为经 `theme_config()` / `layout_profile()` / `tuning()` 访问器读取 override 优先。文件加载逻辑保留（CLI 与单测不受影响）。

> **加载顺序是关键**：模块顶层 `load_json_config` 与 `validate_configuration()` 在浏览器中（无 `__file__`、无本地文件）会直接抛错。适配时需把顶层加载改为容错/惰性——文件缺失时不失败、先置空，`validate_configuration()` 延迟到 `configure_runtime()` 注入三份配置后执行；即 `runPythonAsync(源码)` 成功后调用 `configure_runtime(...)`，再触发校验。

> 落地方式：模块顶层对 `__file__` 缺失做容错（浏览器下三个配置置空、跳过校验），`configure_runtime()` 注入后重算派生常量（`_derive_tuning_constants()`）并执行 `validate_configuration()`；CLI 文件加载行为不变（已用 60 样例回归确认）。

**改造点 2（已实现）：ALT 文本解析入口**

`parse_alt(path)` 已拆出核心为 `parse_alt_text(text: str) -> AltDocument`，原函数变为「读文件 → 调文本版」，逻辑零改动。

**改造点 3（已实现）：浏览器统一入口**

```python
def browser_convert(
    task_spec_text: str, alt_text: str, asc_text: str,
    theme: str | None = None,
    width: float | str | None = None,
    height: float | str | None = None,
) -> str:
    """返回 JSON: {"dsl": str, "warnings": [...], "errors": [...]}"""
    canvas_override = build_canvas_override(width, height)  # None 表示保留默认
    try:
        result = compile_t2d(task_spec_text, alt_text, asc_text,
                             theme=theme, canvas_override=canvas_override)
    except ConversionError as exc:
        return json.dumps({"dsl": "", "errors": [str(exc)], "warnings": []},
                          ensure_ascii=False)
    errors = [f"[{i.node_id}] {i.message}" for i in result.issues
              if i.severity == "error"]
    warnings = result.notes + result.warnings + \
        [f"[{i.node_id}] {i.message}" for i in result.issues
         if i.severity == "warning"]
    return json.dumps({"dsl": "" if errors else result.dsl,
                       "errors": errors, "warnings": warnings},
                      ensure_ascii=False)
```

`main()` 的单卡 t2d 流程已抽成共享 `compile_t2d(task_spec_text, alt_text, asc_text, theme=None, canvas_override=None)`，`main()` 与 `browser_convert()` 共用，从结构上保证双端一致。`compile_t2d` 原样包含 `repair_auto_structure` / `repair_auto_depth` / `normalize_auto_bindings` / `repair_long_auto_text_roles` / `repair_narrow_text_row` / 低优先级裁剪重试等步骤；`auto_layout_document` 内部会 `raise ConversionError`（协议校验失败），浏览器入口用 try/except 捕获并转成错误 JSON。已用 `references/vals` 60 个样例回归：缺省参数下 CLI 输出与原 `alt_converter.py --mode t2d` 逐字节一致。

**改造点 4：`main()` 与 CLI 参数不动**——`argparse`、输入校验、`atomic_write` 全部保留，保证单卡转换 CLI（`--taskspec` / `--alt` / `--asc` / `-o`）行为不变。新增可选参数：

- `--theme THEME`：覆盖卡片主题，必须是 `alt-themes.json` 已声明主题（12 个）；缺省保留 ALT 主题或配置默认值；legacy（非自动布局）ALT 下忽略并给出提示。
- `--width VP` / `--height VP`：覆盖卡片宽高（自动布局同时作为排版画布），缺省保留尺寸默认（2x2 → 140x140，2x4 → 300x140）。

本脚本源自 `alt_converter.py` 的 t2d 分支，已是单卡形态，`compile_t2d` 抽取后与 `browser_convert` 一一对应。

### 5.2 Pyodide 集成（外部仓库 `IamJohnRain/a2ui-pyodide`）

**文件清单（core 包，非 full）**：

| 文件 | 说明 |
| --- | --- |
| `pyodide.mjs` / `pyodide.js` | loader（推荐 `pyodide.js`，传统 script 标签即可） |
| `pyodide.asm.js` | Emscripten 胶水层，加载 wasm 必需（core 包内含） |
| `pyodide.asm.wasm` | CPython 本体（≈6.3MB） |
| `python_stdlib.zip` | 标准库（≈6MB，含 unicodedata/ast/dataclasses 等） |
| `pyodide-lock.json` | 包锁文件（必需） |

获取方式：从 Pyodide GitHub Release 下载 `pyodide-core-<version>.tar.bz2`（当前稳定版如 `v0.26.x`），只解出上述 5 个文件。**不要带 full 包（200MB+）**。运行时已托管在独立仓库 [IamJohnRain/a2ui-pyodide](https://github.com/IamJohnRain/a2ui-pyodide)：raw 主源 `https://raw.githubusercontent.com/IamJohnRain/a2ui-pyodide/master/`（实测 ~3 倍于 Pages 的下载速度）+ GitHub Pages 兜底 `https://iamjohnrain.github.io/a2ui-pyodide/`，两者均返回 `Access-Control-Allow-Origin: *`；主仓库不再包含运行时。

> 运行时补丁：`a2ui-pyodide/pyodide.asm.js` 的 `__tzset_js` 在部分区域设置（如 zh 语言 + `GMT+8` 时区）下，`toLocaleTimeString(...,{timeZoneName:"short"})` 返回的时区名会与时间粘连（如 `GMT+820:35:30`），`.split(" ")[1]` 得到 `undefined`，导致 `stringToUTF8` 抛 `Cannot read properties of undefined (reading 'length')`，Pyodide 启动即崩溃。已在外部仓库用 `patch-tzset.py` 将 `extractZone` 改为带兜底的稳健提取（`p[1]`，否则去掉尾随时分秒，否则 `"UTC"`），升级 Pyodide 后需重新运行该脚本。

**加载策略（严格懒加载，不拖慢首屏）**：

页面打开**不下载任何 Pyodide 文件**（连 loader `pyodide.js` 也不预载，index.html 不写任何外部脚本）；只有用户首次点击「编译并渲染」时才从 `PYODIDE_BASES` 注入 loader 并下载 core 包（约 7MB 压缩传输）。加载期间显示原因提示 + 真实下载进度条（见 5.3），完成后自动继续编译并渲染；同一会话后续转换毫秒级。

```js
let pyodidePromise = null;
let pyodideLoading = false;
const PYODIDE_BASES = [
  'https://raw.githubusercontent.com/IamJohnRain/a2ui-pyodide/master/',
  'https://iamjohnrain.github.io/a2ui-pyodide/'
];

function loadPyodideLoader() {
  // 首次点击时才动态注入 loader，避免页面打开即下载
  return new Promise((resolve, reject) => {
    if (window.loadPyodide) return resolve();
    const tryBase = index => {
      if (index >= PYODIDE_BASES.length) return reject(new Error('Pyodide loader 下载失败'));
      const script = document.createElement('script');
      script.src = PYODIDE_BASES[index] + 'pyodide.js';
      script.onload = () => resolve();
      script.onerror = () => { script.remove(); tryBase(index + 1); };
      document.head.appendChild(script);
    };
    tryBase(0);
  });
}

async function ensurePyodide() {
  if (!pyodidePromise) {
    pyodideLoading = true;
    showAltLoading();                       // 显示原因提示 + 进度条（见 5.3.3）
    pyodidePromise = (async () => {
      installDownloadProgress();            // 拦截 fetch，统计 5 个 core 文件字节
      await loadPyodideLoader();
      let lastError = null;
      for (const base of PYODIDE_BASES) {
        try {
          const py = await loadPyodide({ indexURL: base });
          const converterSrc = await (await fetch('scripts/alt_to_dsl_converter.py')).text();
          const cfg1 = await (await fetch('scripts/config/alt-themes.json')).text();
          const cfg2 = await (await fetch('scripts/config/alt-layout-profile.json')).text();
          const cfg3 = await (await fetch('scripts/config/alt-tuning.json')).text();
          await py.runPythonAsync(converterSrc);
          await py.runPythonAsync(
            `import json; configure_runtime(${JSON.stringify(cfg1)}, ${JSON.stringify(cfg2)}, ${JSON.stringify(cfg3)})`
          );
          return py;
        } catch (error) { lastError = error; }
      }
      throw lastError || new Error('Pyodide 加载失败');
    })().catch(err => {
      pyodidePromise = null;
      throw err;
    }).finally(() => {
      pyodideLoading = false;
      hideAltLoading();
    });
  }
  return pyodidePromise;
}
```

**下载进度（fetch 字节拦截）**：`loadPyodide()` 内部通过 `fetch` 加载 5 个 core 文件，包装 `window.fetch` 对流式响应做字节累计，按 `content-length` 汇总成百分比；浏览器 HTTP 缓存命中时进度会快速跳到 100%。若环境不支持流式拦截（无 `content-length` 或 `response.body` 不可用），2 秒后自动切换为不定态进度条动画，文案仍说明等待原因。

```js
const CORE_FILES = ['pyodide.asm.wasm', 'python_stdlib.zip',
                    'pyodide.asm.js', 'pyodide.js', 'pyodide-lock.json'];
let progressState = { loaded: 0, total: 0 };

function installDownloadProgress() {
  const nativeFetch = window.fetch.bind(window);
  window.fetch = async (input, init) => {
    const response = await nativeFetch(input, init);
    const url = typeof input === 'string' ? input : (input && input.url) || '';
    if (!response.ok || !response.body || !CORE_FILES.some(f => url.includes(f))) {
      return response;
    }
    const total = Number(response.headers.get('content-length')) || 0;
    progressState.total += total;
    const reader = response.body.getReader();
    const stream = new ReadableStream({
      start(controller) {
        let loaded = 0;
        (function pump() {
          reader.read().then(({ done, value }) => {
            if (done) {
              progressState.loaded += loaded;
              controller.close();
              updateAltProgress();
              return;
            }
            loaded += value.byteLength;
            controller.enqueue(value);
            pump();
          }).catch(err => controller.error(err));
        })();
      }
    });
    return new Response(stream, response);
  };
}

function updateAltProgress() {
  const percent = progressState.total
    ? Math.min(100, Math.round(progressState.loaded / progressState.total * 100))
    : 0;
  $('#altLoadingBar').style.width = percent + '%';
  $('#altLoadingText').textContent =
    `${percent}%（${fmtBytes(progressState.loaded)} / ${fmtBytes(progressState.total)}）`;
}
```

**调用转换**：

```js
async function compileAlt(taskSpecText, altText, ascText, theme, width, height) {
  const py = await ensurePyodide();
  py.globals.set('__t', taskSpecText);
  py.globals.set('__a', altText);
  py.globals.set('__s', ascText);
  py.globals.set('__theme', theme);
  py.globals.set('__w', width === '' ? null : width);
  py.globals.set('__h', height === '' ? null : height);
  const result = await py.runPythonAsync(
    'browser_convert(__t, __a, __s, __theme, __w, __h)'
  );
  return JSON.parse(result);
}
```

> 注意：`browser_convert` 已返回 JSON 字符串，**不要再包 `json.dumps(...)`**（双重编码会让 `JSON.parse` 解析回字符串）。另外执行转换器源码前需剥离文件尾部的 `if __name__ == "__main__": raise SystemExit(main())`，否则 Pyodide（`__name__ == "__main__"`）会误触发 CLI。

**首次加载体验**：页面打开零下载；首次点击「编译并渲染」出现全屏遮罩，说明下载原因（本地转换引擎、约 7MB、仅首次需要、内容不上传）并展示真实下载进度条；下载完成后遮罩自动消失并继续编译渲染。失败时显示错误原因与「重试」按钮。

### 5.3 前端 UI 改造

**5.3.1 `index.html`**

- 左侧 `.source-tabs` 增加第三个页签：

```html
<button class="source-tab" data-source-tab="alt" role="tab" aria-selected="false">ALT 生成</button>
```

- 新增面板（置于 `#sourceTreePanel` 之后）：

```html
<div id="sourceAltPanel" class="source-tab-panel alt-tab-panel" hidden>
  <div class="alt-fields">
    <label class="alt-field"><span>TaskSpec（JSON）</span>
      <textarea id="altTaskSpec" spellcheck="false" placeholder='{"userQuery":"...","size":"2x2",...}'></textarea>
    </label>
    <label class="alt-field"><span>ALT 结构（card.alt.txt）</span>
      <textarea id="altInput" spellcheck="false" placeholder='Column root card=2x2 theme=neutral-light&#10;  Text title role=title'></textarea>
    </label>
    <label class="alt-field"><span>ASC 语义（card.asc.txt）</span>
      <textarea id="ascInput" spellcheck="false" placeholder='Text title text=今日用电'></textarea>
    </label>
  </div>
  <div class="alt-options">
    <label class="alt-option"><span>主题</span>
      <select id="altTheme"></select>
    </label>
    <label class="alt-option"><span>宽度（vp）</span>
      <input id="altWidth" type="number" min="1" step="1" placeholder="140">
    </label>
    <label class="alt-option"><span>高度（vp）</span>
      <input id="altHeight" type="number" min="1" step="1" placeholder="140">
    </label>
  </div>
  <div class="source-actions">
    <button id="altCompileBtn" class="primary grow">编译并渲染</button>
  </div>
  <div id="altReport" class="error-box" hidden></div>
</div>

<div id="altLoading" class="alt-loading" hidden>
  <div class="alt-loading-card" role="status" aria-live="polite">
    <div class="alt-loading-title">正在下载转换引擎</div>
    <div class="alt-loading-desc">首次使用需要在本地下载 Python 转换运行时（约 7MB），仅首次需要。下载与转换都在浏览器本地完成，你的内容不会上传。请稍候，下载完成后将自动编译并渲染。</div>
    <div class="alt-loading-track"><div id="altLoadingBar" class="alt-loading-bar"></div></div>
    <div id="altLoadingText" class="alt-loading-text">0%（0 B / 0 B）</div>
    <div id="altLoadingError" class="alt-loading-error" hidden></div>
    <button id="altLoadingRetry" class="primary" hidden>重试</button>
  </div>
</div>
```

**5.3.2 `add-ui.css`**

- `.source-tabs` 从 `grid-template-columns: 1fr 1fr` 改为 `1fr 1fr 1fr`；
- 新增 `.alt-tab-panel` 布局：三个 textarea 纵向排列、各占约 1/3 高度、等宽字体（沿用 `.source-panel>#sourceCodePanel>textarea` 的样式族）；
- 新增 `.alt-options` 一行三列：主题下拉 + 宽高数字输入，等宽字体、紧凑间距，紧随 textarea 之后；
- 新增 `.alt-loading` 全屏遮罩与进度条样式：半透明背景、居中卡片、8px 圆角轨道、强调色进度条（`.indeterminate` 时用滑动动画），文字用等宽数字便于读数。

```css
.alt-loading { position: fixed; inset: 0; z-index: 100; display: flex;
  align-items: center; justify-content: center;
  background: rgba(0, 0, 0, .45); }
.alt-loading-card { width: min(420px, 86vw); padding: 20px; border-radius: 12px;
  background: #fff; box-shadow: 0 8px 30px rgba(0, 0, 0, .25); }
.alt-loading-title { font-size: 16px; font-weight: 600; margin-bottom: 8px; }
.alt-loading-desc { font-size: 13px; line-height: 1.6; color: #555; margin-bottom: 14px; }
.alt-loading-track { height: 8px; border-radius: 4px;
  background: rgba(0, 0, 0, .1); overflow: hidden; }
.alt-loading-bar { height: 100%; width: 0; background: #4a7dff;
  transition: width .15s linear; }
.alt-loading-bar.indeterminate { width: 40%;
  animation: alt-loading-slide 1s ease-in-out infinite alternate; }
.alt-loading-text { margin-top: 8px; font-size: 12px; color: #888;
  font-variant-numeric: tabular-nums; }
.alt-loading-error { margin-top: 10px; font-size: 13px; color: #d33; }
@keyframes alt-loading-slide { from { margin-left: 0; } to { margin-left: 60%; } }
```

**5.3.3 `app.js`**

- 扩展 tab 切换逻辑（现为 code/tree 二值硬编码）：

```js
document.querySelectorAll('[data-source-tab]').forEach(tab => {
  tab.onclick = () => {
    const name = tab.dataset.sourceTab;
    $('#sourceCodePanel').hidden = name !== 'code';
    $('#sourceTreePanel').hidden = name !== 'tree';
    $('#sourceAltPanel').hidden = name !== 'alt';
    // ...aria-selected 切换
  };
});
```

- 新增 `altCompileBtn` 点击处理：

```js
// 主题下拉：由 alt-themes.json 的 themes 生成，默认「跟随 ALT」（不传覆盖参数，保留 ALT/配置默认）
const themeSelect = $('#altTheme');
themeSelect.innerHTML = '<option value="">跟随 ALT（默认）</option>' + Object.keys(themesConfig.themes)
  .map(t => `<option value="${t}">${t}</option>`).join('');
themeSelect.value = '';

// 宽高占位符 = 尺寸默认值：2x2 → 140/140，2x4 → 300/140；随 TaskSpec.size 刷新
function syncSizePlaceholders() {
  let size = null;
  try { size = JSON.parse($('#altTaskSpec').value || '{}').size; } catch (e) {}
  const [w, h] = size === '2x4' ? [300, 140] : [140, 140];
  $('#altWidth').placeholder = w;
  $('#altHeight').placeholder = h;
}
$('#altTaskSpec').addEventListener('input', syncSizePlaceholders);
syncSizePlaceholders();

function fmtBytes(n) {
  if (n < 1024) return n + ' B';
  if (n < 1048576) return (n / 1024).toFixed(1) + ' KB';
  return (n / 1048576).toFixed(1) + ' MB';
}

let altLoadingTimer = null;
function showAltLoading() {
  $('#altLoading').hidden = false;
  $('#altLoadingError').hidden = true;
  $('#altLoadingRetry').hidden = true;
  updateAltProgress();
  // 2 秒内拿不到总字节数（如无 content-length）则切换为不定态动画
  altLoadingTimer = setTimeout(() => {
    $('#altLoadingBar').classList.add('indeterminate');
  }, 2000);
}
function hideAltLoading() {
  clearTimeout(altLoadingTimer);
  $('#altLoading').hidden = true;
  $('#altLoadingBar').classList.remove('indeterminate');
}

async function compileAltAndRender() {
  try {
    $('#altReport').hidden = true;
    const result = await compileAlt(
      $('#altTaskSpec').value, $('#altInput').value, $('#ascInput').value,
      $('#altTheme').value,
      $('#altWidth').value, $('#altHeight').value
    );
    if (result.errors && result.errors.length) {
      showAltReport('编译失败（hard error）', result.errors);
      return;
    }
    if (result.warnings && result.warnings.length) {
      showAltReport('已编译，含警告', result.warnings);
    }
    if (result.dsl) {
      els.input.value = result.dsl;
      renderInput();
    }
  } catch (e) {
    if (pyodideLoading) {
      // 下载阶段失败：遮罩内显示错误 + 重试
      $('#altLoadingError').textContent = '下载失败：' + (e.message || e);
      $('#altLoadingError').hidden = false;
      $('#altLoadingRetry').hidden = false;
      $('#altLoadingBar').classList.add('indeterminate');
    } else {
      showAltReport('编译异常', [e.message || String(e)]);
    }
  }
}
$('#altCompileBtn').onclick = compileAltAndRender;

$('#altLoadingRetry').onclick = () => {
  pyodidePromise = null;   // 允许重新下载
  compileAltAndRender();
};
```

- 占位符行为：宽高输入框的 `placeholder` 默认显示尺寸默认值（灰色阴影）；用户输入后原生 placeholder 阴影消失、显示用户值；清空输入框则回退默认值（`browser_convert` 收到空值传 `null`）。

- 素材预览：生成 DSL 中的 `resources/base/media/*.svg` 经现有 `previewAssetPath` 自动映射，无需额外处理。

### 5.4 GitHub Pages 部署

| 项 | 动作 |
| --- | --- |
| `.nojekyll` | 仓库根新增空文件，阻止 Jekyll 处理 `vendor/` 二进制 |
| 相对路径 | 所有资源引用保持相对路径（现有项目已是） |
| MIME | `.wasm` GitHub Pages 返回 `application/wasm`，已支持；异常时 Pyodide 有非流式编译 fallback |
| 体积 | 主仓库不含运行时；外部仓库新增约 13MB（core 包），远低于单文件 100MB / 仓库 1GB 限制 |
| 缓存 | 浏览器缓存后二次访问零下载；GitHub Pages 无法自定义 Cache-Control，接受默认策略 |

### 5.5 升级 alt_to_dsl_converter.py 后的维护

转换器与前端在 `A2UIDesigner` master 根，GitHub Pages 自动发布（产物 <1MB）；运行时在 `a2ui-pyodide` 独立仓库（Pages/raw 双源，改文件即生效）。升级 Pyodide：下载新版 core 包 → 解压 5 个文件 → 运行 `patch-tzset.py pyodide.asm.js` → push `a2ui-pyodide` master；主仓库无需改动（若 loader API 变化再同步改 `app.js`）。

---

## 6. 代码改动清单

| 文件 | 改动 | 工作量 |
| --- | --- | --- |
| `scripts/alt_to_dsl_converter.py` | 配置注入双轨（含 `alt-tuning.json`）、`parse_alt_text` / `compile_t2d` / `browser_convert`、`--theme` / `--width` / `--height` | ✅ 已完成 |
| `scripts/alt_to_dsl_converter.py` | 单测：60 样例 CLI 与浏览器入口输出对比 | ✅ 已完成 |
| `IamJohnRain/a2ui-pyodide` | 外部仓库托管 core 0.26.4（含 tzset 补丁）+ Pages/raw 双源 | ✅ 已完成 |
| `index.html` | 第三页签 + ALT 面板（主题下拉 + 宽高输入）+ 加载遮罩/进度条 DOM（不写预载 script 标签） | ✅ 已完成 |
| `add-ui.css` | tabs 三列 + 面板样式 + `.alt-options` + 加载遮罩/进度条样式 | ✅ 已完成 |
| `app.js` | tab 切换扩展 + 主题下拉填充 + 宽高占位符同步 + 懒加载 loader 注入 + fetch 进度拦截 + 加载遮罩/重试 handler + 编译按钮 handler + pyodide 调用 | ✅ 已完成 |
| `.nojekyll` | 仓库根空文件（已存在） | ✅ 已完成 |

---

## 7. 测试与验证方案

1. **一致性回归（核心）**：取 `references/vals` 60 个样例（含 legacy 与自动布局两种形态），对每个 Case 分别用 CLI（`python scripts/alt_to_dsl_converter.py --taskspec <case>/task.taskSpec.json --alt <case>/card.alt.txt --asc <case>/card.asc.txt -o <out>/card.dsl.jsonl`）与浏览器入口转换，**diff 两份 DSL JSONL 必须完全一致**；
2. **画布视觉验证**：浏览器编译 → 画布渲染 → 与期望 PNG（如 `card.dsl.png`）对比；同时覆盖 140x140 与 300x140；
3. **失败路径**：构造超容量、超文本预算、Button 溢出等 ALT，确认错误信息在 ALT 页签清晰展示，且不输出 DSL；
4. **素材验证**：确认 `asset=N` 引用的 SVG 在画布正确出图（`references/media/` 映射）；
5. **控制台**：无 JS 报错；首次加载提示与状态正常。
6. **参数回归**：CLI `--theme` / `--width` / `--height` 与浏览器入口传入相同值时输出一致；缺省（不传）输出与旧行为完全一致（已用 60 样例验证）；非法主题 / 非正宽高应报错且不输出 DSL。
7. **占位符行为**：宽高输入框默认显示尺寸默认值阴影（140/140 或 300/140）；输入后阴影消失显示用户值；清空回退默认。
8. **首次加载体验**：页面打开时 Network 面板**无任何 Pyodide 运行时请求**（严格懒加载）；首次点击「编译并渲染」从 `PYODIDE_BASES` 主源（raw）下载并出现原因提示 + 进度条，进度与下载字节一致；主源失败自动切 Pages 兜底；下载完成自动编译并渲染；断网/加载失败显示错误与「重试」，重试成功；二次访问（浏览器缓存命中）不再出现加载遮罩。

> **已验证（本方案落地时）**：用 `tmp/run_luna_v4` 60 个 Case 在真实页面自动化（无头 Chrome）逐例执行「填三份文本 → 编译并渲染」，页面输出 DSL 与当前 CLI **60/60 逐字节一致**；主题下拉（lagoon-jewel）+ 宽高输入（200×120）与 CLI 同参数输出一致；首次编译显示原因提示 + 进度条，页面打开零 pyodide 请求，后续编译毫秒级。注：`run_luna_v4` 参考 DSL/PNG 由旧主题色板生成，与工作区新版 `alt-themes.json` 存在预期色差。

---

## 8. 风险与应对

| 风险 | 影响 | 应对 |
| --- | --- | --- |
| 首次加载 2–10s（国内访问 GitHub Pages 更慢） | 体验 | 严格懒加载（仅首次编译时下载）+ 原因提示 + 真实下载进度条 + 浏览器缓存；后续点击毫秒级 |
| 进度条依赖 fetch 流式拦截，旧浏览器可能拿不到字节数 | 无真实百分比 | 2s 内无总量数据自动切不定态动画，文案仍说明等待原因；进度仅增强体验，不阻塞功能 |
| 主线程执行 Python 阻塞 UI | 转换毫秒级，可忽略 | 若后续需要，可用 Service Worker 注入 COOP/COEP 头启用 worker 模式（进阶项） |
| `unicodedata` 等 stdlib 缺失 | 文本测量失败 | 已在 core 包内，无需额外加载；回归测试覆盖 |
| `browser_convert` 与 `main()` 逻辑漂移 | 双端不一致 | 抽取共享 `compile_t2d`，同源复用 |
| Pyodide 版本与脚本兼容 | 解释行为差异 | 锁定 core 包版本并记录于 README；升级需跑一致性回归 |
| 运行时 13MB 托管于外部仓库 | 主仓库 clone 不受影响；外部仓库部署 14MB 可能较慢 | Pages/raw 双源保证可用；外部仓库低频变更，慢一次可接受 |

---

## 9. 与方案 B / C 的关系

| 方案 | 定位 | 与 A 的关系 |
| --- | --- | --- |
| A（本方案） | 快速上线验证 | 当前 |
| B（JS 移植） | 终局：零运行时、秒开 | 以 A 验证的用例集为回归基准；切换后移除 `a2ui-pyodide` 依赖 |
| C（本地 HTTP 桥） | 仅调试 | A 开发期可临时用 C 快速联调 UI，再替换为 Pyodide |

**迁移路径**：A 上线 → 用 A 的一致性用例集驱动 B 的移植回归 → B 通过后切换，移除 Pyodide 依赖与体积。

---

## 10. 里程碑

| 阶段 | 内容 | 周期 |
| --- | --- | --- |
| M1 | 转换器适配 + 一致性回归 | 1–2 天 |
| M2 | Pyodide 集成 + UI 改造 | 1 天 |
| M3 | GitHub Pages 部署 + 全量验证 | 0.5–1 天 |
| M4 | （可选）启动方案 B 移植 | 后续 |

**验收标准**：ALT 页签输入三份文本 → 点击编译 → 画布渲染与 CLI 输出完全一致的卡片；错误路径信息清晰；GitHub Pages 线上可用。
