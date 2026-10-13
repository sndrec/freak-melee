"""Join stock callback/retail facts with the actual generated pair programs."""
import argparse,hashlib,json,re
from pathlib import Path
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser()
 for n in ('root','evidence','output'):p.add_argument('--'+n,type=Path,required=True)
 x=p.parse_args();root=x.root.resolve();stock=json.loads((HERE/'stock_census.json').read_text(encoding='utf-8'))
 result={'status':'Unregistered full motion composition; pending owned pair/load/common/CPU foundation','forms':{},'borrowed_source_hashes':{},'stock_source_hashes':stock['sources']}
 for family,form in [('ftPopo','popo'),('ftNana','nana')]:
  staged=x.evidence/(form+'-composed');prep=json.loads((staged/'preparation.json').read_text(encoding='utf-8'))
  text=(staged/'owned.fms').read_text(encoding='utf-8');generated=(x.evidence/form/'generated.c').read_text(encoding='utf-8')
  hooks=re.findall(r'^    on (\w+):',text.split('\narticle ',1)[0],re.M)
  if len(hooks)!=len(set(hooks)):raise ValueError('Duplicate fighter handlers')
  phases=[]
  for row in stock['motion_tables'][family]:
   for phase,callback in row['phases'].items():
    suffix={'animation':'animation','input':'interrupt','physics':'physics','collision':'collision'}[phase]
    handler='state_%d_%s'%(row['id'],suffix)
    if (callback!='NULL')!=(handler in hooks):raise ValueError('Stock phase mismatch '+handler)
    phases.append({'motion':row['id'],'phase':phase,'stock':callback,'script':handler if handler in hooks else None})
  meta=re.search(r'generated.state_bytes = script_state_base \+ (\d+)',generated)
  result['forms'][form]={'parameters':len(prep['parameters']),'fighter_state_fields':len(re.findall(r'^state ',text,re.M)),'fighter_state_bytes':int(meta[1]) if meta else None,
   'functions':len(re.findall(r'^(?:export )?func ',text,re.M)),'exports':len(re.findall(r'^export func ',text,re.M)),
   'article_count':len(re.findall(r'^article ',text,re.M)),'nonnull_stock_motion_phases':sum(q['stock']!='NULL' for q in phases),'phases':phases,'hooks':hooks,
   'stock_callbacks':{k:v['Popo' if form=='popo' else 'Nana'] for k,v in stock['kind_tables'].items()},
   'DAT_sha256':prep['DAT_sha256'],'generated_sha256':sha(x.evidence/form/'generated.c'),'owned_sha256':sha(staged/'owned.fms')}
 for namespace in ('ice_climbers_composition','ice_climbers_linked','ice_climbers_rope'):
  for file in (HERE.parent/namespace).iterdir():
   if file.is_file():result['borrowed_source_hashes'][file.relative_to(root).as_posix()]=sha(file)
 metadata={}
 for name in ('Popo','Nana'):
  retail=stock['retail'][name]
  metadata[name]={'stock_internal_kind':retail['internal_kind'],'fighter_root':'ftData'+name,'data_file':'Pl'+('Pp' if name=='Popo' else 'Nn')+'.dat',
   'animation_file':'Pl'+('Pp' if name=='Popo' else 'Nn')+'AJ.dat','motion_first':341,'motion_count':26,'animation_count':321,'demo_count':14,'effect_bank':14,
   'sound_mute_channels':68,'asset_layout':{'article_mask':7,'joint_graph_mask':0,'item_format':'FtCharacterItems_ClimberString','raw_byte_spans':[]},
   'article_assets':[{'item':k,'slot':i,'kind_offset':-1} for i,k in enumerate(('It_Kind_IceClimber_Ice','It_Kind_IceClimber_Blizzard','It_Kind_IceClimber_GumStrings'))],
   'runtime_parts':retail['runtime_parts'],'bones':retail['bones'],'omissions':retail['omissions'],'costumes':retail['costumes'],
   'data_sha256':retail['data_sha256'],'article_offsets':retail['article_offsets'],'rope_model_roots':retail['string_joint_roots'],
   'demo_names':['ftDemoResultMotionFile'+name,'ftDemoIntroMotionFile'+name,'ftDemoEndingMotionFile'+name,'ftDemoViWaitMotionFilePopo']}
 result['owned_metadata_proposal']=metadata
 x.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print([(form,d['parameters'],d['fighter_state_fields'],d['fighter_state_bytes'],d['nonnull_stock_motion_phases']) for form,d in result['forms'].items()])
if __name__=='__main__':main()
