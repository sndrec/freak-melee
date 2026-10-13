"""Extract complete shared input/geometry bodies, retaining source receipts."""
from pathlib import Path
import argparse, hashlib, json
from item_owner_fixture import body

parser = argparse.ArgumentParser()
parser.add_argument('--root', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
sources = {
    'src/melee/ft/kinds/ftCommon/ftCo_AirCatch.c': ['ftCo_800C3B10', 'ftCo_800C3A14'],
    'src/melee/ft/native_fighter_geometry.c': ['nativeFighterGeometry_ProbeLedge'],
}
blocks, receipts = [], {}
for path, names in sources.items():
    text = (args.root / path).read_text()
    receipts[path] = {'sha256': hashlib.sha256(text.encode()).hexdigest(), 'bodies': {}}
    for name in names:
        extracted = body(text, name)
        blocks.append(extracted)
        receipts[path]['bodies'][name] = hashlib.sha256(extracted.encode()).hexdigest()
(args.output / 'air_tether_actual.inc').write_text('\n\n'.join(blocks) + '\n')
(args.output / 'source.json').write_text(json.dumps(receipts, indent=2) + '\n')
