"""Review populated Inbox footer and ticker layout with private local fixture data.

The fixture creates an isolated DPAPI-protected test session. It exercises native
UI rendering and paging, not the account service's authentication checks.
"""
import argparse
import base64
import ctypes
from ctypes import wintypes
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import threading
import time
from uuid import uuid4
from review_native_css_room import Client
from review_native_release import close_window


def secret():
    return base64.urlsafe_b64encode(secrets.token_bytes(32)).decode().rstrip('=')


def session_file(path, origin):
    class Blob(ctypes.Structure):
        _fields_ = [('size',wintypes.DWORD),('data',ctypes.POINTER(ctypes.c_ubyte))]
    plain = b''.join(text.encode().ljust(size,b'\0') for text,size in
                     [(origin,512),('inbox_review',25),(secret(),44),('',44),(secret(),44)]) + b'\0'
    assert len(plain) == 670
    buffer = (ctypes.c_ubyte*len(plain)).from_buffer_copy(plain)
    source, result = Blob(len(plain),buffer), Blob()
    crypt = ctypes.WinDLL('crypt32',use_last_error=True)
    crypt.CryptProtectData.argtypes = [ctypes.POINTER(Blob),wintypes.LPCWSTR,ctypes.c_void_p,
                                      ctypes.c_void_p,ctypes.c_void_p,wintypes.DWORD,ctypes.POINTER(Blob)]
    if not crypt.CryptProtectData(ctypes.byref(source),'Review fixture',None,None,None,1,ctypes.byref(result)):
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        path.write_bytes(ctypes.string_at(result.data,result.size))
    finally:
        ctypes.windll.kernel32.LocalFree(result.data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe','iso','profile','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args = parser.parse_args()
    root = args.output.resolve()
    root.mkdir(parents=True,exist_ok=False)
    owner, peer, key, session = (secret() for _ in range(4))
    now = int(time.time()*1000)
    headline = dict(id=str(uuid4()),title='A long Workshop update for the whole community',
                    ticker='Workshop updates, shared fighters and new ways to play. This long headline scrolls inside the menu frame.',
                    published_at=now,updated_at=now,revision=1)
    letters = [dict(id=secret(),peer=peer,handle='fixture_friend',name='A friend',
                    title=f'Letter {i+1}: a few things to try in the Workshop',kind='letter',status='Received',
                    folder=0,unread=1,created=now-i,expires=0,room='') for i in range(9)]
    profile = dict(id=owner,handle='inbox_review',name='Inbox reviewer',bio='Local UI review fixture')

    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*_): pass
        def answer(self,value):
            body = json.dumps(value).encode()
            self.send_response(200)
            self.send_header('Content-Type','application/json')
            self.send_header('Content-Length',str(len(body)))
            self.end_headers();self.wfile.write(body)
        def do_GET(self):
            if self.path.endswith('/me'):
                self.answer(dict(profile=profile,keys=[dict(id=key,name='Fixture key')],
                                 sessions=[dict(id=session,name='Fixture session',current=True)]))
            elif self.path == '/v1/feed':
                self.answer(dict(headline=headline,announcements=[],announcements_next=None))
            else:self.send_error(404)
        def do_POST(self):
            body = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
            if self.path.endswith('/social/state'):
                folder, offset = body.get('folder',0),body.get('offset',0)
                self.answer(dict(folder=folder,offset=offset,friends_offset=0,total=9 if not folder else 0,
                                 unread=9,friends_total=0,friends=[],messages=letters[offset:offset+8] if not folder else []))
            else:self.send_error(404)

    server = ThreadingHTTPServer(('127.0.0.1',0),Handler)
    thread = threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    endpoint = f'http://localhost:{server.server_port}'
    fixture = root/'profile'
    shutil.copytree(args.profile.resolve(),fixture)
    content = fixture/'Freak Melee/content';content.mkdir(exist_ok=True,parents=True)
    session_file(content/'account-session.bin',endpoint)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = endpoint
    os.environ['FREAK_MELEE_COMMUNITY_ENDPOINT'] = endpoint
    os.environ['FREAK_MELEE_MENU_MOTION'] = '0'
    client = None
    result = dict(status='RUNNING',screenshots={},checks=[])
    try:
        client = Client(args.exe.resolve(),args.iso.resolve(),fixture,root/'client','Inbox reviewer',window_size=(1440,900))
        client.click('nav-4')
        deadline = time.monotonic()+20
        while time.monotonic()<deadline:
            state = client.inspect('hub-social')
            if re.search(r'signed-in=1 busy=0.*count=8 total=9',state):break
            time.sleep(.2)
        else:raise AssertionError(state)
        result['screenshots']['inbox'] = str(client.screenshot())
        client.click('social-next')
        deadline = time.monotonic()+10
        while time.monotonic()<deadline:
            state = client.inspect('hub-social')
            if 'offset=8 count=1 total=9' in state:break
            time.sleep(.2)
        else:raise AssertionError(state)
        result['screenshots']['inbox-page-2'] = str(client.screenshot())
        client.click('social-prev')
        client.resize(960,540)
        result['screenshots']['inbox-narrow'] = str(client.screenshot())
        for i in range(3):
            time.sleep(2)
            result['screenshots'][f'ticker-{i}'] = str(client.screenshot())
        result['checks'] = ['eight populated rows and unread count','next/previous mailbox page',
                            'footer at two window sizes','long ticker sampled at multiple offsets']
        result['status'] = 'PASS'
    finally:
        if client and client.process.poll() is None:
            close_window(client.process.pid);client.process.wait(timeout=20)
        server.shutdown();server.server_close();thread.join(timeout=5)
        (root/'review.json').write_text(json.dumps(result,indent=2))
    print('PASS native Inbox/footer/ticker review')


if __name__ == '__main__':main()

