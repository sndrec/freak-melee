"""Extract unchanged stock bodies/decision branches and actual common routing.

Constructors, imported asset copies, animation/item presentation and CPU probes
are explicit fixture boundaries. No gameplay or algorithm replacement here.
"""
import argparse, hashlib, re
from pathlib import Path
SOURCE_UNITS = [
 "melee/ft/kinds/ftDonkey/ftdonkey.c", "melee/ft/kinds/ftKoopa/ftkoopa.c",
 "melee/ft/ftcpuattack.c", "melee/ft/kinds/ftCommon/ftCo_0A01.c",
 "melee/ft/kinds/ftDonkey/ftdonkeyheavywait0.c",
 "melee/ft/kinds/ftDonkey/ftdonkeyheavylanding.c",
 "melee/ft/kinds/ftDonkey/ftdonkeyheavywalk.c",
 "melee/ft/kinds/ftCommon/ftCo_CargoWalk.c",
 "melee/ft/kinds/ftCommon/ftCo_Throw.c", "melee/ft/kinds/ftCommon/ftCo_ItemThrow.c",
 "melee/ft/kinds/ftKoopa/ftkoopaspecialn.c", "melee/ft/native_fighter_module.c",
 "melee/ft/kinds/ftCommon/ftCo_CargoWait.c", "melee/ft/kinds/ftCommon/ftCo_CargoThrow.c",
 "melee/ft/kinds/ftDonkey/ftdonkeyspecialn.c"]
def brace(text,start):
    end,depth=start+1,1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return end

def body(text,name):
    match=re.search(r'(?m)^(?:static inline |static )?(?:void|bool) '+name+r'\([^;]*?\)\s*\{',text)
    if not match: raise ValueError('Missing actual body '+name)
    return text[match.start():brace(text,match.end()-1)]+'\n'
def condition(text,needle):
    start=text.index(needle);opening=text.index('{',start)
    return text[start:brace(text,opening)]
def case(text,needle,nextcase):
    start=text.index(needle);end=text.index('    case '+nextcase+':',start)
    return text[start:end]
