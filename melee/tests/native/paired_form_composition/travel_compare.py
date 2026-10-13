"""Normal imported Ness/Yoshi CPU counter migration; console only."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('snapshot', 'output', 'iso'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if output.drive.upper() != 'A:':
        parser.error('Output must be on A:')
    output.mkdir(parents=True, exist_ok=False)
    manifest = json.loads((snapshot/'freeze.json').read_text())
    def verify():
        for name, expected in manifest.items():
            if sha(snapshot/name) != expected:
                raise RuntimeError('Changed snapshot '+name)
    verify()
    inputs = {str(Path(__file__)): sha(Path(__file__))}
    sdk, tool = snapshot/'sdk', snapshot/'bin/native_content_tool.exe'
    results, commands, references = {}, [], {}
    for member in ('Ness', 'Yoshi'):
        out = output/member.lower();out.mkdir()
        project = out/'project'
        template = ROOT/('examples/'+member.lower()+'_wip.fms')
        inputs[str(template)] = sha(template)
        def run(command, stage):
            command = list(map(str, command))
            process = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=180)
            (out/(stage+'.log')).write_text(process.stdout+process.stderr, newline='\n')
            commands.append({'member': member, 'stage': stage, 'command': command, 'exit': process.returncode})
            if process.returncode:
                raise RuntimeError(stage+': '+process.stdout+process.stderr)
            return process.stdout
        run([tool, 'clone-stock', args.iso, member, project, 'CPU counter source comparison'], 'clone')
        run([tool, 'import-parameters', project, template, project/'behavior.fms'], 'import')
        generated = project/'behavior.generated.c'
        run([tool, 'script-c', project/'behavior.fms', 'cpu_'+member.lower(), generated], 'generate')
        flags = [snapshot/'compiler/clang.exe', '-std=gnu11', '-O2', '-ffp-contract=off',
                 '-fms-runtime-lib=dll', '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES',
                 '-D_CRT_SECURE_NO_WARNINGS', '-D_DLL', '-D_MT', '-fuse-ld=lld',
                 '-I'+str(sdk/'src'), '-I'+str(sdk/'extern/dolphin/include'),
                 '-I'+str(project), '-I'+str(out)]
        def compile_(command, stage):
            batch = out/(stage+'.cmd')
            batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\n'+
                             subprocess.list2cmdline(list(map(str,command)))+'\n', newline='\n')
            return run(['cmd', '/c', batch],stage)
        compile_(flags+['-shared', generated, '-o', project/'module.dll'], 'module')
        if member == 'Ness':
            source = HERE.parent/'ness_fighter_composition'
            import sys
            sys.path.insert(0,str(source))
            helper = load(source/'run.py')
            helper.prepare_comparisons(snapshot,out,generated.read_text())
            def original(path, names):
                text = subprocess.check_output(['git','show','8d17381d3:'+path],cwd=ROOT,text=True)
                references[path] = {'commit':'8d17381d3', 'sha256':hashlib.sha256(text.encode()).hexdigest()}
                return ''.join(helper.extract(text,name) for name in names)
            (out/'random_original.h').write_text('u32 seed=1; u32* seed_ptr=&seed;\n'+
                original('src/sysdolphin/baselib/random.c',['HSD_Randf']),newline='\n')
            (out/'candidate_original.h').write_text(
                original('src/melee/ft/kinds/ftCommon/ftCo_0A01.c',['ftCo_800A2C08'])+
                original('src/melee/ft/ftcpuattack.c',['ftCo_800B9790']),newline='\n')
            units = [source/'cpu_compare.c', sdk/'src/melee/ft/native_fighter_parameters.c']
            arguments = [project/'fighter_parameters.fmp']
            expression = r'Ness composed CPU: (\d+) checks, 0 failures, (\d+) source pairs'
            expected = 2007
            inputs.update({str(p):sha(p) for p in source.glob('*.py')})
        else:
            helper = load(HERE.parent/'yoshi_cpu_stock_compare.py')
            helper.excerpts(out,sdk)
            units = [HERE.parent/'yoshi_cpu_stock_policy_test.c', sdk/'src/melee/ft/native_fighter_parameters.c',
                     sdk/'src/melee/ft/native_callback_lifetime.c']
            arguments = [project/'module.dll', project/'fighter_parameters.fmp']
            expression = r'(.+)'
            expected = None
            inputs[str(HERE.parent/'yoshi_cpu_stock_compare.py')] = sha(HERE.parent/'yoshi_cpu_stock_compare.py')
        inputs.update({str(p):sha(p) for p in units})
        executable = out/'cpu.exe'
        compile_(flags+units+['-o',executable], 'fixture')
        text = run([executable]+arguments,'compare')
        match = re.search(expression,text)
        if not match or (expected is not None and int(match[2]) != expected):
            raise RuntimeError('Coverage mismatch '+text)
        results[member] = {'result':text.strip(), 'template_sha256':sha(template),
                           'generated_sha256':sha(generated), 'module_sha256':sha(project/'module.dll'),
                           'parameters_sha256':sha(project/'fighter_parameters.fmp')}
    verify()
    for name, expected in inputs.items():
        if sha(Path(name)) != expected:
            raise RuntimeError('Changed source '+name)
    receipt = {'status':'PASS imported shipped CPU counter policies, source-only',
               'snapshot_sha256':sha(snapshot/'freeze.json'), 'inputs':inputs,
               'results':results, 'commands':commands, 'original_references':references,
               'scope':'Original decision/order/RNG/queue comparisons retained; explicit authored handled-travel increment replaces former common implicit increment. No gameplay or full-module behavioral reacceptance.'}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n')
    print(json.dumps(results))


if __name__ == '__main__':
    main()
