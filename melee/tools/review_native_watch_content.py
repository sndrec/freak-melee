"""Check missing spectator fighter/outfit downloads during an active match.

Launches hidden, muted clients with copied profiles on an isolated room service.
The third PC joins late and owns only unrelated practice content.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import struct
import time

from review_native_css_room import Client
from review_native_outfit_matches import choose as choose_outfit,network,select_stage,settings,wait
from review_native_room_rotation import choose,room,session
from review_native_watch_transport import watch
from review_native_watch_playback import playback


def cache(client):
    text=client.inspect('room-state')
    found=re.search(r'\[room-cache\] retained=(\d+) bytes=(\d+) reads=(\d+) requests=(\d+) active_packets=(\d+) choices=(\d+) cached=(\d+) watch_ready=(\d+)',text)
    assert found,text
    return dict(zip(['retained','bytes','reads','requests','active_packets','choices','cached','watch_ready'],map(int,found.groups())))


def remove_match_content(profile,identities,workspace):
    data=(profile/'Freak Melee').resolve();assert data.is_relative_to(workspace)
    for identity in identities:
        for relative in [f'content/installed/{identity}.fmi',f'content/cache/{identity}.fmc']:
            (data/relative).unlink(missing_ok=True)
    path=data/'css-layout.bin';raw=path.read_bytes();count=struct.unpack_from('<H',raw,4)[0]
    entries=[raw[6+48*i:54+48*i] for i in range(count) if raw[14+48*i:46+48*i].hex() not in identities]
    assert entries
    body=raw[:4]+struct.pack('<H',len(entries))+b''.join(entries);digest=2166136261
    for value in body:digest=((digest^value)*16777619)&0xffffffff
    path.write_bytes(body+struct.pack('<I',digest))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ['exe','iso','profile','families','output']:parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--endpoint',required=True)
    parser.add_argument('--playback',action='store_true')
    args=parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT']=args.endpoint
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    base=output/'base';shutil.copytree(args.profile,base,ignore=shutil.ignore_patterns('projects'));settings(base)
    families={c['label']:c for c in json.loads(args.families.read_text())['cases']}
    identities={families[k][field] for k in ['zelda','climbers'] for field in ['parent','outfit']}
    viewer_profile=output/'viewer-profile';shutil.copytree(base,viewer_profile)
    remove_match_content(viewer_profile,identities,output)
    receipt={'status':'RUNNING','required':sorted(identities),'inputs':{str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest()
             for p in [Path(__file__),args.exe,args.families,Path(__file__).with_name('review_native_watch_transport.py')]}}
    clients=[]
    try:
        code=''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        for index,label in enumerate(['zelda','climbers']):
            c=Client(args.exe.resolve(),args.iso.resolve(),base,output/str(index),
                     ['ContentAuthority','ContentPlayer'][index],window_size=(1280,800),without_drafts=True,
                     launch_args=('--netplay-delay',6 if index else 0))
            clients.append(c);c.click('nav-1');c.enter('lobby-code',code);c.click('join-lobby');c.wait_cards(index+1)
        for c,label in zip(clients,['zelda','climbers']):
            c.wait_cards(2);c.click('lobby-ready')
            choose_outfit(c,families[label]['parent'],'Coordinated '+label,0)
            c.click('play-ready');c.element('play-selection')
        authority,player=clients
        wait(lambda: 'enabled=1' in authority.inspect('play-start'),'ready match')
        authority.click('play-start');select_stage(authority)
        sessions=[session(c,1) for c in clients];assert sessions[0][:2]==sessions[1][:2]
        wait(lambda: network(authority)['frame']>150,'running match before viewer joins')
        receipt['before_download']=[cache(c) for c in clients]
        assert all(s['retained']==2 and s['reads']>=2 and not s['active_packets'] for s in receipt['before_download'])
        viewer=Client(args.exe.resolve(),args.iso.resolve(),viewer_profile,output/'2','ColdContentViewer',
                      window_size=(1280,800),without_drafts=True)
        clients.append(viewer)
        for identity in identities:
            assert not (viewer.data/'content/cache'/(identity+'.fmc')).exists()
            assert not (viewer.data/'content/installed'/(identity+'.fmi')).exists()
        print('PASS required match cache is empty before late join',flush=True)
        joined_at=time.monotonic()
        viewer.click('nav-1');viewer.enter('lobby-code',code);viewer.click('join-lobby')
        wait(lambda: room(viewer)['count']==3,'late viewer presence')
        state=room(viewer);assert state['members'][state['local']]['active']==0
        viewer.click('lobby-ready');choose(viewer,families['fox']['parent'],0)
        viewer.click('play-ready');viewer.element('play-selection')
        assert watch(viewer)['practice']==1 and watch(viewer)['state']==0
        if args.playback:viewer.click('play-watch')
        else:viewer.command('room-watch','start')
        wait(lambda: watch(viewer)['records']>50,'live history from current match',45)
        download_started=time.monotonic();samples=[];next_sample=0
        def dependencies():
            nonlocal next_sample
            now=time.monotonic()
            if now>=next_sample:
                samples.append({'seconds':now-download_started,'players':[network(c) for c in clients[:2]]})
                next_sample=now+2
            state=cache(viewer)
            return state if state['watch_ready'] else None
        ready=wait(dependencies,'exact fighter and outfit dependencies',180)
        receipt['download_seconds']=time.monotonic()-download_started
        receipt['download_samples']=samples
        for identity in identities:
            path=viewer.data/'content/cache'/(identity+'.fmc')
            assert hashlib.sha256(path.read_bytes()).hexdigest()==identity
            assert not (viewer.data/'content/installed'/(identity+'.fmi')).exists()
        receipt['content_ready_seconds']=time.monotonic()-joined_at
        receipt['content_ready']=ready;receipt['during_match']=[cache(c) for c in clients[:2]]
        if args.playback:
            receipt['playing']=wait(lambda: s if (s:=playback(viewer))['verified']>120 else None,'cold viewer playback',180)
        for before,after in zip(receipt['before_download'],receipt['during_match']):
            assert after['reads']==before['reads'] and after['requests']>=2 and after['active_packets']>0
        receipt['network']=[network(c) for c in clients[:2]]
        assert all(n['checksums'] and not n['failed'] for n in receipt['network'])
        assert sum(n['restores'] for n in receipt['network'])>0
        receipt['screenshots']=[str(c.screenshot()) for c in clients]
        print('PASS exact dependencies verified without source file rereads or installation',flush=True)
        player.command('hold',0,'sx=80')
        wait(lambda: all('[netplay] Compared ' in (c.folder/'stdout.log').read_text(errors='replace') for c in clients[:2]),'match end',90)
        player.command('hold',0,'sx=0')
        complete=wait(lambda: s if (s:=watch(viewer))['state']==3 else None,'finished history',180 if args.playback else 45)
        source=room(authority)['stream']
        assert complete['records']==source['records'] and complete['digest']==source['digest']
        assert complete['seed']==source['seed'] and cache(viewer)['watch_ready']==1
        assert complete['practice']==(0 if args.playback else 1)
        receipt.update(complete=complete,source=source)
        if args.playback:
            receipt['final_playback']=wait(lambda: s if (s:=playback(viewer))['state']==5 else None,'cold viewer Results',90)
            assert receipt['final_playback']['index']==source['records']
            assert receipt['final_playback']['digest']==source['tail_state']
            receipt['viewer_results']=str(viewer.screenshot())
            def return_from_results():
                if '[css-selection] ready=' in viewer.inspect('css-selection'):return True
                viewer.command('tap',0,'START','frames=6')
                return False
            wait(return_from_results,'cold viewer own CSS',60)
            layout=(viewer.data/'css-layout.bin').read_bytes()
            count=struct.unpack_from('<H',layout,4)[0]
            tiles={layout[14+48*i:46+48*i].hex() for i in range(count)}
            assert not tiles.intersection(identities)
            assert all(not (viewer.data/'content/installed'/(i+'.fmi')).exists() for i in identities)
        else:viewer.command('room-watch','stop')
        for c in clients:
            text=(c.folder/'stdout.log').read_text(errors='replace')+c.stderr.read_text(errors='replace')
            assert not re.search(r'Checksum mismatch|\[netplay\] FAILED:|\[watch-playback\] FAILED:|Fighter source VM failed|native panic stack',text),text[-2000:]
        receipt['status']='PASS cold late spectator receives exact source families and outfits during gameplay, remains cache-only and receives complete history'
        if args.playback:receipt['status']+='; verified playback, Results and own roster return'
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
