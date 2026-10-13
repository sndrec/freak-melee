// Stock input predicate before the optional scripted hook.
// Captured from e6183d64; body SHA256 0b49a741256e0006ceed35240fddfd2251d8d3ece53300f2bd794d87ec72541a
bool stock_air_tether_input(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->used_tether) {
        return false;
    }
    if (fp->kind != Ft_Kind_Link && fp->kind != Ft_Kind_CLink &&
        fp->kind != Ft_Kind_Samus)
    {
        return false;
    }
    if (fp->accessory2_cb != NULL || fp->death1_cb != NULL ||
        fp->accessory3_cb != NULL)
    {
        return false;
    }
    if (fp->item_gobj != NULL) {
        return false;
    }
    if (fp->input.held_buttons[0] & HSD_PAD_LR &&
        fp->input.pressed_buttons & HSD_PAD_A)
    {
        ftCo_800C3BE8(gobj);
        fp->used_tether = true;
        return true;
    }
    return false;
}
