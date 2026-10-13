"""Compose immutable accepted components; no edits to those inputs or defaults."""
from pathlib import Path
import argparse
import hashlib
import json
import re

HERE = Path(__file__).resolve().parent
NATIVE = HERE.parent
INPUTS = {
    'specials': NATIVE / 'ness_thunder_candidate.fms',
    'smashes': NATIVE / 'ness_smash_composition/candidate.fms',
    'lifecycle': HERE / 'lifecycle.fms',
    'cpu': HERE / 'cpu.fms',
    'common': HERE / 'common46.fms',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def remove_function(text, name):
    pattern = r'(?m)^(?:export )?func ' + re.escape(name) + r'\([^\n]*\):\n'
    found = re.search(pattern, text)
    if not found:
        raise RuntimeError('Missing input helper: ' + name)
    following = re.search(r'(?m)^\S', text[found.end():])
    end = found.end() + following.start() if following else len(text)
    return text[:found.start()] + text[end:]


def compose(output):
    specials = INPUTS['specials'].read_text()
    special_fighter, special_articles = specials.split('\narticle ', 1)
    special_fighter = special_fighter[:special_fighter.index('# Candidate cleanup assumes')]
    special_fighter = remove_function(special_fighter, 'thunder_cross_probe')
    special_fighter = re.sub(r'(?m)^state float thunder_probe_[xyz]\n', '', special_fighter)
    smashes = INPUTS['smashes'].read_text()
    smash_fighter, smash_articles = smashes.split('\narticle ', 1)
    smash_fighter = remove_function(smash_fighter, 'SmashResourceDamage')
    smash_fighter = smash_fighter.replace('SmashResourceDamage', 'ResourceDamage')
    smash_fighter = re.sub(r'(?m)^base .*\n', '', smash_fighter)
    # Full lifecycle owns these slots once; the smash action retains its phases.
    smash_fighter = re.sub(r'(?m)^    on (load|pre_hitlag|post_hitlag):\n(?:        .*\n)+',
                          '', smash_fighter)
    smash_fighter = smash_fighter.replace('move Smashes:', 'action Smashes:')
    # These constructors preserve their explicitly supplied spawn positions.
    # The generic owner-origin option would replace them with the ECB midpoint.
    smash_fighter = smash_fighter.replace(
        'facing.direction, true, 0, 0, false)',
        'facing.direction, false, 0, 0, false)')
    smash_fighter = smash_fighter.replace(
        'facing.direction, true, 0, motion.id)',
        'facing.direction, false, 0, motion.id)')
    # Stock installs these at YoYo creation, even when allocation fails. They
    # are not default load-time hooks; Magnet supplies its own effect callbacks.
    smash_fighter = smash_fighter.replace('    callbacks.track_hitlag()\n',
        '    callbacks.pre_hitlag(YoyoPause)\n'
        '    callbacks.post_hitlag(YoyoResume)\n')
    declarations, bodies = {}, []
    for component in [special_fighter, smash_fighter,
                      INPUTS['lifecycle'].read_text(), INPUTS['cpu'].read_text(),
                      INPUTS['common'].read_text()]:
        lines = []
        for line in component.splitlines():
            if line.startswith(('parameter ', 'attribute ', 'state ')):
                key = (line.split()[0], line.split()[2])
                if key in declarations and declarations[key] != line:
                    raise RuntimeError('Conflicting declaration: ' + str(key))
                declarations[key] = line
            else:
                lines.append(line)
        bodies.append('\n'.join(lines))
    # All helper declarations must precede the first move/action. Their relative
    # order is otherwise irrelevant; compiler dependency validation owns calls.
    helpers, handlers = [], []
    for body in bodies:
        chunks = re.split(r'(?m)(?=^(?:export )?func |^move |^action )', body)
        for chunk in chunks:
            if chunk.startswith(('func ', 'export func ')):
                helpers.append(chunk.strip())
            elif chunk.startswith(('move ', 'action ')):
                handlers.append(chunk.strip())
    yoyo_article, bat_article = smash_articles.split('\narticle It_Kind_Ness_Bat', 1)
    bat_article = bat_article.replace('    on signal remove:\n',
        '    on signal presentation_scale:\n'
        '            item.scale_by_attribute(signal.value)\n'
        '    on signal remove:\n')
    smash_articles = yoyo_article + '\narticle It_Kind_Ness_Bat' + bat_article
    candidate = ('# UNREGISTERED full resource/move/common composition.\n'
                 '# Do not activate or declare independent before closure acceptance.\n' +
                 '\n'.join(declarations.values()) + '\n\n' +
                 '\n\n'.join(helpers + handlers) + '\n\narticle ' +
                 special_articles.rstrip() + '\n\narticle ' + smash_articles.rstrip() + '\n')
    output.mkdir(parents=True, exist_ok=True)
    (output / 'candidate.fms').write_text(candidate, encoding='utf-8')
    receipt = {str(p.relative_to(NATIVE)): sha(p) for p in INPUTS.values()}
    (output / 'component_inputs.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return candidate


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    text = compose(args.output.resolve())
    print('Composed', len(text.splitlines()), 'lines,', len(text.encode()), 'bytes')
