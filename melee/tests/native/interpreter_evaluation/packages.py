"""Install isolated paired packages with native or interpreted evaluation bodies."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pipeline', type=Path, required=True)
    parser.add_argument('--modules', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--tool', type=Path, default=ROOT/'build/native/native_content_tool.exe')
    parser.add_argument('--iso', type=Path, default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso')
    args = parser.parse_args()
    out, pipeline, modules = args.output.resolve(), args.pipeline.resolve(), args.modules.resolve()
    if out.exists() or out.drive.upper() != 'A:':
        parser.error('Use a fresh A: output, never an installed user content store')
    out.mkdir(parents=True)
    inputs = [args.tool.resolve(), Path(__file__).resolve(), pipeline/'portrait.png']
    for member in ('popo', 'nana'):
        inputs += [p for p in (pipeline/member/'project').iterdir() if p.is_file()]
        inputs += [modules/member/(mode+'.dll') for mode in ('native', 'interpreted')]
    before = {str(p):sha(p) for p in inputs}
    commands, variants = [], {}
    def command(store, stage, *words):
        cmd = list(map(str, [args.tool.resolve(), '--root', store, *words]))
        result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
            timeout=240, creationflags=subprocess.CREATE_NO_WINDOW)
        log = out/(stage+'.log')
        log.write_text(result.stdout+result.stderr, encoding='utf-8', newline='\n')
        commands.append({'command':cmd, 'exit_code':result.returncode, 'log':str(log), 'log_sha256':sha(log)})
        (out/'commands.json').write_text(json.dumps(commands, indent=2)+'\n', encoding='utf-8')
        if result.returncode:
            raise RuntimeError(stage+' failed: '+str(log)+'\n'+(result.stdout+result.stderr)[-3000:])
        return result.stdout
    def identity(text):
        ids = set(re.findall(r'\b[0-9a-f]{64}\b', text))
        if len(ids) != 1: raise RuntimeError('Ambiguous package identity: '+text)
        return ids.pop()
    for mode in ('native', 'interpreted'):
        store = out/mode/'content'
        ids = []
        for member in ('popo', 'nana'):
            project = out/mode/member
            shutil.copytree(pipeline/member/'project', project)
            shutil.copyfile(modules/member/(mode+'.dll'), project/'module.dll')
            package = identity(command(store, mode+'-'+member+'-bundle',
                'bundle-project', project, pipeline/'portrait.png', args.iso.resolve()))
            verified = command(store, mode+'-'+member+'-verify', 'verify-package', package, args.iso.resolve())
            if 'registered and released.' not in verified:
                raise RuntimeError('Package did not complete actual runtime registration: '+verified)
            ids.append(package)
        family = identity(command(store, mode+'-family', 'compose-family', 'companion',
            'Ice Climbers interpreter evaluation', *ids))
        verified = command(store, mode+'-family-verify', 'verify-package', family, args.iso.resolve())
        if 'registered and released.' not in verified:
            raise RuntimeError('Pair did not complete actual runtime registration')
        command(store, mode+'-accept', 'accept', family, args.iso.resolve())
        variants[mode] = {'content_root':str(store), 'family_id':family, 'members':ids}
    if any(sha(Path(p)) != expected for p,expected in before.items()):
        raise RuntimeError('Source changed during isolated installation')
    receipt = {'status':'PASS isolated native/interpreted paired package installation',
        'variants':variants, 'inputs':before, 'commands':commands,
        'gameplay_or_performance_acceptance':False}
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(variants))

if __name__ == '__main__': main()
