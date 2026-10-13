"""Normal production clone-stock construction; read-only asset audit, no game/install."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import zlib
from audit import audit

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def archive(iso,name):
    with iso.open('rb') as f:
        header=f.read(0x440);assert header[:6]==b'GALE01' and header[7]==2
        offset,size=struct.unpack_from('>II',header,0x424);f.seek(offset);fst=f.read(size)
        count=struct.unpack_from('>I',fst,8)[0];names=fst[count*12:]
        for i in range(1,count):
            label,offset,size=struct.unpack_from('>III',fst,i*12)
            if label>>24:continue
            at=label&0xffffff
            if names[at:names.index(0,at)].decode('ascii')==name:
                f.seek(offset);data=f.read(size);assert len(data)==size;return data
    raise ValueError(name)


def portrait(path):
    def chunk(name,data):return struct.pack('>I',len(data))+name+data+struct.pack('>I',zlib.crc32(name+data))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',16,16,8,6,0,0,0))+
        chunk(b'IDAT',zlib.compress((b'\0'+b'\x80\x80\x80\xff'*16)*16))+chunk(b'IEND',b''))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('tool','snapshot','iso','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();tool=a.tool.resolve();snapshot=a.snapshot.resolve();iso=a.iso.resolve();out=a.output.resolve();sdk=snapshot/'sdk'
    if out.exists() or out.drive.upper()!='A:':p.error('Fresh isolated A: output required')
    freeze=json.loads((snapshot/'freeze.json').read_text(encoding='utf-8'))
    for path,expected in freeze.items():assert sha(snapshot/path)==expected,path
    # Actual tool uses the current production registration. Capture that live
    # source provenance separately from the frozen independent import proof.
    inputs={str(x):sha(x) for directory in (ROOT/'src',ROOT/'extern/dolphin/include',ROOT/'tools')
        for x in directory.rglob('*') if x.is_file() and x.suffix in ('.c','.h')}
    inputs.update({str(x):sha(x) for x in HERE.iterdir() if x.is_file()})
    inputs[str(tool)]=sha(tool);inputs[str(snapshot/'compiler/clang.exe')]=sha(snapshot/'compiler/clang.exe')
    out.mkdir(parents=True);retail_dir=out/'retail';retail_dir.mkdir();commands=[]
    def run(command,stage,cwd=ROOT):
        command=list(map(str,command));r=subprocess.run(command,cwd=cwd,capture_output=True,text=True,encoding='utf-8',timeout=300,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n');commands.append({'stage':stage,'command':command,'exit':r.returncode})
        if r.returncode:raise RuntimeError(stage+'\n'+r.stdout+r.stderr)
        return r.stdout
    def cli(stage,*args):return run([tool,'--root',out/'store',*args],stage)
    retail={name:archive(iso,name) for name in ('PlPp.dat','PlNn.dat','PlPpAJ.dat','PlNnAJ.dat','PlPpNr.dat','PlNnNr.dat')}
    for name,data in retail.items():(retail_dir/name).write_bytes(data)
    image=out/'portrait.png';portrait(image)
    members={};files={}
    for member in ('Popo','Nana'):
        directory=out/member.lower();cli(member+'-clone','clone-stock',iso,member,directory,'Owned '+member+' asset proof')
        inspect=cli(member+'-inspect','inspect-project',directory);assert 'Base: '+member in inspect
        files[member]={n:(directory/n).read_bytes() for n in ('fighter.dat','animation.dat','costume.dat')}
        text=cli(member+'-bundle','bundle-project',directory,image,iso)
        ids=set(re.findall(r'\b[0-9a-f]{64}\b',text));assert len(ids)==1,text;package=ids.pop()
        verification=cli(member+'-verify','verify-package',package,iso)
        assert 'registered and released.' in verification,verification
        manifest=dict(line.split('=',1) for line in (directory/'project.txt').read_text(encoding='utf-8').splitlines())
        assert manifest['base']==member and manifest['code']==member.lower()+'_clone'
        members[member]={'package_id':package,'manifest':manifest,'verification':verification.strip(),
            'files':{x.name:sha(x) for x in directory.iterdir() if x.is_file()}}
    census=audit(retail,files['Popo'],files['Nana'])
    (out/'serialized_census.json').write_text(json.dumps(census,indent=2)+'\n',encoding='utf-8',newline='\n')
    # Audit imports the *produced* bytes. No copy/materialize function is linked.
    units=[HERE/'import.c']
    units += [sdk/'src/melee/ft'/n for n in ('ftcharacter_assets.c','ftdat.c','ftanimation_bank.c','ftaction_data.c')]
    units += [sdk/'src/melee/it'/n for n in ('itarticle_data.c','itcommand_data.c','article_model_layers.c')]
    units += [sdk/'src/melee/lb/lbanim_dat.c']
    units += [sdk/'src/sysdolphin/baselib'/n for n in ('dat.c','dat_import.c','dat_model.c','dat_geometry.c','dat_material.c','dat_animation.c','dat_scene.c','dat_texture.c')]
    compiler=snapshot/'compiler/clang.exe'
    cmd=[compiler,'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),*units,'-Wl,/OPT:REF','-o',out/'import.exe']
    batch=out/'compile.cmd';batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(list(map(str,cmd)))+'\n',encoding='utf-8',newline='\n')
    run(['cmd','/d','/c',batch],'compile-import');actual=json.loads(run([out/'import.exe',retail_dir,out/'popo',out/'nana'],'actual-import'))
    for path,expected in inputs.items():assert sha(Path(path))==expected,'Input changed: '+path
    for path,expected in freeze.items():assert sha(snapshot/path)==expected,'Frozen input changed: '+path
    for member in ('Popo','Nana'):
        for name,data in files[member].items():assert (out/member.lower()/name).read_bytes()==data
    receipt={'status':'PASS actual normal clone/bundle/verify construction and independent produced-byte/import audit; uninstalled',
        'members':members,'serialized_audit':census,'native_import_audit':actual,'commands':commands,'inputs':inputs,
        'snapshot_manifest_sha256':sha(snapshot/'freeze.json'),'retail':{n:hashlib.sha256(d).hexdigest() for n,d in retail.items()},
        'limits':['Raw bases retain stock gameplay inheritance: no independent authored fighter activation or family transfer acceptance.',
            'Actual bundle verification registers/releases each ordinary single-member package; no game or install.',
            'Snapshot importer is a separate captured asset-layout proof, not the current tool ABI; production tool/source hashes are separate.',
            'Two unresolved import visits preserve the original unreachable Nana animation placeholder and primary model placeholder. The active three-article graphs are physically local.',
            'Demo descriptors remain original metadata; separately loaded demo payloads and rendering remain unaccepted.',
            'Earlier frozen71 retarget fixture covers full/partial scalar SRT. This pipeline does not repeat matrix/IK/render/gameplay checks.']}
    receipt['outputs']={str(x.relative_to(out)):sha(x) for x in out.rglob('*') if x.is_file()}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'members':{n:m['package_id'] for n,m in members.items()},'native_import':actual,'actions':census['filled_actions'],'bank_symbols':census['bank_symbols']}))


if __name__=='__main__':main()
