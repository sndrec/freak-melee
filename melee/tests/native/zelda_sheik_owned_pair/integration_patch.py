"""Emit, never apply, the minimal owned base/catalog integration patch."""
import argparse
import difflib
import json
from pathlib import Path
from audit import sha

HERE = Path(__file__).resolve().parent


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot', type=Path, required=True)
    p.add_argument('--census', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    census = json.loads(args.census.read_text())
    assert census['snapshot_sha256']==sha(args.snapshot/'freeze.json')
    patches, inputs = [], {}
    def edit(path, changes):
        source = args.snapshot/'sdk'/path
        before = source.read_text()
        after = before
        for old, new in changes:
            assert after.count(old)==1, (path, old)
            after = after.replace(old, new)
        patches.extend(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
            fromfile='a/'+path, tofile='b/'+path))
        inputs[path]=sha(source)
    edit('src/melee/ft/native_fighter_runtime.c', [
        ('#include <melee/ft/kinds/ftGameWatch/ftgamewatch_clone.h>\n',
         '#include <melee/ft/kinds/ftGameWatch/ftgamewatch_clone.h>\n'
         '#include <melee/ft/kinds/ftZelda/ftzelda_clone.h>\n'
         '#include <melee/ft/kinds/ftSeak/ftsheik_clone.h>\n'),
        ('    { Ft_Kind_GameWatch, &ftGameWatchClone_Definition },\n',
         '    { Ft_Kind_GameWatch, &ftGameWatchClone_Definition },\n'
         '    { Ft_Kind_Zelda, &ftZeldaClone_Definition },\n'
         '    { Ft_Kind_Seak, &ftSheikClone_Definition },\n')])
    edit('src/melee/ft/native_fighter_project.c', [
        ('    case Ft_Kind_GameWatch: return "Game & Watch";\n',
         '    case Ft_Kind_GameWatch: return "Game & Watch";\n'
         '    case Ft_Kind_Zelda: return "Zelda";\n'
         '    case Ft_Kind_Seak: return "Sheik";\n')])
    edit('src/melee/ft/native_fighter_article_bindings.c', [
        ('    SLOT(Ft_Kind_Yoshi, It_Kind_Yoshi_EggThrow, 0, -1),\n',
         ''.join(f'    SLOT(Ft_Kind_{"Zelda" if member=="zelda" else "Seak"}, {kind}, {slot}, -1),\n'
             for member in ('zelda','sheik') for kind,slot in census['members'][member]['articles'].items())+
         '    SLOT(Ft_Kind_Yoshi, It_Kind_Yoshi_EggThrow, 0, -1),\n')])
    edit('tools/native_content_tool.c', [
        ('|Peach|\\"Game & Watch\\" PROJECT_DIRECTORY TITLE',
         '|Peach|\\"Game & Watch\\"|Zelda|Sheik PROJECT_DIRECTORY TITLE')])
    for member, folder in (('zelda','ftZelda'),('sheik','ftSeak')):
        for suffix in ('c','h'):
            path = f'src/melee/ft/kinds/{folder}/ft{member}_clone.{suffix}'
            old = args.snapshot/'sdk'/path
            before = old.read_text() if old.exists() else ''
            new = HERE/'bases'/f'ft{member}_clone.{suffix}'
            patches.extend(difflib.unified_diff(before.splitlines(True), new.read_text().splitlines(True),
                fromfile='a/'+path if old.exists() else '/dev/null', tofile='b/'+path))
            inputs[path]=sha(old) if old.exists() else None
    args.output.write_text(''.join(patches), newline='\n')
    args.output.with_suffix('.json').write_text(json.dumps({'status':'PROPOSED ONLY; no defaults or shared sources edited',
        'snapshot_sha256':sha(args.snapshot/'freeze.json'), 'census_sha256':sha(args.census),
        'source_inputs':inputs, 'patch_sha256':sha(args.output),
        'base_inputs':{p.name:sha(p) for p in (HERE/'bases').iterdir() if p.is_file()}}, indent=2)+'\n', newline='\n')
    print(sha(args.output))


if __name__=='__main__':
    main()
