"""Actual selected generic primitives with immutable pre-cohort native oracles."""
import hashlib,importlib.util,re,subprocess
REFERENCE='1e9849431'
SELECTION={
 'src/melee/ft/ftcommon.c':['ftCommon_8007E2F4'],
 'src/melee/ft/native_fighter_geometry.c':['nativeFighterGeometry_ProbeLedge'],
 'src/melee/ft/ft_081B.c':['ft_80082E3C'],
 'src/melee/ft/kinds/ftCommon/ftCo_ItemParasolDamageFall.c':['ftCo_800CF4DC'],
 'src/melee/ft/kinds/ftCommon/ftCo_DamageFall.c':['ftCo_80090780'],
 'src/melee/ft/kinds/ftCommon/ftCo_CliffJump.c':['ftCo_8009B390']}
def module(path,name):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
def extract(root,out):
 path=root/'tests/native/script_target_queries/extract.py';hashes=module(path,'tether_guards').extract(root,out);hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 path=root/'tests/native/script_form_feedback/extract.py';body=module(path,'tether_body').body;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 path=root/'tests/native/script_target_queries/boundary.c';text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 text=text.replace('CALL(nativeScriptBindingState_Invoke)','').replace('READ(nativeScriptBindingState_Read)','')
 text=text.replace(body(text,'nativeFighterRuntime_SourcePackage'),'const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k){return k==target_package.target?&target_package:NULL;}')
 (out/'boundary.c').write_text(text)
 names=[n for group in SELECTION.values() for n in group];rename=lambda s:re.sub(r'\b('+'|'.join(names)+r')\b',lambda m:'reference_'+m[0],s)
 actual=[];reference=[]
 for relative,functions in SELECTION.items():
  path=root/relative;source=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  old=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  for name in functions:actual.append(body(source,name));reference.append(rename(body(old,name)))
 headers=['melee/ft/fighter.h','melee/ft/native_fighter_lifetime.h','melee/ft/native_fighter_geometry.h','melee/ft/ftcommon.h','melee/ft/ftparts.h','melee/ft/ftanim.h','melee/ft/ft_081B.h','melee/ft/inlines.h','melee/ft/kinds/ftCommon/ftCo_DamageFall.h','melee/ft/kinds/ftCommon/ftCo_CliffJump.h','melee/ft/kinds/ftCommon/ftCo_ItemParasolDamageFall.h','melee/ft/kinds/ftCommon/forward.h','melee/mp/mpcoll.h','melee/mp/mplib.h','sysdolphin/baselib/gobjplink.h','math.h']
 declarations=[s[:s.index('{')].strip()+';' for s in actual+reference]
 (out/'kernels.h').write_text('\n'.join('#include <'+h+'>' for h in headers)+'\n'+'\n'.join(declarations)+'\n')
 (out/'kernels.c').write_text('#include "kernels.h"\n'+'\n'.join(actual+reference)+'\n')
 return hashes
