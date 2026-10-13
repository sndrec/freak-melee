#include <melee/it/native_capture_item.h>
#include <melee/ft/native_fighter_capture.h>
#include <melee/ft/native_capture.h>
Fighter_Part ftKb_SpecialNYs_80109238(Fighter_GObj*);
#include "../ityoshitongue_original.inc"
#include "../itkirbyyoshispecialn_original.inc"

// Unselected victim-capture leaves are unavailable in this Item-only cohort.
bool nativeFighterCapture_Retain(Fighter_GObj* object,NativeFighterCapturePair* pair)
{ (void)object;(void)pair;abort(); }
bool nativeFighterCapture_Begin(Fighter_GObj* victim,Fighter_GObj* holder,FtMotionId motion,
    bool facing,bool air,bool track,bool animation)
{ (void)victim;(void)holder;(void)motion;(void)facing;(void)air;(void)track;(void)animation;abort(); }
bool nativeFighterCapture_AttachToVictim(Fighter_GObj* holder){(void)holder;abort();}
bool nativeCapture_ReleaseIntoEnclosure(Fighter_GObj* holder){(void)holder;abort();}
bool nativeFighterCapture_Current(const NativeFighterCapturePair* pair,bool link){(void)pair;(void)link;abort();}
void nativeFighterCapture_FollowVictim(Fighter_GObj* holder){(void)holder;abort();}
bool nativeFighterCapture_ReleaseThrow(Fighter_GObj* holder){(void)holder;abort();}
void ftCommon_8007E2D0(Fighter* holder,s16 mask,HSD_GObjEvent caught,HSD_GObjEvent other,
    void(*grabbed)(HSD_GObj*,HSD_GObj*))
{ (void)holder;(void)mask;(void)caught;(void)other;(void)grabbed;abort(); }
void ftCommon_8007E2F4(Fighter* holder,s16 mask){(void)holder;(void)mask;abort();}
void ftCommon_8007E2FC(Fighter_GObj* holder){(void)holder;abort();}
void ftColl_8007B62C(Fighter_GObj* holder,enum_t status){(void)holder;(void)status;abort();}

// Real GObj lifetimes, Fighter/Item owners, actor VM and captured-item helper.
// Model attachment and shared native leaves are traced controlled boundaries;
// they do not claim real JObj matrices, pickup statistics or stage physics.
static unsigned capture_trace[32],capture_trace_count,capture_stop,capture_mutation;
static unsigned capture_comparisons,capture_destroyed;
static unsigned capture_bone;
static int capture_destroy_type;
static HSD_GObj* capture_holder,*capture_target,*capture_replacement;
static HSD_JObj capture_joints[4],capture_model;
static FighterBone capture_parts[4];
static Vec3 capture_release_position;
Fighter_Part ftYs_SpecialN_GetBoneIndex(Fighter_GObj* object)
{ (void)object;return (Fighter_Part)capture_bone; }
Fighter_Part ftKb_SpecialNYs_80109238(Fighter_GObj* object)
{ (void)object;return (Fighter_Part)capture_bone; }

static void capture_boundary(unsigned id)
{
    CHECK(capture_trace_count<32);capture_trace[capture_trace_count++]=id;
    if(capture_trace_count!=capture_stop)return;
    switch(capture_mutation){
    case 1:HSD_GObjFree(capture_holder);break;
    case 2:HSD_GObjFree(capture_target);break;
    case 3:++((Fighter*)capture_holder->user_data)->x8_spawnNum;break;
    case 4:++((Item*)capture_target->user_data)->x1C;break;
    case 5:++((NativeScriptFighterProgram*)nativeScriptFighter_Program(Ft_Kind_CustomFirst))->identity;break;
    case 6:capture_holder->classifier=HSD_GOBJ_CLASS_STAGE;break;
    case 7:capture_target->classifier=HSD_GOBJ_CLASS_STAGE;break;
    case 8:((Fighter*)capture_holder->user_data)->target_item_gobj=capture_replacement;break;
    }
}
void it_8026C220(Item_GObj* object,HSD_GObj* holder)
{ ((Item*)object->user_data)->xCB0_source_ply=((Fighter*)holder->user_data)->player_id;capture_boundary(1); }
void it_80274F48(Item_GObj* object,s32 item_bone,HSD_GObj* holder,Fighter_Part part)
{ (void)object;CHECK(item_bone==0&&holder==capture_holder&&part<4);capture_boundary(2); }
void Item_80268E5C(HSD_GObj* object,enum_t state,Item_StateChangeFlags flags)
{ CHECK(state==0&&flags==0);Item* item=object->user_data;item->msid=state;
  if(nativeScriptItem_IsAttached(object))CHECK(nativeScriptItem_SelectState(object,state));
  capture_boundary(3); }
