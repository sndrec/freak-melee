"""Compile unchanged stock and generated authored Arrow lifecycle bodies."""
import argparse
import hashlib
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("stock", type=Path)
parser.add_argument("model", type=Path)
parser.add_argument("generated", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument("--young-generated", type=Path, required=True)
args = parser.parse_args()

def body(source, name):
    match = re.search(rf"(?m)^(?:static )?(?:inline )?(?:void|bool|float|int|int32_t|Fighter\*|HSD_GObj\*|HSD_JObj\*)(?: inline)?\s+{name}\([^;]*?\)\s*\{{", source)
    if match is None:
        raise AssertionError(f"Missing function {name}")
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]

stock, model, generated = (p.read_text(encoding="utf-8") for p in (args.stock, args.model, args.generated))
joint_path = args.model.parent.parent / "lb/native_model_joint.c"
joint_bytes = joint_path.read_bytes()
joint = joint_bytes.decode("utf-8")
prefix = """#include <melee/ft/native_fighter_module.h>
#include <melee/lb/native_model_joint.h>
#include <melee/it/itCharItems.h>
#include <melee/it/inlines.h>
#include <melee/it/kinds/itlinkarrow.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/iteffect.h>
#include <melee/it/ithitbox.h>
#include <melee/it/itcoll.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpcoll.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/ftCo_Guard.h>
#include <melee/ef/efsync.h>
#include <melee/it/itgroundcoll.h>
#include <melee/ft/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftLink/ftlink.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialn.h>
#include <melee/ft/kinds/ftLink/forward.h>
#include <melee/ft/kinds/ftKirby/ftkirby.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/random.h>
#include <math.h>
#include <string.h>
static const NativeFighterHost* engine;
static union { float f; int32_t i; } script_parameters[64];
static unsigned script_state_base;
"""
unit = prefix
array = re.search(r"(?ms)^f32 it_803F6A84\[\] = \{.*?^\};", stock)
if array is None:
    raise AssertionError("Missing stock vibration table")
unit += array[0] + "\n\n"
for name in ("it_802A8330", "itLinkArrow_Logic98_Reflected",
             "itlinkarrow_inline_bool", "itLinkarrow_UnkMotion4_Anim",
             "itLinkarrow_UnkMotion1_Phys", "it_802A8330_inline",
             "itLinkarrow_UnkMotion1_Anim_inline", "itLinkarrow_UnkMotion1_Anim_inline2",
             "itLinkarrow_UnkMotion1_Anim", "it_802A7E40",
             "itLinkArrow_Logic98_DmgDealt", "itLinkArrow_Logic98_Clanked",
             "itLinkarrow_UnkMotion4_Coll_inline", "itLinkarrow_UnkMotion4_Coll",
             "itLinkArrow_Logic98_HitShield_inline", "itLinkArrow_Logic98_HitShield",
             "itLinkarrow_UnkMotion2_Anim", "itLinkarrow_UnkMotion2_Phys",
             "itLinkarrow_UnkMotion1_Coll_inline", "it_802A9458", "itLinkarrow_UnkMotion1_Coll",
             "itLinkarrow_UnkMotion1_Anim_inline_s_2", "itLinkarrow_UnkMotion0_Anim",
             "itLinkArrow_Logic98_Destroyed", "itLinkArrow_Logic98_EvtUnk",
             "itLinkArrow_Logic98_PickedUp", "it_802A83E0",
             "itLinkArrow_802A850C_inline", "itLinkArrow_802A850C_inline_2",
             "it_802A8C7C", "itLinkArrow_802A850C"):
    unit += body(stock, name) + "\n\n"
for path, names in ((args.stock.parent.parent / "it_2725.c", ("it_80273130", "it_802751D8", "it_80272A18")),
                    (args.stock.parent.parent / "iteffect.c", ("it_802787B4",))):
    source = path.read_text(encoding="utf-8")
    for name in names:
        unit += body(source, name) + "\n\n"
jobj = (args.stock.parents[3] / "sysdolphin/baselib/jobj.c").read_text(encoding="utf-8")
for name in ("HSD_JObjSetFlags", "HSD_JObjClearFlags"):
    unit += body(jobj, name) + "\n\n"
unit += f"// Source SHA256 native_model_joint.c: {hashlib.sha256(joint_bytes).hexdigest()}\n"
for name in ("nativeModelJoint_Resolve", "nativeModelJoint_Read", "nativeModelJoint_Write"):
    unit += body(joint, name) + "\n\n"
for name in ("model_root", "nativeArticleModel_Read",
             "nativeArticleModel_Write", "nativeArticleModel_Destroy", "nativeArticleModel_SetTransform",
             "nativeArticleModel_Load", "nativeArticleModel_SpawnEffectAt"):
    unit += body(model, name) + "\n\n"
fighter = (args.stock.parents[2] / "ft/kinds/ftLink/ftlinkspecialn.c").read_text(encoding="utf-8")
for name in ("ftLk_SpecialN_UnsetArrow", "ftLk_SpecialN_IsActiveAnd2071b6"):
    unit += body(fighter, name) + "\n\n"
for name in ("script_state_set_int", "script_fn_bow_unset_arrow",
             "scripted_article_attached_owner", "scripted_article_owner_motion",
             "scripted_article_owner_state_float"):
    unit += body(generated, name) + "\n\n"
for name in ("scripted_article_state_float", "scripted_article_set_state_float",
             "scripted_article_state_int", "scripted_article_set_state_int",
             "scripted_article_state_bool", "scripted_article_set_state_bool",
             "scripted_article_1_signal_0", "scripted_article_1_logic_8",
             "scripted_article_1_state_4_0", "scripted_article_1_state_1_1", "scripted_article_1_state_1_0",
             "scripted_article_1_signal_1", "scripted_article_1_logic_5", "scripted_article_1_logic_9",
             "scripted_article_1_state_4_2", "scripted_article_1_signal_7",
             "scripted_article_1_state_2_0", "scripted_article_1_state_2_1",
             "scripted_article_1_logic_12", "scripted_article_1_signal_4",
             "scripted_article_1_signal_6", "scripted_article_1_state_1_2",
             "scripted_article_1_state_0_0", "scripted_article_1_logic_1",
             "scripted_article_1_logic_2", "scripted_article_1_logic_13", "scripted_article_1_logic_16",
             "scripted_article_1_signal_2", "scripted_article_1_signal_3", "scripted_article_1_signal_5"):
    unit += body(generated, name) + "\n\n"
unit += body((args.stock.parents[2] / "ft/native_fighter_module.c").read_text(encoding="utf-8"), "item_set_uniform_scale") + "\n"
unit += body(args.young_generated.read_text(encoding="utf-8"), "scripted_article_1_signal_6").replace(
    "scripted_article_1_signal_6(", "scripted_clink_embed(", 1) + "\n"
args.output.write_text(unit, encoding="utf-8", newline="\n")
