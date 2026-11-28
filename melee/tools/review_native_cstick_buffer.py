"""Replay C-stick ramps and jump release through fighter input checks."""
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

# (label, jump button, tap-jump Y, attack button, C-stick X, release offset,
#  expected hop, expected action if the aerial stays buffered through takeoff)
HOP_CASES = (
    ('short control', 0x400, 0, 0, 0, 2, 'short', 'JumpF'),
    ('full control', 0x400, 0, 0, 0, 12, 'full', 'JumpF'),
    ('button jump + A', 0x400, 0, 0x100, 0, 2, 'short', 'AttackAirN'),
    ('button jump + C-stick', 0x400, 0, 0, 85, 2, 'short', 'AttackAirF'),
    ('held jump + C-stick', 0x400, 0, 0, 85, 12, 'full', 'AttackAirF'),
    ('tap jump + C-stick', 0, 85, 0, 85, 2, 'short', 'AttackAirF'),
    ('button release on takeoff', 0x400, 0, 0, 85, 4, 'short', 'AttackAirF'),
    ('tap release on takeoff', 0, 85, 0, 85, 4, 'short', 'AttackAirF'),
    ('button release after takeoff', 0x400, 0, 0, 85, 5, 'full', 'AttackAirF'),
)


def recording(path, frames, tilt, check_hops):
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
    hops = {}
    if check_hops:
        for i, (_, jump, stick_y, attack, cstick_x, release, _, _) in enumerate(HOP_CASES):
            start = SAMPLES + i * 130
            for offset in range(release):
                hops[start + offset] = (jump, stick_y, 0)
            hops[start + 1] = (jump | attack, stick_y, cstick_x)
    samples = SAMPLES + (len(HOP_CASES) * 130 if check_hops else 0)
    policy=frames<<24 | (0x20000000 if tilt else 0)
    empty=PAD.pack(0,0,0,0,0,0,0,0,0,-1,0)
    data=bytearray(struct.pack('<8sQII',b'FMREPLAY',0,0x12345678,1))
    for f in range(samples):
        buttons,x,y=schedule.get(f,(0,0,0))
        buttons, stick_y, x = hops.get(f, (buttons, 0, x))
        data.extend(bytes(8)+PAD.pack(buttons,0,stick_y,x,y,0,0,0,0,0,policy)+empty*3+bytes(8))
    path.write_bytes(data)
    return samples


def verify_hops(rows, names, frames, check):
    takeoffs = {}
    for i, (label, _, _, _, _, _, expected_hop, attack) in enumerate(HOP_CASES):
        start = SAMPLES + i * 130
        # Replay sample numbers in the trace are one-based. Mario's first
        # airborne sample is five samples after the last grounded neutral input.
        takeoff = rows[start + 5]
        rise = takeoff['y'] - rows[start]['y']
        motion = names.get(takeoff['motion'])
        expected_motion = attack if frames >= 3 else 'JumpF'
        check(takeoff['air'] and motion == expected_motion,
              f'{label}: expected {expected_motion} on takeoff, got {motion}')
        takeoffs[label] = rise
    short = takeoffs['short control']
    full = takeoffs['full control']
    check(full - short > 0.5, 'short and full hop controls were too close')
    for label, _, _, _, _, _, expected_hop, _ in HOP_CASES[2:]:
        expected_rise = short if expected_hop == 'short' else full
        # Entering the aerial on takeoff applies one frame of Mario's gravity.
        check(abs(takeoffs[label] - expected_rise) < 0.11,
              f'{label}: expected {expected_hop} hop, rose {takeoffs[label]:.3f}')
    return takeoffs


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
                folder=output/key; folder.mkdir(); path=folder/'inputs.fmr'
                check_hops = fighter == 'mario' and not tilt
                samples = recording(path, frames, tilt, check_hops)
                env=dict(os.environ,LOCALAPPDATA=str(folder/'appdata'),SDL_JOYSTICK_HIDAPI='0',SDL_AUDIODRIVER='dummy')
                with (folder/'match.log').open('w') as log:
                    process=subprocess.run([str(exe),'--iso',str(iso),'--hidden','--mute','--fast',
                        '--match','--stage','practice','--p1','human:'+fighter,'--p2','off','--p3','off','--p4','off',
                        '--stocks','1','--items','off','--replay-inputs',str(path),'--trace-actions','--trace-fighter-state'],
                        env=env,cwd=folder,stdout=log,stderr=subprocess.STDOUT,
                        creationflags=subprocess.CREATE_NO_WINDOW,timeout=120)
                log=(folder/'match.log').read_text(errors='replace')
                if process.returncode!=0 or f'[replay] End after {samples} controller samples.' not in log:
                    receipt['errors'].append(key+': replay did not finish'); continue
                rows = state_rows(log)
                events=transitions(rows,names); receipt['cases'][key]=events
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
                if check_hops:
                    # Consecutive smashes share a motion ID; check the animation
                    # restart to distinguish a buffered attack from a held pose.
                    restarted=re.search(
                        r'^\[fighter\] sample 1692 P1 motion=\d+ frame=3f800000 ',log,re.M)
                    check(bool(restarted)==bool(frames),'recovery-buffered smash did not restart as expected')
                    receipt.setdefault('takeoff_rises', {})[key] = verify_hops(rows, names, frames, check)
                print(key, 'checked', flush=True)
        receipt['status']='FAIL' if receipt['errors'] else 'PASS'
    finally:
        (output/'review.json').write_text(json.dumps(receipt,indent=2)+'\n')
    if receipt['errors']: raise AssertionError('\n'.join(receipt['errors']))
    print('PASS:',len(receipt['cases']),'fighter/profile/buffer combinations')


if __name__=='__main__': main()
