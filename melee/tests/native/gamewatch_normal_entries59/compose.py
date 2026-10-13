"""Compose accepted sources unchanged, replacing only pending normal policies."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ACCEPTED = HERE.parent / 'gamewatch_normals_composition'


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compose(output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    module = load(ACCEPTED / 'compose.py', 'accepted_normals')
    candidate = module.compose(output)
    base = candidate.read_text()
    draft = (HERE / 'fighter.fms').read_text()
    declarations, helpers, actions = module.split(draft)
    globals_, functions, callbacks = module.split(base)
    # The accepted source contains explicitly provisional Manhole statements.
    # SDK59 resolves the actual primary-root first child, not Item.pos.
    callbacks = [p.replace('item.joint_world_position("0/0", x, y, z)',
                           'item.joint_world_position(0, "0", x, y, z)')
        .replace('item.wind_at_coordinates(', 'effects.wind_at_position(')
        .replace('# Pending generic output query: actual first child WORLD position.',
                 '# SDK59 reads the actual primary first child WORLD Vec3 once.')
        for p in callbacks]
    text = '\n'.join([globals_, declarations] + functions + helpers +
        [p for p in callbacks if p.startswith('move ')] +
        [p for p in callbacks + actions if p.startswith('action ')] +
        [p for p in callbacks if p.startswith('article ')])
    candidate.write_text(text.rstrip() + '\n')
    inputs = json.loads((output / 'composition_inputs.json').read_text())
    inputs.update({str(p.relative_to(HERE.parent)): hashlib.sha256(p.read_bytes()).hexdigest()
                   for p in ACCEPTED.iterdir() if p.is_file()})
    (output / 'composition_inputs.json').write_text(json.dumps(inputs, indent=2) + '\n')
    return candidate
