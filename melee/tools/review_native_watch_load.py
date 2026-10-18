"""Check two cold viewers and private lobby chat during a rollback match.

All game clients are hidden, muted and use copied profiles. Chat is sent only
to a fresh room on a loopback service. No public service is contacted.
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
from urllib.parse import urlencode, urlsplit

from websockets.sync.client import connect

from review_native_css_room import Client
from review_native_lobby_chat import packet
from review_native_outfit_matches import choose as choose_outfit, costumes, network, select_stage, settings, wait
from review_native_room_rotation import choose, room, session
from review_native_watch_content import cache, remove_match_content
from review_native_watch_playback import playback
from review_native_watch_transport import watch


def chat_state(client, text=None):
    value=client.inspect('lobby-chat-state')
    assert 'connected=1 ready=1' in value,value
    if text is not None:assert text in value,value
    return value


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['exe','iso','profile','families','output']:
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--endpoint',required=True)
    parser.add_argument('--shared-eight',action='store_true')
    args=parser.parse_args()
    endpoint=urlsplit(args.endpoint)
    assert endpoint.hostname in ['127.0.0.1','localhost','::1'],'This test posts chat; a loopback service is required.'
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT']=args.endpoint
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    base=output/'base';shutil.copytree(args.profile,base,ignore=shutil.ignore_patterns('projects'));settings(base)
    families={c['label']:c for c in json.loads(args.families.read_text())['cases']}
    labels=['climbers','climbers'] if args.shared_eight else ['zelda','climbers']
    local_count=2 if args.shared_eight else 1
    identities={families[k][field] for k in labels for field in ['parent','outfit']}
    cold=output/'cold';shutil.copytree(base,cold);remove_match_content(cold,identities,output)
    receipt={'status':'RUNNING','shared_eight':args.shared_eight,'required':sorted(identities),'samples':[],'messages':[],
             'inputs':{str(p.resolve()):hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in [Path(__file__),args.exe,args.families]}}
    clients=[];socket=None
    try:
        code=''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        for index,label in enumerate(labels):
            client=Client(args.exe.resolve(),args.iso.resolve(),base,output/str(index),
                'LoadFighter'+str(index),window_size=(1280,800),without_drafts=True,
                launch_args=('--netplay-delay',6 if index else 0))
            clients.append(client)
            if args.shared_eight:client.command('hold',1,'sx=0')
            client.click('nav-1')
            if args.shared_eight:client.click('local-players-2')
            client.enter('lobby-code',code)
            client.click('join-lobby');client.wait_cards((index+1)*local_count)
        authority,opponent=clients
        for client,label in zip(clients,labels):
            client.wait_cards(2*local_count);client.click('lobby-ready')
            for port in range(local_count):
                choose_outfit(client,families[label]['parent'],'Coordinated '+label,port)
            client.click('play-ready');client.element('play-selection')
        wait(lambda:'enabled=1' in authority.inspect('play-start'),'ready fighters')
        authority.click('play-start');select_stage(authority)
        sessions=[session(c,1) for c in clients];assert sessions[0][:2]==sessions[1][:2]
        wait(lambda:network(authority)['frame']>150,'active match')
        if args.shared_eight:
            expected=[dict(families['climbers'],variant=7)]*4
            receipt['actors']=[costumes(c,expected) for c in clients]
        receipt['retained_before']=[cache(c) for c in clients]
        for index in range(2,4):
            client=Client(args.exe.resolve(),args.iso.resolve(),cold,output/str(index),
                'LoadViewer'+str(index-1),window_size=(1280,800),without_drafts=True)
            clients.append(client);client.click('nav-5');client.click('local-practice')
            choose(client,families['fox']['parent'],0)
            client.click('play-ready');client.element('play-selection')
            client.click('play-lobby');client.click('nav-1');client.enter('lobby-code',code)
        # Prepare both local practice scenes before either late join starts the
        # automatic remote cache. Otherwise the first download can finish while
        # the second viewer is still starting up.
        for index,client in enumerate(clients[2:],3):
            client.click('join-lobby')
            wait(lambda:room(client)['count']==index,'late viewer presence')
            presence=room(client);assert not presence['members'][presence['local']]['active']
            client.click('lobby-ready');client.element('play-selection')
            assert watch(client)['practice']==1 and not watch(client)['state']
            for identity in identities:
                assert not (client.data/'content/cache'/(identity+'.fmc')).exists()
        viewers=clients[2:]
        # This fifth member has no gameplay seats and only exercises private chat.
        query=urlencode(dict(peer='load_chat_'+secrets.token_hex(6),name='Local load sender',
                            players=0,active=0,waiting=0,build='freak-melee-native-dev-cache'))
        websocket_endpoint=args.endpoint.replace('http://','ws://').replace('https://','wss://')
        socket=connect(f'{websocket_endpoint}/v1/rooms/{code}/connect?{query}',max_queue=512,close_timeout=1)
        assert socket.recv(timeout=10)[5]==0x81
        assert socket.recv(timeout=10)[5]==0x82
        socket.send(packet(9))
        while socket.recv(timeout=10)[5]!=0x87:pass
        for client in clients:chat_state(client)
        began=time.monotonic()
        for viewer in viewers:viewer.click('play-watch')
        receipt['started_seconds']=time.monotonic()-began
        next_message=next_sample=0;ready_times=[None,None];last_message=''
        while time.monotonic()-began<150:
            now=time.monotonic()-began
            if now>=next_message:
                number=len(receipt['messages'])+1
                last_message=f'Local concurrent load {number:02}: '+'private lobby traffic '*8
                encoded=last_message.encode();socket.send(packet(8,struct.pack('>IH',number,len(encoded))+encoded))
                deadline=time.monotonic()+5
                while True:
                    message=socket.recv(timeout=max(.01,deadline-time.monotonic()))
                    assert message[5]!=0x88,message
                    if message[5]==0x85 and encoded in message:break
                    assert time.monotonic()<deadline,'chat acknowledgment timeout'
                receipt['messages'].append({'seconds':now,'text':last_message,'ack_seconds':time.monotonic()-began-now})
                next_message=now+2.5
            if now>=next_sample:
                states=[cache(c) for c in clients]
                receipt['samples'].append({'seconds':now,'players':[network(c) for c in clients[:2]],
                                           'cache':states,'viewers':[playback(c) for c in viewers]})
                for i,state in enumerate(states[2:]):
                    if state['watch_ready'] and ready_times[i] is None:ready_times[i]=now
                next_sample=now+2
            if all(value is not None for value in ready_times) and len(receipt['messages'])>=10:
                if all(playback(v)['verified']>120 for v in viewers):break
            time.sleep(.1)
        else:raise AssertionError('Concurrent viewing did not become ready')
        receipt['ready_seconds']=ready_times
        receipt['chat_delivery']=[chat_state(c,last_message) for c in clients]
        assert all(all(m['text'] in state for m in receipt['messages']) for state in receipt['chat_delivery'])
        receipt['retained_after']=[cache(c) for c in clients[:2]]
        for before,after in zip(receipt['retained_before'],receipt['retained_after']):
            assert before['reads']==after['reads']==2
        assert sum(s['requests'] for s in receipt['retained_after'])>=len(viewers)*len(identities)
        for viewer in viewers:
            for identity in identities:
                path=viewer.data/'content/cache'/(identity+'.fmc')
                assert hashlib.sha256(path.read_bytes()).hexdigest()==identity
                assert not (viewer.data/'content/installed'/(identity+'.fmi')).exists()
        assert any(any(s['cache'][i]['requests']>=2 for i in range(2)) and
                   all(not c['watch_ready'] for c in s['cache'][2:]) for s in receipt['samples']), 'No overlapping downloads observed'
        receipt['playing_screenshots']=[str(v.screenshot()) for v in viewers]
        opponent.command('hold',0,'sx=80')
        if args.shared_eight:
            opponent.command('hold',1,'sx=80');authority.command('hold',1,'sx=-80')
        wait(lambda:all('[netplay] Compared ' in (c.folder/'stdout.log').read_text(errors='replace') for c in clients[:2]),'source Results',90)
        opponent.command('hold',0,'sx=0')
        if args.shared_eight:
            opponent.command('hold',1,'sx=0');authority.command('hold',1,'sx=0')
        source=room(authority)['stream'];receipt['source']=source
        receipt['final']=[wait(lambda:s if (s:=playback(v))['state']==5 else None,'viewer Results',90) for v in viewers]
        receipt['received']=[watch(v) for v in viewers]
        endpoints=[re.findall(r'Endpoint frame (\d+): fighter digest ([0-9a-f]+), RNG ([0-9a-f]+)',
                             (c.folder/'stdout.log').read_text(errors='replace'))[-1] for c in clients[:2]]
        assert endpoints[0]==endpoints[1];receipt['endpoints']=endpoints
        expected=((int(endpoints[0][1],16)^int(endpoints[0][2],16))*1099511628211)&((1<<64)-1)
        for final,received in zip(receipt['final'],receipt['received']):
            assert final['index']==source['records'] and final['digest']==source['tail_state']==expected
            assert received['finished'] and received['digest']==source['digest']
        for viewer in viewers:
            def return_home():
                if '[css-selection] ready=' in viewer.inspect('css-selection'):return True
                viewer.command('tap',0,'START','frames=6');return False
            wait(return_home,'viewer home CSS',60)
            layout=(viewer.data/'css-layout.bin').read_bytes();count=struct.unpack_from('<H',layout,4)[0]
            assert not identities.intersection(layout[14+48*i:46+48*i].hex() for i in range(count))
        for client in clients:
            log=(client.folder/'stdout.log').read_text(errors='replace')+client.stderr.read_text(errors='replace')
            assert not re.search(r'Checksum mismatch|\[netplay\] FAILED:|\[watch-playback\] FAILED:|Fighter source VM failed|native panic stack',log),log[-2000:]
        receipt['status']='PASS overlapping cold downloads, private chat, two verified viewers, Results and local roster return'
        print(receipt['status'],flush=True)
    except Exception as error:
        receipt.update(status='FAIL',error=repr(error))
        for client in clients:
            if client.process.poll() is None:client.screenshot()
        raise
    finally:
        if socket:socket.close()
        for client in clients:client.stop()
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))


if __name__=='__main__':main()
