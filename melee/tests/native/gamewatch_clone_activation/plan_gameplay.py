"""Prepare six GameWatch stock/independent replay probes; never launches a game.

Requires a verified SDK65 and completed isolated package gate. Output commands
use the existing parity runner defaults: hidden, muted, fast, drawing enabled.
Root coordinates acceptance and launches separately.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
CASES=[
 {'name':'chef-ground','motions':[353],'item_kinds':[122],'samples':550},
 {'name':'judge-ground','motions':[],'motion_any':list(range(355,364)),'item_kinds':[120],'samples':550},
 {'name':'rescue-ground','motions':[373],'item_kinds':[124],'samples':650},
 {'name':'panic-ground-empty','motions':[375],'item_kinds':[],'samples':550},
 {'name':'normal-ground-sequence','motions':[341,345,346],'item_kinds':[114,115,116],'samples':850},
 {'name':'neutral-aerial','motions':[347],'item_kinds':[117],'samples':650,'requested_additional_motion':350},
]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def replay(template,name,count):
 records=bytearray()
 for tick in range(count):
  sample=bytearray(template[24:104]);sample[8:72]=bytes(64)
  button=x=y=cx=cy=0
  if name=='normal-ground-sequence':
   if tick in (150,350):button=1<<8
   if 340<=tick<365:y=-80
   if tick==500:cx=80
  elif name=='neutral-aerial':
   if 145<=tick<153:button=1<<10
   if tick==170:button=1<<8
  else:
   if tick==150 or (name in ('chef-ground','panic-ground-empty') and 150<=tick<190):button=1<<9
   if tick==150:
    if name=='judge-ground':x=80
    elif name=='rescue-ground':y=80
    elif name=='panic-ground-empty':y=-80
  struct.pack_into('<Hbbbb',sample,8,button,x,y,cx,cy);records.extend(sample)
 return template[:24]+records

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--snapshot',required=True,type=Path)
 ap.add_argument('--prepared',required=True,type=Path)
 ap.add_argument('--output',required=True,type=Path)
 ap.add_argument('--iso',type=Path,default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso')
 a=ap.parse_args();snap,prepared,out=map(Path.resolve,(a.snapshot,a.prepared,a.output))
 if out.exists() or any(out==p or p in out.parents for p in (snap,prepared,HERE)):ap.error('fresh independent output required')
 manifest=json.loads((snap/'freeze.json').read_text())
 for name,digest in manifest.items():
  if sha(snap/name)!=digest:raise ValueError('snapshot changed '+name)
 receipt_path=prepared/'receipt.json';receipt=json.loads(receipt_path.read_text())
 if not receipt['installed_only_in_test_store'] or not receipt['status'].startswith('PASS'):ap.error('completed scratch install package receipt required')
 if receipt['freeze_sha256']!=sha(snap/'freeze.json'):ap.error('package was built with a different SDK')
 store=prepared/'isolated-store'
 if not store.is_dir():ap.error('isolated store absent')
 template_path=ROOT/'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr';template=template_path.read_bytes()
 assert template[:8]==b'FMREPLAY' and (len(template)-24)%80==0
 out.mkdir(parents=True)
 commands=[]
 for case in CASES:
  fmr=out/(case['name']+'.fmr');fmr.write_bytes(replay(template,case['name'],case['samples']))
  cmd=[sys.executable,ROOT/'tools/run_fighter_parity.py','--exe',snap/'bin/melee_native.exe','--iso',a.iso.resolve(),'--inputs',fmr,'--stock-fighter','mrgameandwatch','--package-id',receipt['package_id'],'--content-root',store,'--out-dir',out/case['name'],'--independent','--p2','human:zelda']
  for motion in case['motions']:cmd+=['--expect-motion',motion]
  for kind in case['item_kinds']:cmd+=['--expect-item-kind',kind]
  commands.append(dict(case,command=list(map(str,cmd)),input_sha256=sha(fmr)))
 plan={'status':'PREPARED ONLY; no game launched','snapshot':str(snap),'freeze_sha256':sha(snap/'freeze.json'),'package_receipt_sha256':sha(receipt_path),'package_id':receipt['package_id'],'isolated_store':str(store),'accept_before_launch':[str(snap/'bin/native_content_tool.exe'),'--root',str(store),'accept',receipt['package_id'],str(a.iso.resolve())],'commands':commands,'inputs':{str(p):sha(p) for p in (Path(__file__),ROOT/'tools/run_fighter_parity.py',ROOT/'tools/compare_fighter_parity.py',template_path)},'judge_requirement':'Post-run require one355..363 motion in both traces; --expect-item-kind120 additionally requires Judge creation','panic_requirement':'Empty-charge375; no article expected. Panic121 is created by the charged shoot path, outside this probe.','aerial_requirement':'347+Parachute117 required;350 is requested landing coverage, classify absent as fixture coverage gap','execution_policy':'Existing runner uses --hidden --mute --fast; drawing remains enabled. No --show-window/OS inputs/shader warmup. Never touch human store.','scope':'Six local smoke parity pairs with exact full recorded traces where completed. Not all10articles,allcostumes,collisioncontacts,reflection,damagecleanup,CPU/results/rollback,or pixel/material acceptance.'}
 (out/'plan.json').write_text(json.dumps(plan,indent=2)+'\n',newline='\n');print(out/'plan.json')
if __name__=='__main__':main()
