"""Extract native Item capsule setters, frame kernels and exact scale/stale math."""
import hashlib,importlib.util,re
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 path=root/'tests/native/script_form_feedback/extract.py';spec=importlib.util.spec_from_file_location('combat_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def source(relative):
  path=root/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 text=source('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','combat_guard_error',m.body(text,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 text=source('src/melee/it/itcoll.c');bodies=[m.body(text,name) for name in ['it_8027129C','it_80272460']]
 # Explicit completed-native-call observation. The exact setter statements
 # finish before injected mutation; no subsequent native write uses freed data.
 setter=bodies[1];setter=setter.replace('        return;','        combat_native_completed(arg_item_gobj);\n        return;')
 setter=setter[:-1]+'    combat_native_completed(arg_item_gobj);\n}';bodies[1]=setter
 text=source('src/melee/it/it_2725.c');bodies += [m.body(text,name) for name in ['it_8027129C_by_4','it_80272560','it_802725D4']]
 text=source('src/melee/ft/ft_0881.c');bodies += [m.body(text,name) for name in ['ft_80089118','ft_80089228']]
 bodies += [m.body(source('src/melee/ft/ftchangeparam.c'),'ftCo_CalcYScaledKnockback')]
 bodies += [m.body(source('src/melee/ft/ftidentity.c'),'ftLib_80086960')]
 bodies += [m.body(source('src/melee/lb/lbcollision.c'),'lbColl_80008428')]
 prototypes='\n'.join(body[:body.index('{')].rstrip()+';' for body in bodies)
 (out/'native_combat.inc').write_text(prototypes+'\n'+'\n'.join(bodies)+'\n')
 return hashes
