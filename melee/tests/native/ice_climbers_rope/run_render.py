"""Frozen compiler and complete original Gumstrings renderer matrix."""
import argparse
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'baseline', 'reference', 'output', 'iso'):
        p.add_argument('--'+name, type=Path, required=True)
    x = p.parse_args()
    root, baseline, reference, out = x.root.resolve(), x.baseline.resolve(), x.reference.resolve(), x.output.resolve()
    commands=[]
    def run(command, name):
        commands.append(list(map(str,command)))
        r=subprocess.run(commands[-1],cwd=root,capture_output=True,text=True,encoding='utf-8',errors='replace')
        (out/name).write_text(r.stdout+r.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if r.returncode:raise RuntimeError(str(out/name)+'\n'+(r.stdout+r.stderr)[-12000:])
        return r.stdout
    # Preparation creates a fresh output directory, retaining failed attempts.
    command=[sys.executable,HERE/'prepare.py','--root',root,'--baseline',baseline,'--output',out,'--iso',x.iso]
    r=subprocess.run(list(map(str,command)),cwd=root,capture_output=True,text=True)
    if r.returncode:raise RuntimeError(r.stderr)
    commands.append(list(map(str,command)))
    (out/'prepare.log').write_text(r.stdout+r.stderr,encoding='utf-8')
    sdk=baseline/'sdk';tool=baseline/'bin/native_content_tool.exe';clang=baseline/'compiler/clang.exe'
    run([tool,'script-c',out/'owned.fms','ice_rope',out/'generated.inc'],'generate.log')
    text=(out/'generated.inc').read_text(encoding='utf-8');prep=json.loads((out/'preparation.json').read_text())
    schema=re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL',text)
    assert schema and int(schema[1])==len(prep['parameters'])
    (out/'parameters.inc').write_text('static const uint32_t parameter_bits[]={'+','.join(f"UINT32_C({q['bits']})" for q in prep['parameters'])+'};\n'+f'static const uint64_t parameter_schema={schema[2]}ULL;\n',encoding='utf-8')
    helpers=re.findall(r'^    func (\w+)\(', (HERE/'rope.fms').read_text(encoding='utf-8'),re.M)
    aliases=[]
    for name in ('Extend','Retract','ConstrainForward','Render','Fold'):
        index=helpers.index(name)
        emitted=re.search(r'^static [^\n;]+ (scripted_article_2_(?:mutating_(?:value_)?)?helper_'+str(index)+r')\([^;\n]+\) \{',text,re.M)
        assert emitted,name
        aliases.append(f'#define rope_{name} {emitted[1]}')
    (out/'aliases.inc').write_text('\n'.join(aliases)+'\n',encoding='utf-8')
    spec=importlib.util.spec_from_file_location('paired_extraction',HERE.parent/'ice_climbers_linked/paired_fixture.py')
    extraction=importlib.util.module_from_spec(spec);spec.loader.exec_module(extraction)
    bodies=[];sources={}
    for relative,names in {
        'src/melee/it/kinds/itclimbersstring.c':['it_802C2CA8','it_802C2DB0','it_802C2EC4','it_802C2CA8_outline','it_802C30E8','it_802C32D4','it_802C33B8','it_802C3520'],
        'src/melee/it/kinds/itlinkhookshot.c':['it_802A3C98','it_802A4420','it_802A6DC8'],
        'src/melee/it/kinds/inlines.h':['Item_RetractChain'],
        'src/melee/lb/lbvector.c':['lbVector_Normalize'],
    }.items():
        path=reference/'sdk'/relative;source=path.read_text(encoding='utf-8');entries={}
        for name in names:
            body=extraction.body(source,name);bodies.append(body)
            entries[name]=dict(line=source[:source.index(body)].count('\n')+1,sha256=hashlib.sha256(body.encode()).hexdigest())
        sources[str(path)]=dict(sha256=sha(path),bodies=entries)
    (out/'solver_actual.inc').write_text('\n'.join(b[:b.index('{')].strip()+';' for b in bodies)+'\n\n'+'\n\n'.join(bodies)+'\n',encoding='utf-8')
    (out/'attributes.inc').write_text('\n'.join(f"{{uint32_t bits=UINT32_C({q['bits']});memcpy((uint8_t*)&attributes+{q['source_offset']},&bits,4);}}" for q in prep['parameters'] if q['article_slot']==2 and q['source_offset'] is not None)+'\n',encoding='utf-8')
    flags=[clang,'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out)]
    batch=out/'compile.cmd'
    batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,flags+[HERE/'render_test.c',sdk/'src/melee/ft/native_callback_lifetime.c','-o',out/'test.exe','-Wl,/OPT:REF'])))+'\n',encoding='utf-8')
    run(['cmd','/c',batch],'compile.log');result=run([out/'test.exe'],'run.log')
    match=re.search(r'PASS (\d+) rope render pairs, (\d+) checks, maximum_float_difference (\S+)',result)
    assert match,result
    own=[HERE/name for name in ('prepare.py','primary.fms','rope.fms','run_render.py','render_test.c')]
    borrowed=[root/'tools/extract_native_animation.py',root/'tools/prepare_native_assets.py']
    for directory in (HERE.parent/'ice_climbers_linked',HERE.parent/'ice_climbers_composition'):
        borrowed.extend(path for path in directory.iterdir() if path.is_file())
    inputs={str(path.relative_to(root)):sha(path) for path in own+borrowed}
    import shutil
    for path in own+borrowed:
        destination=out/'input_sources'/path.relative_to(root)
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(path,destination)
    for path in [tool,clang,baseline/'freeze.json',reference/'freeze.json',sdk/'src/melee/ft/native_callback_lifetime.c']:
        inputs[str(path)]=sha(path)
    (out/'result.json').write_text(json.dumps(dict(pairs=int(match[1]),checks=int(match[2]),maximum_float_difference=float(match[3]),commands=commands,
        sources=sources,inputs=inputs,preparation=prep,
        scope='Complete fold/render policy including original double normalization and matrices; controlled affine/HSD/resource boundaries; primary phases, constructor/callback ordering pending',
        outputs={str(q.relative_to(out)):sha(q) for q in out.rglob('*') if q.is_file()}),indent=2)+'\n',encoding='utf-8')
    print(result.strip())
if __name__=='__main__':main()
