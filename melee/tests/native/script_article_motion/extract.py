"""Native debug/collision bodies with explicit completed-operation test probes."""
import hashlib,importlib.util,re
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 path=root/'tests/native/script_form_feedback/extract.py';spec=importlib.util.spec_from_file_location('motion_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def source(relative):
  path=root/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 text=source('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','motion_guard_error',m.body(text,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout','property','vector_component','nativeScriptBindings_Read']]
 marker=guards[1].index('{')+1;guards[1]=guards[1][:marker]+'\n    motion_read_probe(context);'+guards[1][marker:]
 includes=re.sub(r'#include "([^"]+)"',r'#include <melee/ft/\1>',text[:text.index('static bool error')])
 (out/'source_guards.inc').write_text(includes+'\n'.join(guards)+'\n')
 debug=m.body(source('src/melee/db/dbitem.c'),'db_80225DD8')
 debug=debug[:-1]+'    motion_native_completed(item);\n}'
 bodies=[m.body(source('src/melee/ft/ftidentity.c'),'ftLib_80086960'),m.body(source('src/melee/db/dbanim.c'),'fn_8022697C'),debug]
 text=source('src/melee/it/itgroundcoll.c')
 bodies += [m.body(text,'it_8026D9A0').replace('return temp;','motion_native_completed(gobj);\n        return temp;'),m.body(text,'it_8026DA08').replace('return test;','motion_native_completed(gobj);\n    return test;')]
 (out/'native_motion.inc').write_text('\n'.join(b[:b.index('{')].rstrip()+';' for b in bodies)+'\n'+'\n'.join(bodies)+'\n')
 target=source('src/melee/ft/native_fighter_target.c');library=source('src/melee/ft/ftlib.c');item=source('src/melee/it/item.c')
 reflected=[m.body(library,'ftLib_80087460'),m.body(library,'ftLib_80086EB4'),m.body(target,'nativeFighterTarget_Id'),m.body(target,'nativeFighterTarget_Reflector'),m.body(item,'itemCallbackCurrent'),m.body(item,'processSourceCallback'),m.body(item,'Item_80269F14').replace('Item_80269F14','motion_reflected_entry').replace('static bool motion_reflected_entry','bool motion_reflected_entry')]
 (out/'native_reflected.inc').write_text('\n'.join(b[:b.index('{')].rstrip()+';' for b in reflected)+'\n'+'\n'.join(reflected)+'\n')
 return hashes
