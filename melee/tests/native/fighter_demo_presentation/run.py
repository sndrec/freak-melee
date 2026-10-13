"""Fresh frozen-tool clone/import -> generated DLL -> actual demo producer comparison."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import re
import subprocess
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent

def load(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
extract=load('balanced',ROOT/'tests/native/callback_engine_fixture.py')
select=load('native_selection',ROOT/'tests/native/fox_falco_cpu_stock_compare.py')

def extract_sources(sdk,out,family):
 facts={};body_facts={};chunks=['''
int ftMr_SpecialN_VitaminRandom(HSD_GObj*);
Item_GObj* itDrMarioPill_802C09C4(Fighter_GObj*,Vec3*,s32,ItemKind,s32,Fighter_Part,f32);
Item_GObj* it_802AE994(Fighter_GObj*,Fighter_Part,ItemKind);
bool ftCo_800BF228(Fighter_GObj*);
void it_802750F8(Item_GObj*);
static const int it_803F6E68[11]={0,9,9,3,9,9,6,7,8,9,10};
''']
 groups={
 'src/melee/ft/kinds/ftMario/ftmariospecialn.c':['pickMegavitamin','ftMr_SpecialN_VitaminRandom'],
 'src/melee/it/kinds/itdrmariopill.c':['itDrMarioPill_802C1180','itDrMarioPill_802C061C','itDrMarioPill_802C09C4','itDrMarioPill_PickedUp','copy_jobj_scale','itDrMarioPill_Motion6_Anim','itDrMarioPill_Motion6_Phys','itDrMarioPill_Motion6_Coll'],
 'src/melee/it/kinds/itfoxblaster.c':['itFoxBlaster_SetCommandVars','itFoxBlaster_ClearShot','it_802AE7B8','it_802AE994','itFoxBlaster_Logic96_PickedUp','itFoxblaster_UnkMotion10_Anim','itFoxblaster_UnkMotion10_Phys','itFoxblaster_UnkMotion10_Coll'],
 'src/melee/it/it_2725.c':['it_802750F8'],
 'src/melee/ft/ftlib.c':['ftLib_8008741C','ftLib_80087460'],
 'src/melee/ft/native_fighter_target.c':['nativeFighterTarget_Id'],
 'src/melee/ft/ftparts_table.c':['ftParts_GetBoneIndex'],
 'src/melee/it/kinds/inlines.h':['Item_InitSpawn'],
 'src/melee/it/native_item_owner.c':['nativeItem_Creator','find_live_item','find_created_item','nativeItem_BindFighter','nativeItem_Attach','nativeItem_ClearBinding'],
 'src/melee/it/native_article.c':['nativeArticle_Creator','article_logic','nativeArticle_Create'],
 'src/melee/it/item.c':['Item_8026A810','Item_8026AB54'],
 'src/melee/ft/native_fighter_module.c':['find_owned_article','signal_owned_article'],
 }
 for file,names in groups.items():
  p=sdk/file;data=p.read_bytes();facts[str(p)]=hashlib.sha256(data).hexdigest();source=data.decode().replace("\r\n","\n")
  for name in names:
   body=extract.function(source,name);chunks.append('// Actual '+file+'\n'+body)
   live=extract.function((ROOT/file).read_text(),name)
   body_facts[file+':'+name]={'frozen':hashlib.sha256(body.encode()).hexdigest(),'live':hashlib.sha256(live.encode()).hexdigest(),'equal':body==live}
   if body!=live:raise AssertionError('Live producer/API drift: '+file+':'+name)
 for file,names in [('src/melee/ft/ft_0BEC.c',['setupInitialState','FoxHelper','DocHelper','ftCo_800BECB0','ftCo_800BED88']),('src/melee/ft/ft_0BF0.c',['ftCo_800BF034','ftCo_800BF108','ftCo_800BF228'])]:
  p=sdk/file;data=p.read_bytes();facts[str(p)]=hashlib.sha256(data).hexdigest();raw=data.decode().replace("\r\n","\n");stock=select.select_native(raw,False)
  for name in names:
   frozen_body=extract.function(raw,name);live_body=extract.function((ROOT/file).read_text(),name)
   body_facts[file+':'+name]={'frozen':hashlib.sha256(frozen_body.encode()).hexdigest(),'live':hashlib.sha256(live_body.encode()).hexdigest(),'equal':frozen_body==live_body}
   if frozen_body!=live_body:raise AssertionError('Live demo-entry drift: '+file+':'+name)
   body=extract.function(stock,name)
   if name.startswith('ftCo_') and name!='ftCo_800BF228':body=body.replace(name+'(','stock_'+name+'(',1)
   chunks.append('// Stock raw-kind branch '+file+'\n'+body)
  for name in names:
   if name.startswith('ftCo_') and name!='ftCo_800BF228':chunks.append('// Actual native dispatch '+file+'\n'+extract.function(raw,name).replace(name+'(','own_'+name+'(',1))
 p=sdk/('src/melee/ft/kinds/ftDrMario/ftdrmario_clone.c' if family=='DrMario' else f'src/melee/ft/kinds/ft{family}/ft{family.lower()}_clone.c')
 text=p.read_text();facts[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();stem='DrMario' if family=='DrMario' else family
 chunks.append(select.declaration(text,f'static const NativeFighterBone ft{stem}Clone_Bones').replace(f'ft{stem}Clone_Bones','fixture_bones')+';')
 (out/'source_correspondence.json').write_text(json.dumps(body_facts,indent=2)+'\n',encoding='utf-8')
 (out/'actual.inc').write_text('\n\n'.join(chunks)+'\n',encoding='utf-8');return facts

def unused_host(sdk,project,out):
 required=set(re.findall(r'host->(\w+) == 0',(project/'behavior.generated.c').read_text()))
 provided={'callback_source_alive','random_int_excluding','create_article_at','signal_owned_article','bind_fighter_item','item_creator','item_change_state_flags','item_change_state','item_set_lifetime','item_use_manual_lifetime','item_set_appearance','item_reset_spin','item_refresh_physics','item_scale_by_attribute','item_set_uniform_scale','fighter_root_scale_y','fighter_incapacitated','item_joint_value','item_set_joint_value','resolve_bone','attach_created_item','clear_fighter_item','item_advance_animation','item_remove'}
 text=(sdk/'src/melee/ft/native_fighter_module.h').read_text().split('typedef struct NativeFighterHost {',1)[1].split('} NativeFighterHost;',1)[0]
 signatures={m[2]:(m[1].strip(),m[3].strip()) for m in re.finditer(r'^\s*([^;\n]+?)\(\*(\w+)\)\((.*?)\);',text,re.M|re.S)}
 for n in re.findall(r'^\s*HSD_GObjEvent (\w+);',text,re.M):signatures[n]=('void','HSD_GObj*')
 code=[];assign=[]
 for n in sorted(required-provided):
  result,args=signatures[n];parts=[];depth=begin=0
  for at,ch in enumerate(args):
   depth+=(ch=='(')-(ch==')')
   if ch==',' and depth==0:parts.append(args[begin:at].strip());begin=at+1
  parts.append(args[begin:].strip())
  named=[a if a in ('void','...') else a.replace('(*)',f'(*unused_{i})') if '(*)' in a else a+f' unused_{i}' for i,a in enumerate(parts)]
  code.append(f'static {result} trap_{n}('+','.join(named)+f') {{ require(false,"Unexpected Host {n}"); abort(); }}')
  assign.append(f'host->{n}=trap_{n};')
 (out/'unused_host.inc').write_text('\n'.join(code)+'\nstatic void supply_unused_host(NativeFighterHost* host) {\n'+'\n'.join(assign)+'\n}\n',encoding='utf-8')

def main():
 a=argparse.ArgumentParser();a.add_argument('--snapshot',type=Path,required=True);a.add_argument('--output',type=Path,required=True);a.add_argument('--iso',type=Path,required=True);a.add_argument('--reuse-projects',type=Path);args=a.parse_args()
 snap=args.snapshot.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True);sdk=snap/'sdk';tool=snap/'bin/native_content_tool.exe';facts={};results={}
 def run(command,log):
  r=subprocess.run([str(v) for v in command],cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW);log.write_bytes(r.stdout)
  if r.returncode:raise RuntimeError(str(log)+'\n'+r.stdout.decode(errors='replace'))
  return r.stdout.decode(errors='replace')
 for i,(family,template,key) in enumerate([('DrMario','drmario_full.fms','drmario_clone'),('Fox','fox_wip.fms','fox_clone'),('Falco','falco_wip.fms','falco_clone')]):
  case=out/family;case.mkdir(exist_ok=True);project=args.reuse_projects/family if args.reuse_projects else case/'project'
  if not args.reuse_projects:
   for command in [['clone-stock',args.iso,family,project,'Presentation '+family],['import-parameters',project,ROOT/'examples'/template,project/'behavior.fms'],['script-c',project/'behavior.fms',key,project/'behavior.generated.c']]:run([tool,'--root',out/'store']+command,case/(command[0]+'.log'))
  facts.update(extract_sources(sdk,case,family));unused_host(sdk,project,case)
  flags=[snap/'compiler/clang.exe','-fms-runtime-lib=dll','-std=gnu11','-O2','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(case)]
  dll=case/'module.dll';exe=case/'test.exe'
  commands=[flags+['-shared',project/'behavior.generated.c','-o',dll],flags+[HERE/'test.c',sdk/'src/melee/ft/native_fighter_parameters.c',sdk/'src/melee/ft/native_callback_lifetime.c',ROOT/'tests/native/model_effect_absent_fixture.c','-o',exe]]
  setup=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat');cmd=case/'compile.cmd'
  cmd.write_text('@echo off\ncall '+subprocess.list2cmdline([str(setup)])+' -vcvars_ver=14.44 >nul\n'+'\nif errorlevel 1 exit /b 1\n'.join(subprocess.list2cmdline(list(map(str,c))) for c in commands)+'\n',encoding='utf-8')
  run(['cmd','/c',cmd],case/'compile.log');results[family]=run([exe,dll,project/'fighter_parameters.fmp',str(i)],case/'result.log');print(results[family],end='')
  for p in [project/'behavior.fms',project/'behavior.generated.c',project/'fighter_parameters.fmp',case/'actual.inc',case/'unused_host.inc',case/'source_correspondence.json',dll,exe]:facts[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 for p in [HERE/'run.py',HERE/'test.c',ROOT/'examples/fox_wip.fms',ROOT/'examples/falco_wip.fms',ROOT/'examples/drmario_full.fms',snap/'freeze.json',tool,snap/'compiler/clang.exe',sdk/'src/melee/ft/native_callback_lifetime.c',ROOT/'tests/native/model_effect_absent_fixture.c']:facts[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 (out/'receipt.json').write_text(json.dumps({'snapshot':str(snap),'results':results,'sha256':facts},indent=2)+'\n',encoding='utf-8')
 return 0
if __name__=='__main__':raise SystemExit(main())
