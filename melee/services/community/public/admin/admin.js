const $ = id => document.getElementById(id);
let tab = "", current = null, next = null, reportId = null;
let listRequest = 0, detailRequest = 0, tabRequest = 0, saving = false;
const states = new Map();
function status(message, error=false) { $("status").textContent=message; $("status").classList.toggle("error",error); }
async function api(path, options={}) {
  const headers = {"X-Freak-Admin":"1",...options.headers};
  if (options.body) headers["Content-Type"]="application/json";
  const response=await fetch(path,{...options,headers,credentials:"same-origin"});
  if (!response.ok) {
    let message=`Request failed (${response.status}).`;
    try {message=(await response.json()).error||message;} catch {}
    throw new Error(message);
  }
  return response.json();
}
function localDate(stamp) {
  if (!stamp) return "";
  const date=new Date(stamp);
  return new Date(date.getTime()-date.getTimezoneOffset()*60000).toISOString().slice(0,16);
}
function captureForm() {
  return {current, title:$("title").value, ticker:$("ticker").value, body:$("body").value,
    published:$("published").checked, publishedAt:$("published-at").value,
    expiresAt:$("expires-at").value, newsId:$("news-link").value,
    withdrawn:$("withdrawn").checked, archived:$("archived").checked, resend:$("resend").checked};
}
function persistDraft() {
  if(!tab||tab==="feedback")return;
  const state=captureForm();states.set(tab,state);
  try{localStorage.setItem(`freak-admin-${tab}`,JSON.stringify(state));}catch{}
}
function restoreForm(value) {
  current=value.current||null;
  $("title").value=value.title||"";$("ticker").value=value.ticker||"";$("body").value=value.body||"";
  $("published").checked=!!value.published;$("published-at").value=value.publishedAt||"";
  $("expires-at").value=value.expiresAt||"";$("news-link").value=value.newsId||"";
  $("withdrawn").checked=!!value.withdrawn;$("archived").checked=!!value.archived;$("resend").checked=!!value.resend;
  $("editor-title").textContent=(current?"Edit ":"New ")+(tab==="news"?"article":"announcement");
  $("revision").textContent=current?`Revision ${current.revision}`:"Draft";
  $("preview").hidden=true;
}
function formatText(parent,body) {
  for(const block of body.split(/\n\s*\n/u)) {
    if(!block.trim())continue;
    const heading=/^(#{1,3}) (.*)$/u.exec(block);
    const lines=block.split("\n");
    const bullet=lines.every(line=>line.startsWith("- "));
    const element=document.createElement(heading?`h${heading[1].length+1}`:bullet?"ul":"p");
    if(bullet)for(const line of lines){const item=document.createElement("li");item.textContent=line.slice(2);element.append(item);}
    else element.textContent=heading?heading[2]:block;
    parent.append(element);
  }
}
function preview() {
  const pane=$("preview");pane.replaceChildren();pane.classList.toggle("notice",tab==="announcements");
  const title=document.createElement("h1");title.textContent=$("title").value||"Untitled";pane.append(title);
  formatText(pane,$("body").value);pane.hidden=false;
}
function recordButton(row) {
  const button=document.createElement("button");button.className="record";
  const title=document.createElement("strong");title.textContent=row.title||row.build||row.id;
  const caption=document.createElement("small");
  caption.textContent=tab==="feedback"?`${row.status} · ${new Date(row.created_at).toLocaleString()}`:
    `${row.archived?"Archived":row.withdrawn?"Withdrawn":row.published_at?"Published":"Draft"} · ${new Date(row.updated_at).toLocaleString()}`;
  button.append(title,caption);button.onclick=()=>safely(()=>openRecord(row.id));return button;
}
async function list(append=false) {
    const version=++listRequest, source=tab;
    const params=new URLSearchParams();if(append&&next)params.set("before",next);
    params.set("search",$("search").value);
    if(tab==="feedback")params.set("status",$("filter").value);
    const result=await api(`/admin/api/${tab}?${params}`);
    if(version!==listRequest||tab!==source)return;
    if(!append)$("records").replaceChildren();
    for(const row of result.entries)$("records").append(recordButton(row));
    next=result.next;$("older").hidden=!next;
    if(result.usage)$("usage").textContent=`${result.usage.reports} reports · ${(result.usage.bytes/1000000).toFixed(1)} MB`;
    else $("usage").textContent="";
}
async function newsLinks(version) {
  const options=[new Option("None","")];
  let cursor=null;
  // Follow server pagination so older articles remain available for linking.
  do {
    const result=await api(`/admin/api/news${cursor?`?before=${encodeURIComponent(cursor)}`:""}`);
    if(version!==tabRequest||tab!=="announcements")return;
    for(const row of result.entries)options.push(new Option(row.title,row.id));
    cursor=result.next;
  } while(cursor);
  const selected=$("news-link").value;
  $("news-link").replaceChildren(...options);
  if(selected&&!options.some(option=>option.value===selected))$("news-link").append(new Option(selected,selected));
  $("news-link").value=selected;
}
async function switchTab(name) {
  if(saving)return;
  persistDraft();tab=name;next=null;
  const version=++tabRequest;++detailRequest;
  $("records").replaceChildren();$("older").hidden=true;$("usage").textContent="";
  for(const button of document.querySelectorAll("[data-tab]"))button.classList.toggle("selected",button.dataset.tab===tab);
  const feedback=tab==="feedback",announcement=tab==="announcements";
  $("editor").hidden=feedback;$("report").hidden=true;$("new").hidden=feedback;$("filter-wrap").hidden=!feedback;
  $("ticker-wrap").hidden=announcement;$("announcement-options").hidden=!announcement;$("archive-wrap").hidden=announcement;
  $("list-title").textContent=tab==="news"?"News":announcement?"Announcements":"Feedback";
  $("search").value="";$("search").placeholder=feedback?"Find by build":"Find by title";
  $("body").maxLength=announcement?380:32000;
  $("save").textContent=announcement?"Save announcement":"Save article";
  $("editor-help").textContent=announcement?"Active announcements appear when players next reach a convenient menu. Resend creates a new delivery.":"Published articles appear in News. The newest headline appears in the ticker.";
  let saved=states.get(tab);
  if(!saved&& !feedback) {try{saved=JSON.parse(localStorage.getItem(`freak-admin-${tab}`)||"null");}catch{}}
  if(!feedback){
    const newsId=saved?.newsId;
    if(newsId&&!Array.from($("news-link").options).some(option=>option.value===newsId))$("news-link").append(new Option(newsId,newsId));
    restoreForm(saved||{});
  }
  await Promise.all([list(),announcement?newsLinks(version):Promise.resolve()]);
}
function selectPin(noteId) {
  for(const node of document.querySelectorAll("[data-pin]"))node.classList.toggle("selected",Number(node.dataset.pin)===noteId);
  const note=document.querySelector(`.note[data-pin="${noteId}"]`);note?.scrollIntoView({behavior:"smooth",block:"nearest"});
}
async function openRecord(key) {
  if(saving)return;
  const source=tab, version=++detailRequest;
  if(tab!=="feedback")persistDraft();
  const row=await api(`/admin/api/${tab}/${key}`);
  if(version!==detailRequest||source!==tab)return;
  if(tab==="feedback") {
    reportId=key;$("report").hidden=false;$("report-status").value=row.status;
    $("report-meta").textContent=`Captured ${row.capture_at} · ${row.build} · ${row.width} × ${row.height} · ${key}`;
    $("screenshot").src=`/admin/api/feedback/${key}/screen`;
    $("download-screen").href=$("screenshot").src;
    $("download-context").href=`/admin/api/feedback/${key}/context`;
    $("pins").replaceChildren();$("notes").replaceChildren();$("context-preview").textContent="Loading diagnostic context…";
    for(const note of row.notes) {
      const pin=document.createElement("button");pin.className="pin";pin.dataset.pin=note.id;
      pin.style.left=`${note.x*100}%`;pin.style.top=`${note.y*100}%`;pin.title=note.text;
      const number=document.createElement("span");number.textContent=String(note.id+1);pin.append(number);
      pin.onclick=()=>selectPin(note.id);$("pins").append(pin);
      const item=document.createElement("div");item.className="note";item.dataset.pin=note.id;
      const title=document.createElement("strong");title.textContent=`Note ${note.id+1}`;
      item.append(title,document.createTextNode(note.text));item.onclick=()=>selectPin(note.id);$("notes").append(item);
    }
    try {
      const response=await fetch(`/admin/api/feedback/${key}/context`,{credentials:"same-origin"});
      if(!response.ok)throw new Error("Could not load diagnostic context.");
      const bytes=new Uint8Array(await response.arrayBuffer());
      if(version!==detailRequest||source!==tab)return;
      const decoded=new TextDecoder("utf-8",{fatal:true}).decode(bytes);
      try{$("context-preview").textContent=JSON.stringify(JSON.parse(decoded),null,2);}
      catch{$("context-preview").textContent=decoded;}
    } catch(error) {
      if(version===detailRequest&&source===tab)$("context-preview").textContent="Download the diagnostic capture to inspect its binary data.";
    }
    return;
  }
  if(row.news_id&&!Array.from($("news-link").options).some(o=>o.value===row.news_id))$("news-link").append(new Option(row.news_id,row.news_id));
  restoreForm({current:{id:row.id,revision:row.revision},title:row.title,ticker:row.ticker,body:row.body,
    published:row.published_at!==null,publishedAt:localDate(row.published_at),expiresAt:localDate(row.expires_at),
    newsId:row.news_id,withdrawn:!!row.withdrawn,archived:!!row.archived});
  persistDraft();status("");
}
async function save(event) {
  event.preventDefault();if(saving)return;const value=captureForm();
  if(tab==="announcements"&&value.published&&!value.withdrawn&&
      !confirm(value.resend?"Send this announcement again to all players?":"Save this active game-wide announcement?"))return;
  const payload={title:value.title,body:value.body,revision:current?.revision||0,
    published_at:value.published?(value.publishedAt?new Date(value.publishedAt).getTime():Date.now()):null};
  if(tab==="news")Object.assign(payload,{ticker:value.ticker,archived:value.archived});
  else Object.assign(payload,{expires_at:value.expiresAt?new Date(value.expiresAt).getTime():null,news_id:value.newsId||null,withdrawn:value.withdrawn,resend:value.resend});
  saving=true;$("save").disabled=true;$("edit-form").inert=true;
  try {
    const result=await api(`/admin/api/${tab}${current?`/${current.id}`:""}`,{method:"POST",body:JSON.stringify(payload)});
    saving=false;await openRecord(result.id);await list();status("Saved.");
  }finally{saving=false;$("save").disabled=false;$("edit-form").inert=false;}
}
async function safely(operation) {try{await operation();}catch(error){status(error.message,true);}}
for(const button of document.querySelectorAll("[data-tab]"))button.onclick=()=>safely(()=>switchTab(button.dataset.tab));
$("edit-form").onsubmit=event=>safely(()=>save(event));
$("edit-form").oninput=persistDraft;$("edit-form").onchange=persistDraft;
$("published").addEventListener("change",()=>{if($("published").checked&&!$("published-at").value)$("published-at").value=localDate(Date.now());persistDraft();});
$("new").onclick=()=>{if(!confirm("Start a new draft? The current local draft will be replaced."))return;restoreForm({});persistDraft();status("");};
$("discard").onclick=()=>{if(confirm("Discard this local draft? Saved articles stay on the server.")){restoreForm({});persistDraft();status("Local draft cleared.");}};
$("preview-toggle").onclick=preview;$("older").onclick=()=>safely(()=>list(true));$("filter").onchange=()=>safely(()=>list());
$("search").onkeydown=event=>{if(event.key==="Enter"){event.preventDefault();safely(()=>list());}};
$("search").addEventListener("search",()=>safely(()=>list()));
$("report-status").onchange=()=>safely(async()=>{await api(`/admin/api/feedback/${reportId}`,{method:"POST",body:JSON.stringify({status:$("report-status").value})});await list();status("Report status saved.");});
$("delete-report").onclick=()=>safely(async()=>{if(!reportId||!confirm("Permanently delete this report and its attachments?"))return;await api(`/admin/api/feedback/${reportId}`,{method:"DELETE"});reportId=null;$("report").hidden=true;await list();status("Report deleted.");});
safely(()=>switchTab("news"));
