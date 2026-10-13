"""Source motion-rate catalog/VM/native-call contract fixture; no game launch."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('Use a fresh output directory on A:')
    out.mkdir(parents=True)
    names=['native_script_compile', 'native_script_binding_item_resources_resolve','native_script_moves','native_script_move_lowering','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_constants','native_script_execution','native_script_event_payload','native_script_events','native_script_storage','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_properties','native_script_binding_transition','native_script_value','native_script_math','native_script_expression','native_callback_lifetime','native_script_source','native_script_statement','native_script_program','native_script_bytecode','native_script_vm']
    units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[HERE/'test.c',HERE/'fixture.c',HERE/'unavailable.c',ROOT/'src/melee/gm/native_match_pool.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
    units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
    absent=['script_bindings_unavailable_actions', 'script_bindings_unavailable_item_resources.c','script_bindings_unavailable_effect','script_bindings_unavailable_spawn','script_bindings_unavailable_item','script_bindings_unavailable_presentation','script_bindings_unavailable_jump','script_bindings_unavailable_forms','script_bindings_unavailable_ledge','script_receiver_unavailable_identity','script_receiver_unavailable_pool']
    units += [HERE.parent/(name+'.c') for name in absent]
    units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    inputs=units+list(HERE.glob('*.h'))+[HERE/'run.py',HERE/'README.md',a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    hashes={str(path):sha(path) for path in inputs}
    command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,units),'-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd in [('build',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8');commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-16000:])
    if any(sha(Path(path))!=digest for path,digest in hashes.items()):raise RuntimeError('Input changed during fixture')
    receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'actual':['compiler/typed catalog and marshalling','Transition_Invoke and SourceFighterLayout ownership validator','VM/execution/storage and physical source membership guards'],'controlled':['selected SourcePackage/publication tables and actor status/view','Fighter_ChangeMotionState installation and ftAnim startup native leaves'],'limits':['no game or real animation graph/DAT import','no real actor attachment or package publication','numeric native arguments checked bit-exactly; full motion implementation not executed']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
