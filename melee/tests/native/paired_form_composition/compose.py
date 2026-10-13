"""Unregistered complete-special candidates; accepted component bytes untouched."""
import importlib.util
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent


def load_compose(path):
    spec = importlib.util.spec_from_file_location(path.parent.name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.compose()


def marked(source):
    declarations, rest = source.split('# DECLARATIONS\n', 1)[1].split('# FUNCTIONS\n', 1)
    functions, callbacks = rest.split('# CALLBACKS\n', 1)
    return declarations, functions, callbacks


def compose(member):
    if member == 'sheik':
        source = load_compose(HERE.parent / 'sheik_clone_activation/compose.py')
        # The real common constructor zeroes private pointer state. The owned
        # integer handle representation uses -1 instead, seeded only on load.
        source = source.replace('        fighter.enable_walljump()\n',
            '        fighter.enable_walljump()\n'
            '        state.needle_count = 0\n'
            '        state.needle_held = -1\n'
            '        state.chain_handle = -1\n', 1)
    elif member == 'zelda':
        source = (HERE.parent / 'zelda_script_composition/candidate.fms').read_text()
        declarations, functions, callbacks = marked(
            (HERE.parent / 'zelda_travel58_composition/travel.fms').read_text())
        first_function = re.search(r'(?m)^func ', source).start()
        source = source[:first_function] + declarations + '\n' + source[first_function:]
        first_move = re.search(r'(?m)^move ', source).start()
        source = source[:first_move] + functions + '\n' + source[first_move:]
        article = re.search(r'(?m)^article ', source).start()
        source = source[:article] + callbacks + '\n' + source[article:]
        # Stock extended pickup/drop slots point to the same ordinary policies.
        source = source.replace('    on item_drop:\n',
            '    on item_pickup_ext:\n'
            '        fighter.item_pickup(event.flag, 1, 1)\n'
            '    on item_drop_ext:\n'
            '        fighter.item_drop(event.flag, 1, 1)\n'
            '    on item_drop:\n', 1)
    else:
        raise ValueError(member)
    transform = marked((HERE / (member + '_transform.fms')).read_text())
    pending = marked((HERE / 'cpu_pending.fms').read_text())
    if member == 'zelda':
        recovery = marked((HERE / 'zelda_recovery.fms').read_text())
        pending = tuple(pending[i] + '\n' + recovery[i] for i in range(3))
    declarations, functions = tuple(transform[i] + '\n' + pending[i] for i in range(2))
    callbacks = transform[2]
    first_function = re.search(r'(?m)^func ', source).start()
    source = source[:first_function] + declarations + '\n' + source[first_function:]
    first_move = re.search(r'(?m)^move ', source).start()
    source = source[:first_move] + functions + '\n' + pending[2] + '\n' + source[first_move:]
    article = re.search(r'(?m)^article ', source).start()
    source = source[:article] + callbacks + '\n' + source[article:]
    return source.rstrip() + '\n'
