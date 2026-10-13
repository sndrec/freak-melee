"""Actual legacy holdable statics plus scheduler/common primitive bodies."""
import hashlib,importlib.util,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('holdable_scheduler',HERE.parent/'script_item_scheduler/extract_scheduler.py');scheduler=importlib.util.module_from_spec(spec);spec.loader.exec_module(scheduler)
def extract(root,output):
 path=root/'src/melee/it/item.c';source=scheduler.native_only(path.read_text());hashes={str(path):hashlib.sha256(path.read_bytes()).hexdigest()}
 names=['HSD_JObjSetScaleItem','HSD_JObjSetFacingDirItem','itemCallbackCurrent','itemStateSetupCurrent','Item_80268E5C','Item_8026A8EC','Item_OnUserDataRemove','Item_IsGrabbable']
 bodies=[scheduler.reference.body(source,name) for name in names]
 prototypes=[part[:part.index('{')].rstrip()+';' for part in bodies]
 output.write_text('// Actual common state/removal bodies.\n'+ '\n'.join(prototypes)+'\n'+ '\n\n'.join(bodies)+'\n')
 pieces=[]
 functions={
  'src/melee/it/it_2725.c':['it_80272860','it_80274658','itColl_BounceOffVictim','it_80273030','itColl_BounceOffShield','it_80273670'],
  'src/melee/it/itcoll.c':['it_8026FCF8','it_80272460'],
  'src/melee/lb/lbcollision.c':['lbColl_80008440','lbColl_CopyHitCapsule'],
  'src/melee/lb/lbvector.c':['lbVector_Mirror'],
  'src/melee/mp/mpcoll.c':['mpCollSetFacingDir']}
 for relative,names in functions.items():
  path=root/relative;text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  for name in names:pieces.append(scheduler.reference.body(text,name))
 path=root/'src/melee/it/native_holdable.c';text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 names=['definition','expire','held_animation','held_physics','active_animation','flying_physics','flying_collision','picked_up','thrown','dropped','hit','clanked','reflected','shield_bounced']
 for name in names:pieces.append(scheduler.reference.body(text,name))
 start=text.index('static ItemStateTable states[]');table=text[start:text.index('};',start)+2]
 for name in names+['states']:table=re.sub(r'\b'+name+r'\b','legacy_'+name,table)
 pieces.append(table)
 joined='\n\n'.join(pieces)
 for name in names:joined=re.sub(r'\b'+name+r'\b(?=\s*\()','legacy_'+name,joined)
 (output.parent/'actual_holdable.h').write_text('// Actual native statics: only local identifiers renamed to avoid collisions.\n'+joined+'\n')
 return hashes
