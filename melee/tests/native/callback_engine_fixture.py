"""Extract actual production dispatch bodies for the isolated lifetime fixture."""
from pathlib import Path
import re
import sys

def function(source, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
                      re.escape(name) + r'\s*\([^;{}]*\)\s*\{', source)
    if not match:
        raise ValueError(f"Missing production function {name}")
    cursor = source.index('{', match.start())
    depth = 0
    while cursor < len(source):
        if source.startswith('//', cursor):
            end = source.find('\n', cursor)
            cursor = len(source) if end < 0 else end
            continue
        if source.startswith('/*', cursor):
            cursor = source.index('*/', cursor + 2) + 2
            continue
        if source[cursor] in "\"'":
            quote = source[cursor]
            cursor += 1
            while cursor < len(source) and source[cursor] != quote:
                cursor += 2 if source[cursor] == '\\' else 1
            cursor += 1
            continue
        if source[cursor] == '{':
            depth += 1
        elif source[cursor] == '}':
            depth -= 1
            if not depth:
                return source[match.start():cursor + 1]
        cursor += 1
    raise ValueError(f"Unclosed production function {name}")

def main():
    root, output = map(Path, sys.argv[1:])
    groups = {
        'src/melee/it/inlines.h': ['GetItemData'],
        'src/melee/it/item.c': [
            'itemCallbackCurrent','processSourceCallback','processCallback', 'Item_80269978', 'Item_80269A9C',
            'func_8026A158_helper', 'Item_8026A158',
            'func_8026A1E8_inline', 'Item_8026A1E8',
            'OnTakeDamageThink', 'OnClankThink', 'OnGiveDamageThink',
            'EnterHitlagThink', 'checkHitLag', 'Item_8026A294',
            'Item_8026AB54', 'Item_8026ADC0', 'Item_80269BE4',
            'Item_8026A8EC'],
        'src/melee/ft/ftcommon.c': ['ftCommon_8007DB58'],
        'src/melee/ft/kinds/ftCommon/ftpickupitem.c': ['ftpickupitem_80094818'],
        'src/melee/ft/fighter_physics.c': ['Fighter_UpdatePosition'],
        'src/melee/ft/fighter.c': ['Fighter_UnkProcessGrab_8006CA5C',
                                 'Fighter_UnkProcessDeath_80068354'],
        'src/melee/it/it_2725.c': ['it_80273F34', 'it_802750F8'],
    }
    bodies = []
    for relative, names in groups.items():
        source = (root / relative).read_text(encoding='utf-8')
        for name in names:
            bodies.append(f'// Actual {relative}: {name}\n' + function(source, name))
    output.write_text('\n\n'.join(bodies) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
