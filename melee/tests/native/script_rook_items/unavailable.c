#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_item_behavior.h>
#include <melee/it/item.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itgroundcoll.h>
#include <melee/lb/lb_00B0.h>
#include <stdlib.h>
// Only current Item/Food routes are connected. Unrelated presentation/Fighter
// leaves abort rather than invent native success or an authored surrogate.
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* o){(void)o;return NativeScriptVM_SourceRetired;}
void it_8026B724(HSD_GObj* o){(void)o;abort();}
void it_8026B73C(HSD_GObj* o){(void)o;abort();}
void it_80274574(HSD_GObj* o){(void)o;abort();}
bool it_8026B894(HSD_GObj* o,HSD_GObj* other){(void)o;(void)other;abort();}
bool it_8027781C(HSD_GObj* o){(void)o;abort();}
bool it_802751D8(HSD_GObj* o){(void)o;abort();}
bool it_8026D9A0(HSD_GObj* o){(void)o;abort();}
void it_8026B3A8(HSD_GObj* o){(void)o;abort();}
void Item_8026AE84(Item* i,enum_t sound,u8 pan,u8 volume){(void)i;(void)sound;(void)pan;(void)volume;abort();}
void Item_8026AF0C(Item* i,enum_t sound,u8 pan,u8 volume){(void)i;(void)sound;(void)pan;(void)volume;abort();}
void it_8026BB68(HSD_GObj* o,Vec3* v){(void)o;(void)v;abort();}
void ftLib_80086990(Fighter_GObj* o,Vec3* v){(void)o;(void)v;abort();}
void lb_8000B1CC(HSD_JObj* j,Vec3* offset,Vec3* result){(void)j;(void)offset;(void)result;abort();}
Fighter_Part ftParts_GetBoneIndex(Fighter* fp,Fighter_Part part){(void)fp;(void)part;abort();}
uint32_t nativeFighterTarget_Id(Fighter_GObj* o){(void)o;abort();}
void ftpickupitem_80094818(Fighter_GObj* o,bool notify){(void)o;(void)notify;abort();}
bool nativeItemBehavior_EngineAssetsAvailable(ItemKind kind){return kind==It_Kind_Foods;}
void it_802750F8(HSD_GObj* o){(void)o;abort();}
void it_802756D0(HSD_GObj* o){(void)o;abort();}
Article* nativeFighterRuntime_ArticleAssets(FighterKind author,ItemKind kind){(void)author;(void)kind;return NULL;}
