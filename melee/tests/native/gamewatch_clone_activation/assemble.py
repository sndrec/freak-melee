"""Assemble full unregistered Game & Watch import source from accepted inputs.

No accepted component, SDK, installed project or game is modified. Output is
an import draft; normal clone/import validation requires the copied base.
"""
import argparse
import hashlib
import importlib.util
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

# Exact ftGameWatchAttributes fields, independently readable in types.h.
FIGHTER_FIELDS = {
    'chef_loop_frame': ('float', 0x18), 'chef_max_food': ('float', 0x1C),
    'judge_preserve': ('float', 0x20), 'judge_decel': ('float', 0x24),
    'judge_impulse': ('float', 0x28), 'judge_gravity': ('float', 0x2C),
    'judge_terminal': ('float', 0x30), 'rescue_stick': ('float', 0x58),
    'rescue_angle': ('float', 0x5C), 'rescue_landing': ('float', 0x60),
    'panic_preserve': ('float', 0x64), 'panic_decel': ('float', 0x68),
    'panic_gravity': ('float', 0x6C), 'panic_terminal': ('float', 0x70),
    'panic_damage_add': ('float', 0x74), 'panic_damage_mul': ('float', 0x78),
    'panic_turn_frames': ('float', 0x7C), 'panic_absorb_part': ('int', 0x80),
    'panic_absorb_x': ('float', 0x84), 'panic_absorb_y': ('float', 0x88),
    'panic_absorb_z': ('float', 0x8C), 'panic_absorb_radius': ('float', 0x90),
    'model_width': ('float', 0),
}
for i in range(9):
    FIGHTER_FIELDS[f'judge_weight_{i}'] = ('int', 0x34 + 4*i)
for i in range(4):
    for j, c in enumerate('rgba'):
        FIGHTER_FIELDS[f'body_{i}_{c}'] = ('byte', 4 + 4*i + j)
for j, c in enumerate('rgba'):
    FIGHTER_FIELDS[f'outline_{c}'] = ('byte', 0x14 + j)

FOOD_FIELDS = {'food_wall_bounce': 4, 'food_active_lifetime': 8,
               'food_inactive_lifetime': 12}
