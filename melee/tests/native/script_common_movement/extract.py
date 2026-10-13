"""Extract exact catalog/guard and native movement bodies, with source hashes."""
import hashlib,re
from pathlib import Path

def body(text,name):
 m=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not m:raise RuntimeError('missing function '+name)
 at=text.index('{',m.start())+1;depth=1
 while depth:depth+=(text[at]=='{')-(text[at]=='}');at+=1
 return text[m.start():at]

def extract(root,out):
 root=Path(root);out=Path(out);out.mkdir(parents=True,exist_ok=True);hashes={};texts={}
 for relative in ['src/melee/ft/native_script_bindings.c','src/melee/ft/native_script_binding_properties.c','src/melee/ft/ftcommon.c','src/melee/ft/ftmovement.c','src/melee/ft/kinds/ftCommon/ftCo_Pass.c','src/melee/mp/mpcoll.c']:
  p=root/relative;texts[relative]=p.read_text();hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 catalog=texts['src/melee/ft/native_script_bindings.c'];start=catalog.index('#define F ');end=catalog.index('\n};',catalog.index('static const NativeScriptBindingDescriptor catalog'))+3
 guards=texts['src/melee/ft/native_script_binding_properties.c']
 (out/'source_contract.inc').write_text(catalog[start:end]+'\n'+'#include <melee/ft/native_script_binding_item_resources.h>\n#include <melee/ft/native_script_binding_spawn.h>\n'+body(catalog,'nativeScriptBindings_Descriptor')+'\n'+ '\n'.join(re.sub(r'\berror\b','movement_guard_error',body(guards,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])+'\n')
 native=[]
 for relative,names in [('src/melee/ft/ftmovement.c',['ftCommon_ClampGroundVel']),('src/melee/ft/ftcommon.c',['ftCommon_UnlockECB','ftCommon_UpdateFacing','ftCommon_8007DA24','ftCommon_8007D5D4','ftCommon_8007D60C','ftCommon_8007D6A4','ftCommon_8007D7FC']),('src/melee/mp/mpcoll.c',['mpColl_IsOnPlatform','mpUpdateFloorSkip']),('src/melee/ft/kinds/ftCommon/ftCo_Pass.c',['ftCo_8009A134'])]:
  native.extend(body(texts[relative],n) for n in names)
 (out/'native_bodies.inc').write_text('\n'.join(native)+'\n')
 return hashes
