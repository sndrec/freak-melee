// Controlled common decisions expose exact caller order, not input physics.
unsigned fighter_input_calls,fighter_input_stop;
static bool common_input(Fighter_GObj* object,unsigned ordinal)
{
    (void)object;
    if(++fighter_input_calls!=ordinal)abort();
    return fighter_input_stop==ordinal;
}
#define INPUT_LEAF(name,ordinal) bool name(Fighter_GObj* o){return common_input(o,ordinal);}
INPUT_LEAF(ftCo_SpecialS_CheckInput,1)
INPUT_LEAF(ftCo_Attack100_CheckInput,2)
INPUT_LEAF(ftCo_800D6824,3)
INPUT_LEAF(ftCo_800D68C0,4)
INPUT_LEAF(ftCo_Catch_CheckInput,5)
INPUT_LEAF(ftCo_AttackS4_CheckInput,6)
INPUT_LEAF(ftCo_AttackHi4_CheckInput,7)
INPUT_LEAF(ftCo_AttackLw4_CheckInput,8)
INPUT_LEAF(ftCo_AttackS3_CheckInput,9)
INPUT_LEAF(ftCo_AttackHi3_CheckInput,10)
INPUT_LEAF(ftCo_AttackLw3_CheckInput,11)
INPUT_LEAF(ftCo_Attack1_CheckInput,12)
INPUT_LEAF(ftCo_80099794,13)
INPUT_LEAF(ftCo_80091A4C,14)
INPUT_LEAF(ftCo_800DE9D8,15)
INPUT_LEAF(ftCo_Jump_CheckInput,16)
INPUT_LEAF(ftCo_Dash_CheckInput,17)
INPUT_LEAF(ftCo_800D5FB0,18)
INPUT_LEAF(ftCo_Turn_CheckInput,19)
INPUT_LEAF(ftCo_Walk_CheckInput,20)
#undef INPUT_LEAF
bool ftFx_AppealS_CheckInput(Fighter_GObj* object){(void)object;abort();}
bool ftFx_AppealS_CheckIfUsed(Fighter* fighter){(void)fighter;abort();}
