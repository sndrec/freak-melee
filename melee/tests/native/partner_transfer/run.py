"""Actual partner transfer vs unchanged frozen66 original, console only."""
import argparse, hashlib, json, re, subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def body(text,name):
    m=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*(?://[^\n]*\n\s*)?\{',text)
    if not m: raise ValueError('missing body '+name)
    depth,end=1,m.end()
    while depth: depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[m.start():end]
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--reference',required=True,type=Path);ap.add_argument('--output',required=True,type=Path)
    a=ap.parse_args();snap=a.reference.resolve();out=a.output.resolve()
    if out.exists() or snap in out.parents or HERE in out.parents: ap.error('fresh output outside sources/reference required')
    manifest=snap/'freeze.json';frozen=json.loads(manifest.read_text())
    def verify():
        for name,expected in frozen.items():
            if name.replace('\\','/').startswith(('sdk/','compiler/')) and sha(snap/name)!=expected: raise RuntimeError('changed reference '+name)
    verify();out.mkdir(parents=True);inputs={};pieces=[]
    original=snap/'sdk/src/melee/ft/ftcommon.c';inputs[str(original)]=sha(original)
    pieces.append(body(original.read_text(),'ftCommon_8007EFC8').replace('ftCommon_8007EFC8','original_transfer',1))
    # Shared bodies remain exact, apart from the approved native identity stamp.
    hitlag_stamp = '    fp->native_hitlag_link_generation = nativeFighterTarget_Id(fp->x1A5C);\n'
    for rel,names in [('src/melee/ft/fighter.c',['Fighter_UnkInitReset_80067C98']),
        ('src/melee/ft/ft_0C88.c',['ftCo_800C88A0']),('src/melee/ft/ft_0DF0.c',['ftCo_800DEEA8']),
        ('src/melee/ft/ftcommon.c',['_func_8007E2FC_inline'])]:
        p=ROOT/rel;old=snap/'sdk'/rel;inputs[str(p)]=sha(p);inputs[str(old)]=sha(old)
        for name in names:
            actual=body(p.read_text(),name)
            compared=actual
            if name == 'Fighter_UnkInitReset_80067C98':
                if actual.count(hitlag_stamp) != 1 or ('    fp->x1A5C = 0;\n'+hitlag_stamp) not in actual:
                    raise RuntimeError('missing/moved/duplicate hitlag reset metadata stamp')
                compared=actual.replace(hitlag_stamp,'',1)
            if compared!=body(old.read_text(),name):raise RuntimeError('shared original changed '+name)
            pieces.insert(0,actual)
    p=ROOT/'src/melee/ft/native_fighter_target.c';inputs[str(p)]=sha(p)
    pieces.insert(0,body(p.read_text(),'nativeFighterTarget_Id'))
    p=ROOT/'src/melee/pl/player.c';inputs[str(p)]=sha(p)
    pieces[0:0]=[body(p.read_text(),n) for n in ['Player_CheckSlot','Player_HasAlternateMember','Player_GetEntityAtIndex','Player_SwapTransformedStates']]
    p=ROOT/'src/melee/ft/kinds/ftCommon/ftpickupitem.c';inputs[str(p)]=sha(p)
    pieces.append(body(p.read_text(),'ftpickupitem_80094818'))
    ownership=[]
    for rel,names in [('src/melee/it/item.c',['DestroyItemInline','Item_8026A8EC']),
                     ('src/melee/it/it_2725.c',['it_802742F4']),
                     ('src/melee/it/it_26B1.c',['it_8026B9A8']),
                     ('src/melee/pl/plbonuslib.c',['pl_8003E17C'])]:
        p=ROOT/rel;inputs[str(p)]=sha(p)
        ownership.extend(body(p.read_text(),n) for n in names)
    (out/'ownership_reference.c').write_text('\n\n'.join(ownership)+'\n',newline='\n')
    p=ROOT/'src/melee/ft/ftcommon.c';text=p.read_text();begin=text.index('typedef struct PartnerTransferIdentity');end=text.index('\n#ifndef MELEE_NATIVE',begin)
    pieces.append(text[begin:end]);inputs[str(p)]=sha(p)
    header=out/'actual_bodies.h';header.write_text('\n\n'.join(pieces)+'\n',newline='\n');inputs[str(header)]=sha(header)
    units=[HERE/'test.c',ROOT/'src/melee/ft/native_callback_lifetime.c']
    inputs.update({str(p):sha(p) for p in units+[Path(__file__).resolve()]})
    inputs.update({str(p):sha(p) for parent in [ROOT/'src',ROOT/'extern/dolphin/include'] for p in parent.rglob('*.h')})
    exe=out/'partner_transfer.exe';cmd=[str(snap/'compiler/clang.exe'),'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(exe)]
    commands=[]
    for stage,command in [('compile',cmd),('run',[str(exe)])]:
        p=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120)
        (out/(stage+'.log')).write_text(p.stdout+p.stderr,newline='\n');commands.append({'stage':stage,'command':command,'exit':p.returncode})
        if p.returncode:raise RuntimeError(stage+' failed:\n'+p.stdout+p.stderr)
    verify()
    for name,expected in inputs.items():
        if sha(Path(name))!=expected:raise RuntimeError('source changed while testing '+name)
    receipt={'status':'PASS actual common transfer source scope','result':json.loads(p.stdout),'reference_sha256':sha(manifest),'input_sha256':inputs,'commands':commands,'boundaries':['Complete unchanged frozen66 transfer reference and actual live transfer execute. Shared reset differs only by the required native hitlag generation stamp immediately after its original NULL link assignment; actual reset and actual live target-ID registry resolver execute. Generation advance and per-form state remain unchanged.','Actual Player role lookup/swap and relationship predicate, actual registry lifetime/dispatch, and actual pickup notification body execute.','Presentation, collision, camera, item attachment, removal and finish callbacks are controlled ordered sinks. PAGE_NOACCESS retirement protects both fighters and all three retained items. Actual pickup post-hook counter input is checked before simulated itGetKind userdata read.','Ownership/removal/attachment/counter bodies are archived as hashed source evidence, not executed whole. Intrinsic attachment animation continuation remains outside this fixture.','No paired package registration, move timing, renderer, gameplay or rollback acceptance.']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
