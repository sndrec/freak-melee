"""Review stock animated Rules, immediate edits, submenu navigation and CSS return."""
import argparse,json,os,re,time
from pathlib import Path
from review_native_css_room import Client
from review_native_release import close_window

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ['exe','iso','profile','output']: p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args(); out=a.output.resolve(); out.mkdir(parents=True,exist_ok=False)
 os.environ.update(FREAK_MELEE_SERVICE_ENDPOINT='http://localhost:1',FREAK_MELEE_CHAT_ENDPOINT='http://localhost:1',FREAK_MELEE_COMMUNITY_ENDPOINT='http://localhost:1',FREAK_MELEE_MENU_MOTION='0')
 c=None; r=dict(status='RUNNING',screenshots={},checks=[])
 def state(pattern):
  text=c.inspect('match-rules'); assert re.search(pattern,text),(pattern,text); return text
 def shot(name): r['screenshots'][name]=str(c.screenshot())
 try:
  c=Client(a.exe.resolve(),a.iso.resolve(),a.profile.resolve(),out/'client','Stock Rules review',window_size=(1440,900))
  c.click('nav-6'); c.click('settings-category-2'); c.click('settings-rules')
  state('mode=0 time=2 stock=3'); shot('stock-rules')
  c.command('ui-move',*c.element('rules-row-0-right')); c.tap('A',frames=40)
  state('mode=1 time=2 stock=3')
  c.click('rules-row-1-right'); state('stock=4')
  c.click('rules-row-2-right'); state('handicap=1')
  c.click('rules-row-3-right'); state('damage=110')
  c.click('rules-row-4-left'); state('stage=3')
  shot('stock-selected'); c.click('rules-next-page'); c.element('rules-previous-page')
  c.click('rules-row-0-right'); state('timer=1')
  c.command('ui-move',*c.element('rules-row-1-right')); c.tap('A',frames=40); state('friendly=1')
  shot('additional'); c.click('rules-previous-page'); state('page=27 ')
  c.click('rules-row-5'); c.click('rules-item-rate-0'); state('freq=255 ')
  mask=int(re.search(r'mask=([0-9a-f]+)',c.inspect('match-rules'))[1],16); c.click('rules-item-0')
  state(r'mask=0*'+format(mask^1,'x')+r' '); c.click('rules-item-rate-2'); state('freq=1 '); shot('items')
  c.tap('B',frames=40); state('page=27 ')
  c.resize(960,540); shot('stock-rules-narrow'); c.click('rules-close'); c.element('settings-rules')
  c.click('settings-rules'); state('mode=1 time=2 stock=4 timer=1 damage=110 handicap=1 stage=3 friendly=1')
  c.click('rules-close'); c.click('settings-back'); c.click('settings-back'); c.tap('B',frames=40)
  c.element('play-rules'); c.click('play-rules'); state('css=1 '); shot('from-css')
  c.click('rules-close'); c.element('play-rules')
  r['checks']=['Original animated rows render in the native Rules frame','Held A changes a value once','Stock count, damage ratio, stage policy and additional rules edit immediately','Items edit immediately','Back exits only one menu with a held press','Rules persist after leaving and reopening','Two window sizes','CSS header opens Rules and Back returns to CSS']; r['status']='PASS'
 finally:
  if c and c.process.poll() is None: close_window(c.process.pid); c.process.wait(timeout=20)
  (out/'review.json').write_text(json.dumps(r,indent=2))
 print('PASS stock Rules suite')
if __name__=='__main__': main()
