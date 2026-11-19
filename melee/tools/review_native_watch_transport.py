"""Check live spectator history transport while a third native client practices.

Uses copied profiles, hidden/muted clients and an isolated signaling service.
This checks transport through the diagnostic request API; viewer playback and
the final Watch match UI have separate acceptance.
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
from review_native_outfit_matches import network,select_stage,settings,wait
from review_native_room_rotation import choose,click_card,room,session


def watch(client):
    text=client.inspect('room-state')
    found=re.search(r'\[room-watch\] state=(\d+) session=([0-9a-f]+) records=(\d+) available=(\d+) metadata=(\d+) finished=(\d+) digest=([0-9a-f]+) seed=([0-9a-f]+) senders=(\d+) practice=(\d+) failure=([^\r\n]*)',text)
    assert found,text
    result={key:int(value,16 if key in ['session','digest','seed'] else 10)
            for key,value in zip(['state','session','records','available','metadata','finished','digest','seed','senders','practice'],found.groups()[:-1])}
    result['failure']=found.groups()[-1]
    assert result['state']!=4,result
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ['exe','iso','profile','families','output']:
        parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--endpoint',required=True)
    args=parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT']=args.endpoint
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    base=output/'base';shutil.copytree(args.profile,base,ignore=shutil.ignore_patterns('projects'));settings(base)
    fighter=next(c['parent'] for c in json.loads(args.families.read_text())['cases'] if c['label']=='fox')
    receipt={'status':'RUNNING','inputs':{str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest()
             for p in [Path(__file__),args.exe,args.families,Path(__file__).with_name('review_native_room_rotation.py')]}}
    clients=[]
    try:
        code=''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        for index in range(3):
            c=Client(args.exe.resolve(),args.iso.resolve(),base,output/str(index),
                     ['WatchAuthority','WatchPlayer','WatchViewer'][index],window_size=(1280,800),
                     without_drafts=True,launch_args=('--netplay-delay',6 if index==1 else 0))
            clients.append(c);c.click('nav-1');c.enter('lobby-code',code);c.click('join-lobby');c.wait_cards(index+1)
        authority,player,viewer=clients
        viewer_id=room(viewer)['local'];click_card(viewer,2,0)
        wait(lambda: room(authority)['members'][viewer_id]['active']==0,'spectator role')
        for c in [viewer, authority, player]:
            choose(c,fighter,0);c.tap('START',frames=4)
        wait(lambda: watch(viewer)['practice']==1,'spectator practice')
        select_stage(authority)
        sessions=[session(c,1) for c in clients[:2]];assert sessions[0][:2]==sessions[1][:2]
        wait(lambda: network(authority)['frame']>150,'active gameplay')
        receipt['before_request']=watch(viewer)
        assert receipt['before_request']['state']==0 and receipt['before_request']['practice']==1
        assert '[netplay] Session ' not in (viewer.folder/'stdout.log').read_text(errors='replace')
        authority.command('room-watch','start');assert watch(authority)['state']==0
        viewer.command('room-watch','start')
        live=wait(lambda: s if (s:=watch(viewer))['records']>50 else None,'live viewing prefix',45)
        assert live['practice']==1 and live['session']==int(sessions[0][0],16)
        receipt['first_prefix']=live
        viewer.command('room-watch','stop');assert watch(viewer)['state']==0
        wait(lambda: watch(authority)['senders']==0,'cancelled viewer route')
        frame=network(authority)['frame']
        wait(lambda: network(authority)['frame']>frame+20,'gameplay after cancellation')
        viewer.command('room-watch','start')
        receipt['second_prefix']=wait(lambda: s if (s:=watch(viewer))['records']>live['records'] else None,'rejoined viewer catch-up',45)
        receipt['network']=[network(c) for c in clients[:2]]
        assert all(n['checksums'] and not n['failed'] for n in receipt['network'])
        assert sum(n['restores'] for n in receipt['network'])>0
        receipt['screenshots']=[str(c.screenshot()) for c in clients]
        player.command('hold',0,'sx=80')
        wait(lambda: all('[netplay] Compared ' in (c.folder/'stdout.log').read_text(errors='replace') for c in clients[:2]),'match completion',90)
        player.command('hold',0,'sx=0')
        # Keep both players in Results. The room must continue sending its final
        # marker without requiring either player to return to CSS first.
        complete=wait(lambda: s if (s:=watch(viewer))['state']==3 else None,'complete viewer history during Results',30)
        source=room(authority)['stream']
        assert complete['records']==source['records'] and complete['digest']==source['digest']
        assert complete['seed']==source['seed'] and complete['finished']==1 and complete['practice']==1
        assert watch(authority)['senders']==0
        assert all('[css-selection] inactive' in c.inspect('css-selection') for c in clients[:2])
        endpoints=[re.findall(r'\[netplay\] Endpoint frame (\d+): fighter digest ([0-9a-f]+), RNG ([0-9a-f]+)',
                             (c.folder/'stdout.log').read_text(errors='replace'))[-1] for c in clients[:2]]
        assert endpoints[0]==endpoints[1]
        receipt.update(complete=complete,source=source,endpoints=endpoints)
        viewer.command('room-watch','stop');assert watch(viewer)['state']==0
        for c in clients:
            text=(c.folder/'stdout.log').read_text(errors='replace')+c.stderr.read_text(errors='replace')
            assert not re.search(r'Checksum mismatch|\[netplay\] FAILED:|Fighter source VM failed|native panic stack',text),text[-2000:]
        receipt['status']='PASS live native history catch-up, cancellation/rejoin, practice isolation and Results delivery'
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
