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
    common=[m.body(p,name) for name in ['itemCallbackCurrent','Item_8026AB54','Item_8026B074','Item_8026ABD8']]
    common += [m.body(source('src/melee/it/it_26B1.c'),'it_8026B3A8')]
    p=source('src/melee/it/it_2725.c')
    common += [m.body(p,name) for name in ['it_80273168','it_802731A4','it_802742F4','it_80274C88','HSD_JObjSetScale_2','it_80274594','it_80274DFC','it_80274E44']]
    common += [m.body(n.native_only(source('src/melee/ft/ftcommon.c')),'ftCommon_8007E6DC')]
    common += [m.body(source('src/melee/it/it_26B1.c'),name) for name in ['itGetDamageMultiplier','it_8026B724','it_8026B73C','it_8026BB20','it_8026BB44','itGetUseKind','itGetHoldKind','itIsHeavy']]
    common += [m.body(source('src/melee/it/it_2725.c'),name) for name in ['it_80272A18','it_80272A3C']]
    common += [m.body(source('src/sysdolphin/baselib/jobj.c'),name) for name in ['HSD_JObjSetFlags','HSD_JObjSetFlagsAll','HSD_JObjClearFlags','HSD_JObjClearFlagsAll']]
    p=source('src/melee/it/native_item_owner.c')
    common += [m.body(p,name) for name in ['nativeItem_CaptureCreator','nativeItem_Creator']]
    common += [m.body(source('src/melee/ft/ftlib.c'),'ftLib_80086A0C')]
    common += [m.body(source('src/melee/it/ithitbox.c'),'it_80275534')]
    p=n.native_only(source('src/melee/ft/kinds/ftCommon/ftpickupitem.c'))
    common += [m.body(p,'ftpickupitem_80094818')]
    p=n.native_only(source('src/melee/ft/kinds/ftCommon/ftCo_ItemThrow.c'))
    common += [m.body(p,name) for name in ['ftCo_80095700','getAnimSpeed','ftCo_800957F4']]
    p=n.native_only(source('src/melee/it/item.c'))
    common += [m.body(p,name) for name in ['Item_802674AC','Item_CreateEngine','Item_80268B18']]
    common += [m.body(source('src/melee/it/native_item_behavior.c'),'nativeItemBehavior_EngineAssetsAvailable')]
    common += [m.body(source('src/melee/it/kinds/itfoods.c'),'it_8028FAF4')]
    common += [m.body(source('src/melee/it/itspawn.c'),'it_8026D324')]
    common += [m.body(source('src/melee/it/it_2725.c'),'it_80274ED8')]
    common += [m.body(source('src/melee/it/it_26B1.c'),'it_8026BB68')]
    common += [m.body(source('src/melee/it/itmaplib.c'),name) for name in ['it_802762B0','it_802762BC']]
    common += [m.body(source('src/melee/ft/ftlib.c'),name) for name in ['vector_add','ftLib_80086990']]
    getter=m.body(source('src/melee/ft/ftlib.c'),'ftLib_GetItem')
    if getter.count('return fp->item_gobj;')!=1:raise RuntimeError('native held slot getter changed')
    getter=getter.replace('return fp->item_gobj;','HSD_GObj* selected=fp->item_gobj; held_after_capture(gobj); return selected;')
    common += [getter,m.body(source('src/melee/ft/ftlib.c'),'ftLib_8008741C')]
    common += [m.body(source('src/melee/ft/native_fighter_target.c'),name) for name in ['current_held_item','nativeFighterTarget_HeldId','nativeFighterTarget_HeldKind','nativeFighterTarget_HeldMotion','nativeFighterTarget_HeldFrame']]
    prototypes='\n'.join(x[:x.index('{')].rstrip()+';' for x in common)
    (out/'native_pickup.inc').write_text(prototypes+'\n'+'\n'.join(common)+'\n')
    return hashes
