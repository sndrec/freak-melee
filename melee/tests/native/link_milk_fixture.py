"""Extract unchanged Milk/query/HSD bodies and include actual generated script C."""
import argparse
import hashlib
from pathlib import Path
import re

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('source_root', type=Path)
p.add_argument('generated', type=Path)
p.add_argument('output', type=Path)
a = p.parse_args()

def function(source, name):
    match = re.search(rf'(?m)^(?:static )?(?:inline )?(?:void|bool|float|f32|int|Fighter\*|HSD_GObj\*|HSD_JObj\*)\s+{name}\([^;]*?\)\s*\{{', source)
    if match is None:
        raise AssertionError(f'Missing original function {name}')
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

unit = '#include "' + a.generated.resolve().as_posix() + '"\n'
unit += '#include <melee/lb/native_model_joint.h>\n'
stock = (a.source_root / 'melee/it/kinds/itclinkmilk.c').read_text(encoding='utf-8')
unit += re.sub(r'(?m)^#include .*\n', '', stock)
for path, names in (
    ('melee/ft/kinds/ftCLink/ftclink.c', ('ftCl_Init_8014920C', 'ftCl_Init_801492C4', 'ftCl_Init_801492F4')),
    ('melee/ft/ftcommon_data.c', ('ftCommon_GetModelScale',)),
    ('melee/ft/ftlib.c', ('ftLib_800869D4',)),
    ('sysdolphin/baselib/jobj.c', ('HSD_JObjSetMtxDirtySub', 'HSD_JObjSetFlags', 'HSD_JObjClearFlags', 'HSD_JObjSetFlagsAll', 'HSD_JObjClearFlagsAll')),
    ('melee/lb/native_model_joint.c', ('nativeModelJoint_Resolve', 'nativeModelJoint_Read', 'nativeModelJoint_Write')),
    ('melee/it/native_article_model.c', ('model_root', 'nativeArticleModel_Read', 'nativeArticleModel_Write', 'nativeArticleModel_ReadDynamic', 'nativeArticleModel_WriteDynamic')),
):
    source_bytes = (a.source_root / path).read_bytes()
    source = source_bytes.decode('utf-8')
    unit += f'\n// Source SHA256 {path}: {hashlib.sha256(source_bytes).hexdigest()}\n'
    for name in names:
        unit += '\n' + function(source, name) + '\n'
a.output.write_text(unit, encoding='utf-8', newline='\n')
