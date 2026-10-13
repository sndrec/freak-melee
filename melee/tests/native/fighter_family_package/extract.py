"""Exact data-only codec dependency bodies, no game or authoring donor."""
import re,hashlib
from pathlib import Path
def body(text,name):
 m=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not m:raise RuntimeError(name)
 end=text.index('{',m.start())+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[m.start():end]
def extract(root,out):
 root=Path(root);out=Path(out);out.mkdir(parents=True,exist_ok=True);parts=[];hashes={}
 groups=[('src/melee/ft/native_source_asset_validation.c',['error','name','nativeSourceAssets_Validate'],'facts_error'),('src/melee/ft/native_script_moves.c',['error','nativeScriptMoves_Validate'],'moves_error'),('src/melee/ft/ftcharacter_asset_layout.c',['ftCharacterAssetLayout_IsValid'],None),('src/melee/ft/native_fighter_pose_path.c',['nativeFighterPose_PathValid'],None)]
 for relative,names,rename in groups:
  path=root/relative;text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  parts.extend(body(text,n).replace('error(',rename+'(') if rename else body(text,n) for n in names)
 (out/'codec_dependencies.inc').write_text('\n\n'.join(parts)+'\n');return hashes
