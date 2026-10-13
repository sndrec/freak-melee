"""Owned generic visuals/color native comparison, scoped console only."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('source_feedback_contract',HERE.parent/'script_form_feedback/extract.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: archive required')
 out.mkdir(parents=True);hashes=extract.extract(ROOT,out);pieces=[]
 for relative,name in [('src/melee/lb/lb_013B.c','lb_800144C8'),('src/melee/lb/lb_013B.c','lb_80014498'),('src/melee/ft/ftcolanim.c','ftCo_800BFFAC'),('src/sysdolphin/baselib/dobj.c','HSD_DObjSetFlags'),('src/melee/ft/ftparts.c','ftParts_80074D7C'),('src/melee/ft/ftparts.c','ftParts_80074A4C'),('src/melee/ft/ftparts.c','ftParts_80074B0C'),('src/melee/ft/ftparts.c','ftParts_80074A8C'),('src/melee/ft/native_fighter_visuals.c','nativeFighterVisuals_CurrentGroup'),('src/melee/ft/native_fighter_visuals.c','nativeFighterVisuals_PreviousGroup'),('src/melee/ft/ftcolanim.c','ftCo_800BFFD0')]:
  path=ROOT/relative;source=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();body=extract.body(source,name)
  if name in ('ftParts_80074D7C','ftParts_80074A8C'):body='#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wsign-compare"\n'+body+'\n#pragma clang diagnostic pop\n'
  if name in ('ftParts_80074A4C','ftParts_80074B0C'):body=body[:-1]+'after_native();\n}'
  if name=='ftCo_800BFFAC':body=body[:-1]+'after_native();\n}'
  if name=='ftCo_800BFFD0':body=body.replace('return true;','after_native(); return true;').replace('return false;','after_native(); return false;')
  pieces.append(body)
 (out/'native_visuals.inc').write_text('\n\n'.join(pieces)+'\n')
 for relative,names in [('src/melee/ft/native_fighter_visuals.c',['nativeFighterVisuals_SetGroupMaterialFrame','nativeFighterVisuals_InitializeGroupMaterialAnimation']),('src/sysdolphin/baselib/aobj.c',['HSD_AObjSetRate'])]:
  path=ROOT/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  with (out/'native_visuals.inc').open('a') as target:target.write('#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wsign-compare"\n'+'\n'.join(extract.body(path.read_text(),name) for name in names)+'\n#pragma clang diagnostic pop\n')
 units=[ROOT/'src/melee/ft/native_script_binding_item_resources_catalog.c', ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c', HERE/'test.c',ROOT/'src/melee/ft/native_script_binding_visuals.c',ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/gm/native_match_pool.c']
 inputs=units+[HERE/'run.py',HERE/'README.md',HERE.parent/'script_form_feedback/extract.py',a.compiler,out/'source_contract.inc',out/'native_visuals.inc']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs})
 command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
 commands=[]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-8000:])
 for path,digest in hashes.items():
  if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,'actual':['visuals native binding','shared ownership guard/catalog extraction','match pool and callback lifetime checks','saved/current parts setters, channel0 invalidator, DObj flag setter, reset and native current/previous queries','color selection/priority helper original native statements'],'controlled':['owned package/view/storage/rig and model variant provider','native common color table inputs and imported model arena data','test-only injection immediately before native primitive return'],'limits':['no DAT import, frontend/VM marshalling, retained receiver invocation or game scene in this fixture','original parts/color selection functions have no script callbacks; reentry is injected only to check binding continuation','no rendering or color-command playback']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
