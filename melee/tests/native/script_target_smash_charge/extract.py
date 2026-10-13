"""Reuse actual target guards, then add exact native smash-charge bodies."""
import hashlib,importlib.util
from pathlib import Path
def module(path,name):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def extract(root,out):
 path=root/'tests/native/script_target_queries/extract.py';hashes=module(path,'target_base').extract(root,out);hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 p=root/'tests/native/script_form_feedback/extract.py';m=module(p,'native_body');hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 with (out/'native_target.inc').open('a') as f:
  for relative,name in [('src/melee/ft/ftlib.c','ftLib_800876D4'),('src/melee/ft/native_fighter_target.c','nativeFighterTarget_SmashChargeActive')]:
   p=root/relative;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();f.write(m.body(p.read_text(),name)+'\n')
 return hashes
