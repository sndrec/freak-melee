#include <melee/ft/ftcommon.h>
#include <melee/mp/mpdat.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpcoll.h>
#include <melee/lb/types.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdevice.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/kinds/ftCommon/ftCo_Bury.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/gr/forward.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00CE.h>
#include <sysdolphin/baselib/mobj.h>
#include <melee/lb/lbrefract.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool ok, const char* message)
{
    if (!ok) { fprintf(stderr, "%s\n", message); exit(1); }
}
static void put32(u8* p, u32 value)
{
    p[0] = value >> 24; p[1] = value >> 16;
    p[2] = value >> 8; p[3] = value;
}

static void check_runtime(MapCollData* map)
{
    mpLibLoad(map);
    require(mpLib_8004D164() == map, "Collision initialization lost native map");
    CollVtx* vertices = mpGetGroundCollVtx();
    for (int i = 0; i < map->vert_count; ++i) {
        require(vertices[i].pos.x == map->verts[i].x &&
                vertices[i].pos.y == map->verts[i].y,
                "Collision initialization changed unscaled vertices");
    }
    require(OSCheckHeap(HSD_GetHeap()) >= 0, "Collision initialization damaged heap");
}

static unsigned wind_calls;
static bool wind_active;
static bool fixture_wind(Ground_GObj* ground, Fighter_GObj* fighter, Vec3* wind)
{
    require(ground->user_data == fighter, "Wind callback lost its native pointers");
    ++wind_calls;
    *wind = (Vec3) { 0.25f, -0.5f, 1 };
    return wind_active;
}

static int physics_order;
static void* physics_actor;
static unsigned physics_retirements;
static void retire_physics_actor(void* actor)
{
    require(actor == physics_actor, "Physics fixture retired another actor");
    ++physics_retirements;
}
static void fixture_physics(Fighter_GObj* object)
{
    Fighter* fp = object->user_data;
    require(physics_order == 0 && fp->x2064_ledgeCooldown == 0 &&
        fp->capture_timer == 0 && fp->bury_timer_1 == 0,
        "Physics callback ran before cooldown updates");
    physics_order = 1;
    fp->self_vel = (Vec3) { 1, 2, 0 };
    fp->x74_self_accel = (Vec3) { 0.5f, 0.25f, 0 };
}
static bool fixture_physics_wind(Ground_GObj* ground, Fighter_GObj* object, Vec3* wind)
{
    Fighter* fp = object->user_data;
    require(physics_order == 1 && fp->cur_pos.x == 1.5f && fp->cur_pos.y == 2.25f,
        "Wind ran before physics position integration");
    physics_order = 2;
    return fixture_wind(ground, object, wind);
}
static void fixture_hitlag(Fighter_GObj* object)
{
    Fighter* fp = object->user_data;
    require(physics_order == 2, "Hitlag callback ordering changed");
    physics_order = 3;
    fp->cur_pos.x += 10;
}

static void check_physics_phase(void)
{
    struct ftCommonData common = { 0 };
    struct ftCommonData* saved = p_ftCommonData;
    p_ftCommonData = &common;
    Fighter fp = { .ground_or_air = GA_Air, .x2064_ledgeCooldown = 1,
        .capture_timer = 1, .bury_timer_1 = 1,
        .phys_cb = fixture_physics, .hitlag_cb = fixture_hitlag };
    // Callback continuation requires a live registered GObj. A stack-only
    // object is correctly rejected by the production lifetime check.
    HSD_GObj* object = GObj_Create(HSD_GOBJ_CLASS_FIGHTER,
                                  HSD_GOBJ_PLINK_FIGHTER, 0);
    require(object != NULL, "Cannot allocate physics fixture fighter root");
    physics_actor = &fp;
    physics_retirements = 0;
    GObj_InitUserData(object, 4, retire_physics_actor, &fp);
    HSD_GObj ground = { .user_data = object };
    long before = OSCheckHeap(HSD_GetHeap());
    ftCo_800C06C0();
    ftCo_800C06E8(&ground, BuryType_Unk0, fixture_physics_wind);
    wind_active = true; physics_order = 0;
    Fighter_UpdatePosition(object);
    require(physics_order == 3 && fp.cur_pos.x == 11.75f &&
        fp.cur_pos.y == 1.75f && fp.cur_pos.z == 1,
        "Combined physics/callback/wind position changed");
    fp.x2219_b5 = 1; fp.x2064_ledgeCooldown = 1; fp.capture_timer = 1;
    fp.bury_timer_1 = 1; physics_order = 2;
    Fighter_UpdatePosition(object);
    require(physics_order == 3 && fp.cur_pos.x == 21.75f &&
        fp.cur_pos.y == 1.75f && fp.cur_pos.z == 1 &&
        fp.x2064_ledgeCooldown == 1 && fp.capture_timer == 1 && fp.bury_timer_1 == 1,
        "Hitlag advanced cooldown, physics or wind, or skipped its callback");
    ftCo_800C06C0();
    fp = (Fighter) { .ground_or_air = GA_Ground, .facing_dir = 1, .x2092 = 1 };
    fp.coll_data.floor.index = -1;
    fp.coll_data.floor.normal = (Vec3) { 0.6f, 0.8f, 0 };
    common.x4C8 = 2; common.x4D0 = 0.5f; common.x4D4 = 1;
    fp.x2219_b5 = 1;
    Fighter_UpdatePosition(object);
    require(fp.x2092 == 0 && fp.cur_pos.x == -0.4f && fp.cur_pos.y == 0.3f,
        "Ground push did not run during hitlag");
    fp.hurt_capsules_len = 2;
    fp.hurt_capsules[0].capsule.skip_update_pos = true;
    fp.hurt_capsules[1].capsule.skip_update_pos = true;
    ftColl_8007AF28(object);
    require(!fp.hurt_capsules[0].capsule.skip_update_pos &&
        !fp.hurt_capsules[1].capsule.skip_update_pos,
        "Physics completion did not invalidate hurt capsule positions");
    require(OSCheckHeap(HSD_GetHeap()) == before, "Physics phase allocated");
    HSD_GObjFree(object);
    require(physics_retirements == 1, "Physics fixture actor was not retired");
    physics_actor = NULL;
    p_ftCommonData = saved;
    puts("Original combined physics phase ordering, hitlag, ground push and capsule reset passed");
}

