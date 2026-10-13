"""Extract actual two-pose insertion and shared common tuning for comparison."""
from pathlib import Path
import argparse
import hashlib
import json
from item_owner_fixture import body

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    sources = {
        'src/melee/ft/ftparts.c': ['ftParts_80075304', 'ftParts_800753D4'],
        'src/melee/ft/ftanim_tree.c': ['ftAnim_GetNextJointInTree'],
        'src/melee/ft/native_fighter_module.c': ['common_attribute'],
    }
    blocks, receipt = [], {}
    for path, names in sources.items():
        source = args.root / path
        text = source.read_text(encoding='utf-8')
        receipt[path] = hashlib.sha256(source.read_bytes()).hexdigest()
        blocks.extend(body(text, name) for name in names)
    (args.output / 'model_graph_actual.inc').write_text('\n\n'.join(blocks) + '\n', encoding='utf-8')
    (args.output / 'inputs.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
