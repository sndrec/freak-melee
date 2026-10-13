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
 pieces=[];frozen=[]
 selected={'src/melee/ft/ft_081B.c':['ftCommon_ResolveAirLanding','ft_80083090','ft_800831CC','ft_800835B0'],
           'src/melee/ft/ft_084E.c':['ft_800851C0','ft_800851D0'],
           'src/melee/ft/kinds/ftCommon/ftCo_FallSpecial.c':['ftCo_80096CC8']}
 for path,names in selected.items():
  file=root/path;hashes[str(file)]=hashlib.sha256(file.read_bytes()).hexdigest()
  text=native(file.read_text())
  pieces += [body(text,name) for name in names]
 for path,names in {'src/melee/ft/ft_081B.c':['ft_80083090_inline','ft_80083090','ft_800831CC','ft_800835B0'],
                    'src/melee/ft/ft_084E.c':['ft_800851C0','ft_800851D0']}.items():
  text=subprocess.run(['git','show',REFERENCE+':'+path],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  text=native(text)
  for name in names:
   value=body(text,name)
   for old in ['ft_80083090_inline','ft_80083090','ft_800831CC','ft_800835B0','ft_800851C0','ft_800851D0']:
    value=re.sub(r'\b'+old+r'\b','Frozen_'+old,value)
   frozen.append(value)
 all_bodies=pieces+frozen
 (out/'actual_air.h').write_text('\n'.join(x[:x.index('{')].rstrip()+';' for x in pieces)+'\n'+'\n'.join(all_bodies)+'\n')
 boundary=(BASE/'boundary.c').read_text()
 props=root/'src/melee/ft/native_script_binding_properties.c'
 hashes[str(props)]=hashlib.sha256(props.read_bytes()).hexdigest()
 for name in ['nativeScriptBindings_Context','nativeScriptBindings_SourceAlive']:
  boundary=boundary.replace(body(boundary,name),'')
 write=body(boundary,'nativeScriptBindings_Write')
 boundary=boundary.replace(write,'''bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d){(void)s;(void)d;if(c->source_class!=NativeCallbackSource_Fighter||id!=NativeScriptBinding_PositionX||n||v->type!=NativeScriptType_F32||scheduler_fail_write)return false;((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x=v->as.f32;air_point(Air_Helper,c->source);return true;}''')
 guards='\n'.join(re.sub(r'\berror\b','air_guard_error',body(props.read_text(),name)) for name in ['error','nativeScriptBindings_Context','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
 includes='''#include "air_boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/ft_0819.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_StopCeil.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftwalljump.h>
'''
 (out/'boundary.c').write_text(includes+boundary+'\n'+guards+'\n#include "actual_air.h"\n')
 (out/'reference_revision.txt').write_text(REFERENCE+'\n')
 return hashes
