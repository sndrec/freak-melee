"""Original Din CPU steering versus generated paired Zelda callbacks, console only."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

from compose import compose
from stage import stage

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
                      re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match:
        raise ValueError('Missing function '+name)
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('snapshot', 'reference', 'output', 'iso'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    snapshot, reference, output = (p.resolve() for p in (args.snapshot,args.reference,args.output))
    if output.drive.upper() != 'A:' or output.exists():
        parser.error('Use a fresh output directory on A:')
    manifests = [(s,json.loads((s/'freeze.json').read_text())) for s in (snapshot,reference)]
    def verify():
        for base, manifest in manifests:
            for name, expected in manifest.items():
                if sha(base/name) != expected:
                    raise RuntimeError('Changed snapshot input '+str(base/name))
    verify()
    output.mkdir(parents=True)
    inputs = {str(p):sha(p) for p in HERE.iterdir() if p.is_file()}
    for directory in ('paired_form_composition','zelda_script_composition','zelda_travel58_composition'):
        inputs.update({str(p):sha(p) for p in (HERE.parent/directory).iterdir()
                       if p.is_file() and p.suffix in ('.py','.fms')})
    extractor = ROOT/'tools/extract_native_animation.py'
    inputs[str(extractor)] = sha(extractor)
    sys.path.insert(0,str(extractor.parent))
    from extract_native_animation import extract_clip
    dat, top = extract_clip(args.iso,'PlZd.dat','ftDataZelda')
    source = compose()
    (output/'candidate.fms').write_text(source,newline='\n')
    parameters = stage(source,dat,top,output)
    sdk, oldsdk = snapshot/'sdk', reference/'sdk'
    commands = []
    def run(command, name):
        command = list(map(str,command))
        result = subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
        (output/(name+'.log')).write_text(result.stdout+result.stderr,newline='\n')
        commands.append({'stage':name,'command':command,'exit':result.returncode})
        if result.returncode:
            raise RuntimeError(name+':\n'+result.stdout+result.stderr)
        return result.stdout
    run([snapshot/'bin/native_content_tool.exe','script-c',output/'owned.fms',
         'zelda_din_cpu',output/'generated.inc'],'generate')
    generated = (output/'generated.inc').read_text()
    count, schema = re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL',generated).groups()
    assert int(count) == len(parameters)
    (output/'parameters.inc').write_text('static const uint32_t parameter_bits[] = {'+','.join(
        f"UINT32_C({p['bits']})" for p in parameters)+'};\nstatic const uint64_t parameter_schema = '+schema+'ULL;\n'+
        'static const char* parameter_names[] = {'+','.join(json.dumps(p['name']) for p in parameters)+'};\n',newline='\n')
    (output/'hosts.inc').write_text('\n'.join(
        f'host.{name}=(__typeof__(host.{name}))unexpected;' for name in sorted(set(
            re.findall(r'host->(\w+) == 0',generated))))+'\n',newline='\n')
    old = (oldsdk/'src/melee/ft/kinds/ftCommon/ftCo_0A01.c').read_text()
    new = (sdk/'src/melee/ft/kinds/ftCommon/ftCo_0A01.c').read_text()
    pieces = []
    for name in ('ftCo_IsAlly','ftCo_IsAlly_dontinline','inlineD0','ftCo_800A1C44',
                 'ftCo_800A0F00','inlineD1'):
        original = body(old,name)
        assert original == body(new,name),'Changed eligibility helper '+name
        pieces.append(original)
    library = (sdk/'src/melee/ft/ftlib.c').read_text()
    pieces += [body(library,'ftLib_8008732C'),body(library,'ftLib_8008741C'),body(old,'ftCo_800A4E8C')]
    start = new.index('typedef struct CpuQueryIdentity')
    pieces += [new[start:new.index('\n#endif',start)],body(new,'ftCo_CpuNearestEligibleTarget')]
    pieces += [body((sdk/'src/melee/ft/native_fighter_target.c').read_text(),'nativeFighterTarget_Position')]
    (output/'queries.inc').write_text('\n\n'.join(pieces)+'\n',newline='\n')
    commandsource = (oldsdk/'src/melee/ft/ftcmdscript.c').read_text()
    cpusource = (oldsdk/'src/melee/ft/ftcpuattack.c').read_text()
    originals = [body(commandsource,'ftCo_800B463C'),body(commandsource,'ftCo_800B46B8'),
        body(old,'ftCo_800A0C8C'),body(old,'ftCo_CpuSetNeutralStick'),
        body(cpusource,'ftCo_CpuClearTargetAndFinish'),body(cpusource,'ftCo_800B9504')]
    (output/'original.inc').write_text('\n\n'.join(originals)+'\n',newline='\n')
    flags = [snapshot/'compiler/clang.exe','-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld',
        '-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
        '-D_DLL','-D_MT','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(output)]
    def compile_(command,name):
        batch = output/(name+'.cmd')
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\n'+
            subprocess.list2cmdline(list(map(str,command)))+'\n',newline='\n')
        return run(['cmd','/c',batch],name)
    compile_(flags+['-shared','-x','c',output/'generated.inc','-o',output/'module.dll'],'module')
    compile_(flags+[HERE/'compare.c',sdk/'src/melee/it/native_article_refs.c',
        sdk/'src/melee/ft/native_callback_lifetime.c','-o',output/'compare.exe'],'compile')
    result = json.loads(run([output/'compare.exe'],'compare'))
    verify()
    for name, expected in inputs.items():
        if sha(Path(name)) != expected:
            raise RuntimeError('Changed source '+name)
    receipt = {'status':'PASS unregistered paired Zelda Din CPU source scope','result':result,
        'snapshot_sha256':sha(snapshot/'freeze.json'),'reference_sha256':sha(reference/'freeze.json'),
        'inputs':inputs,'parameters':parameters,'commands':commands,'retail_dat_sha256':hashlib.sha256(dat).hexdigest(),
        'outputs':{p.name:sha(p) for p in output.iterdir() if p.is_file()},
        'limits':['Original B9504 and eligibility/command bodies execute unchanged from the reference snapshot.',
          'Actual snapshot same-author article resolver/local-translation and source-lifetime units execute; complete nearest and target-position bodies are extracted without modification.',
          'Unregistered whole Zelda source compiles together; executed new policy scope is attack/travel Din decisions, not all previous move components.',
          'Retail owned parameter staging is independently decoded. Normal paired package registration/import and independent lifecycle closure remain pending.',
          'Stage bounds/game-mode/special-state queries are deterministic controlled boundaries; no whole CPU scheduler, rendering, games or install.',
          'Source invalidation is injected at Host boundaries; normally pure queries do not dispatch authored callbacks.',
          'Named Float tuning uses binary32 arithmetic; bounded original double-compound release-threshold differences are counted separately, with all other bytes/branches exact.']}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