static void check_environment_wind(void)
{
    Fighter fp = { 0 };
    HSD_GObj fighter = { .user_data = &fp };
    HSD_GObj ground = { .user_data = &fighter };
    Vec3 wind = { 99, 99, 99 };
    long before = OSCheckHeap(HSD_GetHeap());
    ftCo_800C06C0();
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind.x == 0 && wind.y == 0 && wind.z == 0,
        "Empty wind registry did not clear output");
    ftCo_800C06E8(&ground, BuryType_Unk2, fixture_wind);
    wind_active = true; wind_calls = 0;
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind.x == 0.25f && wind.y == -0.5f && wind.z == 1 && wind_calls == 1,
        "Registered wind did not contribute its displacement");
    fp.x2224_b4 = 1;
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind.x == 0 && wind.y == 0 && wind.z == 0 && wind_calls == 1,
        "Wind immunity invoked callback or retained displacement");
    fp.x2224_b4 = 0; fp.bury_timer_2 = 1;
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind_calls == 1 && wind.x == 0, "Environment cooldown did not suppress wind");
    fp.bury_grkind = Gr_Kind_Kongo; fp.bury_timer_1 = 1;
    ftCo_800C0A98(&fighter);
    require(fp.bury_timer_1 == 0 && fp.bury_timer_2 == 0,
        "Original environment cooldown did not expire");
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind_calls == 2 && wind.x == 0.25f, "Wind did not resume after cooldown");
    fp.motion_id = ftCo_MS_BarrelWait;
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind_calls == 2 && wind.x == 0, "Barrel state accepted type-2 wind");
    fp.motion_id = 0; fp.x1A6A = 0x20;
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind_calls == 2, "Capture flag accepted type-2 wind");
    fp.x1A6A = 0; fp.x2224_b2 = 1;
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind_calls == 2, "Environment immunity accepted type-2 wind");
    fp.x2224_b2 = 0; wind_active = false;
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind_calls == 3 && wind.x == 0 && wind.y == 0 && wind.z == 0,
        "Inactive wind callback leaked its output");
    ftCo_800C06C0();
    ftColl_GetWindOffsetVec(&fighter, &wind);
    require(wind_calls == 3 && ft_804D6578.x0 == 0,
        "Stage reset retained wind callback");
    require(OSCheckHeap(HSD_GetHeap()) == before, "Wind evaluation allocated");
    puts("Original wind registration, immunity, cooldown, callback and reset passed");
}

static void check_position_integration(void)
{
    struct ftCommonData common = { 0 };
    struct ftCommonData* saved = p_ftCommonData;
    p_ftCommonData = &common;
    common.x204_knockbackFrameDecay = 0.5f;
    common.x3E8_shieldKnockbackFrameDecay = 0.5f;
    Fighter fp = { .ground_or_air = GA_Air, .gr_vel = 2,
        .xE4_ground_accel_1 = 0.25f, .xE8_ground_accel_2 = -0.5f,
        .self_vel = { 1, 2, 3 }, .x74_self_accel = { 0.5f, -1, 2 },
        .x8c_kb_vel = { 2, 0, 0 }, .x98_atk_shield_kb = { 1, 0, 0 },
        .xF8_playerNudgeVel = { 0.25f, -0.25f } };
    long before = OSCheckHeap(HSD_GetHeap());
    ftCommon_IntegrateMovement(&fp);
    require(fp.gr_vel == 1.75f && fp.xE4_ground_accel_1 == 0 &&
        fp.xE8_ground_accel_2 == 0 && fp.x74_self_accel.x == 0 &&
        fp.x74_self_accel.y == 0 && fp.x74_self_accel.z == 0,
        "Physics tick did not consume acceleration exactly once");
    require(fp.cur_pos.x == 3.75f && fp.cur_pos.y == 1 && fp.cur_pos.z == 4.75f,
        "Physics tick velocity decay, nudge or position accumulation changed");
    fp = (Fighter) { .ground_or_air = GA_Air,
        .x8c_kb_vel = { 0, 0.25f, 0 }, .x98_atk_shield_kb = { 0, 0.25f, 0 } };
    ftCommon_IntegrateMovement(&fp);
    require(fp.x8c_kb_vel.y == 0 && fp.x98_atk_shield_kb.y == 0.25f &&
        fp.cur_pos.y == 0.25f, "Original invisible-ceiling behavior changed");
    fp = (Fighter) { .ground_or_air = GA_Air, .self_vel = { 4, 2, 0 },
        .xA4_unk_vel = { 2, 0, 0 } };
    fp.dmg.x1948 = 2; fp.dmg.x194C = 2;
    ftCommon_IntegrateMovement(&fp);
    require(fp.cur_pos.x == 2 && fp.cur_pos.y == 0 && fp.dmg.x194C == 1,
        "Damage velocity interpolation first tick changed");
    ftCommon_IntegrateMovement(&fp);
    require(fp.cur_pos.x == 5 && fp.cur_pos.y == 1 && fp.dmg.x1948 == 0,
        "Damage velocity interpolation completion changed");
    fp = (Fighter) { .ground_or_air = GA_Air, .self_vel = { 1, 2, 0 },
        .xD4_unk_vel = { 3, 4, 0 } };
    fp.x2222_b6 = 1; fp.throw_flags_b2 = 1;
    ftCommon_IntegrateMovement(&fp);
    require(fp.cur_pos.x == 4 && fp.cur_pos.y == 6 && !fp.throw_flags_b2 &&
        fp.xD4_unk_vel.x == 0 && fp.xD4_unk_vel.y == 0,
        "Animation-gated movement release changed");
    require(OSCheckHeap(HSD_GetHeap()) == before, "Position integration allocated");
    p_ftCommonData = saved;
    puts("Original position integration, decay, damage interpolation and movement release passed");
}

