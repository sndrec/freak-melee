"""Mechanical current native bodies and pre-batch independent native oracle."""
import hashlib,importlib.util,re,subprocess
from pathlib import Path
HEAD="7b077ac85f75a68df8f99feaf97fe8298c7609bc"
def extract(root,out):
 root=Path(root);out=Path(out);hashes={}
 def module(name,path):
  spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();return m
 body=module('body',root/'tests/native/script_form_feedback/extract.py').body
 native=module('native',root/'tests/native/script_item_scheduler/extract_scheduler.py').native_only
 def source(path):
  p=root/path;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 props=source('src/melee/ft/native_script_binding_properties.c')
 (out/'source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','air_guard_error',body(props,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])+'\n')
 groups={
 'src/melee/ft/ftcommon.c':['ftCo_GetCStickAngle','ftCo_GetLStickAngle','ftCommon_8007E79C'],
 'src/melee/ft/ft_0DF1.c':['ftCo_800DF478','ftCo_800DF50C'],
 'src/melee/ft/kinds/ftCommon/ftCo_AttackAir.c':['ftCo_AttackAir_GetMsidFromCStick'],
 'src/melee/ft/kinds/ftCommon/ftCo_SpecialAir.c':['ftCo_SpecialAir_CheckInput'],
 'src/melee/ft/ft_0CDD.c':['ftCo_800CDDA0'],
 'src/melee/ft/ft_0CDF.c':['ftCo_800CE010'],
 'src/melee/ft/ft_0CE3.c':['ftCo_800CE480'],
 'src/melee/ft/kinds/ftCommon/ftCo_ItemScopeStart.c':['ft_800D7770'],
 'src/melee/ft/kinds/ftCommon/ftCo_ItemThrow.c':['ftCo_80094E54','ftCo_80095328','ftCo_80095744','getAnimSpeed','ftCo_80095700','ftCo_800957F4'],
 'src/melee/it/item.c':['itemCallbackCurrent','Item_8026ABD8'],
 'src/melee/it/it_26B1.c':['itGetUseKind','itGetDamageMultiplier','it_8026B594','it_8026B6C8'],
 'src/melee/it/it_2725.c':['it_802731A4'],
 'src/melee/it/itidentity.c':['itGetKind'],
 'src/melee/it/ithitbox.c':['it_802754D4'],
 }
 current=[];old=[];rename={n:'reference_'+n for names in groups.values() for n in names}
 for path,names in groups.items():
  current.extend(body(native(source(path)),n) for n in names)
  text=subprocess.run(['git','show',HEAD+':'+path],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  # Keep retail callback path in reference, independent of interpreter routing.
  text=text.replace('MELEE_NATIVE','AIR_REFERENCE_NATIVE_DISABLED')
  for n in names:
   s=body(text,n)
   s=re.sub(r'\b('+'|'.join(map(re.escape,rename))+r')\b',lambda m:rename[m[0]],s)
   old.append(s)
 def write(path,pieces):
  prototypes='\n'.join(x[:x.index('{')].rstrip()+';' for x in pieces)
  path.write_text('#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wunused-variable"\n#pragma clang diagnostic ignored "-Wunused-function"\n'+prototypes+'\n'+'\n'.join(pieces)+'\n#pragma clang diagnostic pop\n')
 write(out/'native_air.inc',current);write(out/'reference_air.inc',old)
 return hashes
