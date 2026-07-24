#!/usr/bin/env node
'use strict';

const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const { spawn } = require('node:child_process');

const projectRoot = path.resolve(__dirname, '..');
const defaultOutputName = 'card.dsl.png';

function usage() {
  return [
    '用法：node cli/render-card.js -i <card.dsl.jsonl> [-o <输出目录>] [-n <输出文件名>]',
    '',
    '参数：',
    '  -i, --input    输入的 card.dsl.jsonl 文件路径（必填）',
    '  -o, --output   输出目录；默认使用输入文件所在目录',
    `  -n, --name     输出文件名；默认为 ${defaultOutputName}`,
    '  -h, --help     显示帮助',
    '',
    '环境变量：',
    '  A2UI_BROWSER_PATH  Chrome、Edge 或 Chromium 可执行文件路径'
  ].join('\n');
}

function parseArguments(argv) {
  const options = {};
  const aliases = { '-i': 'input', '--input': 'input', '-o': 'output', '--output': 'output', '-n': 'name', '--name': 'name' };
  for (let index = 0; index < argv.length; index += 1) {
    const argument = argv[index];
    if (argument === '-h' || argument === '--help') return { help: true };
    const key = aliases[argument];
    if (!key) throw new Error(`未知参数：${argument}`);
    const value = argv[index + 1];
    if (!value || value.startsWith('-')) throw new Error(`参数 ${argument} 缺少值`);
    options[key] = value;
    index += 1;
  }
  if (!options.input) throw new Error('必须通过 -i 指定 card.dsl.jsonl 文件');
  return options;
}

function findBrowser() {
  const configured = process.env.A2UI_BROWSER_PATH;
  const candidates = [
    configured,
    process.platform === 'win32' && 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe',
    process.platform === 'win32' && 'C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe',
    process.platform === 'win32' && 'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe',
    process.platform === 'win32' && 'C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe',
    process.platform === 'darwin' && '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
    process.platform === 'darwin' && '/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge',
    process.platform === 'linux' && '/usr/bin/google-chrome',
    process.platform === 'linux' && '/usr/bin/chromium',
    process.platform === 'linux' && '/usr/bin/chromium-browser'
  ].filter(Boolean);
  return candidates.find(candidate => fs.existsSync(candidate));
}

function contentType(filePath) {
  return ({ '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.png': 'image/png',
    '.jpg': 'image/jpeg', '.jpeg': 'image/jpeg', '.webp': 'image/webp', '.svg': 'image/svg+xml' }[path.extname(filePath).toLowerCase()]
    || 'application/octet-stream');
}

