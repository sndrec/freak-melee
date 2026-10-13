"""Exact entry bodies; native static identifier collisions are renamed only."""
import hashlib, importlib.util, re
from pathlib import Path
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'script_fighter_scheduler'
spec=importlib.util.spec_from_file_location('attack_reference',BASE/'extract_scheduler.py')
reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
FUNCTIONS={
 'Attack1':['onPkPc21EC','decideAttack11','getMotionFlags','ftCo_Attack11_EnterMotion','doAttack12Rapid','doAttack12Normal','doAttack12','doAttack13'],
 'Attack100':['fn_800D6AC4','ftCo_800D6B00','fn_800D6B8C'],
 'AttackLw3':['callUnk','decideFighter','doEnter'],
 'AttackAir':['decideFighter','ftCo_AttackAir_GetMsidFromCStick','ftCo_AttackAir_EnterFromCStick','ftCo_AttackAir_EnterFromMsid'],
 'AttackS4':['decideFighter','doEnter'],
 'SquatWait':['ftCo_SquatWait_Enter_inline','fn_800D62C4','ftCo_800D638C'],
}

def body(source,name):
    return reference.reference.reference.body(source,name)

def extract(root,out):
    hashes=reference.extract(root,out/'actual_scheduler.h');bodies=[];prefix=[];unit_includes=[]
    for unit,names in FUNCTIONS.items():
        path=root/f'src/melee/ft/kinds/ftCommon/ftCo_{unit}.c'
        hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
        source=reference.reference.native_only(path.read_text())
        unit_includes += re.findall(r'^#include .+$',source,re.M)
        rename={name:unit+'_'+name for name in names if name.startswith(('decide','doEnter','doAttack','getMotion','onPk','callUnk'))}
        if unit in ['AttackS4','AttackLw3']:
            match=re.search(r'typedef enum cmd_var_idx \{.*?\} cmd_var_idx;',source,re.S)
            assert match;rename.update({'cmd_var_idx':unit+'_cmd_var_idx','cmd_unk0_bool':unit+'_cmd_unk0_bool'})
            declaration=match.group(0)
            for old,new in rename.items():declaration=re.sub(r'\b'+old+r'\b',new,declaration)
            prefix.append(declaration)
        for name in names:
            value=body(source,name)
            value=re.sub(r'^(static\s+)?(inline\s+)?','',value)
            for old,new in rename.items():value=re.sub(r'\b'+old+r'\b',new,value)
            bodies.append(value)
    prototypes=[value[:value.index('{')].rstrip()+';' for value in bodies]
    (out/'actual_attack.h').write_text('\n'.join(prefix+prototypes+bodies)+'\n')
    source=(BASE/'boundary.c').read_text()
    old='void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId id,MotionFlags flags,float start,float speed,float blend,Fighter_GObj* target){(void)flags;(void)start;(void)speed;(void)blend;(void)target;((Fighter*)o->user_data)->motion_id=id;nativeScriptFighter_SelectMotion(o,id);}'
    assert old in source
    source=source.replace(old,old[:-1]+'attack_record_motion(o,id,flags,start,speed,blend);}')
    old='fighter->cur_pos.x=v->as.f32;'
    assert old in source
    source=source.replace(old,old+'attack_record_source(c->source,v->as.f32);')
    includes='''#include "attack_boundary.h"
#include <melee/ft/ftwaitanim.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack1.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_SquatWait.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftCommon/ftCo_LandingAir.h>
#include <melee/ft/ft_0DF1.h>
'''
    (out/'boundary.c').write_text(includes+'\n'.join(dict.fromkeys(unit_includes))+'\n'+source+'\n#include "actual_attack.h"\n')
    return hashes
