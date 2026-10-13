"""Owned Game & Watch load callback versus full original C; console only."""
import argparse,hashlib,json,re,shutil,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def body(text,name):
 m=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text)
 if not m:raise ValueError('missing actual definition '+name)
 depth,end=1,m.end()
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[m.start():end]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--snapshot',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
 ap.add_argument('--iso',type=Path,default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso');a=ap.parse_args();snap=a.snapshot.resolve();out=a.output.resolve();sdk=snap/'sdk'
 if out.exists() or snap in out.parents or HERE in out.parents:ap.error('fresh output outside input namespaces required')
 frozen=json.loads((snap/'freeze.json').read_text())
 def verify():
  for name,digest in frozen.items():
   if sha(snap/name)!=digest:raise RuntimeError('snapshot changed '+name)
 verify();out.mkdir(parents=True);inputs={};commands=[]
 def run(stage,cmd):
  cmd=list(map(str,cmd));p=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120);(out/(stage+'.log')).write_text(p.stdout+p.stderr,encoding='utf-8',newline='\n');commands.append({'stage':stage,'command':cmd,'exit':p.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',newline='\n')
  if p.returncode:raise RuntimeError(stage+' failed: '+p.stdout+p.stderr)
  return p.stdout
 # No fabricated base metadata/normal clone claim: GameWatch clone-stock is not
 # yet available. Actual special-attribute bytes provide the editable values.
 sys.path.insert(0,str(sdk/'tools'));from extract_native_animation import extract_clip
 dat,root=extract_clip(a.iso,'PlGw.dat','ftDataGamewatch');(out/'PlGw.dat').write_bytes(dat)
 attrs=struct.unpack_from('>I',dat,32+root+4)[0];values=[]
 text=(HERE/'candidate.fms').read_text()
 for kind,name,offset in re.findall(r'^attribute (float|byte) (\w+) (\d+)$',text,re.M):
  at=32+attrs+int(offset);value=struct.unpack_from('>f',dat,at)[0] if kind=='float' else dat[at]
  values.append(('float' if kind=='float' else 'int',name,value))
 # This fixture's raw-byte parameter import boundary removes source DAT recipes
 # just as normal import-parameters does; the compiler reads only owned FMP fields.
 (out/'candidate.fms').write_text(re.sub(r'^attribute .*\n','',text,flags=re.M),newline='\n')
 (out/'fighter_parameters.fmp').write_text(''.join(f'{kind} {name} = {value}\n' for kind,name,value in values),newline='\n')
 generated=out/'behavior.generated.h';run('generate',[snap/'bin/native_content_tool.exe','script-c',out/'candidate.fms','gamewatch_load_probe',generated])
 schema=re.search(r'schema != (0x[0-9a-fA-F]+)ULL',generated.read_text())[1]
 bits=[struct.unpack('<I',struct.pack('<f',v))[0] if k=='float' else v for k,n,v in values]
 (out/'parameters.h').write_text('#define FIXTURE_PARAMETER_COUNT '+str(len(bits))+'\n#define FIXTURE_PARAMETER_SCHEMA '+schema+'ULL\nstatic uint32_t fixture_parameter_bits[]={'+','.join(map(str,bits))+'};\nstatic const char* fixture_parameter_names[]={'+','.join(json.dumps(n) for k,n,v in values)+'};\n',newline='\n')
 for rel,name,dest in [('src/melee/ft/kinds/ftGameWatch/ftgamewatch.c','ftGw_Init_OnLoad','original_load.h'),('src/melee/ft/ftmaterial.c','ftMaterial_800BFB4C','material_body.h')]:
  p=sdk/rel;inputs[str(p)]=sha(p);code=body(p.read_text(),name)
  if name=='ftMaterial_800BFB4C':
   joint=sdk/'src/sysdolphin/baselib/jobj.c';inputs[str(joint)]=sha(joint);code=body(joint.read_text(),'HSD_JObjGetDObj')+'\n'+code
  (out/dest).write_text(code+'\n',newline='\n')
 clang=snap/'compiler/clang.exe';exe=out/'load_compare.exe'
 flags=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out)]
 run('compile',flags+[HERE/'compare.c','-o',exe]);result=json.loads(run('run',[exe]).strip());verify()
 inputs.update({str(p):sha(p) for p in [HERE/'candidate.fms',HERE/'compare.c',HERE/'run.py',out/'candidate.fms',generated,out/'fighter_parameters.fmp',out/'parameters.h',out/'original_load.h',out/'material_body.h']})
 receipt={'status':'PASS source/generated load policy only; unregistered','result':result,'snapshot_sha256':sha(snap/'freeze.json'),'dat_sha256':sha(out/'PlGw.dat'),'fighter_root':root,'special_attributes_source':attrs,'inputs':inputs,'commands':commands,'boundaries':['Complete actual original OnLoad and PUSH_ATTRS copy execute; attribute inputs decoded directly from original DAT into typed stock and editable owned parameters.','Actual complete production diffuse traversal executes against bounded controlled HSD graph/material nodes; no GX or allocator.','Visibility install Host is a controlled publication boundary; production active-package/load-window validation is separate engine evidence.','Original ten article registrations are checked for actual source order; immutable module resource binding replaces these outside authored callback.','Synthetic inherited fixture base, direct generated callback invocation; no normal clone/DLL ownership/full fighter activation/gameplay claim.'],'intentional_extensions':['Dead x2223_b1 write omitted; common-initialized x2222_b6 is preserved.','Invalid custom costume indexes select explicit palette0; defined original source domain is0..3.']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(result))
if __name__=='__main__':main()
