"""Actual source guards and common facing-aligned native callback body."""
import hashlib,importlib.util,subprocess

REFERENCE='37c81e872'

def module(path,name):
    spec=importlib.util.spec_from_file_location(name,path)
    value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value)
    return value

def extract(root,out):
    path=root/'tests/native/script_target_queries/extract.py'
    hashes=module(path,'target_guards').extract(root,out)
    hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    extractor=root/'tests/native/script_form_feedback/extract.py'
    body=module(extractor,'native_body').body
    path=root/'src/melee/ft/ftcommon.c'
    hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    (out/'native_velocity.inc').write_text(body(path.read_text(),'ftCommon_8007F76C')+'\n')
    path=root/'tests/native/script_target_queries/boundary.c'
    hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    text=path.read_text().replace('CALL(nativeScriptBindingSetup_Invoke)','')
    for name in ['nativeScriptBindingSetup_Read','nativeScriptBindingSetup_Write']:
        text=text.replace(body(text,name),'')
    text=text.replace('#include "fixture.h"','#include "fixture.h"\n#include <melee/ft/ftcommon.h>\n#include <melee/ft/inlines.h>')
    path=root/'src/melee/ft/fighter.c'
    hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    current=body(path.read_text(),'Fighter_CallAcessoryCallbacks_8006C624')
    reference=subprocess.run(['git','show',REFERENCE+':src/melee/ft/fighter.c'],cwd=root,
        capture_output=True,text=True,check=True).stdout
    reference=body(reference,'Fighter_CallAcessoryCallbacks_8006C624').replace(
        'Fighter_CallAcessoryCallbacks_8006C624','reference_accessory_scheduler')
    (out/'native_accessory.inc').write_text(current+'\n'+reference+'\n')
    (out/'boundary.c').write_text(text+'\n#include <melee/ft/native_fighter_lifetime.h>\n#include <sysdolphin/baselib/jobj.h>\n#include "native_velocity.inc"\n#include "native_accessory.inc"\n')
    return hashes
