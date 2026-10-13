"""Audit composed Samus and run geometry/lifecycle source through the VM.

The constructor arithmetic slice is compared before allocation. Receiver values,
metadata and imported tuning are controlled; this is not full grapple acceptance.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--tool', type=Path, required=True)
    p.add_argument('--base-project', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    out = a.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        p.error('fresh A: output required')
    out.mkdir(parents=True)
    composition = load(HERE/'compose.py', 'composition')
    source = composition.compose()
    candidate = out/'samus.fms'
    candidate.write_text(source, encoding='utf-8', newline='\n')
    inputs = list(composition.INPUTS)+[HERE/'compose.py', Path(__file__), HERE/'interpreter_geometry.c']
    attrs = (ROOT/'tests/native/samus_grapple_article_composition/article.component.fms').read_text()
    ctor = (HERE/'constructor.component.fms').read_text()
    declarations = '\n'.join(re.findall(r'^    (?:attribute|state) (?:float|int|bool) .+$', attrs+'\n'+ctor, re.M))
    helper = re.search(r'^    func InitializeGeometry\(\) -> bool:\n.*?(?=^    func )', ctor, re.M|re.S).group()
    fragment = out/'geometry.fms'
    fragment.write_text('article It_Kind_Samus_GBeam states 9:\n'+declarations+'\n\n'+helper, encoding='utf-8', newline='\n')
    stock = ROOT/'src/melee/it/kinds/itsamusgrapple.c'
    extractor = load(HERE/'fixture.py', 'extractor')
    body = extractor.body(stock.read_text(), 'it_802B75FC')
    start = body.index('    attrs = ip->xC4_article_data->x4_specialAttributes;')
    end = body.index('    prev_link = NULL;', start)
    arithmetic = body[start:end].replace('    zero_vel = it_803B8674;\n', '')
    # No arithmetic statement, branch, conversion or literal is rewritten.
    (out/'native_geometry.inc').write_text('static void native_geometry(Item* ip, int arg2, float scale)\n{\n    float coeff, temp;\n    itSamusGrappleAttributes* attrs;\n'+arithmetic+'}\n', encoding='utf-8', newline='\n')
    inputs += [stock, HERE/'fixture.py', fragment, candidate, out/'native_geometry.inc']
    # Execute the exact composed load/reload/death callbacks and scale helper,
    # including owned tether reset fields, without unrelated move/Article code.
    root_declarations = '\n'.join(re.findall(r'^(?:state|attribute|parameter) .+$', source, re.M))
    scale_helper = re.search(r'^func Samus_ScaleOwnedAttributes\(\):\n.*?(?=^func )', source, re.M|re.S).group()
    sequence_helper = re.search(r'^func Tether_AdvanceCodeSequence\(\):\n.*?(?=^func )', source, re.M|re.S).group()
    hang_helper = re.search(r'^func Tether_HoldHangTimer\(int modified_mode\):\n.*?(?=^func )', source, re.M|re.S).group()
    # This fixture executes load/reload/death, not throw presentation. Select
    # those exact callbacks even when another hook precedes on load. The full
    # candidate audit and connected accessory VM cohort cover the new hook.
    event_start = source.index('action FighterEvents:')
    events = 'action FighterEvents:\n'+source[source.index('    on load:\n', event_start):]
    events = events[:events.index('    on damage:')]
    lifecycle = out/'lifecycle.fms'
    lifecycle.write_text('fighter:\n    behavior independent\n'+root_declarations+'\n\n'+scale_helper+'\n'+sequence_helper+'\n'+hang_helper+'\n'+events, encoding='utf-8', newline='\n')
    lifecycle_stock = ROOT/'src/melee/ft/kinds/ftSamus/ftsamus.c'
    references = [extractor.body(lifecycle_stock.read_text(), name) for name in ['ftSs_Init_OnLoad','ftSs_Init_LoadSpecialAttrs','ftSs_Init_OnDeath']]
    references.append(extractor.body(stock.read_text(), 'samus_grapple_state_sync'))
    native_hang = extractor.body(stock.read_text(), 'fn_802B8D38')
    start = native_hang.index('    if (ip->xDD4_itemVar.samusgrapple.x16 != 0 &&')
    end = native_hang.index('    if (fp2->ground_or_air != GA_Air)', start)
    # Extract the unchanged conditional/timer slice, before unrelated exits.
    references.append('static void native_hold_hang_timer(Item* ip, Fighter* fp2)\n{\n'+native_hang[start:end]+'}\n')
    (out/'native_lifecycle.inc').write_text('\n\n'.join(references)+'\n', encoding='utf-8', newline='\n')
    inputs += [lifecycle_stock, lifecycle, out/'native_lifecycle.inc', HERE/'interpreter_lifecycle.c']
    # Reuse the compiler cohort's build manifest, not its tests or runtime.
    compiler_manifest = ROOT/'tests/native/script_compile/run.py'
    units = [ROOT/'src/melee/ft'/n for n in load(compiler_manifest, 'compiler_manifest').UNITS]
    boundaries = ROOT/'tests/native/script_compile/native_boundaries.c'
    boundary_text = boundaries.read_text()
    controlled_boundaries = out/'controlled_boundaries.c'
    controlled_boundaries.write_text(boundary_text.replace(extractor.body(boundary_text, 'nativeScriptBindings_SourceAlive'), ''), encoding='utf-8')
    inputs.append(boundaries)
    units += [ROOT/'src/melee/ft/native_source_bank.c', HERE/'interpreter_geometry.c', controlled_boundaries]
    units += sorted((ROOT/'tests/native').glob('script_bindings_unavailable_*.c'))
    units += [ROOT/'tests/native'/('script_receiver_unavailable_'+n+'.c') for n in ['identity','pool','registry']]
    units += [ROOT/'src/melee/gm/native_match_pool.c', ROOT/'src/sysdolphin/baselib/dat.c', ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c']
    inputs += units+[compiler_manifest, a.compiler.resolve(), a.tool.resolve()]
    inputs += [path for path in a.base_project.resolve().rglob('*') if path.is_file()]
    inputs += list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    pins = {str(path.resolve()): sha(path) for path in inputs}
    commands = []
    def run(stage, command, expected=0):
        result = subprocess.run(list(map(str, command)), cwd=ROOT, capture_output=True, text=True, timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr, encoding='utf-8')
        commands.append({'stage':stage,'command':list(map(str, command)),'exit':result.returncode})
        (out/'commands.json').write_text(json.dumps(commands, indent=2)+'\n')
        if result.returncode != expected:
            raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-6000:])
        return result
    result = run('audit', [a.tool.resolve(), 'source-audit', a.base_project.resolve(), candidate], 1)
    audit = json.loads(result.stdout.splitlines()[-1])
    if not (audit['diagnostics']==1 and audit['package_diagnostic'] and audit['metadata_complete'] and audit['bodies_complete'] and
            'exactly one independent Fighter declaration' in result.stderr):
        raise RuntimeError('unexpected candidate audit result')
    exe = out/'geometry.exe'
    flags = ['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-fuse-ld=lld','-Wl,/errorlimit:0']
    run('fixture-strict', [a.compiler.resolve(), *flags[:-2], '-Wall', '-Wextra', '-Werror', '-Wno-array-parameter', '-fsyntax-only', HERE/'interpreter_geometry.c'])
    run('build', [a.compiler.resolve(), *flags, *units, '-o', exe])
    summary = json.loads(run('geometry', [exe, fragment]).stdout)
    lifecycle_exe = out/'lifecycle.exe'
    lifecycle_units = [HERE/'interpreter_lifecycle.c' if path==HERE/'interpreter_geometry.c' else path for path in units]
    run('lifecycle-strict', [a.compiler.resolve(), *flags[:-2], '-Wall', '-Wextra', '-Werror', '-Wno-array-parameter', '-fsyntax-only', HERE/'interpreter_lifecycle.c'])
    run('lifecycle-build', [a.compiler.resolve(), *flags, *lifecycle_units, '-o', lifecycle_exe])
    lifecycle_summary = json.loads(run('lifecycle', [lifecycle_exe, lifecycle]).stdout)
    changed = {name for name, value in pins.items() if sha(Path(name)) != value}
    if changed:
        raise RuntimeError('fixture inputs changed: '+repr(changed))
    receipt = {'status':'PASS','audit':audit,'geometry':summary,'lifecycle':lifecycle_summary,'composition':composition.inventory(source),
        'input_sha256':pins,'commands':commands,'native_arithmetic_sha256':hashlib.sha256(arithmetic.encode()).hexdigest(),
        'actual':['production compiler, storage and VM; unchanged InitializeGeometry body', 'constructor pre-allocation arithmetic slice, including native s32 count conversion',
            'composed load, special-attribute reload, primary death, scaling and input-code source; whole native reference bodies',
            'modified-hang helper against unchanged pre-exit native conditional/timer slice'],
        'controlled':['Article-only metadata and imported tuning; Creator scale/motion reads; no live actor/resource allocation',
            'Fighter setup copy and visual-group boundaries; native article registration is observed, script Articles are admitted by the candidate audit'],
        'limits':['no constructor node/model/animation order, catch/control-signal VM, damage-cleanup VM, modified solver, game or renderer acceptance', 'candidate is deliberately not independently registered']}
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps({'audit':audit,'geometry':summary,'lifecycle':lifecycle_summary}))

if __name__ == '__main__':
    main()
