#include <melee/gm/native_versus_control_snapshot.h>
#include <melee/gm/gmvs_state_internal.h>
#include <melee/it/itspawn_state_internal.h>
#include <melee/if/ifstatus_state_internal.h>
#include <melee/if/ifnametag_state_internal.h>
#include <melee/if/ifmagnify.h>
ifMagnify ifMagnify_804A1DE0;
u32 it_804D6D10,it_804D6D14,it_804D6D18;
s8 it_804D6D00;
s32 it_804D6D08,it_804D6D0C;
u8 it_804D6D1C[4];
DamageLogEntry it_804A0E70[15];
Item_FtTrack Item_804A0CCC;
S32Vec3 Item_804A0E24;
HSD_ObjAllocUnk Item_804A0C64;
int ftData_CharacterReferenceCounts[Ft_Kind_RuntimeCapacity];
Element_803F9628 ifStatus_803F9628[8];
IfTimeState if_time_state;
IfStockState if_stock_state;
GmScoreState gm_score_state;
HSD_CObj* devtext_cobj;
CrowdSFX_UnkStruct un_804A2F08;
CrowdSFX_UnkStruct* un_804D7050;
CrowdConfig* gCrowdConfig;
static void forbidden_vs_callback(void) {abort();}
static void forbidden_status_callback(s32 argument) {(void)argument;abort();}
static void test_versus_controls(void)
{
    camera_idle=true;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,camera_objects,sizeof(camera_objects),camera_slots,camera_free,
                              8,sizeof(HSD_GObj),_Alignof(HSD_GObj),901));
    NativeMatchPoolReference object_ref;HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&object_ref);CHECK(object);
    object->classifier=0xE;object->p_link=0xE;object->obj_kind=HSD_GObj_JObjKind;
    HSD_JObj* joint=hsdNew(&stage_joint_class);CHECK(joint);object->hsd_obj=joint;
    static u8 kinds[3][2]={{1,2},{3,4},{5,6}};
    static u16 weights[3][2]={{2,5},{3,8},{4,9}};
    memset(&vs_scene_controller,0,sizeof(vs_scene_controller));vs_scene_controller.frame_count=200;
    vs_scene_controller.start.on_frame_start=forbidden_vs_callback;
    memset(&ifStatus_HudInfo,0,sizeof(ifStatus_HudInfo));ifStatus_HudInfo.players[0].HUD_parent_entity=object;
    ifStatus_HudInfo.players[0].jobjs[0]=joint;hud_damage_display_count=2;hud_rounding_toggle=1;
    it_804A0E30=(RandomItemSpawner){.x0=120,.pick_table={2,kinds[0],5,weights[0]},.x18=3};
    it_804A0E50=(ItemPickTable){2,kinds[1],8,weights[1]};it_804A0E60=(ItemPickTable){2,kinds[2],9,weights[2]};
    if_time_state=(IfTimeState){.countdown_timer=object,.countdown_seconds=3};
    gm_score_state=(GmScoreState){.flags={.initialized=1,.track_leaders=1},.top_score=3};
    gm_score_state.flags.players[0].leader=1;
    memset(&if_stock_state,0,sizeof(if_stock_state));
    if_stock_state.hud.player[0].stocks=1;
    if_stock_state.fall_queue.x108=object;
    if_stock_indicators[1]=object;
    nativeVersusControl_BeginScene();
    devtext_cobj=NULL;
    un_804A2F08.x4=29;
    it_804D6D10=10;
    Item_804A0C64=(HSD_ObjAllocUnk){.x0=1,.x18=3};
    memset(ftData_CharacterReferenceCounts,0,sizeof(ftData_CharacterReferenceCounts));
    ftData_CharacterReferenceCounts[Ft_Kind_Popo]=1;
    memset(ifStatus_803F9628,0,sizeof(ifStatus_803F9628));
    ifStatus_803F9628[0].x0=object;
    ifStatus_803F9628[0].x18=forbidden_status_callback;
    ifStatus_803F9628[0].x20=17;
    hud_hidden=0;nametag_hidden=1;
    for(unsigned i=0;i<Gm_Player_NumMax;++i) {
        nametag_force_visible[i]=1;
        ifAll_804A0FD8.x18[i]=(Vec3){(float)i,2.0f,3.0f};
    }
    NativeVersusControlSnapshot saved,after,bad;size_t bytes;
    CHECK(nativeVersusControl_Save(&saved,sizeof(saved),&bytes));
    CHECK(bytes==sizeof(saved));
    vs_scene_controller.frame_count=777;vs_scene_controller.fighters[0].respawn_timer=55;
    it_804A0E30.x0=4;it_804A0E50.total_weight=1;kinds[0][0]=8;weights[2][1]=14;
    ifStatus_HudInfo.players[0].damage_percent=140;hud_rounding_toggle=0;
    it_804D6D10=11;
    Item_804A0C64.x0=2;Item_804A0C64.x18=4;
    ftData_CharacterReferenceCounts[Ft_Kind_Popo]=2;
    ifStatus_803F9628[0].x12.x2=1;ifStatus_803F9628[0].x18=NULL;
    ifStatus_803F9628[0].x20=42;
    hud_hidden=1;nametag_hidden=0;memset(nametag_force_visible,0,sizeof(nametag_force_visible));
    ifAll_804A0FD8.x18[3].y=777.0f;
    if_time_state.countdown_timer=NULL;if_time_state.countdown_seconds=5;
    gm_score_state.flags.players[0].leader=0;gm_score_state.top_score=7;
    if_stock_state.hud.player[0].stocks=7;if_stock_state.fall_queue.x108=NULL;
    if_stock_indicators[1]=NULL;
    HSD_CObj* lazy_camera=hsdNew(&stage_camera_class);CHECK(lazy_camera);
    devtext_cobj=lazy_camera;
    un_804A2F08.x4=37;
    CHECK(nativeVersusControl_Load(&saved,sizeof(saved)));
    CHECK(vs_scene_controller.frame_count==200 && vs_scene_controller.fighters[0].respawn_timer==0 && it_804A0E30.x0==120);
    CHECK(it_804A0E50.total_weight==8 && kinds[0][0]==1 && weights[2][1]==9);
    CHECK(it_804D6D10==10);
    CHECK(Item_804A0C64.x0==1 && Item_804A0C64.x18==3);
    CHECK(ftData_CharacterReferenceCounts[Ft_Kind_Popo]==1);
    CHECK(!ifStatus_803F9628[0].x12.x2 && ifStatus_803F9628[0].x18==forbidden_status_callback && ifStatus_803F9628[0].x20==17);
    CHECK(!hud_hidden && nametag_hidden && ifAll_804A0FD8.x18[3].y==2.0f);
    CHECK(if_time_state.countdown_timer==object && if_time_state.countdown_seconds==3);
    CHECK(gm_score_state.flags.players[0].leader==1 && gm_score_state.top_score==3);
    CHECK(if_stock_state.hud.player[0].stocks==1 && if_stock_state.fall_queue.x108==object);
    CHECK(if_stock_indicators[1]==object);
    CHECK(!devtext_cobj);
    CHECK(un_804A2F08.x4==29);
    for(unsigned i=0;i<Gm_Player_NumMax;++i)CHECK(nametag_force_visible[i]==1);
    CHECK(ifStatus_HudInfo.players[0].damage_percent==0 && hud_rounding_toggle==1);
    CHECK(nativeVersusControl_Save(&after,sizeof(after),&bytes));CHECK(!memcmp(&after,&saved,sizeof(saved)));
