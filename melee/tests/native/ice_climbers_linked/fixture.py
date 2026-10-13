"""Freeze complete Squall/Belay bodies and stage retail-owned named tuning."""
import argparse, hashlib, json, re, struct, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
TUNING = {'squall_input_threshold':0x40, 'squall_ground_input_accel':0x30,
 'squall_air_input_accel':0x34, 'squall_air_max':0x3c,
 'squall_solo_gravity':0x4c, 'squall_pair_gravity':0x50,
 'squall_solo_terminal':0x54, 'squall_pair_terminal':0x58,
 'squall_gravity_duration':0x5c, 'squall_solo_pump_speed':0x60,
 'squall_pair_pump_speed':0x64, 'squall_pump_interval':0x68,
 'belay_mobility':0x74, 'belay_landing_lag':0x78,
 'belay_solo_launch_speed':0xa4, 'belay_solo_gravity':0xa8,
 'belay_solo_terminal':0xac}

def body(text, name):
    m = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text)
    if not m: raise ValueError(name)
    end, depth=m.end(),1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[m.start():end]

def main():
    a=argparse.ArgumentParser()
    for k in ('root','baseline','output','iso'): a.add_argument('--'+k,type=Path,required=True)
    x=a.parse_args(); root=x.root.resolve(); sdk=x.baseline.resolve()/'sdk'; out=x.output.resolve();out.mkdir(parents=True,exist_ok=True)
    sys.path.insert(0,str(root/'tools'))
    from extract_native_animation import extract_clip
    data,offset=extract_clip(x.iso.resolve(),'PlPp.dat','ftDataPopo')
    start=struct.unpack_from('>I',data,32+offset+4)[0]
    params=[];lines=[];initializers=[]
    for line in (HERE/'solo.fms').read_text(encoding='utf-8').splitlines():
        m=re.fullmatch(r'parameter (float|int) (\w+) = .+',line)
        if not m: lines.append(line);continue
        kind,name=m.groups(); at=TUNING[name];bits=struct.unpack_from('>I',data,32+start+at)[0]
        value=struct.unpack('>i' if kind=='int' else '>f',struct.pack('>I',bits))[0]
        params.append(dict(name=name,type=kind,value=value,bits=bits,offset=at))
        initializers.append('{ uint32_t bits=UINT32_C(%d); memcpy((char*)&attributes+%d,&bits,4); }'%(bits,at))
    (out/'owned.fms').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    (out/'fighter_parameters.fmp').write_text(''.join(f"{p['type']} {p['name']} = {p['value']}\n" for p in params),encoding='utf-8')
    (out/'attributes.inc').write_text('\n'.join(initializers)+'\n',encoding='utf-8')
    groups={
      'src/melee/ft/kinds/ftPopo/ftpopospecials.c':['ftPp_SpecialS1_IASA','ftPp_SpecialS2_IASA','ftPp_SpecialAirS1_IASA','ftPp_SpecialAirS2_IASA','ftPp_SpecialAirS1_Phys','ftPp_SpecialAirS2_Phys'],
      'src/melee/ft/kinds/ftPopo/ftpopospecialhi.c':[
       'ftPp_SpecialHiStart_1_Anim','ftPp_SpecialAirHiStart_1_Anim','ftPp_SpecialHiStart_1_IASA','ftPp_SpecialAirHiStart_1_IASA',
       'ftPp_SpecialHiStart_1_Phys','ftPp_SpecialAirHiStart_1_Phys','ftPp_SpecialHiStart_1_Coll','ftPp_SpecialAirHiStart_1_Coll',
       'ftPp_SpecialHiThrow_1_Anim','ftPp_SpecialAirHiThrow_1_Anim','ftPp_SpecialHiThrow_1_IASA','ftPp_SpecialAirHiThrow_1_IASA',
       'ftPp_SpecialHiThrow_1_Phys','ftPp_SpecialAirHiThrow_1_Phys','ftPp_SpecialHiThrow_1_Coll','ftPp_SpecialAirHiThrow_1_Coll',
       'ftPp_SpecialHi_80121FD8','ftPp_SpecialHi_80122038','ftPp_SpecialHi_801222E8','ftPp_SpecialHi_80122348','ftPp_SpecialHi_80122380'],
      'src/melee/ft/ftmovement.c':['ftCommon_Fall','ftCommon_FallBasic','ftCommon_CalcSelfAccel_DeaccelAir','ftCommon_CalcSelfAccel_AccelToVel']}
    actual=[];sources={}
    for path,names in groups.items():
        raw=(sdk/path).read_bytes(); text=raw.decode('utf-8');entry=dict(sha256=hashlib.sha256(raw).hexdigest(),bodies={})
        for name in names:
            b=body(text,name);actual.append(b);entry['bodies'][name]=dict(sha256=hashlib.sha256(b.encode()).hexdigest(),line=text[:text.index(b)].count('\n')+1)
        sources[path]=entry
    (out/'actual.inc').write_text('\n\n'.join(actual)+'\n',encoding='utf-8')
    census={}
    for path in ['src/melee/ft/kinds/ftPopo/ftpopo.c','src/melee/ft/kinds/ftNana/ftnana.c','src/melee/ft/kinds/ftPopo/ftpopospecials.c','src/melee/ft/kinds/ftNana/ftnanaspecials.c','src/melee/ft/kinds/ftPopo/ftpopospecialhi.c','src/melee/ft/kinds/ftNana/ftnanaspecialhi.c','src/melee/it/kinds/itclimbersstring.c','src/melee/it/kinds/itlinkhookshot.c','src/melee/ft/kinds/ftCommon/ftCo_0A01.c','src/melee/ft/fighter.c','src/melee/ft/ftcommon.c','src/melee/ft/ftmovement.c']:
        raw=(sdk/path).read_bytes();census[path]=hashlib.sha256(raw).hexdigest()
    (out/'provenance.json').write_text(json.dumps(dict(parameters=params,sources=sources,closure_sources=census,DAT_sha256=hashlib.sha256(data).hexdigest(),scope='22 complete scalar phase callbacks; no paired/entry/cleanup claim'),indent=2)+'\n',encoding='utf-8')
if __name__=='__main__':main()