static void check_ground_projection(void)
{
    Fighter fighter = { .gr_vel = 2, .xE4_ground_accel_1 = 1 };
    HSD_GObj object = { .user_data = &fighter };
    fighter.coll_data.floor.index = 0;
    fighter.coll_data.floor.flags = 12;
    fighter.coll_data.floor.normal = (Vec3) { 0.6f, 0.8f, 0 };
    long before = OSCheckHeap(HSD_GetHeap());
    ftCommon_SetSelfMovementFromGroundedMovement(&object);
    require(fabsf(fighter.xE4_ground_accel_1 - 0.1f) < 0.000001f &&
        fabsf(fighter.x74_self_accel.x - 0.08f) < 0.000001f &&
        fabsf(fighter.x74_self_accel.y + 0.06f) < 0.000001f &&
        fighter.self_vel.x == 1.6f && fighter.self_vel.y == -1.2f,
        "Original surface friction or slope projection changed");
    fighter.kind = Ft_Kind_Popo;
    fighter.xE4_ground_accel_1 = 1;
    ftCommon_SetSelfMovementFromGroundedMovement(&object);
    require(fighter.xE4_ground_accel_1 == 1 && fighter.x74_self_accel.x == 0.8f,
        "Popo lost original surface friction exemption");
    fighter.kind = Ft_Kind_Nana;
    ftCommon_SetSelfMovementFromGroundedMovement(&object);
    require(fighter.xE4_ground_accel_1 == 1, "Nana lost original surface friction exemption");
    fighter.kind = Ft_Kind_Mario;
    fighter.coll_data.floor.index = -1;
    ftCommon_SetSelfMovementFromGroundedMovement(&object);
    require(fighter.xE4_ground_accel_1 == 1, "Missing floor did not use neutral friction");
    fighter.coll_data.floor.index = 0;
    ftCommon_SetSelfMovementFromGroundedMovement_NoFriction(&object);
    require(fighter.xE4_ground_accel_1 == 1 && fighter.x74_self_accel.y == -0.6f &&
        fighter.self_vel.z == 0 && fighter.x74_self_accel.z == 0,
        "Friction-free grounded projection changed");
    require(OSCheckHeap(HSD_GetHeap()) == before, "Ground projection allocated");
    puts("Original surface friction, slope projection and Ice Climbers exemptions passed");
}

static void check_movement(const ftCo_DatAttrs* attributes)
{
    Fighter fighter = { .co_attrs = *attributes };
    long before = OSCheckHeap(HSD_GetHeap());
    ftCommon_FallBasic(&fighter);
    require(fighter.self_vel.y == -attributes->gravity, "Original gravity step changed");
    for (int frame = 0; frame < 120; ++frame) ftCommon_FallBasic(&fighter);
    require(fighter.self_vel.y == -attributes->terminal_velocity, "Original terminal fall speed changed");
    ftCommon_FallFast(&fighter);
    require(fighter.self_vel.y == -attributes->fast_fall_velocity, "Original fast fall speed changed");
    fighter.self_vel.x = attributes->air_drift_max * 2;
    ftCommon_ClampAirDrift(&fighter);
    require(fighter.self_vel.x == attributes->air_drift_max, "Original positive air drift clamp changed");
    fighter.self_vel.x = -attributes->air_drift_max * 2;
    ftCommon_ClampAirDrift(&fighter);
    require(fighter.self_vel.x == -attributes->air_drift_max, "Original negative air drift clamp changed");
    for (int sign = -1; sign <= 1; sign += 2) {
        fighter.gr_vel = sign * attributes->ground_friction * 0.5f;
        ftCommon_CalcGroundAccel_Deaccel(&fighter, attributes->ground_friction);
        require(fighter.gr_vel + fighter.xE4_ground_accel_1 == 0,
                "Original grounded friction overshot zero");
        fighter.self_vel.x = sign * attributes->aerial_friction * 0.5f;
        ftCommon_CalcSelfAccel_DeaccelAir(&fighter);
        require(fighter.self_vel.x + fighter.x74_self_accel.x == 0,
                "Original aerial friction overshot zero");
    }
    fighter.gr_vel = 0;
    fighter.input.lstick[0].x = 0.5f;
    ftCommon_CalcGroundAccel_AccelToLStickX(&fighter, 0.3f, 0.2f, 1);
    require(fighter.xE4_ground_accel_1 == 0.1f, "Original stick acceleration changed");
    fighter.gr_vel = 0.45f;
    ftCommon_CalcGroundAccel_AccelToLStickX(&fighter, 0.3f, 0.2f, 1);
    require(fighter.gr_vel + fighter.xE4_ground_accel_1 == 0.5f,
            "Original stick target speed overshot");
    fighter.input.lstick[0].x = 0.2f;
    ftCommon_CalcGroundAccel_AccelToLStickX(&fighter, 0.3f, 0.2f, 1);
    require(fighter.gr_vel + fighter.xE4_ground_accel_1 == 0,
            "Original stick threshold behavior changed");
    require(OSCheckHeap(HSD_GetHeap()) == before, "Original movement helpers allocated");
    puts("Original movement gravity, terminal/fast fall, drift, friction and stick acceleration passed");
}

