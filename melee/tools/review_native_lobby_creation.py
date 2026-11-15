"""Review leaving Quick Play for a new lobby and the enlarged CSS cards locally."""
import argparse,json,os,time,re
from pathlib import Path
from review_native_css_room import Client
from review_native_release import close_window

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['exe','iso','profile','output']:p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--endpoint',required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 os.environ.update(FREAK_MELEE_SERVICE_ENDPOINT=a.endpoint,FREAK_MELEE_COMMUNITY_ENDPOINT='http://localhost:1',FREAK_MELEE_MENU_MOTION='0')
 clients=[];r=dict(status='RUNNING',screenshots={},checks=[])
 def new(name):
  c=Client(a.exe.resolve(),a.iso.resolve(),a.profile.resolve(),out/name,name,window_size=(1440,900));clients.append(c);return c
 def wait(c,id,pattern):
  end=time.monotonic()+20
  while time.monotonic()<end:
   text=c.inspect(id)
   if re.search(pattern,text):return text
   time.sleep(.15)
  raise AssertionError((pattern,text))
 try:
  host=new('Host');host.click('nav-0');host.click('quick-singles');wait(host,'quick-play',r'mode=1 state=0')
  host.tap('B',frames=35);host.click('nav-1');host.click('create-lobby');host.element('lobby-title')
  wait(host,'quick-play','unavailable');host.enter('lobby-title','Normal lobby after Quick Play');host.click('confirm-create-lobby');host.wait_cards(1)
  assert 'unavailable' in host.inspect('quick-play')
  guest=new('Guest');guest.click('nav-1');host_room=''
  end=time.monotonic()+20
  while time.monotonic()<end:
   text=guest.inspect('hub-lobbies')
   row=re.search(r'row=(\d+) code=([A-Z0-9]+) title=Normal lobby after Quick Play',text)
   if row:host_room=row[2];guest.click('join-listed-'+row[1]);break
   if 'enabled=1' in guest.inspect('refresh-lobbies'):guest.click('refresh-lobbies')
   time.sleep(.3)
  assert host_room,text
  host.wait_cards(2);guest.wait_cards(2);assert 'unavailable' in guest.inspect('quick-play')
  r['room']=host_room;r['screenshots']['cards']=str(host.screenshot());host.resize(960,540);r['screenshots']['cards-narrow']=str(host.screenshot())
  host.click('play-rules'); host.click('rules-row-6'); host.click('rules-lobby')
  r['screenshots']['lobby-rules-narrow']=str(host.screenshot()); host.resize(1440,900)
  r['screenshots']['lobby-rules']=str(host.screenshot())
  host.command('ui-move',*host.element('change-listing')); host.tap('A',frames=40)
  wait(host,'match-rules',r'listed=0 ')
  host.click('rotation-policy'); wait(host,'match-rules',r'rotation=1 '); host.click('lobby-seat-1-0'); wait(host,'match-rules',r'lobby-person row=1 active=0 waiting=1 '); host.click('rules-close'); host.click('rules-close'); host.click('rules-close')
  guest.click('play-rules'); guest.click('rules-row-6'); guest.click('rules-lobby'); guest.click('lobby-seat-1-0'); wait(guest,'match-rules',r'lobby-person row=1 active=0 waiting=0 ')
  r['screenshots']['guest-rules']=str(guest.screenshot())

  r['checks']=['Create lobby leaves Quick Play immediately','confirmation creates a normal listed room','second client can join that room','four authored cards at two sizes','stock-styled lobby rows at two sizes','held A changes listing once','rotation policy changes','host overrides another seat','guest can change their own choice again'];r['status']='PASS'
 finally:
  for c in clients:
   if c.process.poll() is None:close_window(c.process.pid);c.process.wait(timeout=20)
  (out/'review.json').write_text(json.dumps(r,indent=2))
 print('PASS normal lobby after Quick Play and CSS layout')
if __name__=='__main__':main()
