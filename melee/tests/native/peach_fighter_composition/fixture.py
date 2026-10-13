"""Extract complete setup bodies plus explicitly bounded original CPU/dynamics slices."""
import argparse,hashlib,json,re,struct
from pathlib import Path

def body(text,name):
    m=re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text)
    if not m:raise ValueError(name)
    end=m.end();depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[m.start():end]

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--generated',type=Path,required=True);p.add_argument('--parameters',type=Path,required=True);p.add_argument('--dat',type=Path,required=True);a=p.parse_args();out=a.output;root=a.root
    units={'src/melee/ft/kinds/ftPeach/ftpeach.c':['ftPe_Init_OnDeath','ftPe_Init_OnLoad','ftPe_Init_OnItemPickup','ftPe_Init_OnItemInvisible','ftPe_Init_OnItemVisible','ftPe_Init_OnItemDrop','ftPe_Init_LoadSpecialAttrs','ftPe_Init_OnKnockbackEnter','ftPe_Init_OnKnockbackExit'],'src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c':['ftPe_JumpAerial_Enter'],'src/melee/ft/ftcpuattack.c':['ftCo_800B630C','ftCo_CpuRangedAttackUnsafe','ftCo_CpuAddAttackCandidate']}
    blocks=[];sources={}
    cpu_source=(root/'src/melee/ft/ftcpuattack.c').read_text()
    blocks.append(re.search(r'typedef struct ftCo_CollData \{.*?\} ftCo_CollData;',cpu_source,re.S)[0])
    for path,names in units.items():
        text=(root/path).read_text();sources[path]={'sha256':hashlib.sha256((root/path).read_bytes()).hexdigest(),'bodies':{}}
        for name in names:
            b=body(text,name);sources[path]['bodies'][name]={'line':text[:text.index(b)].count('\n')+1,'sha256':hashlib.sha256(b.encode()).hexdigest()}
            if name=='ftCo_800B630C':
                # Compile the original stock path, not its later authored dispatch branch.
                b=re.sub(r'#ifdef MELEE_NATIVE.*?#endif\n','',b,flags=re.S)
            blocks.append('// Actual '+path+'\n'+b)
    cpu=(root/'src/melee/ft/ftcpuattack.c').read_text();start=cpu.index('    case Ft_Kind_Peach:',cpu.index('void ftCo_800B77E8('));end=cpu.index('    case Ft_Kind_Kirby:',start)
    ranged=cpu[start:end]
    blocks.append('static void original_peach_ranged(Fighter* fp) {\nstruct CpuFighter* cpu=&fp->cpu; ftCo_CollData c10; Fighter* target; int can_attack; f32 x,y;\nswitch(fp->kind) {\n'+ranged+'default:break; }\n}')
    dynpath='src/melee/ft/ftdynamics.c';dyn=(root/dynpath).read_text();start=dyn.index('        if (fp->kind == Ft_Kind_Peach) {',dyn.index('on_dynamics_interaction_start'));end=dyn.index(' else if (fp->kind == Ft_Kind_Mewtwo)',start)
    ds=dyn[start:end]
    blocks.append('static void original_peach_dynamics(Fighter* fp,BoneDynamicsDesc* dyn,u32 dynamics_num,f32 arg1,bool ret_B0) { u32 i;\n'+ds+'\n}')
    sources[dynpath]={'sha256':hashlib.sha256((root/dynpath).read_bytes()).hexdigest(),'statement_slice_sha256':hashlib.sha256(ds.encode()).hexdigest()}
    sources['src/melee/ft/ftcpuattack.c']['ranged_slice_sha256']=hashlib.sha256(ranged.encode()).hexdigest()
    (out/'actual.inc').write_text('\n\n'.join(blocks)+'\n')
    rebound_path='src/melee/ft/kinds/ftPeach/ftpeachspecials.c';rebound=(root/rebound_path).read_text();start=rebound.index('            case Ft_Kind_Peach:',rebound.index('static void doAirEnd0('));end=rebound.index('{',start)+1;depth=1
    while depth:depth+=(rebound[end]=='{')-(rebound[end]=='}');end+=1
    rs=rebound[start:end]
    blocks.append('static void original_peach_rebound(Fighter* other_fp) { rebound_allowed=false; switch(other_fp->kind) {\n'+rs+'\ndefault:break; } rebound_allowed=true; }')
    sources[rebound_path]={'sha256':hashlib.sha256((root/rebound_path).read_bytes()).hexdigest(),'statement_slice_sha256':hashlib.sha256(rs.encode()).hexdigest()}
    (out/'actual.inc').write_text('\n\n'.join(blocks)+'\n')
    generated=a.generated.read_text();values=[];names=[]
    for line in a.parameters.read_text().splitlines():
        k,n,eq,v=line.split();assert eq=='=';names.append({'type':k,'name':n,'value':v});values.append(struct.unpack('<I',struct.pack('<f',float(v)))[0] if k=='float' else int(v)&0xffffffff)
    # The real importer currently has no Peach article slots. Pin provisional
    # inline article values against the actual DAT, without faking its import.
    raw=a.dat.read_bytes(); data_size,relocs,public,external=struct.unpack_from('>4I',raw,4)
    symbols=32+data_size+4*relocs; strings=symbols+8*(public+external)
    roots={}
    for i in range(public):
        off,name=struct.unpack_from('>2I',raw,symbols+8*i)
        roots[raw[strings+name:].split(b'\0',1)[0].decode()]=off
    word=lambda off:struct.unpack_from('>I',raw,32+off)[0]
    peach=roots['ftDataPeach']; table=word(peach+72)
    slots=[word(table+4*i) for i in range(5)]
    turnip=word(slots[1]+4); values_by_name={row['name']:row for row in names}
    offsets={'lifetime':0,'face_count':4}
    offsets.update({f'face_{suffix}_{i}':8+8*i+(4 if suffix=='damage' else 0) for i in range(8) for suffix in ['weight','damage']})
    tuning=[]
    for name,offset in offsets.items():
        row=values_by_name['It_Kind_Peach_Turnip.'+name]
        actual=struct.unpack_from('>f' if row['type']=='float' else '>i',raw,32+turnip+offset)[0]
        expected=float(row['value']) if row['type']=='float' else int(row['value'])
        if actual!=expected:raise ValueError('Provisional tuning differs from retail: '+name)
        tuning.append({'parameter':name,'offset':offset,'retail_value':actual})
    source=Path(__file__).with_name('candidate.fms').read_text()
    function_names=re.findall(r'(?m)^(?:export )?func (\w+)\(',source)
    if len(function_names)!=len(set(function_names)):raise ValueError('Duplicate function')
    declarations=re.findall(r'(?m)^state (bool|int|float) (\w+)$',source)
    if len(declarations)!=14 or len({n for _,n in declarations})!=14:raise ValueError('Unexpected fighter state census')
    if re.search(r'(?m)^\s*behavior independent\s*$',source):raise ValueError('Incomplete staging candidate cannot be independent')
    count,schema=re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL',generated).groups();assert int(count)==len(values)
    (out/'parameters.inc').write_text('static const uint32_t parameter_bits[]={'+','.join('UINT32_C(%d)'%v for v in values)+'};\nstatic const uint64_t parameter_schema='+schema+'ULL;\n')
    fields=re.findall(r'\{ "(\w+)", (\d+), (\d+), (\d+), (\d+), (true|false) \}',generated)
    funcs=re.findall(r'(?m)^(?:export )?func (\w+)\([^\n]*',Path(__file__).with_name('candidate.fms').read_text())
    hosts=sorted(set(re.findall(r'host->(\w+) == 0',generated)))
    (out/'hosts.inc').write_text('\n'.join('host.'+h+' = (__typeof__(host.'+h+'))unexpected_boundary;' for h in hosts)+'\n')
    census={'parameter_count':len(names),'parameters':names,'fighter_state_field_count':14,'fighter_state_bytes':57,'all_state_metadata_rows':fields,'functions':funcs,'function_count':len(funcs),'article_state_counts':{'It_Kind_Peach_Turnip':5,'It_Kind_Peach_Parasol':3,'It_Kind_Peach_Toad':2},'complete_article':'It_Kind_Peach_Turnip','partial_articles':['It_Kind_Peach_Parasol','It_Kind_Peach_Toad'],'custom_motion_phase_overrides':0,'required_hosts':hosts,'complete_original_body_count':13,'original_statement_slices':3,'sources':sources,'provisional_article_tuning':bool(re.search(r'(?m)^    parameter (?:float|int) (?:lifetime|face_count|face_weight_|face_damage_)',source)),'retail_turnip_tuning_checks':tuning,'article_dat_slot_offsets':slots,'normal_import_fighter_attribute_count':len(re.findall(r'(?m)^attribute ',source)),'normal_clone_import_pending':True}
    (out/'census.json').write_text(json.dumps(census,indent=2)+'\n')
if __name__=='__main__':main()
