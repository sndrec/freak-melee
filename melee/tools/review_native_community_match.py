"""Review news input, deferred match announcements and selected-fighter deletion.

Uses two hidden, muted clients, copied user data and a supplied local signaling
service. The community feed is a loopback fixture owned by this review.
"""
from argparse import ArgumentParser
from hashlib import sha256
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from uuid import uuid4
import json
import os
import re
import secrets
import shutil
import struct
import threading
import time

from review_native_css_room import Client
from review_native_outfit_matches import network, select_stage, wait
from review_native_room_rotation import room, session
from review_native_quick_play import choose


def checksum(data):
    value = 2166136261
    for byte in data: value = ((value ^ byte)*16777619) & 0xffffffff
    return struct.pack('<I', value)


def main():
    parser = ArgumentParser(description=__doc__)
    for key in ('exe', 'iso', 'profile', 'output'): parser.add_argument('--'+key, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    args = parser.parse_args()
    assert args.endpoint.startswith('http://127.0.0.1:'), 'Use an isolated local signaling service'
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    base = output/'base'; shutil.copytree(args.profile.resolve(), base)
    installed = list((base/'Freak Melee/content/installed').glob('*.fmi'))
    assert len(installed) == 1, 'Use a one-fighter editable review profile'
    fighter = installed[0].stem
    layout = b'FMCL'+struct.pack('<HQ',1,0)+bytes.fromhex(fighter)+struct.pack('<HHHH',450,100,100,140)
    (base/'Freak Melee/css-layout.bin').write_bytes(layout+checksum(layout))
    settings = bytearray(b'FMVS'+bytes(24))
    settings[4],settings[5],settings[6],settings[8],settings[12],settings[15] = 1,2,1,10,1,255
    (base/'Freak Melee/versus-settings.fms').write_bytes(settings+checksum(settings))
    now = int(time.time()*1000)
    headline = dict(id=str(uuid4()),title='Live community review',ticker='Open the news',published_at=now,updated_at=now,revision=1)
    notice = dict(id=str(uuid4()),delivery_id=str(uuid4()),title='After the match',body='This arrived during gameplay.',
                  news_id=None,published_at=now,expires_at=None,revision=1)
    state = dict(notices=[],match=False,during_match=0)

    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args): pass
        def do_GET(self):
            path = self.path.partition('?')[0]
            if path == '/v1/feed':
                if state['match'] and state['notices']: state['during_match'] += 1
                result = dict(headline=headline,announcements=state['notices'],announcements_next=None)
            elif path == '/v1/news/'+headline['id']: result = dict(**headline,body='The news works across the hub.')
            elif path == '/v1/announcements/'+notice['delivery_id']: result = notice
            else: self.send_error(404);return
            data = json.dumps(result).encode();etag='"'+sha256(data).hexdigest()+'"'
            same = self.headers.get('If-None-Match') == etag
            self.send_response(304 if same else 200);self.send_header('ETag',etag)
            self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(data)))
            self.end_headers()
            if not same:self.wfile.write(data)

    server = ThreadingHTTPServer(('127.0.0.1',0),Handler)
    thread = threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    os.environ.update(FREAK_MELEE_SERVICE_ENDPOINT=args.endpoint,SDL_AUDIODRIVER='dummy',FREAK_MELEE_MENU_MOTION='0',
                      FREAK_MELEE_COMMUNITY_ENDPOINT=f'http://127.0.0.1:{server.server_port}')
    clients=[];receipt=dict(status='RUNNING',checks=[],screenshots=[])
    try:
        code=''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        for index in range(2):
            client=Client(args.exe.resolve(),args.iso.resolve(),base,output/str(index),f'Community peer {index}',
                          chat_endpoint=args.endpoint,window_size=(1100,760))
            clients.append(client)
            client.click('nav-1');client.enter('lobby-code',code);client.click('join-lobby')
            wait(lambda:room(client)['count']==index+1,'private lobby join',35)
        owner,guest=clients;wait(lambda:room(owner)['count']==2,'both lobby members',20)
        # Mouse and controller A must reach the ticker on every main hub page.
        for page in range(7):
            owner.click('nav-'+str(page));owner.click('news-ticker')
            wait(lambda:'article='+headline['id'] in owner.inspect('hub-news'),'mouse ticker',15)
            owner.click('nav-'+str(page));owner.command('ui-move',*owner.element('news-ticker'));owner.tap('a',frames=20)
            wait(lambda:'article='+headline['id'] in owner.inspect('hub-news'),'controller ticker',15)
        receipt['checks'].append('mouse/controller ticker on all seven main hub pages')
        for client in clients:
            client.click('nav-1');client.click('lobby-ready');choose(client)
            client.click('play-ready');client.element('play-selection')
        peers=[room(client)['local'] for client in clients]
        wait(lambda:all(room(guest)['members'][p]['confirmed'] for p in peers),'ready practice',30)
        owner.click('play-lobby');owner.click('nav-5')
        assert 'enabled=0' in owner.inspect('delete-fighter')
        owner.click('delete-fighter')
        assert 'absent' in owner.inspect('delete-confirm')
        assert (owner.data/f'content/installed/{fighter}.fmi').exists()
        owner.click('nav-1');owner.click('lobby-ready')
        receipt['checks'].append('deletion disabled during ready practice')
        owner.click('play-start');select_stage(owner)
        sessions=[session(client,1) for client in clients];assert sessions[0][:2]==sessions[1][:2]
        wait(lambda:network(owner)['frame']>120,'real match running',50)
        notice['published_at'] = int(time.time()*1000)
        state.update(match=True,notices=[notice])
        wait(lambda:state['during_match']>=2,'both clients fetch during match',40)
        for client in clients:
            assert network(client)['frame']>300
            assert 'absent' in client.inspect('hub-input')
            seen=client.data/'content/seen-announcements.txt'
            assert not seen.exists() or notice['delivery_id'] not in seen.read_text()
        receipt['checks'].append('announcement waits through real online match')
        guest.command('hold',0,'sx=80')
        wait(lambda:'[netplay] Compared ' in (guest.folder/'stdout.log').read_text(errors='replace'),'one-stock match finishes',55)
        guest.command('hold',0,'sx=0');state['match']=False
        for client in clients:
            for _ in range(50):
                if 'count=1' in client.inspect('hub-notice'):break
                client.tap('start',frames=2)
            else:raise AssertionError('Deferred notice did not appear after results')
            assert 'title=After the match' in client.inspect('hub-notice')
            receipt['screenshots'].append(str(client.screenshot()))
            time.sleep(.4);client.tap('a',frames=30)
            wait(lambda:'count=0' in client.inspect('hub-notice'),'notice dismissed',10)
            assert notice['delivery_id'] in (client.data/'content/seen-announcements.txt').read_text()
        receipt['checks'].append('deferred notice appears after match and held A dismisses once')
        owner.click('nav-5');owner.click('delete-fighter');owner.click('delete-confirm')
        assert not (owner.data/f'content/installed/{fighter}.fmi').exists()
        assert (guest.data/f'content/installed/{fighter}.fmi').exists()
        assert (owner.data/f'content/cache/{fighter}.fmc').exists()
        assert struct.unpack_from('<H',(owner.data/'css-layout.bin').read_bytes(),4)[0]==0
        assert not list((owner.data/'projects').iterdir())
        owner.click('local-practice')
        wait(lambda:'chosen=0' in owner.inspect('css-selection'),'local choice cleared',15)
        wait(lambda:re.search(r'\[css-card\].*canonical=0 local=0 ready=0 selected=0',guest.inspect('css-cards')),
             'deleted selection cleared on peer',20)
        assert not room(guest)['members'][peers[0]]['confirmed']
        receipt['checks'].append('deletion clears CSS, project, local choice and remote lobby choice; retains peer install/cache')
        receipt['screenshots'].append(str(owner.screenshot()))
        receipt['status']='PASS'
    except Exception as error:
        receipt.update(status='FAIL',error=repr(error));raise
    finally:
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))
        for client in reversed(clients):client.stop()
        server.shutdown();server.server_close();thread.join(timeout=5)
    print('PASS native community match, notice deferral and selected-fighter deletion')


if __name__=='__main__':main()
