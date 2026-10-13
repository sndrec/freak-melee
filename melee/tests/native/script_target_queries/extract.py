"""Actual guard/native reference extraction with explicit test observation hook."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path
def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 path=root/'tests/native/script_form_feedback/extract.py';spec=importlib.util.spec_from_file_location('target_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def source(relative):
  path=root/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 text=source('src/melee/ft/native_script_binding_properties.c')
 guards=[re.sub(r'\berror\b','target_guard_error',m.body(text,name)) for name in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']]
 marker=guards[1].index('{')+1;guards[1]=guards[1][:marker]+'\n    target_guard_probe(context);'+guards[1][marker:]
 (out/'source_guards.inc').write_text('\n'.join(guards)+'\n')
 text=source('src/melee/ft/native_fighter_target.c')
 names=['nativeFighterTarget_Position','nativeFighterTarget_PositionDelta','nativeFighterTarget_Airborne','nativeFighterTarget_IsFighter','nativeFighterTarget_Damage','nativeFighterTarget_Facing','nativeFighterTarget_CpuBodyHalfWidth','nativeFighterTarget_Sleeping','nativeFighterTarget_FloorLine']
 native=[m.body(text,name) for name in names]
 reference=subprocess.run(['git','show','50f2b476b:src/melee/ft/native_fighter_target.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 if any(m.body(text,name)!=m.body(reference,name) for name in names):raise RuntimeError('Native target reference body changed')
 ftlib=source('src/melee/ft/ftlib.c');reference=subprocess.run(['git','show','50f2b476b:src/melee/ft/ftlib.c'],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
 for name in ['ftLib_8008741C','ftLib_80086BEC']:
  actual=m.body(ftlib,name)
  if actual!=m.body(reference,name):raise RuntimeError('Native Fighter position/lookup body changed')
  native.insert(0,actual)
 (out/'native_target.inc').write_text('\n'.join(native)+'\n')
 return hashes
