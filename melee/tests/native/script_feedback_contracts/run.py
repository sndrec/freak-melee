"""Owned-source feedback/source primitive comparisons; no game launch."""
import argparse,hashlib,json,subprocess,importlib.util,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('actual_body',HERE.parent/'script_item_spawn/extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
form_spec=importlib.util.spec_from_file_location('source_feedback_contract',HERE.parent/'script_form_feedback/extract.py');form=importlib.util.module_from_spec(form_spec);form_spec.loader.exec_module(form)
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: archive required')
 out.mkdir(parents=True);pieces=[];hashes=form.extract(ROOT,out)
 for relative,names in [('src/melee/ft/ftcoll.c',['ftColl_CreateReflectHit','ftColl_8007AEF8']),('src/melee/ft/native_fighter_module.c',['item_spawn_effect_on_bone'])]:
  path=ROOT/relative;text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();pieces += [extractor.body(text,n) for n in names]
 (out/'actual_feedback.h').write_text('\n\n'.join(pieces)+'\n')
 units=[ROOT/'src/melee/ft/native_script_binding_item_resources_catalog.c', ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c', HERE/'test.c',ROOT/'src/melee/ft/native_script_binding_feedback.c',ROOT/'src/melee/ft/native_script_effect_dispatch.c',ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/gm/native_match_pool.c']
 inputs=units+[HERE/'run.py',HERE/'README.md',HERE.parent/'script_form_feedback/extract.py',HERE.parent/'script_item_spawn/extract.py',a.compiler,out/'source_contract.inc',out/'dispatch_expected.inc',out/'dispatch_source_proof.json']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs})
 command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
 commands=[]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr)
  commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-6000:])
 for path,digest in hashes.items():
  if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'compiler':command,'commands':commands,'boundaries':['actual reflector creation/update, fighter part mapping and Item bone-effect source','actual callback lifetime and match-pool reference checks; selected package/actor view/storage controlled','unchanged complete descriptor catalog extraction and actual effect dispatch ABI classifier; source ownership guard bodies extracted unchanged','effect-spawn/wind/joint-position leaves controlled','position plus two scalars: exact ordered copies, malformed/nonfinite/other-recipe rejection, nine post-spawn ownership retirements','direct typed feedback primitive fixture; unrelated dispatchers/VM are not linked'],'game_acceptance':False}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
