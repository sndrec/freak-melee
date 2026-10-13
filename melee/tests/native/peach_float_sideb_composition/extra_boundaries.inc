
/* Engine animation, drift, probes and input entry outcomes are controlled.
 * Real common mask/math/selector/contact/lifetime bodies are linked/extracted. */
static ftCommonData common_fighter;
ftCommonData* p_ftCommonData = &common_fighter;
static bool special_input, item_throw_input, jump_input, special_ground_hit;
static float landing_threshold, ground_friction;
static int use_category, selector_calls, contact_mutation;
static NativeFighterDefinition recipient_policy;
static Fighter_Part resolve_bone(Fighter* f, Fighter_Part part)
{ CHECK(f == &fighter && part == FtPart_TransN); return 1; }
Fighter_Part ftParts_GetBoneIndex(Fighter* f, Fighter_Part part)
{ return resolve_bone(f, part); }
void efAsync_Spawn(HSD_GObj* g, void* q, u32 selector, u32 effect, HSD_JObj* j, ...)
{ CHECK(g == &source && q == &fighter.x60C && j == &joint);
  record(Effect, effect, selector, 0, 0, 0, 0); }
void ftCommon_CalcSelfAccel_Drift(Fighter* f)
{ CHECK(f == &fighter); record(AirPhysics, 1, 0, 0, 0, 0, 0); }
void ft_80084DB0(Fighter_GObj* g)
{ CHECK(g == &source); record(AirPhysics, 2, 0, 0, 0, 0, 0); }
void ft_800851D0(Fighter_GObj* g)
{ CHECK(g == &source); record(AirPhysics, 3, 0, 0, 0, 0, 0); }
void ftCo_FallAerial_Enter(Fighter_GObj* g)
{ CHECK(g == &source); record(Fall, 1, 0, 0, 0, 0, 0); fighter.motion_id = 30; }
void ftCo_Landing_Enter_Basic(Fighter_GObj* g)
{ CHECK(g == &source); record(Finish, 1, 0, 0, 0, 0, 0); fighter.motion_id = 42; }
float ftCo_800D0EC8(Fighter* f) { CHECK(f == &fighter); return landing_threshold; }
float ft_GetGroundFrictionMultiplier(Fighter* f) { CHECK(f == &fighter); return ground_friction; }
bool ft_800827A0(Fighter_GObj* g)
{ CHECK(g == &source); record(GroundProbe, 2, 0, 0, 0, 0, 0); return special_ground_hit; }
bool ftCo_SpecialAir_CheckInput(Fighter_GObj* g)
{ CHECK(g == &source); record(Actions, 1, 0, 0, 0, 0, 0); return special_input; }
bool ftCo_80095328(Fighter_GObj* g, bool* flag)
{ CHECK(g == &source && flag == NULL); record(Actions, 2, 0, 0, 0, 0, 0); return item_throw_input; }
bool ftCo_800CB870(Fighter_GObj* g)
{ CHECK(g == &source); record(Actions, 3, 0, 0, 0, 0, 0); return jump_input; }
void ftCo_800CDDA0(Fighter_GObj* g)
{ CHECK(g == &source); record(Motion, 999, 0, 0, 1, 0, 0); fighter.motion_id = 999; }
enum_t itGetUseKind(Item_GObj* g)
{ CHECK(g == &object); record(Interact, use_category, 0, 0, 0, 0, 0); return use_category; }
float ftCo_GetLStickAngle(Fighter* f)
{ CHECK(f == &fighter); selector_calls++; return atan2f(f->input.lstick[0].y, fabsf(f->input.lstick[0].x)); }
float ftCo_GetCStickAngle(Fighter* f)
{ CHECK(f == &fighter); selector_calls++; return atan2f(f->input.cstick[0].y, fabsf(f->input.cstick[0].x)); }
/* The complete 831CC contact solver is outside this unit. Validate its named
 * continuation and shared fall-special check identity, then select landing. */
void ft_800831CC(Fighter_GObj* g, bool (*check)(Fighter_GObj*, int), HSD_GObjEvent cb)
{ CHECK(g == &source && check == ftCo_80096CC8 && cb != NULL);
  record(AirProbe, 1, 0, 0, 0, 0, 0); if (air_hit) cb(g); }
bool ftCo_80096CC8(Fighter_GObj* g, int arg) { (void)g; (void)arg; unexpected_boundary(); return false; }
void ftCo_LandingAir_EnterWithMsidLag(Fighter_GObj* g, FtMotionId id, float lag)
{ (void)g; (void)id; (void)lag; unexpected_boundary(); }
static float common_attribute(NativeFighterCommonAttribute a)
{
 switch(a) {
 case NativeFighterCommon_TapJumpThreshold: return common_fighter.tap_jump_threshold;
 case NativeFighterCommon_AerialActionDownThreshold: return common_fighter.x88;
 case NativeFighterCommon_AerialJumpReverse: return common_fighter.x78;
 default: unexpected_boundary(); return 0;
 }
}
static int32_t held_id(Fighter_GObj* g, bool auxiliary)
{ CHECK(g == &source && !auxiliary); return fighter.item_gobj == &object ? (int32_t)handle : -1; }
static bool recipient_rebound(Fighter_GObj* g)
{
 CHECK(g == &other); record(Interact, 100, contact_mutation, 0, 0, 0, 0);
 switch(contact_mutation) {
 case 1: fighter.x8_spawnNum++; break;
 case 2: foreign.x8_spawnNum++; break;
 case 3: source.next = NULL; break;
 case 4: heads[HSD_GOBJ_PLINK_FIGHTER] = &other; break;
 case 5: fighter.kind = Ft_Kind_Mario; break;
 case 6: foreign.kind = Ft_Kind_Mario; break;
 case 7: recipient_definition = NULL; break;
 case 8: fighter.unk_gobj = NULL; break;
 }
 return contact_mutation != 9;
}
