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
  function renderAll(){reindex();const rootId=state.update.updateComponents.root;els.canvas.innerHTML='';const node=makeNode(rootId);els.canvas.appendChild(node);els.canvas.style.width=(state.create.createSurface.width||140)+'px';els.canvas.style.height=(state.create.createSurface.height||140)+'px';els.stage.hidden=false;els.empty.hidden=true;renderer.finalize(node);updateScale();if(state.selectedId)select(state.selectedId,false)}
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
  function fail(e){els.error.textContent=e.message||e;els.error.hidden=false;els.parse.textContent='解析失败';els.parse.className='status bad'}
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
  document.querySelectorAll('[data-source-tab]').forEach(tab=>tab.onclick=()=>{const tree=tab.dataset.sourceTab==='tree';$('#sourceCodePanel').hidden=tree;$('#sourceTreePanel').hidden=!tree;document.querySelectorAll('[data-source-tab]').forEach(x=>{const active=x===tab;x.classList.toggle('active',active);x.setAttribute('aria-selected',String(active))});if(tree)renderLayoutTree()});
  $('#closeAssetDialog').onclick=closeAssetDialog;$('#cancelAssetDialog').onclick=closeAssetDialog;$('#assetDialog').onclick=e=>{if(e.target===$('#assetDialog'))closeAssetDialog()};$('#insertAssetBtn').onclick=()=>{if(!pendingAssetSrc)return;const src=pendingAssetSrc;closeAssetDialog();addComponentSafely('Image',src)};
  updateAddControls();renderLayoutTree();
})();
