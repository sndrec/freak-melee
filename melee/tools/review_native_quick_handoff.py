"""Exercise 1v1 token handoff, controller queue entry, practice readiness and incoming chat."""
import argparse,json,os,re,time
from pathlib import Path
from review_native_css_room import Client
from review_native_release import close_window

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['exe','iso','profile','output']: p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--endpoint',required=True); a=p.parse_args(); out=a.output.resolve(); out.mkdir(parents=True,exist_ok=False)
 os.environ.update(FREAK_MELEE_SERVICE_ENDPOINT=a.endpoint,FREAK_MELEE_COMMUNITY_ENDPOINT='http://localhost:1',FREAK_MELEE_MENU_MOTION='0')
 clients=[]; result=dict(status='RUNNING',screenshots={},checks=[])
 def new(name):
  c=Client(a.exe.resolve(),a.iso.resolve(),a.profile.resolve(),out/name,name,window_size=(1440,900)); clients.append(c); return c
 def wait(c,id,pattern,timeout=40):
  end=time.monotonic()+timeout
  while time.monotonic()<end:
   text=c.inspect(id)
   if re.search(pattern,text): return text
   time.sleep(.15)
  raise AssertionError((id,pattern,text))
 def select(c,port):
  state,text=c.state(port); tile=re.search(r'index=0 center=([\d.-]+),([\d.-]+)',text); assert tile,text
  x,y=map(float,tile.groups())
  if state['hand']!=1: c.tap('B',frames=3,port=port)
  c.move(x-2.7,y+2,port); c.tap('A',frames=40,port=port)
  state,text=c.state(port); assert state['chosen']==1 and state['holding']==0,text
 def shot(c,name): result['screenshots'][name]=str(c.screenshot())
 try:
  host=new('Handoff host'); host.click('nav-6'); host.click('settings-category-4'); host.click('local-players-4')
  host.click('controls-back'); host.click('settings-back'); host.click('nav-0'); host.click('quick-singles')
  wait(host,'quick-play',r'mode=1 state=0')
  for port in range(4): host.command('hold',port,'sx=0')
  for port in range(4): assert host.state(port)[0]['kind']==0
  select(host,1); wait(host,'css-cards',r'display=0 [^\r\n]*physical=1 [^\r\n]*local=1 [^\r\n]*selected=1')
  assert 'enabled=0' in host.inspect('play-ready'); host.tap('START',frames=5,port=1)
  assert 'practice=0' in host.inspect('practice-state')
  select(host,2); previous,_=host.state(1); assert previous['chosen']==0 and previous['holding']==0
  wait(host,'css-cards',r'display=0 [^\r\n]*physical=2 [^\r\n]*local=1 [^\r\n]*selected=1'); shot(host,'handoff')
  _,text=host.state(2); left,right,top,bottom=map(float,re.search(r'viewport=([\d.-]+),([\d.-]+),([\d.-]+),([\d.-]+)',text).groups())
  host.move(left+(451+177/2)/640*(right-left),top-(451+12)/480*(top-bottom),2); host.tap('A',frames=40,port=2)
  wait(host,'quick-play',r'mode=1 state=1'); host.tap('START',frames=5,port=0); assert 'practice=0' in host.inspect('practice-state')
  wait(host,'css-selection',r'ready=1 '); host.tap('START',frames=5,port=2); wait(host,'practice-state',r'practice=1 ready=1 paused=0 quick-port=2 buttons=0 hud=1')
  host.tap('B',frames=40,port=2); wait(host,'practice-state',r'practice=1 ready=1 paused=0')
  host.tap('START',frames=40,port=2); wait(host,'practice-state',r'practice=1 ready=0 paused=2')
  host.tap('START',frames=40,port=2); wait(host,'practice-state',r'practice=1 ready=1 paused=0')
  shot(host,'practice-status')
  guest=new('Chat guest'); guest.click('nav-0'); guest.click('quick-singles'); select(guest,0); guest.click('play-queue')
  wait(host,'quick-play',r'mode=1 state=2'); wait(guest,'quick-play',r'mode=1 state=2')
  wait(guest,'css-chat',r'visible=1 ')
  guest.enter('css-chat-input','First incoming practice note'); guest.command('ui-key','enter')
  wait(host,'practice-state',r'practice-chat messages=1 active=1 ')
  guest.enter('css-chat-input','Second note appears underneath'); guest.command('ui-key','enter')
  wait(host,'practice-state',r'practice-chat messages=2 active=2 '); shot(host,'floating-chat')
  host.tap('START',frames=40,port=2); wait(host,'practice-state',r'practice=1 ready=0 paused=2')
  wait(guest,'css-cards',r'display=1 [^\r\n]*local=0 ready=0 ')
  host.tap('START',frames=40,port=2); wait(host,'practice-state',r'practice=1 ready=1 paused=0')
  wait(host,'practice-state',r'practice-chat messages=2 active=0 ',timeout=14); shot(host,'faded-chat')
  host.tap('START',frames=4,port=2); wait(host,'practice-state',r'practice=1 ready=0 paused=2')
  host.command('tap',2,'L','R','A','START','frames=4'); time.sleep(.5)
  wait(host,'css-selection',r'\[css-slot\] slot=3 kind=0 ')
  assert 'practice=0' in host.inspect('practice-state'); shot(host,'returned-css')
  result['checks']=['Four configured hands in 1v1 with one network seat','Newest placed token takes the seat and ejects previous choice','START before queue is rejected','Queue entered with controller A and does not toggle repeatedly while held','Inactive controller cannot ready the selected seat','Chosen physical controller starts practice','Status-only practice toolbar','B remains gameplay input','START unreadies/pauses and resumes/readies once per press','Incoming CSS chat reaches practice, newest below older, and fades away','Paused L+R+A+START returns to CSS and restores configured hands']; result['status']='PASS'
 finally:
  for c in clients:
   if c.process.poll() is None: close_window(c.process.pid); c.process.wait(timeout=20)
  (out/'review.json').write_text(json.dumps(result,indent=2))
 print('PASS Quick Play handoff and practice suite')
if __name__=='__main__': main()
