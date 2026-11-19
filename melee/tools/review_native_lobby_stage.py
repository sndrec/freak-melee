"""Check stage input on a shared PC while a spectator owns the lobby.

Runs three hidden, muted clients against a supplied localhost service. Uses
copies of an existing fixture profile and keeps the owner in local practice.
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
from review_native_outfit_matches import network, select_stage, settings, wait
from review_native_room_rotation import choose, click_card, room, session


def stage(client):
    text = client.inspect('room-state')
    m = re.search(r'\[room-stage\] peer=(\w*) seat=(\d+) stage=(\d+) physical=(-?\d+)', text)
    assert m, text
    return dict(peer=m[1], seat=int(m[2]), stage=int(m[3]), physical=int(m[4]))


def cursor(client):
    text = client.inspect('sss-state')
    m = re.search(r'\[sss-cursor\] x=([\d.-]+) y=([\d.-]+)', text)
    return tuple(map(float, m.groups())) if m else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+key, required=True, type=Path)
    parser.add_argument('--endpoint', required=True)
    args = parser.parse_args()
    assert args.endpoint.startswith('http://127.0.0.1:'), 'Use an isolated localhost service'
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    base = output/'base'
    shutil.copytree(args.profile, base, ignore=shutil.ignore_patterns('projects'))
    settings(base)
    clients = []
    receipt = {'status':'RUNNING', 'exe_sha256':hashlib.sha256(args.exe.read_bytes()).hexdigest()}
    try:
        code = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        for index in range(3):
            c = Client(args.exe.resolve(), args.iso.resolve(), base, output/str(index),
                       'Stage review '+str(index), without_drafts=True, window_size=(960,540))
            clients.append(c)
            if index == 1:
                c.command('hold', 1, 'sx=0')
                c.click('nav-6'); c.click('settings-category-4'); c.click('local-players-2')
                c.click('settings-back'); c.click('settings-back')
            c.click('nav-1')
            c.enter('lobby-code', code); c.click('join-lobby')
            wait(lambda: room(c)['count']==index+1, 'lobby join')
        owner, couch, guest = clients
        wait(lambda: room(owner)['count']==3, 'owner sees all members')
        peers = [room(c)['local'] for c in clients]
        click_card(owner, 0, 0)
        click_card(couch, 1, 0)
        wait(lambda: room(owner)['members'][peers[0]]['active']==0 and
             room(owner)['members'][peers[1]]['active']==2, 'fighting controllers')
        for index,c in enumerate(clients[1:], start=1):
            raw = (c.data/'css-layout.bin').read_bytes()
            fighter = raw[14:46].hex()
            port = 1 if index == 1 else 0
            choose(c,fighter,port)
            c.tap('START',frames=5,port=port)
        wait(lambda: cursor(couch) is not None and cursor(guest) is not None, 'active peers enter SSS')
        assert '[sss-state] inactive' in owner.inspect('sss-state')
        assert stage(couch)==dict(peer=peers[1],seat=1,stage=255,physical=1)
        assert stage(owner)['physical']==stage(guest)['physical']==-1
        time.sleep(2)
        before = cursor(couch)
        couch.command('tap',0,'sx=80','frames=10')
        time.sleep(.7)
        assert cursor(couch)==before, 'Inactive couch controller moved SSS cursor'
        before = cursor(guest)
        guest.command('tap',0,'sx=80','frames=10')
        time.sleep(.7)
        assert cursor(guest)==before, 'Non-choosing peer moved SSS cursor'
        select_stage(couch,1)
        sessions = [session(c,1) for c in [couch,guest]]
        assert sessions[0][:2]==sessions[1][:2]
        wait(lambda: network(couch)['frame']>180, 'match gameplay')
        assert not re.search(r'\[netplay\] Session ', (owner.folder/'stdout.log').read_text(errors='replace'))
        assert room(owner)['state']==2
        receipt.update(first_chooser=stage(couch),sessions=sessions,
                       match_screenshots=[str(c.screenshot()) for c in clients])
        # The second canonical port loses. Its peer must get the next pick.
        guest.command('hold',0,'sx=80')
        wait(lambda: '[netplay] Compared ' in (guest.folder/'stdout.log').read_text(errors='replace'),
             'stock match finishes',90)
        guest.command('hold',0,'sx=0')
        wait(lambda: stage(owner)['peer']==peers[2], 'loser becomes next stage chooser')
        assert stage(owner)['seat']==0
        receipt['next_chooser']=stage(owner)
        receipt['results_screenshots']=[str(c.screenshot()) for c in clients]
        assert '[sss-state] inactive' in owner.inspect('sss-state')
        # Returning to ready practice must still wait for another host request.
        for c in [couch,guest]:
            deadline=time.monotonic()+45
            while time.monotonic()<deadline:
                if '[css-selection] ready=' in c.inspect('css-selection'): break
                c.tap('start',frames=6,port=1 if c is couch else 0); time.sleep(.3)
            else: raise RuntimeError('Results did not return to CSS')
            if 'origin=' in c.inspect('lobby-ready'): c.click('lobby-ready')
            c.tap('start',frames=6,port=1 if c is couch else 0)
            c.element('play-selection')
        time.sleep(1)
        assert all('[sss-state] inactive' in c.inspect('sss-state') for c in clients)
        assert room(owner)['match']==0, 'Previous host request started an automatic rematch'
        wait(lambda: 'enabled=1' in owner.inspect('play-start'), 'host can start rematch')
        owner.click('play-start')
        wait(lambda: cursor(guest) is not None, 'loser enters next stage scene')
        assert stage(guest)==dict(peer=peers[2],seat=0,stage=255,physical=0)
        assert stage(couch)['physical']==-1
        select_stage(guest,0)
        next_sessions=[session(c,2) for c in [couch,guest]]
        assert next_sessions[0][:2]==next_sessions[1][:2]
        assert next_sessions[0][0]!=sessions[0][0]
        receipt['rematch_sessions']=next_sessions
        for c in clients:
            text=(c.folder/'stdout.log').read_text(errors='replace')+c.stderr.read_text(errors='replace')
            assert not re.search(r'Checksum mismatch|\[netplay\] FAILED:|native panic stack|Fighter source VM failed',text), text[-1800:]
        receipt['status']='PASS spectator-owned match, shared-PC stage input, previous-loser handoff'
        print(receipt['status'],flush=True)
    except Exception as error:
        receipt.update(status='FAIL',error=repr(error))
        raise
    finally:
        for c in clients: c.stop()
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))


if __name__=='__main__': main()