static void check_common_attributes(void)
{
    u8 bytes[0x1A1] = { [2] = 1, [3] = 0xA1, [6] = 1, [7] = 0x81 };
    u8* data = bytes + 32;
    data[0x5B] = 2;
    data[0x88] = 0x42; data[0x89] = 0xC4;
    data[0xA4] = data[0xA5] = data[0xA6] = data[0xA7] = 255;
    data[0x180] = 5;
    HSD_Dat dat;
    ftCo_DatAttrs attributes;
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK &&
        ftData_DecodeCommonAttrs(&dat, 0, &attributes) == HSD_DAT_OK &&
        attributes.max_jumps == 2 && attributes.weight == 98 &&
        attributes.unused_0 == -1 && attributes.weight_independent_throws_mask == 5,
        "Common fighter attribute scalar types changed");
    dat.data_size = 0x180;
    require(ftData_DecodeCommonAttrs(&dat, 0, &attributes) == HSD_DAT_DESCRIPTOR &&
        attributes.weight == 98, "Truncated attributes changed output");
    dat.data_size = 0x181;
    data[0] = 0x7F; data[1] = 0x80;
    require(ftData_DecodeCommonAttrs(&dat, 0, &attributes) == HSD_DAT_DESCRIPTOR &&
        attributes.weight == 98, "Nonfinite fighter attribute was accepted");
    puts("Native common fighter attributes preserve floats, integers and final byte");
}

static void check_material_utilities(void)
{
    HSD_TExpTevDesc stage = { 0 };
    HSD_TExp occupied[7] = { 0 };
    for (unsigned i = 0; i < 7; ++i) {
        occupied[i].cnst.reg = i;
        occupied[i].cnst.next = i < 6 ? &occupied[i + 1] : NULL;
    }
    occupied[1].cnst.next = NULL;
    HSD_MObj material = { .tevdesc = &stage, .texp = occupied };
    require(lbGetFreeColorRegister(0, &material, NULL) == 2 &&
        lb_8000CCA4(2) == GX_TEV_KCSEL_K2,
        "Original material register allocation lost a nonboolean index");
    occupied[1].cnst.next = &occupied[2];
    require(lbGetFreeColorRegister(0, &material, NULL) == -1,
        "Original material allocator failed to report register exhaustion");
    Fighter fighter = { 0 };
    require(ftCo_800C0658(&fighter) == &fighter.x488, "Wrong default color overlay");
    // The selector only tests this opaque token; no pointee is accessed.
    fighter.x408.x28_colanim.ptr = (void*) ((uintptr_t) 1 << 32);
    require(ftCo_800C0658(&fighter) == &fighter.x408,
        "Color overlay selector truncated a native pointer");
    puts("Original material register allocation, exhaustion and native overlay selection passed");
}

static void check_refraction_texels(void)
{
    u8 pixels[256] = { 0 };
    lbRefract_CallbackData cb;
    u32 r, g, b, a;
    require(lbRefract_8002219C(&cb, pixels, GX_TF_RGB565, 8, 8) == 0,
        "Cannot initialize RGB565 refraction texture");
    cb.callback0(&cb, 4, 4, 255, 128, 0, 255);
    // Tile (1,1) starts at byte 96. Packed RGB565 is FC00, regardless of CPU endian.
    require(pixels[96] == 0xFC && pixels[97] == 0 && pixels[95] == 0 && pixels[98] == 0,
        "Refraction RGB565 texture bytes or tile addressing changed");
    cb.callback1(&cb, 4, 4, &r, &g, &b, &a);
    require(r == 7 && g == 3 && b == 0 && a == 255,
        "Original RGB565 callback interpretation changed");
    memset(pixels, 0, sizeof(pixels));
    require(lbRefract_8002219C(&cb, pixels, GX_TF_IA8, 8, 8) == 0,
        "Cannot initialize IA8 refraction texture");
    cb.callback0(&cb, 4, 4, 0, 0, 73, 91);
    cb.callback1(&cb, 4, 4, &r, &g, &b, &a);
    require(pixels[96] == 91 && pixels[97] == 73 && b == 73 && a == 91,
        "Refraction IA8 callback layout changed");
    memset(pixels, 0, sizeof(pixels));
    require(lbRefract_8002219C(&cb, pixels, GX_TF_RGBA8, 8, 8) == 0,
        "Cannot initialize RGBA8 refraction texture");
    cb.callback0(&cb, 4, 4, 11, 22, 33, 44);
    cb.callback1(&cb, 4, 4, &r, &g, &b, &a);
    require(pixels[192] == 44 && pixels[193] == 11 &&
        pixels[224] == 22 && pixels[225] == 33 && r == 11 && g == 22 && b == 33 && a == 44,
        "Refraction RGBA8 planes changed");
    puts("Native refraction texture callback types, tile addressing and byte order passed");
}

