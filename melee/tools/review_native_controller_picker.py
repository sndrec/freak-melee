"""Review controller picking, swapping, hotplug and persistence in hidden, muted clients."""
import argparse
import json
import os
from pathlib import Path
import re
import time

from review_native_css_room import Client
from review_native_release import close_window


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','iso','profile','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args(); output=args.output.resolve(); output.mkdir(parents=True,exist_ok=False)
    for service in ('SERVICE','CHAT','COMMUNITY'):
        os.environ['FREAK_MELEE_'+service+'_ENDPOINT']='http://localhost:1'
    os.environ['FREAK_MELEE_MENU_MOTION']='0'
    os.environ['SDL_JOYSTICK_HIDAPI']='0'
    os.environ['SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS']='1'
    receipt=dict(status='RUNNING',checks=[],screenshots={}); client=None

    def launch(profile,folder):
        return Client(args.exe.resolve(),args.iso.resolve(),profile,output/folder,
                      'Controller picker review',window_size=(1440,900),without_drafts=True)
    def open_profiles():
        client.click('nav-6'); client.click('settings-category-4'); client.click('edit-controls')
    def device(command):
        client.command('device',command); time.sleep(.2)
    def devices():
        text=client.inspect('controller-picker')
        return {name:(int(instance),int(port)) for instance,port,name in
                re.findall(r'controller-device index=\d+ instance=(\d+) port=(-?\d+) name=([^\r\n]+)',text)}
    def pad(number): return 'Freak Melee virtual controller '+str(number)
    def choose(port,number,held=False):
        client.click('profile-port-'+str(port)); client.click('profile-controller')
        instance=devices()[pad(number)][0]
        if held:
            client.command('ui-move',*client.element('controller-device-'+str(instance)))
            client.tap('A',frames=40)
        else: client.click('controller-device-'+str(instance))
        client.element('profile-controller')
    def shot(name): receipt['screenshots'][name]=str(client.screenshot())
    def close():
        close_window(client.process.pid); assert client.process.wait(timeout=20)==0

    try:
        client=launch(args.profile.resolve(),'first'); open_profiles()
        client.click('profile-new'); client.enter('profile-name','Picker A'); client.command('ui-key','enter')
        client.click('profile-port-2')
        for _ in range(3): client.click('profile-buffer')
        client.click('profile-save')
        client.click('profile-new'); client.enter('profile-name','Picker B'); client.command('ui-key','enter')
        client.click('profile-port-3')
        for _ in range(6): client.click('profile-buffer')
        client.click('profile-save')
        shot('profiles')
        # Reset, Delete and Use defaults have the same width, with Controller
        # between Delete and Use defaults. The UI driver queries actual bounds.
        widths=[]
        for name in ('profile-reset','profile-delete','profile-unassign'):
            text=client.inspect(name); widths.append(float(re.search(r'size=([\d.]+),',text)[1]))
        assert max(widths)-min(widths)<1,widths
        receipt['checks'].append('compact footer and separate Controller button')
        device('attach,0'); device('attach,1')
        choose(2,1); choose(3,2)
        client.click('profile-port-2'); client.click('profile-controller'); before=devices()
        assert before[pad(1)][1]==2 and before[pad(2)][1]==3,before
        shot('picker'); client.click('controller-picker-back')
        state=client.inspect('controller-profile'); assert 'name=Picker B' in state,state
        choose(2,2,held=True); client.click('profile-controller'); swapped=devices()
        assert swapped[pad(2)][1]==2 and swapped[pad(1)][1]==3,swapped
        client.click('controller-picker-back')
        assert 'assigned=Picker A' in client.inspect('controller-profile')
        client.click('profile-port-3'); assert 'assigned=Picker B' in client.inspect('controller-profile')
        receipt['checks'] += ['two controllers selected through the UI','occupied slots swap without moving their profiles','held A selects once','Back preserves unsaved profile selection']
        client.click('profile-port-2'); client.click('profile-controller')
        device('detach,1'); assert pad(2) not in devices()
        device('attach,1'); restored=devices(); assert restored[pad(2)][1]==2,restored
        client.resize(960,540); shot('picker-narrow'); client.click('controller-picker-back')
        receipt['checks'].append('hotplug list refresh and saved slot reconnect')
        previous=client.folder/'appdata'; close()
        client=launch(previous,'reopened'); open_profiles()
        # Reverse attachment order to check that device identity, rather than
        # enumeration order, selects the remembered player slots.
        device('attach,1'); device('attach,0')
        client.click('profile-port-2'); client.click('profile-controller'); restored=devices()
        assert restored[pad(2)][1]==2 and restored[pad(1)][1]==3,restored
        client.click('controller-picker-none'); client.click('profile-controller')
        cleared=devices(); assert cleared[pad(2)][1]==-1 and cleared[pad(1)][1]==3,cleared
        client.click('controller-picker-back')
        previous=client.folder/'appdata'; close()
        client=launch(previous,'cleared'); open_profiles(); device('attach,0'); device('attach,1')
        client.click('profile-port-2'); client.click('profile-controller'); restored=devices()
        assert restored[pad(2)][1]==-1 and restored[pad(1)][1]==3,restored
        device('detach,0'); device('detach,1'); assert not devices(); shot('empty-picker')
        client.command('ui-key','escape'); client.element('profile-controller')
        receipt['checks'] += ['assignments survive restart with reverse connection order','No controller persists without affecting other slots','empty picker and Escape return']
        receipt['status']='PASS'
    finally:
        if client and client.process.poll() is None: close()
        (output/'review.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))


if __name__=='__main__': main()
