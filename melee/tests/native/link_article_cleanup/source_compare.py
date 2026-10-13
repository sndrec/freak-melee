"""Compare composed actual cleanup bodies using frozen SDK and full owned callbacks."""
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def body(text,name):
    m=re.search(r'^[^\n;{}]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
    if not m:raise RuntimeError('missing function '+name)
    i=text.index('{',m.start());depth=1;j=i+1
    while depth:depth+=(text[j]=='{')-(text[j]=='}');j+=1
    return text[m.start():j]
SOURCES={
'melee/ft/kinds/ftLink/ftlink.c':['ftLk_Init_BoomerangExists','ftLk_800EAF58'],
'melee/ft/kinds/ftLink/ftlinkspecials.c':['ftLk_SpecialS_RemoveBoomerang0','ftLk_SpecialS_RemoveBoomerang1'],
'melee/ft/kinds/ftLink/ftlinkspecialn.c':['ftLk_SpecialN_UnsetArrow','ftLk_SpecialN_UnsetFv14','ftLk_SpecialN_ProcessFv10','ftLk_SpecialN_ProcessFv14'],
'melee/ft/kinds/ftCLink/inlines.h':['checkFighter2244'],
'melee/ft/kinds/ftCLink/ftclink.c':['ftCl_Init_80149268'],
'melee/ft/kinds/ftCommon/ftCo_0D8E.c':['ftCo_800D94D8'],
'melee/it/kinds/itlinkboomerang.c':['remove_boomerang','it_802A07B4','it_802A0810','itLinkBoomerang_Logic18_Destroyed'],
'melee/it/kinds/itlinkarrow.c':['it_802A8330','itLinkArrow_Logic98_Destroyed','it_802A8A7C'],
'melee/it/kinds/itlinkbow.c':['itLinkBow_Logic100_Destroyed','it_802AF304'],
'melee/it/kinds/itclinkmilk.c':['it_802C8C34'],
'melee/it/kinds/itlinkhookshot.c':['it_802A2B10','it_802A7AAC'],
}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    snap=args.snapshot.resolve();out=args.output.resolve()
    if out.exists():p.error('use fresh output')
    def run(stage,cmd):
        r=subprocess.run(list(map(str,cmd)),cwd=ROOT,capture_output=True,text=True,timeout=180)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n')
        commands.append({'stage':stage,'command':list(map(str,cmd)),'exit':r.returncode});(out/'comparison_commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8',newline='\n')
        if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-12000:])
        return r.stdout
    # The normal preparation runner creates out and validates all frozen inputs.
    r=subprocess.run([sys.executable,'-B',str(HERE/'prepare.py'),'--snapshot',str(snap),'--output',str(out)],cwd=ROOT)
    if r.returncode:raise RuntimeError('normal preparation failed')
    commands=[];sdk=snap/'sdk';clang=snap/'compiler/clang.exe';tool=snap/'bin/native_content_tool.exe'
    extracted=[];inputs=[Path(__file__).resolve(),HERE/'stock_test.c',HERE/'recipient.fms',HERE/'prepare.py',HERE/'README.md',*sorted(HERE.glob('*.component.fms'))]
    for path,names in SOURCES.items():
        source=sdk/'src'/path;inputs.append(source)
        for n in names:extracted.append(body(source.read_text(encoding='utf-8'),n))
    (out/'cleanup_originals.h').write_text('\n\n'.join(extracted)+'\n',encoding='utf-8',newline='\n')
    native=[]
    for path,names in [('melee/ft/native_fighter_module.c',['article_signal_author','signal_article']),('melee/it/native_article.c',['nativeArticle_Creator'])]:
        source=sdk/'src'/path;inputs.append(source);native += [body(source.read_text(encoding='utf-8'),n) for n in names]
    (out/'dispatch_originals.h').write_text('\n\n'.join(native)+'\n',encoding='utf-8',newline='\n')
    item_source=sdk/'src/melee/it/item.c';inputs.append(item_source)
    item_symbols=['DestroyItemInline','ItemSwitch','func_8026A8EC_inline1','func_8026A8EC_inline3','Item_8026A8EC']
    (out/'item_removal_originals.h').write_text('\n\n'.join(body(item_source.read_text(encoding='utf-8'),n) for n in item_symbols)+'\n',encoding='utf-8',newline='\n')
    production=[sdk/'src'/n for n in ['melee/ft/native_callback_lifetime.c','melee/ft/native_fighter_parameters.c','melee/ft/native_fighter_events.c','melee/it/native_article_refs.c','melee/it/native_article_resource.c','melee/it/native_article_model.c','melee/lb/native_model_joint.c']]
    inputs+=production+[out/'cleanup_originals.h',out/'dispatch_originals.h',out/'item_removal_originals.h']
    flags=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out)]
    recipient=out/'recipient';run('recipient_clone',[tool,'clone-stock',ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso','Link',recipient,'Independent cleanup recipient'])
    run('recipient_import',[tool,'import-parameters',recipient,HERE/'recipient.fms',recipient/'behavior.fms'])
    rec=recipient/'recipient.generated.c';run('recipient_generate',[tool,'script-c',recipient/'behavior.fms','cleanup_recipient',rec]);run('recipient_dll',flags+['-shared',rec,'-o',out/'recipient.dll'])
    inputs += [recipient/n for n in ['behavior.fms','fighter_parameters.fmp','recipient.generated.c']]
    results=[]
    for family in ['link','young_link']:
        project=out/family;generated=project/'cleanup.generated.c';s=generated.read_text(encoding='utf-8')
        fields=sorted(set(re.findall(r'host->(\w+)\s*==\s*0',s)));(project/'unused_host.h').write_text('\n'.join('host.'+f+'=(__typeof__(host.'+f+'))unused_host;' for f in fields)+'\n',encoding='utf-8',newline='\n')
        run(family+'_compile',flags+['-I'+str(project),HERE/'stock_test.c',*production,'-o',project/'cleanup_test.exe'])
        result=run(family+'_matrix',[project/'cleanup_test.exe',project/'fighter_parameters.fmp',family,out/'recipient.dll',recipient/'fighter_parameters.fmp']);results.append(dict(fighter=family,**json.loads(result)))
        inputs += [project/n for n in ['fighter.dat','fighter_parameters.fmp','behavior.fms','cleanup.generated.c','unused_host.h']]
    receipt={'snapshot':str(snap),'snapshot_sha256':sha(snap/'freeze.json'),'results':results,'input_sha256':{str(x):sha(x) for x in inputs},'symbols':SOURCES,'commands':commands,'normal_preparation_receipt_sha256':sha(out/'preparation.json'),'item_removal_symbols':item_symbols,'scope':'source-only composed fighter cleanup and complete article removal/destruction callbacks; generated C invoked directly; separate ordinary DLL compiled','controlled_boundaries':['HSD fixed graph allocation/free and model-descriptor construction','Actual Item_8026A8EC: controlled hitbox/global-item counts/effects/audio cleanup; HSD free unlinks/poisons fixed fixture storage','live-list scene construction and immutable Definition lookup','root descriptors/collision initialization; no rendering or constructor/solver execution'],'production_linked':[str(x) for x in production],'limits':['no gameplay/default registration/full fighter independence','stock stale raw pointer or owner userdata destruction excluded from strict comparisons; generated safe rejection checked separately','original Hookshot leaves borrowed part/capture/hitbox dangling; authored resource cleanup invalidates them safely, classified as defined safety extension','stock fresh resource graphs configured directly; no whole tether constructor/solver claim; node/joint solvers are not run']}
    manifest=json.loads((snap/'freeze.json').read_text(encoding='utf-8'))
    for name,expected in manifest.items():
        if name.startswith(('sdk/','sdk\\','compiler/','compiler\\')) or name.replace('\\','/')=='bin/native_content_tool.exe':
            if sha(snap/name)!=expected:raise RuntimeError('frozen input changed: '+name)
    (out/'acceptance.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'receipt':str(out/'acceptance.json'),'results':results}))
if __name__=='__main__':main()
