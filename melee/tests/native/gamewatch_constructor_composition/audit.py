"""Read-only SDK61 constructor census; no compilation or gameplay."""
import argparse, hashlib, json, re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
ROWS = [
    ('Greenhouse', 'itGamewatchGreenhouse_Spawn', 4, 'attached'),
    ('Manhole', 'it_802C65E4', 1, 'attached'),
    ('Fire', 'itGamewatchFire_Spawn', 1, 'attached'),
    ('Parachute', 'it_802C6C38', 2, 'attached'),
    ('Turtle', 'it_802C6F40', 2, 'attached'),
    ('Breath', 'it_802C720C', 2, 'attached'),
    ('Judge', 'it_802C7774', 1, 'attached_judge'),
    ('Panic', 'it_802C7D60', 2, 'attached'),
    ('Chef', 'it_802C837C', 2, 'unattached_food'),
    ('Rescue', 'it_802C8038', 2, 'unattached_rescue'),
]
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def extract(text, name):
    pattern = (r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'
               + r'\(?\s*' + re.escape(name) + r'\s*\)?\s*\([^;{}]*\)\s*\{')
    match = re.search(pattern, text)
    if not match:
        raise ValueError('missing actual definition: ' + name)
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end], text.count('\n', 0, match.start()) + 1

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    snapshot, out = args.snapshot.resolve(), args.output.resolve()
    if out.exists() or snapshot == out or snapshot in out.parents or HERE in out.parents:
        parser.error('fresh output outside source/snapshot required')
    frozen = json.loads((snapshot / 'freeze.json').read_text())
    files = {}
    def source(relative):
        path = snapshot / 'sdk' / relative
        key = str(path.relative_to(snapshot)).replace('/', '\\')
        digest = sha(path)
        if frozen.get(key) != digest:
            raise ValueError('snapshot manifest mismatch: ' + key)
        live = ROOT / relative
        files[relative] = {'snapshot_sha256': digest,
                           'live_sha256': sha(live) if live.exists() else None,
                           'live_matches_snapshot': live.exists() and sha(live) == digest}
        return path.read_text()
    onload = source('src/melee/ft/kinds/ftGameWatch/ftgamewatch.c')
    load_body, _ = extract(onload, 'ftGw_Init_OnLoad')
    registrations = re.findall(r'it_8026B3F8\(items\[(\d+)\], It_Kind_GameWatch_(\w+)\)', load_body)
    assert registrations == [(str(i), row[0]) for i, row in enumerate(ROWS)]
    rows = []
    for slot, (name, symbol, count, family) in enumerate(ROWS):
        relative = 'src/melee/it/kinds/itgamewatch' + name.lower() + '.c'
        text = source(relative)
        body, line = extract(text, symbol)
        table = re.search(r'ItemStateTable\s+\w+\[\]\s*=\s*\{(.*?)\};', text, re.S)
        assert table and len(re.findall(r'\{\s*\d+', table[1])) == count
        calls = re.findall(r'\b([A-Za-z_]\w*)\s*\(', body[body.index('{'):])
        calls = [x for x in calls if x not in ('if', 'while', 'for', 'sizeof')]
        assert calls.count('it_8027CE64') == 1
        rows.append({'kind': 'It_Kind_GameWatch_' + name, 'slot': slot,
                     'source': relative, 'constructor': symbol,
                     'start_line': line, 'end_line': line + body.count('\n'),
                     'body_sha256': hashlib.sha256(body.encode()).hexdigest(),
                     'state_count': count, 'family': family, 'ordered_calls': calls})
    helpers = [
        ('src/melee/it/itzako.c', ['it_8027CE64']),
        ('src/melee/ft/ftlib.c', ['ftLib_8008770C', 'ftLib_80087744', 'ftLib_800870BC']),
        ('src/melee/ft/kinds/ftGameWatch/ftgamewatch.c', ['ftGw_Init_8014A7F4', 'ftGw_Init_8014A814']),
        ('src/melee/it/kinds/itgamewatchjudge.c', ['it_802C78B8', 'it_2725_Logic77_PickedUp']),
        ('src/melee/it/kinds/itgamewatchchef.c', ['it_802C84A0']),
        ('src/melee/it/kinds/itgamewatchrescue.c', ['it_802C8208']),
        ('src/melee/it/article_model_layers.c', ['articleModelLayers_Bind']),
        ('src/melee/it/native_article.c', ['nativeArticle_Creator', 'nativeArticle_Create']),
        ('src/melee/it/it_2725.c', ['it_80273670', 'it_80274594']),
        ('src/melee/it/it_26B1.c', ['it_8026BB68']),
    ]
    helper_rows = []
    for relative, symbols in helpers:
        text = source(relative)
        for symbol in symbols:
            body, line = extract(text, symbol)
            helper_rows.append({'source': relative, 'symbol': symbol, 'start_line': line,
                                'end_line': line + body.count('\n'),
                                'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    for relative in ['src/melee/it/kinds/inlines.h', 'src/melee/it/itdraw.c',
                     'src/melee/it/itarticle_data.c', 'src/melee/ft/ftcharacter_assets.c',
                     'src/melee/ft/native_fighter_module.c', 'src/melee/it/itCharItems.h',
                     'src/melee/ft/native_fighter_article_bindings.c',
                     'src/melee/it/item.c', 'src/melee/db/dbitem.c',
                     'src/melee/it/kinds/itkirbygamewatchchefpan.c',
                     'tools/native_fighter_item_script.c', 'tools/native_fighter_item_operations.c',
                     'tools/native_fighter_script_functions.c', 'tools/native_fighter_script_functions.h',
                     'tools/native_fighter_item_functions.h', 'tools/native_fighter_script_colors.c',
                     'tools/native_fighter_script_article_refs.h', 'tools/native_fighter_script_dependencies.h']:
        source(relative)
    for relative in sorted((snapshot / 'sdk/src/melee/ft/kinds/ftGameWatch').glob('*.c')):
        if relative.name != 'ftgamewatch.c':
            source(str(relative.relative_to(snapshot / 'sdk')).replace('\\', '/'))
    accepted = [
        'gamewatch_load_composition/candidate.fms',
        'gamewatch_normals_composition/props.fms',
        'gamewatch_normals_composition/pending_entries.fms',
        'gamewatch_normals_composition/fighter_parameters.fmp',
        'gamewatch_specials_composition/articles.fms',
        'gamewatch_specials_composition/pending.fms',
        'gamewatch_specials_composition/fighter_parameters.fmp',
        'gamewatch_specials_contracts/compose.py',
        'gamewatch_chef_composition/candidate.fms',
    ]
    accepted_hashes = {name: sha(HERE.parent / name) for name in accepted}
    out.mkdir(parents=True)
    inventory = {'status': 'read-only source census; no callback comparison or compilation',
                 'snapshot': str(snapshot), 'freeze_manifest_sha256': sha(snapshot / 'freeze.json'),
                 'constructors': rows, 'actual_helper_bodies': helper_rows,
                 'required_extension': 'typed creator.call/owner.call expression results; no new Host/palette storage',
                 'accepted_inputs': accepted_hashes, 'source_inputs': files,
                 'drafts': {x.name: sha(x) for x in HERE.glob('*.fms')}}
    (out / 'inventory.json').write_text(json.dumps(inventory, indent=2) + '\n', newline='\n')
    print(json.dumps({'constructors': len(rows), 'helper_bodies': len(helper_rows),
                      'source_files': len(files), 'inventory': str(out / 'inventory.json')}))
if __name__ == '__main__':
    main()
