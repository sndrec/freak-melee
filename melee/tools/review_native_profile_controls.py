"""Recheck saved profile UI and canonical gameplay input policies locally.

Uses copied profiles, hidden/muted clients and deterministic offline input files.
The recordings enter after raw-device mapping; they prove fighter semantics,
not physical adapter calibration or successful raw SDL binding capture.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import subprocess

from review_native_css_room import Client


def inputs(path, policy):
    data=bytearray(struct.pack('<8sQII',b'FMREPLAY',0,0x12345678,1))
    empty=struct.pack('<HbbbbBBBBbxI',0,0,0,0,0,0,0,0,0,-1,0)
    for frame in range(2200):
        extension=policy|(0x80000000 if 650<=frame<668 else 0)
        pad=struct.pack('<HbbbbBBBBbxI',0x400 if 350<=frame<368 else 0,
                        0,80 if 950<=frame<954 else 0,80 if frame==1250 else 0,
                        80 if frame==1550 else -80 if frame==1850 else 0,
                        0,0,0,0,0,extension)
        data.extend(bytes(8)+pad+empty*3+bytes(8))
    path.write_bytes(data)


def motion_ids():
    source=(Path(__file__).resolve().parents[1]/'src/melee/ft/kinds/ftCommon/forward.h').read_text()
    body=source.split('typedef enum ftCommon_MotionState {',1)[1].split('}',1)[0]
    result={};value=-1
    for name,explicit in re.findall(r'(ftCo_MS_\w+)(?:\s*=\s*(-?\d+))?',body):
        value=int(explicit) if explicit else value+1
        result[name]=value
    return result


def state_rows(text):
    def real(bits):return struct.unpack('<f',struct.pack('<I',int(bits,16)))[0]
    return {int(frame):dict(motion=int(motion),air=int(air),y=real(y))
        for frame,motion,air,y in re.findall(
            r'^\[fighter\] sample (\d+) P1 motion=(\d+) .*?ga=(\d+) .*?pos=[\da-f]+,([\da-f]+),',text,re.M)}


def review_ui(args, output, receipt):
    first=Client(args.exe.resolve(),args.iso.resolve(),args.profile.resolve(),output/'settings',
                 'Profile Review',window_size=(1440,900),without_drafts=True)
    try:
        first.click('nav-6');first.click('controls');first.element('profile-name')
        first.click('profile-new');first.enter('profile-name','Native controls review')
        first.click('profile-port-3');first.click('profile-tap');first.click('profile-cstick')
        first.click('profile-save')
        files=list((first.data/'profiles').glob('*.fmp'))
        matches=[p for p in files if p.read_bytes()[4:36].split(b'\0')[0]==b'Native controls review']
        assert len(matches)==1,files
        profile=matches[0];saved=profile.read_bytes()
        assert len(saved)==191 and saved[36:38]==bytes((0,1))
        assert struct.unpack_from('<4H',saved,40)==(8000,8000,31150,31150)
        receipt['ui']['saved_profile']=profile.name
        receipt['ui']['gameplay_screenshot']=str(first.screenshot())
        first.command('tap',0,'START','sy=80','frames=8')
        assert 'visible=1' in first.inspect('profiles-panel'),'Background input closed the settings scene'
        first.click('profile-bind');first.click('bind-0')
        capture=first.inspect('profile-status')
        if 'Connect a controller to this port first.' in capture:
            receipt['ui']['capture']='Disconnected port rejects capture before it starts'
        else:
            assert 'Release controls, then press the input to bind.' in capture,capture
            first.command('ui-key','escape')
            assert 'Binding cancelled.' in first.inspect('profile-status')
            receipt['ui']['capture']='Capture starts and Escape cancels it'
        first.click('profile-tune');first.enter('profile-main-zone','bad')
        first.click('profile-save');assert profile.read_bytes()==saved
        receipt['ui']['invalid_screenshot']=str(first.screenshot())
        first.enter('profile-main-zone','17.5');first.enter('profile-l-zone','0');first.enter('profile-r-zone','100')
        first.click('profile-save');saved=profile.read_bytes()
        assert struct.unpack_from('<4H',saved,40)==(math.floor(32767*.175+.5),8000,0,32767)
        receipt['ui']['calibration_screenshot']=str(first.screenshot())
        receipt['ui']['saved_sha256']=hashlib.sha256(saved).hexdigest()
        first.click('profile-close')
    finally:
        first.stop();receipt['ui']['first_stopped']=first.process.poll() is not None
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))
    second=Client(args.exe.resolve(),args.iso.resolve(),first.folder/'appdata',output/'reopened',
                  'Profile Review',window_size=(1440,900),without_drafts=True)
    try:
        second.click('nav-6');second.click('controls');second.click('profile-list')
        # The copied catalog may contain other fixtures; select the exact saved ID.
        selected=False
        for _ in range(65):
            second.command('ui-key','down');second.command('ui-key','enter')
            if 'Native controls review' in second.inspect('profile-name'):
                selected=True;break
            second.click('profile-list')
        assert selected,'Saved profile did not reopen'
        assert (second.data/'profiles'/profile.name).read_bytes()==saved
        second.click('profile-tune')
        assert 'value=17.5' in second.inspect('profile-main-zone')
        assert 'value=0' in second.inspect('profile-l-zone')
        assert 'value=100' in second.inspect('profile-r-zone')
        receipt['ui']['reopened_screenshot']=str(second.screenshot())
        second.click('profile-delete');second.click('profile-delete-cancel')
        assert (second.data/'profiles'/profile.name).exists()
        second.click('profile-delete');second.click('profile-delete-yes')
        assert not (second.data/'profiles'/profile.name).exists()
        second.click('profile-close')
    finally:
        second.stop();receipt['ui']['second_stopped']=second.process.poll() is not None
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))


def review_gameplay(args, output, receipt):
    ids=motion_ids()
    for name,policy in (('default',0),('no-tap',0x40000000),('tilt',0x20000000)):
        folder=output/name;folder.mkdir();recording=folder/'inputs.fmr';inputs(recording,policy)
        environment=dict(os.environ,LOCALAPPDATA=str(folder/'appdata'))
        with (folder/'stdout.log').open('w') as stdout,(folder/'stderr.log').open('w') as stderr:
            process=subprocess.run([str(args.exe.resolve()),'--iso',str(args.iso.resolve()),
                '--hidden','--mute','--fast','--match','--stage','practice','--p1','human:ganondorf',
                '--p2','off','--p3','off','--p4','off','--stocks','1','--items','off',
                '--replay-inputs',str(recording),'--trace-actions','--trace-fighter-state'],
                cwd=folder,env=environment,stdout=stdout,stderr=stderr,
                creationflags=subprocess.CREATE_NO_WINDOW,timeout=120)
        text=(folder/'stdout.log').read_text(errors='replace')+(folder/'stderr.log').read_text(errors='replace')
        assert process.returncode==0 and '[replay] End after 2200 controller samples.' in text,text[-2000:]
        assert not re.search(r'Fighter source VM failed|native panic stack|Shader compilation failed',text)
        rows=state_rows(text)
        def motions(start,end):return sorted({r['motion'] for f,r in rows.items() if start<=f<end})
        def height(start,end):return max(r['y'] for f,r in rows.items() if start<=f<end)-rows[start-1]['y']
        full,short=height(350,600),height(650,900)
        assert full>short>0,(name,full,short)
        tap=motions(950,1150)
        jumps={ids['ftCo_MS_JumpF'],ids['ftCo_MS_JumpB'],ids['ftCo_MS_KneeBend']}
        assert bool(jumps.intersection(tap))==(name!='no-tap'),(name,tap)
        attacks={}
        for label,start,tilt,smash in (
            ('side',1250,'AttackS3S','AttackS4S'),('up',1550,'AttackHi3','AttackHi4'),
            ('down',1850,'AttackLw3','AttackLw4')):
            seen=motions(start,start+180)
            expected=ids['ftCo_MS_'+(tilt if name=='tilt' else smash)]
            assert expected in seen,(name,label,expected,seen)
            attacks[label]=seen
        receipt['gameplay'][name]=dict(full_height=full,short_height=short,tap_motions=tap,attacks=attacks,
                                       replay_sha256=hashlib.sha256(recording.read_bytes()).hexdigest())
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','iso','profile','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    receipt=dict(status='RUNNING',exe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                 script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                 ui={},gameplay={},limitations=['Raw SDL binding capture','Physical GameCube adapter feel','Physical rumble'])
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT']='http://127.0.0.1:1'
    review_ui(args,output,receipt)
    review_gameplay(args,output,receipt)
    receipt['status']='PASS'
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2))
    print(json.dumps(receipt,indent=2))


if __name__=='__main__':main()
