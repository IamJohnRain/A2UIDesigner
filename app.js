(() => {
  const $ = s => document.querySelector(s);
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
  function evalBinding(value,local){if(typeof value!=='string'||!/^\{\{[\s\S]*\}\}$/.test(value.trim()))return value;let exp=value.trim().slice(2,-2).trim();exp=exp.replace(/\$\{([^}]+)\}/g,(_,p)=>JSON.stringify(getPath(p,local)));try{return Function(`"use strict";return (${exp})`)()}catch{return value}}
  function cssColor(v){if(!v)return '';if(/^#[0-9a-f]{8}$/i.test(v))return '#'+v.slice(3)+v.slice(1,3);return v}
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
  function box(v){if(typeof v==='number')return `${v}px`;if(v&&typeof v==='object')return `${v.top||0}px ${v.right||0}px ${v.bottom||0}px ${v.left||0}px`;return ''}
  function gradient(g){if(!g?.colors)return '';const dirs={RightBottom:'135deg',LeftBottom:'45deg',RightTop:'225deg',LeftTop:'315deg',Right:'90deg',Bottom:'180deg'};return `linear-gradient(${dirs[g.direction]||'135deg'},${g.colors.map(x=>`${cssColor(x[0])} ${x[1]*100}%`).join(',')})`}
  function applyStyles(el,c){const s=c.styles||{}, st=el.style; if(s.width!=null)st.width=s.width+'px';if(s.height!=null)st.height=s.height+'px';if(s.padding!=null)st.padding=box(s.padding);if(s.margin!=null)st.margin=box(s.margin);if(s.borderRadius!=null)st.borderRadius=s.borderRadius+'px';if(s.clip)st.overflow='hidden';if(s.backgroundColor)st.backgroundColor=cssColor(s.backgroundColor);if(s.linearGradient)st.backgroundImage=gradient(s.linearGradient);else if(s.backgroundImage){const bg=evalBinding(s.backgroundImage);st.backgroundImage=`url("${previewAssetPath(bg)}")`;st.backgroundSize='cover';st.backgroundPosition='center'}if(s.fontSize!=null)st.fontSize=s.fontSize+'px';if(s.fontWeight!=null)st.fontWeight=s.fontWeight;if(s.fontColor)st.color=cssColor(s.fontColor);if(s.textAlign)st.textAlign=s.textAlign;if(s.opacity!=null)st.opacity=s.opacity;if(s.borderWidth)st.border=`${s.borderWidth}px solid ${cssColor(s.borderColor||'#000')}`;if(s.shadow)st.boxShadow=`${s.shadow.offsetX||0}px ${s.shadow.offsetY||0}px ${s.shadow.radius||0}px ${cssColor(s.shadow.color)}`;if(s.flexGrow!=null)st.flexGrow=s.flexGrow;if(s.flexShrink!=null)st.flexShrink=s.flexShrink;
    if(c.component==='Row'||c.component==='Column'){st.display='flex';st.flexDirection=c.component==='Row'?'row':'column';st.gap=(c.itemMargin||0)+'px';st.justifyContent=mapAlign(s.justifyContent);st.alignItems=mapAlign(s.alignItems);if(c.wrap)st.flexWrap='wrap'}
    if(c.component==='Stack'){st.position='relative';st.display='grid';st.placeItems=stackAlign(s.alignContent);[...el.children].forEach(x=>{x.style.gridArea='1 / 1'})}
  }
  function mapAlign(v){return ({start:'flex-start',end:'flex-end',center:'center',spaceBetween:'space-between',spaceAround:'space-around',spaceEvenly:'space-evenly'}[v]||v||'flex-start')}
  function stackAlign(v){return ({topStart:'start',top:'start center',topEnd:'start end',start:'center start',center:'center',end:'center end',bottomStart:'end start',bottom:'end center',bottomEnd:'end end'}[v]||'center')}
  function makeNode(id,local){const c=state.map.get(id);if(!c){const x=document.createElement('div');x.textContent=`缺失: ${id}`;return x}const el=document.createElement('div');el.className='dsl-node';el.dataset.id=c.id;el.dataset.label=`${c.component} · ${c.id}`;el.draggable=c.id!==state.update.updateComponents.root;
    if(containerTypes.has(c.component)){let ids=Array.isArray(c.children)?c.children:[];ids.forEach(cid=>el.appendChild(makeNode(cid,local)))}
    else if(c.component==='Text'){el.textContent=evalBinding(c.content,local)??'';el.style.whiteSpace='pre-wrap';el.style.display='flex';el.style.alignItems='center';if(c.styles?.textAlign==='center')el.style.justifyContent='center';if(c.styles?.textAlign==='end')el.style.justifyContent='flex-end'}
    else if(c.component==='Button'){el.textContent=evalBinding(c.label,local)??'按钮';el.style.display='grid';el.style.placeItems='center'}
    else if(c.component==='Image'){const img=document.createElement('img');const src=evalBinding(c.src,local),previewSrc=previewAssetPath(src);img.alt=c.id;img.style.width='100%';img.style.height='100%';img.style.objectFit=c.styles?.objectFit||'contain';const showPlaceholder=()=>{img.style.display='none';el.classList.add('image-placeholder');el.title=src||'';el.style.display='grid';el.style.placeItems='center';el.style.color='#8c94a5';el.style.background='repeating-linear-gradient(45deg,#eef0f4,#eef0f4 4px,#e5e8ee 4px,#e5e8ee 8px)';if(!el.querySelector('.asset-missing')){const mark=document.createElement('span');mark.className='asset-missing';mark.textContent='▧';el.appendChild(mark)}};if(previewSrc){img.src=previewSrc;img.onerror=showPlaceholder}else showPlaceholder();el.appendChild(img)}
    else if(c.component==='Progress'){const value=Number(evalBinding(c.value,local))||0,total=Number(evalBinding(c.total,local))||100,p=Math.max(0,Math.min(100,value/total*100));if(c.styles?.type==='ring'){el.style.borderRadius='50%';el.style.background=`conic-gradient(${cssColor(c.styles.color||'#5b5ce2')} ${p}%,${cssColor(c.styles.backgroundColor||'#2a000000')} 0)`;const hole=document.createElement('i');hole.style.cssText='width:72%;height:72%;border-radius:50%;background:inherit;filter:brightness(1.2)';el.style.display='grid';el.style.placeItems='center';el.appendChild(hole)}else{el.style.background=cssColor(c.styles?.backgroundColor||'#22000000');const fill=document.createElement('i');fill.style.cssText=`display:block;width:${p}%;height:100%;background:${cssColor(c.styles?.color||'#5b5ce2')};border-radius:inherit`;el.appendChild(fill)}}
    else if(c.component==='Divider'){el.style.background=cssColor(c.styles?.color||'#22000000')}
    else if(c.component==='Checkbox'){
      const selected=Boolean(evalBinding(c.select,local)),s=c.styles||{},mark=s.mark||{};
      el.setAttribute('role','checkbox');el.setAttribute('aria-checked',String(selected));el.style.display='grid';el.style.placeItems='center';el.style.boxSizing='border-box';
      el.style.borderRadius=s.shape==='circle'?'50%':`${Math.max(3,Math.round(Math.min(s.width||16,s.height||16)*.25))}px`;
      if(selected){
        el.style.backgroundColor=cssColor(s.selectedColor||'#ff0a59f7');el.style.border='none';
        const svg=document.createElementNS('http://www.w3.org/2000/svg','svg'),path=document.createElementNS('http://www.w3.org/2000/svg','path');
        const size=Number(mark.size)||8;svg.setAttribute('width',size);svg.setAttribute('height',size);svg.setAttribute('viewBox','0 0 12 10');svg.setAttribute('aria-hidden','true');
        path.setAttribute('d','M1 5 L4.3 8.3 L11 1.5');path.setAttribute('fill','none');path.setAttribute('stroke',cssColor(mark.strokeColor||'#ffffffff'));path.setAttribute('stroke-width',String(mark.strokeWidth||2));path.setAttribute('stroke-linecap','round');path.setAttribute('stroke-linejoin','round');svg.appendChild(path);el.appendChild(svg);
      }else{
        el.style.backgroundColor='transparent';el.style.border=`1px solid ${cssColor(s.unSelectedColor||'#66000000')}`;
      }
    }
    applyStyles(el,c);bindNodeEvents(el,c);return el}
  function bindNodeEvents(el,c){el.addEventListener('click',e=>{e.stopPropagation();select(c.id)});el.addEventListener('dragstart',e=>{e.stopPropagation();e.dataTransfer.setData('text/plain',c.id);setTimeout(()=>el.style.opacity='.35')});el.addEventListener('dragend',()=>el.style.opacity='');el.addEventListener('dragover',e=>{e.preventDefault();e.stopPropagation();el.classList.add('drag-over')});el.addEventListener('dragleave',()=>el.classList.remove('drag-over'));el.addEventListener('drop',e=>{e.preventDefault();e.stopPropagation();el.classList.remove('drag-over');const moving=e.dataTransfer.getData('text/plain');containerTypes.has(c.component)?moveInto(moving,c.id):moveBefore(moving,c.id)})}
  function renderAll(){reindex();const rootId=state.update.updateComponents.root;els.canvas.innerHTML='';const node=makeNode(rootId);els.canvas.appendChild(node);els.canvas.style.width=(state.create.createSurface.width||140)+'px';els.canvas.style.height=(state.create.createSurface.height||140)+'px';els.stage.hidden=false;els.empty.hidden=true;updateScale();if(state.selectedId)select(state.selectedId,false)}
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
  function applyField(key,val,type){const c=state.map.get(state.selectedId);if(!c)return;snapshot();let target=c;if(key.startsWith('styles.')){target=c.styles=c.styles||{};key=key.slice(7)}if(val==='')delete target[key];else target[key]=type==='number'?Number(val):type==='checkbox'?!!val:val;syncSource();renderAll();select(c.id)}
  function buildInspector(c){$('#selectedType').textContent=c.component;$('#selectedId').textContent=c.id;$('#selectionHint').textContent='正在编辑组件';const content=$('#contentFields'),size=$('#sizeFields'),app=$('#appearanceFields'),text=$('#textFields'),layout=$('#layoutFields');[content,size,app,text,layout].forEach(x=>x.innerHTML='');const contentKey=c.component==='Text'?'content':c.component==='Button'?'label':c.component==='Image'?'src':null;if(contentKey)content.appendChild(field(contentKey==='src'?'图片路径':'显示内容',contentKey,c[contentKey],'text'));['width','height'].forEach(k=>size.appendChild(field(k==='width'?'宽度':'高度','styles.'+k,c.styles?.[k])));size.appendChild(field('内边距','styles.padding',typeof c.styles?.padding==='number'?c.styles.padding:'','number'));size.appendChild(field('元素间距','itemMargin',c.itemMargin));app.appendChild(colorField('背景颜色','styles.backgroundColor',c.styles?.backgroundColor));['borderRadius','opacity'].forEach(k=>app.appendChild(field({borderRadius:'圆角',opacity:'透明度'}[k],'styles.'+k,c.styles?.[k],'number')));const isText=['Text','Button'].includes(c.component);$('#textSection').hidden=!isText;if(isText){text.appendChild(field('字号','styles.fontSize',c.styles?.fontSize));text.appendChild(field('字重','styles.fontWeight',c.styles?.fontWeight,'text'));text.appendChild(field('文字颜色','styles.fontColor',c.styles?.fontColor,'text'));text.appendChild(field('对齐','styles.textAlign',c.styles?.textAlign,'text',['','start','center','end']))}const isContainer=containerTypes.has(c.component);$('#layoutSection').hidden=!isContainer;if(isContainer){layout.appendChild(field('主轴对齐','styles.justifyContent',c.styles?.justifyContent,'text',['','start','center','end','spaceBetween','spaceAround']));layout.appendChild(field('交叉轴对齐','styles.alignItems',c.styles?.alignItems,'text',['','start','center','end','stretch']))}$('#componentJson').value=JSON.stringify(c,null,2)}
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
})();
