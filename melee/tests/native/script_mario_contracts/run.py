"""Compile the complete Mario source and report exact body diagnostics."""
import argparse,hashlib,json,subprocess,importlib.util,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('source_body',HERE.parent/'script_item_spawn/extract.py');body=importlib.util.module_from_spec(spec);spec.loader.exec_module(body)
def extract(root,out):
 source=root/'src/melee/ft/native_script_compile.c';text=source.read_text()
 context=re.search(r'typedef struct CompileContext \{.*?\} CompileContext;',text,re.S).group()
 includes='\n'.join(line for line in text.splitlines() if line.startswith('#include'))
 out.mkdir(parents=True,exist_ok=True)
 (out/'actual_compile_context.h').write_text(includes+'\n'+context+'\n'+'\n'.join(body.body(text,n) for n in ['error','resolve','function_context','describe']))
 return {str(source):hashlib.sha256(source.read_bytes()).hexdigest()}
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: archive required')
 hashes=extract(ROOT,out)
 names=['source','expression','statement','program','value','bytecode','vm','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','compile', 'binding_item_resources_resolve','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','binding_effect_resolve','binding_spawn_resolve','item_program', 'resource_metadata','metadata']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/sysdolphin/baselib/dat.c',HERE/'test.c',HERE.parent/'script_compile/native_boundaries.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [HERE.parent/('script_bindings_unavailable_'+n+'.c') for n in ['actions', 'item_resources','effect','item','presentation','spawn', 'hitlag', 'stage', 'family']]
 units += [HERE.parent/('script_receiver_unavailable_'+n+'.c') for n in ['registry','identity','pool','item_actor']]
 command=[str(a.compiler),'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/melee/ft'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
 hashes.update({str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in units+[HERE/'run.py',ROOT/'examples/mario_full.fms']})
 for stage,cmd in [('compile',command),('census',[str(out/'test.exe'),str(ROOT/'examples/mario_full.fms')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr)
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-7000:])
 receipt=json.loads(r.stdout);receipt.update({'actual_source_sha256':hashes,'scope':'exact production source compilation and body resolver/lowerer; explicit audited metadata with controlled assets/import scalar values; no runtime/package/game acceptance'})
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
