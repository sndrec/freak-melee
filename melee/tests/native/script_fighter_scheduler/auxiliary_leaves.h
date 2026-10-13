static ftCommonData fighter_common;
ftCommonData* p_ftCommonData=&fighter_common;
// Controlled effects surrounding the actual auxiliary-item scheduler bodies.
HSD_GObj* scheduler_aux_seen_first;
HSD_GObj* scheduler_aux_seen_second;
HSD_GObj* scheduler_aux_spawn;
int scheduler_aux_seen_timer;
unsigned scheduler_aux_tail;
const Vec3 ftCo_803B74A0 = {0};
void fighter_scheduler_aux_native(bool enabled)
{
    legacy_fighter.on_auxiliary_item_attach = enabled ? stock_event : NULL;
    legacy_fighter.on_auxiliary_item_visibility_reset = enabled ? stock_event : NULL;
}
void ft_PlaySFX(Fighter* fp, enum_t sound, u8 volume, u8 pan)
{ (void)fp; (void)sound; (void)volume; (void)pan; ++scheduler_aux_tail; }
void ft_80081C88(Fighter_GObj* object, float scale)
{ (void)object; (void)scale; ++scheduler_aux_tail; }
void ftCommon_8007EBAC(Fighter* fp, u32 id, u32 value)
{ (void)fp; (void)id; (void)value; }
float it_8026B54C(Item_GObj* object) { (void)object; return 27; }
void it_8026BD0C(Item_GObj* object) { (void)object; }
void Item_8026ABD8(Item_GObj* object, Vec3* pos, float value)
{ (void)object; (void)pos; (void)value; }
void Item_8026AB54(Item_GObj* object, HSD_GObj* owner, Fighter_Part bone)
{ (void)object; (void)owner; (void)bone; }
void it_8026BCF4(Item_GObj* object) { (void)object; ++scheduler_aux_tail; }
void it_80294EB0(Item_GObj* object, Vec3* a, Vec3* b)
{ (void)object; (void)a; (void)b; ++scheduler_aux_tail; }
void it_80294E78(Item_GObj* object, float scale)
{ (void)object; (void)scale; ++scheduler_aux_tail; }
void Item_8026A8EC(Item_GObj* object) { (void)object; ++scheduler_aux_tail; }
Item_GObj* it_8029A748(Fighter_GObj* object, Vec3* pos, Fighter_Part bone, float facing)
{ (void)object; (void)pos; (void)bone; (void)facing; return scheduler_aux_spawn; }
void ftCommon_8007FDA0(HSD_GObj* object)
{ (void)object; ++scheduler_aux_tail; }

uint32_t scheduler_jab_flags;
void ft_800892A0(Fighter_GObj* object) { (void)object; ++scheduler_aux_tail; }
void ft_80089824(Fighter_GObj* object) { (void)object; ++scheduler_aux_tail; }
bool ftpickupitem_80094790(Fighter_GObj* object) { (void)object; return false; }
void ftAnim_8006EBA4(Fighter_GObj* object) { (void)object; ++scheduler_aux_tail; }
