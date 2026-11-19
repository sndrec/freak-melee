"""Literal dispatcher ABI inventory with complete synchronous Vec3 usage proof."""
import hashlib,json,re,subprocess
from pathlib import Path

def masked(text):
 return re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:''.join('\n' if c=='\n' else ' ' for c in m.group()),text,flags=re.S)

def closing(text,start):
 mask=masked(text);depth=1;at=start+1
 while depth:depth+=(mask[at]=='{')-(mask[at]=='}');at+=1
 return at

def body(text,name):
 m=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not m:raise RuntimeError('missing '+name)
 return text[m.start():closing(text,text.index('{',m.start()))]

def cases(text,function,minimum,maximum):
 text=body(text,function);switch=re.search(r'switch\s*\(gfx_id\)\s*\{',text)
 if not switch:raise RuntimeError('missing effect switch')
 start=text.index('{',switch.start());end=closing(text,start);mask=masked(text);depth=1;labels=[];at=start+1
 for m in re.finditer(r'\bcase\s+(0x[0-9A-Fa-f]+|\d+)\s*:|\bdefault\s*:',mask[start+1:end-1]):
  pos=start+1+m.start()
  for c in mask[at:pos]:depth+=(c=='{')-(c=='}')
  at=pos
  if depth==1:labels.append((pos,start+1+m.end(),int(m.group(1),0) if m.group(1) else None))
 pending=[];result={}
 for i,(pos,after,label) in enumerate(labels):
  part=text[after:labels[i+1][0] if i+1<len(labels) else end-1];pending.append(label)
  stripped=re.sub(r'^\s*#.*$','',masked(part),flags=re.M)
  if not stripped.strip():continue
  types=[a or b for a,b in re.findall(r'va_arg\(\w+,\s*([^)]*)\)|EFALT_VA_ARG\(([^)]*)\)',part)]
  for identifier in pending:
   if identifier is not None and minimum<=identifier<maximum:result[identifier]=(types,part)
  pending=[]
 return result

def vector_proof(part):
 read=r'(?:va_arg\(\w+,\s*Vec3\*\)|EFALT_VA_ARG\(Vec3\*\))'
 if re.search(r'efLib_Create(?:Generator|_Attach_Pos)\([^;]*'+read+r'\)',part):return 'synchronous helper copy'
 if re.search(r'->params\s*=\s*\*'+read,part):return 'direct value copy'
 if re.search(r'va_vec3\s*=\s*'+read,part):
  occurrences=[line.strip() for line in part.splitlines() if re.search(r'\bva_vec3\b',line)]
  if len(occurrences)==2 and re.fullmatch(r'va_vec3\s*=\s*'+read+r';',occurrences[0]) and re.fullmatch(r'(?:translate|psAppSRT->translate)\s*=\s*\*va_vec3;',occurrences[1]):return 'local or AppSRT value copy'
 raise RuntimeError('unverified exact Vec3 pointer consumer:\n'+part)

def position_requirements(part,eflib,helpers):
 flags=0;observations=[]
 root_reads=[line.strip() for line in part.splitlines() if re.search(r'\bGET_JOBJ\(gobj\)|\bgobj->hsd_obj',line)]
 if root_reads:flags|=1;observations+=root_reads
 if 'efLib_Create_Attach_Pos(' in part:flags|=4;observations.append('efLib_Create_Attach_Pos stores parent GObj and copies position into the created model root')
 direct=[line.strip() for line in part.splitlines() if re.search(r'\bGET_FIGHTER\(gobj\)|\bgobj->user_data',line)]
 if direct:raise RuntimeError('new direct creator userdata dependency needs source-class audit:\n'+part)
 for callback in re.findall(r'->update\s*=\s*([^;]+);',part):
  callback=callback.strip()
  if not re.fullmatch(r'\w+',callback):raise RuntimeError('unresolved Position update callback '+callback)
  source=body(eflib,callback);helpers[callback]=source
  if 'parent_gobj' in source:
   if not re.search(r'GET_FIGHTER\(effect->parent_gobj\)',source) and callback!='efLib_Cb_ftCo_Bury':raise RuntimeError('unverified callback parent assumption '+callback)
   flags|=2;observations.append(callback+' reads actual Fighter parent userdata')
  else:observations.append(callback+' reads only its own effect/model')
 return flags,observations

