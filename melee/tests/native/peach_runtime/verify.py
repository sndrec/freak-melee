"""Run one hidden, muted independent-Peach gameplay comparison batch.

Prepare an isolated package with peach_activation58_composition/prepare.py first.
This runner explicitly accepts that package only in the supplied isolated store.
"""
import argparse
import hashlib
import json
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
CASES = [
    ('toad',365), ('pluck',352), ('parasol',361), ('side',354),
    ('air-toad',367), ('air-parasol',363),
    ('left-toad',365), ('left-pluck',352), ('left-parasol',361),
    ('left-side',354), ('air-pluck',25), ('air-side',357),
    ('float',341), ('float-attack',344), ('smash',None), ('cpu',None),
]


def inputs(template, name):
    count = 1200 if name == 'cpu' else 550 if name in dict(CASES[:6]) else 650
    frames = bytearray()
    for tick in range(count):
        sample = bytearray(template[24:104])
        sample[8:72] = bytes(64)
        button,x,y,cx,cy = 0,0,0,0,0
        if name.startswith('left-') and 110 <= tick < 118:
            x = -80
        if name.startswith('air-') and 145 <= tick < 153:
            button = 1 << 10
        if name.startswith('float') and 145 <= tick < 270:
            button = 1 << 10
            if tick >= 175:
                y = -80
            if name == 'float-attack' and tick == 215:
                button |= 1 << 8
                y = 0
        if name == 'smash' and tick in (150,280,410):
            cx = 80
        if name not in ('float','float-attack','smash','cpu') and tick == (
                175 if name.startswith('air-') else 150):
            button = 1 << 9
            if name.endswith('pluck'):
                y = -80
            elif name.endswith('parasol'):
                y = 80
            elif name.endswith('side'):
                x = -80 if name.startswith('left-') else 80
        struct.pack_into('<Hbbbb',sample,8,button,x,y,cx,cy)
        frames.extend(sample)
    return template[:24] + frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot',type=Path,required=True)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--iso',type=Path,default=ROOT.parent /
                        'Super Smash Bros. Melee (USA) (v1.02).iso')
    args = parser.parse_args()
    snap, prepared, out = map(Path.resolve,(args.snapshot,args.prepared,args.output))
    if out.exists() or out == prepared or prepared in out.parents:
        parser.error('use a fresh output outside the prepared package')
    out.mkdir(parents=True)
    receipt = json.loads((prepared/'receipt.json').read_text(encoding='utf-8'))
    store = prepared/'isolated-store'
    subprocess.run([str(snap/'bin/native_content_tool.exe'),'--root',str(store),
                    'accept',receipt['package_id'],str(args.iso.resolve())],
                   cwd=ROOT,check=True)
    template = (ROOT/'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    assert template[:8] == b'FMREPLAY' and (len(template)-24) % 80 == 0
    results = []
    for name, expected in CASES:
        path = out/(name+'.fmr')
        path.write_bytes(inputs(template,name))
        command = [sys.executable,str(ROOT/'tools/run_fighter_parity.py'),
            '--exe',str(snap/'bin/melee_native.exe'),'--iso',str(args.iso.resolve()),
            '--inputs',str(path),'--stock-fighter','peach',
            '--package-id',receipt['package_id'],'--content-root',str(store),
            '--out-dir',str(out/name),'--independent','--p2','human:zelda']
        if expected is not None:
            command += ['--expect-motion',str(expected)]
        if name == 'cpu':
            command += ['--p1-type','cpu','--cpu-level','9']
        with (out/(name+'.log')).open('wb') as log:
            run = subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,timeout=200)
        results.append({'name':name,'exit':run.returncode,'command':command,
                        'input_sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
        (out/'result.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
        print(name,run.returncode,flush=True)
    return any(row['exit'] for row in results)


if __name__ == '__main__':
    sys.exit(main())
