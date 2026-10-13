"""Resolve the accepted53 pending gameplay inputs into an UNREGISTERED draft.

This is source composition, not constructor/paper-render acceptance. Accepted
component files are read and hashed; none are rewritten or registered.
"""
from pathlib import Path
import hashlib
import json
import re

HERE = Path(__file__).resolve().parent
ACCEPTED = HERE.parent / 'gamewatch_specials_composition'


def compose(output):
    output = Path(output)
    fighter = (ACCEPTED / 'fighter.fms').read_text()
    article = (ACCEPTED / 'articles.fms').read_text()
    pending = (ACCEPTED / 'pending.fms').read_text()
    inputs = {name: hashlib.sha256((ACCEPTED / name).read_bytes()).hexdigest()
              for name in ['fighter.fms', 'articles.fms', 'pending.fms', 'fighter_parameters.fmp']}
    for name in ['JudgeSetup', 'RescueSetup', 'RescueRemove', 'PanicSetup']:
        pattern = r'(?ms)^func ' + name + r'\(\):\n.*?(?=^func |^action |^article |\Z)'
        replacement = re.search(pattern, pending)[0]
        if name == 'JudgeSetup':
            start = replacement.index('            # Pending ordinary ENGINE spawn')
            end = replacement.index('        if state.judge_handle', start)
            replacement = replacement[:start] + (
                '            let bool spawned_food = items.spawn_food(x, y, z)\n') + replacement[end:]
        fighter, count = re.subn(pattern, lambda _: replacement, fighter)
        assert count == 1
    fighter = fighter.replace('attrs.panic_stick_deadzone', 'controls.horizontal_stick_deadzone')
    fighter = re.sub(r'(?m)^# (Paper construction.*|Absorber hit count.*|panic_stick_deadzone.*)\n', '', fighter)
    fighter = ('# SDK56 draft: setup/prop creation only syntax-checked; actual paper renderer excluded.\n' + fighter)
    absorb = re.search(r'(?ms)^action Absorb:.*?(?=^article )', pending)[0]
    absorb = absorb.replace(
        '        # Original s32 += Float damage truncates the sum, not damage alone.\n'
        '        state.panic_damage = int(float(state.panic_damage) + absorber.damage)',
        '        # Both source fields are s32; preserve their integer sum.\n'
        '        state.panic_damage = state.panic_damage + absorber.damage')
    fighter = fighter.rstrip() + '\n' + absorb
    for kind in ['Judge', 'Panic', 'Rescue']:
        pattern = r'(?ms)^article It_Kind_GameWatch_' + kind + r' states \d+:\n(.*?)(?=^article |\Z)'
        extra = re.search(pattern, pending)[1].replace('targets.exists', 'targets.is_fighter')
        marker = re.search(pattern, article)
        article = article[:marker.start(1)] + extra + article[marker.start(1):]
    article = article.replace(
        '    # PickedUp requires live current-owner ground/air query, kept in pending.fms.\n', '')
    parameters = '\n'.join(line for line in (ACCEPTED / 'fighter_parameters.fmp').read_text().splitlines()
                           if not line.startswith('float panic_stick_deadzone =')) + '\n'
    output.mkdir(parents=True, exist_ok=True)
    (output / 'composed.fms').write_text(fighter.rstrip() + '\n' + article.rstrip() + '\n')
    (output / 'fighter_parameters.fmp').write_text(parameters)
    (output / 'composition_inputs.json').write_text(json.dumps(inputs, indent=2) + '\n')
    return output / 'composed.fms'
