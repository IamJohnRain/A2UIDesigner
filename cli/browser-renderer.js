(() => {
  const renderer = window.GenUIRenderer;
  const containerTypes = new Set(['Row', 'Column', 'Stack', 'List']);

  function parseJsonl(text) {
    const lines = text.split(/\r?\n/).map(line => line.trim()).filter(Boolean);
    if (!lines.length) throw new Error('输入文件不包含 JSONL 消息');
    const messages = lines.map((line, index) => {
      try { return JSON.parse(line); }
      catch (error) { throw new Error(`第 ${index + 1} 行 JSON 无效：${error.message}`); }
    });
    const create = messages.find(message => message.createSurface);
    const update = messages.find(message => message.updateComponents);
    const data = messages.find(message => message.updateDataModel);
    if (!create || !update) throw new Error('DSL 至少需要 createSurface 和 updateComponents 消息');
    if (!Array.isArray(update.updateComponents.components)) {
      throw new Error('updateComponents.components 必须是数组');
    }
    return { create, update, data };
  }

  function renderCard(text) {
    const parsed = parseJsonl(text);
    const dataRoot = parsed.data?.updateDataModel?.value || {};
    const components = parsed.update.updateComponents.components;
    const componentMap = new Map(components.map(component => [component.id, component]));

    function getPath(path, local) {
      const root = path.startsWith('/') ? dataRoot : local || dataRoot;
      const parts = path.replace(/^\//, '').split('/').filter(Boolean)
        .map(part => part.replace(/~1/g, '/').replace(/~0/g, '~'));
      return parts.reduce((value, key) => value == null ? '' : value[key], root);
    }

    function evalBinding(value, local) {
      return renderer.evaluateBinding(value, getPath, local);
    }

    function previewAssetPath(source) {
      if (typeof source !== 'string' || !source) return '';
      if (source.startsWith('data:') || source.startsWith('blob:') || /^(https?:)?\/\//i.test(source)) return source;
      return `/asset?source=${encodeURIComponent(source)}`;
    }

    function makeNode(id, local) {
      const component = componentMap.get(id);
      if (!component) throw new Error(`缺少组件：${id}`);
      const element = document.createElement('div');
      element.className = 'dsl-node';
      element.dataset.id = component.id;
      if (containerTypes.has(component.component)) {
        const childIds = Array.isArray(component.children) ? component.children : [];
        childIds.forEach(childId => element.appendChild(makeNode(childId, local)));
      } else {
        renderer.renderLeaf(element, component, { evalBinding, previewAssetPath, local });
      }
      renderer.apply(element, component, { evalBinding, previewAssetPath, local });
      return element;
    }

    const surface = parsed.create.createSurface;
    const width = Math.max(1, Number(surface.width) || 140);
    const height = Math.max(1, Number(surface.height) || 140);
    const canvas = document.querySelector('#cardCanvas');
    canvas.replaceChildren(makeNode(parsed.update.updateComponents.root));
    canvas.style.width = `${width}px`;
    canvas.style.height = `${height}px`;
    renderer.finalize(canvas.firstElementChild);
    return { width, height };
  }

  async function waitForAssets() {
    if (document.fonts?.ready) await document.fonts.ready;
    const root = document.querySelector('#cardCanvas')?.firstElementChild;
    if (root) renderer.finalize(root);
    await Promise.all([...document.images].map(image => image.complete
      ? Promise.resolve()
      : new Promise(resolve => {
        image.addEventListener('load', resolve, { once: true });
        image.addEventListener('error', resolve, { once: true });
      })));
    await new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)));
  }

  window.CardCliRenderer = Object.freeze({ renderCard, waitForAssets });
})();
