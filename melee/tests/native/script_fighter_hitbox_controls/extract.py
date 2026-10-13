"""Actual control references; reuse frozen coordinate guard/engine substrate."""
import hashlib,importlib.util,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
base=HERE.parent/'script_fighter_hitbox_coordinates/extract.py'
spec=importlib.util.spec_from_file_location('coordinates',base)
coordinates=importlib.util.module_from_spec(spec);spec.loader.exec_module(coordinates)
body=coordinates.module.body

def extract(root,out):
 hashes=coordinates.extract(root,out)
 hashes[str(base)]=hashlib.sha256(base.read_bytes()).hexdigest()
 def read(path):
  p=root/path;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 parts=[]
 for path,names in [('src/melee/ft/kinds/ftSeak/ftseakspecials.c',['ftSeakSpecialS_LoopChainHitActivate','ftSeakSpecialS_LoopChainHitCollisions']),
                    ('src/melee/ft/kinds/ftNess/ftnessattackhi4.c',['ftNs_AttackHi4_YoyoCheckTimedRehit'])]:
  text=read(path);parts.extend(body(text,name) for name in names)
 fighter=read('src/melee/ft/fighter.c')
 start=fighter.index('if ((flags & Ft_MF_SkipHit) == 0)',fighter.index('void Fighter_ChangeMotionState'))
 end=fighter.index('ftColl_8007AFF8(gobj);',start)+len('ftColl_8007AFF8(gobj);')
 end=fighter.index('}',end)+1
 parts.append('static void reference_cleanup(Fighter_GObj* gobj,MotionFlags flags)\n{Fighter* fp=GET_FIGHTER(gobj);\n'+fighter[start:end]+'\n}\n}')
 (out/'control_native.inc').write_text('\n\n'.join(parts)+'\n')
 sheik=read('examples/sheik_full.fms');ness=read('examples/ness_wip.fms')
 def helper(text,name):
  match=re.search(r'^func '+name+r'\(.*?(?=^func |^move |^action |^article |\Z)',text,re.M|re.S)
  if not match:raise RuntimeError('missing source helper '+name)
  return match.group(0).rstrip()+'\n'
 script='fighter:\n    behavior independent\nstate int calls\nstate int yoyo_rehit_timer\n'
 for name in ['chain_zero_capsules','chain_activate','chain_disable']:script+=helper(sheik,name)
 script+=helper(ness,'YoyoCheckTimedRehit')
 script+='func Index() -> int:\n    state.calls = state.calls + 1\n    return 1\nfunc EnableOnce(bool value):\n    hitbox.set_enabled(Index(), value)\nfunc ClearOnce():\n    hitbox.clear_victims(Index())\nfunc Enable(int slot, bool value):\n    hitbox.set_enabled(slot, value)\nfunc Clear(int slot):\n    hitbox.clear_victims(slot)\nfunc ClearAll():\n    hitbox.clear_all()\nfunc Timer(int value):\n    state.yoyo_rehit_timer = value\nfunc TimerValue() -> int:\n    return state.yoyo_rehit_timer\nfunc Calls() -> int:\n    return state.calls\nfunc Fault():\n    let int zero = 0\n    state.calls = 1 / zero\n'
 (out/'coordinate_source.inc').write_text('static const char* script_source='+json.dumps(script)+';\n')
 return hashes
