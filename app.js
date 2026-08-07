(() => {
    let assetCatalog=Array.isArray(window.MediaAssetCatalog)?window.MediaAssetCatalog:[];
    const treeExpanded=new Set(); let pendingAssetSrc='';
    function canContain(component){return !!component&&(containerTypes.has(component.component)||Array.isArray(component.children))}
    function componentDefaults(type,src=''){const defs={Text:{content:'新文字',styles:{width:80,height:20,fontSize:14,fontColor:'#E5000000'}},Button:{label:'按钮',styles:{width:80,height:32,fontSize:14,fontColor:'#FFFFFFFF',backgroundColor:'#FF5B5CE2',borderRadius:16}},Image:{src,styles:{width:40,height:40,objectFit:'contain'}},Progress:{value:40,total:100,styles:{width:80,height:8,type:'linear',color:'#FF5B5CE2',backgroundColor:'#22000000',borderRadius:4}},Divider:{styles:{width:80,height:1,color:'#22000000'}},Row:{children:[],itemMargin:4,styles:{width:100,height:50,alignItems:'center'}},Column:{children:[],itemMargin:4,styles:{width:100,height:60}},Stack:{children:[],styles:{width:80,height:80,alignContent:'center'}}};return {id:uniqueId(type),component:type,...defs[type]}}
    function updateAddControls(){const selected=state.map.get(state.selectedId),hint=document.querySelector('.add-section p'),canAddLeaf=canContain(selected),canAddContainer=!!selected&&(canContain(selected)||!!findParent(selected.id));document.querySelectorAll('[data-add]').forEach(button=>{button.disabled=containerTypes.has(button.dataset.add)?!canAddContainer:!canAddLeaf});if(!hint)return;hint.classList.toggle('add-error',!!selected&&!canContain(selected));hint.textContent=!state.update?'请先渲染一份 DSL。':!selected?'请选择组件后再添加元素。':canContain(selected)?`将在 ${selected.component} · ${selected.id} 内部添加子组件。`:`${selected.component} 不能包含子组件；可添加容器来包裹它。`}
    function addComponentSafely(type,src=''){if(!state.update)return toast('请先渲染一份 DSL');const selected=state.map.get(state.selectedId),isLayout=containerTypes.has(type);if(!selected)return toast('请先选择组件');if(!isLayout&&!canContain(selected))return toast('当前组件不能包含子组件');const component=componentDefaults(type,src);if(isLayout&&!canContain(selected)){const parent=findParent(selected.id);if(!parent)return toast('根组件不能被包裹');mutate(()=>{const index=parent.children.indexOf(selected.id);component.children=[selected.id];parent.children.splice(index,1,component.id);state.components.push(component)});select(component.id);toast(`已用 ${type} 包裹 ${selected.component} · ${selected.id}`);return}mutate(()=>{selected.children=Array.isArray(selected.children)?selected.children:[];selected.children.push(component.id);state.components.push(component)});select(component.id);toast(`已在 ${selected.component} · ${selected.id} 内添加 ${type}`)}
    function renderLayoutTree(){const root=$('#layoutTree');if(!root)return;root.innerHTML='';if(!state.update){root.innerHTML='<div class="tree-empty">渲染 DSL 后显示组件布局。</div>';return}const rootId=state.update.updateComponents.root,seen=new Set(),icons={Text:'T',Button:'B',Image:'▧',Progress:'%',Divider:'—',Row:'↔',Column:'↕',Stack:'▣',List:'☷'};const addRow=(id,depth,orphan=false)=>{const c=state.map.get(id),row=document.createElement('button');row.type='button';row.className='tree-row'+(id===state.selectedId?' active':'')+(orphan?' tree-orphan':'');row.style.paddingLeft=(6+depth*14)+'px';if(!c){row.innerHTML=`<span class="tree-expander"></span><span class="tree-icon">!</span><span class="tree-label">缺失组件</span><span class="tree-id">${id}</span><span class="tree-warning">⚠</span>`;root.appendChild(row);return}const children=Array.isArray(c.children)?c.children:[],expandable=children.length>0,expanded=treeExpanded.has(id)||id===rootId;row.innerHTML=`<span class="tree-expander">${expandable?(expanded?'⌄':'›'):''}</span><span class="tree-icon">${icons[c.component]||'□'}</span><span class="tree-label">${c.component}</span><span class="tree-id">${c.id}</span>${orphan?'<span class="tree-warning" title="未挂载组件">⚠</span>':''}`;row.onclick=e=>{if(expandable&&e.target.classList.contains('tree-expander')){treeExpanded.has(id)?treeExpanded.delete(id):treeExpanded.add(id);renderLayoutTree();return}select(id);requestAnimationFrame(()=>document.querySelector(`.dsl-node[data-id="${CSS.escape(id)}"]`)?.scrollIntoView({block:'center',inline:'center',behavior:'smooth'}))};root.appendChild(row);seen.add(id);if(expanded)children.forEach(child=>addRow(child,depth+1,orphan))};addRow(rootId,0);state.components.filter(c=>!seen.has(c.id)).forEach(c=>addRow(c.id,0,true))}
    async function loadAssetCatalog(){if(!assetCatalog.length)throw Error('素材清单加载失败');return assetCatalog}
    function assetPreview(name){if(name==='layered_image.json')return '<span class="layered-asset"><img src="references/media/background.png" alt=""><img src="references/media/foreground.png" alt=""></span>';const src=`references/media/${encodeURIComponent(name)}`;return `<img src="${src}" alt="${name}">`}
    async function showAssetDialog(){if(!canContain(state.map.get(state.selectedId)))return toast('请先选择容器，再添加图片');pendingAssetSrc='';const dialog=$('#assetDialog'),grid=$('#assetGrid');grid.innerHTML='<div class="tree-empty">正在加载素材…</div>';dialog.hidden=false;$('#assetPreviewImage').textContent='请选择素材';$('#assetPreviewName').textContent='未选择素材';$('#insertAssetBtn').disabled=true;try{await loadAssetCatalog()}catch(error){grid.innerHTML='<div class="tree-empty">素材加载失败，请刷新后重试。</div>';return}grid.innerHTML='';assetCatalog.forEach(name=>{const button=document.createElement('button');button.type='button';button.className='asset-item';button.innerHTML=`<span class="asset-thumb">${assetPreview(name)}</span><span>${name}</span>`;button.onclick=()=>{pendingAssetSrc=`resources/base/media/${name}`;grid.querySelectorAll('.asset-item').forEach(x=>x.classList.remove('selected'));button.classList.add('selected');$('#assetPreviewImage').innerHTML=assetPreview(name);$('#assetPreviewName').textContent=name;$('#insertAssetBtn').disabled=false};grid.appendChild(button)})}
    function closeAssetDialog(){$('#assetDialog').hidden=true;pendingAssetSrc=''}
  const $ = s => document.querySelector(s);
  const renderer = window.GenUIRenderer;
  const state = { messages: [], create: null, update: null, dataMsg: null, components: [], map: new Map(), selectedId: null, scale: 2, history: [], future: [], fileName: 'card.dsl.jsonl' };
  const containerTypes = new Set(['Row','Column','Stack','List']);
  const els = { input:$('#dslInput'), canvas:$('#cardCanvas'), stage:$('#stage'), empty:$('#emptyStage'), error:$('#errorBox'), parse:$('#parseState'), editor:$('#propertyEditor'), noSel:$('#noSelection') };

  function parseJsonl(text){
    const lines=text.split(/\r?\n/).map(x=>x.trim()).filter(Boolean); if(!lines.length) throw Error('请输入 DSL JSONL');
    const messages=lines.map((line,i)=>{try{return JSON.parse(line)}catch(e){throw Error(`第 ${i+1} 行 JSON 无效：${e.message}`)}});
    const create=messages.find(x=>x.createSurface), update=messages.find(x=>x.updateComponents), dataMsg=messages.find(x=>x.updateDataModel);
    if(!create||!update) throw Error('DSL 至少需要 createSurface 和 updateComponents 消息');
    if(!Array.isArray(update.updateComponents.components)) throw Error('updateComponents.components 必须是数组');
    return {messages,create,update,dataMsg};
  }
  function loadParsed(parsed, resetHistory=true){Object.assign(state,parsed);state.components=state.update.updateComponents.components;reindex();state.selectedId=null;if(resetHistory){state.history=[];state.future=[]}renderAll();syncSource();setStatus('已渲染','ok')}
  function reindex(){state.map=new Map(state.components.map(c=>[c.id,c]))}
  function dataRoot(){return state.dataMsg?.updateDataModel?.value||{}}
  function getPath(path, local){const root=path.startsWith('/')?dataRoot():local||dataRoot();const parts=path.replace(/^\//,'').split('/').filter(Boolean).map(x=>x.replace(/~1/g,'/').replace(/~0/g,'~'));return parts.reduce((v,k)=>v==null?'':v[k],root)}
  function evalBinding(value,local){return renderer.evaluateBinding(value,getPath,local)}
  function previewAssetPath(src){
    if(typeof src!=='string'||!src)return '';
    if(src.startsWith('data:')||src.startsWith('blob:')||/^(https?:)?\/\//i.test(src))return src;
    const clean=src.replace(/\\/g,'/');
    const fileName=clean.split('/').pop();
    return clean.startsWith('resources/')?`references/media/${encodeURIComponent(fileName)}`:src;
  }
  function protocolAssetPath(src){
    if(typeof src!=='string'||!src||src.includes('{{'))return src;
    const clean=src.replace(/\\/g,'/');
    const marker='/references/media/';
    const markerAt=clean.toLowerCase().indexOf(marker);
    const relativeAt=clean.toLowerCase().indexOf('references/media/');
    if(markerAt<0&&relativeAt<0)return src;
    const fileName=decodeURIComponent(clean.split('/').pop().split(/[?#]/)[0]);
    return `resources/base/media/${fileName}`;
  }
  function normalizeExportAssets(messages){
    const copy=structuredClone(messages);
    const visit=value=>{
      if(!value||typeof value!=='object')return;
      for(const [key,child] of Object.entries(value)){
        if((key==='src'||key==='backgroundImage')&&typeof child==='string')value[key]=protocolAssetPath(child);
        else visit(child);
      }
    };
    copy.forEach(visit);
    return copy;
  }
  function applyStyles(el,c,local){renderer.apply(el,c,{evalBinding,previewAssetPath,local})}
  function makeNode(id,local){const c=state.map.get(id);if(!c){const x=document.createElement('div');x.textContent=`缺失: ${id}`;return x}const el=document.createElement('div');el.className='dsl-node';el.dataset.id=c.id;el.dataset.label=`${c.component} · ${c.id}`;el.draggable=c.id!==state.update.updateComponents.root;
    if(containerTypes.has(c.component)){let ids=Array.isArray(c.children)?c.children:[];ids.forEach(cid=>el.appendChild(makeNode(cid,local)))}
    else renderer.renderLeaf(el,c,{evalBinding,previewAssetPath,local});
    applyStyles(el,c,local);bindNodeEvents(el,c);return el}
  function bindNodeEvents(el,c){el.addEventListener('click',e=>{e.stopPropagation();select(c.id)});el.addEventListener('dragstart',e=>{e.stopPropagation();e.dataTransfer.setData('text/plain',c.id);setTimeout(()=>el.style.opacity='.35')});el.addEventListener('dragend',()=>el.style.opacity='');el.addEventListener('dragover',e=>{e.preventDefault();e.stopPropagation();el.classList.add('drag-over')});el.addEventListener('dragleave',()=>el.classList.remove('drag-over'));el.addEventListener('drop',e=>{e.preventDefault();e.stopPropagation();el.classList.remove('drag-over');const moving=e.dataTransfer.getData('text/plain');containerTypes.has(c.component)?moveInto(moving,c.id):moveBefore(moving,c.id)})}
  function renderAll(){reindex();const rootId=state.update.updateComponents.root;els.canvas.innerHTML='';const node=makeNode(rootId);els.canvas.appendChild(node);els.canvas.style.width=(state.create.createSurface.width||140)+'px';els.canvas.style.height=(state.create.createSurface.height||140)+'px';els.stage.hidden=false;els.empty.hidden=true;renderer.finalize(node);document.fonts?.ready.then(()=>{if(node.isConnected)renderer.finalize(node)});updateScale();if(state.selectedId)select(state.selectedId,false)}
  function updateScale(){state.scale=Math.max(.5,Math.min(4,state.scale));$('#canvasWrap').style.transform=`scale(${state.scale})`;$('#zoomLabel').textContent=Math.round(state.scale*100)+'%'}
  function findParent(id){return state.components.find(c=>Array.isArray(c.children)&&c.children.includes(id))}
  function snapshot(){state.history.push(JSON.stringify(state.messages));if(state.history.length>60)state.history.shift();state.future=[]}
  function restore(raw){const p=parseJsonl(JSON.parse(raw).map(x=>JSON.stringify(x)).join('\n'));loadParsed(p,false)}
  function mutate(fn){snapshot();fn();reindex();syncSource();renderAll()}
  function moveInto(id,targetId){if(!id||id===targetId)return;const target=state.map.get(targetId);if(!target||!containerTypes.has(target.component))return;let p=state.map.get(targetId);while(p){if(p.id===id)return;p=findParent(p.id)}mutate(()=>{const old=findParent(id);if(old)old.children=old.children.filter(x=>x!==id);target.children=Array.isArray(target.children)?target.children:[];target.children.push(id)});select(id)}
  function moveBefore(id,beforeId){if(!id||id===beforeId)return;const targetParent=findParent(beforeId);if(!targetParent)return;let p=targetParent;while(p){if(p.id===id)return;p=findParent(p.id)}mutate(()=>{const old=findParent(id);if(old)old.children=old.children.filter(x=>x!==id);const at=targetParent.children.indexOf(beforeId);targetParent.children.splice(at,0,id)});select(id)}
  function clearSelectionVisuals(){document.querySelectorAll('.editor-selected').forEach(x=>x.classList.remove('editor-selected'));document.querySelectorAll('.resize-handle').forEach(x=>x.remove())}
  function deselect(){clearSelectionVisuals();state.selectedId=null;els.editor.hidden=true;els.noSel.hidden=false;$('#selectionHint').textContent='选择画布中的元素'}
  function select(id,rerender=true){if(!state.map.has(id))return;state.selectedId=id;clearSelectionVisuals();const node=document.querySelector(`.dsl-node[data-id="${CSS.escape(id)}"]`);if(node){node.classList.add('editor-selected');const handle=document.createElement('span');handle.className='resize-handle';node.appendChild(handle);startResize(handle,id)}els.noSel.hidden=true;els.editor.hidden=false;buildInspector(state.map.get(id));if(rerender){} }
  function startResize(handle,id){handle.onpointerdown=e=>{e.stopPropagation();e.preventDefault();const c=state.map.get(id),node=handle.parentElement,startX=e.clientX,startY=e.clientY,w=Number(c.styles?.width)||node.offsetWidth,h=Number(c.styles?.height)||node.offsetHeight;snapshot();handle.setPointerCapture(e.pointerId);handle.onpointermove=ev=>{c.styles=c.styles||{};c.styles.width=Math.max(4,Math.round(w+(ev.clientX-startX)/state.scale));c.styles.height=Math.max(4,Math.round(h+(ev.clientY-startY)/state.scale));node.style.width=c.styles.width+'px';node.style.height=c.styles.height+'px';syncSource()};handle.onpointerup=()=>{renderAll();select(id)}}}
  function field(label,key,value,type='number',options){const wrap=document.createElement('label');wrap.className='field';wrap.innerHTML=`<span>${label}</span>`;let input;if(options){input=document.createElement('select');options.forEach(v=>input.add(new Option(v||'—',v)))}else{input=document.createElement('input');input.type=type;if(type==='number')input.step='1'}input.value=value??'';input.dataset.key=key;input.addEventListener('change',()=>applyField(key,input.value,type));wrap.appendChild(input);return wrap}
  function pickerColor(value){if(/^#[0-9a-f]{8}$/i.test(value||''))return '#'+value.slice(3);if(/^#[0-9a-f]{6}$/i.test(value||''))return value;return '#ffffff'}
  function colorField(label,key,value){
    const palette=['#FFFFFF','#F2F3F5','#191A1C','#000000','#5B5CE2','#0A59F7','#64BB5C','#ED6F21','#D94838','#F5DC62'];
    const wrap=document.createElement('div');wrap.className='field full color-field';wrap.innerHTML=`<span>${label}</span>`;
    const controls=document.createElement('div');controls.className='color-controls';
    const picker=document.createElement('input');picker.type='color';picker.className='color-native';picker.value=pickerColor(value);picker.title='打开系统色板';
    const textInput=document.createElement('input');textInput.type='text';textInput.className='color-code';textInput.value=value??'';textInput.placeholder='#AARRGGBB 或 #RRGGBB';
    const applyPicker=()=>{const current=textInput.value.trim();const next=/^#[0-9a-f]{8}$/i.test(current)?'#'+current.slice(1,3)+picker.value.slice(1):picker.value.toUpperCase();textInput.value=next;applyField(key,next,'text')};
    picker.addEventListener('change',applyPicker);textInput.addEventListener('change',()=>{picker.value=pickerColor(textInput.value);applyField(key,textInput.value.trim(),'text')});
    controls.append(picker,textInput);wrap.appendChild(controls);
    const swatches=document.createElement('div');swatches.className='color-swatches';
    palette.forEach(color=>{const button=document.createElement('button');button.type='button';button.className='color-swatch';button.style.backgroundColor=color;button.title=color;button.setAttribute('aria-label',`选择颜色 ${color}`);button.onclick=()=>{const current=textInput.value.trim();const next=/^#[0-9a-f]{8}$/i.test(current)?'#'+current.slice(1,3)+color.slice(1):color;textInput.value=next;picker.value=color;applyField(key,next,'text')};swatches.appendChild(button)});
    wrap.appendChild(swatches);return wrap;
  }
  function applyField(key,val,type){
    const c=state.map.get(state.selectedId);if(!c)return;snapshot();
    const parts=key.split('.');let target=c;
    for(let i=0;i<parts.length-1;i++){const part=parts[i],next=parts[i+1];if(target[part]==null||typeof target[part]!=='object')target[part]=/^\d+$/.test(next)?[]:{};target=target[part]}
    const last=parts[parts.length-1];if(val==='')delete target[last];else target[last]=type==='number'?Number(val):type==='checkbox'?!!val:val;
    syncSource();renderAll();select(c.id)
  }
  function buildInspector(c){
    $('#selectedType').textContent=c.component;$('#selectedId').textContent=c.id;$('#selectionHint').textContent='正在编辑组件';
    const content=$('#contentFields'),size=$('#sizeFields'),app=$('#appearanceFields'),text=$('#textFields'),layout=$('#layoutFields'),s=c.styles||{};
    [content,size,app,text,layout].forEach(x=>x.innerHTML='');
    const contentKey=c.component==='Text'?'content':c.component==='Button'?'label':c.component==='Image'?'src':null;
    if(contentKey)content.appendChild(field(contentKey==='src'?'图片路径':'显示内容',contentKey,c[contentKey],'text'));
    ['width','height'].forEach(k=>size.appendChild(field(k==='width'?'宽度':'高度','styles.'+k,s[k])));
    size.appendChild(field('内边距','styles.padding',typeof s.padding==='number'?s.padding:'','number'));size.appendChild(field('元素间距','itemMargin',c.itemMargin));
    app.appendChild(colorField('背景颜色','styles.backgroundColor',s.backgroundColor));
    app.appendChild(colorField('边框颜色','styles.borderColor',s.borderColor));
    if(c.component==='Progress'||c.component==='Divider')app.appendChild(colorField(c.component==='Progress'?'进度颜色':'分割线颜色','styles.color',s.color));
    if(c.component==='Checkbox'){
      app.appendChild(colorField('选中颜色','styles.selectedColor',s.selectedColor));
      app.appendChild(colorField('未选中颜色','styles.unSelectedColor',s.unSelectedColor));
      app.appendChild(colorField('勾选标记颜色','styles.mark.strokeColor',s.mark?.strokeColor));
    }
    if(s.shadow)app.appendChild(colorField('阴影颜色','styles.shadow.color',s.shadow.color));
    if(Array.isArray(s.linearGradient?.colors))s.linearGradient.colors.forEach((stop,index)=>app.appendChild(colorField(`渐变色 ${index+1}`,`styles.linearGradient.colors.${index}.0`,stop?.[0])));
    ['borderRadius','opacity'].forEach(k=>app.appendChild(field({borderRadius:'圆角',opacity:'透明度'}[k],'styles.'+k,s[k],'number')));
    const isText=['Text','Button'].includes(c.component);$('#textSection').hidden=!isText;
    if(isText){text.appendChild(field('字号','styles.fontSize',s.fontSize));text.appendChild(field('字重','styles.fontWeight',s.fontWeight,'text'));text.appendChild(colorField('文字颜色','styles.fontColor',s.fontColor));text.appendChild(field('对齐','styles.textAlign',s.textAlign,'text',['','start','center','end']))}
    const isContainer=containerTypes.has(c.component);$('#layoutSection').hidden=!isContainer;
    if(isContainer){layout.appendChild(field('主轴对齐','styles.justifyContent',s.justifyContent,'text',['','start','center','end','spaceBetween','spaceAround']));layout.appendChild(field('交叉轴对齐','styles.alignItems',s.alignItems,'text',['','start','center','end','stretch']))}
    $('#componentJson').value=JSON.stringify(c,null,2)
  }
  function syncSource(){els.input.value=normalizeExportAssets(state.messages).map(x=>JSON.stringify(x)).join('\n')}
  function setStatus(t,cls){els.parse.textContent=t;els.parse.className='status '+cls;els.error.hidden=true}
  function fail(e){els.error.textContent=e.message||e;els.error.hidden=false;els.parse.textContent='解析失败';els.parse.className='status bad';pushRenderWarnings('渲染失败',[e.message||String(e)],'error')}
  function renderInput(){try{loadParsed(parseJsonl(els.input.value));if(window.innerWidth<=760)requestAnimationFrame(()=>els.stage.scrollIntoView({behavior:'smooth',block:'center'}))}catch(e){fail(e)}}
  function uniqueId(base){let id=base.toLowerCase(),n=1;while(state.map.has(id))id=base.toLowerCase()+'_'+n++;return id}
  function addComponent(type){if(!state.update)return toast('请先渲染一份 DSL');const id=uniqueId(type);const defs={Text:{content:'新文字',styles:{width:80,height:20,fontSize:14,fontColor:'#E5000000'}},Button:{label:'按钮',styles:{width:80,height:32,fontSize:14,fontColor:'#FFFFFFFF',backgroundColor:'#FF5B5CE2',borderRadius:16}},Image:{src:'',styles:{width:40,height:40,objectFit:'contain'}},Progress:{value:40,total:100,styles:{width:80,height:8,type:'linear',color:'#FF5B5CE2',backgroundColor:'#22000000',borderRadius:4}},Divider:{styles:{width:80,height:1,color:'#22000000'}},Row:{children:[],itemMargin:4,styles:{width:100,height:50,alignItems:'center'}},Column:{children:[],itemMargin:4,styles:{width:100,height:60}},Stack:{children:[],styles:{width:80,height:80,alignContent:'center'}}};const c={id,component:type,...defs[type]};let parent=state.map.get(state.selectedId);if(!parent||!containerTypes.has(parent.component))parent=findParent(state.selectedId)||state.map.get(state.update.updateComponents.root);mutate(()=>{state.components.push(c);parent.children=parent.children||[];parent.children.push(id)});select(id)}
  function removeSelected(){const id=state.selectedId;if(!id||id===state.update.updateComponents.root)return toast('根容器不能删除');mutate(()=>{const remove=new Set();const walk=x=>{remove.add(x);const c=state.map.get(x);(c?.children||[]).forEach(walk)};walk(id);const p=findParent(id);p.children=p.children.filter(x=>x!==id);state.components=state.update.updateComponents.components=state.components.filter(x=>!remove.has(x));state.selectedId=null});els.editor.hidden=true;els.noSel.hidden=false}
  function wrapSelected(){const id=state.selectedId;if(!id||id===state.update.updateComponents.root)return toast('请选择根容器内的元素');const p=findParent(id),wid=uniqueId('container');const wrapper={id:wid,component:'Column',children:[id],styles:{width:state.map.get(id).styles?.width||100,height:state.map.get(id).styles?.height||60,padding:4,borderRadius:8}};mutate(()=>{const i=p.children.indexOf(id);p.children[i]=wid;state.components.push(wrapper)});select(wid)}
  function duplicate(){const id=state.selectedId,p=findParent(id);if(!id||!p)return toast('根容器不能复制');const copies=[];const cloneTree=old=>{const src=state.map.get(old),nid=uniqueId(old+'_copy');const c=structuredClone(src);c.id=nid;copies.push(c);if(c.children)c.children=c.children.map(cloneTree);return nid};const rootCopy=cloneTree(id);mutate(()=>{state.components.push(...copies);p.children.splice(p.children.indexOf(id)+1,0,rootCopy)});select(rootCopy)}
  function save(){if(!state.update)return toast('没有可保存的 DSL');syncSource();const blob=new Blob([els.input.value+'\n'],{type:'application/x-ndjson;charset=utf-8'}),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=state.fileName;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);toast('DSL 已保存到下载目录')}
  function toast(msg){const t=$('#toast');t.textContent=msg;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),1800)}

  $('#renderBtn').onclick=renderInput;$('#formatBtn').onclick=()=>{try{const p=parseJsonl(els.input.value);els.input.value=p.messages.map(x=>JSON.stringify(x)).join('\n');setStatus('格式正确','ok')}catch(e){fail(e)}};$('#openBtn').onclick=()=>$('#fileInput').click();$('#fileInput').onchange=async e=>{const f=e.target.files[0];if(!f)return;state.fileName=f.name;els.input.value=await f.text();renderInput();e.target.value=''};$('#saveBtn').onclick=save;$('#zoomIn').onclick=()=>{state.scale+=.25;updateScale()};$('#zoomOut').onclick=()=>{state.scale-=.25;updateScale()};$('#deleteBtn').onclick=removeSelected;$('#wrapBtn').onclick=wrapSelected;$('#duplicateBtn').onclick=duplicate;document.querySelectorAll('[data-add]').forEach(b=>b.onclick=()=>addComponent(b.dataset.add));$('#applyJsonBtn').onclick=()=>{try{const next=JSON.parse($('#componentJson').value),idx=state.components.findIndex(x=>x.id===state.selectedId);if(!next.id||!next.component)throw Error('组件必须包含 id 和 component');mutate(()=>state.components[idx]=next);state.selectedId=next.id;select(next.id)}catch(e){toast('JSON 无效：'+e.message)}};
  els.stage.addEventListener('click',e=>{if(e.target===els.stage||e.target===$('#canvasWrap'))deselect()});
  $('#undoBtn').onclick=()=>{if(!state.history.length)return;state.future.push(JSON.stringify(state.messages));restore(state.history.pop())};$('#redoBtn').onclick=()=>{if(!state.future.length)return;state.history.push(JSON.stringify(state.messages));restore(state.future.pop())};
  document.addEventListener('keydown',e=>{if(['INPUT','TEXTAREA','SELECT'].includes(e.target.tagName))return;if(e.key==='Delete')removeSelected();if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='s'){e.preventDefault();save()}if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='z'){e.preventDefault();e.shiftKey?$('#redoBtn').click():$('#undoBtn').click()}});
  function renderLayoutTreeFixed(){
    const root=$('#layoutTree');
    if(!root)return;
    root.innerHTML='';
    if(!state.update){root.innerHTML='<div class="tree-empty">渲染 DSL 后显示组件布局。</div>';return}
    const rootId=state.update.updateComponents.root;
    const reachable=new Set(),visiting=new Set(),cycleIds=new Set();
    const walkReferences=id=>{
      if(visiting.has(id)){cycleIds.add(id);return}
      if(reachable.has(id))return;
      reachable.add(id);
      const component=state.map.get(id);
      if(!component)return;
      visiting.add(id);
      (Array.isArray(component.children)?component.children:[]).forEach(walkReferences);
      visiting.delete(id);
    };
    walkReferences(rootId);
    const icons={Text:'T',Button:'B',Image:'▧',Progress:'%',Divider:'—',Row:'↔',Column:'↕',Stack:'▣',List:'☷'};
    const rendered=new Set();
    const addRow=(id,depth,orphan=false,path=new Set())=>{
      const component=state.map.get(id),row=document.createElement('button');
      row.type='button';row.className='tree-row'+(id===state.selectedId?' active':'')+(orphan?' tree-orphan':'');row.style.paddingLeft=(6+depth*14)+'px';
      if(!component){row.innerHTML=`<span class="tree-expander"></span><span class="tree-icon">!</span><span class="tree-label">缺失组件</span><span class="tree-id">${id}</span><span class="tree-warning" title="引用的组件不存在">⚠</span>`;root.appendChild(row);return}
      const children=Array.isArray(component.children)?component.children:[],expandable=children.length>0,expanded=treeExpanded.has(id)||id===rootId,isCycle=path.has(id)||cycleIds.has(id);
      row.innerHTML=`<span class="tree-expander">${expandable&&!isCycle?(expanded?'⌄':'›'):''}</span><span class="tree-icon">${icons[component.component]||'□'}</span><span class="tree-label">${component.component}</span><span class="tree-id">${component.id}</span>${orphan?'<span class="tree-warning" title="未挂载组件">⚠</span>':isCycle?'<span class="tree-warning" title="循环引用">↻</span>':''}`;
      row.onclick=e=>{if(expandable&&!isCycle&&e.target.classList.contains('tree-expander')){treeExpanded.has(id)?treeExpanded.delete(id):treeExpanded.add(id);renderLayoutTreeFixed();return}select(id);requestAnimationFrame(()=>document.querySelector(`.dsl-node[data-id="${CSS.escape(id)}"]`)?.scrollIntoView({block:'center',inline:'center',behavior:'smooth'}))};
      root.appendChild(row);rendered.add(id);
      if(expanded&&!isCycle){const nextPath=new Set(path);nextPath.add(id);children.forEach(child=>addRow(child,depth+1,orphan,nextPath))}
    };
    addRow(rootId,0);
    const unreachable=state.components.filter(component=>!reachable.has(component.id));
    const unreachableIds=new Set(unreachable.map(component=>component.id));
    const referencedByUnreachable=new Set();
    unreachable.forEach(component=>(Array.isArray(component.children)?component.children:[]).forEach(id=>{if(unreachableIds.has(id))referencedByUnreachable.add(id)}));
    let orphanRoots=unreachable.filter(component=>!referencedByUnreachable.has(component.id));
    if(!orphanRoots.length&&unreachable.length)orphanRoots=[unreachable[0]];
    orphanRoots.forEach(component=>addRow(component.id,0,true));
    unreachable.filter(component=>!rendered.has(component.id)&&!referencedByUnreachable.has(component.id)).forEach(component=>addRow(component.id,0,true));
  }
  renderLayoutTree=renderLayoutTreeFixed;
  const baseRenderAll=renderAll,baseSelect=select,baseDeselect=deselect;
  renderAll=function(){baseRenderAll();renderLayoutTree();updateAddControls()};
  select=function(id,rerender=true){baseSelect(id,rerender);renderLayoutTree();updateAddControls()};
  deselect=function(){baseDeselect();renderLayoutTree();updateAddControls()};
  document.querySelectorAll('[data-add]').forEach(button=>button.onclick=()=>{const type=button.dataset.add;if(type==='Image')showAssetDialog();else addComponentSafely(type)});
  document.querySelectorAll('[data-source-tab]').forEach(tab=>tab.onclick=()=>{const name=tab.dataset.sourceTab;document.querySelectorAll('[data-source-panel]').forEach(panel=>{panel.hidden=panel.dataset.sourcePanel!==name});document.querySelectorAll('[data-source-tab]').forEach(x=>{const active=x===tab;x.classList.toggle('active',active);x.setAttribute('aria-selected',String(active))});if(name==='layout')renderLayoutTree();if(name==='taskspec'||name==='alt')initAltTab()});
  $('#closeAssetDialog').onclick=closeAssetDialog;$('#cancelAssetDialog').onclick=closeAssetDialog;$('#assetDialog').onclick=e=>{if(e.target===$('#assetDialog'))closeAssetDialog()};$('#insertAssetBtn').onclick=()=>{if(!pendingAssetSrc)return;const src=pendingAssetSrc;closeAssetDialog();addComponentSafely('Image',src)};
  updateAddControls();renderLayoutTree();

  // ---- ALT 生成页签 ----
  const ALT_THEMES_URL='scripts/config/alt-themes.json';
  let altThemesReady=null;
  function initAltTab(){
    if(!altThemesReady){
      altThemesReady=fetch(ALT_THEMES_URL).then(r=>{if(!r.ok)throw Error('主题配置加载失败');return r.json()}).then(cfg=>{
        if(!cfg||!cfg.themes||typeof cfg.themes!=='object')throw Error('主题配置格式错误');
        const select=$('#altTheme');
        select.innerHTML='<option value="">跟随 ALT（默认）</option>'+Object.keys(cfg.themes).map(t=>`<option value="${t}">${t}</option>`).join('');
        select.value='';
      }).catch(e=>{altThemesReady=null;throw e});
    }
    return altThemesReady;
  }
  function syncSizePlaceholders(){
    let size=null;
    try{size=JSON.parse($('#altTaskSpec').value||'{}').size}catch(e){/* keep current */ }
    const d=size==='2x4'?[300,140]:[140,140];
    $('#altWidth').placeholder=d[0];
    $('#altHeight').placeholder=d[1];
  }
  $('#altTaskSpec').addEventListener('input',syncSizePlaceholders);
  syncSizePlaceholders();

  function fmtBytes(n){
    if(n<1024)return n+' B';
    if(n<1048576)return (n/1024).toFixed(1)+' KB';
    return (n/1048576).toFixed(1)+' MB';
  }
  function escapeHtml(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}

  let pyodidePromise=null;
  let pyodideLoading=false;
  const ALT_CORE_FILES=['pyodide.asm.wasm','python_stdlib.zip','pyodide.asm.js','pyodide.js','pyodide-lock.json'];
  const PYODIDE_BASES=[
    'https://raw.githubusercontent.com/IamJohnRain/a2ui-pyodide/master/',
    'https://iamjohnrain.github.io/a2ui-pyodide/'
  ];
  let altProgress={loaded:0,total:0,unknownTotal:false,fixedTotal:false,current:''};
  let downloadProgressInstalled=false;
  let progressRaf=0;

  function installDownloadProgress(){
    if(downloadProgressInstalled)return;
    downloadProgressInstalled=true;
    const nativeFetch=window.fetch.bind(window);
    window.fetch=async(input,init)=>{
      const response=await nativeFetch(input,init);
      const url=typeof input==='string'?input:(input&&input.url)||'';
      if(!response.ok||!response.body||!ALT_CORE_FILES.some(f=>url.includes(f)))return response;
      const total=Number(response.headers.get('content-length'))||0;
      if(!altProgress.fixedTotal)altProgress.total+=total;
      if(!total&&!altProgress.fixedTotal)altProgress.unknownTotal=true;
      altProgress.current=url.split('/').pop()||'';
      const reader=response.body.getReader();
      const stream=new ReadableStream({
        start(controller){
          (function pump(){
            reader.read().then(({done,value})=>{
              if(done){
                scheduleAltProgress(true);
                controller.close();
                return;
              }
              altProgress.loaded+=value.byteLength;
              scheduleAltProgress(false);
              controller.enqueue(value);
              pump();
            }).catch(err=>controller.error(err));
          })();
        }
      });
      return new Response(stream,response);
    };
  }

  function scheduleAltProgress(force){
    if(force){
      if(progressRaf){cancelAnimationFrame(progressRaf);progressRaf=0}
      updateAltProgress();
      return;
    }
    if(progressRaf)return;
    progressRaf=requestAnimationFrame(()=>{progressRaf=0;updateAltProgress()});
  }

  function updateAltProgress(){
    const bar=$('#altLoadingBar'),text=$('#altLoadingText'),file=$('#altLoadingFile');
    const known=!altProgress.unknownTotal&&altProgress.total>0;
    if(bar)bar.classList.toggle('indeterminate',!known);
    if(file){
      file.hidden=!altProgress.current;
      file.textContent=altProgress.current?`正在下载 ${altProgress.current}`:'';
    }
    if(known){
      const percent=Math.min(100,Math.round(altProgress.loaded/altProgress.total*100));
      if(bar)bar.style.width=percent+'%';
      if(text)text.textContent=`${percent}%（${fmtBytes(altProgress.loaded)} / ${fmtBytes(altProgress.total)}）`;
    }else{
      if(bar)bar.style.width='';
      if(text)text.textContent=`已下载 ${fmtBytes(altProgress.loaded)}`;
    }
  }

  async function preloadAltTotal(base){
    try{
      const response=await fetch(base+'pyodide-lock.json',{cache:'no-cache'});
      if(!response.ok)return null;
      const lock=await response.json();
      const wanted=new Set(ALT_CORE_FILES);
      let total=0,count=0;
      for(const [name,meta] of Object.entries(lock.packages||{})){
        if(wanted.has(name)&&meta&&typeof meta.size==='number'){total+=meta.size;count++}
      }
      return count>=4?total:null;
    }catch(e){return null}
  }

  function loadPyodideLoader(){
    return new Promise((resolve,reject)=>{
      if(window.loadPyodide)return resolve();
      const tryBase=index=>{
        if(index>=PYODIDE_BASES.length){reject(new Error('Pyodide loader 下载失败'));return}
        fetch(PYODIDE_BASES[index]+'pyodide.js')
          .then(response=>{
            if(!response.ok)throw Error('HTTP '+response.status);
            return response.blob();
          })
          .then(blob=>{
            const url=URL.createObjectURL(blob);
            const script=document.createElement('script');
            script.src=url;
            script.onload=()=>{URL.revokeObjectURL(url);resolve()};
            script.onerror=()=>{URL.revokeObjectURL(url);script.remove();tryBase(index+1)};
            document.head.appendChild(script);
          })
          .catch(()=>tryBase(index+1));
      };
      tryBase(0);
    });
  }

  async function ensurePyodide(){
    if(!pyodidePromise){
      pyodideLoading=true;
      showAltLoading();
      pyodidePromise=(async()=>{
        installDownloadProgress();
        await loadPyodideLoader();
        const preloaded=await preloadAltTotal(PYODIDE_BASES[0]);
        if(preloaded){altProgress.total=preloaded;altProgress.fixedTotal=true;scheduleAltProgress(true)}
        let lastError=null;
        for(const base of PYODIDE_BASES){
          try{
            const py=await loadPyodide({indexURL:base});
            let converterSrc=await (await fetch('scripts/alt_to_dsl_converter.py')).text();
            converterSrc=converterSrc.replace(/\r?\nif __name__ == "__main__":\s*raise SystemExit\(main\(\)\)\s*$/,'');
            const cfg1=await (await fetch('scripts/config/alt-themes.json')).text();
            const cfg2=await (await fetch('scripts/config/alt-layout-profile.json')).text();
            const cfg3=await (await fetch('scripts/config/alt-tuning.json')).text();
            await py.runPythonAsync(converterSrc);
            await py.runPythonAsync(`import json; configure_runtime(${JSON.stringify(cfg1)},${JSON.stringify(cfg2)},${JSON.stringify(cfg3)})`);
            const builderSrc=await (await fetch('scripts/taskspec_to_alt_chat_completions.py')).text();
            const builderCode=prepareAltPromptBuilder(builderSrc);
            py.globals.set('_CFG_THEMES',cfg1);
            py.globals.set('_CFG_LAYOUT',cfg2);
            py.globals.set('_CFG_TUNING',cfg3);
            await py.runPythonAsync(builderCode);
            return py;
          }catch(e){lastError=e;}
        }
        throw lastError||new Error('Pyodide 加载失败');
      })().catch(err=>{pyodidePromise=null;throw err}).finally(()=>{pyodideLoading=false;hideAltLoading()});
    }
    return pyodidePromise;
  }

  async function compileAlt(taskSpecText,altText,ascText,theme,width,height){
    const py=await ensurePyodide();
    py.globals.set('__t',taskSpecText);
    py.globals.set('__a',altText);
    py.globals.set('__s',ascText);
    py.globals.set('__theme',theme===''?null:theme);
    py.globals.set('__w',width===''?null:Number(width));
    py.globals.set('__h',height===''?null:Number(height));
    const result=await py.runPythonAsync('browser_convert(__t,__a,__s,__theme,__w,__h)');
    return JSON.parse(result);
  }

  function showAltLoading(){
    $('#altLoading').hidden=false;
    $('#altLoadingError').hidden=true;
    $('#altLoadingRetry').hidden=true;
    altProgress={loaded:0,total:0,unknownTotal:false,fixedTotal:false,current:''};
    updateAltProgress();
  }
  function hideAltLoading(){
    const overlay=$('#altLoading');
    if(overlay)overlay.hidden=true;
    const bar=$('#altLoadingBar');
    if(bar)bar.classList.remove('indeterminate');
  }
  function showAltReport(title,lines,kind='error'){pushRenderWarnings(title,lines,kind)}

  async function compileAltAndRender(){
    try{
      await initAltTab();
      clearRenderWarnings();
      const result=await compileAlt(
        $('#altTaskSpec').value,$('#altInput').value,$('#ascInput').value,
        $('#altTheme').value,$('#altWidth').value,$('#altHeight').value
      );
      if(result.errors&&result.errors.length){
        showAltReport('编译失败（hard error）',result.errors);
        return;
      }
      if(result.warnings&&result.warnings.length){
        showAltReport('已编译，含警告',result.warnings,'warn');
      }
      if(result.dsl){
        els.input.value=result.dsl;
        renderInput();
      }
    }catch(e){
      if(pyodideLoading){
        $('#altLoadingError').textContent='下载失败：'+(e.message||e);
        $('#altLoadingError').hidden=false;
        $('#altLoadingRetry').hidden=false;
        $('#altLoadingBar').classList.add('indeterminate');
      }else{
        showAltReport('编译异常',[e.message||String(e)]);
      }
    }
  }
  $('#altCompileBtn').onclick=compileAltAndRender;
  $('#altLoadingRetry').onclick=()=>{pyodidePromise=null;compileAltAndRender()};

  // ---- 渲染与编译告警（画布下方，可折叠） ----
  const RENDER_WARNINGS_KEY='a2ui.renderWarnings.collapsed';
  let renderWarningItems=[];
  function updateRenderWarnings(){
    const region=$('#renderWarnings'),body=$('#renderWarningsBody'),badge=$('#renderWarningsBadge'),caret=$('#renderWarningsCaret'),toggle=$('#renderWarningsToggle');
    if(!region)return;
    if(!renderWarningItems.length){region.hidden=true;return}
    region.hidden=false;
    badge.hidden=renderWarningItems.length<2;
    badge.textContent=renderWarningItems.length;
    const collapsed=localStorage.getItem(RENDER_WARNINGS_KEY)==='1';
    body.hidden=collapsed;
    caret.textContent=collapsed?'›':'⌄';
    toggle.setAttribute('aria-expanded',String(!collapsed));
    body.innerHTML=renderWarningItems.map(item=>`<div class="render-warning${item.kind==='error'?' is-error':''}"><strong>${escapeHtml(item.title)}</strong>${item.items.length?`<ul>${item.items.map(x=>`<li>${escapeHtml(x)}</li>`).join('')}</ul>`:''}</div>`).join('');
  }
  function pushRenderWarnings(title,items,kind='warn'){
    renderWarningItems.push({title,items:Array.isArray(items)?items:[items],kind});
    if(renderWarningItems.length>50)renderWarningItems.shift();
    updateRenderWarnings();
  }
  function clearRenderWarnings(){renderWarningItems=[];updateRenderWarnings()}
  $('#renderWarningsToggle').onclick=()=>{
    const body=$('#renderWarningsBody');
    localStorage.setItem(RENDER_WARNINGS_KEY,body.hidden?'0':'1');
    updateRenderWarnings();
  };
  $('#renderBtn').onclick=()=>{clearRenderWarnings();renderInput()};

  // ---- 大模型配置（BYOK + WebCrypto 加密存储） ----
  const LLM_STORAGE_KEY='a2ui.llm.v1';
  const LLM_ITERATIONS=310000;
  let llmConfig=null;
  let llmPlainKey=null;
  let llmUnlockPromise=null;

  function bufToBase64(buf){
    const bytes=new Uint8Array(buf);
    let binary='';
    for(let i=0;i<bytes.length;i+=0x8000){
      binary+=String.fromCharCode.apply(null,bytes.subarray(i,i+0x8000));
    }
    return btoa(binary);
  }
  function base64ToBuf(b64){
    const binary=atob(b64);
    const bytes=new Uint8Array(binary.length);
    for(let i=0;i<binary.length;i++)bytes[i]=binary.charCodeAt(i);
    return bytes.buffer;
  }
  function requireWebCrypto(){
    if(!window.crypto||!window.crypto.subtle)throw Error('当前浏览器不支持 WebCrypto，请使用 HTTPS 或 localhost 环境');
  }
  async function deriveLlmKey(password,saltBuf){
    requireWebCrypto();
    const material=await crypto.subtle.importKey('raw',new TextEncoder().encode(password),'PBKDF2',false,['deriveKey']);
    return crypto.subtle.deriveKey({name:'PBKDF2',salt:saltBuf,iterations:LLM_ITERATIONS,hash:'SHA-256'},material,{name:'AES-GCM',length:256},false,['encrypt','decrypt']);
  }
  async function encryptApiKey(apiKey,password){
    requireWebCrypto();
    const salt=crypto.getRandomValues(new Uint8Array(16));
    const iv=crypto.getRandomValues(new Uint8Array(12));
    const aesKey=await deriveLlmKey(password,salt);
    const data=await crypto.subtle.encrypt({name:'AES-GCM',iv},aesKey,new TextEncoder().encode(apiKey));
    return {kdf:{algo:'PBKDF2',hash:'SHA-256',iterations:LLM_ITERATIONS,salt:bufToBase64(salt)},aead:{algo:'AES-GCM',iv:bufToBase64(iv),data:bufToBase64(data)}};
  }
  async function decryptApiKey(password){
    requireWebCrypto();
    if(!llmConfig)throw Error('尚未保存大模型配置');
    const salt=base64ToBuf(llmConfig.kdf.salt),iv=base64ToBuf(llmConfig.aead.iv);
    const aesKey=await deriveLlmKey(password,salt);
    try{
      const plain=await crypto.subtle.decrypt({name:'AES-GCM',iv},aesKey,base64ToBuf(llmConfig.aead.data));
      return new TextDecoder().decode(plain);
    }catch(e){throw Error('主口令不正确，无法解密 API Key')}
  }
  function loadLlmConfig(){
    if(llmConfig)return llmConfig;
    try{
      const raw=localStorage.getItem(LLM_STORAGE_KEY);
      if(!raw)return null;
      const cfg=JSON.parse(raw);
      if(!cfg||typeof cfg!=='object'||!cfg.baseURL||!cfg.model||!cfg.kdf||!cfg.aead)throw Error('配置格式损坏');
      llmConfig=cfg;
      return cfg;
    }catch(e){llmConfig=null;localStorage.removeItem(LLM_STORAGE_KEY);return null}
  }
  async function saveLlmConfig(baseURL,model,apiKey,password){
    const enc=await encryptApiKey(apiKey,password);
    const cfg={v:1,baseURL,model,kdf:enc.kdf,aead:enc.aead};
    llmConfig=cfg;
    localStorage.setItem(LLM_STORAGE_KEY,JSON.stringify(cfg));
    llmPlainKey=apiKey;
    return cfg;
  }
  function clearLlmConfig(){llmConfig=null;llmPlainKey=null;llmUnlockPromise=null;localStorage.removeItem(LLM_STORAGE_KEY)}

  const LLM_VENDOR_PRESETS=[
    {label:'DeepSeek',baseURL:'https://api.deepseek.com/v1',model:'deepseek-chat'},
    {label:'OpenAI',baseURL:'https://api.openai.com/v1',model:'gpt-4o-mini'},
    {label:'Moonshot Kimi',baseURL:'https://api.moonshot.cn/v1',model:'moonshot-v1-8k'},
    {label:'智谱 GLM',baseURL:'https://open.bigmodel.cn/api/paas/v4',model:'glm-4-flash'},
    {label:'通义 DashScope',baseURL:'https://dashscope.aliyuncs.com/compatible-mode/v1',model:'qwen-plus'},
    {label:'SiliconFlow',baseURL:'https://api.siliconflow.cn/v1',model:'deepseek-ai/DeepSeek-V3'}
  ];
  const presetSelect=$('#llmVendorPreset');
  presetSelect.innerHTML='<option value="">自定义（默认）</option>'+LLM_VENDOR_PRESETS.map(p=>`<option value="${p.label}">${p.label}</option>`).join('');
  presetSelect.onchange=()=>{
    const preset=LLM_VENDOR_PRESETS.find(p=>p.label===presetSelect.value);
    if(preset){$('#llmBaseURL').value=preset.baseURL;$('#llmModel').value=preset.model}
  };

  function ensureLlmUnlocked(){
    const cfg=loadLlmConfig();
    if(!cfg)return Promise.reject(Error('请先点击右上角 ⚙ 配置大模型'));
    if(llmPlainKey)return Promise.resolve(llmPlainKey);
    if(llmUnlockPromise)return llmUnlockPromise;
    const dialog=$('#llmUnlockDialog'),input=$('#llmUnlockPassword'),errorEl=$('#llmUnlockError');
    llmUnlockPromise=new Promise((resolve,reject)=>{
      errorEl.hidden=true;
      input.value='';
      dialog.hidden=false;
      setTimeout(()=>input.focus(),30);
      const close=result=>{
        dialog.hidden=true;
        llmUnlockPromise=null;
        if(result.ok)resolve(result.key);
        else reject(result.error||Error('已取消解锁'));
      };
      $('#llmUnlockConfirm').onclick=async()=>{
        try{
          const key=await decryptApiKey(input.value);
          llmPlainKey=key;
          close({ok:true,key});
        }catch(e){errorEl.textContent=e.message;errorEl.hidden=false;input.select()}
      };
      const cancel=()=>close({ok:false,error:Error('已取消解锁')});
      $('#llmUnlockCancel').onclick=cancel;
      $('#llmUnlockClose').onclick=cancel;
      dialog.onclick=e=>{if(e.target===dialog)cancel()};
      input.onkeydown=e=>{if(e.key==='Enter')$('#llmUnlockConfirm').click();if(e.key==='Escape')cancel()};
    });
    return llmUnlockPromise;
  }

  async function chatCompletion(messages,{timeoutMs=90000}={}){
    const cfg=loadLlmConfig();
    if(!cfg)throw Error('请先点击右上角 ⚙ 配置大模型');
    const key=await ensureLlmUnlocked();
    let base=cfg.baseURL.trim().replace(/\/+$/,'');
    if(!/\/chat\/completions$/i.test(base))base+='/chat/completions';
    const controller=new AbortController();
    const timer=setTimeout(()=>controller.abort(),timeoutMs);
    const body={model:cfg.model,messages,temperature:0.3};
    for(let attempt=0;attempt<2;attempt++){
      try{
        const response=await fetch(base,{method:'POST',headers:{'Content-Type':'application/json','Authorization':'Bearer '+key},body:JSON.stringify(body),signal:controller.signal});
        if(response.status===429&&attempt===0){await new Promise(r=>setTimeout(r,1800));continue}
        if(!response.ok){
          let detail='';
          try{const data=await response.json();detail=data&&data.error&&data.error.message?data.error.message:''}catch(e){detail=''}
          if(!detail){try{detail=(await response.text()).slice(0,300)}catch(e){detail=''}}
          const hint={400:'请求参数错误',401:'API Key 无效或未授权',403:'无权限访问',404:'BaseURL 或接口路径错误',429:'请求过于频繁（限流）'}[response.status]||`请求失败（HTTP ${response.status}）`;
          throw Error(`${hint}${detail?`：${detail}`:''}`);
        }
        const data=await response.json();
        const text=data&&data.choices&&data.choices[0]&&data.choices[0].message?data.choices[0].message.content:null;
        if(typeof text!=='string'||!text.trim())throw Error('模型返回内容为空');
        return text;
      }catch(e){
        if(e.name==='AbortError')throw Error(`请求超时（超过 ${Math.round(timeoutMs/1000)} 秒），请检查网络或模型响应速度`);
        if(e instanceof TypeError)throw Error('无法连接模型服务：请检查 BaseURL、网络连接，以及服务是否允许浏览器直连（CORS）');
        throw e;
      }
    }
    throw Error('请求失败');
  }

  function estimateTokens(text){
    const s=String(text||'');
    const cjk=(s.match(/[\u3400-\u9fff\u3040-\u30ff\uac00-\ud7af\u3000-\u303f\uff00-\uffef]/g)||[]).length;
    return Math.max(1,Math.round(cjk+(s.length-cjk)/4));
  }
  function estimateMessagesTokens(messages){
    return messages.reduce((sum,message)=>sum+estimateTokens((message.role||'')+'\n'+(message.content||'')),0);
  }

  async function chatCompletionStream(messages,{timeoutMs=180000,onChunk,onReasoning,onUsage}={}){
    const cfg=loadLlmConfig();
    if(!cfg)throw Error('请先点击右上角 ⚙ 配置大模型');
    const key=await ensureLlmUnlocked();
    let base=cfg.baseURL.trim().replace(/\/+$/,'');
    if(!/\/chat\/completions$/i.test(base))base+='/chat/completions';
    const controller=new AbortController();
    const timer=setTimeout(()=>controller.abort(),timeoutMs);
    const body={model:cfg.model,messages,temperature:0.3,stream:true};
    let fullText='';
    let fullReasoning='';
    try{
      const response=await fetch(base,{method:'POST',headers:{'Content-Type':'application/json','Authorization':'Bearer '+key},body:JSON.stringify(body),signal:controller.signal});
      if(response.status===429){
        clearTimeout(timer);
        await new Promise(resolve=>setTimeout(resolve,1800));
        return chatCompletionStream(messages,{timeoutMs,onChunk,onReasoning,onUsage});
      }
      if(!response.ok){
        let detail='';
        try{const data=await response.json();detail=data&&data.error&&data.error.message?data.error.message:''}catch(e){detail=''}
        if(!detail){try{detail=(await response.text()).slice(0,300)}catch(e){detail=''}}
        const hint={400:'请求参数错误',401:'API Key 无效或未授权',403:'无权限访问',404:'BaseURL 或接口路径错误',429:'请求过于频繁（限流）'}[response.status]||`请求失败（HTTP ${response.status}）`;
        throw Error(`${hint}${detail?`：${detail}`:''}`);
      }
      if(!response.body)throw Error('当前浏览器不支持流式响应');
      const reader=response.body.getReader();
      const decoder=new TextDecoder();
      let buffer='';
      const handleLine=line=>{
        if(!line.startsWith('data:'))return;
        const payload=line.slice(5).trim();
        if(!payload||payload==='[DONE]')return;
        let data;
        try{data=JSON.parse(payload)}catch(e){return}
        if(data.error)throw Error(data.error.message||'模型流式返回错误');
        if(data.usage&&onUsage)onUsage(data.usage);
        const delta=data.choices&&data.choices[0]&&data.choices[0].delta;
        if(!delta)return;
        if(typeof delta.content==='string'&&delta.content){
          fullText+=delta.content;
          if(onChunk)onChunk(delta.content);
        }
        const reasoning=delta.reasoning_content||delta.reasoning;
        if(typeof reasoning==='string'&&reasoning){
          fullReasoning+=reasoning;
          if(onReasoning)onReasoning(reasoning);
        }
      };
      while(true){
        const {done,value}=await reader.read();
        if(done)break;
        buffer+=decoder.decode(value,{stream:true});
        let separator=buffer.indexOf('\n');
        while(separator>=0){
          const line=buffer.slice(0,separator).trim();
          buffer=buffer.slice(separator+1);
          if(line)handleLine(line);
          separator=buffer.indexOf('\n');
        }
      }
      if(buffer.trim())handleLine(buffer.trim());
      if(!fullText&&!fullReasoning)throw Error('模型返回内容为空');
      return fullText;
    }catch(e){
      if(e.name==='AbortError')throw Error(`请求超时（超过 ${Math.round(timeoutMs/1000)} 秒），请检查网络或模型响应速度`);
      if(e instanceof TypeError)throw Error('无法连接模型服务：请检查 BaseURL、网络连接，以及服务是否允许浏览器直连（CORS）');
      throw e;
    }finally{clearTimeout(timer)}
  }

  // ---- 提示词模板加载 ----
  const ALT_PROMPT_BASE='scripts/alt-prompts/';
  let altPromptCache={};
  async function fetchPromptFile(name){
    const url=ALT_PROMPT_BASE+name;
    const r=await fetch(url,{cache:'no-cache'});
    if(!r.ok)throw Error(`模板加载失败：${url}（HTTP ${r.status}）`);
    return r.text();
  }
  async function loadPromptTemplate(name){
    if(altPromptCache[name])return altPromptCache[name];
    const text=await fetchPromptFile(name);
    altPromptCache[name]=text;
    return text;
  }
  async function loadTaskSpecTemplate(){
    const template=await loadPromptTemplate('task-spec-generation.md');
    if(!template.includes('{{ASSET_LIBRARY}}')||!template.includes('{{CLICK_EVENT}}'))throw Error('模板缺少附录占位符 {{ASSET_LIBRARY}} / {{CLICK_EVENT}}');
    const [assets,events]=await Promise.all([fetchPromptFile('reference/asset-library.md'),fetchPromptFile('reference/click-event.md')]);
    return template.replace('{{ASSET_LIBRARY}}',assets).replace('{{CLICK_EVENT}}',events);
  }
  function prepareAltPromptBuilder(src){
    let code=src;
    code=code.replace('from __future__ import annotations\n','');
    code=code.replace(/from alt_converter import \([\s\S]*?\n\)\n/,'');
    code=code.replace(/\r?\nif __name__ == "__main__":\s*raise SystemExit\(main\(\)\)\s*$/,'');
    const configBlock=[
      'CONFIG_DIR = Path(__file__).resolve().parent / "config"',
      'LAYOUT_PROFILE = json.loads((CONFIG_DIR / "alt-layout-profile.json").read_text(encoding="utf-8"))',
      'TUNING = json.loads((CONFIG_DIR / "alt-tuning.json").read_text(encoding="utf-8"))',
      'THEME_CONFIG = json.loads((CONFIG_DIR / "alt-themes.json").read_text(encoding="utf-8"))'
    ].join('\n');
    const injection=[
      'LAYOUT_PROFILE = json.loads(_CFG_LAYOUT)',
      'TUNING = json.loads(_CFG_TUNING)',
      'THEME_CONFIG = json.loads(_CFG_THEMES)'
    ].join('\n');
    if(!code.includes(configBlock))throw Error('ALT 上下文构建器配置块未找到，无法注入');
    code=code.replace(configBlock,injection);
    code+='\n\ndef build_alt_asc_messages(task_spec_text):\n    spec = json.loads(task_spec_text)\n    request = build_request(spec, str(spec.get("userQuery", "")), "")\n    return json.dumps(request["messages"][:2], ensure_ascii=False)\n';
    return code;
  }

  // ---- 输出解析与 TaskSpec 字段校验 ----
  function extractJsonBlock(text){
    const candidates=[];
    const fences=Array.from(text.matchAll(/```(?:json)?\s*([\s\S]*?)```/gi),match=>match[1]);
    fences.forEach(fence=>candidates.push(fence));
    const stripped=String(text).replace(/<think>[\s\S]*?<\/think>/gi,' ').replace(/```[\s\S]*?```/g,' ');
    const start=stripped.indexOf('{'),end=stripped.lastIndexOf('}');
    if(start>=0&&end>start)candidates.push(stripped.slice(start,end+1));
    let fallback=null;
    for(const candidate of candidates){
      try{
        const parsed=JSON.parse(candidate.trim());
        if(!parsed||typeof parsed!=='object'||Array.isArray(parsed))continue;
        fallback=parsed;
        if('userQuery' in parsed&&'size' in parsed)return parsed;
      }catch(e){/* 尝试下一个候选 */}
    }
    if(fallback)return fallback;
    throw Error('响应中未找到 JSON 对象');
  }
  function normalizeTaskSpec(spec,query){
    if(!spec||typeof spec!=='object'||Array.isArray(spec))return spec;
    const normalized={...spec};
    const inner=normalized.data;
    if(inner&&typeof inner==='object'&&!Array.isArray(inner)){
      if(!normalized.userQuery&&!normalized.size&&(inner.userQuery!==undefined||inner.size!==undefined)){
        Object.keys(inner).forEach(key=>{if(!(key in normalized))normalized[key]=inner[key]});
      }else if(!normalized.dataModelSchema){
        normalized.dataModelSchema=inner;
      }
    }
    delete normalized.data;
    if(normalized.size==null)normalized.size='2x2';
    if(!Array.isArray(normalized.assetCandidates))normalized.assetCandidates=[];
    if(!Array.isArray(normalized.eventCandidates))normalized.eventCandidates=[];
    if(typeof normalized.userQuery!=='string'||!normalized.userQuery.trim())normalized.userQuery=query||'';
    return normalized;
  }
  function extractAltAsc(text){
    const alt=text.match(/<alt>([\s\S]*?)<\/alt>/i);
    const asc=text.match(/<asc>([\s\S]*?)<\/asc>/i);
    if(!alt||!asc)throw Error('响应中未找到 <alt>...</alt> 或 <asc>...</asc> 段落');
    const clean=s=>String(s).replace(/```[A-Za-z0-9_+\-]*\s*/g,'').replace(/\s*```/g,'').trim();
    const altText=clean(alt[1]),ascText=clean(asc[1]);
    if(!altText||!ascText)throw Error('<alt> 或 <asc> 内容为空');
    return {alt:altText,asc:ascText};
  }
  const TASKSPEC_TOP_LEVEL=new Set(['userQuery','size','eventCandidates','dataModelSchema','assetCandidates']);
  const EVENT_CALLS=new Set(['clickToCallPhone','clickToDeeplink','clickToIntent']);
  const FORBIDDEN_EVENT_FIELDS=['id','label','description','required','onClick'];
  function normalizeAssetSrc(src){
    if(typeof src!=='string')return null;
    const name=src.replace(/^.*[\\/]/,'');
    if(!name.endsWith('.svg'))return null;
    return Array.isArray(window.MediaAssetCatalog)&&window.MediaAssetCatalog.includes(name)?'resources/base/media/'+name:null;
  }
  function validateSchemaNodes(node,path,root){
    const issues=[];
    if(!node||typeof node!=='object'||Array.isArray(node))return issues;
    if(!root&&'description' in node){
      if(!('type' in node))issues.push(`${path} 节点缺 type`);
      if(!('sampleValue' in node))issues.push(`${path} 节点缺 sampleValue`);
      if(typeof node.description!=='string'||!node.description.trim())issues.push(`${path}.description 不能为空`);
    }
    Object.keys(node).forEach(key=>{
      const child=node[key];
      if(child&&typeof child==='object'&&!['type','description','sampleValue','maxLength','default'].includes(key)){
        issues.push(...validateSchemaNodes(child,`${path}.${key}`,false));
      }
    });
    return issues;
  }
  function validateTaskSpec(spec){
    const issues=[];
    if(!spec||typeof spec!=='object'||Array.isArray(spec))return ['TaskSpec 必须是 JSON 对象'];
    Object.keys(spec).forEach(key=>{if(!TASKSPEC_TOP_LEVEL.has(key))issues.push(`顶层字段 ${key} 不在协议内（仅允许 userQuery/size/eventCandidates/dataModelSchema/assetCandidates）`)});
    if(!['2x2','2x4'].includes(spec.size))issues.push('size 必须是 "2x2" 或 "2x4"');
    if(typeof spec.userQuery!=='string'||!spec.userQuery.trim())issues.push('userQuery 不能为空');
    if(!spec.dataModelSchema||typeof spec.dataModelSchema!=='object'||Array.isArray(spec.dataModelSchema))issues.push('dataModelSchema 必须是对象');
    else issues.push(...validateSchemaNodes(spec.dataModelSchema,'dataModelSchema',true));
    if(!Array.isArray(spec.assetCandidates))issues.push('assetCandidates 必须是数组');
    else spec.assetCandidates.forEach((asset,index)=>{
      if(!asset||typeof asset!=='object'||Array.isArray(asset)){issues.push(`assetCandidates[${index}] 必须是对象`);return}
      const normalized=normalizeAssetSrc(asset.src);
      if(!normalized)issues.push(`assetCandidates[${index}].src 必须是素材库内声明的本地 SVG（resources/base/media/*.svg）`);
      else if(asset.src!==normalized)issues.push(`assetCandidates[${index}].src 应规范为 ${normalized}`);
      if(typeof asset.description!=='string'||!asset.description.trim())issues.push(`assetCandidates[${index}].description 不能为空`);
    });
    if(!Array.isArray(spec.eventCandidates))issues.push('eventCandidates 必须是数组');
    else spec.eventCandidates.forEach((event,index)=>{
      if(!event||typeof event!=='object'||Array.isArray(event)){issues.push(`eventCandidates[${index}] 必须是对象`);return}
      FORBIDDEN_EVENT_FIELDS.forEach(key=>{if(key in event)issues.push(`eventCandidates[${index}] 不允许出现 ${key} 字段`)});
      if(!EVENT_CALLS.has(event.call))issues.push(`eventCandidates[${index}].call 必须是 clickToCallPhone / clickToDeeplink / clickToIntent`);
      if(event.args!==undefined&&(typeof event.args!=='object'||event.args===null||Array.isArray(event.args)))issues.push(`eventCandidates[${index}].args 必须是对象`);
    });
    return issues;
  }

  // ---- 两段生成流程 ----
  let llmSession=null;
  function resetLlmSession(stage,messages,promptText){
    llmSession={
      stage,
      status:'请求中…',
      prompt:promptText,
      output:'',
      reasoning:'',
      inputTokens:estimateMessagesTokens(messages),
      outputTokens:0,
      usage:null
    };
    renderLlmSession();
    $('#llmSessionDialog').hidden=false;
  }
  function renderLlmSession(){
    if(!llmSession)return;
    const session=llmSession;
    $('#llmSessionTitle').textContent=session.stage||'大模型请求';
    $('#llmSessionSubtitle').textContent=session.status||'';
    $('#llmSessionPrompt').textContent=session.prompt||'（暂无 Prompt）';
    $('#llmSessionOutput').textContent=session.output;
    $('#llmSessionReasoning').textContent=session.reasoning;
    const statusEl=$('#llmSessionStatus');
    statusEl.textContent=session.status||'';
    statusEl.className='session-status'+(session.status==='请求中…'||session.status==='生成中…'?' is-active':(session.status&&session.status.includes('失败')?' is-error':''));
    const inputTokens=session.usage&&session.usage.prompt_tokens!=null?session.usage.prompt_tokens:session.inputTokens;
    const outputTokens=session.usage&&session.usage.completion_tokens!=null?session.usage.completion_tokens:session.outputTokens;
    $('#llmSessionTokens').textContent=`输入 ${inputTokens} · 输出 ${outputTokens}${session.usage?'':'（估算）'}`;
    const outputEl=$('#llmSessionOutput'),reasoningEl=$('#llmSessionReasoning');
    if(outputEl.scrollHeight-outputEl.scrollTop-outputEl.clientHeight<40)outputEl.scrollTop=outputEl.scrollHeight;
    if(reasoningEl.scrollHeight-reasoningEl.scrollTop-reasoningEl.clientHeight<40)reasoningEl.scrollTop=reasoningEl.scrollHeight;
  }
  function openLlmSession(){
    if(!llmSession)resetLlmSession('大模型请求',[],'（暂无请求记录，点击「生成 TaskSpec」或「生成 ALT / ASC」后自动显示）');
    renderLlmSession();
    $('#llmSessionDialog').hidden=false;
  }
  $('#llmSessionBtn').onclick=openLlmSession;
  $('#llmSessionClose').onclick=()=>{$('#llmSessionDialog').hidden=true};
  $('#llmSessionDialog').onclick=e=>{if(e.target===$('#llmSessionDialog'))$('#llmSessionDialog').hidden=true};
  $('#llmSessionCopyPrompt').onclick=()=>{
    if(!llmSession||!llmSession.prompt)return;
    navigator.clipboard.writeText(llmSession.prompt).then(()=>toast('Prompt 已复制')).catch(()=>toast('复制失败'));
  };
  $('#llmSessionTabAnswer').onclick=()=>{
    $('#llmSessionTabAnswer').classList.add('active');
    $('#llmSessionTabReasoning').classList.remove('active');
    $('#llmSessionOutput').hidden=false;
    $('#llmSessionReasoning').hidden=true;
  };
  $('#llmSessionTabReasoning').onclick=()=>{
    $('#llmSessionTabReasoning').classList.add('active');
    $('#llmSessionTabAnswer').classList.remove('active');
    $('#llmSessionReasoning').hidden=false;
    $('#llmSessionOutput').hidden=true;
  };

  function showGenError(box,title,items){
    box.hidden=false;
    box.innerHTML=`<strong>${escapeHtml(title)}</strong>${items&&items.length?`<ul>${items.map(x=>`<li>${escapeHtml(x)}</li>`).join('')}</ul>`:''}`;
  }
  async function generateTaskSpec(){
    const genError=$('#altGenError'),btn=$('#altGenTaskSpecBtn');
    genError.hidden=true;
    const query=$('#altUserQuery').value.trim();
    if(!query){showGenError(genError,'请先输入用户 Query');return}
    btn.disabled=true;
    const oldLabel=btn.textContent;
    btn.textContent='生成中…';
    try{
      const template=await loadTaskSpecTemplate();
      const system=template.replace('{userQuery}',query);
      const messages=[{role:'system',content:system},{role:'user',content:query}];
      resetLlmSession('生成 TaskSpec',messages,system+'\n\n[user]\n'+query);
      const text=await chatCompletionStream(messages,{
        timeoutMs:120000,
        onChunk:chunk=>{llmSession.output+=chunk;llmSession.outputTokens+=estimateTokens(chunk);llmSession.status='生成中…';renderLlmSession()},
        onReasoning:chunk=>{llmSession.reasoning+=chunk;llmSession.outputTokens+=estimateTokens(chunk);renderLlmSession()},
        onUsage:usage=>{llmSession.usage=usage;llmSession.outputTokens=usage.completion_tokens!=null?usage.completion_tokens:llmSession.outputTokens;renderLlmSession()}
      });
      llmSession.status='已完成';
      renderLlmSession();
      const spec=normalizeTaskSpec(extractJsonBlock(text),query);
      if(!spec||typeof spec!=='object'||Array.isArray(spec))throw Error('模型输出不是 JSON 对象');
      const issues=validateTaskSpec(spec);
      $('#altTaskSpec').value=JSON.stringify(spec,null,2);
      syncSizePlaceholders();
      if(issues.length)showGenError(genError,`TaskSpec 已生成，但有 ${issues.length} 条校验提示（可手动修改后继续）：`,issues);
      else toast('TaskSpec 已生成');
    }catch(e){
      if(llmSession){llmSession.status='失败：'+(e.message||e);renderLlmSession()}
      showGenError(genError,'生成 TaskSpec 失败：',[e.message||String(e)]);
    }finally{
      btn.disabled=false;
      btn.textContent=oldLabel;
    }
  }
  async function generateAltAsc(){
    const genError=$('#altGenError'),btn=$('#altGenAltAscBtn');
    genError.hidden=true;
    const taskSpecText=$('#altTaskSpec').value.trim();
    if(!taskSpecText){showGenError(genError,'请先填写或生成 TaskSpec');return}
    let spec;
    try{spec=JSON.parse(taskSpecText)}catch(e){showGenError(genError,'TaskSpec 不是合法 JSON：',[e.message||String(e)]);return}
    const issues=validateTaskSpec(spec);
    const blocking=issues.filter(i=>/顶层字段|size 必须|assetCandidates\[.*\]\.src/.test(i));
    if(blocking.length){showGenError(genError,'TaskSpec 校验未通过，请先修正：',blocking);return}
    btn.disabled=true;
    const oldLabel=btn.textContent;
    btn.textContent='生成中…';
    try{
      const py=await ensurePyodide();
      py.globals.set('__ts',taskSpecText);
      const messagesJson=await py.runPythonAsync('build_alt_asc_messages(__ts)');
      const messages=JSON.parse(messagesJson);
      const promptText=messages.map(message=>`[${message.role}]\n${message.content}`).join('\n\n');
      resetLlmSession('生成 ALT / ASC',messages,promptText);
      const text=await chatCompletionStream(messages,{
        timeoutMs:180000,
        onChunk:chunk=>{llmSession.output+=chunk;llmSession.outputTokens+=estimateTokens(chunk);llmSession.status='生成中…';renderLlmSession()},
        onReasoning:chunk=>{llmSession.reasoning+=chunk;llmSession.outputTokens+=estimateTokens(chunk);renderLlmSession()},
        onUsage:usage=>{llmSession.usage=usage;llmSession.outputTokens=usage.completion_tokens!=null?usage.completion_tokens:llmSession.outputTokens;renderLlmSession()}
      });
      llmSession.status='已完成';
      renderLlmSession();
      const {alt,asc}=extractAltAsc(text);
      $('#altInput').value=alt;
      $('#ascInput').value=asc;
      toast('ALT / ASC 已生成，可直接编译并渲染');
      const altTab=document.querySelector('[data-source-tab="alt"]');
      if(altTab)altTab.click();
    }catch(e){
      if(llmSession){llmSession.status='失败：'+(e.message||e);renderLlmSession()}
      showGenError(genError,'生成 ALT/ASC 失败：',[e.message||String(e),'可检查模型输出是否包含 <alt>...</alt> 与 <asc>...</asc> 后重试']);
    }finally{
      btn.disabled=false;
      btn.textContent=oldLabel;
    }
  }
  $('#altGenTaskSpecBtn').onclick=generateTaskSpec;
  $('#altGenAltAscBtn').onclick=generateAltAsc;

  // ---- 设置弹窗 ----
  function setLlmStatus(el,text,isError){
    el.textContent=text;
    el.className='llm-status'+(isError?' err':'');
    el.hidden=false;
  }
  function openLlmSettings(){
    const cfg=loadLlmConfig();
    $('#llmBaseURL').value=cfg?cfg.baseURL:'';
    $('#llmModel').value=cfg?cfg.model:'';
    $('#llmApiKey').value='';
    $('#llmMasterPassword').value='';
    $('#llmMasterPasswordConfirm').value='';
    $('#llmVendorPreset').value='';
    $('#llmSettingsStatus').hidden=true;
    $('#llmSettingsDialog').hidden=false;
  }
  $('#llmSettingsBtn').onclick=openLlmSettings;
  $('#llmCloseSettings').onclick=()=>{$('#llmSettingsDialog').hidden=true};
  $('#llmCancelSettings').onclick=()=>{$('#llmSettingsDialog').hidden=true};
  $('#llmSettingsDialog').onclick=e=>{if(e.target===$('#llmSettingsDialog'))$('#llmSettingsDialog').hidden=true};
  $('#llmSaveConfig').onclick=async()=>{
    const baseURL=$('#llmBaseURL').value.trim(),model=$('#llmModel').value.trim(),apiKey=$('#llmApiKey').value,pwd=$('#llmMasterPassword').value,confirmPwd=$('#llmMasterPasswordConfirm').value;
    const status=$('#llmSettingsStatus');
    status.hidden=true;
    if(!baseURL)return setLlmStatus(status,'BaseURL 不能为空',true);
    if(!model)return setLlmStatus(status,'模型名不能为空',true);
    if(!apiKey)return setLlmStatus(status,'API Key 不能为空',true);
    if(!pwd)return setLlmStatus(status,'主口令不能为空',true);
    if(pwd.length<4)return setLlmStatus(status,'主口令至少 4 个字符',true);
    if(pwd!==confirmPwd)return setLlmStatus(status,'两次输入的口令不一致',true);
    try{
      await saveLlmConfig(baseURL,model,apiKey,pwd);
      $('#llmApiKey').value='';
      $('#llmMasterPassword').value='';
      $('#llmMasterPasswordConfirm').value='';
      setLlmStatus(status,'已保存：API Key 已用 AES-GCM + PBKDF2 加密，口令未落盘，本次会话已解锁',false);
      toast('大模型配置已保存');
    }catch(e){setLlmStatus(status,'保存失败：'+(e.message||e),true)}
  };
  $('#llmClearConfig').onclick=()=>{
    if(!confirm('确定清除已保存的大模型配置吗？加密的 API Key 将无法恢复。'))return;
    clearLlmConfig();
    $('#llmBaseURL').value='';
    $('#llmModel').value='';
    $('#llmApiKey').value='';
    $('#llmMasterPassword').value='';
    $('#llmMasterPasswordConfirm').value='';
    setLlmStatus($('#llmSettingsStatus'),'配置已清除',false);
  };
})();
