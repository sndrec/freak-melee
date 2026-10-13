"""Extract the actual owner-scale/model-scale bodies for a bounded regression."""
from pathlib import Path
import re
import sys


def extract(source, name):
    match = re.search(r'(?m)^void\s+' + re.escape(name) +
                      r'\s*\([^;{}]*\)\s*\{', source)
    if match is None:
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
            if depth == 0:
                return source[match.start():cursor + 1]
        cursor += 1
    raise ValueError(f"Unclosed production function {name}")


def main():
    root, output = map(Path, sys.argv[1:])
    functions = (
        ('src/sysdolphin/baselib/jobj.c', 'HSD_JObjSetMtxDirtySub'),
        ('src/melee/it/it_2725.c', 'it_80272F7C'),
        ('src/melee/it/kinds/itlinkhookshot.c',
         'itLinkHookshot_ApplyOwnerScale'),
    )
    bodies = []
    for relative, name in functions:
        source = (root / relative).read_text(encoding='utf-8')
        bodies.append(f'// Actual {relative}: {name}\n' + extract(source, name))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n\n'.join(bodies) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
