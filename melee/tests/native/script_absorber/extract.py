"""Actual common absorber and absorption scheduler slices; unrelated tails omitted."""
import hashlib,importlib.util,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('body',HERE.parent/'script_form_feedback/extract.py')
body_module=importlib.util.module_from_spec(spec);spec.loader.exec_module(body_module)
def extract(root,out):
 hashes={}
 def read(path):
  p=root/path;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 properties=read('src/melee/ft/native_script_binding_properties.c')
 guards='\n'.join(body_module.body(properties,n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
 guards=re.sub(r'\berror\b','absorber_guard_error',guards)
 boundary=read('tests/native/script_fighter_scheduler/boundary.c')
 for name in ['nativeScriptBindings_SourceAlive','nativeScriptBindings_Read','nativeScriptBindings_Write']:
  boundary=boundary.replace(body_module.body(boundary,name),'')
 boundary=boundary.replace('READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)','READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write)')
 boundary='#include <melee/ft/native_source_package.h>\n#include <melee/ft/native_script_binding_absorber.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n'
 (out/'boundary.c').write_text(boundary)
 coll=read('src/melee/ft/ftcoll.c')
 (out/'native_absorber.inc').write_text(body_module.body(coll,'ftColl_CreateAbsorbHit')+'\n')
 fighter=read('src/melee/ft/fighter.c')
 start=fighter.index('if (nativeScriptFighter_IsAttached(gobj))',fighter.index('} else if (fp->AbsorbAttr.x1A40_absorbHitDirection)'))
 end=fighter.index('\n            } else if (fp->unk_gobj',start)
 branch=fighter[start:end].replace('#endif','')
 reset='\n'.join(re.findall(r'^\s*fp->AbsorbAttr\.\w+ = .*;',fighter[fighter.index('void Fighter_ProcessHit_8006D1EC'):],re.M))
 assert len(reset.splitlines())==3
 (out/'native_absorb_schedule.inc').write_text('static void absorber_schedule(Fighter_GObj* gobj) {\nFighter* fp=gobj->user_data;uint32_t callback_generation=fp->x8_spawnNum;\nif(fp->AbsorbAttr.x1A40_absorbHitDirection){\n'+branch+'\n}\n++completed_tails;\n'+reset+'\n}\n')
 combat=read('src/melee/ft/native_script_binding_combat.c')
 arm=re.search(r'case NativeScriptBinding_DamageHeal:([^\n]+)',combat).group(1)
 (out/'heal_dispatch.inc').write_text('bool nativeScriptBindingCombat_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic){(void)diagnostic;HSD_GObj* object=context->source;if(semantic!=NativeScriptBinding_DamageHeal||count!=1||values[0].type!=NativeScriptType_F32)return false;switch(semantic){case NativeScriptBinding_DamageHeal:'+arm+'default:abort();}*result=(NativeScriptValue){.type=NativeScriptType_Void};return true;}\n')
 return hashes
