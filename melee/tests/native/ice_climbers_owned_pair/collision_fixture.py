"""Compose frozen accepted modules and pin full original primary Squall collision bodies."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('root','baseline','reference','output','iso'):p.add_argument('--'+name,type=Path,required=True)
 x=p.parse_args();root=x.root.resolve();out=x.output.resolve();reference=x.reference.resolve()/'sdk'
 command=[sys.executable,HERE.parent/'ice_climbers_linked/paired_fixture.py']
 for name in ('root','baseline','reference','output','iso'):command.extend(['--'+name,getattr(x,name)])
 subprocess.run(list(map(str,command)),cwd=root,check=True)
 prov_forms={}
 for index,form in enumerate(('popo','nana')):
  import shutil
  staged=out/(form+'-composed')
  command=[sys.executable,HERE/'prepare.py']
  for name in ('root','baseline','iso'):command.extend(['--'+name,getattr(x,name)])
  command.extend(['--output',staged,'--member',form]);subprocess.run(list(map(str,command)),cwd=root,check=True)
  for name in ('owned.fms','fighter_parameters.fmp'):shutil.copyfile(staged/name,out/form/name)
  prep=json.loads((staged/'preparation.json').read_text(encoding='utf-8'))
  prov_forms[form]=prep
  initializers=[]
  for q in prep['parameters']:
   if q['article_slot'] is None and q['source_offset'] is not None:
    initializers.append('{uint32_t bits=UINT32_C(%d);memcpy((char*)&attributes[%d]+%d,&bits,4);}'%(q['bits'],index,q['source_offset']))
  (out/form/'attributes.inc').write_text('\n'.join(initializers)+'\n',encoding='utf-8')
 spec=importlib.util.spec_from_file_location('accepted_extract',HERE.parent/'ice_climbers_linked/paired_fixture.py')
 extraction=importlib.util.module_from_spec(spec);spec.loader.exec_module(extraction)
 bodies=[];prov=json.loads((out/'provenance.json').read_text(encoding='utf-8'))
 for relative,wanted in {
  'src/melee/ft/kinds/ftPopo/ftpopospecialhi.c':lambda n:n!='sdata2_order',
  'src/melee/ft/kinds/ftPopo/ftpopospecials.c':lambda n:n=='ftPp_SpecialS_8011F720' or n in ('inline2','inline3','inline4') or n.endswith('_Coll'),
  'src/melee/lb/lbvector.c':lambda n:n=='lbVector_Normalize',
  'src/melee/ft/ftcommon.c':lambda n:n=='ftCommon_UpdateFacing',
 }.items():
  path=reference/relative;source=path.read_text(encoding='utf-8');entries={}
  for name in extraction.names(source):
   if not wanted(name):continue
   body=extraction.body(source,name);bodies.append(body)
   entries[name]={'sha256':hashlib.sha256(body.encode()).hexdigest(),'line':source[:source.index(body)].count('\n')+1}
  prov['sources'][relative]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bodies':entries}
 # Exact primary collision box, distinct from the secondary box.
 import re
 text=(reference/'src/melee/ft/kinds/ftPopo/ftpopospecials.c').read_text(encoding='utf-8')
 box=re.search(r'(?s)static ftCollisionBox ftNn_Init_803CD820 = \{.*?\};',text).group()
 with (out/'actual.inc').open('a',encoding='utf-8') as f:
  f.write('\n'+box+'\n'+'\n'.join(b[:b.index('{')].strip()+';' for b in bodies)+'\n'+'\n\n'.join(bodies)+'\n')
 for form in ('popo','nana'):prov['forms'][form]['parameters']=prov_forms[form]['parameters']
 prov['scope']='Four complete primary Squall collisions; NULL retained effect transport; original transition/clamp bodies; controlled stage/motion/animation; full assembled two-member callback census'
 (out/'provenance.json').write_text(json.dumps(prov,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
