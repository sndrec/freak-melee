"""Exercise edited source-owned items under corrected multiplayer inputs.

Uses two hidden, muted clients, copied profiles and a loopback room service.
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
from urllib.parse import urlsplit

from review_native_css_room import Client
from review_native_outfit_matches import network, select_stage, settings, wait
from review_native_room_rotation import choose, session


def items(client):
    text=client.inspect('netplay-state')
    assert 'failed=0' in text,text
    found=re.findall(r'\[netplay-item\] index=(\d+) id=(\d+) kind=(-?\d+) state=(-?\d+) holder=(-?\d+) scripted=(\d+) recipe=(\d+) age=(\d+) pos=([\d.-]+),([\d.-]+) vel=([\d.-]+),([\d.-]+)',text)
    keys=['index','id','kind','state','holder','scripted','recipe','age','x','y','vx','vy']
    rows=[{k:(float(v) if i>=8 else int(v)) for i,(k,v) in enumerate(zip(keys,row))} for row in found]
    count=re.search(r'\[netplay-items\] count=(\d+)',text)
    assert count and len(rows)==int(count[1]),text
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ['exe','iso','profile','families','output']:p.add_argument('--'+key,type=Path,required=True)
    p.add_argument('--endpoint',required=True);args=p.parse_args()
    assert urlsplit(args.endpoint).hostname in ['127.0.0.1','localhost','::1']
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT']=args.endpoint
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    base=output/'base';shutil.copytree(args.profile,base,ignore=shutil.ignore_patterns('projects'));settings(base)
    families={c['label']:c for c in json.loads(args.families.read_text())['cases']}
    receipt={'status':'RUNNING','cycles':[],'inputs':{str(f.resolve()):hashlib.sha256(f.read_bytes()).hexdigest() for f in [Path(__file__),args.exe,args.families]}}
    clients=[]
    try:
        code=''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        for index,label in enumerate(['fox','zelda']):
            c=Client(args.exe.resolve(),args.iso.resolve(),base,output/str(index),'ItemPlayer'+str(index),
                without_drafts=True,window_size=(1280,800),launch_args=('--netplay-delay',6 if index else 0))
            clients.append(c);c.click('nav-1');c.enter('lobby-code',code);c.click('join-lobby');c.wait_cards(index+1)
        for c,label in zip(clients,['fox','zelda']):
            c.wait_cards(2);c.click('lobby-ready');choose(c,families[label]['parent'],0)
            c.click('play-ready');c.element('play-selection')
        wait(lambda:'enabled=1' in clients[0].inspect('play-start'),'ready fighters')
        clients[0].click('play-start');select_stage(clients[0])
        receipt['sessions']=[session(c,1) for c in clients]
        wait(lambda:network(clients[0])['frame']>240,'normal gameplay')
        assert all(not items(c) for c in clients)
        for cycle in range(2):
            entry={'before':[network(c) for c in clients]}
            for c in clients:c.command('tap',0,'B','frames=8')
            def held():
                states=[items(c) for c in clients]
                return states if all(sum(i['scripted'] and i['holder']>=0 for i in state)==2 for state in states) else None
            entry['held']=wait(held,'both script-owned items held',20)
            entry['held_screenshots']=[str(c.screenshot()) for c in clients]
            # Wait for the stock special to finish, then use the ordinary item
            # throw input. Each edited recipe adds upward speed in on thrown.
            start=network(clients[0])['frame']
            wait(lambda:network(clients[0])['frame']>start+100,'special recovery')
            for c in clients:c.command('tap',0,'A','sy=80','frames=8')
            def thrown():
                states=[items(c) for c in clients]
                return states if all(sum(i['scripted'] and i['holder']==-1 and i['age']>0 for i in state)>=2 for state in states) else None
            entry['thrown']=wait(thrown,'both items thrown',15)
            wait(lambda:all(not items(c) for c in clients),'script-owned item expiry',15)
            entry['after']=[network(c) for c in clients]
            receipt['cycles'].append(entry)
            assert all(b['checksums']>a['checksums'] for a,b in zip(entry['before'],entry['after']))
            assert sum(b['restores']-a['restores'] for a,b in zip(entry['before'],entry['after']))>0
        clients[1].command('hold',0,'sx=80')
        wait(lambda:all('[netplay] Compared ' in (c.folder/'stdout.log').read_text(errors='replace') for c in clients),'Results',90)
        clients[1].command('hold',0,'sx=0')
        endpoints=[re.findall(r'Endpoint frame (\d+): fighter digest ([0-9a-f]+), RNG ([0-9a-f]+)',(c.folder/'stdout.log').read_text(errors='replace'))[-1] for c in clients]
        assert endpoints[0]==endpoints[1];receipt['endpoints']=endpoints
        for c in clients:
            text=(c.folder/'stdout.log').read_text(errors='replace')+c.stderr.read_text(errors='replace')
            assert not re.search(r'Checksum mismatch|\[netplay\] FAILED:|Fighter source VM failed|native panic stack',text),text[-2000:]
        receipt['status']='PASS source-owned item creation, throwing, expiry and recreation with corrected inputs'
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
