/* Compare the repaired native callback with its explicit-table stock helper.
 * Common engine effects/lifetime operations are recorded; no authored Arrow
 * override is exercised or registered by this test. */
#include <melee/it/kinds/itlinkarrow.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float random_value;
static unsigned random_calls, effects, hides, collision_modes, removals;
static bool expired;

void __assert(char* file,u32 line,char* message)
{ fprintf(stderr,"%s:%lu: %s\n",file,line,message);abort(); }
float HSD_Randf(void) { ++random_calls;return random_value; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags|=JOBJ_MTX_DIRTY; }
void (HSD_JObjSetMtxDirty)(HSD_JObj* joint) { joint->flags|=JOBJ_MTX_DIRTY; }
void HSD_JObjRemoveAll(HSD_JObj* joint) { (void)joint;++removals; }
bool it_80273130(Item_GObj* gobj) { (void)gobj;return expired; }
void it_802787B4(Item_GObj* gobj,s32 id)
{ (void)gobj;if(id!=0x421)abort();++effects; }
void it_802756D0(Item_GObj* gobj)
{ ++collision_modes;((Item*)gobj->user_data)->xD0C=2; }
void it_8026BB44(Item_GObj* gobj)
{ ++hides;((HSD_JObj*)gobj->hsd_obj)->flags|=JOBJ_HIDDEN; }

int main(void)
{
    unsigned cases=0;
    const float values[]={0,.25f,.75f,1};
    const int tails[]={0,1,299,300};
    for(int index=-1;index<=7;++index) for(unsigned value=0;value<4;++value)
    for(unsigned expiry=0;expiry<2;++expiry) for(unsigned tail=0;tail<4;++tail) {
        Item a={0},b={0};HSD_JObj root_a={0},root_b={0},trails[2]={0};
        HSD_GObj helper={.user_data=&a,.hsd_obj=&root_a};
        HSD_GObj callback={.user_data=&b,.hsd_obj=&root_b};
        a.xDD4_itemVar.linkarrow.x9C=b.xDD4_itemVar.linkarrow.x9C=index;
        a.xDD4_itemVar.linkarrow.x94=b.xDD4_itemVar.linkarrow.x94=.75f;
        b.xDD4_itemVar.linkarrow.xF0=tails[tail];
        b.xDD4_itemVar.linkarrow.xB4[0]=&trails[0];
        b.xDD4_itemVar.linkarrow.xB4[1]=&trails[1];
        random_value=values[value];expired=expiry!=0;random_calls=0;
        itLinkArrow_802A81C4(&helper);unsigned helper_random=random_calls;
        random_calls=effects=hides=collision_modes=removals=0;
        bool removed=itLinkarrow_UnkMotion4_Anim(&callback);
        bool first=expiry && tails[tail]==0;
        bool expected_remove=expiry && tails[tail]+1>300;
        bool ok=root_a.rotate.z==root_b.rotate.z &&
            a.xDD4_itemVar.linkarrow.x9C==b.xDD4_itemVar.linkarrow.x9C &&
            helper_random==random_calls && random_calls==(unsigned)(index>=0 && index<=6) &&
            removed==expected_remove && effects==(unsigned)first && collision_modes==(unsigned)first &&
            hides==expiry && removals==2*expiry && b.xDCF_flag.b2==expiry &&
            b.xDD4_itemVar.linkarrow.xF0==tails[tail]+(int)expiry &&
            b.xD0C==(first?2:0) &&
            (b.xDD4_itemVar.linkarrow.xB4[0]==NULL)==(expiry!=0) &&
            (b.xDD4_itemVar.linkarrow.xB4[1]==NULL)==(expiry!=0);
        ++cases;
        if(!ok) {
            fprintf(stderr,"Arrow vibration failed index%d rng%g expiry%u tail%d: angles%g/%g rng%u/%u remove%d\n",
                    index,values[value],expiry,tails[tail],root_a.rotate.z,root_b.rotate.z,helper_random,random_calls,removed);
            return 1;
        }
    }
    printf("Stock Arrow vibration comparison passed %u cases.\n",cases);return 0;
}
