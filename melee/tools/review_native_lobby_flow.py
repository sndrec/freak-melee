"""Review shared stock Rules, automatic readiness and stage policies in real local clients."""
import argparse,json,os,re,secrets,shutil,struct,subprocess,time
from pathlib import Path
from review_native_css_room import Client
from review_native_release import close_window
from review_native_outfit_matches import network,select_stage,wait
from review_native_room_rotation import choose,click_card,room,session

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for key in ['exe','iso','profile','output']:p.add_argument('--'+key,type=Path,required=True)
 p.add_argument('--endpoint',required=True);p.add_argument('--policies',default='4,0,1,2,3')
 p.add_argument('--spectator-host',action='store_true');p.add_argument('--shared-couch',action='store_true')
 p.add_argument('--chat-probe',type=Path,help='Send a message during the match in an isolated local service');a=p.parse_args()
 assert a.endpoint.startswith(('http://127.0.0.1:','http://localhost:')) or a.endpoint=='https://freak-melee-signaling.sndrec32exe.workers.dev'
 if a.chat_probe and not a.endpoint.startswith(('http://127.0.0.1:','http://localhost:')):
  p.error('--chat-probe requires a local service')
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 os.environ.update(FREAK_MELEE_SERVICE_ENDPOINT=a.endpoint,FREAK_MELEE_COMMUNITY_ENDPOINT='http://localhost:1',FREAK_MELEE_MENU_MOTION='0')
 result={'status':'RUNNING','cases':[]};clients=[]
 def inspect(c,kind,pattern,timeout=30):
  return wait(lambda:(text if re.search(pattern,text:=c.inspect(kind)) else None),pattern,timeout)
 def flow(c):
  text=c.inspect('room-state');m=re.search(r'\[room-flow\] revision=(\d+) phase=(\d+) remaining=(\d+) rules=(\d+) mode=(\d+) stock=(\d+) timer=(\d+) stage_policy=(\d+) damage=(\d+) penalty=(\d+) teams=(\d+)',text)
  assert m,text;return dict(zip(['revision','phase','remaining','rules','mode','stock','timer','stage_policy','damage','penalty','teams'],map(int,m.groups())))
 def close():
  for c in clients:
   if c.process.poll() is None:close_window(c.process.pid);c.process.wait(timeout=20)
  clients.clear()
 def rules_menu(c):c.click('play-rules')
 def finish_rules(c):c.click('rules-close')
 def enter_practice(c,port):
  c.tap('START',frames=4,port=port);inspect(c,'practice-state',r'practice=1 ready=1 paused=0',90)
 try:
  for policy in map(int,a.policies.split(',')):
   case={'policy':policy,'screenshots':{}};result['cases'].append(case)
   folder=out/str(policy);folder.mkdir();base=folder/'base';shutil.copytree(a.profile,base,ignore=shutil.ignore_patterns('projects'))
   path=base/'Freak Melee/versus-settings.fms';data=bytearray(32);data[:4]=b'FMVS'
   data[4:16]=bytes([1,2,1,0,10,4,10,0,1,0,1,255]);struct.pack_into('<I',data,24,0xffffffff)
   if policy==1:struct.pack_into('<I',data,24,1<<25) # FD only; exercise the shared Random Stage Switch mask.
   digest=2166136261
   for value in data[:28]:digest=((digest^value)*16777619)&0xffffffff
   struct.pack_into('<I',data,28,digest);path.write_bytes(data)
   code=''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
   count=3 if a.spectator_host else 2
   for index in range(count):
    c=Client(a.exe.resolve(),a.iso.resolve(),base,folder/str(index),'Flow player '+str(index),window_size=(1440,900),chat_endpoint=a.endpoint if a.chat_probe else None);clients.append(c)
    if a.shared_couch and index==1:
     c.click('nav-6');c.click('settings-category-4');c.click('local-players-2');c.click('settings-back');c.click('settings-back')
     c.command('hold',1,'sx=0')
    c.click('nav-1');c.enter('lobby-code',code);c.click('join-lobby')
    inspect(c,'room-state',r'owner=\w+ revision=')
    wait(lambda:room(c)['count']==index+1,'joined lobby',25)
    inspect(c,'room-state',r'\[room-flow\].*rules=1 ')
   host=clients[0];wait(lambda:room(host)['count']==count,'complete presence',20)
   peers=[room(c)['local'] for c in clients]
   rules_menu(host)
   for _ in range((policy-4)%5):host.click('rules-row-4-right')
   host.click('rules-row-3-right') # 110% damage; guest must display this shared value.
   host.click('rules-next-page');host.click('rules-row-4-right') # SD penalty 2.
   host.click('rules-next-page')
   assert 'origin=' in host.inspect('lobby-capacity-left')
   assert 'origin=' not in host.inspect('rules-lobby')
   if not a.shared_couch and not a.spectator_host:
    for direction,limit in [('left',3),('left',2),('right',3),('left',2)]:
     host.click('lobby-capacity-'+direction)
     inspect(host,'match-rules',rf'capacity={limit} ')
    case['screenshots']['lobby-rules']=str(host.screenshot())
   host.click('rules-next-page');finish_rules(host)
   if a.spectator_host:
    click_card(host,0,0)
    wait(lambda:room(host)['members'][peers[0]]['active']==0,'host released exact card')
   if a.shared_couch:
    click_card(clients[1],1,0)
    wait(lambda:room(host)['members'][peers[1]]['active']==2,'couch controller released its card')
   active=clients[1:] if a.spectator_host else clients
   for c in active:
    wait(lambda:flow(c)['stage_policy']==policy and flow(c)['damage']==11 and flow(c)['penalty']==2,'shared rules',20)
   guest=clients[-1];guest_settings=(guest.data/'versus-settings.fms').read_bytes()
   if not a.shared_couch and not a.spectator_host:
    for c in active:
     inspect(c,'css-cards',r'count=2')
     inspect(c,'css-cards',r'\[css-rules-label\] 2-man survival test\s+-\s+1 stock\s+10 min')
    case['screenshots']['lobby-css']=str(host.screenshot())
    click_card(guest,1,0)
    wait(lambda:room(host)['members'][peers[-1]]['active']==0,'guest released exact card')
    inspect(host,'css-cards',rf'\[css-hand\] peer={peers[-1]} seat=0 numbered=0 held=1 ')
    case['screenshots']['spectating-hand']=str(host.screenshot())
    click_card(guest,1,0,mouse=True)
    wait(lambda:room(host)['members'][peers[-1]]['active']==1,'guest claimed exact card')
    inspect(host,'css-cards',rf'\[css-hand\] peer={peers[-1]} seat=0 numbered=1 held=1 ')
    assert 'origin=' not in guest.inspect('css-fight-seat-0')
   rules_menu(guest);inspect(guest,'match-rules',rf'editable=0 .*damage=110 .*stage={policy} ')
   case['screenshots']['shared-rules']=str(guest.screenshot());finish_rules(guest)
   for c in active:
    port=1 if a.shared_couch and c is clients[1] else 0
    if a.shared_couch and c is clients[1]:c.command('hold',0,'sx=0');c.command('hold',1,'sx=0')
    raw=(c.data/'css-layout.bin').read_bytes();choose(c,raw[14:46].hex(),port)
    assert 'origin=' not in c.inspect('play-start')
   if not a.shared_couch and not a.spectator_host:
    inspect(host,'css-cards',rf'\[css-hand\] peer={peers[-1]} seat=0 numbered=1 held=0 ')
   first=active[0];first_port=1 if a.shared_couch and first is clients[1] else 0
   enter_practice(first,first_port)
   assert flow(first)['phase']==0
   # Pause the first fighter before the last fighter readies: no countdown.
   inspect(first,'practice-state',r'hud=1');first.tap('START',frames=4,port=first_port)
   inspect(first,'practice-state',r'ready=0 paused=2')
   second=active[1];second_port=1 if a.shared_couch and second is clients[1] else 0
   enter_practice(second,second_port);assert flow(first)['phase']==0
   first.tap('START',frames=4,port=first_port)
   wait(lambda:flow(first)['phase']==1,'authoritative countdown',20)
   case['screenshots']['countdown']=str(first.screenshot())
   # One press cancels the countdown, with B still available for gameplay.
   first.tap('START',frames=4,port=first_port);inspect(first,'practice-state',r'ready=0 paused=2')
   wait(lambda:flow(second)['phase']==0,'countdown cancellation',20)
   time.sleep(.3);first.tap('START',frames=4,port=first_port)
   wait(lambda:flow(first)['phase']==1,'restarted countdown',20)
   if policy not in [1,2]:
    for c in active:inspect(c,'sss-state',r'\[sss-cursor\]',90)
    chooser=second if policy==0 else first;port=second_port if policy==0 else first_port
    if policy!=0:assert 'physical=-1' in second.inspect('room-state')
    if a.shared_couch and chooser is clients[1]:
     before=re.search(r'\[sss-cursor\].*',chooser.inspect('sss-state'))[0];chooser.tap('A',frames=4,port=0)
     assert re.search(r'\[sss-cursor\].*',chooser.inspect('sss-state'))[0]==before
    select_stage(chooser,port,confirm=False)
    target=chooser.inspect('sss-state')
    expected_cursor=list(map(float,re.search(r'\[sss-cursor\] x=([\d.-]+) y=([\d.-]+)',target).groups()))
    expected_tile=re.search(r'\[sss-state\] selected=(\d+)',target)[1]
    for observer in active:
     if observer is chooser:continue
     def synced():
      text=observer.inspect('sss-state');cursor=re.search(r'\[sss-cursor\] x=([\d.-]+) y=([\d.-]+)',text)
      return cursor and all(abs(float(v)-e)<.01 for v,e in zip(cursor.groups(),expected_cursor)) and 'selected='+expected_tile in text
     wait(synced,'replicated original stage cursor and hover',20)
     assert 'peer='+room(chooser)['local'] in inspect(observer,'room-state',r'\[room-cursor\]')
    case['screenshots']['stage-observer']=str((second if chooser is first else first).screenshot())
    chooser.tap('a',port=port)
   sessions=[session(c,1) for c in active];assert sessions[0][:2]==sessions[1][:2]
   wait(lambda:network(first).get('frame',0)>120,'live agreed match',90)
   case['sessions']=sessions;case['screenshots']['match']=str(first.screenshot())
   if a.chat_probe:
    def chat_count(c):
     text=c.inspect('global-chat-state')
     return int(re.search(r'\[global-chat\] state=\d+ messages=(\d+)',text)[1])
    for c in active:inspect(c,'global-chat-state',r'\[global-chat\] state=1 ')
    counts=[chat_count(c) for c in active]
    marker='Match feedback review '+secrets.token_hex(6)
    probe=subprocess.run([str(a.chat_probe.resolve()),a.endpoint,'Match chat probe',marker,marker,'4'],
                         capture_output=True,text=True,timeout=15,creationflags=subprocess.CREATE_NO_WINDOW)
    assert probe.returncode==0,probe.stdout+probe.stderr
    for c,before in zip(active,counts):
     wait(lambda:chat_count(c)==before+1,'global message received during active match',12)
     assert network(c).get('frame',0)>120
   if policy in [1,2]:
    for c in active:
     log=(c.folder/'stdout.log').read_text(errors='replace')+c.stderr.read_text(errors='replace')
     assert '[match-setup] Automatic stage;' in log and '[stage-select] Entered' not in log,log[-3000:]
   if a.spectator_host:assert '[sss-state] inactive' in host.inspect('sss-state') and '[netplay] Session ' not in (host.folder/'stdout.log').read_text(errors='replace')
   # End a real one-stock game and verify the next loser/Turns picker.
   second.command('hold',second_port,'sx=80')
   wait(lambda:'[netplay] Compared ' in (second.folder/'stdout.log').read_text(errors='replace'),'completed match',90)
   second.command('hold',second_port,'sx=0')
   for c in active:
    port=first_port if c is first else second_port
    deadline=time.monotonic()+45
    while time.monotonic()<deadline:
     if '[css-selection] ready=' in c.inspect('css-selection'):break
     c.tap('START',frames=4,port=port);time.sleep(.3)
    else:raise AssertionError('Results did not return to CSS')
   assert (guest.data/'versus-settings.fms').read_bytes()==guest_settings
   for c in active:
    assert 'origin=' not in c.inspect('return-session')
    enter_practice(c,first_port if c is first else second_port)
   if policy not in [1,2]:
    inspect(second,'sss-state',r'\[sss-cursor\]',90)
    expected=second_port if policy in [3,4] else first_port
    if policy in [3,4]:assert f'physical={expected}' in second.inspect('room-state')
    case['screenshots']['next-stage']=str(second.screenshot())
   else:
    next_sessions=[session(c,2) for c in active];assert next_sessions[0][:2]==next_sessions[1][:2]
    text=first.inspect('room-state');expected=32 if policy==1 else 11
    assert re.search(rf'\[room-stage\].* stage={expected} ',text),text
    case['rematch_sessions']=next_sessions
   for c in clients:
    text=(c.folder/'stdout.log').read_text(errors='replace')+c.stderr.read_text(errors='replace')
    assert not re.search(r'\[netplay\] FAILED:|native panic stack|Fighter source VM failed',text),text[-2000:]
   case['status']='PASS';close();(out/'review.json').write_text(json.dumps(result,indent=2))
  result['status']='PASS'
 finally:
  close();(out/'review.json').write_text(json.dumps(result,indent=2))
 print('PASS shared lobby rules, readiness and stage flow')
if __name__=='__main__':main()
