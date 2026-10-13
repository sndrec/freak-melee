// Original common caller bodies from HEAD before generic capture hooks.

void original_fn_800DB5D8(Fighter_GObj* gobj)
{
    ftHurtboxInit sp18;
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter* victim_fp = GET_FIGHTER(fp->victim_gobj);
    if (victim_fp->kind == Ft_Kind_Yoshi) {
        fp->invisible = true;
        fp->accessory1_cb = ftCo_800DB464;
        ftColl_8007B0C0(gobj, HurtCapsule_Intangible);
        sp18.bone_idx = ftParts_GetBoneIndex(fp, FtPart_XRotN);
        sp18.height = HurtHeight_Mid;
        sp18.is_grabbable = false;
        sp18.a_offset.x = sp18.a_offset.y = sp18.a_offset.z = 0.0F;
        sp18.b_offset.x = sp18.b_offset.y = sp18.b_offset.z = 0.0F;
        sp18.scale =
            ftYs_Init_8012BAC0(victim_fp) / ftCommon_GetModelScale(fp);
        ftColl_HurtboxInit(fp, fp->hurt_capsules, &sp18);
    }
    ftCommon_8007E2F4(fp, 0x1FF);
}

void original_fn_800DB790(Fighter_GObj* gobj)
{
    ftHurtboxInit hurt;
    Fighter* victim_fp;
    Fighter* fp;

    Fighter_ChangeMotionState(gobj, ftCo_MS_CaptureWaitHi, 0, 0.0F, 1.0F, 0.0F,
                              NULL);

    if ((victim_fp = GET_FIGHTER((fp = GET_FIGHTER(gobj))->victim_gobj))
            ->kind == Ft_Kind_Yoshi)
    {
        fp->invisible = true;
        fp->accessory1_cb = ftCo_800DB464;
        ftColl_8007B0C0(gobj, HurtCapsule_Intangible);
        hurt.bone_idx = ftParts_GetBoneIndex(fp, FtPart_XRotN);
        hurt.height = HurtHeight_Mid;
        hurt.is_grabbable = false;
        hurt.a_offset.x = hurt.a_offset.y = hurt.a_offset.z = 0.0F;
        hurt.b_offset.x = hurt.b_offset.y = hurt.b_offset.z = 0.0F;
        hurt.scale =
            ftYs_Init_8012BAC0(victim_fp) / ftCommon_GetModelScale(fp);
        ftColl_HurtboxInit(fp, fp->hurt_capsules, &hurt);
    }

    ftCommon_8007E2F4(fp, 0x1FF);
}

void original_fn_800DBAE4(Fighter_GObj* gobj)
{
    ftHurtboxInit hurt;
    Fighter* victim_fp;
    Fighter* fp;

    Fighter_ChangeMotionState(gobj, ftCo_MS_CaptureWaitLw, 0, 0.0F, 1.0F, 0.0F,
                              NULL);

    if ((victim_fp = GET_FIGHTER((fp = GET_FIGHTER(gobj))->victim_gobj))
            ->kind == Ft_Kind_Yoshi)
    {
        fp->invisible = true;
        fp->accessory1_cb = ftCo_800DB464;
        ftColl_8007B0C0(gobj, HurtCapsule_Intangible);
        hurt.bone_idx = ftParts_GetBoneIndex(fp, FtPart_XRotN);
        hurt.height = HurtHeight_Mid;
        hurt.is_grabbable = false;
        hurt.a_offset.x = hurt.a_offset.y = hurt.a_offset.z = 0.0F;
        hurt.b_offset.x = hurt.b_offset.y = hurt.b_offset.z = 0.0F;
        hurt.scale =
            ftYs_Init_8012BAC0(victim_fp) / ftCommon_GetModelScale(fp);
        ftColl_HurtboxInit(fp, fp->hurt_capsules, &hurt);
    }

    ftCommon_8007E2F4(fp, 0x1FF);
}

