#ifndef SCRIPT_NORMAL_ATTACK_BOUNDARY_H
#define SCRIPT_NORMAL_ATTACK_BOUNDARY_H
#include <melee/ft/types.h>
#include <melee/ft/kinds/ftCommon/forward.h>
enum {Attack_Pickup=1,Attack_Motion,Attack_Animation,Attack_HeldSetup,Attack_CommonFlags,Attack_Source};
enum{Normal_Recount=6,Normal_HitReset,Normal_Finish,Normal_Geometry,Normal_Response};
void normal_point(unsigned,HSD_GObj*);
extern bool normal_contact,normal_interrupted;
extern unsigned attack_trace[64],attack_count,attack_stop,attack_failure,attack_stock;
extern bool attack_pickup,attack_held;
extern int attack_last_motion;
void attack_record_motion(HSD_GObj*,int,MotionFlags,float,float,float);
void attack_record_source(HSD_GObj*,float);
void attack_reset(void);
void Attack1_decideAttack11(HSD_GObj*);
void Attack1_doAttack12Rapid(HSD_GObj*);
void Attack1_doAttack12(HSD_GObj*);
void Attack1_doAttack13(HSD_GObj*);
void AttackLw3_decideFighter(HSD_GObj*);
void AttackAir_decideFighter(HSD_GObj*);
void AttackS4_decideFighter(HSD_GObj*,float,float);
void fn_800D6AC4(HSD_GObj*);
void ftCo_800D6B00(HSD_GObj*,enum_t);
void fn_800D62C4(HSD_GObj*);
#endif
