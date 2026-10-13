"""Compare 341 genuine material shader outputs to pre-edit native kernels."""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.dont_write_bytecode=True
sys.path.insert(0,str(HERE.parent/'hsd_reservation'))
from extract import extract,function

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
    out.mkdir(parents=True);commands=[]
    def run(stage,command):
        result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':command,'exit':result.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if result.returncode:raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-16000:])
        return result.stdout
    baseline=run('head',['git','rev-parse','HEAD']).strip();base=ROOT/'src/sysdolphin/baselib'
    original={}
    for name in ('texp.c','texpdag.c','texp.h','texpdag.h','mobj.c','mobj.h','tobj.c'):
        original[name]=run('original-'+name.replace('.','-'),['git','show',baseline+':src/sysdolphin/baselib/'+name])
    arrows=[Path('A:/FreakMelee-test-archives/fighter-callback-source-census')/('fighter-'+str(i))/'fighter.dat' for i in (2,3)]
    inputs=[base/n for n in original]+list(base.rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+arrows
    inputs+=[HERE/'material_reference.py',HERE/'shader_fingerprint.h',HERE.parent/'hsd_reservation/test.c',HERE.parent/'hsd_reservation/extract.py',args.compiler]
    digest=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();hashes={str(path):digest(path) for path in inputs}
    summaries={}
    for variant in ('baseline','current'):
        folder=out/variant;folder.mkdir();extract(ROOT,folder)
        include=folder/'include';(include/'sysdolphin/baselib').mkdir(parents=True)
        if variant=='baseline':
            # Current importer headers name the new mode type. This test-only
            # declaration changes no original compiler body or old signature.
            original_headers=dict(original)
            original_headers['texp.h']=original['texp.h'].replace('int HSD_TExpCompile(', 'typedef enum HSD_TExpCompileMode { HSD_TExpCompileMode_Abort,HSD_TExpCompileMode_Checked } HSD_TExpCompileMode;\nint HSD_TExpCompile(',1)
            for name in ('texp.h','texpdag.h','mobj.h'):
                (include/'sysdolphin/baselib'/name).write_text(original_headers[name])
            # Quote includes in baseline compiler use its original declarations.
            for name in ('texp.h','texpdag.h'):(folder/name).write_text(original_headers[name])
            text=original['texp.c']
            for omitted in ('HSD_TExpSetReg','HSD_TExpSetupTev'):text=text.replace(function(text,omitted),'')
            (folder/'texp.c').write_text(text);(folder/'texpdag.c').write_text(original['texpdag.c'])
            material='#include <sysdolphin/baselib/mobj.h>\n#include <sysdolphin/baselib/tev.h>\n#include <sysdolphin/baselib/class.h>\n'
            for name in ('HSD_TexMapID2PTTexMtx','MakeColorGenTExp','TObjMakeTExp','HSD_TObjAssignResources','HSD_Index2TexCoord','HSD_Index2TexMap'):
                material+=function(original['tobj.c'],name)+'\n'
            material+='HSD_TObjInfo hsdTObj={.make_texp=TObjMakeTExp};\n'
            material+=function(original['mobj.c'],'MObjMakeTExp')+'\nHSD_MObjInfo hsdMObj={.make_texp=MObjMakeTExp};\nHSD_TObj *tobj_toon,*tobj_shadows;\n'
            material+=function(original['mobj.c'],'HSD_MObjCompileTev')+'\n';(folder/'material.c').write_text(material)
        test=(HERE.parent/'hsd_reservation/test.c').read_text()
        if variant=='baseline':test=test.replace(',HSD_TExpCompileMode_Abort','')
        test=test.replace('static void compare_shader(', '#include "shader_fingerprint.h"\nstatic void compare_shader(',1)
        test=test.replace('    const HSD_TExpTevDesc *a=first->tevdesc, *b=second->tevdesc;', '    fingerprint_shader(first);\n    const HSD_TExpTevDesc *a=first->tevdesc, *b=second->tevdesc;',1)
        # Emit an additional JSON line without changing the fixture operations.
        before,after=test.rsplit('return 0;\n}',1)
        test=before+'printf("{\\\"shader_fingerprint\\\":\\\"%016llx\\\"}\\n",shader_fingerprint);return 0;\n}'+after
        testpath=folder/'test.c';testpath.write_text(test)
        run(variant+'-os',[str(args.compiler),'-std=c++17','-O2','-ffunction-sections','-fdata-sections','-DTARGET_PC','-DAURORA','-I'+str(ROOT.parent/'aurora/include'),'-c',str(folder/'OSAlloc.cpp'),'-o',str(folder/'OSAlloc.obj')])
        units=[base/(n+'.c') for n in ('native_hsd_reservation','class','objalloc','memory','id','hash')]
        units+=[ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/melee/it/native_article_model_demand.c',testpath]
        units+=list(base.glob('dat*.c'))+[folder/(n+'.c') for n in ('satellites','texp','material','stage')]
        units+=[folder/'texpdag.c' if variant=='baseline' else base/'texpdag.c']
        flags=['-std=c11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(include),'-I'+str(HERE),'-I'+str(ROOT/'src'),'-I'+str(base),'-I'+str(ROOT/'extern/dolphin/include'),'-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref']
        run(variant+'-compile',[str(args.compiler),*flags,*map(str,units),str(folder/'OSAlloc.obj'),'-o',str(folder/'test.exe')])
        lines=run(variant+'-fixture',[str(folder/'test.exe'),*map(str,arrows)]).splitlines();summaries[variant]=[json.loads(line) for line in lines]
    if summaries['baseline']!=summaries['current']:raise RuntimeError('Native material outputs differ: '+str(summaries))
    for path,sha in hashes.items():
        if digest(Path(path))!=sha:raise RuntimeError('Input changed: '+path)
    receipt={'status':'PASS','reference_head':baseline,'summaries':summaries,'source_sha256':hashes,'commands':commands,
             'scope':'341 complete native material/shader comparisons including four imported Arrow roots, ordinary/reserved paths and recompilation; actual pre-edit MObj/TObj/TEV kernels versus current kernels',
             'limits':'native render leaves/environment and model graph loading not exercised; fingerprint excludes pointers, undefined padding and unassigned coord/map slots'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(summaries['current']))
if __name__=='__main__':main()
