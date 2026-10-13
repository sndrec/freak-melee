"""Extract actual common entry continuations and model-group reads."""
from pathlib import Path
import argparse
import hashlib
import json
import re
from item_owner_fixture import body

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    sources = {
        'src/melee/ft/ft_08A1.c': ['ft_8008A348'],
        'src/melee/ft/kinds/ftCommon/ftCo_SquatWait.c': ['ftCo_SquatWait_Enter_inline'],
        'src/melee/ft/kinds/ftCommon/ftCo_AttackAir.c': ['ftCo_AttackAir_EnterFromMsid'],
        'src/melee/ft/native_fighter_visuals.c': ['nativeFighterVisuals_CurrentGroup',
                                              'nativeFighterVisuals_PreviousGroup'],
    }
    blocks, inputs = [], {}
    for path, names in sources.items():
        source = args.root / path
        text = source.read_text(encoding='utf-8')
        inputs[path] = hashlib.sha256(source.read_bytes()).hexdigest()
        blocks.extend(body(text, name) for name in names)
    # Static helper names repeat across translation units; rename each directly
    # for this shared fixture TU, without changing any extracted body logic.
    for family in ('S4', 'Hi4', 'Lw4'):
        path = f'src/melee/ft/kinds/ftCommon/ftCo_Attack{family}.c'
        source = args.root / path
        text = source.read_text(encoding='utf-8')
        inputs[path] = hashlib.sha256(source.read_bytes()).hexdigest()
        if family == 'S4':
            declaration = re.search(r'typedef enum cmd_var_idx\s*\{[^}]*\}\s*cmd_var_idx;',text)
            if declaration is None:
                raise RuntimeError('S4 command enum missing')
            blocks.append(declaration.group(0))
        names = ['doEnter'] + (['decideFighter'] if family == 'S4' else [])
        for name in names:
            actual = body(text, name)
            actual = re.sub(r'\bdoEnter\b', f'common_{family}_enter', actual)
            actual = re.sub(r'\bdecideFighter\b', f'common_{family}_decide', actual)
            blocks.append('// Actual ' + path + '; TU-local symbol names only renamed.\n' + actual)
    # Exact grounded prefix through the first common flag reset only. This
    # deliberately excludes earlier/later motion-transition engine work.
    path = 'src/melee/ft/fighter.c'
    source = args.root / path
    text = source.read_text(encoding='utf-8')
    transition = body(text, 'Fighter_ChangeMotionState')
    start = transition.index('if (fp->ground_or_air == GA_Ground) {')
    marker = 'fp->x2221_b5 = false;'
    end = transition.index(marker, start) + len(marker)
    fragment = transition[start:end]
    if fragment.count('on_grounded_motion_setup(gobj)') != 1:
        raise RuntimeError('grounded prefix extraction drift')
    inputs[path] = hashlib.sha256(source.read_bytes()).hexdigest()
    inputs['grounded_prefix_sha256'] = hashlib.sha256(fragment.encode()).hexdigest()
    blocks.append('// Actual Fighter_ChangeMotionState grounded prefix only.\n'
                  '// Harness locals mirror the actual transition entry; no full transition claim.\n'
                  'static void common_entry_grounded_fragment(Fighter_GObj* gobj) {\n'
                  '    Fighter* fp = GET_FIGHTER(gobj);\n'
                  '    const uint32_t callback_generation = (uint32_t)fp->x8_spawnNum;\n' +
                  fragment + '\n    }\n}\n')
    (args.output / 'common_entry_actual.inc').write_text('\n\n'.join(blocks) + '\n', encoding='utf-8')
    (args.output / 'inputs.json').write_text(json.dumps(inputs, indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