#define BAD(change) do {bad=saved;change;CHECK(!nativeVersusControl_Validate(&bad,sizeof(bad))); \
    CHECK(!nativeVersusControl_Load(&bad,sizeof(bad)));CHECK(nativeVersusControl_Save(&after,sizeof(after),&bytes)); \
    CHECK(!memcmp(&after,&saved,sizeof(saved)));}while(0)
    BAD(++bad.epoch);BAD(bad.versus.start.on_frame_start=NULL);BAD(bad.spawner.pick_table.kinds=(void*)1);
    BAD(bad.fighter_references[Ft_Kind_Popo]=-1);
    BAD(bad.hud_hidden=2);BAD(bad.nametag_hidden=2);BAD(bad.nametag_force_visible[4]=2);
    BAD(bad.message_objects[0].slot=UINT32_MAX);
    BAD(bad.timer_objects[1].slot=UINT32_MAX);
    BAD(bad.score_state.flags.players[5].leader=2);
    BAD(bad.stock_object_refs[0].slot=UINT32_MAX);
    BAD(bad.stock_hud_objects[6+1+16+131].slot=UINT32_MAX);
    BAD(bad.debug_text_camera_type=&stage_camera_class);
    BAD(bad.objects[0][0].slot=UINT32_MAX);BAD(bad.joint_types[0][0]=(void*)1);
    BAD(++bad.joints[0][0].offset);BAD(++bad.picks[1].count);
#undef BAD
    bad=saved;++bad.objects[0][0].generation;
    CHECK(nativeVersusControl_Validate(&bad,sizeof(bad)));CHECK(!nativeVersusControl_Load(&bad,sizeof(bad)));
    CHECK(nativeVersusControl_Save(&after,sizeof(after),&bytes));CHECK(!memcmp(&after,&saved,sizeof(saved)));
    CHECK(!nativeVersusControl_Save(&vs_scene_controller,sizeof(saved),&bytes));
    CHECK(!nativeVersusControl_Save(&after,sizeof(after),(size_t*)joint));
    CHECK(!nativeVersusControl_Save(kinds[0],sizeof(saved),&bytes));
    ifStatus_HudInfo.players[0].HUD_parent_entity=NULL;ifStatus_HudInfo.players[0].jobjs[0]=(void*)1;
    CHECK(nativeVersusControl_Save(&after,sizeof(after),&bytes));CHECK(nativeVersusControl_Load(&after,sizeof(after)));
    CHECK(ifStatus_HudInfo.players[0].jobjs[0]==(void*)1);
    nativeVersusControl_EndScene();CHECK(!nativeVersusControl_Load(&after,sizeof(after)));
    hsdFreeMemPiece(lazy_camera,sizeof(*lazy_camera));
    memset(&ifStatus_HudInfo,0,sizeof(ifStatus_HudInfo));hsdFreeMemPiece(joint,sizeof(*joint));camera_idle=false;
    fprintf(stderr,"versus-controls bytes=%zu owner restoration/malformed/dormant/retirement checks passed\n",sizeof(saved));
}
