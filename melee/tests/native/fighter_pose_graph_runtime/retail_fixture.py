"""Extract immutable retail DAT inputs and complete real part setup bodies."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
from fixture import body

def archive(iso,name):
    with iso.open('rb') as f:
        header=f.read(0x440);assert header[:6]==b'GALE01' and header[7]==2
        offset,size=struct.unpack_from('>II',header,0x424);f.seek(offset);fst=f.read(size)
        count=struct.unpack_from('>I',fst,8)[0];names=fst[count*12:]
        for i in range(1,count):
            n,offset,size=struct.unpack_from('>III',fst,i*12)
            if n>>24:continue
            at=n&0xffffff
            if names[at:names.index(0,at)].decode('ascii')==name:
                f.seek(offset);value=f.read(size);assert len(value)==size;return value
    raise ValueError(name)

def main():
    p=argparse.ArgumentParser()
    for k in ('root','iso','output'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);inputs={}
    for name in ('PlSk.dat','PlSkAJ.dat','PlSkNr.dat','PlSkRe.dat','PlSkBu.dat','PlSkGr.dat','PlSkWh.dat','PlCo.dat'):
        value=archive(a.iso,name);(a.output/name).write_bytes(value);inputs[name]=hashlib.sha256(value).hexdigest()
    # Retail PlCo own mapping, not a guessed semantic/physical identity.
    blob=(a.output/'PlCo.dat').read_bytes();size,data_size,relocs,roots,exts=struct.unpack_from('>5I',blob)
    table=32+data_size+relocs*4;strings=table+8*(roots+exts)
    root=None
    for i in range(roots):
        at,n=struct.unpack_from('>II',blob,table+8*i);start=strings+n
        if blob[start:blob.index(0,start)]==b'ftLoadCommonData':root=at
    assert root is not None
    def w(at):return struct.unpack_from('>I',blob,32+at)[0]
    descriptor=w(w(root+16)+7*4);inverse,semantic,count=(w(descriptor+4*i) for i in range(3))
    omission=w(w(root+20)+7*4)
    assert count==58 and (not omission or w(omission+4)==0)
    mapping=blob[32+semantic:32+semantic+54];assert len(mapping)==54
    (a.output/'rig.inc').write_text('static u8 semantic_rig[54] = {'+','.join(str(x) for x in mapping)+'};\n',encoding='utf-8')
    sources={};parts=[]
    for rel,names in {'src/melee/ft/ftparts.c':['ftParts_80074E58'],'src/melee/ft/ftparts_bind.c':['ftParts_80074194','ftParts_SetupParts','ftParts_8007462C']}.items():
        f=a.root/rel;text=f.read_text(encoding='utf-8');sources[rel]=hashlib.sha256(f.read_bytes()).hexdigest();parts.extend(body(text,n) for n in names)
    (a.output/'retail_setup.inc').write_text('\n\n'.join(parts)+'\n',encoding='utf-8')
    (a.output/'retail_inputs.json').write_text(json.dumps({'iso':str(a.iso),'archives':inputs,'sources':sources,'runtime_parts':count,'semantic_map':list(mapping),'omission_count':0},indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