for i in range(5):
    for j, channel in enumerate(('horizontal', 'vertical', 'gravity', 'terminal', 'angular')):
        FOOD_FIELDS[f'food_{i}_{channel}'] = 16 + 20*i + 4*j


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def assemble(snapshot, output, iso):
    frozen = json.loads((snapshot / 'freeze.json').read_text())
    for name, digest in frozen.items():
        if sha(snapshot / name) != digest:
            raise ValueError('snapshot changed: ' + name)
    sys.path.insert(0, str(snapshot / 'sdk/tools'))
    from extract_native_animation import extract_clip
    dat, root = extract_clip(iso, 'PlGw.dat', 'ftDataGamewatch')
    accepted = HERE.parent / 'gamewatch_constructor_composition/run.py'
    sys.path.insert(0, str(accepted.parent))
    spec = importlib.util.spec_from_file_location('accepted_constructors', accepted)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    output.mkdir(parents=True)
    provisional = module.compose(output, dat, root)
    text = provisional.read_text()
    entries = (HERE.parent / 'gamewatch_normals_composition/pending_entries.fms').read_text()
    entries = re.sub(r'(?m)^#.*\n', '', entries)
    entries = entries.replace('items.try_pickup()', 'items.try_ground_pickup()')
    entries = entries.replace('facing.flip()', 'facing.direction = -facing.direction')
    policies, helpers = entries.split('func GreenhouseSetup():', 1)
    helpers = 'func GreenhouseSetup():' + helpers
    # Place normal helpers before all move/action/article blocks. Helper calls
    # remain direct; the compiler resolves the existing forward DAG.
    first = re.search(r'(?m)^(move |action |article )', text).start()
    text = text[:first] + helpers + '\n' + text[first:]
    load = ('        visuals.preserve_secondary_material = true\n'
            '        model.instance_scale_z = attrs.model_width\n'
            '        visuals.outline_color = PaperOutlineColor()\n'
            '        visuals.set_diffuse_color(PaperBodyColor())\n'
            '        visuals.install_outline_visibility(10)\n')
    old = '        # Palette/outline/width, layered renderer and asset binding remain pending.\n'
    assert text.count(old) == 1
    text = text.replace(old, load)
    # Stock COPY_ATTRS refreshes the fighter extended attribute block. Keep
    # that actual generic reload operation even with owned authored tuning.
    marker = 'move Lifecycle:' if 'move Lifecycle:' in text else 'action Lifecycle:'
    assert text.count(marker) == 1
    text = text.replace(marker, marker + '\n    on load_special_attrs:\n        fighter.reload_ext_attributes()')
    # The common Landing entry resets original x2234 after shared setup.
    text += ('\naction GroundLandingPolicy:\n    on landing_action:\n'
             '        state.judge_impulse_used = false\n')
    # Accepted tests used a synthetic rig; normal import uses actual ordinal
    # mappings from PlCo.dat: LThumbNb17, RThumbNb32.
    overrides = {'chef_spawn_joint': 17, 'judge_attachment_part': 32}
    recipes, declarations = {}, []
    values = (output / 'fighter_parameters.fmp').read_text().splitlines()
    for row in values:
        kind, name, equal, value = row.split()
        assert equal == '='
        if name in FOOD_FIELDS:
            recipes[name] = {'scope': 'It_Kind_GameWatch_Chef', 'type': kind,
                             'offset': FOOD_FIELDS[name]}
        elif name in FIGHTER_FIELDS:
            imported_type, offset = FIGHTER_FIELDS[name]
            declarations.append(f'attribute {imported_type} {name} {offset}')
            recipes[name] = {'scope': 'fighter', 'type': imported_type, 'offset': offset}
        else:
            value = str(overrides.get(name, value))
            declarations.append(f'parameter {kind} {name} = {value}')
            recipes[name] = {'scope': 'owned literal', 'type': kind, 'value': value}
    assert set(FIGHTER_FIELDS) <= set(recipes)
    assert set(FOOD_FIELDS) <= set(recipes)
    food = 'article It_Kind_GameWatch_Chef states 2:\n'
    assert text.count(food) == 1
    text = text.replace(food, food + ''.join(
        f'    attribute float {name} {offset}\n' for name, offset in FOOD_FIELDS.items()))
    for name in FOOD_FIELDS:
        text = text.replace('attrs.' + name, 'item.attr.' + name)
    # A base-prefixed complete fighter uses the explicit-state grammar. Actions
    # become named move groups; Chef's two source motions are353/354 rather
    # than the special-only generation route's ground/air phase shorthand.
    text = re.sub(r'(?m)^action ', 'move ', text)
    text = text.replace('    entry explicit\n', '')
    chef = re.search(r'(?ms)^move Chef:\n.*?(?=^move |^article |\Z)', text)
    assert chef
    body = chef[0]
    for family, motion in [('ground', 353), ('air', 354)]:
        for phase in ['animation', 'interrupt', 'physics', 'collision']:
            body = body.replace('    on ' + family + '_' + phase + ':',
                                f'    on state_{motion}_{phase}:')
    text = text[:chef.start()] + body + text[chef.end():]
    # Remove stale historical fixture scope comments, retaining actual rationale.
    text = '\n'.join(line for line in text.splitlines() if not (
        line.startswith('# UNREGISTERED') or line.startswith('# SDK56') or
        line.startswith('# Paper ') or line.startswith('# UNCOMPILED') or
        'synthetic base only' in line)) + '\n'
    header = ('# Full Game & Watch composition; unregistered pending closure gate.\n'
              '# Import recipes derive from actual stock fields; literals name source constants.\n'
              'fighter:\n    behavior independent\n\n'
              'base ftDataGamewatch 341 40\nvisibility_asset 10\n' + policies + '\n')
    target = output / 'gamewatch_full.fms'
    target.write_text(header + '\n'.join(declarations) + '\n\n' + text, newline='\n')
    # Controlled comparison FMP is not a normal project's tuning input.
    provisional.unlink()
    (output / 'fighter_parameters.fmp').unlink()
    (output / 'parameter_recipes.json').write_text(json.dumps(recipes, indent=2) + '\n', newline='\n')
    inputs = {str(p.relative_to(ROOT)): sha(p)
              for directory in ('gamewatch_constructor_composition', 'gamewatch_normals_composition',
                                'gamewatch_chef_composition', 'gamewatch_specials_composition',
                                'gamewatch_specials_contracts')
              for p in (HERE.parent / directory).iterdir() if p.is_file()}
    receipt = {'status': 'ASSEMBLED; normal import and new normal-phase comparison pending',
               'snapshot': str(snapshot), 'freeze_sha256': sha(snapshot / 'freeze.json'),
               'PlGw.dat_sha256': hashlib.sha256(dat).hexdigest(),
               'accepted_inputs': inputs, 'source': str(target), 'source_sha256': sha(target),
               'source_lines': len(target.read_text().splitlines()),
               'fighter_recipes': len(FIGHTER_FIELDS), 'chef_recipes': len(FOOD_FIELDS),
               'parameter_count': len(recipes)}
    (output / 'assembly.json').write_text(json.dumps(receipt, indent=2) + '\n', newline='\n')
    return receipt


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--snapshot', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--iso', type=Path, default=ROOT.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso')
    args = ap.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if output.exists() or any(p == output or p in output.parents for p in (snapshot, HERE)):
        ap.error('fresh output outside immutable snapshot and source namespace required')
    print(json.dumps(assemble(snapshot, output, args.iso), indent=2))
