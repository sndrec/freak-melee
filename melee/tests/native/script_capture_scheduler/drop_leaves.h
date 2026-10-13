// Controlled stage queries and pose sampling around the actual native unlink.
#include <melee/ft/kinds/ftKirby/forward.h>
#include <melee/ft/native_script_binding_fighter_observations.h>
#include <melee/mp/mpcoll.h>
static Fighter* drop_attached;
static unsigned drop_floor_mode;
void lb_8000B1CC(HSD_JObj* j,Vec3* local,Vec3* out)
{CHECK(j&&!local&&out);*out=j->translate;}
void lb_8000C390(HSD_JObj* j){CHECK(j);step(31,drop_attached->gobj);}
bool mpLib_80054ED8(int line){CHECK(line==7);return drop_floor_mode<4;}
int mpLib_8005199C_Floor(Vec3* p,int skip,int only)
{CHECK(p&&skip==-1&&only==-1);return drop_floor_mode==1?-1:9;}
bool mpLinesConnected(int a,int b){CHECK(a==9&&b==7);return drop_floor_mode!=2;}
int mpLib_8004DD90_Floor(int line,Vec3* p,float* height,u32* flags,Vec3* normal)
{CHECK(line==9&&p&&height&&!flags&&!normal);*height=drop_floor_mode==3?-2:2;return 1;}
void mpColl_80043680(CollData* c,Vec3* p){CHECK(c&&p);c->cur_pos=c->last_pos=*p;}
void mpColl_80043670(CollData* c){CHECK(c);}
bool mpColl_80048654(CollData* c){CHECK(c);c->cur_pos.y=5;return drop_floor_mode==4;}
bool mpColl_800477E0(CollData* c){return mpColl_80048654(c);}
void ftCommon_8007D7FC(Fighter* fp){step(32,fp->gobj);fp->ground_or_air=GA_Ground;}
