"""Mechanically extract the actual color/presentation/demo/taunt schedulers."""
import hashlib,importlib.util,sys,subprocess
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
s=importlib.util.spec_from_file_location('native_bodies',HERE.parent/'script_item_scheduler/extract_scheduler.py')
reference=importlib.util.module_from_spec(s);s.loader.exec_module(reference)
FUNCTIONS={
 'src/melee/ft/ftcolanim.c':['ftCo_800C0134','ftCo_800C0200','ftCo_800C0408'],
 'src/melee/ft/ft_0BEC.c':['setupInitialState','FoxHelper','DocHelper','ftCo_800BECB0','ftCo_800BED88','ftCo_800BEF04'],
 'src/melee/ft/ft_0BF0.c':['ftCo_800BF034','ftCo_800BF108'],
 'src/melee/ft/ftdemo.c':['initFighter','ftDemo_CreateFighter'],
 'src/melee/ft/kinds/ftCommon/ftCo_AppealS.c':['ftCo_800DEA28','ftCo_800DEAE8','ftCo_800DEBD0'],
 'src/melee/ft/ftwaitanim.c':['inlineA0','select_wait_animation','ftCo_8008A7A8'],
 'src/melee/ft/ft_0892.c':['ft_80089914'],
 'src/melee/ft/kinds/ftCommon/ftCo_ItemScrew.c':['ft_800D2E7C'],
 'src/melee/ft/kinds/ftCommon/ftCo_JumpAerialF1.c':['ftCo_800D74A4_inline','ftCo_800D74A4'],
}
def extract(root,out):
 bodies=[];hashes={}
 for name,functions in FUNCTIONS.items():
  path=root/name;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();source=reference.native_only(path.read_text())
  for fn in functions:bodies.append(reference.reference.body(source,fn))
 prototypes=[body[:body.index('{')].rstrip()+';' for body in bodies]
 context=root/'src/melee/ft/native_script_binding_properties.c';hashes[str(context)]=hashlib.sha256(context.read_bytes()).hexdigest()
 text=reference.native_only(context.read_text());context_bodies=[reference.reference.body(text,n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_Context']]
 (out.parent/'actual_context.h').write_text('// Actual native context/source guards.\n'+'\n\n'.join(context_bodies)+'\n')
 demo=(root/'src/melee/ft/ftdemo.c').read_text();first=demo.index('static HSD_GObjEvent on_create_fighter[16]');last=demo.index('};',first)+2
 out.write_text('// Actual source bodies; only engine leaves are controlled.\n'+'\n'.join(prototypes)+'\n'+demo[first:last]+'\n'+'\n\n'.join(bodies)+'\n')
 baseline=[]
 for path,fn in [('src/melee/ft/ftwaitanim.c','ftCo_8008A7A8'),('src/melee/ft/ft_0892.c','ft_80089914'),('src/melee/ft/kinds/ftCommon/ftCo_ItemScrew.c','ft_800D2E7C')]:
  text=subprocess.run(['git','show','5a9b7e468:'+path],cwd=root,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  body=reference.reference.body(reference.native_only(text),fn).replace(fn+'(', 'reference_'+fn+'(',1)
  baseline.append(body)
 reference_path=out.parent/'reference_common_policies.h';reference_path.write_text('\n\n'.join(baseline)+'\n')
 hashes[str(reference_path)]=hashlib.sha256(reference_path.read_bytes()).hexdigest()
 return hashes
