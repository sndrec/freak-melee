"""Extract unchanged source bodies from immutable57 and current native SDK."""
import argparse, hashlib, json, re
from pathlib import Path

GROUPS = {
    'jab': ('ftCo_Attack1.c', ['onPkPc21EC', 'getMotionFlags', 'decideAttack11', 'doAttack12Rapid', 'doAttack12Normal', 'doAttack12', 'doAttack13', 'ftCo_Attack1_CheckInput']),
    'rapid': ('ftCo_Attack100.c', ['ftCo_800D6B00', 'ftCo_800D6C60', 'fn_800D6B8C', 'fn_800D6AC4', 'ftCo_Attack_800D6A50']),
    'landing': ('ftCo_LandingAir.c', ['ftCo_LandingAir_EnterWithMsidLag']),
    'air': ('ftCo_AttackAir.c', ['decideFighter', 'ftCo_AttackAir_GetMsidFromCStick', 'ftCo_AttackAir_EnterFromCStick', 'ftCo_AttackAir_EnterFromMsid', 'ftCo_AttackAir_CheckItemThrowInput']),
    'tilt': ('ftCo_AttackLw3.c', ['callUnk', 'doEnter', 'decideFighter']),
}

def extract(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match: raise ValueError(name)
    end = match.end(); depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}'); end += 1
    return text[match.start():end]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original-sdk', 'candidate-sdk', 'output'): parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args(); args.output.mkdir(parents=True, exist_ok=True); manifest = {}
    for version, sdk in [('old', args.original_sdk), ('new', args.candidate_sdk)]:
        all_bodies = {}; names = {}; globals_map = {}
        for group, (filename, listed) in GROUPS.items():
            path = sdk / 'src/melee/ft/kinds/ftCommon' / filename
            raw = path.read_bytes(); source = raw.decode(); selected = list(listed)
            if version == 'new' and group == 'rapid': selected[selected.index('ftCo_800D6C60')] = 'ftCo_RapidJab_ShouldFinish'
            if group == 'jab': selected.append('checkAttack11' if version == 'old' else 'ftCo_Attack11_EnterMotion')
            names[group] = selected; all_bodies[group] = {name: extract(source, name) for name in selected}
            manifest[f'{version}/{filename}'] = {'path': str(path.resolve()), 'sha256': hashlib.sha256(raw).hexdigest(),
                'bodies': {name: {'sha256': hashlib.sha256(value.encode()).hexdigest(), 'line': source[:source.index(value)].count('\n') + 1} for name, value in all_bodies[group].items()}}
            globals_map.update({name: f'{version}_{group}_{name}' for name in selected if not name.startswith(('decide', 'doEnter', 'callUnk', 'getMotionFlags', 'onPkPc'))})
        prototypes = []; bodies = []
        for group, selected in names.items():
            mapping = dict(globals_map); mapping.update({name: f'{version}_{group}_{name}' for name in selected})
            token = re.compile(r'\b(' + '|'.join(map(re.escape, sorted(mapping, key=len, reverse=True))) + r')\b')
            for value in all_bodies[group].values():
                value = token.sub(lambda match: mapping[match[0]], value)
                prototypes.append(value[:value.index('{')].strip() + ';')
                bodies.append(value)
        # Enum is scoped to the original file in production; expose its exact
        # value only once to avoid duplicate enumeration in this combined TU.
        (args.output / f'{version}_attack.inc').write_text('\n'.join(prototypes) + '\n\n' + '\n\n'.join(bodies) + '\n')
    (args.output / 'source_manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')

if __name__ == '__main__': main()
