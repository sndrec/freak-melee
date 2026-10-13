"""Actual owned layer import/bind/draw console matrix; no GX or game launch."""
import argparse, hashlib, json, re, shutil, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def body(text,name):
 m=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text)
 if not m: raise ValueError('missing definition '+name)
 depth,end=1,m.end()
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[m.start():end]
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--snapshot',required=True,type=Path);ap.add_argument('--output',required=True,type=Path)
 ap.add_argument('--source-root',type=Path,default=ROOT)
 ap.add_argument('--iso',type=Path,default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso')
 a=ap.parse_args();snap=a.snapshot.resolve();out=a.output.resolve();root=a.source_root.resolve()
 if out.exists() or snap in out.parents or HERE in out.parents: ap.error('fresh output outside input namespaces required')
 frozen=json.loads((snap/'freeze.json').read_text());inputs={};commands=[]
 def verify():
  for name,digest in frozen.items():
   if name.replace('\\','/').startswith(('sdk/','compiler/')) and sha(snap/name)!=digest: raise RuntimeError('changed frozen input '+name)
 verify();out.mkdir(parents=True);sdk=snap/'sdk';overlay=out/'overlay/melee/it';overlay.mkdir(parents=True)
 for p in (sdk/'src/melee/it').glob('*.h'):
  shutil.copy2(p,overlay/p.name);inputs[str(p)]=sha(p)
 for name in ['article_model_layers.h','forward.h','types.h','itCharItems.h','itCommonItems.h','itdraw.h','itzako.h','itarticle_data.h']:
  p=root/'src/melee/it'/name;shutil.copy2(p,overlay/name);inputs[str(p)]=sha(p)
 # Extract exact fighter attribute import bodies; all normal data/graph and full
 # article importer code run from their real production units.
 p=root/'src/melee/ft/ftcharacter_assets.c';text=p.read_text();inputs[str(p)]=sha(p)
 pieces=[body(text,n) for n in ['fail','resolve','local','read_f32_words','import_gamewatch_article_attributes','import_gamewatch_chef_attributes']]
 (out/'fighter_import_bodies.h').write_text('\n\n'.join(pieces)+'\n',newline='\n')
 pieces=[]
 for rel,names in [('src/sysdolphin/baselib/jobj.c',['HSD_JObjGetFlags','HSD_JObjSetFlags','HSD_JObjSetFlagsAll','HSD_JObjClearFlags','HSD_JObjClearFlagsAll']),('src/melee/it/it_2725.c',['it_80272A18','it_80272A3C']),('src/melee/it/itdraw.c',['it_8026EBC8','it_8026EC54','it_8026EECC_inline_1','it_8026EECC_inline_2','it_8026EECC_inline_3','it_8026EECC_inline_0','it_8026EECC_inline_sw','it_8026EECC']),('src/melee/it/itzako.c',['it_8027CE64']),('src/melee/it/item.c',['Item_802680CC']),('src/melee/it/it_2725.c',['it_2725_JObjSetTranslateInline','it_80273318'])]:
  p=root/rel;text=p.read_text();inputs[str(p)]=sha(p);pieces.extend(body(text,n) for n in names)
 (out/'render_bodies.h').write_text('\n\n'.join(pieces)+'\n',newline='\n')
 sys.path.insert(0,str(sdk/'tools'));from extract_native_animation import extract_clip
 dat,offset=extract_clip(a.iso,'PlGw.dat','ftDataGamewatch');datfile=out/'PlGw.dat';datfile.write_bytes(dat)
 units=['dat','dat_import','dat_material','dat_texture','dat_animation','dat_model','dat_scene','dat_geometry']
 sources=[sdk/'src/sysdolphin/baselib'/f'{n}.c' for n in units]
 sources.append(sdk/'src/melee/it/itcommand_data.c')
 for name in ['article_model_layers.c','itarticle_data.c']:
  p=root/'src/melee/it'/name;text=p.read_text()
  for header in ['article_model_layers.h','types.h','itarticle_data.h','itCharItems.h','itCommonItems.h','kinds/itkinoko.h']:
   text=text.replace('#include "'+header+'"','#include <melee/it/'+header+'>')
  dst=out/name;dst.write_text(text,newline='\n');sources.append(dst);inputs[str(p)]=sha(p)
 inputs.update({str(p):sha(p) for p in sources})
 exe=out/'layers_test.exe';cmd=[str(snap/'compiler/clang.exe'),'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(out/'overlay'),'-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out),str(HERE/'test.c'),*map(str,sources),'-o',str(exe)]
 for stage,cmd in [('compile',cmd),('run',[str(exe),str(datfile)])]:
  p=subprocess.run(cmd,capture_output=True,text=True,timeout=120,cwd=ROOT);(out/(stage+'.log')).write_text(p.stdout+p.stderr,newline='\n');commands.append({'stage':stage,'command':cmd,'exit':p.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',newline='\n')
  if p.returncode: raise RuntimeError(stage+' failed: '+p.stdout+p.stderr)
  if stage=='run': result=json.loads(p.stdout.strip())
 verify();inputs.update({str(p):sha(p) for p in [HERE/'run.py',HERE/'test.c',out/'fighter_import_bodies.h',out/'render_bodies.h']})
 receipt={'status':'PASS actual source/import/graph console scope; no GX/pixel/game acceptance','snapshot_sha256':sha(snap/'freeze.json'),'dat_sha256':sha(datfile),'dat_root':offset,'result':result,'inputs':inputs,'commands':commands,'boundaries':['Full actual itArticle_Import and production DAT/graph/command transcoders execute. Fighter layer attribute bodies are exact extracted real functions, not full fighter importer.','HSD graph construction from immutable descriptors is fixture-controlled; actual runtime validation and actual subtree flag functions execute.','Parent color getters, diffuse/scale, camera/displacement, draw/GX and material state sinks controlled. Actual CE64 and complete three-pass dispatch body execute.','No HSD allocator, renderer pixels, material pipeline or gameplay acceptance.']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(result))
if __name__=='__main__':main()
