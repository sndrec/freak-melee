"""Census actual retail symbols and preserve exact relocation-chain evidence."""
import argparse,struct,json,hashlib,importlib.util
from pathlib import Path
root=Path(__file__).resolve().parents[3]
parser=argparse.ArgumentParser(description='Read-only Samus grapple external symbol census; no graph import/build/runtime.')
parser.add_argument('--iso',required=True,type=Path)
parser.add_argument('--inventory',required=True,type=Path)
parser.add_argument('--output',required=True,type=Path)
args=parser.parse_args();iso=args.iso;out=args.output;out.mkdir(parents=True,exist_ok=False)
inv=json.loads(args.inventory.read_text(encoding='utf-8'))
names={x['reference']['symbol'] for x in inv['article_animation_pointer_arrays'] if x['reference']['kind']=='external'}
with iso.open('rb') as f:
 header=f.read(0x440);assert header[:6]==b'GALE01' and header[7]==2;at,size=struct.unpack_from('>II',header,0x424);f.seek(at);fst=f.read(size);n=struct.unpack_from('>I',fst,8)[0];strings=fst[n*12:];files=[];matches=[];owned=[]
 for i in range(1,n):
  word,at,size=struct.unpack_from('>III',fst,i*12)
  if word>>24:continue
  start=word&0xffffff;name=strings[start:strings.index(0,start)].decode('ascii')
  if not name.lower().endswith('.dat'):continue
  f.seek(at);h=f.read(32)
  if len(h)!=32:continue
  length,data,relocs,roots,externs=struct.unpack_from('>5I',h)
  table=32+data+relocs*4;symbols=table+8*(roots+externs)
  if length!=size or symbols>=size:continue
  f.seek(at+table);tail=f.read(size-table);records=[]
  for j in range(roots+externs):
   target,symbol=struct.unpack_from('>II',tail,j*8);s=tail[8*(roots+externs)+symbol:];s=s[:s.index(0)].decode('ascii');records.append((s,target,j<roots))
   if s in names:matches.append({'archive':name,'symbol':s,'kind':'root' if j<roots else 'external','target_or_chain_head':target})
  files.append(name)
  if name.startswith('PlSs') or name=='PlCo.dat':
   f.seek(at);blob=f.read(size);owned.append({'archive':name,'sha256':hashlib.sha256(blob).hexdigest(),'roots':roots,'externs':externs,'matched':[r for r in records if r[0] in names]})
   if name=='PlSs.dat':(out/name).write_bytes(blob)
# Re-use the committed archive reader to follow exact external chains, never graph-dereference chain words.
spec=importlib.util.spec_from_file_location('samus_audit',root/'tests/native/samus_tether_composition/audit.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);dat=mod.Dat(out/'PlSs.dat')
attr=inv['grapple_attribute_source'];refs=[];locals=[]
for e in inv['article_animation_pointer_arrays']:
 offset=e['attribute_offset'];ref=dat.reference(attr+offset)
 assert ref==e['reference'], (offset,ref,e['reference'])
 if ref['kind']=='local':
  first=dat.reference(ref['target']);assert first['kind']=='local' and first['target']==e['first_graph']
  locals.append({'attribute_offset':offset,'array':ref['target'],'first_graph':first['target']})
 if ref['kind']=='external':
  refs.append({'attribute_offset':offset,'pointer_slot':attr+offset,'reference':ref,'slots':[s for s,n in dat.externals.items() if n==ref['symbol']]})
inputs={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in ['tests/native/samus_tether_composition/audit.py','src/melee/lb/lbarchive.c','src/sysdolphin/baselib/archive.c','src/sysdolphin/baselib/dat.c','src/sysdolphin/baselib/dat_import.c','src/melee/it/itarticle_data.c','src/melee/ft/ftdata.c','src/melee/ft/ftcharacter_assets.c','src/melee/it/kinds/itsamusgrapple.c','src/melee/ft/kinds/ftSamus/types.h']}
assert len(names)==7 and len(matches)==7 and all(m['archive']=='PlSs.dat' and m['kind']=='external' for m in matches)
assert len(locals)==8 and len(refs)==7
inputs['input_inventory']=hashlib.sha256(args.inventory.read_bytes()).hexdigest()
result={'inventory_snapshot_sha256':inv['snapshot_sha256'],'disc':str(iso),'disc_header_sha256':hashlib.sha256(header).hexdigest(),'fst_sha256':hashlib.sha256(fst).hexdigest(),'dat_archives_scanned':len(files),'scan_scope':'All disc .dat files with exact length/structurally bounded DAT header; public/external symbol tables only. No external word decoded as graph.','matches':matches,'owned_candidates':owned,'external_components':refs,'local_components':locals,'source_sha256':inputs}
(out/'audit.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(f'{len(files)} DAT archives; {len(matches)} matching records; {len(refs)} external components')
