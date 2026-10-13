"""Compose frozen accepted modules and pin full original primary Belay bodies."""
import argparse,hashlib,importlib.util,json,re,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('root','baseline','reference','output','iso'):p.add_argument('--'+name,type=Path,required=True)
 x=p.parse_args();root=x.root.resolve();out=x.output.resolve();reference=x.reference.resolve()/'sdk'
 command=[sys.executable,HERE.parent/'ice_climbers_linked/paired_fixture.py']
 for name in ('root','baseline','reference','output','iso'):command.extend(['--'+name,getattr(x,name)])
 subprocess.run(list(map(str,command)),cwd=root,check=True)
 staged=out/'primary'
 command=[sys.executable,HERE/'prepare.py']
 for name in ('root','baseline','iso'):command.extend(['--'+name,getattr(x,name)])
 command.extend(['--output',staged]);subprocess.run(list(map(str,command)),cwd=root,check=True)
 import shutil
 for name in ('owned.fms','fighter_parameters.fmp','PlPp.dat'):shutil.copyfile(staged/name,out/'popo'/name)
 prep=json.loads((staged/'preparation.json').read_text(encoding='utf-8'))
 (out/'rope_attributes.inc').write_text('\n'.join('{uint32_t bits=UINT32_C(%d);memcpy((char*)&rope_attributes+%d,&bits,4);}'%(q['bits'],q['source_offset']) for q in prep['parameters'] if q['article_slot']==2 and q['source_offset'] is not None)+'\n',encoding='utf-8')
 initializers=[]
 for q in prep['parameters']:
  if q['article_slot'] is None and q['source_offset'] is not None:
   initializers.append('{uint32_t bits=UINT32_C(%d);memcpy((char*)&attributes[0]+%d,&bits,4);}'%(q['bits'],q['source_offset']))
 (out/'popo/attributes.inc').write_text('\n'.join(initializers)+'\n',encoding='utf-8')
 spec=importlib.util.spec_from_file_location('accepted_extract',HERE.parent/'ice_climbers_linked/paired_fixture.py')
 extraction=importlib.util.module_from_spec(spec);spec.loader.exec_module(extraction)
 bodies=[];prov=json.loads((out/'provenance.json').read_text(encoding='utf-8'))
 for relative,wanted in {
  'src/melee/ft/kinds/ftPopo/ftpopospecialhi.c':lambda n:n!='sdata2_order',
  'src/melee/lb/lbvector.c':lambda n:n=='lbVector_Normalize',
  'src/melee/ft/ftcommon.c':lambda n:n=='ftCommon_UpdateFacing',
 }.items():
  path=reference/relative;source=path.read_text(encoding='utf-8');entries={}
  for name in re.findall(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*?\b(\w+)\s*\([^;{}]*\)\s*\{',source):
   if not wanted(name):continue
   body=extraction.body(source,name);bodies.append(body)
   entries[name]={'sha256':hashlib.sha256(body.encode()).hexdigest(),'line':source[:source.index(body)].count('\n')+1}
  prov['sources'][relative]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bodies':entries}
 with (out/'actual.inc').open('a',encoding='utf-8') as f:
  f.write('\n'+'\n'.join(b[:b.index('{')].strip()+';' for b in bodies)+'\n'+'\n\n'.join(bodies)+'\n')
 prov['forms']['popo']['parameters']=prep['parameters']
 prov['scope']='Primary Belay phases with complete original primary+recipient bodies; controlled motion/animation/stage/attachment and article-constructor boundaries; rope allocation/lifecycle separate'
 (out/'provenance.json').write_text(json.dumps(prov,indent=2)+'\n',encoding='utf-8')
 bodies=[]
 for relative,wanted in {
  'src/melee/it/kinds/itclimbersstring.c':lambda n:True,
  'src/melee/it/kinds/itlinkhookshot.c':lambda n:n in ['it_802A3C98','it_802A4420','it_802A6DC8','it_802A2474','it_802A24A0'],
  'src/melee/it/kinds/inlines.h':lambda n:n=='Item_RetractChain',
  'src/melee/it/it_2725.c':lambda n:n=='it_80275328',
  'src/melee/it/item.c':lambda n:n=='Item_8026AB54',
  'src/melee/ft/kinds/ftPopo/ftpopo.c':lambda n:n in ['ftPp_Init_8011F060','ftPp_Init_8011F16C','ftPp_Init_8011F190'],
  'src/melee/ft/kinds/ftPopo/ftpopospeciallw.c':lambda n:n=='ftPp_SpecialHi_80122898',
  'src/melee/it/kinds/itclimbersice.c':lambda n:n in ['it_802C17DC','it_2725_Logic90_Destroyed'],
 }.items():
  path=reference/relative;source=path.read_text(encoding='utf-8');entries={}
  for name in re.findall(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*?\b(\w+)\s*\([^;{}]*\)\s*\{',source):
   if not wanted(name):continue
   body=extraction.body(source,name);bodies.append(body)
   entries[name]={'sha256':hashlib.sha256(body.encode()).hexdigest(),'line':source[:source.index(body)].count('\n')+1}
  prov['sources'][relative]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bodies':entries}
 (out/'constructor_actual.inc').write_text('\n'.join(b[:b.index('{')].strip()+';' for b in bodies)+'\n'+'\n\n'.join(bodies)+'\n',encoding='utf-8')
 # Actual candidate constructor and creator resolver; no parallel allocation algorithm.
 source=(x.baseline.resolve()/'sdk/src/melee/it/native_article.c').read_text(encoding='utf-8')
 article_source=source
 names=['nativeArticle_Creator','article_logic','nativeArticle_Create']
 bodies=[extraction.body((x.baseline.resolve()/'sdk/src/melee/it/kinds/inlines.h').read_text(encoding='utf-8'),'Item_InitSpawn')]+[extraction.body(source,n) for n in names]
 (out/'constructor_candidate.inc').write_text('\n'.join(b[:b.index('{')].strip()+';' for b in bodies)+'\n'+'\n\n'.join(bodies)+'\n',encoding='utf-8')
 source=(x.baseline.resolve()/'sdk/src/melee/ft/native_fighter_module.c').read_text(encoding='utf-8')
 entries={}
 with (out/'constructor_candidate.inc').open('a',encoding='utf-8') as file:
  for name in ('article_signal_author','signal_article'):
   body=extraction.body(source,name);file.write(body+'\n');entries[name]={'sha256':hashlib.sha256(body.encode()).hexdigest()}
 prov['sources']['candidate:src/melee/ft/native_fighter_module.c']={'sha256':hashlib.sha256((x.baseline.resolve()/'sdk/src/melee/ft/native_fighter_module.c').read_bytes()).hexdigest(),'bodies':entries}
 prov['scope']='Primary scheduler/constructor, rope allocation/signals/accessory/expiry/interaction, ordered Ice-Blizzard-rope-hitlag cleanup; actual resource/ref/lifetime libraries and complete AB54; controlled allocation/model/contact/setup/removal/effect subdependencies'
 prov['sources']['candidate:src/melee/it/native_article.c']={'sha256':hashlib.sha256((x.baseline.resolve()/'sdk/src/melee/it/native_article.c').read_bytes()).hexdigest(),'bodies':{n:{'sha256':hashlib.sha256(b.encode()).hexdigest()} for n,b in zip(names,bodies[1:])}}
 spawn_source=(x.baseline.resolve()/'sdk/src/melee/it/kinds/inlines.h').read_bytes()
 prov['sources']['candidate:src/melee/it/kinds/inlines.h']={'sha256':hashlib.sha256(spawn_source).hexdigest(),'bodies':{'Item_InitSpawn':{'sha256':hashlib.sha256(bodies[0].encode()).hexdigest()}}}
 for relative in ('src/melee/it/native_article_resource.c','src/melee/it/native_article_refs.c','src/melee/ft/native_callback_lifetime.c','src/melee/ft/native_fighter_relations.c'):
  prov['sources']['candidate:'+relative]={'sha256':hashlib.sha256((x.baseline.resolve()/'sdk'/relative).read_bytes()).hexdigest(),'compiled_complete_unit':True}
 (out/'provenance.json').write_text(json.dumps(prov,indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
