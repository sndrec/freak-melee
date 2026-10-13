"""Mechanical native family query and actual source admission extraction."""
import hashlib, importlib.util, re
from pathlib import Path
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('family_body', HERE.parent/'script_form_feedback/extract.py')
module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)

def extract(root, out):
    hashes = {}
    def read(name):
        p = root/name; hashes[str(p)] = hashlib.sha256(p.read_bytes()).hexdigest()
        return p.read_text()
    props = read('src/melee/ft/native_script_binding_properties.c')
    guards = '\n'.join(module.body(props, n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
    guards = re.sub(r'\berror\b', 'family_guard_error', guards)
    guards = guards.replace('bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* context)\n{',
        'void family_guard_observe(void);\nbool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* context)\n{\n    family_guard_observe();')
    boundary = read('tests/native/script_fighter_scheduler/boundary.c')
    boundary = boundary.replace(module.body(boundary,'nativeScriptBindings_SourceAlive'),'')
    (out/'boundary.c').write_text('#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n')
    runtime = read('src/melee/ft/native_fighter_runtime.c')
    records=runtime[runtime.index('typedef struct NativeFighterFamily NativeFighterFamily;'):runtime.index('static unsigned staged_count;')]
    constants=runtime[runtime.index('enum {'):runtime.index('};',runtime.index('enum {'))+2]
    identity=runtime[runtime.index('typedef struct NativeFamilyMemberIdentity {'):runtime.index('} NativeFamilyMemberIdentity;')+len('} NativeFamilyMemberIdentity;')]
    names=['nativeFighterRuntime_SourcePackage','family_member_slot','family_member_current','family_member_capture','family_member_published','family_member_id','nativeFighterRuntime_MemberId','nativeFighterRuntime_CpuHistorySource']
    (out/'native_family.inc').write_text(constants+'\n'+records+identity+'\n'+'\n'.join(module.body(runtime,n) for n in names)+'\n')
    player=read('src/melee/pl/player.c')
    (out/'native_player.inc').write_text('\n'.join(module.body(player,n) for n in ['Player_GetPlayerCharacter','Player_GetEntityAtIndex','Player_SwapTransformedStates'])+'\n')
    common=read('src/melee/ft/kinds/ftCommon/ftCo_0A01.c')
    attack=read('src/melee/ft/ftcpuattack.c');library=read('src/melee/ft/ftlib.c')
    (out/'native_initializer.inc').write_text(module.body(library,'ftLib_8008741C')+'\n'+module.body(attack,'ftCo_800B9704')+'\n'+module.body(common,'ftCo_800A101C')+'\n')
    boundary=boundary.replace(module.body(boundary,'ftCo_800A0FB0'),module.body(boundary,'ftCo_800A0FB0').replace('abort();','return false;'))
    (out/'boundary.c').write_text('#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n')
    with (out/'boundary.c').open('a') as f:f.write('\nvoid ftCommon_8007F76C(Fighter_GObj* o){(void)o;abort();}\n')
    for path in ['tools/native_fighter_script_relations.c','tools/native_fighter_script_primitives.c','src/melee/ft/ftcolanim.c','src/melee/ft/fighter.c','src/melee/ft/kinds/ftPopo/ftpopo.c','examples/popo_full.fms','examples/nana_full.fms']:
        read(path)
    return hashes