def scalar_position_proof(identifier,types,part,all_cases):
 # Literal branches below consume one scalar, although mutually exclusive
 # va_arg expressions appear twice in the source. They never execute in series.
 if identifier in {0x3EF,0x3F0,0x3F1,0x3F2,0x400,0x401}:
  if types!=['Vec3*','f32*','f32*'] or not re.search(r'if \(gfx_id == 0x[0-9A-F]+\).*?\} else \{',part,re.S):raise RuntimeError('facing branch arity changed')
 elif identifier==0x3E8:
  if types!=['Vec3*','Vec3*','f32*'] or 'if (HSD_Randi(8) == 0)' not in part:raise RuntimeError('random model branch arity changed')
 elif identifier in {0x435,0x436}:
  if types or 'goto block_515;' not in part:raise RuntimeError('shared rotation jump changed')
  types,target=all_cases[0x437]
  if types!=['Vec3*','f32*'] or 'block_515:' not in target:raise RuntimeError('shared rotation target changed')
  part+=target
 elif types!=['Vec3*','f32*'] or identifier==0x4D0:return None
 # No variadic forwarding or erased payload enters this family.
 remainder=re.sub(r'va_arg\(\w+,\s*[^)]*\)|EFALT_VA_ARG\([^)]*\)','',part)
 if re.search(r'\bvlist(?:_arg)?\b',remainder):return None
 if 'va_vec3' in part:
  uses=[line.strip() for line in part.splitlines() if re.search(r'\bva_vec3\b',line)]
  for line in uses:
   if not re.fullmatch(r'(?:va_vec3 = va_arg\(vlist, Vec3\*\);|translate = \*va_vec3;|psAppSRT->translate = \*va_vec3;|f32_1 = va_vec3->x;)',line):raise RuntimeError('new position pointer use: '+line)
 else:vector_proof(part)
 if 'value_ptr' in part:
  uses=[line.strip() for line in part.splitlines() if re.search(r'\bvalue_ptr\b',line)]
  if uses!=['value_ptr = EFALT_VA_ARG(f32*);','if (*value_ptr < 0.0f) {']:raise RuntimeError('new scalar pointer retention')
 return part

