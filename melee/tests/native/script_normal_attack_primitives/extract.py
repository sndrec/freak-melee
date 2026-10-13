"""Current substantive kernels/guards plus frozen native completion reference."""
import hashlib,importlib.util,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'script_fighter_attack_scheduler'
spec=importlib.util.spec_from_file_location('normal_attack_reference',BASE/'extract.py')
reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)

def extract(root,out):
    reference.FUNCTIONS['Attack100'].append('ftCo_RapidJab_ShouldFinish')
    hashes=reference.extract(root,out)
    props=root/'src/melee/ft/native_script_binding_properties.c'
    collision=root/'src/melee/ft/ft_081B.c'
    text=reference.reference.reference.native_only(collision.read_text())
    native=reference.body(text,'ft_80081D0C')+'\n'+reference.body(text,'ft_80082C74')
    (out/'actual_attack.h').write_text((out/'actual_attack.h').read_text()+'\n'+native+'\n')
    frozen=root/'.local/baseline72/sdk/src/melee/ft/kinds/ftCommon/ftCo_Attack100.c'
    frozen_body=reference.body(reference.reference.reference.native_only(frozen.read_text()),'ftCo_800D6C60')
    (out/'actual_attack.h').write_text((out/'actual_attack.h').read_text()+'\n'+frozen_body.replace('ftCo_800D6C60','frozen_rapid_update')+'\n')
    for path in [props,collision,frozen]:hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    path=out/'boundary.c';boundary=path.read_text()
    for name in ['nativeScriptBindings_Context','nativeScriptBindings_SourceAlive']:
        value=reference.body(boundary,name);boundary=boundary.replace(value,'')
    guard_source=props.read_text()
    guards='\n'.join(re.sub(r'\berror\b','normal_guard_error',reference.body(guard_source,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout','nativeScriptBindings_Context'])
    boundary='#include <melee/ft/ft_0819.h>\n#include <melee/ft/native_source_package.h>\n'+boundary+'\n'+guards+'\n'
    path.write_text(boundary)
    return hashes
