"""Attribute a collision migration difference using independently linked controls.

The original and isolated unfixed clients must agree exactly. The isolated
corrected control and current client must agree exactly. All trace fields remain
compared; this does not normalize or waive a gameplay discrepancy.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--unfixed', type=Path, required=True)
    parser.add_argument('--control', type=Path, required=True)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--samples', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.samples < 1 or args.output.exists():
        parser.error('positive sample count and fresh output required')
    spec = importlib.util.spec_from_file_location('compare', ROOT/'tools/compare_fighter_parity.py')
    compare = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(compare)
    paths = dict(original=args.original, unfixed=args.unfixed,
                 control=args.control, current=args.current)
    traces = {}
    for name, path in paths.items():
        state, end, missing_visual = compare.read_trace(path, [1, 2, 3, 4], [])
        if end != args.samples or not state['fighter'] or missing_visual:
            parser.error(name+' has incomplete/missing gameplay or item visual traces')
        traces[name] = state

    def differences(first, second):
        result = []
        for family, a in traces[first].items():
            b = traces[second][family]
            if a == b:
                continue
            index = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
            count = sum(x != y for x, y in zip(a, b)) + abs(len(a)-len(b))
            result.append(dict(family=family, count=count, index=index,
                               first=a[index] if index < len(a) else None,
                               second=b[index] if index < len(b) else None))
        return result

    checks = dict(original_vs_isolated_unfixed=differences('original', 'unfixed'),
                  isolated_control_vs_current=differences('control', 'current'),
                  original_vs_current=differences('original', 'current'),
                  isolated_unfixed_vs_current=differences('unfixed', 'current'))
    result = dict(status='PASS' if not checks['original_vs_isolated_unfixed']
                  and not checks['isolated_control_vs_current']
                  and checks['original_vs_current'] == checks['isolated_unfixed_vs_current'] else 'FAIL',
                  checks=checks, samples=args.samples,
                  input_hashes={str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in [*paths.values(), Path(__file__).resolve(), ROOT/'tools/compare_fighter_parity.py']},
                  scope='Exact trace attribution only. Build/source provenance and correctness of the corrected query need separate evidence.')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(dict(status=result['status'], checks={k: [dict(family=x['family'], count=x['count'], index=x['index']) for x in v]
                                                         for k, v in checks.items()}, receipt=str(args.output))))
    return int(result['status'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
