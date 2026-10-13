#include <sysdolphin/baselib/gobj.h>
#include <melee/ft/ftmaterial.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tev.h>
#include <sysdolphin/baselib/memory.h>
#include <stdio.h>
#include <stdlib.h>

void test_native_fighter_material(void)
{
    Fighter fighter = { 0 };
    fighter.x488.x7C_flag2 = true;
    fighter.x488.x7C_light_enable = true;
    fighter.x488.x50_light_color = (GXColor) { 64, 128, 192, 255 };
    HSD_TExpTevDesc stage = { 0 };
    HSD_TExp reserved[2] = { 0 }, overlay = { 0 };
    reserved[0].cnst.reg = 0;
    reserved[0].cnst.next = &reserved[1];
    reserved[1].cnst.reg = 1;
    HSD_MObj material = { .tevdesc = &stage, .texp = reserved };
    static u8 commands[4096];
    long before = OSCheckHeap(HSD_GetHeap());
    HSD_StateInitTev();
    GXBeginDisplayList(commands, sizeof(commands));
    HSD_TExp* result = ftMaterial_800BF534(&fighter, &material, &overlay, 0);
    u32 size = GXEndDisplayList();
    if (result != &overlay || overlay.cnst.reg != 2 ||
        overlay.cnst.val != &fighter.x488.x50_light_color ||
        overlay.cnst.type != HSD_TE_CNST || overlay.cnst.comp != HSD_TE_RGB ||
        !size || OSCheckHeap(HSD_GetHeap()) != before)
    {
        fprintf(stderr, "Original fighter light overlay failed native template/register setup\n");
        exit(1);
    }
    fighter.x488.x7C_light_enable = false;
    if (ftMaterial_800BF534(&fighter, &material, &overlay, 0) != NULL) {
        fprintf(stderr, "Disabled fighter overlay remained active\n");
        exit(1);
    }
    puts("Original fighter TEV light overlay used register 2 and emitted GX commands");
}

void test_native_fighter_material_setup(HSD_MObj* material)
{
    Fighter fighter = { 0 };
    HSD_GObj owner = { 0 };
    owner.user_data = &fighter;
    fighter.gobj = &owner;
    fighter.x61D = 255;
    HSD_GObj* previous = HSD_GObj_804D7814;
    HSD_GObj_804D7814 = &owner;
    if (HSD_MOBJ_METHOD(material)->setup != ftMaterial_800BF2B8) {
        fprintf(stderr, "Fighter material class installation failed\n");
        exit(1);
    }
    static u8 commands[65536];
    for (unsigned variant = 0; variant < 4; ++variant) {
        fighter.x61D = variant == 1 ? 128 : 255;
        fighter.x2223_b3 = variant == 2;
        fighter.x488.x7C_color_enable = variant == 3;
        fighter.x488.x2C_hex = (GXColor) { 64, 128, 192, 96 };
        long before = OSCheckHeap(HSD_GetHeap());
        GXBeginDisplayList(commands, sizeof(commands));
        HSD_MOBJ_METHOD(material)->setup(material, material->rendermode);
        u32 size = GXEndDisplayList();
        if (!size || OSCheckHeap(HSD_GetHeap()) != before) {
            fprintf(stderr, "Fighter material variant %u failed or allocated during drawing\n", variant);
            exit(1);
        }
    }
    HSD_GObj_804D7814 = previous;
}
