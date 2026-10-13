"""Exact current shared bodies plus pre-batch healthy native comparisons."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'script_fighter_scheduler'
spec=importlib.util.spec_from_file_location('air_scheduler',BASE/'extract_scheduler.py')
substrate=importlib.util.module_from_spec(spec);spec.loader.exec_module(substrate)
REFERENCE='7b077ac85'
def extract(root,out):
 hashes=substrate.extract(root,out/'actual_scheduler.h')
 body=substrate.reference.reference.body
 native=substrate.reference.native_only
 pieces=[]
 path=root/'src/melee/ft/kinds/ftPeach/ftpeachspecials.c'
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 value=body(native(path.read_text()),'doAirEnd0')
 value=value.replace('static void doAirEnd0','void ContactPeachContinuation')
 (out/'actual_contact.h').write_text(value+'\n')
 reference=subprocess.run(['git','show',REFERENCE+':src/melee/ft/native_fighter_contact.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 reference=body(reference,'nativeFighterContact_ReboundAllowed').replace('nativeFighterContact_ReboundAllowed','ContactReferenceReboundAllowed')
 (out/'reference_contact.h').write_text(reference+'\n')
 boundary=(BASE/'boundary.c').read_text()
 props=root/'src/melee/ft/native_script_binding_properties.c'
 hashes[str(props)]=hashlib.sha256(props.read_bytes()).hexdigest()
 for name in ['nativeScriptBindings_Context','nativeScriptBindings_SourceAlive']:
  boundary=boundary.replace(body(boundary,name),'')
 write=body(boundary,'nativeScriptBindings_Write')
 boundary=boundary.replace(write,'''bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d){(void)s;(void)d;if(c->source_class!=NativeCallbackSource_Fighter||id!=NativeScriptBinding_PositionX||n||v->type!=NativeScriptType_F32||scheduler_fail_write)return false;((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x=v->as.f32;contact_observe(c->source);return true;}''')
 guards='\n'.join(re.sub(r'\berror\b','air_guard_error',body(props.read_text(),name)) for name in ['error','nativeScriptBindings_Context','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
 includes='''#include "fixture.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_contact.h>
static void enterAirEndSmash(HSD_GObj* object){((Fighter*)object->user_data)->motion_id=888;}
'''
 (out/'boundary.c').write_text(includes+boundary+'\n'+guards+'\n#include "actual_contact.h"\n#include "reference_contact.h"\n')
 (out/'reference_revision.txt').write_text(REFERENCE+'\n')
 return hashes
