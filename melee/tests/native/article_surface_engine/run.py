"""Compile complete actual surface and legacy async queue bodies; console only."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def body(source, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
                      re.escape(name) + r'\s*\([^;{}]*\)\s*\{', source)
    if not match:
        raise ValueError('No body for ' + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    args = parser.parse_args()
    snapshot, output, root = args.snapshot.resolve(), args.output.resolve(), args.source_root.resolve()
    if output.exists() or snapshot in output.parents or HERE in output.parents:
        parser.error('Fresh output outside immutable source/snapshot namespaces required')
    manifest_path = snapshot / 'freeze.json'
    manifest = json.loads(manifest_path.read_text())
    frozen = {k:v for k,v in manifest.items() if k.replace('\\','/').startswith(('sdk/','compiler/'))}
    def verify():
        for name, expected in frozen.items():
            if digest(snapshot/name) != expected:
                raise RuntimeError('Frozen input changed: ' + name)
    verify()
    output.mkdir(parents=True)
    selections = {
        'src/melee/lb/native_model_joint.c': ['nativeModelJoint_Resolve'],
        'src/melee/lb/lbvector.c': ['lbVector_Angle','lbVector_Mirror'],
        'src/melee/it/itmaplib.c': ['it_80276308','it_802763E0','it_80276408',
            'it_8027649C_ref','it_802765BC','it_80276CB8','it_8027770C'],
        'src/melee/it/native_article_model.c': ['model_root','nativeArticleModel_SpawnAsyncEffect'],
        'src/melee/ef/efasync.c': ['efAsync_QueueProcessDeferred','efAsync_QueueFlush',
            'efAsync_QueueClear','efAsync_Spawn'],
    }
    inputs = {str(HERE/'test.c'):digest(HERE/'test.c'), str(Path(__file__).resolve()):digest(Path(__file__).resolve())}
    pieces = ['// Complete unchanged production bodies. Source SHA256 in receipt.json.']
    for relative, names in selections.items():
        path = root/relative; inputs[str(path)] = digest(path)
        text = path.read_text()
        pieces.extend('// '+relative+': '+name+'\n'+body(text,name) for name in names)
    header=output/'actual_bodies.h';header.write_text('\n\n'.join(pieces)+'\n',newline='\n')
    sdk=snapshot/'sdk';exe=output/'surface_test.exe'
    command=[str(snapshot/'compiler/clang.exe'),'-std=gnu11','-O2','-ffp-contract=off',
        '-fuse-ld=lld','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(output),
        str(HERE/'test.c'),'-o',str(exe)]
    commands=[]
    for stage, cmd in [('compile',command),('run',[str(exe)])]:
        result=subprocess.run(cmd,capture_output=True,text=True,cwd=ROOT,timeout=90)
        (output/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n')
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        if result.returncode:
            raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    verify()
    for path, expected in inputs.items():
        if digest(Path(path)) != expected:
            raise RuntimeError('Actual source changed: '+path)
    receipt={'status':'PASS console actual source scope','result':json.loads(result.stdout),
        'snapshot_manifest_sha256':digest(manifest_path),'input_sha256':inputs,
        'extracted_sha256':digest(header),'source_bodies':selections,'commands':commands,
        'boundaries':['Complete unchanged itmaplib contact/alignment/reflection bodies and actual lbVector_Angle/Mirror execute.',
            'Actual native model path resolver, async item action, efAsync_Spawn/process/flush/clear execute.',
            'Allocator is a bounded fixture pool; efSync construction/rendering is an observed sink. HSD joint access uses actual inline setters with controlled matrix-dirty sink.',
            'Only ordinary item queue modes0/3 exercised; authored model-policy branch is asserted unreachable. No particle construction, joint retirement before flush, renderer pixels or game acceptance.']}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n')
    print(json.dumps(receipt['result']))

if __name__ == '__main__': main()
