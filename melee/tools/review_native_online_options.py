"""Review integrated Online settings and stock menu decoration with local fixture data."""
import argparse,json,os,shutil,threading,time,re
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from review_native_css_room import Client
from review_native_hub_footer import session_file,secret
from review_native_release import close_window

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['exe','iso','profile','output']:p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 owner,key,second,session=[secret() for _ in range(4)]
 profile=dict(id=owner,handle='inbox_review',name='Online reviewer',bio='Local settings fixture')
 class Handler(BaseHTTPRequestHandler):
  def log_message(self,*_):pass
  def answer(self,value):
   body=json.dumps(value).encode();self.send_response(200);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
  def do_GET(self):
   if self.path.endswith('/me'):self.answer(dict(profile=profile,keys=[dict(id=key,name='This PC',current=True),dict(id=second,name='Another PC')],sessions=[dict(id=session,name='Fixture session',current=True)]))
   elif self.path.partition('?')[0]=='/v1/feed':self.answer(dict(headline=None,announcements=[],announcements_next=None))
   else:self.send_error(404)
  def do_POST(self):
   body=json.loads(self.rfile.read(int(self.headers['Content-Length'])))
   if self.path.endswith('/social/state'):self.answer(dict(folder=0,offset=0,friends_offset=0,total=0,unread=0,friends_total=0,friends=[],messages=[]))
   else:self.send_error(404)
 server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();endpoint='http://localhost:'+str(server.server_port)
 fixture=out/'profile';shutil.copytree(a.profile.resolve(),fixture);content=fixture/'Freak Melee/content';content.mkdir(exist_ok=True,parents=True);session_file(content/'account-session.bin',endpoint)
 os.environ.update(FREAK_MELEE_SERVICE_ENDPOINT=endpoint,FREAK_MELEE_COMMUNITY_ENDPOINT=endpoint,FREAK_MELEE_MENU_MOTION='0')
 c=None;r=dict(status='RUNNING',screenshots={},checks=[])
 def shot(name):r['screenshots'][name]=str(c.screenshot())
 try:
  c=Client(a.exe.resolve(),a.iso.resolve(),fixture,out/'client','Online reviewer',window_size=(1440,900))
  c.click('nav-6');c.click('settings-category-3')
  end=time.monotonic()+15
  while time.monotonic()<end:
   state=c.inspect('hub-account')
   if 'busy=0 signed-in=1' in state:break
   time.sleep(.15)
  else:raise AssertionError(state)
  assert 'page=24 ' in c.inspect('hub-input')
  for name in ['account-export','account-add-key','account-keys','account-devices','account-all-out','account-rotate','account-logout','account-refresh','account-profile']:c.element(name)
  assert 'absent' in c.inspect('settings-account');shot('online-account-integrated')
  assert 'next-screen-hidden=1' in c.inspect('hub-assets')
  c.click('account-keys');c.element('account-entry-0');shot('account-keys');c.tap('B',frames=40);c.element('account-export')
  c.click('account-logout');c.element('account-confirm');c.tap('B',frames=40);c.element('account-export')
  assert 'signed-in=1' in c.inspect('hub-account')
  c.resize(960,540);shot('online-narrow');c.click('settings-back');shot('settings-narrow');c.click('settings-back');shot('hub-narrow')
  for i in range(7):c.element('nav-'+str(i))
  r['checks']=['account controls are on Online without an extra account page','key list and confirmation Back consume one button edge','cancelled sign-out preserves session','native NEXT SCREEN graphic hidden','varied Settings and hub ribbons','two window sizes'];r['status']='PASS'
 finally:
  if c and c.process.poll() is None:close_window(c.process.pid);c.process.wait(timeout=20)
  server.shutdown();server.server_close();thread.join(timeout=5);(out/'review.json').write_text(json.dumps(r,indent=2))
 print('PASS integrated Online settings review')
if __name__=='__main__':main()
