"""Extract complete original phases/constructors and bounded shared bodies from the frozen SDK."""
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
    for path in ['src/melee/ft/kinds/ftPeach/ftpeachspeciallw.c','src/melee/ft/kinds/ftPeach/ftpeachattacks4.c','src/melee/it/kinds/itpeachexplode.c']:
        text=(r/path).read_text();blocks.append(re.sub(r'(?m)^#include[^\n]*\n','',text));sources[path]={'sha256':hashlib.sha256((r/path).read_bytes()).hexdigest(),'scope':'complete file without include directives'}
    groups={'src/melee/it/native_article.c':['nativeArticle_Creator','article_logic','nativeArticle_Create'],'src/melee/ft/ft_081B.c':['ft_8008403C','ft_80082C74'],'src/melee/it/kinds/itpeachturnip.c':['it_802BD45C','it_802BD32C','it_802BD4AC','itPeachTurnip_Logic56_PickedUp'],'src/melee/it/it_26B1.c':['it_8026B3A8','it_8026BB44','it_8026BD24','it_8026BDB4'],'src/melee/it/it_2725.c':['it_80272C08','it_80274C60','HSD_JObjSetScale_2','it_80274594','it_80274574','it_80275158','it_8027518C','it_802751D8']}
    # Prototypes from actual headers cover forward calls in the complete files.
    for path,names in groups.items():
        text=(r/path).read_text();sources[path]={'sha256':hashlib.sha256((r/path).read_bytes()).hexdigest(),'bodies':{}}
        for name in names:
            b=body(text,name);blocks.append(b);sources[path]['bodies'][name]={'line':text[:text.index(b)].count('\n')+1,'sha256':hashlib.sha256(b.encode()).hexdigest()}
    table_path='src/melee/ft/kinds/ftPeach/ftpeach.c'
    table_source=(r/table_path).read_text();table=table_source[table_source.index('MotionState ftPe_Init_MotionStateTable'):];table=table[:table.index('};')]
    rows=re.findall(r'\{\s*// ([^\n]+)\n(.*?)\n    \},',table,re.S)
    if len(rows)!=30:raise ValueError('Retail Peach table changed')
    selected=[];ids=[]
    for comment,row in rows:
        motion=int(comment.split('=')[-1])
        if 349<=motion<=353 or motion==356:selected.append('{'+row+'\n}');ids.append(motion)
    (o/'retail_slots.inc').write_text('static const unsigned retail_ids[]={'+','.join(map(str,ids))+'};\nstatic const MotionState retail_slots[]={\n'+',\n'.join(selected)+'\n};\n')
    sources[table_path]={'sha256':hashlib.sha256((r/table_path).read_bytes()).hexdigest(),'scope':'exact source motion rows349..353 and356 compiled as C metadata'}
    logic_path='src/melee/it/it_3F2F.c';logic_source=(r/logic_path).read_text()
    logic_row=re.search(r'\{\s*// Peach explode\n(.*?)\n    \},',logic_source,re.S)[1]
    blocks.append('static const ItemLogicTable retail_explode_logic={'+logic_row+'\n};')
    sources[logic_path]={'sha256':hashlib.sha256((r/logic_path).read_bytes()).hexdigest(),'scope':'complete actual Explode logic row'}
    (o/'actual.inc').write_text('\n\n'.join(blocks)+'\n')
    values=[];params=[]
    for line in a.parameters.read_text().splitlines():
        typ,name,eq,v=line.split();assert eq=='=';params.append({'type':typ,'name':name,'value':v});values.append(struct.unpack('<I',struct.pack('<f',float(v)))[0] if typ=='float' else int(v,0)&0xffffffff)
    gen=a.generated.read_text();count,schema=re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL',gen).groups();assert int(count)==len(values)==53
    (o/'parameters.inc').write_text('static const uint32_t parameter_bits[]={'+','.join('UINT32_C(%d)'%v for v in values)+'};\nstatic const uint64_t parameter_schema='+schema+'ULL;\n')
    hosts=sorted(set(re.findall(r'host->(\w+) == 0',gen)))
    (o/'hosts.inc').write_text('\n'.join('host.'+h+' = (__typeof__(host.'+h+'))unexpected_boundary;' for h in hosts)+'\n')
    (o/'census.json').write_text(json.dumps({'parameters':params,'parameter_count':len(params),'article_states':{'Explode':2,'Turnip':5,'Parasol':3,'Toad':2},'fighter_states':14,'fighter_state_bytes':57,'authored_motion_phases':18,'untouched_null_state':356,'full_original_files':3,'extracted_shared_bodies':sum(len(x) for x in groups.values()),'source_hashes':sources,'normal_clone_import_pending':True,'required_hosts':hosts},indent=2)+'\n')
if __name__=='__main__':main()