void it_802762BC(Item* item){item->ground_or_air=GA_Air;capture_boundary(4);}
void it_802756D0(Item_GObj* object)
{ ((Item*)object->user_data)->x5D4_hitboxes[0].hit.state=HitCapsule_Disabled;capture_boundary(5); }
void it_80274ECC(Item_GObj* object,bool value)
{ ((Item*)object->user_data)->xD09=value;capture_boundary(6); }
void it_80274C88(Item_GObj* object){(void)object;capture_boundary(7);}
void it_8026BD54(Item_GObj* object){(void)object;capture_boundary(8);}
void it_80273408(Item_GObj* object)
{ ((Item*)object->user_data)->x40_vel=(Vec3){0};capture_boundary(9); }
void it_80275070(Item_GObj* object,s32 part)
{ (void)object;CHECK(part==0);capture_boundary(10); }
HSD_JObj* ftLib_80086630(Fighter_GObj* object,Fighter_Part part)
{ return ((Fighter*)object->user_data)->parts[part].joint; }
void lb_8000B1CC(HSD_JObj* joint,Vec3* local,Vec3* world)
{ CHECK(!local);*world=joint->translate;capture_release_position=*world;capture_boundary(11); }
void it_8027B4A4(Fighter_GObj* holder,Item_GObj* object)
{ CHECK(holder==capture_holder&&object==capture_target);capture_boundary(12); }
void it_8027B378(Fighter_GObj* holder,Item_GObj* object,float damage)
{ CHECK(holder==capture_holder&&object==capture_target&&damage==0);capture_boundary(13); }
void it_8027CE44(Item_GObj* object){(void)object;capture_boundary(14);}
void Item_8026A8EC(Item_GObj* object)
{
    capture_destroy_type=((Item*)object->user_data)->destroy_type;
    capture_boundary(15);
    if(nativeScriptItem_IsAttached(object)){
        NativeScriptValue out;
        CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_Destroyed,NULL,0,&out)==NativeScriptVM_Ok);
        ++capture_destroyed;
    }
    // Simulate a destruction callback choosing a new capture target. Real VM
    // destruction dispatch/storage teardown is covered by item_scheduler.
    if(capture_replacement)((Fighter*)capture_holder->user_data)->target_item_gobj=
        ((Fighter*)capture_holder->user_data)->x1A64=capture_replacement;
    HSD_GObjFree(object);
}

static void capture_fixture_pair(bool owned)
{
    capture_holder=fighter(true);capture_target=item();
    Fighter* holder=capture_holder->user_data;Item* target=capture_target->user_data;
    holder->parts=capture_parts;holder->target_item_gobj=holder->x1A64=capture_target;
    for(unsigned bone=0;bone<4;++bone){capture_parts[bone].joint=&capture_joints[bone];
        capture_joints[bone].translate=(Vec3){(float)bone+1,(float)bone+2,(float)bone+3};}
    target->xCC_item_attr=&attributes;capture_target->hsd_obj=&capture_model;
    target->destroy_type=9;target->x40_vel=(Vec3){2,3,4};target->xDD0_flag.b1=true;
    if(!owned){nativeScriptItem_Detach(capture_target);target->native_script_owner=0;}
    capture_trace_count=capture_stop=capture_mutation=0;capture_replacement=NULL;
}

