"""Actual effect control loops, clear body and source-owned admission."""
import hashlib,importlib.util,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('hitlag_body',HERE.parent/'script_form_feedback/extract.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def extract(root,out):
    hashes={}
    def read(path):
        p=root/path;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
    props=read('src/melee/ft/native_script_binding_properties.c')
    guards='\n'.join(module.body(props,n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
    guards=re.sub(r'\berror\b','hitlag_guard_error',guards)
    boundary=read('tests/native/script_fighter_scheduler/boundary.c')
    boundary=boundary.replace(module.body(boundary,'nativeScriptBindings_SourceAlive'),'')
    boundary=boundary.replace(module.body(boundary,'efLib_DestroyAll'),'')
    boundary=boundary.replace('READ(nativeScriptBindingCombat_Read) WRITE(nativeScriptBindingCombat_Write) CALL(nativeScriptBindingCombat_Invoke)','')
    (out/'boundary.c').write_text('#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n')
    effects=read('src/melee/ef/eflib.c');common=read('src/melee/ft/ftcommon.c')
    bodies=[module.body(effects,n) for n in ['efLib_PauseAll','efLib_ResumeAll','efLib_DestroyAll']]+[module.body(common,'ftCommon_8007DB24')]
    (out/'native_effects.inc').write_text('#define GET_EFFECT(o) ((EF_Effect*)(o)->user_data)\n'+ '\n'.join(bodies)+'\n')
    for name in ['popo_full.fms','nana_full.fms']:read('examples/'+name)
    return hashes
