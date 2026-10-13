"""Compare actual shape interpolation against a frozen prior source; no GPU."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE.parent / 'hsd_reservation'))
from extract import function


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    out = a.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        p.error('fresh A: archive required')
    out.mkdir(parents=True)
    current = ROOT / 'src/sysdolphin/baselib/pobj.c'
    inputs = [current, a.baseline, HERE / 'test.c', HERE / 'run.py', HERE / 'README.md',
              HERE.parent / 'hsd_reservation/extract.py', a.compiler]
    inputs += list((ROOT / 'src/sysdolphin/baselib').glob('*.h'))
    inputs += list((ROOT / 'src/Runtime').rglob('*.h'))
    inputs += list((ROOT / 'extern/dolphin/include').rglob('*.h'))
    sha = lambda f: hashlib.sha256(f.read_bytes()).hexdigest()
    pins = {str(f.resolve()): sha(f) for f in inputs}
    for base, target in [(ROOT / 'src', out / 'src'),
                         (ROOT / 'extern/dolphin/include', out / 'dolphin')]:
        shutil.copytree(base, target, ignore=shutil.ignore_patterns('*.c', '*.cpp'))
    shutil.copy2(HERE / 'test.c', out / 'test.c')
    old = a.baseline.read_text()
    new = current.read_text()
    names = ['shape_read_u16', 'shape_read_f32', 'decode_shape_components',
             'get_shape_vertex_xyz', 'get_shape_normal_xyz', 'get_shape_nbt_xyz']
    helpers = '\n'.join(function(new, name) for name in names)
    for name in names:
        if function(old, name) != function(new, name):
            raise RuntimeError('baseline decoder changed: ' + name)
    (out / 'helpers.h').write_text(helpers)
    emissions = ''
    for text, name in [(old, 'prior_emission'), (new, 'fixed_emission')]:
        emissions += function(text, 'interpretShapeAnimDisplayList').replace(
            'interpretShapeAnimDisplayList(', name + '(') + '\n'
    (out / 'emission.h').write_text(emissions)
    old_draw = function(old, 'drawShapeAnim')
    old_draw = old_draw.replace('drawShapeAnim(', 'prior_draw(')
    old_draw = old_draw.replace('interpretShapeAnimDisplayList(', 'capture(')
    # Only names/storage/sink differ. Both kernels use the source's decoding
    # and interpolation bodies; capture copies precisely the consumed prefix.
    (out / 'prior.h').write_text('static f32 (*vertex_buffer)[3], (*normal_buffer)[3];\n'
                                'static u32 vertex_buffer_size, normal_buffer_size;\n' + old_draw)
    new_draw = function(new, 'drawShapeAnim')
    new_draw = new_draw.replace('drawShapeAnim(', 'fixed_draw(')
    new_draw = new_draw.replace('interpretShapeAnimDisplayList(', 'capture(')
    new_draw = new_draw.replace('vertex_buffer', 'fixed_vertices').replace('normal_buffer', 'fixed_normals')
    (out / 'fixed.h').write_text('static f32 fixed_vertices[HSD_DEFAULT_MAX_SHAPE_VERTICES][3];\n'
                                'static f32 fixed_normals[HSD_DEFAULT_MAX_SHAPE_NORMALS][3];\n' + new_draw)
    command = [str(a.compiler), '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
               '-Wno-array-parameter', '-Wno-sign-compare', '-DMELEE_NATIVE', '-D_USE_MATH_DEFINES',
               '-I' + str(out / 'src'), '-I' + str(out / 'dolphin'),
               '-fuse-ld=lld', str(out / 'test.c'), '-o', str(out / 'test.exe')]
    commands = []
    summary = None
    for stage, cmd in [('compile', command), ('fixture', [str(out / 'test.exe')])]:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        (out / (stage + '.log')).write_text(result.stdout + result.stderr)
        commands.append({'stage': stage, 'command': cmd, 'exit': result.returncode})
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        if result.returncode:
            raise RuntimeError(stage + ': ' + (result.stdout + result.stderr)[-5000:])
        if stage == 'fixture':
            summary = json.loads(result.stdout)
    if any(sha(Path(f)) != h for f, h in pins.items()):
        raise RuntimeError('input changed')
    receipt = {'status': 'PASS', 'summary': summary, 'source_sha256': pins,
               'commands': commands,
               'actual': 'Frozen native shape decoding/interpolation and display-list bodies; prior lazy buffers versus fixed buffers.',
               'controlled': 'GX leaves capture actual emitted values; sink also copies interpolated prefixes; prior heap allocation is counted malloc.',
               'limits': 'No GX emission/pixels, allocator topology, animation decoder or complete-match timing proof.'}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt['summary']))


if __name__ == '__main__':
    main()