static void check_part_tables(void)
{
    u8 bytes[128] = { 0 };
    u8* data = bytes + 32;
    put32(bytes, sizeof(bytes)); put32(bytes + 4, 84); put32(bytes + 8, 3);
    put32(bytes + 116, 0); put32(bytes + 120, 4); put32(bytes + 124, 72);
    put32(data, 12); put32(data + 4, 16); put32(data + 8, 3);
    data[12] = 0; data[13] = FTPART_INVALID; data[14] = 1;
    memset(data + 16, FTPART_INVALID, FT_COMMON_PART_COUNT);
    data[16] = 0; data[17] = 2;
    put32(data + 72, 80); put32(data + 76, 1);
    data[80] = 1; data[81] = 3; data[82] = 4; data[83] = 5;
    HSD_Dat dat;
    FighterPartsTable parts;
    u8 joint_map[3], semantic_map[FT_COMMON_PART_COUNT];
    struct Fighter_804D6540_t omissions;
    struct Fighter_804D6540_x0_t records[1];
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK &&
        ftData_DecodeParts(&dat, 0, &parts, joint_map, 3, semantic_map) == HSD_DAT_OK &&
        parts.parts_num == 3 && joint_map[1] == FTPART_INVALID && semantic_map[1] == 2 &&
        ftData_DecodePartOmissions(&dat, 72, &omissions, records, 1, 3) == HSD_DAT_OK &&
        omissions.x4 == 1 && records[0].x0 == 1 && records[0].x3 == 5,
        "Fighter part mapping or omission record changed");
    joint_map[0] = 2;
    require(data[12] == 0 && ftData_DecodeParts(&dat, 0, &parts, joint_map, 2,
        semantic_map) == HSD_DAT_CAPACITY, "Fighter map storage is not independent or bounded");
    data[17] = 3;
    require(ftData_DecodeParts(&dat, 0, &parts, joint_map, 3, semantic_map) == HSD_DAT_DESCRIPTOR,
        "Invalid mapped joint accepted");
    require(ftData_DecodePartOmissions(&dat, 72, &omissions, records, 0, 3) == HSD_DAT_CAPACITY &&
        ftData_DecodePartOmissions(&dat, 72, &omissions, records, 1, 1) == HSD_DAT_DESCRIPTOR,
        "Invalid omission capacity or slot accepted");
    data[17] = 2;
    require(ftData_DecodeParts(&dat, 0, &parts, joint_map, 3, semantic_map) == HSD_DAT_OK,
        "Cannot restore part fixture");
    FighterPartsTable* tables[Ft_Kind_StockCount] = { &parts, &parts };
    struct Fighter_804D6540_t* omitted_tables[Ft_Kind_StockCount] = { &omissions };
    ftPartsTable = tables;
    Fighter_804D6540 = omitted_tables;
    Fighter fighter = { .kind = Ft_Kind_Mario };
    require(ftParts_GetBoneIndex(&fighter, FtPart_TransN) == 2 &&
        ftPartsRemap(1, 0, 2) == 2 && ftPartsRemap(1, 0, 1) == FTPART_INVALID &&
        ftPartsRemap(1, 0, 3) == FTPART_INVALID &&
        ftParts_8007506C(Ft_Kind_Mario, 1) == 1 &&
        ftParts_8007506C(Ft_Kind_Mario, 0) == 0,
        "Original fighter mapping or omitted-slot lookup differs");
    struct Fighter_804D6540_x0_t many_records[32] = { 0 };
    for (unsigned i = 0; i < 32; ++i) many_records[i].x0 = i;
    struct Fighter_804D6540_t many = { many_records, 32 };
    omitted_tables[0] = &many;
    require(ftParts_8007506C(Ft_Kind_Mario, 31) == 0x80000000U,
        "Original omitted-slot lookup lost its highest mask bit");
    ftPartsTable = NULL;
    Fighter_804D6540 = NULL;
    puts("Fighter part maps, omission records and storage bounds passed");
}

