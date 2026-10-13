"""Actual resource authoring CLI transaction, private A: console only."""
import argparse,ctypes,hashlib,json,shutil,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]

def main():
 p=argparse.ArgumentParser();p.add_argument('--tool',type=Path,required=True);p.add_argument('--iso',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
 out.mkdir(parents=True);private=out/'tool';private.mkdir();source_tool=a.tool.resolve();tool=private/source_tool.name
 shutil.copy2(source_tool,tool)
 for dll in source_tool.parent.glob('*.dll'):shutil.copy2(dll,private/dll.name)
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();icons=ROOT/'examples/rook_workshop/items';hashes={str(p):sha(p) for p in [source_tool,tool,Path(__file__).resolve(),icons/'snack0.png',icons/'snack1.png']}
 commands=[];checks=0;cases=0;draft=out/'draft';bundle=draft/'resources.fmr'
 def check(value,message):
  nonlocal checks
  checks+=1
  if not value:raise AssertionError(message)
 def invoke(label,words,success=True):
  cmd=[str(tool),'--root',str(out/'cache')]+list(map(str,words));r=subprocess.run(cmd,cwd=private,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(label+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':label,'command':cmd,'exit':r.returncode,'expected_success':success});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  check(r.returncode==0 if success else r.returncode==1,label+' unexpected exit:\n'+r.stdout+r.stderr);return r.stdout
 def entries():
  data=bundle.read_bytes();check(data[:4]==b'FMRS','magic');count,total=struct.unpack_from('<II',data,4);check(total==len(data),'total');rows={};at=12+count*140;last=''
  for i in range(count):
   row=data[12+i*140:12+(i+1)*140];name=row[:128].split(b'\0',1)[0].decode();kind,offset,size=struct.unpack_from('<III',row,128)
   check(last<name and offset==at and size>0,'canonical order/spans');rows[name]=(kind,data[offset:offset+size]);at+=size;last=name
  check(at==len(data),'tight payload');return rows
 def reject(label,words):
  before=bundle.read_bytes();invoke(label,words,False);check(bundle.read_bytes()==before,'rejected operation changed bundle');check(not list(draft.glob('resources.fmr.*.tmp')),'owned temp leaked')
 facts=out/'rook.fma';again=out/'rook-again.fma';invoke('export-rook',['export-fighter-facts','rook',facts]);invoke('export-rook-again',['export-fighter-facts','rook',again]);check(facts.read_bytes()[:4]==b'FMAS' and facts.read_bytes()==again.read_bytes(),'facts export is not canonical/deterministic')
 accepted_facts=facts.read_bytes();invoke('export-unknown',['export-fighter-facts','no_such_template',facts],False);check(facts.read_bytes()==accepted_facts,'rejected facts export overwrote accepted output');check(not list(out.glob('rook.fma.*.tmp')),'facts temp leaked');cases+=1
 invoke('clone',['clone-stock',a.iso.resolve(),'Mario',draft,'Resource authoring']);check(not bundle.exists(),'new draft unexpectedly has resource bundle')
 invoke('png-first',['project-resource-import',draft,'items/snack0.png','png',icons/'snack0.png']);check(entries()['items/snack0.png']==(2,(icons/'snack0.png').read_bytes()),'PNG bytes changed');cases+=1
 opaque=out/'opaque.bin';opaque.write_bytes(b'\0\xff\x11\0binary');invoke('blob',['project-resource-import',draft,'data/opaque.bin','blob',opaque]);before=entries()
 invoke('png-second',['project-resource-import',draft,'items/snack1.png','png',icons/'snack1.png']);after=entries();check(all(after[k]==v for k,v in before.items()),'unrelated entry changed');cases+=1
 invoke('replace',['project-resource-import',draft,'items/snack0.png','png',icons/'snack1.png']);after=entries();check(after['items/snack0.png']==after['items/snack1.png'],'exact replacement failed');check(after['data/opaque.bin']==before['data/opaque.bin'],'blob changed');cases+=1
 for label,name in [('parent-path','../outside'),('case-alias','ITEMS/snack0.png'),('device','items/CON.png')]:reject(label,['project-resource-import',draft,name,'png',icons/'snack0.png'])
 reject('unknown-kind',['project-resource-import',draft,'data/wrong','jpeg',opaque]);reject('missing-file',['project-resource-import',draft,'data/missing','blob',out/'missing.bin']);cases+=1
 bad=out/'bad.png';bad.write_bytes((icons/'snack0.png').read_bytes()[:-1]+b'\0');reject('bad-png',['project-resource-import',draft,'items/bad.png','png',bad])
 huge=out/'huge.bin';huge.write_bytes(b'\0'*(4*1024*1024+1));reject('oversize',['project-resource-import',draft,'data/huge','blob',huge]);cases+=1
 accepted=bundle.read_bytes();bundle.write_bytes(b'bad-header');reject('corrupt-existing',['project-resource-import',draft,'data/new','blob',opaque]);bundle.write_bytes(accepted);cases+=1
 # Disallow replacement while the existing bundle is open; production must
 # remove only its successfully-created private temporary and preserve bytes.
 kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p];kernel.CreateFileW.restype=ctypes.c_void_p
 kernel.CloseHandle.argtypes=[ctypes.c_void_p];kernel.CloseHandle.restype=ctypes.c_int
 handle=kernel.CreateFileW(str(bundle),0x80000000,1,None,3,0x80,None);check(handle!=ctypes.c_void_p(-1).value,'could not lock private bundle')
 try:reject('locked-destination',['project-resource-import',draft,'data/locked','blob',opaque])
 finally:check(kernel.CloseHandle(handle)!=0,'could not close private lock')
 cases+=1
 # Independently extract the actual original ItCo.dat and its Capsule Article
 # offset, rather than obtaining expected values from the tool/writer.
 with a.iso.open('rb') as iso:
  iso.seek(0x424);fst_at,fst_size=struct.unpack('>II',iso.read(8));iso.seek(fst_at);fst=iso.read(fst_size);count=struct.unpack_from('>I',fst,8)[0];strings=fst[count*12:];disc=None
  for i in range(1,count):
   tag,offset,size=struct.unpack_from('>III',fst,i*12);n=tag&0xffffff;name=strings[n:strings.find(b'\0',n)]
   if name==b'ItCo.dat':iso.seek(offset);disc=iso.read(size);break
 check(disc is not None,'ItCo.dat not in actual ISO');dat=out/'ItCo.dat';dat.write_bytes(disc)
 data_size,relocs,roots,externs=struct.unpack_from('>IIII',disc,4);root_at=32+data_size+relocs*4;strings_at=root_at+(roots+externs)*8;public=None
 for i in range(roots):
  offset,name=struct.unpack_from('>II',disc,root_at+i*8);n=strings_at+name
  if disc[n:disc.find(b'\0',n)]==b'itPublicData':public=offset;break
 check(public is not None,'itPublicData not found');table=struct.unpack_from('>I',disc,32+public+4)[0];article=struct.unpack_from('>I',disc,32+table)[0]
 prior=entries();invoke('stock-blueprint',['project-resource-stock-item',draft,'blueprints/default.item',a.iso.resolve(),0]);built=entries();check(all(built[k]==v for k,v in prior.items()),'stock import changed other entries')
 envelope=built['blueprints/default.item'];check(envelope[0]==1 and envelope[1][:4]==b'FMIB','typed blueprint envelope');check(struct.unpack_from('<III',envelope[1],4)==(len(disc),article,0),'blueprint descriptor identity');check(envelope[1][16:]==disc,'stock Item DAT was not copied exactly');cases+=1
 accepted=bundle.read_bytes();invoke('explicit-blueprint',['project-resource-item',draft,'blueprints/default.item',dat,hex(article)]);check(bundle.read_bytes()==accepted,'explicit DAT writer differs from stock authoring result');cases+=1
 reject('bad-offset',['project-resource-item',draft,'blueprints/invalid.item',dat,'0xffffffff']);reject('negative-offset',['project-resource-item',draft,'blueprints/invalid.item',dat,-1]);reject('unsupported-kind',['project-resource-stock-item',draft,'blueprints/invalid.item',a.iso.resolve(),43]);cases+=1
 check(all(sha(Path(path))==value for path,value in hashes.items()),'tool/script/PNG inputs changed')
 receipt={'status':'PASS','checks':checks,'cases':cases,'commands':commands,'input_sha256':hashes,'bundle_sha256':sha(bundle),'original_dat_bytes':len(disc),'article_offset':article,'blueprint_bytes':len(envelope[1]),'scope':'Actual authoring CLI +canonical codec/FMIB writer and ISO common-table selection, atomic exact replacement/private lock rejection; blueprint full native graph import belongs to parent candidate fixture; no game/global build/user install'}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'status':'PASS source resource tool','checks':checks,'cases':cases,'receipt':str(out/'receipt.json')}))
if __name__=='__main__':main()
