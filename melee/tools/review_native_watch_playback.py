"""Exercise Watch match with two players and an independent practicing viewer.

Runs hidden, muted clients from copied profiles against a private local service.
Checks catch-up state digests, cancellation, re-entry, Results and own-roster return.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import time

from review_native_css_room import Client
from review_native_outfit_matches import choose as choose_outfit,network,select_stage,settings,wait
from review_native_room_rotation import choose,role,room,session
from review_native_watch_transport import watch


def playback(client):
    text=client.inspect('watch-playback')
    found=re.search(r'\[watch-playback\] state=(\d+) phase=(\d+) frame=(\d+) index=(\d+) verified=(\d+) hidden=(\d+) digest=([0-9a-f]+) failed=(\d+)',text)
    assert found,text
    value={k:int(v,16 if k=='digest' else 10) for k,v in zip(
        ['state','phase','frame','index','verified','hidden','digest','failed'],found.groups())}
    assert not value['failed'],value
    return value


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ['exe','iso','profile','families','output']:
        p.add_argument('--'+key,type=Path,required=True)
    p.add_argument('--endpoint',required=True)
    p.add_argument('--trace',action='store_true')
    p.add_argument('--paired',action='store_true')
    args=p.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT']=args.endpoint
    if args.trace:os.environ['FREAK_MELEE_NETPLAY_CHECKSUM_TRACE']='1'
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    base=output/'base';shutil.copytree(args.profile,base,ignore=shutil.ignore_patterns('projects'));settings(base)
    families={c['label']:c for c in json.loads(args.families.read_text())['cases']}
    fighter=families['fox']['parent']
    receipt={'status':'RUNNING','inputs':{str(f.resolve()):hashlib.sha256(f.read_bytes()).hexdigest()
             for f in [Path(__file__),args.exe,args.families]}}
    clients=[]
    try:
        code=''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        for index in range(3):
            c=Client(args.exe.resolve(),args.iso.resolve(),base,output/str(index),
                ['PlayAuthority','PlayOpponent','PlayViewer'][index],window_size=(1280,800),
                without_drafts=True,launch_args=('--netplay-delay',6 if index==1 else 0))
            clients.append(c);c.click('nav-1');c.enter('lobby-code',code);c.click('join-lobby');c.wait_cards(index+1)
        for index,c in enumerate(clients):
            c.wait_cards(3);c.click('lobby-ready')
            if args.paired and index<2:
                label=['zelda','climbers'][index]
                choose_outfit(c,families[label]['parent'],'Coordinated '+label,0)
            else:choose(c,fighter,0)
            c.click('play-ready');c.element('play-selection')
        authority,opponent,viewer=clients
        viewer.click('play-lobby');viewer.click('nav-1')
        viewer_id=room(viewer)['local'];role(viewer,viewer_id,0,'spectating')
        wait(lambda: room(authority)['members'][viewer_id]['active']==0,'viewer role')
        viewer.click('lobby-ready')
        wait(lambda: 'enabled=1' in authority.inspect('play-start'),'ready fighters')
        authority.click('play-start');select_stage(authority)
        active_sessions=[session(c,1) for c in clients[:2]]
        assert active_sessions[0][:2]==active_sessions[1][:2]
        wait(lambda: network(authority)['frame']>180,'match before explicit viewing')
        receipt['before_request']=watch(viewer)
        assert receipt['before_request']['practice']==1 and receipt['before_request']['state']==0
        viewer.click('play-watch')
        receipt['first_playback']=wait(lambda: s if (s:=playback(viewer))['verified']>120 else None,'verified viewer catch-up',90)
        assert receipt['first_playback']['hidden']>0
        receipt['viewing_screenshot']=str(viewer.screenshot())
        viewer.command('tap',0,'B')
        wait(lambda: '[css-selection] ready=' in viewer.inspect('css-selection'),'viewer CSS after cancellation')
        receipt['returned_css']=viewer.inspect('css-selection')
        viewer.click('nav-1');viewer.click('lobby-ready');viewer.element('play-ready')
        choose(viewer,fighter,0);viewer.click('play-ready');viewer.element('play-selection')
        assert watch(viewer)['practice']==1
        viewer.click('play-watch')
        receipt['second_playback']=wait(lambda: s if (s:=playback(viewer))['verified']>120 else None,'second verified viewer catch-up',90)
        receipt['network']=[network(c) for c in clients[:2]]
        assert all(n['checksums'] and not n['failed'] for n in receipt['network'])
        assert sum(n['restores'] for n in receipt['network'])>0
        opponent.command('hold',0,'sx=80')
        wait(lambda: all('[netplay] Compared ' in (c.folder/'stdout.log').read_text(errors='replace') for c in clients[:2]),'source Results',90)
        opponent.command('hold',0,'sx=0')
        receipt['final']=wait(lambda: s if (s:=playback(viewer))['state']==5 else None,'viewer Results',90)
        receipt['complete']=watch(viewer)
        source=room(authority)['stream'];receipt['source']=source
        assert receipt['complete']['digest']==source['digest'] and receipt['complete']['finished']==1
        assert receipt['final']['index']==source['records']
        endpoints=[re.findall(r'\[netplay\] Endpoint frame (\d+): fighter digest ([0-9a-f]+), RNG ([0-9a-f]+)',
            (c.folder/'stdout.log').read_text(errors='replace'))[-1] for c in clients[:2]]
        assert endpoints[0]==endpoints[1]
        expected=((int(endpoints[0][1],16)^int(endpoints[0][2],16))*1099511628211)&((1<<64)-1)
        assert receipt['final']['digest']==expected
        receipt['endpoints']=endpoints
        receipt['results_screenshot']=str(viewer.screenshot())
        def return_from_results():
            if '[css-selection] ready=' in viewer.inspect('css-selection'):return True
            viewer.command('tap',0,'START','frames=6')
            return False
        wait(return_from_results,'viewer own CSS after Results',60)
        receipt['final_css']=viewer.inspect('css-selection')
        for c in clients:
            text=(c.folder/'stdout.log').read_text(errors='replace')+c.stderr.read_text(errors='replace')
            assert not re.search(r'Checksum mismatch|\[netplay\] FAILED:|\[watch-playback\] FAILED:|Fighter source VM failed|native panic stack',text),text[-3000:]
        receipt['status']='PASS explicit Watch match, verified catch-up, cancel/rejoin, Results and own CSS return'
        print(receipt['status'],flush=True)
    except Exception as error:
        receipt.update(status='FAIL',error=repr(error))
        for c in clients:
            if c.process.poll() is None:c.screenshot()
        raise
    finally:
        for c in clients:c.stop()
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))


if __name__=='__main__':main()
