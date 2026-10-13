// Original baseline49 ftPeach/ftpeachfloat.c, SHA256 3764ec024bfaa2af8a03509984fb0bdd6bec9a0356a956341e6eb1417d4ca076
// Four actual source bodies; only BA54/BAD8 names change to link beside new API.
// LF-normalized extraction; no expected-body edits or float-entry implementation.
// END ORIGINAL PROVENANCE
bool ftPe_Float_CheckContinueInput(Fighter* fp)
{
    return fp->input.lstick[0].y >= p_ftCommonData->tap_jump_threshold ||
           fp->input.held_buttons[0] & HSD_PAD_XY;
}

static bool checkStartFloatInput(HSD_GObj* gobj)
{
    Fighter* temp_r6 = GET_FIGHTER(gobj);
    return temp_r6->input.lstick[0].y <= -p_ftCommonData->x88 &&
           temp_r6->input.held_buttons[0] & HSD_PAD_XY;
}

bool original_manual(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    bool float_input = checkStartFloatInput(gobj);
    if (fp->kind == Ft_Kind_Peach && fp->u.pe.has_float && float_input) {
        ftPe_8011BB6C(gobj, true);
        return true;
    }
    return false;
}

bool original_automatic(HSD_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->kind == Ft_Kind_Peach) {
        if (fp->self_vel.y <= 0 && fp->u.pe.has_float) {
            if (ftPe_Float_CheckContinueInput(fp)) {
                ftPe_8011BB6C(gobj, true);
                return true;
            }
        }
    }
    return false;
}