void original_fn_800DB6C8(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter* victim_fp = GET_FIGHTER(fp->victim_gobj);

    if (fp->motion_id == ftCo_MS_CapturePulledHi) {
        original_fn_800DB790(gobj);
    } else {
        original_fn_800DBAE4(gobj);
    }

    switch (victim_fp->kind) {
    case Ft_Kind_Yoshi:
        ftCo_800DB368(victim_fp, fp);
        break;
    case Ft_Kind_Link:
    case Ft_Kind_CLink:
        if (victim_fp->u.lk.xC != NULL) {
            it_802A7840((HSD_GObj*) victim_fp->u.lk.xC);
        }
        break;
    case Ft_Kind_Samus:
        if (victim_fp->u.ss.x223C != NULL) {
            it_802BAA94(victim_fp->u.ss.x223C);
        }
        break;
    default:
        break;
    }

    ftCommon_8007EBAC(fp, 3, 0);
}

void original_fn_800D9CE8(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftYoshiAttributes* yattrs;
    Item_GObj* item;
    Item* it;
    f32 frame;
    f32 rate;
    f32 startFrame;
    s32 nextMotion;
    PAD_STACK(0x18);

    fp->gr_vel = 0.0f;
    frame = fp->cur_anim_frame;
    startFrame = frame;
    if (fp->motion_id == 0xD4) {
        nextMotion = 0xD5;
        if (fp->kind == Ft_Kind_Yoshi) {
            yattrs = fp->dat_attrs;
            if (frame >= yattrs->x124 && frame < yattrs->x128) {
                rate = frame - yattrs->x124;
                ftAnim_SetAnimRate(gobj, rate);
                ftAnim_8006EBA4(gobj);
                startFrame = (f32) yattrs->x12C[(s32) rate];
            }
        }
    } else {
        nextMotion = 0xD7;
    }

    switch (fp->kind) {
    case Ft_Kind_CLink:
    case Ft_Kind_Link:
        item = fp->u.lk.xC;
        it = GET_ITEM(item);
        it_802A7840((HSD_GObj*) item);
        fp->mv.co.capturedamage.x18 = it->xDD4_itemVar.linkhookshot.xC;
        break;
    case Ft_Kind_Samus:
        item = fp->u.ss.x223C;
        it = GET_ITEM(item);
        it_802BAA94(item);
        fp->mv.co.capturedamage.x18 = it->xDD4_itemVar.samusgrapple.xC;
        break;
    default:
        fp->mv.co.capturedamage.x18 = fp->parts[fp->ft_data->x8->x11].joint;
        break;
    }

    fp->throw_flags = 0;
    Fighter_ChangeMotionState(gobj, nextMotion, 0x4000, startFrame, 1.0f, 0.0f,
                              NULL);
    fp->accessory1_cb = fn_800DA190;
    fp->x221B_b7 = 0;
}

void original_ftCo_800DE3FC(Fighter_GObj* gobj, FtMotionId msid, float anim_speed)
{
    Fighter* fp;
    Fighter* thrower_fp;

    fp = GET_FIGHTER(gobj);
    thrower_fp = GET_FIGHTER(fp->victim_gobj);
    ftCo_800DB368(thrower_fp, fp);
    fp->facing_dir = thrower_fp->facing_dir;
    fp->mv.co.thrown.unk_bool = false;
    Fighter_ChangeMotionState(gobj, msid, 0, 0.0f, anim_speed, 0.0f,
                              fp->victim_gobj);
    if (thrower_fp->kind == Ft_Kind_Yoshi) {
        ftColl_8007B62C(gobj, 2);
    }
    if (thrower_fp->kind != Ft_Kind_Kirby || msid - ftCo_MS_ThrownF > 1U) {
        pl_80040614(thrower_fp->player_id, thrower_fp->x221F_b4,
                    fp->grab_timer);
        fp->grab_timer = 0.0f;
    }
    fp->accessory1_cb = ftCo_800DE508;
    ftCommon_8007E2F4(fp, 0x1FF);
    ftCommon_8007E358(gobj);
    ftAnim_8006EBA4(gobj);
}
