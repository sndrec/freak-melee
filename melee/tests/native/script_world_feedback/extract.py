"""Actual native quake/rumble paths; only rendering/data/hardware leaves controlled."""
import hashlib,importlib.util,re
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 path=root/'tests/native/script_form_feedback/extract.py';spec=importlib.util.spec_from_file_location('feedback_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def source(relative):
  path=root/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 text=source('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','feedback_guard_error',m.body(text,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 camera=source('src/melee/cm/camera.c');request=m.body(camera,'Camera_RequestQuake');request=request[:-1]+'    feedback_completed();\n}'
 bodies=[m.body(camera,n) for n in ['Camera_GetState','Camera_SetQuakeOffset']]+[request]
 ground=source('src/melee/gr/grlib.c');bodies += [m.body(ground,n) for n in ['grLib_801C9BC8','grLib_801C9C40','grLib_801C9CEC']]
 player=source('src/melee/pl/player.c');bodies += [m.body(player,n) for n in ['Player_CheckSlot','Player_8003544C']]
 fighter=source('src/melee/ft/ftlib.c');bodies += [m.body(fighter,n) for n in ['helper','ftLib_80086D40']]
 bodies += [m.body(source('src/melee/ft/ftidentity.c'),'ftLib_80086960'),m.body(source('src/melee/it/it_2725.c'),'it_80273598'),m.body(source('src/melee/lb/lb_013B.c'),'lb_80014574')]
 bodies.append(m.body(source('src/melee/ft/ftcommon.c'),'ftCommon_8007EBAC'))
 (out/'native_feedback.inc').write_text('\n'.join(b[:b.index('{')].rstrip()+';' for b in bodies)+'\n'+'\n'.join(bodies)+'\n')
 return hashes