def extract(source_root,output):
    sources={p:(source_root/p).read_text() for p in SOURCE_UNITS}
    unit='// Actual frozen production source; scoped extracted branch glue is explicit.\n'
    for path in SOURCE_UNITS:
        unit+='// '+path+' SHA256 '+hashlib.sha256((source_root/path).read_bytes()).hexdigest()+'\n'
    for path in sorted((source_root/'melee/ft/kinds').glob('*/forward.h')):
        unit+='#include <'+path.relative_to(source_root).as_posix()+'>\n'
    tables=[];callbacks=set()
    for family,prefix in [('ftDonkey','ftDk'),('ftKoopa','ftKp')]:
        text=sources[f'melee/ft/kinds/{family}/{family.lower()}.c']
        match=re.search(r'MotionState '+prefix+r'_Init_MotionStateTable\[.*?\n\};',text,re.S)
        assert match;tables.append(match[0])
        callbacks.update(re.findall(r'(?m)^\s+(ft\w+_(?:Anim|IASA|Phys|Coll)|ftCamera_UpdateCameraBox),',match[0]))
        for event in ['OnDeath','OnLoad','LoadSpecialAttrs','OnItemPickup','OnItemDrop','OnItemVisible','OnItemInvisible','OnKnockbackEnter','OnKnockbackExit']:
            unit+=body(text,prefix+'_Init_'+event)
        if family=='ftDonkey':unit+=body(text,'ftDk_Init_UnkMotionStates4')
    specials={'ftDk':('N','AirN','S','AirS','Hi','AirHi','Lw'),'ftKp':('N','AirN','S','AirS','Hi','AirHi','Lw','AirLw')}
    for prefix,names in specials.items():callbacks.update(prefix+'_Special'+name+'_Enter' for name in names)
    punch_names=['ftDk_SpecialN_Coll','ftDk_SpecialAirN_Coll','ftDk_SpecialNFull_Coll','ftDk_SpecialAirNFull_Coll']
    callbacks.difference_update(punch_names)
    for callback in sorted(callbacks):unit+=f'void {callback}(HSD_GObj* object) {{ (void)object; unexpected_host_call(); }}\n'
    unit+='\n'.join(tables)+'\n'
    common=sources['melee/ft/kinds/ftCommon/ftCo_0A01.c'];attack=sources['melee/ft/ftcpuattack.c']
    for name in ['ftCo_CpuSetNeutralStick','ftCo_CpuTapB','ftCo_CpuStartCharging','ftCo_CpuTapR','ftCo_CpuTapRAndWait']:
        unit+=body(common,name)
    for name in ['ftCo_CpuTapRAndWaitFiveFrames','ftCo_CpuPressAWithLstickX']:unit+=body(attack,name)
    # Original full bodies compile their stock branches with native hook removed
    # only by the body's original preprocessor conditional, not text rewriting.
    unit+='#undef MELEE_NATIVE\n'
    for name,text in [('ftCo_800A0CB0',common),('ftCo_IsGrabbing',common),('ftCo_800B630C',attack),('ftCo_800ADC28',common)]:
        unit+=body(text,name).replace(name,'original_'+name)
    unit+='#define MELEE_NATIVE 1\n'
    # Scoped stock-only branches copied verbatim; the glue declares original
    # temporaries and returns no replacement decisions.
    branch=condition(attack[attack.index('void ftCo_800B683C'):], 'if (fp->kind == Ft_Kind_Donkey &&')
    unit+='static void original_cargo_decision(Fighter* fp) { mp_UnkStruct0* temp_r3_2; Vec3 sp6C,sp78; float var_f1,var_f2;\n'+branch+'\n}\n'
    charge_point=common.index('fp->motion_id != ftDk_MS_SpecialNLoop && fp->u.dk.x222C == 0')
    branch=condition(common[common.rfind('if (fp->kind == Ft_Kind_Donkey)',0,charge_point):], 'if (fp->kind == Ft_Kind_Donkey)')
    unit+='static void original_charge_decision(Fighter* fp) {\n'+branch+'\n}\n'
    branch=condition(attack[attack.index('void ftCo_800B658C'):], 'if (fp->motion_id == ftDk_MS_SpecialNLoop)')
    unit+='static void original_charge_attack(Fighter* fp) {\n'+branch+'\n}\n'
    unit+='static void original_spacing_init(Fighter* arg0) { struct CpuFighter* temp_r30=&arg0->cpu; switch(arg0->kind) {\n'+case(common[common.index('void ftCo_800A101C'):],'    case Ft_Kind_Donkey:','Ft_Kind_GKoops')+'default: break; } }\n'
    unit+='static void original_ranged(Fighter* fp) { struct CpuFighter* cpu=&fp->cpu; Fighter* target; int can_attack; switch(fp->kind) {\n'+case(attack[attack.index('void ftCo_800B77E8'):],'    case Ft_Kind_Koopa:','Ft_Kind_GKoops')+'default: break; } }\n'
    unit+='static bool original_threat(Fighter* fp,Fighter* arg1) { Vec3 sp24,sp18; int sp14; u32 sp10; float temp_f1,temp_f2,temp_f3,temp_f4; struct CpuFighter* temp_r31=&fp->cpu; switch(arg1->kind) {\n'+case(attack[attack.index('bool ftCo_800BB768'):],'    case Ft_Kind_GKoops:','Ft_Kind_Yoshi')+'default: break; } return false; }\n'
    # Full current hook caller bodies test real interception as well.
    unit+=body(common,'ftCo_IsGrabbing').replace('ftCo_IsGrabbing','production_grabbing')
    unit+=body(attack,'ftCo_800B630C').replace('ftCo_800B630C','production_attacking')
    unit+=body(common,'ftCo_800A0CB0').replace('ftCo_800A0CB0','production_spacing')
    unit+=body(sources['melee/ft/kinds/ftDonkey/ftdonkeyheavywait0.c'],'ftDk_MS_341_800DF980')
    landing=sources['melee/ft/kinds/ftDonkey/ftdonkeyheavylanding.c']
    for name in ['checkSomething','doSomething','ftDk_MS_349_800E06D8']:unit+=body(landing,name)
    unit+=body(sources['melee/ft/kinds/ftDonkey/ftdonkeyheavywalk.c'],'ftDk_MS_341_800DFAE4')
    unit+=body(sources['melee/ft/kinds/ftCommon/ftCo_CargoWalk.c'],'ftCo_8009B6C8')
    throw=sources['melee/ft/kinds/ftCommon/ftCo_Throw.c']
    unit+=body(throw,'ftCo_800DD4B0')
    unit+='#undef MELEE_NATIVE\n'+body(throw,'ftCo_ThrowF_Anim').replace('ftCo_ThrowF_Anim','original_throw_forward')+'#define MELEE_NATIVE 1\n'
    unit+=body(throw,'ftCo_ThrowF_Anim').replace('ftCo_ThrowF_Anim','production_throw_forward')
    cargo=sources['melee/ft/kinds/ftCommon/ftCo_CargoWait.c']
    unit+=body(cargo,'inlineA0').replace('inlineA0','cargo_wait_original')
    unit+=body(cargo,'ftCo_8009B56C').replace('inlineA0','cargo_wait_original')
    cargo=sources['melee/ft/kinds/ftCommon/ftCo_CargoThrow.c']
    unit+=body(cargo,'inlineB0').replace('inlineB0','cargo_transition_original')
    punch=sources['melee/ft/kinds/ftDonkey/ftdonkeyspecialn.c']
    unit+=body(punch,'setCallbacks')
    for name in punch_names:unit+=body(punch,name)
    unit+=body(sources['melee/ft/kinds/ftCommon/ftCo_ItemThrow.c'],'ftCo_800961D0')
    unit+=body(sources['melee/ft/kinds/ftCommon/ftCo_ItemThrow.c'],'ftCo_80096498')
    unit+=body(sources['melee/ft/kinds/ftKoopa/ftkoopaspecialn.c'],'ftKp_SpecialLw_80134D78')
    for name in ['copy_ext_attributes','reload_ext_attributes']:unit+=body(sources['melee/ft/native_fighter_module.c'],name)
    host=(source_root/'melee/ft/native_fighter_module.h').read_text().split('typedef struct NativeFighterHost {',1)[1].split('} NativeFighterHost;',1)[0]
    host=re.sub(r'//[^\n]*','',host)
    slots=re.findall(r'\(\*(\w+)\)\s*\(',host)+re.findall(r'\bHSD_GObj(?:Event|Predicate)\s+(\w+)\s*;',host)
    remainder=re.sub(r'[^;]*\(\*\w+\)\s*\([^;]*?;','',host)
    remainder=re.sub(r'\bHSD_GObj(?:Event|Predicate)\s+\w+\s*;','',remainder)
    assert len(slots)==len(set(slots)) and re.sub(r'\s+','',remainder)=='constMotionState*common_motion_states;'
    unit+='static void closure_host_traps(NativeFighterHost* host) {\n'+''.join(f'host->{slot}=(__typeof__(host->{slot}))unexpected_host_call;\n' for slot in slots)+'}\n'
    output.write_text(unit,encoding='utf-8',newline='\n')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('source_root',type=Path);p.add_argument('output',type=Path);a=p.parse_args();extract(a.source_root,a.output)
