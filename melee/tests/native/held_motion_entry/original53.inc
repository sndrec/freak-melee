static inline FtMotionId original53_ftCo_ItemParasolGetFallMotionId(
    Fighter_GObj* gobj, ftPeach_MotionState pe_msid, FtMotionId co_msid)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->kind == Ft_Kind_Peach && fp->u.pe.parasol_gobj_0 != NULL) {
        return pe_msid;
    } else {
        return co_msid;
    }
}

static inline void original53_ft_800CEF08_inline1(Fighter* fp, int x2104)
{
    fp->x2221_b7 = false;
    fp->x2221_b4 = true;
    if (fp->x2221_b6) {
        fp->x2104 = x2104;
        fp->x2221_b6 = false;
    }
}

void original53_ft_800CEF08(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->mv.co.parasol_open.prev_msid = fp->motion_id;
    Fighter_ChangeMotionState(
        gobj,
        original53_ftCo_ItemParasolGetFallMotionId(gobj, ftPe_MS_ItemParasolOpen,
                                        ftCo_MS_ItemParasolOpen),
        0, 0.0f, 1.0f, 0.0f, NULL);
    ftAnim_8006EBA4(gobj);
    original53_ft_800CEF08_inline1(fp, p_ftCommonData->x59C);
}

void original53_ftCo_800CEFE0(Fighter_GObj* gobj, int arg1)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->mv.co.parasol_open.prev_msid = fp->motion_id;
    Fighter_ChangeMotionState(
        gobj,
        original53_ftCo_ItemParasolGetFallMotionId(gobj, ftPe_MS_ItemParasolOpen,
                                        ftCo_MS_ItemParasolOpen),
        0, 0.0f, 1.0f, 10.0f, NULL);
    ftAnim_8006EBA4(gobj);
    original53_ft_800CEF08_inline1(fp, p_ftCommonData->x59C);
}

void original53_ftCo_800CF3C8(Fighter_GObj* gobj)
{
    Fighter* temp_r4;
    s32 var_r4;

    temp_r4 = gobj->user_data;
    Fighter_ChangeMotionState(
        gobj,
        original53_ftCo_ItemParasolGetFallMotionId(gobj, ftPe_MS_ItemParasolFall,
                                        ftCo_MS_ItemParasolFallSpecial),
        Ft_MF_SkipHit | Ft_MF_SkipParasol, 0.0f, 1.0f, 0.0f, NULL);
}
