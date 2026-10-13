"""Actual Item dispatch, state setup, cleanup and constructor bodies."""
import hashlib,importlib.util
import re
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('scheduler_body',HERE.parent/'script_kinetics/extract_primitives.py');reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
FUNCTIONS={
 'src/melee/it/item.c':['HSD_JObjSetScaleItem','HSD_JObjSetFacingDirItem','checkHitLag','itemCallbackCurrent','itemStateSetupCurrent','Item_80268E5C','Item_802693E4',
  'Item_80269528','Item_802697D4','Item_80269978','Item_80269A9C','Item_80269B60','Item_80269BE4',
  'Item_80269DC8','Item_80269F14','Item_8026A158','Item_8026A1E8','EnterHitlagThink',
  'processSourceCallback','processCallback','OnTakeDamageThink','OnClankThink','OnGiveDamageThink',
  'Item_8026A294','Item_8026A810','Item_8026A8EC','Item_8026AB54','Item_8026ABD8',
  'Item_8026AC74','Item_8026AD20','Item_8026ADC0','Item_OnUserDataRemove','Item_IsGrabbable',
  'Item_80267AA8','Item_8026862C'],
 'src/melee/it/it_26B1.c':['RunCallbackUnk','it_8026B7F8','it_8026C368'],
 'src/melee/it/itgroundcoll.c':['it_8026DD5C'],
}
def native_only(source):
 output=[];stack=[];active=True
 for line in source.splitlines(keepends=True):
  directive=re.match(r'\s*#\s*(if|ifdef|ifndef|else|endif)\b(.*)',line)
  if directive:
   kind,condition=directive.groups();condition=condition.strip()
   if kind in ('if','ifdef','ifndef'):
    known=condition=='MELEE_NATIVE' or condition=='defined(MELEE_NATIVE)'
    stack.append((active,known))
    if known:active=active and kind!='ifndef';continue
   elif kind=='else' and stack:
    parent,known=stack[-1]
    if known:active=parent and not active;continue
   elif kind=='endif' and stack:
    parent,known=stack.pop()
    if known:active=parent;continue
  if active:output.append(line)
 return ''.join(output)
def extract(root,output):
 pieces=[];inputs={};prototypes=[]
 for relative,names in FUNCTIONS.items():
  path=root/relative;source=native_only(path.read_text());inputs[str(path.resolve())]=hashlib.sha256(path.read_bytes()).hexdigest()
  for name in names:
   piece=reference.body(source,name);pieces.append(piece);prototypes.append(piece[:piece.index('{')].rstrip()+';')
 source=native_only((root/'src/melee/it/it_26B1.c').read_text());first=source.index('typedef struct ItemInteractionReference');last=source.index('/// Remove item from player on death?',first)
 scratch=source[first:last]
 output.write_text('// Mechanically extracted actual scheduler bodies.\n#pragma clang diagnostic push\n#pragma clang diagnostic ignored "-Wunused-variable"\n#pragma clang diagnostic ignored "-Wunused-function"\n#pragma clang diagnostic ignored "-Wsign-compare"\n'+scratch+'\n'+ '\n'.join(prototypes)+'\n'+ '\n\n'.join(pieces)+'\n#pragma clang diagnostic pop\n',newline='\n')
 return inputs