function resolveAsset(source, inputDirectory) {
  if (!source || source.includes('\0')) return null;
  const normalized = source.replace(/\\/g, '/');
  const fileName = path.basename(normalized.split(/[?#]/)[0]);
  const candidates = [
    path.resolve(inputDirectory, normalized),
    path.resolve(inputDirectory, fileName),
    path.resolve(projectRoot, 'references', 'media', fileName)
  ];
  return candidates.find(candidate => fs.existsSync(candidate) && fs.statSync(candidate).isFile()) || null;
}

function startServer(inputDirectory) {
  const routes = new Map([
    ['/', path.join(__dirname, 'renderer.html')],
    ['/browser-renderer.js', path.join(__dirname, 'browser-renderer.js')],
    ['/genui-renderer.js', path.join(projectRoot, 'genui-renderer.js')]
  ]);
  const server = http.createServer((request, response) => {
    const url = new URL(request.url, 'http://127.0.0.1');
    const filePath = url.pathname === '/asset'
      ? resolveAsset(url.searchParams.get('source'), inputDirectory)
      : routes.get(url.pathname);
    if (!filePath || !fs.existsSync(filePath)) {
      response.writeHead(404).end('Not found');
      return;
    }
    response.writeHead(200, { 'Content-Type': contentType(filePath), 'Cache-Control': 'no-store' });
    fs.createReadStream(filePath).pipe(response);
  });
  return new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(0, '127.0.0.1', () => resolve({ server, port: server.address().port }));
  });
}

async function waitForDevTools(tempDirectory, child) {
  const activePortFile = path.join(tempDirectory, 'DevToolsActivePort');
  for (let attempt = 0; attempt < 200; attempt += 1) {
    if (child.exitCode != null) throw new Error(`浏览器启动失败，退出码 ${child.exitCode}`);
    if (fs.existsSync(activePortFile)) {
      const [port] = fs.readFileSync(activePortFile, 'utf8').trim().split(/\r?\n/);
      if (port) return Number(port);
    }
    await new Promise(resolve => setTimeout(resolve, 50));
  }
  throw new Error('等待浏览器调试端口超时');
}

async function openDebugPage(port, targetUrl) {
  const response = await fetch(`http://127.0.0.1:${port}/json/new?${encodeURIComponent(targetUrl)}`, { method: 'PUT' });
  if (!response.ok) throw new Error(`创建浏览器页面失败：HTTP ${response.status}`);
  return response.json();
}

function connectCdp(webSocketUrl) {
  const socket = new WebSocket(webSocketUrl);
  let nextId = 1;
  const pending = new Map();
  const listeners = new Map();
  socket.addEventListener('message', event => {
    const message = JSON.parse(event.data);
    if (message.id) {
      const request = pending.get(message.id);
      if (!request) return;
      pending.delete(message.id);
      if (message.error) request.reject(new Error(message.error.message));
      else request.resolve(message.result);
      return;
    }
    const callbacks = listeners.get(message.method) || [];
    callbacks.splice(0).forEach(callback => callback(message.params));
  });
  const ready = new Promise((resolve, reject) => {
    socket.addEventListener('open', resolve, { once: true });
    socket.addEventListener('error', () => reject(new Error('无法连接浏览器调试接口')), { once: true });
  });
  return {
    ready,
    send(method, params = {}) {
      return new Promise((resolve, reject) => {
        const id = nextId++;
        pending.set(id, { resolve, reject });
        socket.send(JSON.stringify({ id, method, params }));
      });
    },
    once(method) {
      return new Promise(resolve => {
        const callbacks = listeners.get(method) || [];
        callbacks.push(resolve);
        listeners.set(method, callbacks);
      });
    },
    close() { socket.close(); }
  };
}

async function renderWithBrowser(browserPath, pageUrl, dsl, outputPath) {
  const tempDirectory = fs.mkdtempSync(path.join(os.tmpdir(), 'a2ui-render-'));
  const browser = spawn(browserPath, [
    '--headless=new', '--disable-gpu', '--hide-scrollbars', '--no-first-run', '--no-default-browser-check',
    '--remote-debugging-port=0', `--user-data-dir=${tempDirectory}`, 'about:blank'
  ], { stdio: 'ignore', windowsHide: true });
  let cdp;
  try {
    const debugPort = await waitForDevTools(tempDirectory, browser);
    const page = await openDebugPage(debugPort, pageUrl);
    cdp = connectCdp(page.webSocketDebuggerUrl);
    await cdp.ready;
    await cdp.send('Page.enable');
    await cdp.send('Runtime.enable');
    const load = cdp.once('Page.loadEventFired');
    await cdp.send('Page.navigate', { url: pageUrl });
    await load;
    const expression = `window.CardCliRenderer.renderCard(${JSON.stringify(dsl)})`;
    const rendered = await cdp.send('Runtime.evaluate', { expression, returnByValue: true, awaitPromise: true });
    if (rendered.exceptionDetails) throw new Error(rendered.exceptionDetails.exception?.description || '页面渲染失败');
    const size = rendered.result.value;
    await cdp.send('Emulation.setDeviceMetricsOverride', {
      width: Math.ceil(size.width), height: Math.ceil(size.height), deviceScaleFactor: 1, mobile: false
    });
    const assets = await cdp.send('Runtime.evaluate', {
      expression: 'window.CardCliRenderer.waitForAssets()', awaitPromise: true, returnByValue: true
    });
    if (assets.exceptionDetails) throw new Error(assets.exceptionDetails.exception?.description || '等待资源失败');
    const screenshot = await cdp.send('Page.captureScreenshot', {
      format: 'png', fromSurface: true, captureBeyondViewport: true,
      clip: { x: 0, y: 0, width: size.width, height: size.height, scale: 1 }
    });
    fs.writeFileSync(outputPath, Buffer.from(screenshot.data, 'base64'));
    return size;
  } finally {
    if (cdp) cdp.close();
    if (browser.exitCode == null) {
      const exited = new Promise(resolve => browser.once('exit', resolve));
      browser.kill();
      await exited;
    }
    for (let attempt = 0; attempt < 20; attempt += 1) {
      try {
        fs.rmSync(tempDirectory, { recursive: true, force: true });
        break;
      } catch (error) {
        if (attempt === 19) throw error;
        await new Promise(resolve => setTimeout(resolve, 50));
      }
    }
  }
}

async function main() {
  let options;
  try { options = parseArguments(process.argv.slice(2)); }
  catch (error) {
    console.error(`错误：${error.message}\n\n${usage()}`);
    process.exitCode = 2;
    return;
  }
  if (options.help) {
    console.log(usage());
    return;
  }
  const inputPath = path.resolve(options.input);
  if (!fs.existsSync(inputPath) || !fs.statSync(inputPath).isFile()) {
    throw new Error(`输入文件不存在：${inputPath}`);
  }
  const outputDirectory = options.output ? path.resolve(options.output) : path.dirname(inputPath);
  const outputName = options.name || defaultOutputName;
  if (path.basename(outputName) !== outputName) throw new Error('-n 只能指定文件名，目录请使用 -o');
  fs.mkdirSync(outputDirectory, { recursive: true });
  const outputPath = path.join(outputDirectory, outputName);
  if (path.resolve(outputPath).toLowerCase() === inputPath.toLowerCase()) {
    throw new Error('输出文件不能覆盖输入 DSL 文件');
  }
  const browserPath = findBrowser();
  if (!browserPath) throw new Error('未找到 Chrome、Edge 或 Chromium；可通过 A2UI_BROWSER_PATH 指定浏览器路径');
  const dsl = fs.readFileSync(inputPath, 'utf8');
  const { server, port } = await startServer(path.dirname(inputPath));
  try {
    const size = await renderWithBrowser(browserPath, `http://127.0.0.1:${port}/`, dsl, outputPath);
    console.log(`已生成：${outputPath} (${size.width}x${size.height})`);
  } finally {
    await new Promise(resolve => server.close(resolve));
  }
}

main().catch(error => {
  console.error(`渲染失败：${error.message}`);
  process.exitCode = 1;
});
