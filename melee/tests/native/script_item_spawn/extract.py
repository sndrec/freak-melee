"""Reuse frozen scheduler substrate, explicitly replacing controlled leaves."""
import hashlib,importlib.util,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
def body(text,name):
 m=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not m:raise RuntimeError('missing '+name)
 end=text.index('{',m.start())+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[m.start():end]
def extract(root,out,overrides=()):
 root=Path(root);out=Path(out);out.mkdir(parents=True,exist_ok=True)
 source=root/'tests/native/script_item_scheduler/extract_scheduler.py'
 spec=importlib.util.spec_from_file_location('frozen_scheduler',source);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 hashes=module.extract(root,out/'actual_scheduler.h')
 actual=module.native_only((root/'src/melee/it/item.c').read_text())
 extra=[body(actual,n) for n in ['Item_802674AC','Item_CreateAuthored','Item_CreateEngine']]
 with(out/'actual_scheduler.h').open('a') as target:target.write('\n'+'\n'.join(p[:p.index('{')].rstrip()+';' for p in extra)+'\n'+'\n\n'.join(extra)+'\n')
 boundary=root/'tests/native/script_item_scheduler/boundary.c';text=boundary.read_text()
 text=text.replace('#include "boundary.h"','#include "boundary.h"\n#include "spawn_stage.h"')
 text='#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdata.h>\n#include <melee/ft/ftdat.h>\n'+text
 replaced=['nativeScriptFighter_CanBeginScene','nativeScriptFighter_BeginScene','nativeScriptFighter_RetireScene','nativeScriptFighter_View','nativeScriptFighter_Helper',
  'HSD_ObjAlloc','HSD_ObjFree','nativeItemBehavior_Select','Item_80267978','nativeItem_CaptureCreator','nativeItem_Creator','it_80275158','db_80225DD8','it_80279B64','Item_80267130','Item_802680CC']
 for name in replaced:text=text.replace(body(text,name),'')
 for name in overrides:
  pattern=re.compile(r'\b(?:READ|WRITE|CALL)\('+re.escape(name)+r'\)')
  if pattern.search(text):text=pattern.sub('',text)
  else:text=text.replace(body(text,name),'')
 properties=root/'src/melee/ft/native_script_binding_properties.c'
 guards='\n'.join(body(properties.read_text(),n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
 guards=re.sub(r'\berror\b','spawn_guard_error',guards)
 if 'nativeScriptBindings_SourceAlive' not in overrides:
  text=text.replace(body(text,'nativeScriptBindings_SourceAlive'),'')+'\n'+guards+'\n'
 hashes[str(properties)]=hashlib.sha256(properties.read_bytes()).hexdigest()
 text=text.replace('#include "actual_scheduler.h"','#include "spawn_leaves.h"\n#include "actual_scheduler.h"')
 text=text.replace('HOLD(nativeHoldable_PickedUp)','')
 text=text.replace('item->pos.x=v->as.f32;','item->pos.x=v->as.f32;if(v->as.f32==101)stage(StageSpawned);else if(v->as.f32==103)stage(StageCreated);else if(v->as.f32==107)stage(StageLaunched);else if(v->as.f32==109)stage(StagePickup);')
 (out/'boundary.c').write_text(text,encoding='utf-8')
 protocol=root/'src/melee/it/native_holdable_protocol.c';protocol_source=protocol.read_text()
 declaration=re.search(r'typedef struct HoldableSource \{[^}]*\} HoldableSource;',protocol_source).group()
 with(out/'actual_scheduler.h').open('a') as target:target.write('\n'+declaration+'\n'+'\n\n'.join(body(protocol_source,n) for n in ['begin','current','nativeHoldable_PickedUp'])+'\n')
 generated=(out/'actual_scheduler.h').read_text();original=body(generated,'Item_80268E5C');point=original.index('{')+1
 instrumented=original[:point]+'\n    stage(StageState);'+original[point:]
 (out/'actual_scheduler.h').write_text(generated.replace(original,instrumented))
 lookup=root/'src/melee/ft/ftlib.c'
 with(out/'actual_scheduler.h').open('a') as target:
  target.write('\n#include <melee/ft/inlines.h>\n'+body(lookup.read_text(),'ftLib_8008741C')+'\n')
 hashes[str(lookup)]=hashlib.sha256(lookup.read_bytes()).hexdigest()
 hashes[str(protocol)]=hashlib.sha256(protocol.read_bytes()).hexdigest()
 for path in [source,boundary,root/'tests/native/script_item_scheduler/boundary.h',root/'tests/native/script_kinetics/extract_primitives.py',Path(__file__).resolve()]:hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 return hashes
