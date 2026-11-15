"""Review native rule, outfit and package menus using an isolated installed roster."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import time
from review_native_css_room import Client
from review_native_release import close_window


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','iso','profile','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--outfit-fighter',required=True,help='Installed fighter ID with a compatible outfit in this fixture')
    args=parser.parse_args(); output=args.output.resolve(); output.mkdir(parents=True,exist_ok=False)
    for service in ('SERVICE','CHAT','COMMUNITY'): os.environ['FREAK_MELEE_'+service+'_ENDPOINT']='http://localhost:1'
    os.environ['FREAK_MELEE_MENU_MOTION']='0'
    receipt={'status':'RUNNING','checks':[],'screenshots':{},'inputs':{str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in (args.exe.resolve(),Path(__file__).resolve())}}; client=None
    def shot(name): receipt['screenshots'][name]=str(client.screenshot())
    def inspect(name,pattern):
        text=client.inspect(name); assert re.search(pattern,text),(pattern,text); return text
    def game(): client.click('nav-6'); client.click('settings-category-2')
    try:
        client=Client(args.exe.resolve(),args.iso.resolve(),args.profile.resolve(),output/'client','Options suite',window_size=(1440,900))
        game(); client.click('settings-rules')
        client.click('rules-row-0-right')
        for _ in range(2): client.click('rules-row-1-right'); client.click('rules-row-3-right')
        client.click('rules-row-6')
        for _ in range(8): client.click('rules-row-0-right')
        inspect('match-rules',r'mode=1 time=2 stock=5 timer=8 damage=120')
        client.command('ui-move',*client.element('rules-row-1-right')); before=client.inspect('match-rules')
        client.tap('A',frames=45); after=client.inspect('match-rules')
        assert int(re.search(r'friendly=(\d+)',before)[1]) != int(re.search(r'friendly=(\d+)',after)[1])
        shot('additional-rules'); client.click('rules-close'); shot('rules'); client.click('rules-row-5'); client.click('rules-item-none'); inspect('match-rules',r'freq=255 mask=0')
        client.command('ui-move',*client.element('rules-item-0')); client.tap('A',frames=45); inspect('match-rules',r'mask=1 ')
        client.click('rules-items'); inspect('match-rules',r'freq=0 mask=1 ')
        client.click('rules-item-all'); inspect('match-rules',r'mask=efffffff '); shot('item-switch')
        client.tap('B',frames=40); inspect('match-rules',r'page=27 ')
        client.click('rules-close'); client.element('settings-rules'); client.click('settings-rules')
        inspect('match-rules',r'mode=1 time=2 stock=5 timer=8 damage=120'); client.click('rules-close')
        receipt['checks'] += ['stock animated rules confirm immediately','held A toggles once','item frequency and 31 item switches','rule persistence after reopen','held B returns one level']
        client.click('settings-packages'); text=inspect('packages',r'count=[1-9]'); shot('packages')
        assert 'kind=2' in text or 'kind=1' in text
        client.click('collection-next'); shot('packages-next'); client.click('collection-prev')
        client.click('collection-send-0'); client.element('collection-confirm'); shot('package-recipient-empty')
        assert 'enabled=0' in client.inspect('collection-confirm'); client.click('collection-close')
        client.click('collection-drafts'); shot('drafts'); client.click('collection-close'); client.element('settings-packages')
        receipt['checks'] += ['installed packages and pagination','draft tab','no recipient disables Send','package Back preserves Game page']
        client.click('settings-back'); client.click('settings-back'); client.click('local-practice'); time.sleep(.6); client.state()
        # Select the first installed custom tile through the ordinary CSS hand.
        layout=(client.data/'css-layout.bin').read_bytes(); total=struct.unpack_from('<H',layout,4)[0]
        index=[layout[14+48*i:46+48*i].hex() for i in range(total)].index(args.outfit_fighter)
        state,text=client.state(); tile=re.search(r'\[css-tile\] index='+str(index)+r' center=([\d.-]+),([\d.-]+) custom=1',text); assert tile,text
        if state['hand'] != 1: client.tap('B',frames=3)
        x,y=map(float,tile.groups()); client.move(x-2.7,y+2); client.tap('A',frames=3)
        _,selected_css=client.state(); assert 'chosen=1' in selected_css,selected_css
        client.move(-1,26); client.tap('A',frames=3); inspect('match-rules',r'css=1 '); client.click('rules-close'); client.state()
        client.tap('B',frames=35); game(); client.click('settings-costumes')
        costume=inspect('costumes',r'port=0 '); shot('costumes')
        entries=[]
        for i in range(12):
            text=client.inspect('costume-entry-'+str(i))
            if 'origin=' in text: entries.append(i)
        if entries:
            client.command('ui-move',*client.element('costume-entry-'+str(entries[0]))); client.tap('A',frames=45)
            selected=inspect('costumes',r'status=Outfit selected\.')
            assert not re.search(r'outfit=0{64}',selected.split('costume-player port=0',1)[1].split('\n')[0])
            client.click('costume-stock'); inspect('costumes',r'status=Default outfit selected\.')
            receipt['checks'].append('compatible family outfit selected and reset through native menu')
        else: raise AssertionError('Use a fixture with a compatible installed outfit')
        client.resize(960,540); shot('costumes-narrow'); client.click('costume-close'); client.click('settings-rules'); shot('rules-narrow')
        client.click('rules-row-5'); shot('items-narrow'); client.click('rules-close'); client.click('rules-close'); client.click('settings-packages'); client.click('collection-installed'); shot('packages-narrow')
        receipt['checks'] += ['CSS Match rules returns to CSS','two window sizes','ordinary controller-hand navigation']
        previous=client.folder/'appdata'; close_window(client.process.pid); client.process.wait(timeout=20)
        client=Client(args.exe.resolve(),args.iso.resolve(),previous,output/'reopened','Options suite',window_size=(960,540))
        game(); client.click('settings-rules'); inspect('match-rules',r'mode=1 time=2 stock=5 timer=8 damage=120')
        receipt['checks'].append('saved rules survive process restart'); receipt['status']='PASS'
    finally:
        if client and client.process.poll() is None: close_window(client.process.pid); client.process.wait(timeout=20)
        (output/'review.json').write_text(json.dumps(receipt,indent=2))
    print('PASS native Options suite')

if __name__=='__main__': main()
