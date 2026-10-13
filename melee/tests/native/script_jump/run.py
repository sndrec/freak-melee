"""Actual shared jump bodies and complete typed source/VM primitive execution."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from extract import extract
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--compiler',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():parser.error('Use fresh output on A:')
    out.mkdir(parents=True);inputs=extract(ROOT,out)
    names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_properties','binding_jump',
        'compile', 'binding_item_resources_resolve','metadata','item_program', 'resource_metadata','fighter_program','effect_program','moves','move_lowering','events','event_payload','storage','execution',
        'binding_spawn_resolve','binding_effect_resolve','receiver','binding_receiver','runtime']
    units=[ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in names]
    units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
    units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
    units+=[ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/ft/ftwalljump.c',ROOT/'src/sysdolphin/baselib/dat.c',HERE/'boundary.c',HERE/'test.c']
    units += [HERE.parent/('script_bindings_unavailable_'+name+'.c') for name in ['actions', 'item_resources','item','spawn','effect','presentation','ledge','forms','feedback', 'hitlag', 'stage', 'family']]
    units += [HERE.parent/'script_receiver_unavailable_identity.c',HERE.parent/'script_receiver_unavailable_pool.c',ROOT/'src/melee/gm/native_match_pool.c']
    units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    inputs.update({str(path):sha(path) for path in units+[HERE/'extract.py',HERE/'run.py',HERE/'boundary.h',
        ROOT/'src/melee/ft/native_script_fighter.h',ROOT/'src/melee/ft/native_script_binding_jump.h']})
    command=[args.compiler,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref','-Wl,/errorlimit:0',
        '-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-unused-variable',
        '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES',
        '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+units+['-o',out/'test.exe']
    commands=[]
    for stage,cmd in [('compile',command),('run',[out/'test.exe'])]:
        cmd=list(map(str,cmd));result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-15000:])
    if any(sha(Path(path))!=value for path,value in inputs.items()):raise RuntimeError('Source changed during test')
    receipt={'status':'PASS','result':json.loads(result.stdout),'inputs':inputs,'commands':commands,
        'scope':'Actual source compilation, prepared hook/motion IDs, VM/storage/catalog/owned guard/jump bindings, shared entry/IASA bodies and walljump body. Source asset/view/actor dispatch boundary and motion/audio/input/geometry leaves controlled. No actual actor attach, DAT/package preparation, game or renderer.'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