def extract(root,out):
 root=Path(root);out=Path(out);out.mkdir(parents=True,exist_ok=True);texts={};hashes={}
 for rel in ['src/melee/ef/efsync.c','src/melee/ef/efalt.c','src/melee/ef/efasync.c','src/melee/ef/eflib.c','src/sysdolphin/baselib/jobj.h','src/sysdolphin/baselib/generator.c']:
  p=root/rel;texts[rel]=p.read_text();hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 helpers={name:body(texts['src/melee/ef/eflib.c'],name) for name in ['efLib_CreateGenerator','efLib_Create_Attach_Pos']}
 helpers['HSD_JObjSetTranslate']=body(texts['src/sysdolphin/baselib/jobj.h'],'HSD_JObjSetTranslate')
 for n in ['efLib_CreateGenerator_Attach_Scale','eflib_create_generator_add_appsrt','efLib_CreateGenerator_AppSRT_SetFacingDir']:helpers[n]=body(texts['src/melee/ef/eflib.c'],n)
 helpers['efLib_CreateGenerator_Translate_FacingDir']=body(texts['src/melee/ef/eflib.c'],'efLib_CreateGenerator_Translate_FacingDir')
 for name in ['efAsync_SetEffectFacingDir','efAsync_SetEffectRotationZFromPtr']:
  helpers[name]=body(texts['src/melee/ef/efasync.c'],name)
 rotation=helpers['efAsync_SetEffectRotationZFromPtr']
 if 'HSD_JObjSetRotationZ(GET_JOBJ(effect->gobj), *rotation);' not in rotation or len(re.findall(r'\brotation\b',rotation))!=2:raise RuntimeError('rotation pointer consumption changed')
 bury=root/'src/melee/ft/kinds/ftCommon/ftCo_Bury.c';hashes[str(bury)]=hashlib.sha256(bury.read_bytes()).hexdigest()
 helpers['ftCo_800C0FCC']=body(bury.read_text(),'ftCo_800C0FCC')
 if 'Fighter* fp = GET_FIGHTER(arg1);' not in helpers['ftCo_800C0FCC']:raise RuntimeError('Bury callback creator class changed')
 helpers['hsd_8039EFAC']=body(texts['src/sysdolphin/baselib/generator.c'],'hsd_8039EFAC')
 generator=helpers['efLib_CreateGenerator'];position=helpers['efLib_Create_Attach_Pos'];translate=helpers['HSD_JObjSetTranslate']
 helpers['efLib_Create']=body(texts['src/melee/ef/eflib.c'],'efLib_Create')
 create=helpers['efLib_Create']
 if 'new_effect->update = NULL;' not in create or 'effect->parent_gobj = parent_gobj;' not in create:raise RuntimeError('common model parent/update initialization changed')
 if re.search(r'parent_gobj->|GET_(?:JOBJ|FIGHTER)\(parent_gobj\)',create):raise RuntimeError('common model constructor now reads parent root or userdata')
 if re.search(r'\bgobj->|GET_(?:JOBJ|FIGHTER)\(gobj\)',position):raise RuntimeError('position helper now reads creator root or userdata')
 if [line.strip() for line in generator.splitlines() if re.search(r'\bpos\b',line)]!=[
    'HSD_Generator* efLib_CreateGenerator(s32 gfx_id, Vec3* pos)', 'generator->pos.x = pos->x;','generator->pos.y = pos->y;','generator->pos.z = pos->z;']:raise RuntimeError('generator pointer consumption changed')
 if [line.strip() for line in position.splitlines() if re.search(r'\bposition\b',line)]!=[
    'EF_Effect* efLib_Create_Attach_Pos(u32 gfx_id, HSD_GObj* gobj, Vec3* position)','HSD_JObjSetTranslate(jobj, position);']:raise RuntimeError('model position pointer use changed')
 if [line.strip() for line in translate.splitlines() if re.search(r'\btranslate\b',line)]!=[
    'static inline void HSD_JObjSetTranslate(HSD_JObj* jobj, Vec3* translate)','HSD_ASSERT(917, translate);','jobj->translate = *translate;']:raise RuntimeError('JObj translation copy/retention changed')
 scale=helpers['efLib_CreateGenerator_Attach_Scale']
 if re.findall(r'va_arg\(\w+,\s*([^)]*)\)',scale)!=['HSD_JObj*'] or 'HSD_JObjGetScale' not in scale or 'generator->appsrt->scale.z = scale.y;' not in scale:raise RuntimeError('Attach_Scale argument/read contract changed')
 attach=helpers['eflib_create_generator_add_appsrt'];native=helpers['hsd_8039EFAC']
 if 'hsd_8039EFAC(0, gfx_id / 1000, gfx_id, jobj)' not in attach or 'gen->jobj = jobj;' not in native or 'ref_INC(jobj);' not in native:raise RuntimeError('generator attachment/refcount contract changed')
 facing=helpers['efLib_CreateGenerator_AppSRT_SetFacingDir']
 if re.findall(r'va_arg\(\w+,\s*([^)]*)\)',facing)!=['HSD_JObj*','f32*'] or 'direction = *va_arg(vlist, f32*);' not in facing or 'generator->appsrt->rot.y = direction < 0.0F ? -M_PI_2 : M_PI_2;' not in facing or 'eflib_create_generator_add_appsrt(&generator, gfx_id,' not in facing:raise RuntimeError('facing generator joint/copied scalar contract changed')
 if len(re.findall(r'\bdirection\b',facing))!=3:raise RuntimeError('facing generator now retains or otherwise consumes scalar')
 offset_helper=body(texts['src/melee/ef/eflib.c'],'efLib_CreateGenerator_AppSRT_SetPos')
 if 'effect->params = *vec;' not in offset_helper or 'effect->attach_jobj = jobj;' not in offset_helper or 'effect->update = efLib_Cb_AccumOffset_FromParams;' not in offset_helper:
  raise RuntimeError('joint offset constructor copy/attachment contract changed')
 if re.findall(r'\bvec\b',offset_helper)!=['vec','vec']:raise RuntimeError('joint offset constructor retains caller pointer')
 helpers['efLib_CreateGenerator_AppSRT_SetPos']=offset_helper
 for name in ['efLib_Create_Attach','efLib_Create_Attach_Scale_FacingDir','efLib_Cb_SetRotY_FromFighterDir']:
  helpers[name]=body(texts['src/melee/ef/eflib.c'],name)
 model_attach=helpers['efLib_Create_Attach'];model_scale=helpers['efLib_Create_Attach_Scale_FacingDir']
 if not re.search(r'HSD_JObj\*\s+jobj\)',model_scale) or 'efLib_Create_Attach(gfx_id, gobj, jobj)' not in model_scale or 'HSD_JObjGetScale(GET_JOBJ(gobj), &scale);' not in model_scale or 'HSD_JObjSetScale(GET_JOBJ(effect->gobj), &scale);' not in model_scale or 'effect->update = efLib_Cb_SetRotY_FromFighterDir;' not in model_scale:
  raise RuntimeError('typed attached model scale/facing contract changed')
 if 'effect->attach_jobj = jobj;' not in model_attach or 'lb_8000C1C0(effect_jobj, jobj);' not in model_attach or 'GET_FIGHTER(effect->parent_gobj)->facing_dir' not in helpers['efLib_Cb_SetRotY_FromFighterDir']:
  raise RuntimeError('model attachment or Fighter-parent facing dependency changed')
 expected={};requirements={};scalar_requirements={};two_scalar_requirements={};proof=[];positions=[]
 for filename,function,low,high in [('efsync.c','efSync_Spawn',0x4BA,65536),('efalt.c','efAlt_Spawn',0x478,0x4BA),('efasync.c','efAsync_Dispatch',0x250,0x478)]:
  all_cases=cases(texts['src/melee/ef/'+filename],function,low,high)
  for identifier,(types,part) in all_cases.items():
   remainder=re.sub(r'va_arg\(\w+,\s*[^)]*\)|EFALT_VA_ARG\([^)]*\)','',part)
   forwarded=bool(re.search(r'\bvlist(?:_arg)?\b',remainder))
   classification=1 if types==['HSD_JObj*'] else 2 if types==['HSD_JObj*','f32*'] else 0
   if forwarded:classification=0
   forwarding_proof=None
   if types==['void*'] and not forwarded and re.search(r'efLib_Create_Attach_Scale_FacingDir\(\s*0x[0-9A-Fa-f]+,\s*gobj,\s*va_arg\(vlist,\s*void\*\)\)',part):
    classification=1;forwarding_proof='one erased JObj through typed Attach_Scale_FacingDir; native attachment, copied creator-root scale, live Fighter-parent facing update'
   match=re.search(r'efLib_CreateGenerator_Attach_Scale\(0x[0-9A-Fa-f]+,\s*vlist(?:_arg)?,\s*gobj\)',part)
   if not types and match and not re.search(r'\bvlist(?:_arg)?\b',remainder[:match.start()]+remainder[match.end():]):
    classification=1;forwarding_proof='one JObj through Attach_Scale; native refcounted attachment, copied root scale'
   match=re.search(r'efLib_CreateGenerator_AppSRT_SetFacingDir\(0x[0-9A-Fa-f]+,\s*vlist(?:_arg)?\)',part)
   if not types and match and not re.search(r'\bvlist(?:_arg)?\b',remainder[:match.start()]+remainder[match.end():]):
    classification=2;forwarding_proof='JObj plus synchronously consumed F32 pointer through AppSRT facing helper; refcounted attachment'
   consumed=None
   if types==['Vec3*'] and not forwarded:consumed=vector_proof(part);classification=3;positions.append(identifier)
   scalar_part=scalar_position_proof(identifier,types,part,all_cases)
   if scalar_part is not None:
    classification=5;consumed='complete executed branch Vec3/F32 immediate copy; no caller pointer retained'
    flags,dependencies=position_requirements(scalar_part,texts['src/melee/ef/eflib.c'],helpers)
    scalar_requirements[identifier]=flags
   if identifier==0x3FF:
    if types!=['Vec3*','f32*','f32*'] or forwarded:raise RuntimeError('position/two-scalar native argument contract changed')
    vector_proof(part)
    if not re.search(r'efAsync_SetEffectFacingDir\(spawned.object.model,\s*\*va_arg\(vlist, f32\*\)\);',part) or not re.search(r'efAsync_SetEffectRotationZFromPtr\(spawned.object.model,\s*va_arg\(vlist, f32\*\)\);',part):raise RuntimeError('ordered facing/rotation scalar consumption changed')
    classification=6;consumed='copied position, dereferenced facing, immediately copied rotation; no caller pointer retained'
    flags,dependencies=position_requirements(part,texts['src/melee/ef/eflib.c'],helpers)
    two_scalar_requirements[identifier]=flags
   if identifier==0x448:
    if types!=['void*','void*'] or forwarded or 'efLib_CreateGenerator_AppSRT_SetPos(0x92, gobj, va_jobj, va_pos)' not in part:
     raise RuntimeError('joint offset argument order changed')
    classification=7;consumed='typed JObj attachment and copied Vec3 offset; no caller vector retained'
   if identifier in {0x48F,0x4BB,0x4BC,0x50B}:
    signatures={0x48F:['HSD_JObj*','HSD_JObj*'],0x4BB:['void*','HSD_JObj*'],0x4BC:['void*','HSD_JObj*'],0x50B:['void*','void*']}
    if types!=signatures[identifier] or forwarded:raise RuntimeError('two-joint native argument contract changed: '+hex(identifier))
    first='efLib_Create_Attach_Scale_FacingDir('
    second='efLib_Create_AttachChild_Scale(' if identifier in {0x4BB,0x4BC} else first
    if first not in part or second not in part or 'spawned.object.model != NULL' not in part:raise RuntimeError('ordered conditional two-joint construction changed: '+hex(identifier))
    classification=4;forwarding_proof='exact two-joint signature; second construction conditional on first model success'
   flags=scalar_requirements.get(identifier);dependencies=[]
   if classification==5:flags,dependencies=position_requirements(scalar_part,texts['src/melee/ef/eflib.c'],helpers)
   if classification==3:flags,dependencies=position_requirements(part,texts['src/melee/ef/eflib.c'],helpers);requirements[identifier]=flags
   if classification==6:flags,dependencies=position_requirements(part,texts['src/melee/ef/eflib.c'],helpers)
   expected[identifier]=classification;proof.append({'file':filename,'id':identifier,'arguments':types,'forwarded':forwarded,'classification':classification,'position_consumption':consumed,'position_requirements':flags,'dependency_proof':dependencies,'forwarding_proof':forwarding_proof,'body_sha256':hashlib.sha256(part.encode()).hexdigest(),'body':part})
 # The two early synchronous generator branches precede every literal switch.
 # Prove their exact pointer consumption independently of the production table.
 sync=body(texts['src/melee/ef/efsync.c'],'efSync_Spawn');generator_ranges=[]
 for condition,low,high in [('gfx_id < 0x250',0,0x250),('gfx_id / 1000 == 0x1E',30000,31000)]:
  marker='if ('+condition+') {';start=sync.index(marker)+len(marker)-1
  part=sync[start+1:closing(sync,start)-1]
  if re.findall(r'va_arg\(\w+,\s*([^)]*)\)',part)!=['Vec3*'] or 'efLib_CreateGenerator(gfx_id, va_vec3)' not in part or 'return spawned;' not in part:
   raise RuntimeError('numeric generator argument contract changed: '+condition)
  uses=[line.strip() for line in part.splitlines() if re.search(r'\bva_vec3\b',line)]
  if uses!=['va_vec3 = va_arg(vlist, Vec3*);','spawned.object.generator = efLib_CreateGenerator(gfx_id, va_vec3);']:
   raise RuntimeError('numeric generator pointer retention changed: '+condition)
  for identifier in range(low,high):expected[identifier]=3;requirements[identifier]=0;positions.append(identifier)
  generator_ranges.append({'condition':condition,'first':low,'end_exclusive':high,'body':part,'body_sha256':hashlib.sha256(part.encode()).hexdigest()})
 (out/'expected.inc').write_text('static const unsigned char expected[65536]={\n'+''.join(f'[{i}]={v},\n' for i,v in sorted(expected.items()) if v)+'};\n')
 (out/'requirements.inc').write_text('static const unsigned char expected_requirements[65536]={\n'+''.join(f'[{i}]={v},\n' for i,v in sorted(requirements.items()) if v)+'};\n')
 (out/'scalar_requirements.inc').write_text('static const unsigned char expected_scalar_requirements[65536]={\n'+''.join(f'[{i}]={v},\n' for i,v in sorted(scalar_requirements.items()))+'};\n')
 (out/'two_scalar_requirements.inc').write_text('static const unsigned char expected_two_scalar_requirements[65536]={\n'+''.join(f'[{i}]={v},\n' for i,v in sorted(two_scalar_requirements.items()))+'};\n')
 # The actual selected dispatcher/helper bodies must match the committed
 # native checkpoint before the controlled fixture compiles them.
 for rel,fn in [('src/melee/ef/efsync.c','efSync_Spawn'),('src/melee/ef/efasync.c','efAsync_Dispatch'),('src/melee/ef/efalt.c','efAlt_Spawn'),('src/melee/ef/eflib.c','efLib_CreateGenerator_Translate_FacingDir')]:
  reference=subprocess.run(['git','show','50f2b476b:'+rel],cwd=root,capture_output=True,text=True,check=True).stdout
  expected=body(reference,fn)
  if fn=='efSync_Spawn':
   expected=expected.replace('efLib_Create(0x32C9, gobj)', 'efLib_Create(0x32C9, gobj, NULL)')
  if body(texts[rel],fn)!=expected:raise RuntimeError('native checkpoint body changed: '+fn)
 (out/'proof.json').write_text(json.dumps({'cases':proof,'generator_ranges':generator_ranges,'position_ids':sorted(positions),'position_requirements':requirements,'position_scalar_requirements':scalar_requirements,'position_two_scalar_requirements':two_scalar_requirements,'helpers':helpers},indent=2)+'\n')
 (out/'copy_bodies.inc').write_text(helpers['efLib_CreateGenerator']+'\n'+helpers['efLib_Create_Attach_Pos']+'\n'+helpers['efLib_CreateGenerator_Translate_FacingDir']+'\n'+helpers['efLib_CreateGenerator_AppSRT_SetPos']+'\n')
 return hashes,sorted(positions)
