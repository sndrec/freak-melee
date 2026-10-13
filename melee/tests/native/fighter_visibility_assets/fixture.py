"""Extract real typed visibility import/load-context bodies for a console fixture."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from item_owner_fixture import body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    blocks, inputs = [], {}
    asset_path = 'src/melee/ft/ftcharacter_assets.c'
    source = (args.root / asset_path).read_text()
    for name in ['fail', 'resolve', 'local', 'read_s32',
                 'import_visibility_lookups', 'count_model_draws',
                 'ftCharacterAssets_ValidateVisibility',
                 'ftCharacterAssetLayout_IsValid', 'import_fighter_visibility']:
        blocks.append('// Actual ' + asset_path + '\n' + body(source, name))
    runtime_path = 'src/melee/ft/native_fighter_runtime.c'
    source = (args.root / runtime_path).read_text()
    blocks.append('typedef struct NativeFighterFamily NativeFighterFamily;')
    for name in ['NativeFighterSlot', 'NativeFighterLoadContext']:
        match = re.search(r'typedef struct ' + name + r'\s*\{.*?\}\s*' + name + r';',
                          source, re.S)
        if match is None:
            raise ValueError(name)
        blocks.append(match.group(0))
    blocks += [re.search(r'enum\s*\{\s*NATIVE_FIGHTER_FAMILY_CAPACITY\s*=.*?\};',source,re.S)[0],
               'static NativeFighterSlot slot_banks[2][NATIVE_FIGHTER_ACTOR_CAPACITY];',
               'static NativeFighterSlot* slots = slot_banks[0];',
               'static NativeFighterLoadContext load_contexts[NATIVE_CALLBACK_DISPATCH_LIMIT];',
               'static unsigned load_context_count;']
    for name in ['release_slot', 'nativeFighterRuntime_Definition',
                 'nativeFighterRuntime_EnterLoad', 'nativeFighterRuntime_LeaveLoad',
                 'nativeFighterRuntime_InstallOutlineVisibility']:
        blocks.append('// Actual ' + runtime_path + '\n' + body(source, name))
    for path in [asset_path, runtime_path, 'src/melee/ft/ftcharacter_assets.h',
                 'src/melee/ft/ftcharacter_asset_layout.h',
                 'src/melee/ft/native_fighter_runtime.h',
                 'src/melee/ft/native_callback_lifetime.c',
                 'src/melee/ft/native_callback_lifetime.h',
                 'src/melee/ft/fighter.c']:
        inputs[path] = hashlib.sha256((args.root / path).read_bytes()).hexdigest()
    (args.output / 'visibility_actual.inc').write_text('\n\n'.join(blocks) + '\n')
    (args.output / 'source.json').write_text(json.dumps({
        'inputs': inputs,
        'scope': 'Actual import decoder, descriptor validation, private slot retirement, '
                 'load contexts and installation; actual creation-ID registry. '
                 'Slot/cache binding and retirement cleanup sinks controlled. '
                 'Full package loader, Fighter_Create and renderer not executed.'
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
