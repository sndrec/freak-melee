"""Actual EF model creation/update path with explicit excluded gfx branches."""
from pathlib import Path
import importlib.util,hashlib,re

def module(path,name):
 spec=importlib.util.spec_from_file_location(name,path);loaded=importlib.util.module_from_spec(spec);spec.loader.exec_module(loaded);return loaded
def extract(root,out):
 root=Path(root);out=Path(out)
 spawn=module(root/'tests/native/script_item_spawn/extract.py','effect_spawn')
 hashes=spawn.extract(root,out,overrides=['nativeScriptBindings_Read','nativeScriptBindings_Write','nativeScriptBindings_Context','nativeScriptBindings_SourceAlive','nativeModelEffect_Resolve','nativeModelEffect_Snapshot','nativeModelEffect_Creator','HSD_JObjAnimAll','nativeScriptBindingKinetics_Read','nativeScriptBindingState_Read','nativeScriptBindingSetup_Read'])
 leaves=(root/'tests/native/script_item_spawn/spawn_leaves.h').read_text()
 leaves=leaves.replace('void* HSD_ObjAlloc(HSD_ObjAllocData* data)\n{','void* HSD_ObjAlloc(HSD_ObjAllocData* data)\n{if(data==&efLib_AllocData)return effect_allocate();if(data==&efAsync_AllocData)return queue_allocate();')
 leaves=leaves.replace('void HSD_ObjFree(HSD_ObjAllocData* data,void* object)\n{','void HSD_ObjFree(HSD_ObjAllocData* data,void* object)\n{if(data==&efLib_AllocData){effect_free(object);return;}if(data==&efAsync_AllocData){queue_free(object);return;}')
 (out/'spawn_leaves.h').write_text(leaves)
 boundary=(out/'boundary.c').read_text().replace('#include "spawn_leaves.h"','#include <melee/it/native_item_pool.h>\n#include <melee/ft/native_fighter_lifetime.h>\n#include "effect_leaves.h"\n#include "spawn_leaves.h"\n#include "actual_effect.inc"')
 for name in ['efAsync_QueueClear','efAsync_QueueFlush']:
  boundary=boundary.replace(spawn.body(boundary,name),'')
 (out/'boundary.c').write_text(boundary+'\nREAD(nativeScriptBindingActions_Read) CALL(nativeScriptBindingActions_Invoke)\n'
  'StKind Stage_80225194(void){abort();}\n')
 paths=[root/'src/melee/ef'/name for name in ['eflib.c','efsync.c','efalt.c','efasync.c']]
 lib,sync,alt,queued=[path.read_text() for path in paths]
 lib=lib.replace('static inline EF_Effect*\n','static inline EF_Effect* ')
 parts=[spawn.body(lib,n) for n in ['efLib_remove_user_data','efLib_Update','efLib_Create','eflib_create_effect_and_attach','efLib_Create_Attach','efLib_Create_Attach_Scale','efLib_Create_Attach_Pos']]
 parts.insert(0,spawn.body(sync,'efSync_ModelRecipe'))
 parts.append(spawn.body(lib,'efLib_Cb_ftMr_SpecialLw').replace('efLib_Cb_ftMr_SpecialLw(','effect_reference_tornado('))
 parts.append(spawn.body(lib,'efLib_Cb_SetRotY_FromFighterDir'))
 # Preserve exact Tornado branch and common dispatch setup. Other gfx recipes
 # remain unavailable in this explicitly scoped fixture, never synthesized.
 alternate=spawn.body(alt,'efAlt_Spawn');start=alternate.index('    case 0x47C:');end=alternate.index('    case 0x47D:',start)
 branch=alternate[start:end];switch=alternate.index('    switch (gfx_id)');tail=alternate.index('    while (efLib_AnimCount != 0)')
 parts.append(alternate[:switch]+'    switch(gfx_id){\n'+branch+'default:break;\n}\n'+alternate[tail:])
 dispatch=spawn.body(sync,'efSync_Spawn');prefix=dispatch[:dispatch.index('    switch (gfx_id)')]
 # Retain only the exact pre-switch dispatcher families. Unused family leaves
 # trap or fail, so the test cannot manufacture another successful recipe.
 pair_begin=dispatch.index('    case 0x4DA:');pair_end=dispatch.index('    case 0x4DB:',pair_begin)
 point_begin=dispatch.index('    case 0x4CC:');point_end=dispatch.index('    case 0x4CD:',point_begin)
 parts.append(prefix+'    switch(gfx_id){\n'+dispatch[pair_begin:pair_end]+dispatch[point_begin:point_end]+'default:break;\n}\n    va_end(vlist);return spawned;\n}')
 parts.extend(spawn.body(queued,n) for n in ['efAsync_QueueProcessDeferred','efAsync_QueueFlush','efAsync_QueueClear'])
 helpers=[]
 for relative,names in [('src/melee/ft/native_script_binding_kinetics.c',['error','nativeScriptBindingKinetics_Read']),('src/melee/ft/native_script_binding_state.c',['nativeScriptBindingState_Read']),('src/melee/ft/native_script_binding_setup.c',['command_index','nativeScriptBindingSetup_Read'])]:
  path=root/relative;text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();helpers.extend(spawn.body(text,n).replace('error(','effect_binding_error(') for n in names)
 parts.extend(helpers)
 # Use the same disabled profiling and trace hooks as the other VM fixtures.
 observers = root/'tests/native/script_graph_observers.c'
 parts.append('#include <sysdolphin/baselib/native_callback_profile.h>\nbool nativeCallbackProfileEnabled;')
 for name in ['nativeCallbackProfile_Begin','nativeCallbackProfile_End','nativeRandom_TraceEvent']:
  parts.append(spawn.body(observers.read_text(),name))
 matrix = root.parent/'aurora/lib/dolphin/mtx/mtx.c'
 # Aurora's implementation accepts a const quaternion. Melee's SDK header
 # declares the same ABI with a mutable pointer.
 parts.append('#include <assert.h>\n' + spawn.body(matrix.read_text(),'C_MTXQuat')
              .replace('const Quaternion* q','QuaternionPtr q'))
 rotations = root/'src/sysdolphin/baselib/quatlib.c'
 for name in ['HSD_QuatLib_8037EB28','EulerToQuat']:
  parts.append(spawn.body(rotations.read_text(),name))
 paths.extend([matrix, rotations, observers])
 (out/'actual_effect.inc').write_text('\n\n'.join(parts)+'\n')
 for path in paths+[root/'tests/native/script_item_spawn/spawn_leaves.h',Path(__file__).resolve()]:hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 return hashes
