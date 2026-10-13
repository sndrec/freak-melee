"""Extract complete original/current common policies; no audio or game execution."""
import argparse
import hashlib
import json
import re
from pathlib import Path

def body(text, name):
    match = re.search(r'(?m)^[\w][\w\s*]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', text)
    if not match:
        raise ValueError(name)
    i, depth = match.end(), 1
    while depth:
        depth += (text[i] == '{') - (text[i] == '}')
        i += 1
    return text[match.start():i]

def main():
    parser = argparse.ArgumentParser()
    for key in ('root', 'output'):
        parser.add_argument('--' + key, type=Path, required=True)
    parser.add_argument('--reference', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    hashes = {}
    definitions = [('ft_0877.c', 'ft_80087D0C'), ('ftlib.c', 'ftLib_8008746C'), ('ftmovement.c', 'ft_GetGroundFrictionMultiplier')]
    references = [('original_', args.reference), ('', args.root)] if args.reference else [('', args.root)]
    if args.reference:
        common = args.reference / 'src/melee/ft/ftcommon.c'
        predicate = body(common.read_text(encoding='utf-8'), 'ftCommon_80080144')
        hashes[str(common)] = hashlib.sha256(common.read_bytes()).hexdigest()
    else:
        here = Path(__file__).resolve().parent
        original = (here / 'originals.h').read_bytes()
        manifest = json.loads((here / 'reference_manifest.json').read_text(encoding='utf-8'))
        assert hashlib.sha256(original).hexdigest() == manifest['originals.h']
        (args.output / 'original_common_init.inc').write_bytes(original)
        hashes[str(here / 'originals.h')] = manifest['originals.h']
    for prefix, root in references:
        chunks = []
        for filename, name in definitions:
            path = root / 'src/melee/ft' / filename
            chunks.append(body(path.read_text(encoding='utf-8'), name))
            hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
        text = '\n\n'.join(chunks)
        if prefix:
            text = re.sub(r'\b(' + '|'.join(name for _, name in definitions) + r')\b', lambda m: prefix + m[0], text)
            text = predicate + '\n\n' + text
        (args.output / ('original_common_init.inc' if prefix else 'candidate_common_init.inc')).write_text(text + '\n', encoding='utf-8')
    (args.output / 'common_init_source_manifest.json').write_text(json.dumps(hashes, indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
