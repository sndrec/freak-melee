"""Recheck saved profile UI and canonical gameplay input policies locally.

Uses copied profiles, hidden/muted clients and deterministic offline input files.
The recordings enter after raw-device mapping; they prove fighter semantics,
not physical adapter calibration. Raw capture uses an SDL virtual gamepad.
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
import time

from review_native_css_room import Client
from review_native_release import close_window


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
    client = None
    ui = receipt['ui']; ui['screenshots'] = {}
    os.environ['FREAK_MELEE_MENU_MOTION'] = '0'
    os.environ['SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS'] = '1'
    for service in ('SERVICE','CHAT','COMMUNITY'):
        os.environ['FREAK_MELEE_'+service+'_ENDPOINT'] = 'http://localhost:1'

    def state(): return client.inspect('controller-profile')
    def wait(pattern, timeout=15):
        deadline = time.monotonic()+timeout
        while time.monotonic()<deadline:
            text = state()
            if re.search(pattern,text): return text
            time.sleep(.1)
        raise AssertionError((pattern,text))
    def shot(name): ui['screenshots'][name] = str(client.screenshot())
    def device(command):
        client.command('device',command); time.sleep(.12)
    def axes(x=0,y=0,cx=0,cy=0,lt=-32768,rt=-32768):
        device(f'axes,0,{x},{y},{cx},{cy},{lt},{rt}')
    def open_profiles():
        client.click('nav-6'); client.click('settings-category-4'); client.click('edit-controls')
        client.element('profile-name')
    def select(name):
        entries = re.findall(r'controller-profile-entry index=(\d+) id=([^ ]+) name=([^\r\n]+)',state())
        index = next(int(i) for i,_,n in entries if n==name)
        client.click('profile-new')
        for _ in range(index//5): client.click('profile-list-next')
        client.click('profile-entry-'+str(index))
    def drag(name,fraction,first,last):
        start = client.element(name); left,right = client.element(first),client.element(last)
        point = (round(left[0]+(right[0]-left[0])*fraction),start[1])
        client.command('ui-down',*start); client.command('ui-move',*point)
        time.sleep(.25); client.command('ui-up',*point); time.sleep(.15)

    try:
        client = Client(args.exe.resolve(),args.iso.resolve(),args.profile.resolve(),output/'settings',
                        'Native profile review',window_size=(1440,900),without_drafts=True)
        open_profiles(); client.click('profile-new')
        client.enter('profile-name','Native controls review'); client.command('ui-key','enter')
        client.click('profile-port-3')
        client.command('ui-move',*client.element('profile-tap')); client.tap('A',frames=40)
        wait(r'tap=0 buffer=0 cstick=0')
        for _ in range(3): client.click('profile-buffer')
        client.click('profile-cstick'); client.click('profile-save')
        wait(r'tap=0 buffer=3 cstick=1')
        main_id = re.search(r' id=([^ ]+) name=Native controls review ',state())[1]
        profile = client.data/'profiles'/(main_id+'.fmp'); saved = profile.read_bytes()
        assert saved[4:36].split(b'\0')[0] == b'Native controls review'
        assert saved[36:38] == bytes((6,1))
        shot('gameplay')
        device('attach,0'); time.sleep(.6); device('port,0,3')
        wait(r'controller-profile-status device=\d+ ')
        client.click('profile-tab-1'); client.click('bind-0')
        wait(r'capture=0,-1 ready=1 release=0')
        device('button,0,0,1'); wait(r'capture=0,-1 ready=1 release=1')
        device('button,0,0,0'); wait(r'capture=-1,-1')
        assert 'controller-input action=0 binding=0 type=1 input=0' in state()
        client.click('add-0'); wait(r'capture=0,-1 ready=1')
        axes(x=24000); wait(r'release=1'); axes(); wait(r'capture=-1,-1')
        assert 'controller-binding action=0 count=2' in state()
        client.click('add-0'); wait(r'capture=0,-1 ready=1')
        device('button,0,0,1'); wait(r'release=1'); device('button,0,0,0'); wait(r'capture=-1,-1')
        assert 'controller-binding action=0 count=2' in state()
        client.click('bind-1'); wait(r'capture=1,-1 ready=1')
        client.command('ui-key','escape'); wait(r'capture=-1,-1')
        client.click('bind-1'); wait(r'capture=1,-1 ready=1')
        device('button,0,6,1'); time.sleep(.8); wait(r'capture=-1,-1')
        device('button,0,6,0')
        assert 'controller-input action=1 binding=0 type=0 input=-1' in state()
        shot('buttons')
        client.click('profile-tab-2'); client.click('axis-1'); wait(r'capture=-1,1 ready=1')
        axes(y=-24000); wait(r'release=1'); axes(); wait(r'capture=-1,-1')
        assert 'axis=1 direction=-1 assigned=1' in state()
        shot('axes'); client.click('profile-tab-3')
        for field,value in [('profile-main-zone','17.5'),('profile-l-zone','0'),('profile-r-zone','100')]:
            client.enter(field,value); client.command('ui-key','enter')
        assert 'controller-zones main=5734 c=8000 l=0 r=32767' in state()
        client.enter('profile-c-zone','bad'); client.command('ui-key','enter')
        assert 'controller-zones main=5734 c=8000 l=0 r=32767' in state()
        client.click('profile-save'); saved = profile.read_bytes()
        assert struct.unpack_from('<4H',saved,40)==(5734,8000,0,32767)
        shot('calibration-options')
        axes(x=1000,y=-750,cx=-400,cy=250,lt=-30000,rt=-30000)
        client.click('profile-calibrate'); wait(r'step=0 testing=0')
        time.sleep(1.2); shot('rest')
        client.click('profile-calibration-next'); wait(r'step=1 testing=0')
        before = client.inspect('hub-pointer')
        for x,y in [(29000,0),(29000,28000),(0,28000),(-29000,28000),(-29000,0),(-29000,-28000),(0,-28000),(29000,-28000)]:
            axes(x=x,y=y,cx=round(x*.92),cy=round(y*.92),lt=-30000,rt=-30000)
        axes(x=1000,y=-750,cx=-400,cy=250,lt=-30000,rt=-30000)
        assert re.search(r'local=([^ ]+)',before)[1] == re.search(r'local=([^ ]+)',client.inspect('hub-pointer'))[1]
        shot('sticks'); client.click('profile-calibration-next'); wait(r'step=2 testing=0 applied=1')
        for value in [-18000,0,18000,30000,-30000]:
            axes(x=1000,y=-750,cx=-400,cy=250,lt=value,rt=value)
        for trigger in range(2):
            drag(f'profile-trigger-{trigger}-start',.08,f'profile-trigger-{trigger}-start',f'profile-trigger-{trigger}-end')
            drag(f'profile-trigger-{trigger}-end',.90,f'profile-trigger-{trigger}-start',f'profile-trigger-{trigger}-end')
        shot('triggers'); client.click('profile-calibration-next'); wait(r'step=3 testing=0 applied=1')
        assert profile.read_bytes() == saved, 'Preview must not save the profile'
        ranges = re.findall(r'controller-binding action=[45].*?center=(\d+) min=\d+ max=(\d+)',state())
        assert len(ranges)==2 and all(int(low)>2500 and int(high)<31000 for low,high in ranges),ranges
        shot('review'); client.click('profile-test-toggle'); wait(r'testing=1 applied=1')
        before = client.inspect('hub-pointer'); axes(x=26000,y=-22000,cx=-24000,cy=23000)
        device('button,0,0,1'); device('button,0,1,1'); wait(r'testing=1 applied=1')
        assert re.search(r'local=([^ ]+)',before)[1] == re.search(r'local=([^ ]+)',client.inspect('hub-pointer'))[1]
        shot('input-test'); device('button,0,0,0'); device('button,0,1,0'); axes()
        device('button,0,6,1'); wait(r'testing=0 applied=1'); device('button,0,6,0')
        client.click('profile-calibration-next'); wait(r'measured=1 .*applied=0')
        client.click('profile-save'); saved = profile.read_bytes()
        client.resize(960,540); shot('profiles-narrow')
        client.click('profile-test'); wait(r'step=3 testing=0 applied=1')
        client.tap('B',frames=40,port=3); wait(r'applied=0'); client.element('profile-name')
        client.click('profile-calibrate'); wait(r'step=0')
        device('detach,0'); wait(r'Controller changed')
        assert 'applied=0' in state(); client.element('profile-name')
        client.click('profile-close'); client.element('edit-controls')
        ui['checks'] = ['native gameplay/buttons/axes/calibration tabs','one toggle per held A press',
            'saved and assigned profile','raw SDL button and signed-axis binding','additional/duplicate binding',
            'Escape and held START cancel capture without changing a binding','Enter commits percentages; invalid input restores the prior value',
            'raw rest and edge capture','no cursor movement while capturing sticks','manual trigger ranges',
            'temporary calibration assignment','test mode suppresses hand and button navigation; START stops it',
            'controller disconnect restores the previous assignment','both window sizes']
        ui['saved_profile'] = profile.name; ui['saved_sha256'] = hashlib.sha256(saved).hexdigest()
        source = client.folder/'appdata'
        close_window(client.process.pid); client.process.wait(timeout=20)
        client = Client(args.exe.resolve(),args.iso.resolve(),source,output/'reopened',
                        'Native profile review',window_size=(960,540),without_drafts=True)
        open_profiles(); select('Native controls review'); wait(r'tap=0 buffer=3 cstick=1 measured=1')
        assert (client.data/'profiles'/profile.name).read_bytes()==saved
        client.click('profile-tab-1'); assert 'controller-binding action=0 count=2' in state(); shot('reopened')
        client.click('profile-delete'); shot('delete-confirm')
        client.click('profile-delete-cancel'); assert (client.data/'profiles'/profile.name).exists()
        client.click('profile-delete'); client.click('profile-delete-yes')
        assert not (client.data/'profiles'/profile.name).exists()
        ui['checks'] += ['saved values and bindings survive restart','delete cancellation and confirmed removal']
    except Exception as error:
        receipt.update(status='FAIL',error=str(error))
        raise
    finally:
        if client and client.process.poll() is None:
            close_window(client.process.pid); client.process.wait(timeout=20)
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
                 ui={},gameplay={},limitations=['Physical GameCube adapter feel','Physical rumble'])
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT']='http://127.0.0.1:1'
    review_ui(args,output,receipt)
    review_gameplay(args,output,receipt)
    receipt['status']='PASS'
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2))
    print(json.dumps(receipt,indent=2))


if __name__=='__main__':main()
