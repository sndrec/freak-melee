"""Pinned PPC-offset oracle versus actual widened native Scope bodies."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
REFERENCE='2b3d0253b4a5b5503b6516303dca492646ba9606'
spec=importlib.util.spec_from_file_location('extract_body',HERE.parent/'script_form_feedback/extract.py')
extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest();hashes={};parts=[];actual=[]
 for file,names in [('ftCo_ItemScopeStart.c',['fn_800D79B4','fn_800D7938','ft_800D76B8']),('ftCo_ItemScopeRapid.c',['ftCo_ItemScopeRapid_IASA','ftCo_ItemScopeAirRapid_IASA','fn_800D7BDC','fn_800D7C60','ftCo_ItemScopeRapid_Anim','ftCo_ItemScopeAirRapid_Anim']),('ftCo_ItemScopeFire.c',['fn_800D8140','fn_800D81D0'])]:
  relative='src/melee/ft/kinds/ftCommon/'+file
  current_path=ROOT/relative;current=current_path.read_text();hashes[str(current_path)]=sha(current_path)
  actual.extend(extractor.body(current,name) for name in names)
  old=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
  if file.endswith('Start.c'):parts.append(old[old.index('typedef struct {'):old.index('FtMotionId fn_800D769C')])
  for name in names:
   text=extractor.body(old,name);text=text.replace(name+'(', 'reference_'+name+'(',1);parts.append(text)
  (out/(file+'.reference')).write_text(old)
 for file,names in [('ftCo_ItemScopeStart.c',['fn_800D769C']),('ftCo_ItemScopeFire.c',['fn_800D8378','fn_800D86B8']),('ftCo_ItemScopeRapid.c',['fn_800D80F4']),('ftCo_ItemScope.c',['fn_800D86E0','fn_800D874C'])]:
  current_path=ROOT/('src/melee/ft/kinds/ftCommon/'+file);hashes[str(current_path)]=sha(current_path)
  actual.extend(extractor.body(current_path.read_text(),name) for name in names)
 (out/'actual.inc').write_text('\n'.join(actual))
 it_path=ROOT/'src/melee/it/kinds/itsscope.c';it=it_path.read_text();hashes[str(it_path)]=sha(it_path)
 (out/'item_charge.inc').write_text('\n'.join(extractor.body(it,n) for n in ['it_80291CF4','it_80291D38','it_80291DAC_level','it_80291DAC','it_80291F14']))
 (out/'reference.inc').write_text('\n'.join(parts))
 production=[ROOT/('src/melee/ft/kinds/ftCommon/'+n+'.c') for n in ['ftCo_ItemScopeStart','ftCo_ItemScopeRapid','ftCo_ItemScopeFire','ftCo_ItemScope']]
 units=[HERE/'test.c']
 inputs=units+[HERE/'run.py',HERE/'README.md',HERE.parent/'script_form_feedback/extract.py',a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+list(out.glob('*.inc'))+list(out.glob('*.reference'));hashes.update({str(f):sha(f) for f in inputs})
 command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-unused-parameter','-Wno-unused-variable','-Wno-return-type','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')];commands=[]
 syntax=[str(a.compiler),'-std=c11','-fsyntax-only','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-unused-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,production)]
 for stage,cmd in [('production-syntax',syntax),('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+':\n'+(r.stdout+r.stderr)[-8000:])
 for f,d in hashes.items():
  if sha(Path(f))!=d:raise RuntimeError('Input changed: '+f)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'native_reference':REFERENCE,'source_sha256':hashes,'commands':commands,'scope':['strict full-unit native syntax plus mechanically extracted actual Scope control/entry/End bodies and Item charge/ammunition kernels','unchanged reference bodies with original PPC offset views; widened actual pointer layout and whole-Fighter canaries','controlled motion/animation/input/presentation leaves and trace order'],'limits':['no real scene/Actor/VM, animation command interpreter, collision or rendered Scope acceptance','reference offset words are deliberately written/read in private byte image; never used as native production layout','retirement/lifetime orchestration remains outside this layout correction','existing Item charge kernel undefined fallthrough inputs excluded; original return-type warning suppressed only in fixture']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()

