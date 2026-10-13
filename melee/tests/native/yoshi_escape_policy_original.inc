// Stock Yoshi common roll/spotdodge policy bodies.

void original_ftCo_80099438(Fighter_GObj* gobj, FtMotionId msid, bool arg2)
{
    u8 _[8] = { 0 };
    Fighter* fp = gobj->user_data;
    ftCo_80099314(gobj, msid, arg2);
    ftParts_80074B0C(gobj, 0, 1);
    ftYs_Init_8012BDA0(gobj);
    fp->anim_cb = original_ftCo_80099644;
}

void original_ftCo_80099644(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    if (ftCheckThrowB3(fp)) {
        fp->facing_dir = -fp->facing_dir;
    }
    if (!ftAnim_IsFramesRemaining(gobj)) {
        fp->gr_vel = 0;
        if (ftYs_Shield_8012CC1C(gobj)) {
            return;
        }
        ftYs_Init_8012BE3C(gobj);
        ft_8008A2BC(gobj);
    }
    ftYoshi_UpdateShieldMaterialFrame(gobj);
}

void original_ftCo_80099954(Fighter_GObj* gobj)
{
    u8 _[8] = { 0 };
    Fighter* fp = gobj->user_data;
    if (fp->x5F4_arr[0].idx == 1) {
        ftYs_Init_8012BE3C(gobj);
    }
    ftCo_800998EC(gobj);
}
