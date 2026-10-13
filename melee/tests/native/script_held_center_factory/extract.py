"""Actual constructor/scheduler substrate, source guards and ECB kernels."""
import hashlib,importlib.util,re
from pathlib import Path

def extract(root,out):
 root=Path(root);out=Path(out);base=root/'tests/native/script_positional_article_spawn/extract.py'
 spec=importlib.util.spec_from_file_location('held_substrate',base);shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
 hashes=shared.extract(root,out)
 bodies=root/'tests/native/script_item_spawn/extract.py';spec=importlib.util.spec_from_file_location('held_body',bodies);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 leaves=out/'spawn_leaves.h';text=leaves.read_text()
 for name in ['it_8026BB68','ftLib_80086990']:text=text.replace(module.body(text,name),'')
 center=root/'src/melee/ft/ftlib.c';native=center.read_text();item=root/'src/melee/it/it_26B1.c'
 text+='\n#include <melee/ft/inlines.h>\n'+module.body(native,'vector_add')+'\n'+module.body(native,'ftLib_80086990')+'\n'+module.body(item.read_text(),'it_8026BB68')+'\n'
 leaves.write_text(text)
 boundary=out/'boundary.c';text=boundary.read_text();text=text.replace(module.body(text,'nativeScriptBindings_Read'),
 'bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){(void)s;(void)d;if(id!=NativeScriptBinding_ItemCreationValue||n)return false;float value;if(!nativeScriptItem_GetCreationValue(c->source,&value))return false;*r=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};return true;}')
 text=text.replace('if(scheduler_clear_owner==c->source)', 'if(scheduler_fail_write)return false;if(scheduler_clear_owner==c->source)')
 text=text.replace('(void)part;((Item*)o->user_data)->owner=target;','held_native_attach(o,target,part);')
 text=text.replace('void Item_8026B074(Item* item){(void)item;++scheduler_tail;}','void Item_8026B074(Item* item){(void)item;++scheduler_tail;++held_sound_tail;}')
 # Prototypes follow normal engine includes, so complete actual types are available.
 text=text.replace('#pragma clang section data = ""','#pragma clang section data = ""\nextern unsigned held_sound_tail;\nvoid positional_stage(unsigned,HSD_GObj*);\nvoid held_native_attach(HSD_GObj*,HSD_GObj*,Fighter_Part);')
 boundary.write_text(text)
 stock=root/'src/melee/it/kinds/itlinkbomb.c';source=module.body(stock.read_text(),'it_8029DD58');start=source.index('    spawn.kind =');end=source.index('    temp_r3 =',start)
 prefix=source[start:end]
 (out/'native_reference.inc').write_text('static SpawnItem native_spawn_arguments(Item_GObj* fighter_gobj,Vec3* arg1,u32 arg2,int arg3,f32 arg4)\n{(void)arg2;SpawnItem spawn={0};\n'+prefix+'return spawn;}\n')
 for path in [base,bodies,center,item,stock,Path(__file__).resolve()]:hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 return hashes