static void check_floor_queries(void)
{
    Vec2 vertices[] = { { -10, 0 }, { 10, 0 }, { -10, 5 }, { 10, 5 } };
    MapLine lines[] = {
        { 0, 1, -1, -1, -1, -1, CollLine_Floor, 7 },
        { 2, 3, -1, -1, -1, -1, CollLine_Floor, 9 },
    };
    MapJoint joint = { .floor_count = 2, .left_bound = -10, .right_bound = 10,
        .top_bound = 5, .vtx_count = 4 };
    MapCollData map = { .verts = vertices, .vert_count = 4, .lines = lines,
        .line_count = 2, .floor_count = 2, .joints = &joint, .joint_count = 1 };
    check_runtime(&map);
    long before = OSCheckHeap(HSD_GetHeap());
    Vec3 hit, normal; int line; u32 flags;
    mpFloorGetLeft(1, &hit);
    require(hit.x == -10 && hit.y == 5, "Native floor left endpoint is wrong");
    mpFloorGetRight(1, &hit);
    require(hit.x == 10 && hit.y == 5, "Native floor right endpoint is wrong");
    require(mpCheckFloor(0, 10, 0, -10, 0, &hit, &line, &flags, &normal,
                -1, -1, -1, NULL, NULL) && line == 1 && hit.y == 5 &&
                hit.x == 0 && flags == 9 && normal.x == 0 && normal.y == 1,
            "Floor sweep missed nearest nonzero-index floor");
    require(mpCheckFloor(0, 10, 0, -10, 0, &hit, &line, &flags, &normal,
                1, -1, -1, NULL, NULL) && line == 0 && hit.y == 0 && flags == 7,
            "Skipped floor was not excluded");
    require(!mpCheckFloor(20, 10, 20, -10, 0, &hit, &line, &flags, &normal,
                -1, -1, -1, NULL, NULL), "Out-of-bounds sweep hit a floor");
    require(!mpCheckFloor(0, -10, 0, 10, 0, &hit, &line, &flags, &normal,
                -1, -1, -1, NULL, NULL), "Upward sweep hit horizontal floor");
    require(OSCheckHeap(HSD_GetHeap()) == before, "Floor queries allocated or damaged heap");
    for (int run = 0; run < 3; ++run) {
        CollData body;
        memset(&body, 0xA5, sizeof(body));
        body.cur_pos = (Vec3) { 0, run == 2 ? 100 : 10, 0 };
        mpColl_80041EE4(&body);
        require(!body.x34_flags.b5 && body.prev_env_flags == 0 &&
                    body.env_flags == 0 && body.prev_pos.y == body.cur_pos.y,
                "Reused collision body retained prior query/collision flags");
        mpColl_SetECBSource_Fixed(&body, NULL, 8, 0, 4, -4);
        body.cur_pos.y = run == 2 ? -100 : -5;
        require(mpColl_800471F8(&body) && body.floor.index == 1 &&
            body.cur_pos.x == 0 && body.cur_pos.y == 5.0001F &&
            (body.env_flags & Collide_FloorMask) == Collide_FloorMask &&
            body.floor.flags == 9 && body.floor.normal.y == 1,
            "Original ECB landing failed, including subdivided high-speed fall");
    }
    Fighter walking = { .ground_or_air = GA_Ground, .cur_pos = { 0, 5.0001f, 0 },
        .self_vel = { 1, 0, 0 } };
    HSD_GObj walker = { .user_data = &walking };
    walking.coll_data.cur_pos = walking.cur_pos;
    mpColl_80041EE4(&walking.coll_data);
    mpColl_SetECBSource_Fixed(&walking.coll_data, NULL, 8, 0, 4, -4);
    walking.coll_data.floor.index = 1;
    walking.coll_data.floor.normal.y = 1;
    Fighter_UpdatePosition(&walker);
    bool walk_result = ft_80082708(&walker);
    require(walk_result && walking.cur_pos.x == 1 &&
        fabsf(walking.cur_pos.y - 5.0001f) < 0.00001f &&
        walking.coll_data.last_pos.x == 0 && walking.coll_data.cur_pos.x == 1,
        "Original grounded collision did not reconcile movement with its floor");
    walking.self_vel.x = 20;
    Fighter_UpdatePosition(&walker);
    walk_result = ft_80082708(&walker);
    // Twenty units subdivide into five-unit steps; support is lost at x=11.
    require(!walk_result && walking.cur_pos.x == 11,
        "Original grounded collision did not report walking off the floor");
    puts("Original fighter movement followed by grounded collision and floor departure passed");
    CollData missed = { .cur_pos = { 20, 10, 0 } };
    mpColl_80041EE4(&missed);
    mpColl_SetECBSource_Fixed(&missed, NULL, 8, 0, 4, -4);
    missed.cur_pos.y = -5;
    require(!mpColl_800471F8(&missed) && missed.cur_pos.y == -5 &&
        !(missed.env_flags & Collide_FloorMask), "ECB landed beyond platform bounds");
    require(OSCheckHeap(HSD_GetHeap()) == before, "ECB landing allocated or damaged heap");
    CollVtx* live = mpGetGroundCollVtx();
    live[2].x10 = -10; live[2].x14 = 5;
    live[3].x10 = 10; live[3].x14 = 5;
    Vec3 position = { 0, 6, 9 }, speed = { 99, 99, 99 };
    require(mpGetSpeed(1, &position, &speed) && speed.x == 0 &&
        speed.y == 0 && speed.z == 0, "Static platform added displacement");
    live[2].pos = (Vec2) { -7, 7 }; live[3].pos = (Vec2) { 13, 7 };
    require(mpGetSpeed(1, &position, &speed) && speed.x == 3 &&
        speed.y == 2 && speed.z == 0, "Translating platform carry changed");
    Fighter carried = { .ground_or_air = GA_Ground, .cur_pos = { 0, 6, 0 },
        .self_vel = { 1, 0, 0 } };
    HSD_GObj carried_object = { .user_data = &carried };
    carried.coll_data.floor.index = 1;
    Fighter_UpdatePosition(&carried_object);
    require(carried.cur_pos.x == 4 && carried.cur_pos.y == 8,
        "Combined physics phase omitted platform carry after self movement");
    carried.x2219_b5 = 1;
    Fighter_UpdatePosition(&carried_object);
    require(carried.cur_pos.x == 7 && carried.cur_pos.y == 10,
        "Hitlag incorrectly suppressed platform carry");
    live[2].pos = (Vec2) { -10, 5 }; live[3].pos = (Vec2) { 10, 9 };
    require(mpGetSpeed(1, &position, &speed) && speed.x == 0 && speed.y == 2,
        "Platform endpoint interpolation changed");
    position.x = 20;
    require(mpGetSpeed(1, &position, &speed) && speed.y == 4,
        "Platform carry did not clamp to its endpoint");
    require(!mpGetSpeed(-1, &position, &speed), "Missing floor produced platform carry");
    require(OSCheckHeap(HSD_GetHeap()) == before, "Platform carry allocated");
    puts("Original platform carry: static, translation, endpoint interpolation and clamp passed");
    puts("Original ECB landing, high-speed subdivision and platform miss passed");
    puts("Original collision initialization, nearest floor, skip and miss queries passed");
}

