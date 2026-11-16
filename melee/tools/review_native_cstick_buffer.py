"""Replay C-stick ramps through real fighter input checks, hidden and muted."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess

from review_native_profile_controls import motion_ids, state_rows


SAMPLES = 1900
PAD = struct.Struct('<HbbbbBBBBbxI')
DIRECTIONS = [('right',1,0,'AttackS4S','AttackS3S'),
              ('left',-1,0,'AttackS4S','AttackS3S'),
              ('up',0,1,'AttackHi4','AttackHi3'),
              ('down',0,-1,'AttackLw4','AttackLw3')]


def recording(path, frames, tilt):
    schedule = {}
    for i,(_,x,y,_,_) in enumerate(DIRECTIONS):
        start=250+i*200
        schedule[start]=(0,40*x,40*y)
        for f in range(start+1,start+5): schedule[f]=(0,85*x,85*y)
    schedule[1050]=(0,85,0)  # Full-strength snap without an intermediate sample.
    schedule[1250]=(0,40,0); schedule[1251]=(0,85,0)  # Release immediately.
    for f in range(1450,1454): schedule[f]=(0x400,0,0)
    schedule[1460]=(0,40,0); schedule[1461]=(0,85,0)
    # Mario's second smash is entered just before the first one's recovery ends.
    schedule[1650]=(0,85,0)
    schedule[1689]=(0,40,0); schedule[1690]=(0,85,0)
    policy=frames<<24 | (0x20000000 if tilt else 0)
    empty=PAD.pack(0,0,0,0,0,0,0,0,0,-1,0)
    data=bytearray(struct.pack('<8sQII',b'FMREPLAY',0,0x12345678,1))
    for f in range(SAMPLES):
        buttons,x,y=schedule.get(f,(0,0,0))
        data.extend(bytes(8)+PAD.pack(buttons,0,0,x,y,0,0,0,0,0,policy)+empty*3+bytes(8))
    path.write_bytes(data)


def transitions(rows, names):
    result=[]
    for frame,value in rows.items():
        motion=names.get(value['motion'],str(value['motion']))
        if not result or result[-1][1]!=motion: result.append((frame,motion))
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','iso','output'): parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args(); exe=args.exe.resolve(); iso=args.iso.resolve(); output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    names={value:key.removeprefix('ftCo_MS_') for key,value in motion_ids().items()}
    receipt=dict(status='RUNNING',exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),cases={},errors=[])
    try:
        for fighter,tilt,buffers in [('mario',False,(0,1,3,7)),('ganondorf',False,(0,3)),
                                     ('falco',False,(0,3)),('mario',True,(0,3))]:
            reference=None
            for frames in buffers:
                key=f'{fighter}-'+('tilt' if tilt else 'smash')+f'-buffer-{frames}'
                folder=output/key; folder.mkdir(); path=folder/'inputs.fmr'; recording(path,frames,tilt)
                env=dict(os.environ,LOCALAPPDATA=str(folder/'appdata'),SDL_JOYSTICK_HIDAPI='0',SDL_AUDIODRIVER='dummy')
                with (folder/'match.log').open('w') as log:
                    process=subprocess.run([str(exe),'--iso',str(iso),'--hidden','--mute','--fast',
                        '--match','--stage','practice','--p1','human:'+fighter,'--p2','off','--p3','off','--p4','off',
                        '--stocks','1','--items','off','--replay-inputs',str(path),'--trace-actions','--trace-fighter-state'],
                        env=env,cwd=folder,stdout=log,stderr=subprocess.STDOUT,
                        creationflags=subprocess.CREATE_NO_WINDOW,timeout=120)
                log=(folder/'match.log').read_text(errors='replace')
                if process.returncode!=0 or f'[replay] End after {SAMPLES} controller samples.' not in log:
                    receipt['errors'].append(key+': replay did not finish'); continue
                events=transitions(state_rows(log),names); receipt['cases'][key]=events
                def check(condition,message):
                    if not condition: receipt['errors'].append(key+': '+message)
                for i,(direction,_,_,smash,ground_tilt) in enumerate(DIRECTIONS):
                    start=250+i*200; expected=ground_tilt if tilt else smash
                    check((start+2,expected) in events,direction+' ramp missed its expected attack frame')
                expected='AttackS3S' if tilt else 'AttackS4S'
                check((1051,expected) in events,'instant snap missed')
                check((1252,expected) in events,'short ramp missed')
                check((1461,'AttackAirF') in events,'aerial ramp changed')
                ordinary=[row for row in events if row[0]<1650]
                if reference is None: reference=ordinary
                else: check(ordinary==reference,'action timing differs from zero-buffer reference')
                if fighter=='mario' and not tilt:
                    # Consecutive smashes share a motion ID; check the animation
                    # restart to distinguish a buffered attack from a held pose.
                    restarted=re.search(
                        r'^\[fighter\] sample 1692 P1 motion=\d+ frame=3f800000 ',log,re.M)
                    check(bool(restarted)==bool(frames),'recovery-buffered smash did not restart as expected')
                print(key, 'checked', flush=True)
        receipt['status']='FAIL' if receipt['errors'] else 'PASS'
    finally:
        (output/'review.json').write_text(json.dumps(receipt,indent=2)+'\n')
    if receipt['errors']: raise AssertionError('\n'.join(receipt['errors']))
    print('PASS:',len(receipt['cases']),'fighter/profile/buffer combinations')


if __name__=='__main__': main()
