"""Hidden native class/ObjAlloc reservation and descriptor census fixture."""
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.dont_write_bytecode = True
sys.path.insert(0,str(HERE))
from extract import extract

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        parser.error('A fresh A: archive is required')
    out.mkdir(parents=True)
    units = [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in
             ['native_hsd_reservation', 'class', 'objalloc', 'memory', 'id', 'hash','texpdag']]
    units += [ROOT/'src/melee/gm/native_match_pool.c',
              ROOT/'src/melee/it/native_article_model_demand.c', HERE/'test.c']
    units += sorted((ROOT/'src/sysdolphin/baselib').glob('dat*.c'))
    arrows = [Path('A:/FreakMelee-test-archives/fighter-callback-source-census')/
              ('fighter-'+str(i))/'fighter.dat' for i in (2,3)]
    extracted_sources = extract(ROOT,out)
    units += [out/(n+'.c') for n in ['satellites','texp','material','stage']]
    native_os = out/'OSAlloc.obj'
    inputs = units + extracted_sources + arrows + [out/'OSAlloc.cpp',args.compiler,
                                          HERE/'run.py',HERE/'extract.py',HERE/'README.md']
    # This fixture does not consume scripting/binding/actor headers. Fingerprint
    # complete allocator and platform header trees plus the two direct native
    # interfaces rather than unrelated concurrently edited gameplay headers.
    inputs += list((ROOT/'src/sysdolphin/baselib').rglob('*.h'))
    inputs += list((ROOT/'src/Runtime').rglob('*.h'))
    inputs += [ROOT/'src/melee/gm/native_match_pool.h',
               ROOT/'src/melee/it/native_article_model_demand.h']
    inputs += list((ROOT/'extern/dolphin/include').rglob('*.h'))
    inputs += list((ROOT.parent/'aurora/include').rglob('*.h'))
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    expected_arrow_hashes = ['84c806ea3760c7290ba9d9eaec3588ce2d2ebdef19b8505418a1cd0c91c60dde',
                             'e614e91710e8a8a648c2da749d2c4bb964caa35d4dcf61ccf7d4810738f02a60']
    for path, digest in zip(arrows, expected_arrow_hashes):
        if sha(path) != digest:
            raise RuntimeError('Actual arrow census input changed: '+str(path))
    hashes = {str(p): sha(p) for p in inputs}
    commands = []
    def run(stage, command):
        r = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                           timeout=120, creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr)
        commands.append({'stage':stage,'command':command,'exit':r.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if r.returncode: raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-16000:])
        return r.stdout, r.stderr
    flags = ['-std=c11','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS',
             '-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),
             '-I'+str(ROOT/'src/sysdolphin/baselib'),
             '-I'+str(ROOT/'extern/dolphin/include')]
    strict = ['-Wall','-Wextra','-Werror','-Wno-array-parameter']
    run('strict', [str(args.compiler),*flags,*strict,'-fsyntax-only',
                  str(units[0]),str(ROOT/'src/melee/it/native_article_model_demand.c'),
                  str(HERE/'test.c'),str(out/'satellites.c')])
    # Compare unchanged HEAD warnings, rather than assuming new branches are
    # harmless. Baseline files remain in the A: archive, never in production.
    baseline_warnings = {}
    for source in units[1:3]:
        rel = source.relative_to(ROOT).as_posix()
        text, _ = run('git-'+source.stem, ['git','show','HEAD:'+rel])
        old = out/(source.stem+'-HEAD.c'); old.write_text(text)
        extra = ['-I'+str(source.parent)]
        _, a = run(source.stem+'-HEAD-warnings', [str(args.compiler),*flags,*extra,
                         '-Wall','-Wextra','-Wno-array-parameter','-fsyntax-only',str(old)])
        _, b = run(source.stem+'-current-warnings', [str(args.compiler),*flags,*extra,
                         '-Wall','-Wextra','-Wno-array-parameter','-fsyntax-only',str(source)])
        normalize = lambda s: sorted(re.findall(r'warning: (.*) \[(-W[^\]]+)\]',s))
        if normalize(a) != normalize(b): raise RuntimeError('New baseline warning: '+str(source))
        baseline_warnings[rel] = normalize(a)
    run('native-os',[str(args.compiler),'-std=c++17','-O2','-ffunction-sections',
         '-fdata-sections','-DTARGET_PC','-DAURORA','-I'+str(ROOT.parent/'aurora/include'),
         '-c',str(out/'OSAlloc.cpp'),'-o',str(native_os)])
    compile = [str(args.compiler),*flags,'-O2','-ffp-contract=off',
               '-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref',
               *map(str,units),str(native_os),'-o',str(out/'test.exe')]
    run('compile',compile)
    stdout,_ = run('fixture',[str(out/'test.exe'),*map(str,arrows)])
    for path, digest in hashes.items():
        if sha(Path(path)) != digest: raise RuntimeError('Input changed: '+path)
    receipt = {'status':'PASS','summary':json.loads(stdout),'source_sha256':hashes,
               'commands':commands,'unchanged_HEAD_warnings':baseline_warnings,
               'actual':['class/ObjAlloc/memory hard-fence direct production entry points',
                         'native MatchPool metadata, reuse, generation and reservation release',
                         'actual Aurora OSAlloc bodies (diagnostic OSDumpHeap omitted) and real HSD ID/Vec/Mtx allocator bodies',
                         'bounded imported-descriptor core/TEV census and uncovered-feature rejection',
                         'bounded native draw-input admission;43 positive/negative texture, palette, vertex-list, display and material cases with failure-output preservation',
                         'actual MObj/TObj expression builders, material compiler, entire DAG optimizer/scheduler and native shader cleanup',
                         'ordinary versus reserved native stage/constant outputs across341 materials, including recompilation and shader-only allocation claims',
                         'actual DAT graph imports for four Link/YoungLink arrow roots at previously proven attribute offsets36/40'],
               'controlled':['trusted synthetic descriptor graphs and class initializer',
                             'synthetic shader-only textures have explicit owned I4 image bytes and nonzero repeats; no imported texture replacement',
                             'native panic leaf exits on any assertion; report leaf writes stderr',
                             'core material/texture structs and default method pointers set explicitly; no whole graph loader',
                             'global toon/shadow environment explicitly empty for the comparisons'],
               'limits':['no HSD runtime graph construction or model binding admitted',
                         'census consumes trusted imported pointers; actual DAT importer validates image/palette/display/indexed-vertex byte extents; no draw execution or GPU command validation here',
                         'local exact backing/metadata restore only; no game rollback claim',
                         'toon/shadow environment, unsupported custom TEV arithmetic/operands, shape/envelope/reference/instance/animation demand remains unclosed',
                         'allocation budget is conservative; native shader legality and default-class selection remain admission gates']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt['summary']))

if __name__ == '__main__': main()
