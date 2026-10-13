"""Read-only exact composed Rook source/body/metadata audit; no assets invented."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);folder=ROOT/'examples/rook_workshop';files=[folder/'fighter_state.fms',*sorted((folder/'moves').glob('*.fms'))]
 composed='';mapping=[]
 for path in files:
  if composed and not composed.endswith('\n'):composed+='\n'
  start=composed.count('\n')+1;text=path.read_bytes().decode('utf-8');composed+=text
  mapping.append({'file':str(path),'first_line':start,'line_count':text.count('\n')+bool(text and not text.endswith('\n'))})
 source=out/'composed.fms';source.write_bytes(composed.encode('utf-8'));(out/'line_map.json').write_text(json.dumps(mapping,indent=2)+'\n')
 spec=importlib.util.spec_from_file_location('actual_compiler',ROOT/'tests/native/script_mario_contracts/run.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor);hashes=extractor.extract(ROOT,out)
 names=['source','expression','statement','program','value','bytecode','vm','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','compile', 'binding_item_resources_resolve','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','binding_effect_resolve','binding_spawn_resolve','item_program', 'resource_metadata','metadata']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/sysdolphin/baselib/dat.c',HERE/'test.c',ROOT/'tests/native/script_compile/native_boundaries.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['actions', 'item_resources','effect','item','presentation','spawn','ledge','jump','forms','feedback', 'hitlag', 'stage', 'family']]
 units += [ROOT/'tests/native'/('script_receiver_unavailable_'+n+'.c') for n in ['registry','identity','pool','item_actor']]
 inputs=units+[HERE/'run.py',*files,*(folder/'items').glob('*.png'),ROOT/'src/melee/ui/native_shell_workshop_script_files.cpp']
 hashes.update({str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in inputs})
 command=[str(a.compiler),'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/melee/ft'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
 commands=[]
 for stage,cmd in [('compile',command),('audit',[str(out/'test.exe'),str(source)])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-7000:])
 def local(line):
  found=next((m for m in mapping if m['first_line']<=line<m['first_line']+m['line_count']),None)
  return {'file':found['file'],'line':line-found['first_line']+1} if found else {}
 result=json.loads(r.stdout)
 for entry in result.get('bodies',[]):entry['authored_location']=local(entry['line']);entry['diagnostic']['authored_location']=local(entry['diagnostic']['line'])
 for entry in result.get('metadata_without_assets',[]):entry['authored_location']=local(entry['line'])
 for entry in result.get('calls_in_failed_bodies',[]):entry['authored_location']=local(entry['line'])
 for path,digest in hashes.items():
  if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError('Source changed '+path)
 result.update({'repository_resources':{'icons':[str(p) for p in sorted((folder/'items').glob('*.png'))],'canonical_blueprint':str(folder/'items/base.fib'),'blueprint_exists':(folder/'items/base.fib').is_file(),'dat_files':[str(p) for p in folder.rglob('*.dat')],'asset_record_exists':(folder/'assets.fma').is_file()},'commands':commands,'input_sha256':hashes,'line_map':mapping,'scope':'exact unmodified Workshop composition, production source/index/event signatures and actual compiler resolver/body/lowering/bytecode; metadata uses actual validator without assets; body successes do not imply whole metadata/package/runtime acceptance','limits':['first diagnostic per complete body only; later errors may remain','motion extent341/12 is controlled census input matching prior authored base/native Rook table; not imported DAT evidence; no named special IDs guessed','no Rook DAT/facts/DAT model or complete holdable binding available; PNG bytes only inventoried','no game/global build/production/authored behavior edits']})
 (out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result.get('summary',result)))
if __name__=='__main__':main()
