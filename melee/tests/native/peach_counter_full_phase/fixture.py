"""Extract whole original counter/Toad/Spore files and bounded shared bodies."""
import argparse,hashlib,json,re,struct
from pathlib import Path

def body(text,name):
    m=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*(?://[^\n]*\n\s*)?\{',text)
    if not m:raise ValueError(name)
    end=m.end();depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[m.start():end]

def main():
    p=argparse.ArgumentParser()
    for n in ('root','output','generated','parameters','dat'):p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();r=a.root;o=a.output;sources={};blocks=[]
    for path in ['src/melee/ft/kinds/ftPeach/ftpeachspecialn.c','src/melee/it/kinds/itpeachtoad.c','src/melee/it/kinds/itpeachtoadspore.c']:
        text=(r/path).read_text();blocks.append(re.sub(r'(?m)^#include[^\n]*\n','',text));sources[path]={'sha256':hashlib.sha256((r/path).read_bytes()).hexdigest(),'scope':'complete file without include directives'}
    groups={
      'src/melee/it/native_article.c':['nativeArticle_Creator','article_logic','nativeArticle_Create'],
      'src/melee/ft/native_fighter_module.c':['article_signal_author','signal_article'],
      'src/melee/ft/ftmovement.c':['ftCommon_CalcSelfAccel_Deaccel','ftCommon_Fall','ftCommon_FallBasic'],
      'src/melee/ft/ftcoll.c':['ftColl_8007AEE0','ftColl_8007B1B8'],
      'src/melee/it/it_26B1.c':['it_8026B3A8','it_8026BB20','it_8026BB44','it_8026B724','it_8026B73C'],
      'src/melee/it/it_2725.c':['it_80272A18','it_80272A3C','HSD_JObjSetScale_2','it_80274594','it_80274574','it_80275158'],
    }
    for path,names in groups.items():
        text=(r/path).read_text();sources[path]={'sha256':hashlib.sha256((r/path).read_bytes()).hexdigest(),'bodies':{}}
        for name in names:
            b=body(text,name);blocks.append(b);sources[path]['bodies'][name]={'line':text[:text.index(b)].count('\n')+1,'sha256':hashlib.sha256(b.encode()).hexdigest()}
    (o/'actual.inc').write_text('\n\n'.join(blocks)+'\n')
    table_path='src/melee/ft/kinds/ftPeach/ftpeach.c';text=(r/table_path).read_text();table=text[text.index('MotionState ftPe_Init_MotionStateTable'):];table=table[:table.index('};')]
    rows=re.findall(r'\{\s*// ([^\n]+)\n(.*?)\n    \},',table,re.S);assert len(rows)==30
    selected=[]
    for comment,row in rows:
        motion=int(comment.split('=')[-1])
        if 365<=motion<=368:selected.append('{'+row+'\n}')
    (o/'retail_slots.inc').write_text('static const MotionState retail_slots[]={\n'+',\n'.join(selected)+'\n};\n')
    sources[table_path]={'sha256':hashlib.sha256((r/table_path).read_bytes()).hexdigest(),'scope':'four exact retail motion rows365..368'}
    path='src/melee/it/it_3F2F.c';text=(r/path).read_text();logic=[]
    for label,symbol in [('Peach toad','toad'),('Peach toad spore','spore')]:
        m=re.search(r'\{\s*// '+label+r'\n(.*?)\n    \},',text,re.S|re.I)
        if not m:raise ValueError('Logic label: '+label)
        logic.append('static const ItemLogicTable retail_'+symbol+'_logic={'+m[1]+'\n};')
    (o/'retail_logic.inc').write_text('\n'.join(logic)+'\n')
    sources[path]={'sha256':hashlib.sha256((r/path).read_bytes()).hexdigest(),'scope':'two complete Toad/Spore logic rows'}
    values=[];params=[]
    for line in a.parameters.read_text().splitlines():
        typ,name,eq,v=line.split();assert eq=='=';params.append({'type':typ,'name':name,'value':v});values.append(struct.unpack('<I',struct.pack('<f',float(v)))[0] if typ=='float' else int(v,0)&0xffffffff)
    gen=a.generated.read_text();count,schema=re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL',gen).groups();assert int(count)==len(values)
    (o/'parameters.inc').write_text('static const uint32_t parameter_bits[]={'+','.join('UINT32_C(%d)'%v for v in values)+'};\nstatic const uint64_t parameter_schema='+schema+'ULL;\n')
    with (o/'parameters.inc').open('a') as f:
        f.write('static const NativeFighterParameter fixture_parameters[]={\n'+',\n'.join('{"%s",NativeFighterParameter_%s,UINT32_C(%d)}'%(p['name'],p['type'].title(),v) for p,v in zip(params,values))+'\n};\n')
    hosts=sorted(set(re.findall(r'host->(\w+) == 0',gen)))
    (o/'hosts.inc').write_text('\n'.join('host.'+h+' = (__typeof__(host.'+h+'))unexpected_boundary;' for h in hosts)+'\n')
    source=Path(__file__).with_name('candidate.fms').read_text();fields=re.findall(r'(?m)^state (bool|int|float) (\w+)$',source)
    assert len(fields)==15 and len({n for _,n in fields})==15
    assert 'behavior independent' not in source
    # Independent big-endian DAT reads verify every imported declaration.
    raw=a.dat.read_bytes();data_size,relocs,public,external=struct.unpack_from('>4I',raw,4)
    symbols=32+data_size+4*relocs;strings=symbols+8*(public+external)
    roots={}
    for i in range(public):
        offset,name=struct.unpack_from('>2I',raw,symbols+8*i)
        roots[raw[strings+name:].split(b'\0',1)[0].decode()]=offset
    word=lambda offset:struct.unpack_from('>I',raw,32+offset)[0]
    peach=roots['ftDataPeach'];fighter_attrs=word(peach+4);slots=word(peach+72)
    slot_map={'It_Kind_Peach_Explode':0,'It_Kind_Peach_Turnip':1,'It_Kind_Peach_Parasol':2,'It_Kind_Peach_Toad':3,'It_Kind_Peach_ToadSpore':4}
    by_name={p['name']:(p,v) for p,v in zip(params,values)};active=None;provenance=[]
    for line in source.splitlines():
        if line.startswith('article '):active=line.split()[1]
        m=re.match(r'(\s*)attribute (float|int) (\w+) (\d+)$',line)
        if not m:continue
        indent,typ,name,offset=m.groups();offset=int(offset)
        base=fighter_attrs if not indent else word(word(slots+slot_map[active]*4)+4)
        qualified=name if not indent else active+'.'+name
        p,v=by_name[qualified];assert p['type']==typ
        retail=word(base+offset)
        if retail!=v:raise ValueError('Raw retail bytes disagree with import: '+qualified)
        provenance.append({'name':qualified,'offset':offset,'DAT_data_offset':base+offset,'bits':retail,'value':p['value'],'slot':None if not indent else slot_map[active]})
    assert len(provenance)==41
    state_bytes=int(re.search(r'generated.state_bytes = script_state_base \+ (\d+);',gen)[1])
    (o/'census.json').write_text(json.dumps({'parameters':params,'parameter_count':len(params),'article_states':{'Toad':2,'ToadSpore':1,'Turnip':5,'Parasol':3},'fighter_states':len(fields),'fighter_state_bytes':state_bytes,'fighter_state_payload_bytes':sum(1 if t=='bool' else 4 for t,_ in fields),'authored_motion_phases':16,'full_original_files':3,'extracted_shared_bodies':sum(len(x) for x in groups.values()),'source_hashes':sources,'required_hosts':hosts,'raw_dat_provenance':provenance,'DAT_slot_map':slot_map},indent=2)+'\n')
if __name__=='__main__':main()
