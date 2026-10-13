"""Source-only healthy Samus Grapple Beam callbacks; normal frozen import and DLL."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
SAMUS = ['samus_grapple_fighter_compare', 'samus_grapple_calc_grav',
         'samus_grapple_setup_pos', 'samus_grapple_anim', 'samus_grapple_state_sync',
         'it_802B7B84', 'fn_802B7E34_inline', 'fn_802B7E34_anim_inline']
SAMUS += ['fn_' + n for n in ['802B7E34','802B805C','802B8384','802B8524','802B8684',
                              '802B8814','802B895C','802B8B54','802B8D38']]
SAMUS += [f'itSamusgrapple_UnkMotion{i}_Phys' for i in range(9)]
SAMUS += ['it_802B900C','it_802B91C4','it_802B9328_attach','it_802B9328_grav',
          'it_802B9328','it_802B99A0','it_802B9CE8','it_802B9FD4','it_802BA194',
          'it_802BA2D8','it_802BA3BC','it_802BA5DC','itSamusGrapple_Logic53_PickedUp',
          'it_802BA9B8','it_802BAA08','it_802BAA58','it_802BAB40','it_802BAB7C',
          'it_802BABB8','it_802BAC3C','it_802BAC80','it_802BACC4','it_802BA760','itSamusGrapple_Logic53_EvtUnk']
LINK = ['it_802A2474','itLinkHookshot_ApplyOwnerScale','it_802A3C98','it_802A3D90',
        'it_802A43B8','it_802A43EC','it_802A4420','it_802A4454','it_802A6F80','it_802A7168']


def body(text, name):
    match = re.search(r'^[A-Za-z_][A-Za-z0-9_ \t*]*[ \t*]' + re.escape(name)
                      + r'\s*\([^;{}]*\)\s*\{', text, re.M)
    if not match:
        raise ValueError(f'actual definition missing: {name}')
    end, depth = text.index('{', match.start()) + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--iso', type=Path, default=ROOT.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso')
    args = p.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if output == snapshot or snapshot in output.parents or output == HERE or HERE in output.parents:
        p.error('output cannot modify frozen or fixture inputs')
    if output.exists():
        p.error('fresh output required')
    output.mkdir(parents=True)
    sdk, tool, clang = snapshot/'sdk', snapshot/'bin/native_content_tool.exe', snapshot/'compiler/clang.exe'
    manifest = snapshot/'freeze.json'
    frozen = {n:h for n,h in json.loads(manifest.read_text()).items()
              if n.replace('\\','/').startswith(('sdk/','compiler/','bin/native_content_tool.exe'))}
    def verify():
        if not frozen:
            raise RuntimeError('empty snapshot inventory')
        for n,h in frozen.items():
            if digest(snapshot/n) != h:
                raise RuntimeError('snapshot changed: '+n)
    verify()
    commands=[]
    def run(stage, command):
        command=list(map(str,command))
        r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
        (output/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n')
        commands.append(dict(stage=stage,command=command,exit=r.returncode))
        (output/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if r.returncode:
            raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-8000:])
        return r.stdout

    inputs=[Path(__file__).resolve(),HERE/'stock_test.c',HERE/'article.component.fms',HERE/'README.md',HERE/'API.md',manifest]
    pieces=[]
    for relative in ['it/kinds/itsamusgrapple.c','it/kinds/itlinkhookshot.c']:
        path=sdk/'src/melee'/relative; text=path.read_text(); inputs.append(path)
        symbols=SAMUS if 'samus' in relative else LINK
        for symbol in symbols:
            value=body(text,symbol)
            if symbol=='it_802BA3BC':
                # Native decompiled C reads uninitialized dir_ptr for x before
                # assigning &dir. Those timelines are counted separately.
                old='link->pos.x = (dir_ptr->x * attrs->x38) + head->pos.x;'
                if value.count(old)!=1:
                    raise RuntimeError('undefined boundary changed')
                value=value.replace(old,'++ub_constraints;\n            link->pos.x = (dir.x * attrs->x38) + head->pos.x;')
            pieces.append((text.index(body(text,symbol)),value,relative))
    vector_path=sdk/'src/melee/lb/lbvector.c';inputs.append(vector_path)
    pieces.append((0,body(vector_path.read_text(),'lbVector_Mirror'),'zz_vector'))
    original='\n\n'.join(v for _,v,_ in sorted(pieces,key=lambda x:(x[2],x[0])))
    # Headers provide external declarations; extracted inline definitions retain
    # actual source order. Shared Item_RetractChain is the actual frozen inline.
    (output/'samus_originals.h').write_text(original+'\n',encoding='utf-8',newline='\n')
    for path,names,target in [
        (sdk/'src/melee/it/native_article_model.c',['model_root','nativeArticleModel_Write'],'samus_joint_access.h'),
        (sdk/'src/melee/it/itarticle_data.c',['fail','resolve','itArticle_ImportModelRoots'],'samus_model_import.h')]:
        inputs.append(path)
        (output/target).write_text('\n\n'.join(body(path.read_text(),n) for n in names)+'\n',encoding='utf-8',newline='\n')
    source_inputs=[ROOT/'examples/samus_wip.fms',ROOT/'tests/native/samus_tether_composition/fighter.component.fms',HERE/'article.component.fms']
    inputs+=source_inputs
    chunks=[]
    for path in source_inputs:
        text=path.read_text(); starts=list(re.finditer(r'^(?:article|func|export func|action|move|state|attribute|parameter)\s',text,re.M))
        chunks += [text[m.start():starts[i+1].start() if i+1<len(starts) else len(text)] for i,m in enumerate(starts)]
    def group(s):
        return 0 if s.startswith(('state ','attribute ','parameter ')) else 1 if s.startswith(('func ','export func ')) else 2 if s.startswith('article ') else 3
    candidate=output/'samus.candidate.fms'
    candidate.write_text('# UNREGISTERED healthy solver fixture; asset/modified-mode gates remain.\n'+'\n'.join(sorted(chunks,key=group))+'''
action GrappleFixtureBindings:
    on load:
        callbacks.accessory_before(Tether_Update)
        callbacks.death1(Tether_Remove)
        callbacks.accessory_hitlag(Tether_Hitlag)
''',encoding='utf-8',newline='\n')
    project=output/'samus'
    run('clone',[tool,'clone-stock',args.iso.resolve(),'Samus',project,'Samus grapple article source fixture'])
    run('import',[tool,'import-parameters',project,candidate,project/'behavior.fms'])
    generated=project/'generated.c'
    run('generate',[tool,'script-c',project/'behavior.fms','samus_grapple_source',generated])
    required=sorted(set(re.findall(r'host->(\w+)\s*==\s*0',generated.read_text())))
    (output/'unused_host.h').write_text('\n'.join(f'host.{n} = (__typeof__(host.{n}))unused_host;' for n in required)+'\n',encoding='utf-8',newline='\n')
    flags=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(output)]
    run('dll',flags+['-shared',generated,'-o',project/'grapple.dll'])
    sources=[HERE/'stock_test.c',sdk/'src/melee/it/native_article_resource.c',sdk/'src/melee/lb/native_model_joint.c',
             sdk/'src/melee/ft/native_callback_lifetime.c',ROOT/'tests/native/model_effect_absent_fixture.c',
             sdk/'src/melee/ft/native_fighter_parameters.c',sdk/'src/sysdolphin/baselib/dat.c']
    sources += [sdk/'src/sysdolphin/baselib'/('dat_'+n+'.c') for n in ['import','material','texture','animation','model','geometry','scene']]
    inputs+=sources[1:]
    run('compile',flags+sources+['-o',output/'stock_test.exe'])
    summary=json.loads(run('matrix',[output/'stock_test.exe',project/'grapple.dll',project/'fighter.dat','samus',project/'fighter_parameters.fmp']))
    verify()
    inputs += [candidate]+[project/n for n in ['fighter.dat','fighter_parameters.fmp','behavior.fms','generated.c']]
    receipt=dict(status='healthy state source slice passed',summary=summary,commands=commands,
                 snapshot=str(snapshot),input_sha256={str(n):digest(n) for n in inputs},
                 original_symbols=dict(samus=SAMUS,shared_link_geometry=LINK),
                 controlled_boundaries=['node construction and imported scalar derivation (not constructor acceptance)',
                     'HSD allocation/free, model loading, animation advancement and draw setup',
                     'matrix/vector math and ECB/stage/moving-line responses; actual lbVector_Mirror extracted',
                     'active Definition and validated creator lookup', 'dyadic deterministic random stream (draw order compared)',
                     'fighter state/ledge/capture/scale transitions and absent modified-mode hitbox/target boundaries'],
                 limits=['unregistered and incomplete constructor/rendering/animation assets',
                     'modified grapple mode1 and Up/Down/A sequence excluded; unsupported boundaries trap',
                     'AirCatch state1 wall-hit particles excluded until resource effect provenance API',
                     'native uninitialized dir_ptr x constraint uses computed dir.x only in separately counted undefined timelines',
                     'unsuffixed double0.9/0.12 boundaries use authored float literals; measured narrow differences are accepted',
                     'native hang-duration UNK_T cast retained; raw source word copied into zeroed field, warning visible',
                     'no retail PPC undefined-pointer claim; no game or activation acceptance'])
    (output/'acceptance.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(dict(receipt=str(output/'acceptance.json'),summary=summary)))


if __name__=='__main__':
    main()
