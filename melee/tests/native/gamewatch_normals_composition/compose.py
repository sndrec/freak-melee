"""Compose immutable accepted components plus the new unregistered normal props.

No stock handlers or renderer are substituted. The SDK57 comparison excludes
only Manhole's cmd0!=0 wind branch, whose two missing operations are documented.
"""
from pathlib import Path
import hashlib
import json
import re
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def split(source):
    """Keep declaration order while moving all globals before actual functions."""
    pieces = re.split(r'(?m)(?=^(?:func |export func |move |action |article ))', source)
    return pieces[0], [p for p in pieces[1:] if p.startswith(('func ', 'export func '))], [
        p for p in pieces[1:] if not p.startswith(('func ', 'export func '))]


def compose(output, compare=False):
    output = Path(output)
    # Import this accepted composer under its own file name, avoiding our name.
    import importlib.util
    path = HERE.parent / 'gamewatch_specials_contracts/compose.py'
    spec = importlib.util.spec_from_file_location('accepted_specials', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    special_path = module.compose(output / 'specials')
    chef_path = HERE.parent / 'gamewatch_chef_composition/candidate.fms'
    sources = [chef_path.read_text(), special_path.read_text(), (HERE / 'fighter.fms').read_text()]
    globals_, functions, callbacks = [], [], []
    for source in sources:
        declaration, helpers, actions = split(source)
        globals_.append(declaration)
        functions += helpers
        callbacks += actions
    functions = [p for p in functions if not p.startswith('func SpecialDamage():')]
    # Adopt original Rescue explicit detach-before-remove composition. Its old
    # isolated helper is replaced here, not mutated in the accepted component.
    functions = [p if not p.startswith('func RescueRemove():') else
        'func RescueRemove():\n    if state.rescue_handle != -1:\n'
        '        articles.signal_owned(state.rescue_handle, "Remove", 0)\n'
        '        RescueClear()\n' for p in functions]
    callbacks = [p.replace('SpecialDamage', 'PropDamage') for p in callbacks]
    functions = [p.replace('SpecialDamage', 'PropDamage') for p in functions]
    callbacks = [p for p in callbacks if not p.startswith('action ChefHistory:')]
    text = '\n'.join(globals_ + functions +
        [p for p in callbacks if p.startswith('move ')] +
        [p for p in callbacks if p.startswith('action ')] +
        [p for p in callbacks if p.startswith('article ')])
    props = (HERE / 'props.fms').read_text()
    if compare:
        # No cmd0!=0 case enters these unresolved statements in the fixture.
        # Keep the complete source draft; never pretend item.pos is child world.
        props = re.sub(r'(?m)^                item\.(joint_world_position|wind_at_coordinates)\(.*\)$',
                       '                pass', props)
    text = text.rstrip() + '\n' + props.rstrip() + '\n'
    parameters = (HERE.parent / 'gamewatch_chef_composition/fighter_parameters.fmp').read_text()
    parameters += (output / 'specials/fighter_parameters.fmp').read_text()
    parameters += (HERE / 'fighter_parameters.fmp').read_text()
    names = [line.split()[1] for line in parameters.splitlines()]
    assert len(names) == len(set(names)), 'Parameter name overlap'
    (output / 'fighter_parameters.fmp').write_text(parameters)
    (output / 'candidate.fms').write_text(text)
    inputs = {str(p.relative_to(HERE.parent)): hashlib.sha256(p.read_bytes()).hexdigest()
              for directory in ['gamewatch_chef_composition', 'gamewatch_specials_composition',
                                'gamewatch_specials_contracts']
              for p in (HERE.parent / directory).iterdir() if p.is_file()}
    (output / 'composition_inputs.json').write_text(json.dumps(inputs, indent=2) + '\n')
    return output / 'candidate.fms'
