"""Assemble the unregistered smash candidate from accepted resource components."""
from pathlib import Path
import hashlib
import json
import re

HERE = Path(__file__).resolve().parent
NATIVE = HERE.parent
INPUTS = {
    'solvers': NATIVE/'ness_yoyo_solvers/candidate.fms',
    'entries': NATIVE/'ness_yoyo_entries/candidate.fms',
    'bat': NATIVE/'ness_resource_closure/bat.fms',
}

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def article(text, name):
    found = re.search(r'^article '+re.escape(name)+r' .*$', text, re.M)
    if not found:
        raise RuntimeError('Missing article: '+name)
    tail = text[found.start():]
    following = re.search(r'^article ', tail[1:], re.M)
    return tail[:following.start()+1] if following else tail

def states(text):
    begin = re.search(r'^    state 0 anim ', text, re.M)
    if not begin:
        raise RuntimeError('Missing state zero.')
    return text[:begin.start()], re.split(r'(?=^    state \d+ anim )', text[begin.start():], flags=re.M)[1:]

def main():
    receipts = {str(path.relative_to(NATIVE)): sha(path) for path in INPUTS.values()}
    solver = article(INPUTS['solvers'].read_text(), 'It_Kind_Ness_Yoyo')
    solver = solver.replace('attrs.yoyo_up_charge_motion', 'attrs.yoyo_up_initial_motion')
    entry = article(INPUTS['entries'].read_text(), 'It_Kind_Ness_Yoyo')
    solver_head, solver_states = states(solver)
    entry_head, entry_states = states(entry)
    helpers = entry_head[entry_head.index('    func Held():'):]
    helpers = helpers.replace('    on signal held:\n            Held()',
        '    on signal held:\n            item.state.input_x = 0\n            item.state.input_y = 0\n            item.state.input_z = 0\n            Held()')
    helpers += '''    on signal charge_release:
            item.state.input_x = item.params.release_velocity_x * signal.value
            item.state.input_y = item.params.release_velocity_y
            item.state.input_z = 0
            Released()
    on signal pause:
            item.pause()
    on signal remove:
            ReleaseOwner()
            item.remove()
    on destroyed:
            item.resources.release()
'''
    imports = '''    attribute float released_spin_rate 24
    attribute float charged_spin_rate 28
    attribute float initial_charge_spin_rate 32
    attribute float release_velocity_x 36
    attribute float release_velocity_y 48
    attribute int up_smash_rotate_frame 64
    attribute int up_smash_retract_frame 68
    attribute int down_smash_rotate_frame 72
    attribute int down_smash_retract_frame 76
'''
    solver_head = solver_head.replace('    resources capacity 90\n', imports+'    resources capacity 90\n')
    solver_head = solver_head.replace('    state int source_motion\n',
        '    state float input_x\n    state float input_y\n    state float input_z\n    state int source_motion\n')
    if len(solver_states) != 4 or len(entry_states) != 4:
        raise RuntimeError('Expected exactly four resource phases.')
    merged = solver_head+helpers+'\n'
    for index in range(4):
        entry_lines = entry_states[index].splitlines(keepends=True)
        solver_lines = solver_states[index].splitlines(keepends=True)
        merged += ''.join(entry_lines).rstrip()+'\n'+''.join(solver_lines[1:]).rstrip()+'\n'
    fighter = (HERE/'fighter.fms').read_text()
    solver_globals = INPUTS['solvers'].read_text().split('action Probe:')[0]
    solver_globals = solver_globals.replace('yoyo_up_charge_motion', 'yoyo_up_initial_motion')
    entry_globals = INPUTS['entries'].read_text().split('action Probe:')[0]
    bat = INPUTS['bat'].read_text()
    parameters = {}
    for text in [fighter, solver_globals, entry_globals]:
        for line in text.splitlines():
            if line.startswith('parameter '):
                name = line.split()[2]
                if name in parameters and parameters[name] != line:
                    raise RuntimeError('Conflicting parameter: '+name)
                parameters[name] = line
    parameters['bat_lifetime'] = 'parameter float bat_lifetime = 1200'
    attributes = '\n'.join(line for line in fighter.splitlines() if line.startswith('attribute '))
    # Global declarations precede all helpers/handlers; article declarations are
    # copied as immutable inputs, not parsed or dispatched dynamically at runtime.
    fighter = '\n'.join(line for line in fighter.splitlines()
                        if not line.startswith(('parameter ', 'attribute ')))+'\n'
    candidate = '# UNREGISTERED smash source composition; no default activation.\nbase ftDataNess 341 35\n'+\
        '\n'.join(parameters.values())+'\n'+attributes+'\n'+fighter+'\n'+merged+'\n'+bat
    candidate = candidate.rstrip()+'\n'
    (HERE/'candidate.fms').write_text(candidate, encoding='utf-8')
    (HERE/'component_inputs.json').write_text(json.dumps(receipts, indent=2)+'\n', encoding='utf-8')
    print('Candidate:', len(candidate.encode()), 'bytes;', len(parameters), 'named defaults')

if __name__ == '__main__':
    main()
