"""Mechanical actual source-body extraction; no authored behavior fallback."""
import hashlib,importlib.util,re
from pathlib import Path
def extract(root,out):
    root=Path(root);out=Path(out);hashes={}
    path=root/'tests/native/script_form_feedback/extract.py'
    spec=importlib.util.spec_from_file_location('native_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    path=root/'tests/native/script_item_scheduler/extract_scheduler.py'
    spec=importlib.util.spec_from_file_location('native_preprocess',path);n=importlib.util.module_from_spec(spec);spec.loader.exec_module(n)
    hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    def source(relative):
        p=root/relative;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
    p=source('src/melee/ft/native_script_binding_properties.c')
    guards=[re.sub(r'\berror\b','lifecycle_guard_error',m.body(p,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
    (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
    p=source('src/melee/ft/native_script_binding_item.c')
    fields=[re.sub(r'\berror\b','item_property_error',m.body(p,name)) for name in ['error','holdable','nativeScriptBindingItem_Read','nativeScriptBindingItem_Write']]
    (out/'item_properties.inc').write_text('\n'.join(fields)+'\n')
    p=n.native_only(source('src/melee/it/item.c'))
    common=[m.body(p,name) for name in ['itemCallbackCurrent','Item_8026AB54','Item_8026B074']]
    common += [m.body(source('src/melee/it/it_26B1.c'),'it_8026B3A8')]
    p=source('src/melee/it/it_2725.c')
    common += [m.body(p,name) for name in ['it_80273168','it_802742F4','it_80274C88']]
    p=source('src/melee/it/native_item_owner.c')
    common += [m.body(p,name) for name in ['nativeItem_CaptureCreator','nativeItem_Creator']]
    prototypes='\n'.join(x[:x.index('{')].rstrip()+';' for x in common)
    (out/'native_pickup.inc').write_text(prototypes+'\n'+'\n'.join(common)+'\n')
    return hashes
