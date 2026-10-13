"""Extract unchanged stock bodies and include the actual generated Boomerang module."""
import argparse
from pathlib import Path
import re

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("stock", type=Path)
p.add_argument("model", type=Path)
p.add_argument("generated", type=Path)
p.add_argument("output", type=Path)
a = p.parse_args()

def body(source, name):
    m = re.search(rf"(?m)^(?:static )?(?:inline )?(?:void|bool|float|f32|int|int32_t|Fighter\*|Fighter_Part|HSD_GObj\*|HSD_JObj\*)(?: inline)?\s+{name}\([^;]*?\)\s*\{{", source)
    if m is None: raise AssertionError(f"Missing original function {name}")
    end, depth = m.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[m.start():end]

stock = a.stock.read_text(encoding="utf-8")
model = a.model.read_text(encoding="utf-8")
unit = "#include <melee/it/kinds/itlinkboomerang.h>\n"
unit += "#include <melee/it/native_article_model.h>\n#include <melee/it/native_article.h>\n"
unit += "#include <melee/lb/lb_00B0.h>\n#include <sysdolphin/baselib/jobj.h>\n"
unit += '#include "' + a.generated.resolve().as_posix() + '"\n'
# Original translation unit, including original inline helpers, state table and
# constructors. Only include directives are removed; bodies remain unchanged.
unit += re.sub(r"(?m)^#include .*\n", "", stock)
for name in ("resolve_joint", "read_joint", "write_joint",
             "nativeArticleModel_Read", "nativeArticleModel_Write",
             "nativeArticleModel_SetTransform", "nativeArticleModel_Destroy",
             "nativeArticleModel_Load", "nativeArticleModel_AddAnimation",
             "nativeArticleModel_RequestAnimation", "nativeArticleModel_AdvanceAnimation",
             "nativeArticleModel_RemoveAnimation", "nativeArticleModel_SetAnimationRate",
             "nativeArticleModel_ReadMatrixRotation"):
    unit += "\n" + body(model, name) + "\n"
fighter = (a.stock.parents[2] / "ft/kinds/ftLink/ftlinkspecials.c").read_text(encoding="utf-8")
unit += "\n#include <melee/ft/kinds/ftLink/forward.h>\n"
unit += "#include <melee/ft/kinds/ftLink/ftlink.h>\n"
unit += "#include <melee/ft/kinds/ftCommon/inlines.h>\n"
unit += "#include <melee/ft/ftanim.h>\n#include <melee/ft/ftparts.h>\n"
unit += "#include <melee/ft/ft_081B.h>\n#include <melee/ft/ft_084E.h>\n#include <melee/ft/ft_0892.h>\n"
unit += "#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>\n"
unit += "#include <melee/ft/kinds/ftCommon/ftCo_Dash.h>\n#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>\n"
unit += "#include <melee/ft/kinds/ftCommon/ftCo_Guard.h>\n#include <melee/ft/kinds/ftCommon/ftCo_Jump.h>\n"
unit += "#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>\n#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>\n"
unit += "#include <melee/ft/kinds/ftCommon/ftCo_SpecialAir.h>\n#include <melee/ft/kinds/ftCommon/ftCo_SpecialS.h>\n"
unit += re.sub(r"(?m)^#include .*\n", "", fighter)
for path, names in (
    (a.stock.parents[2] / "lb/lbvector.c", ("lbVector_Mirror", "lbVector_CosAngle")),
    (a.stock.parents[2] / "ft/kinds/ftLink/ftlink.c", ("ftLk_Init_BoomerangExists",)),
    (a.stock.parents[2] / "ft/kinds/ftLink/ftlinkspecialhi.c", ("ftLk_SpecialHi_GetFv4", "ftLk_SpecialHi_GetPosWithAdjustedY", "ftLk_SpecialHi_ProcessPartLThumbNb")),
):
    source = path.read_text(encoding="utf-8")
    for name in names: unit += "\n" + body(source, name) + "\n"
a.output.write_text(unit, encoding="utf-8", newline="\n")
