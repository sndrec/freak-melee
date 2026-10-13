"""Actual source/stock common transfer vs frozen shared reference, console only."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def body(text,name):
    match=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*(?://[^\n]*\n\s*)?\{',text)
    if not match:raise ValueError('Missing '+name)
    depth=1;end=match.end()
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[match.start():end]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--reference',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();reference=args.reference.resolve();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():parser.error('Fresh output on A: required')
    out.mkdir(parents=True);hashes={};parts=[];sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    frozen=reference/'sdk/src/melee/ft/ftcommon.c';parts.append(body(frozen.read_text(),'ftCommon_8007EFC8').replace('ftCommon_8007EFC8','original_transfer',1));hashes[str(frozen)]=sha(frozen)
    names={
        'src/melee/ft/fighter.c':['Fighter_UnkInitReset_80067C98'],
        'src/melee/ft/ft_0C88.c':['ftCo_800C88A0'],
        'src/melee/ft/ft_0DF0.c':['ftCo_800DEEA8'],
        'src/melee/ft/ftcommon.c':['_func_8007E2FC_inline','ftCommon_8007E2FC'],
        'src/melee/ft/native_fighter_target.c':['nativeFighterTarget_Id'],
        'src/melee/pl/player.c':['Player_CheckSlot','Player_HasAlternateMember','Player_GetEntityAtIndex','Player_SwapTransformedStates'],
        'src/melee/ft/kinds/ftCommon/ftpickupitem.c':['ftpickupitem_80094818']}
    for path,functions in names.items():
        file=ROOT/path;source=file.read_text();hashes[str(file)]=sha(file)
        parts[0:0]=[body(source,name) for name in functions]
    (out/'actual_bodies.h').write_text('\n\n'.join(parts)+'\n',newline='\n')
    units=[HERE/'test.c',ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/ft/ftformtransfer.c']
    hashes.update({str(path):sha(path) for path in units+[HERE/'run.py',ROOT/'src/melee/ft/native_script_fighter.h',ROOT/'src/melee/ft/native_fighter_runtime.h']})
    compiler=reference/'compiler/clang.exe';exe=out/'test.exe'
    command=[compiler,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+units+['-o',exe]
    commands=[]
    for stage,cmd in [('compile',command),('run',[exe])]:
        cmd=list(map(str,cmd));result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-14000:])
    if any(sha(Path(path))!=value for path,value in hashes.items()):raise RuntimeError('Changed source during test')
    receipt={'status':'PASS','result':json.loads(result.stdout),'inputs':hashes,'commands':commands,
        'scope':'Actual production transfer unit, shared reset/Player role/pickup bodies, native lifetime registry and immutable frozen common-transfer reference. Source package/bank/view/controller/finish-event and graphics/status/item attachment leaves controlled; no authored native source callback, VM, actual actor attach, package publication, rollback or game claim.'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
