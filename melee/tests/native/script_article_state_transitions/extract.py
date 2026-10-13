"""Reuse actual native Item bodies; control only unselected engine leaves."""
import hashlib, importlib.util
from pathlib import Path

def extract(root,output):
 base=root/'tests/native/script_item_scheduler'
 spec=importlib.util.spec_from_file_location('actual_item_scheduler',base/'extract_scheduler.py')
 module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 hashes=module.extract(root,output)
 body=module.reference.body
 source=(base/'boundary.c').read_text()
 source=source.replace(body(source,'efAsync_QueueFlush'),'void transition_stage(HSD_GObj*);\nvoid efAsync_QueueFlush(HSD_GObj* o,void* q){(void)q;transition_stage(o);}')
 source=source.replace(body(source,'Item_80268D34'),'void Item_80268D34(HSD_GObj* o,ItemStateDesc* descriptor){(void)descriptor;transition_animation(o);}')
 source=source.replace('#include "boundary.h"','#include "boundary.h"\nvoid transition_animation(HSD_GObj*);')
 for name in ['nativeScriptBindings_SourceAlive','nativeScriptBindings_Context']:
  source=source.replace(body(source,name),'')
 properties=(root/'src/melee/ft/native_script_binding_properties.c').read_text()
 source+='\n'+body(properties,'error')+'\n'+body(properties,'nativeScriptBindings_SourceAlive')+'\n'+body(properties,'nativeScriptBindings_Context')+'\n'
 (output.parent/'boundary.c').write_text(source)
 for path in [base/'boundary.c',root/'src/melee/ft/native_script_binding_properties.c']:
  hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 return hashes
