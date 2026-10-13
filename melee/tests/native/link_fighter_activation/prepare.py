"""Normal independent Link/Young clone/import/DLL ownership/package gate; no install or game."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
COMPOSITION=HERE.parent/'link_fighter_composition'
sys.path.insert(0,str(COMPOSITION))
from assemble import compose
sys.path.insert(0,str(HERE.parent))
from fighter_script_bundle_roundtrip import portrait,bundled_id

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def declaration(text,marker):
 start=text.index(marker);i=text.index('{',start);depth=1;j=i+1
 while depth:depth+=(text[j]=='{')-(text[j]=='}');j+=1
 return text[start:text.index(';',j)+1]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--iso',type=Path,default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso');args=p.parse_args()
 snap,out,iso=args.snapshot.resolve(),args.output.resolve(),args.iso.resolve()
 if out.exists() or any(out==i or i in out.parents for i in [snap,HERE,COMPOSITION]):p.error('use fresh output outside inputs')
 manifest=json.loads((snap/'freeze.json').read_text());out.mkdir(parents=True);commands=[]
 def verify():
  for n,h in manifest.items():
   if n.replace('\\','/').startswith(('sdk/','compiler/','bin/')) and sha(snap/n)!=h:raise RuntimeError('changed frozen input '+n)
 def run(stage,cmd):
  cmd=list(map(str,cmd));r=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True,timeout=180)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n');commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8',newline='\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-6000:])
  return r.stdout
 verify();sdk=snap/'sdk';tool=snap/'bin/native_content_tool.exe';clang=snap/'compiler/clang.exe'
 flags=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include')]
 header=(sdk/'src/melee/ft/native_fighter_runtime.h').read_text();definition=header.split('typedef struct NativeFighterDefinition {',1)[1].split('} NativeFighterDefinition;',1)[0]
 callbacks=sorted(set(re.findall(r'\(\*(\w+)\)',definition)+re.findall(r'\b(?:HSD_GObjEvent|HSD_GObjPredicate|Fighter_ItemEvent)\s+(\w+)\s*;',definition)))
 (out/'definition_callbacks.h').write_text(''.join('VERIFY_CALLBACK('+f+');\n' for f in callbacks),encoding='utf-8',newline='\n')
 image=out/'portrait.png';portrait(image);results=[];inputs=[HERE/'prepare.py',HERE/'ownership.c',COMPOSITION/'assemble.py',HERE.parent/'fighter_script_bundle_roundtrip.py',out/'definition_callbacks.h']
 for family,label,dir,stem,name,physical,bank,omit,count in [('link','Link','ftLink','ftlink','ftLinkClone',75,16,68,5),('young_link','Young Link','ftCLink','ftclink','ftCLinkClone',79,7,72,6)]:
  source,components=compose(family);inputs.extend(components)
  # The real copied table begins341. Common58..62 use the existing post-entry
  # binding hook, not out-of-range extended-row declarations. Helper decisions
  # remain the exact accepted source; no stock fighter policy is delegated.
  common_rows=re.compile(r'    on state_(?:58|59|60|61|62)_interrupt:\n        SwordCombo_Interrupt\(\)\n')
  source,removed=common_rows.subn('',source);assert removed==5
  source=source.replace('action SwordCombo:\n','action SwordCombo:\n    on forward_smash_start:\n        callbacks.interrupt(SwordCombo_Interrupt)\n',1)
  source='# UNREGISTERED independent activation candidate; console-only gate.\nfighter:\n    behavior independent\n\n'+source
  raw=out/(family+'.candidate.fms');raw.write_text(source,encoding='utf-8',newline='\n');project=out/family
  run(family+'_clone',[tool,'clone-stock',iso,label,project,'Unregistered independent '+label]);run(family+'_import',[tool,'import-parameters',project,raw,project/'behavior.fms'])
  key=re.search(r'^code=(\S+)$',(project/'project.txt').read_text(),re.M)[1];gen=project/'behavior.generated.c';dll=project/'module.dll'
  run(family+'_generate',[tool,'script-c',project/'behavior.fms',key,gen]);code=gen.read_text();assert 'generated.source_kind = Ft_Kind_None;' in code
  for bad in ['fighter->u.lk','fighter->mv.lk','itemVar.link','ftLk_','ftCl_']:
   if bad in code:raise RuntimeError('stock union/policy reference in generated source: '+bad)
  run(family+'_dll',flags+['-shared',gen,'-o',dll])
  stock=sdk/f'src/melee/ft/kinds/{dir}/{stem}.c';clone=sdk/f'src/melee/ft/kinds/{dir}/{stem}_clone.c';inputs.extend([stock,clone])
  table=declaration(stock.read_text(),('MotionState ftLk_Init_MotionStateTable' if family=='link' else 'MotionState ftCl_Init_MotionStateTable'))
  text=clone.read_text();base=''
  for m in ['static const NativeFighterBone '+name+'_Bones','static const NativeFighterPartOmission '+name+'_Omissions','static const NativeFighterArticleAsset '+name+'_ArticleAssets','static const Fighter_DemoStrings '+name+'_DemoMotionNames','const NativeFighterDefinition '+name+'_Definition']:
   if m in text:base+=declaration(text,m)+'\n'
  names=sorted(set(re.findall(r'\bft(?:Lk|Cl|Co)_\w+\b',table+base)))
  names=[n for n in names if not any(token in n for token in ['_MS_','_MF_','_SM_','_MotionStateTable'])];names.append('ftCamera_UpdateCameraBox')
  (project/'base.h').write_text('\n'.join('#define '+n+' stock_slot' for n in names)+'\n'+table+base+'\n'+'\n'.join('#undef '+n for n in names)+'\n',encoding='utf-8',newline='\n')
  required=sorted(set(re.findall(r'host->(\w+)\s*==\s*0',code)));(project/'required_host.h').write_text('\n'.join('host.'+n+'=(__typeof__(host.'+n+'))unexpected;' for n in required),encoding='utf-8',newline='\n')
  defs=[f'-DBASE_DEFINITION={name}_Definition',f'-DBASE_KIND=Ft_Kind_{"Link" if family=="link" else "CLink"}',f'-DPHYSICAL_JOINTS={physical}',f'-DSOUND_BANK={bank}',f'-DOMITTED_PART={omit}',f'-DARTICLE_COUNT={count}']
  exe=project/'ownership.exe';run(family+'_ownership_compile',flags+defs+['-I'+str(out),'-I'+str(project),HERE/'ownership.c','-o',exe]);ownership=json.loads(run(family+'_ownership',[exe,dll]))
  store=out/'isolated-store';package=bundled_id(run(family+'_bundle',[tool,'--root',store,'bundle-project',project,image,iso]));run(family+'_verify',[tool,'--root',store,'verify-package',package,iso])
  inputs.extend([raw,gen,dll,project/'behavior.fms',project/'fighter_parameters.fmp',project/'project.txt',project/'base.h',project/'required_host.h',*[project/n for n in ['fighter.dat','animation.dat','costume.dat','cpu_profile.fcp']]])
  results.append({'family':family,'package_id':package,'ownership':ownership,'generated_sha256':sha(gen),'dll_sha256':sha(dll),'independent':True})
 verify();(out/'receipt.json').write_text(json.dumps({'snapshot':str(snap),'snapshot_sha256':sha(snap/'freeze.json'),'results':results,'commands':commands,'input_sha256':{str(i):sha(i) for i in inputs},'status':'unregistered independent console/DLL/package validation only; not installed; no gameplay or renderer claim'},indent=2)+'\n',encoding='utf-8',newline='\n');print(out/'receipt.json')
if __name__=='__main__':main()
