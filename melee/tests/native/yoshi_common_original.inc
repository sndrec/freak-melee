// Unmodified original C functions selected from the named live stock sources.

void ft_800CB6EC(Fighter* fp, s32 arg1)
{
    if (fp->mv.co.jumpaerial.x0 != 0) {
        fp->mv.co.jumpaerial.x0 -= 1;
        HSD_JObjAddRotationY(fp->parts->joint, -MTXDegToRad(180.0F / arg1));
        if (fp->mv.co.jumpaerial.x0 == (arg1 / 2)) {
            fp->facing_dir = -fp->facing_dir;
        }
    }
}

void ftYs_JumpAerial_Enter(Fighter_GObj* gobj)
{
    // mv.co.jumpaerial.x0 could also be mv.ys.jumpaerial.x0
    Fighter* fp = GET_FIGHTER(gobj);
    ftYoshiAttributes* ys_attrs = fp->dat_attrs;
    Vec3 vel;
    int arg1;
    PAD_STACK(16);

    ftCommon_8007D5D4(fp);
    fp->cmd_vars[0] = 1;
    vel.x = fp->input.lstick[0].x * fp->co_attrs.air_jump_h_multiplier;
    vel.y = 0.0F;
    vel.z = 0.0F;
    ftCo_JumpAerial_Begin(gobj, ftCo_MS_JumpAerialF, &vel, true, true, true);
    fp->anim_cb = (HSD_GObjEvent) ftYs_JumpAerial_Anim_Cb;
    fp->dmg.armor1 = ys_attrs->x8;
    if ((fp->input.lstick[0].x * fp->facing_dir) < -ys_attrs->x4) {
        fp->mv.co.jumpaerial.x0 = ys_attrs->x0;
    } else {
        fp->mv.co.jumpaerial.x0 = 0;
    }

    arg1 = ys_attrs->x0;
    ft_800CB6EC(fp, arg1);
}

void ftYs_JumpAerial_Anim_Cb(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftYoshiAttributes* ys_attrs = fp->dat_attrs;
    PAD_STACK(8);
    ftCo_JumpAerial_Anim(gobj);
    ft_800CB6EC(fp, ys_attrs->x0);
}

void ftYoshi_InitializeShieldMaterialAnimations(
    Fighter* fp, FtPartsVisLookup* visibility)
{
    u8 _[12];
    ftYoshiAttributes* attr_r26;
    TempS* shield_variant;
    s32 i;
    float zero_float;

    attr_r26 = fp->ft_data->ext_attr;
    shield_variant = &visibility[0].x4[1];
    zero_float = 0.0f;

    for (i = 0; i < shield_variant->x0; i++) {
        HSD_DObj* dobj_r3 =
            fp->dobj_list.data[shield_variant->x4[i]];
        HSD_MObj* mobj_r3;
        HSD_AObj* aobj_r24;

        if (dobj_r3) {
            mobj_r3 = dobj_r3->mobj;
        } else {
            mobj_r3 = 0;
        }

        aobj_r24 = mobj_r3->aobj;
        HSD_AObjSetRate(aobj_r24, 0.0f);

        if (zero_float == attr_r26->xC) {
            attr_r26->xC = HSD_AObjGetEndFrame(aobj_r24);
        } else {
            if (attr_r26->xC != HSD_AObjGetEndFrame(aobj_r24)) {
                HSD_ASSERTREPORT(97, 0, "yoshi matanim frame not same\n");
            }
        }
    }
}

void ftYoshi_SetShieldMaterialFrame(Fighter* fp,
                                    FtPartsVisLookup* visibility,
                                    float start_frame)
{
    u8 _[8];

    if (visibility) {
        TempS* shield_variant = &visibility[0].x4[1];
        s32 i;

        for (i = 0; i < shield_variant->x0; i++) {
            HSD_DObj* dobj_r3 =
                fp->dobj_list.data[shield_variant->x4[i]];
            HSD_MObj* mobj_r3;
            HSD_MObj* mobj;

            if (dobj_r3) {
                mobj_r3 = dobj_r3->mobj;
            } else {
                mobj_r3 = 0;
            }

            mobj = mobj_r3;
            HSD_MObjReqAnim(mobj, start_frame);
            HSD_MObjAnim(mobj);
        }
    }
}

void ftYoshi_UpdateShieldMaterialFrame(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftYoshiAttributes* da = fp->dat_attrs;
    u8 _[8];
    float tempf =
        da->xC *
        (1.0f - (fp->shield_health / p_ftCommonData->x260_startShieldHealth));
    ftYoshi_SetShieldMaterialFrame(fp, fp->x5AC.xC[0], tempf);
    ftYoshi_SetShieldMaterialFrame(fp, fp->x5AC.xC[1], tempf);
}

// src/melee/ft/kinds/ftCommon/ftCo_FallAerial.c
void ftCo_FallAerial_Enter(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_ChangeMotionState(gobj, ftCo_MS_FallAerial, Ft_MF_None, 0.0F, 1.0F,
                              0.0F, NULL);
    fp->mv.co.fallaerial.smid = ftCo_SM_FallAerial;
    fp->mv.co.fallaerial.x4 = 0.0F;
}

// src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c
void ftCo_JumpAerial_Anim(Fighter_GObj* gobj)
{
    if (!ftAnim_IsFramesRemaining(gobj)) {
        ftCo_FallAerial_Enter(gobj);
    }
}