static void capture_item_cases(void)
{
    for(unsigned copied=0;copied<2;++copied)
      for(unsigned owned=0;owned<2;++owned)for(unsigned bone=0;bone<4;++bone)
        for(unsigned clear=0;clear<2;++clear){
            capture_fixture_pair(owned);
            capture_bone=bone;
            if(copied)it_802F2D98(capture_target,capture_holder);
            else it_802F2BFC(capture_target,capture_holder);
            Item reference=*(Item*)capture_target->user_data;
            unsigned attach_trace[32],attach_count=capture_trace_count;
            memcpy(attach_trace,capture_trace,sizeof(attach_trace));capture_trace_count=0;
            if(copied)it_802F2E7C(capture_target,clear);
            else it_802F2CE0(capture_target,clear);
            unsigned release_trace[32],release_count=capture_trace_count;
            memcpy(release_trace,capture_trace,sizeof(release_trace));Vec3 reference_pos=capture_release_position;
            int reference_destroy_type=capture_destroy_type;
            HSD_GObjFree(capture_holder);

            capture_fixture_pair(owned);
            CHECK(nativeCaptureItem_Attach(capture_target,capture_holder,bone));
            Item* target=capture_target->user_data;Fighter* holder=capture_holder->user_data;
            CHECK(capture_trace_count==attach_count&&!memcmp(attach_trace,capture_trace,attach_count*sizeof(unsigned)));
            CHECK(target->xCB0_source_ply==reference.xCB0_source_ply&&target->xDD0_flag.b1==reference.xDD0_flag.b1&&
                target->ground_or_air==reference.ground_or_air&&target->xD09==reference.xD09&&
                !memcmp(&target->x40_vel,&reference.x40_vel,sizeof(Vec3))&&target->msid==reference.msid);
            CHECK(target->atk_victim==capture_holder&&target->grab_victim==capture_holder&&target->native_capture_inert);
            CHECK(target->xBC_itemStateContainer[0].anim_id==-1);
            NativeCaptureItemPair pair;CHECK(nativeCaptureItem_Retain(capture_holder,&pair)&&nativeCaptureItem_Current(&pair,true));
            CHECK(nativeScriptItem_IsAttached(capture_target)==(bool)owned);
            capture_trace_count=0;
            CHECK(nativeCaptureItem_Release(capture_target,bone,clear));
            CHECK(!holder->target_item_gobj&&!holder->x1A64);
            CHECK(capture_trace_count==release_count&&!memcmp(release_trace,capture_trace,release_count*sizeof(unsigned)));
            CHECK(!memcmp(&reference_pos,&capture_release_position,sizeof(Vec3)));
            CHECK(capture_destroy_type==reference_destroy_type&&capture_destroy_type==(clear?0:9));
            CHECK(nativeCaptureItem_Retain(capture_holder,&pair)&&!pair.item.object);
            HSD_GObjFree(capture_holder);++capture_comparisons;
        }
    // Attach stops after a synchronous boundary retires either actor or its
    // retained authored program, before another native continuation is read.
    for(unsigned mode=1;mode<=8;++mode){
        capture_fixture_pair(true);capture_replacement=mode==8?item():NULL;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(Ft_Kind_CustomFirst);
        uint64_t identity=program->identity;
        capture_stop=1;capture_mutation=mode;
        CHECK(!nativeCaptureItem_Attach(capture_target,capture_holder,0));
        CHECK(capture_trace_count==1);
        program->identity=identity;
        if(mode!=1)HSD_GObjFree(capture_holder);
        if(mode!=2)HSD_GObjFree(capture_target);
        if(capture_replacement)HSD_GObjFree(capture_replacement);
    }
    capture_fixture_pair(true);
    CHECK(nativeCaptureItem_Attach(capture_target,capture_holder,0));
    capture_replacement=item();
    CHECK(nativeCaptureItem_Release(capture_target,0,false));
    CHECK(((Fighter*)capture_holder->user_data)->target_item_gobj==capture_replacement&&
        ((Fighter*)capture_holder->user_data)->x1A64==capture_replacement);
    HSD_GObjFree(capture_replacement);HSD_GObjFree(capture_holder);capture_replacement=NULL;

    capture_fixture_pair(true);capture_trace_count=0;
    CHECK(!nativeCaptureItem_Attach(capture_target,capture_holder,4)&&!capture_trace_count);
    CHECK(!nativeCaptureItem_Release(capture_target,0,false)&&!capture_trace_count);
    NativeCaptureItemPair pair;
    ((Fighter*)capture_holder->user_data)->target_item_gobj=capture_holder;
    CHECK(!nativeCaptureItem_Retain(capture_holder,&pair));
    HSD_GObjFree(capture_target);HSD_GObjFree(capture_holder);
    capture_stop=capture_mutation=0;
    comparisons+=capture_comparisons;
}