static void fixture(void)
{
    u8 bytes[164] = { 0 }, saved[164];
    u8* data = bytes + 32;
    put32(bytes, sizeof(bytes)); put32(bytes + 4, 120);
    put32(bytes + 8, 3);
    put32(bytes + 152, 0); put32(bytes + 156, 8); put32(bytes + 160, 36);
    put32(data, 48); put32(data + 4, 2);
    put32(data + 8, 64); put32(data + 12, 1);
    data[19] = 1; // One floor.
    put32(data + 36, 80); put32(data + 40, 1);
    put32(data + 48, 0xC1200000); // (-10, 0) to (10, 0).
    put32(data + 56, 0x41200000);
    data[67] = 1;
    memset(data + 68, 255, 8); // Disconnected links retain -1.
    data[76] = 0x12; data[77] = 0x34; data[78] = 0x56; data[79] = 0x78;
    data[83] = 1;
    put32(data + 100, 0xC1200000); put32(data + 108, 0x41200000);
    data[119] = 2;
    memcpy(saved, bytes, sizeof(bytes));
    HSD_Dat dat;
    Vec2 vertices[2]; MapLine lines[1]; MapJoint joints[1]; MapCollData map;
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK,
            "Cannot open collision fixture");
    require(mpDecodeCollision(&dat, 0, &map, vertices, 2, lines, 1, joints, 1) == HSD_DAT_OK,
            "Cannot decode collision fixture");
    require(map.vert_count == 2 && map.line_count == 1 && map.joint_count == 1 &&
        map.floor_count == 1 && vertices[0].x == -10 && vertices[1].x == 10 &&
        lines[0].v1_idx == 1 && lines[0].prev_id0 == -1 &&
        lines[0].hi_flags == 0x1234 && lines[0].lo_flags == 0x5678 &&
        joints[0].left_bound == -10 && joints[0].right_bound == 10 &&
        joints[0].floor_count == 1 && joints[0].vtx_count == 2,
        "Collision fields differ from the source");
    lines[0].prev_id0 = 0;
    require(!memcmp(bytes, saved, sizeof(bytes)), "Native collision writes modified source DAT");
    MapCollData previous = map;
    require(mpDecodeCollision(&dat, 0, &map, vertices, 1, lines, 1, joints, 1) == HSD_DAT_CAPACITY &&
        !memcmp(&map, &previous, sizeof(map)), "Capacity failure published partial output");
    const unsigned bad_offsets[] = { 67, 69, 19, 83, 119 };
    for (unsigned i = 0; i < sizeof(bad_offsets) / sizeof(*bad_offsets); ++i) {
        unsigned at = bad_offsets[i]; u8 old = data[at]; data[at] = 127;
        require(mpDecodeCollision(&dat, 0, &map, vertices, 2, lines, 1, joints, 1) == HSD_DAT_DESCRIPTOR,
                "Invalid collision index or range accepted");
        data[at] = old;
    }
    put32(data + 48, 0x7F800000);
    require(mpDecodeCollision(&dat, 0, &map, vertices, 2, lines, 1, joints, 1) == HSD_DAT_DESCRIPTOR,
            "Nonfinite collision coordinate accepted");
    put32(data + 48, 0xC1200000); put32(data + 36, 116);
    require(mpDecodeCollision(&dat, 0, &map, vertices, 2, lines, 1, joints, 1) == HSD_DAT_DESCRIPTOR,
            "Truncated collision joint accepted");
    puts("Collision decoding, writable storage, indices, ranges and bounds passed");
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const size_t heap_size = 16 * 1024 * 1024;
    u8* heap = malloc(heap_size);
    require(heap != NULL, "Cannot allocate collision test heap");
    void* start = OSInitAlloc(heap, heap + heap_size, 1);
    OSHeapHandle handle = OSCreateHeap(start, heap + heap_size);
    require(handle >= 0, "Cannot create collision test heap");
    HSD_SetHeap(handle); OSSetCurrentHeap(handle);
    HSD_GObjLibInitDataType config;
    HSD_GObj_803912E0(&config);
    HSD_GObj_80391304(&config);
    require(lbExpf(0) == 1 && fabsf(lbExpf(1) - 2.7182818f) < 0.000001f &&
        fabsf(lbExpf(-1) - 0.36787944f) < 0.000001f &&
        fabsf(lbPowf(2, 3) - 8) < 0.00001f && lbPowf(0, 0) == 0,
        "Original game exponential/power behavior changed after native symbol separation");
    require(fabs(__frsqrte(1764.0) * 42.0 - 1.0) < 0.000000000001,
        "PowerPC reciprocal-square-root estimate has incorrect native semantics");
    fixture();
    check_part_tables();
    const ftCo_DatAttrs movement_attributes = { .gravity = 0.1f, .terminal_velocity = 2,
        .fast_fall_velocity = 3, .air_drift_max = 1, .ground_friction = 0.06f,
        .aerial_friction = 0.02f };
    check_ground_projection();
    check_position_integration();
    check_environment_wind();
    check_physics_phase();
    check_movement(&movement_attributes);
    check_common_attributes();
    check_material_utilities();
    check_refraction_texels();
    check_floor_queries();
    for (int arg = 1; arg < argc; ++arg) {
        FILE* file = fopen(argv[arg], "rb");
        require(file && fseek(file, 0, SEEK_END) == 0, "Cannot open stage DAT");
        long size = ftell(file);
        require(size > 0 && fseek(file, 0, SEEK_SET) == 0, "Cannot size stage DAT");
        void* bytes = malloc(size);
        require(bytes && fread(bytes, 1, size, file) == (size_t) size, "Cannot read stage DAT");
        fclose(file);
        HSD_Dat dat; u32 root, nv, nl, nj;
        require(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK, "Invalid input DAT");
        if (HSD_DatFindRoot(&dat, "ftLoadCommonData", &root)) {
            u32 table, omissions;
            const char* external;
            require(HSD_DatBytes(&dat, root, 24) &&
                HSD_DatResolve(&dat, root + 16, &table, &external) == HSD_DAT_REF_LOCAL &&
                HSD_DatResolve(&dat, root + 20, &omissions, &external) == HSD_DAT_REF_LOCAL,
                "Cannot resolve common fighter part table");
            for (unsigned kind = 0; kind < Ft_Kind_StockCount; ++kind) {
                u32 descriptor;
                HSD_DatReferenceKind ref = HSD_DatResolve(&dat, table + kind * 4,
                    &descriptor, &external);
                if (ref == HSD_DAT_REF_NULL) continue;
                u8 joints[256], parts[FT_COMMON_PART_COUNT];
                FighterPartsTable decoded;
                require(ref == HSD_DAT_REF_LOCAL &&
                    ftData_DecodeParts(&dat, descriptor, &decoded, joints, sizeof(joints), parts) == HSD_DAT_OK,
                    "Cannot decode common fighter part mapping");
                struct Fighter_804D6540_t omitted = { 0 };
                struct Fighter_804D6540_x0_t records[32];
                ref = HSD_DatResolve(&dat, omissions + kind * 4, &descriptor, &external);
                require(ref == HSD_DAT_REF_NULL || (ref == HSD_DAT_REF_LOCAL &&
                    ftData_DecodePartOmissions(&dat, descriptor, &omitted, records, 32,
                        decoded.parts_num) == HSD_DAT_OK), "Cannot decode omitted fighter slots");
                printf("%s: fighter %u has %lu part slots, %d omitted\n",
                    argv[arg], kind, decoded.parts_num, omitted.x4);
            }
            free(bytes);
            continue;
        }
        if (HSD_DatFindRoot(&dat, "ftDataMario", &root)) {
            u32 collision_offset, attribute_offset;
            const char* external;
            ftCo_DatAttrs attributes;
            require(HSD_DatResolve(&dat, root, &attribute_offset, &external) == HSD_DAT_REF_LOCAL &&
                ftData_DecodeCommonAttrs(&dat, attribute_offset, &attributes) == HSD_DAT_OK,
                "Cannot decode Mario common attributes");
            check_movement(&attributes);
            printf("Mario attributes: walk %g, dash %g, gravity %g, weight %g, model scale %g, jumps %d\n",
                attributes.walk_max_vel, attributes.dash_max_velocity, attributes.gravity,
                attributes.weight, attributes.model_scaling, attributes.max_jumps);
            ftData_x44_t parameters;
            require(HSD_DatBytes(&dat, root, 96) &&
                HSD_DatResolve(&dat, root + 0x44, &collision_offset, &external) == HSD_DAT_REF_LOCAL &&
                ftData_DecodeCollision(&dat, collision_offset, &parameters) == HSD_DAT_OK,
                "Cannot decode Mario collision parameters");
            printf("%s: collision parts %d,%d,%d,%d,%d,%d; offset %g; ledge snap %g,%g,%g\n",
                argv[arg], parameters.unk0, parameters.unk2, parameters.unk4,
                parameters.unk6, parameters.unk8, parameters.unkA, parameters.unkC,
                parameters.ledge_snap_x, parameters.ledge_snap_y, parameters.ledge_snap_height);
            free(bytes);
            continue;
        }
        require(HSD_DatFindRoot(&dat, "coll_data", &root) && HSD_DatBytes(&dat, root, 48),
            "Cannot find collision root");
        HSD_DatReadU32(&dat, root + 4, &nv);
        HSD_DatReadU32(&dat, root + 12, &nl);
        HSD_DatReadU32(&dat, root + 40, &nj);
        Vec2* vertices = calloc(nv, sizeof(*vertices));
        MapLine* lines = calloc(nl, sizeof(*lines));
        MapJoint* joints = calloc(nj, sizeof(*joints));
        MapCollData map;
        require(vertices && lines && joints &&
            mpDecodeCollision(&dat, root, &map, vertices, nv, lines, nl, joints, nj) == HSD_DAT_OK,
            "Cannot decode stage collision");
        printf("%s: %lu vertices, %lu lines, %lu joints; floors %d, ceilings %d, right walls %d, left walls %d, dynamic %d\n",
            argv[arg], nv, nl, nj, map.floor_count, map.ceiling_count,
            map.right_wall_count, map.left_wall_count, map.dynamic_count);
        check_runtime(&map);
        free(joints); free(lines); free(vertices); free(bytes);
    }
    // Original stage allocations live until the stage heap is retired. This
    // harness uses one heap for all fixtures and retires it after the last query.
    OSDestroyHeap(handle); HSD_SetHeap(-1); free(heap);
    return 0;
}
