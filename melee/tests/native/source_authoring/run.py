"""Actual source CLI + real retail clone transaction proof, no game launch."""
import argparse,hashlib,json,subprocess,re,struct,zlib
from pathlib import Path
def main():
 p=argparse.ArgumentParser();p.add_argument('--tool',type=Path,required=True);p.add_argument('--iso',type=Path,required=True);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir();draft=out/'draft';cache=out/'cache';commands=[];checks=0;cases=0;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
 def invoke(label,args,expected=0):
  nonlocal checks
  cmd=[str(a.tool.resolve())]+list(map(str,args));r=subprocess.run(cmd,cwd=a.tool.resolve().parent,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(label+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':label,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n');checks+=1
  if (r.returncode==0)!=(expected==0):raise RuntimeError(label+' unexpected status: '+r.stdout+r.stderr)
  return r
 def files():return{p.name:sha(p) for p in draft.iterdir() if p.is_file()}
 invoke('clone',['clone-stock',a.iso.resolve(),'Mario',draft,'Source transaction']);checks+=1
 if not(draft/'assets.fma').is_file() or(draft/'module.dll').exists() or(draft/'behavior.generated.c').exists():raise RuntimeError('source clone schema missing or native code payload present')
 original=files();invoke('check',['source-check',draft,a.source.resolve()]);checks+=1
 if files()!=original:raise RuntimeError('read-only check changed draft')
 cases+=1;invoke('build',['script-build',draft,a.source.resolve()]);accepted=files();checks+=1
 if sha(draft/'behavior.fms')!=sha(a.source) or accepted['assets.fma']!=original['assets.fma']:raise RuntimeError('source commit or ownedfacts changed')
 cases+=1;bad=out/'bad.fms';bad.write_text('fighter:\n    behavior independent\naction Invalid:\n    on load:\n        nonexistent_api()\n')
 invoke('failed-build',['script-build',draft,bad],1);checks+=1
 if files()!=accepted:raise RuntimeError('failed source build modified accepted draft')
 cases+=1;unsupported=out/'workshop.fms';unsupported.write_text('fighter:\n    behavior independent\nitem Snack damage 1 angle 1 lifetime 1:\n    on tick:\n        pass\n')
 invoke('unsupported-resource',['source-check',draft,unsupported],1);checks+=1
 if files()!=accepted:raise RuntimeError('unsupported resource check modified draft')
 cases+=1
 manifest=(draft/'project.txt').read_text();(draft/'project.txt').write_bytes(manifest.replace('template=mario','template=no_donor_lookup').encode());unrelated=files();invoke('owned-profile',['source-check',draft,a.source.resolve()]);checks+=1
 if files()!=unrelated:raise RuntimeError('source validation silently recopied authoring profile')
 cases+=1
 def chunk(name,data):return struct.pack('>I',len(data))+name+data+struct.pack('>I',zlib.crc32(name+data)&0xffffffff)
 png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',32,32,8,6,0,0,0))+chunk(b'IDAT',zlib.compress((b'\0'+bytes([70,120,210,255])*32)*32))+chunk(b'IEND',b'');portrait=out/'portrait.png';portrait.write_bytes(png)
 result=invoke('package',['--root',cache,'package-project',draft,portrait]);matched=re.search(r'Cached source fighter ([0-9a-f]{64})',result.stdout)
 if not matched:raise RuntimeError('missing package identity')
 identifier=matched.group(1);invoke('verify',['--root',cache,'verify-package',identifier]);checks+=1
 packets=list(cache.rglob('*.fmc'))
 if not packets or all(p.read_bytes()[:4]!=b'FMSC' for p in packets):raise RuntimeError('ordinary package does not use source schema')
 cases+=1
 receipt={'status':'PASS','cases':cases,'checks':checks,'commands':commands,'tool_sha256':sha(a.tool),'source_sha256':sha(a.source),'draft_sha256':files(),'package_id':identifier,'scope':'Actual retail clone, ownedfacts/sourcecandidate, readonlycheck+atomicbuild+sourcepacket; no game/globalactor or render/parity acceptance'}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'receipt':str(out/'receipt.json'),'cases':cases,'checks':checks}))
if __name__=='__main__':main()
