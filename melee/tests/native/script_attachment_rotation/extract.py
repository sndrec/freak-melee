"""Exact native creator bootstrap, class admission and existing-matrix math."""
import hashlib,importlib.util,re,subprocess
def extract(root,out):
 hashes={};path=root/'tests/native/script_form_feedback/extract.py'
 spec=importlib.util.spec_from_file_location('attachment_body',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 def read(name):
  path=root/name;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return path.read_text()
 properties=read('src/melee/ft/native_script_binding_properties.c')
 (out/'source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','attachment_guard_error',m.body(properties,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])+'\n')
 bodies=[]
 for path,names in [('src/melee/it/native_item_owner.c',['nativeItem_CaptureCreator','nativeItem_Creator']),('src/sysdolphin/baselib/class.c',['hsdObjIsDescendantOf']),('src/sysdolphin/baselib/mtx.c',['calcVal','HSD_MtxGetRotation','HSD_MkRotationMtx'])]:
  text=read(path)
  reference=subprocess.run(['git','show','948cf3ed5:'+path],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for name in names:
   actual=m.body(text,name)
   if actual!=m.body(reference,name):raise RuntimeError('Native matrix/bootstrap body changed: '+name)
   bodies.append(actual)
 (out/'native_bodies.c').write_text('#include "fixture.h"\n#include <melee/it/native_item_owner.h>\n#include <melee/ft/native_callback_lifetime.h>\n#include <melee/ft/native_fighter_runtime.h>\n#include <math.h>\n#define FLOAT_MIN 1.1754943E-38f\n'+'\n'.join(bodies)+'\n')
 return hashes
