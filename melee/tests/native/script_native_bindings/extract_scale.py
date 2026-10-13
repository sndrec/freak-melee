"""Extract shared field queries and the native signed float-direction test."""
import argparse, re
from pathlib import Path

def extract(root, output):
    source = (Path(root) / 'src/melee/ft/ftcommon_data.c').read_text()
    match = re.search(r'^float ftCommon_GetModelScale\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('missing shared model-scale body')
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    destination = Path(output)
    destination.parent.mkdir(parents=True, exist_ok=True)
    scale = source[match.start():end].replace('ftCommon_GetModelScale', 'reference_model_scale')
    source = (Path(root) / 'src/melee/ft/inlines.h').read_text()
    match = re.search(r'^static inline int ftGetFacingDirInt\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('missing shared facing-sign body')
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    facing = source[match.start():end].replace('ftGetFacingDirInt', 'reference_facing_sign')
    source = (Path(root) / 'src/melee/ft/kinds/ftPeach/ftpeachfloatfall.c').read_text()
    match = re.search(r'^static ftPeach_MotionState getFloatDir\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('missing native float-direction body')
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    direction = source[match.start():end].replace('getFloatDir', 'reference_float_direction')
    extra = []
    for path, declaration, old, new in [
        ('src/melee/ft/native_fighter_module.c', r'^static float fighter_root_scale_y\([^;{}]*\)\s*\{', 'fighter_root_scale_y', 'reference_root_scale_y'),
        ('src/sysdolphin/baselib/class.c', r'^bool hsdObjIsDescendantOf\([^;{}]*\)\s*\{', 'hsdObjIsDescendantOf', 'hsdObjIsDescendantOf'),
    ]:
        source = (Path(root) / path).read_text()
        match = re.search(declaration, source, re.M)
        if not match:
            raise RuntimeError('missing native root/class body: ' + path)
        end, depth = match.end(), 1
        while depth:
            depth += (source[end] == '{') - (source[end] == '}')
            end += 1
        extra.append(source[match.start():end].replace(old, new))
    destination.write_text(scale + '\n' + facing + '\n' + direction + '\n' + '\n'.join(extra) + '\n')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    extract(args.root, args.output)
