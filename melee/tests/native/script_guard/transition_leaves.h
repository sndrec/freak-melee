// Controlled leaves for the complete native transition scheduler bodies.
// Any stock/fighter-specific fallback is an error in the authored-hook cases.
static unsigned transition_special_tails,transition_statistics;
void ftCo_80092E50(Fighter_GObj*);
#define TRANSITION_STOCK(name) void name(Fighter_GObj* o){(void)o;abort();}
TRANSITION_STOCK(ftCo_800924C0)
TRANSITION_STOCK(ftCo_80092908)
TRANSITION_STOCK(ftCo_80092C54)
TRANSITION_STOCK(ftCo_80092C54_inline)
TRANSITION_STOCK(ftCo_8009388C)
TRANSITION_STOCK(ftCo_80093A50)
TRANSITION_STOCK(ftCo_80092908_inline2)
TRANSITION_STOCK(ftYs_Init_8012BECC)
TRANSITION_STOCK(ftYs_Shield_8012C1D4)
TRANSITION_STOCK(ftYs_Shield_8012C49C)
TRANSITION_STOCK(ftYs_Shield_8012C850)
TRANSITION_STOCK(ftYs_Shield_8012C914)
#undef TRANSITION_STOCK
void ftYs_Shield_8012C600(Fighter_GObj* o,bool special)
{(void)o;(void)special;abort();}
void ftCo_80092F2C(Fighter_GObj* o,bool special)
{(void)o;(void)special;abort();}
void ftParts_80074B0C(Fighter_GObj* o,int model,int value)
{(void)o;(void)model;(void)value;abort();}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId motion,
    MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* copy)
{if(!flow_testing)abort();CHECK(flags==Ft_MF_None&&frame==0&&rate==1&&blend==0&&!copy);((Fighter*)o->user_data)->motion_id=motion;flow_event('m',o);}
void ftCo_80092158_inline(Fighter_GObj* o,int effect,HSD_JObj* j)
{(void)o;(void)effect;(void)j;abort();}
void ftCo_80091E78(Fighter_GObj* o,float blend)
{(void)o;(void)blend;abort();}
void ftCo_800C36DC(Fighter_GObj* o)
{CHECK(o&&nativeScriptFighter_Status(o)==NativeScriptVM_Ok);++transition_special_tails;}
void pl_8003E114(int player,int flag,float health)
{(void)player;(void)flag;(void)health;++transition_statistics;}
