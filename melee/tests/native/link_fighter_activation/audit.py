"""Read-only independent caller inventory with exact frozen/source hashes."""
import argparse,hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--composition-audit',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--projects',type=Path)
 a=p.parse_args();snap=a.snapshot.resolve();out=a.output.resolve()
 if out.exists():p.error('use fresh output')
 source=json.loads(a.composition_audit.read_text());sdk=snap/'sdk';manifest=json.loads((snap/'freeze.json').read_text())
 for n,h in manifest.items():
  if n.replace('\\','/').startswith(('sdk/','compiler/','bin/')) and sha(snap/n)!=h:raise RuntimeError('changed frozen input '+n)
 sites=[];inputs=[]
 for path in sorted((sdk/'src/melee').rglob('*.c')):
  rel=path.relative_to(sdk/'src').as_posix()
  if any(token in rel for token in ['/ftLink/','/ftCLink/','/ftKirby/']):continue
  lines=path.read_text(encoding='utf-8').splitlines();found=[{'line':i+1,'text':line.strip()} for i,line in enumerate(lines) if re.search(r'\bFt_Kind_(?:Link|CLink)\b|\bft(?:Lk|Cl)_|\bu\.lk\b|\bmv\.lk\b',line)]
  if found:sites.append({'path':rel,'sites':found,'sha256':sha(path)});inputs.append(path)
 policies={}
 for section,paths in {'cpu':['ft/ftcpuattack.c','ft/kinds/ftCommon/ftCo_0A01.c'],'effect':['ef/eflib.c','ef/efsync.c','ef/efasync.c']}.items():
  policies[section]=[]
  for rel in paths:
   path=sdk/'src/melee'/rel;inputs.append(path);text=path.read_text();matches=re.findall(r'\bFt_Kind_(?:Link|CLink)\b|\bft(?:Lk|Cl)_\w+|(?:u|mv)\.lk',text)
   policies[section].append({'path':rel,'sha256':sha(path),'named_link_policy_matches':matches})
   if matches:raise RuntimeError('new named policy needs review: '+rel)
 shared_callers=[]
 for rel in ['ft/kinds/ftCommon/ftCo_AttackS4.c','ft/ftattacks4combo.c','ft/kinds/ftCommon/ftCo_AttackAir.c','ft/ft_08A1.c','ft/kinds/ftCommon/ftCo_SquatWait.c','ft/kinds/ftCommon/ftCo_Catch.c','ft/kinds/ftCommon/ftCo_0D8E.c','ft/kinds/ftCommon/ftCo_CatchPull.c','ft/kinds/ftCommon/ftCo_CatchWait.c','ft/kinds/ftCommon/ftCo_CaptureWait.c','ft/kinds/ftCommon/ftCo_AirCatch.c','ft/ftafterimage.c','ft/kinds/ftCommon/ftCo_AppealS.c']:
  path=sdk/'src/melee'/rel;inputs.append(path);shared_callers.append({'path':rel,'sha256':sha(path)})
 generated=[]
 if a.projects:
  for family in ['link','young_link']:
   path=a.projects.resolve()/family/'behavior.generated.c';text=path.read_text()
   for bad in ['fighter->u.lk','fighter->mv.lk','itemVar.link','ftLk_','ftCl_']:
    if bad in text:raise RuntimeError('unexpected stock policy/union '+bad)
   generated.append({'family':family,'path':str(path),'sha256':sha(path),'source_kind_none':'generated.source_kind = Ft_Kind_None;' in text});inputs.append(path)
   assert generated[-1]['source_kind_none']
 out.mkdir(parents=True);result={'snapshot':str(snap),'manifest_sha256':sha(snap/'freeze.json'),'composition_audit_sha256':sha(a.composition_audit),'original_callback_tables':source['callback_tables'],'normal_rig_and_cpu':source['graphs'],'common_named_sites':sites,'explicit_cpu_effect_search':policies,'generated':generated,'explicit_shared_callers':shared_callers,'input_sha256':{str(i):sha(i) for i in inputs},'remaining_generic_api_gaps':[],'metadata_gate':'baseline51 real copied bases own5/6article rows +53semantic bindings +one omission; prepare.py validates exact production tables','shared_entry_scope':'ownership gate directly invokes generated forward_smash_start binding after fixture-selected58..62 motion; full angle-selection/motion/animation caller and continuation liveness are separate root checks','limits':['textual/source inventory and DLL ownership are not gameplay/render/CPU/results acceptance','Kirby copy, Classic/video excluded','full Bow fighter phase and non-unit Boomerang flight replay remain separate evidence']}
 (out/'caller_inventory.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n');print(out/'caller_inventory.json')
if __name__=='__main__':main()
