"""Exact common Fighter hitbox math and selected-source guard bodies."""
import hashlib,importlib.util,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('body',HERE.parent/'script_form_feedback/extract.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def extract(root,out):
 hashes={}
 def read(path):
  p=root/path;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 props=read('src/melee/ft/native_script_binding_properties.c')
 guards='\n'.join(module.body(props,n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])
 guards=re.sub(r'\berror\b','hitbox_guard_error',guards)
 boundary=read('tests/native/script_fighter_scheduler/boundary.c')
 boundary=boundary.replace(module.body(boundary,'nativeScriptBindings_SourceAlive'),'')
 (out/'boundary.c').write_text('#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n')
 bodies=[]
 for path,names in [('src/melee/ft/ftchangeparam.c',['ftCo_CalcYScaledKnockback']),
                    ('src/melee/ft/ft_0DF0.c',['ftCo_800DEEB8']),
                    ('src/melee/ft/ft_0881.c',['ft_80089118','ft_80089228']),
                    ('src/melee/ft/ftcoll.c',['ftColl_8007ABD0','ftColl_8007B8A8','ftColl_8007AFF8']),
                    ('src/melee/lb/lbcollision.c',['lbColl_80008428','lbColl_80008434','lbColl_80008440'])]:
  text=read(path);bodies.extend(module.body(text,n) for n in names)
 (out/'native_hitbox.inc').write_text('#include <melee/lb/lbcollision.h>\n'+'\n\n'.join(bodies)+'\n')
 return hashes
