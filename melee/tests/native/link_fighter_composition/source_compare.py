"""Compare actual common Catch/grounded tether and composed cleanup with generated callbacks."""
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
ACCEPTED=HERE.parent/"link_article_cleanup"
GROUNDED=HERE.parent/"link_grounded_composition"

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def body(text,name):
    m=re.search(r'^[A-Za-z_][A-Za-z0-9_ \t*]*[ \t*]'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
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
COMMON_SOURCES={
'melee/it/it_26B1.c':['it_8026BAE8'],
'melee/ft/kinds/ftCommon/ftCo_Catch.c':['ftCo_Catch_CheckInput','ftCo_800D8A38','ftCo_800D8AE0','ftCo_800D8B9C','fn_800D8BFC','ftCo_800D8C54','ftCo_Catch_Anim','ftCo_CatchDash_Anim','fn_800D8E30','fn_800D8E94'],
'melee/ft/kinds/ftCommon/ftCo_0D8E.c':['fn_800D8EC8','fn_800D9228','fn_800D949C','fn_800D952C'],
'melee/ft/kinds/ftCommon/ftCo_0D95.c':['fn_800D9C64'],
'melee/ft/kinds/ftCommon/ftCo_CatchPull.c':['fn_800D9CE8','ftCo_CatchPull_Anim','fn_800DA054','fn_800DA190'],
'melee/ft/kinds/ftCommon/ftCo_CatchWait.c':['fn_800DA1D8','ftCo_CatchWait_IASA','fn_800DA490','fn_800DA4A0','fn_800DA4C0'],
'melee/ft/kinds/ftCommon/ftCo_CaptureWait.c':['ftCo_CaptureWait_Setup','fn_800DB6C8','fn_800DB790','fn_800DBAE4'],
'melee/it/kinds/itlinkhookshot.c':['itLinkHookshot_Logic20_PickedUp_inline','it_802A77DC','it_802A7840','it_802A78B8'],
}

PHASE_SOURCES={
'melee/ft/kinds/ftLink/ftlink.c':['ftLk_Init_OnDeath','ftLk_Init_OnItemPickupExt','ftLk_Init_OnItemDropExt','ftLk_Init_OnItemPickup','ftLk_Init_OnItemDrop','ftLk_Init_OnItemVisible','ftLk_Init_OnItemInvisible','ftLk_Init_OnKnockbackEnter','ftLk_Init_OnKnockbackExit'],
'melee/ft/kinds/ftCLink/ftclink.c':['ftCl_Init_OnDeath','ftCl_Init_OnItemPickupExt','ftCl_Init_OnItemDropExt','ftCl_Init_OnItemPickup','ftCl_Init_OnItemDrop','ftCl_Init_80149318'],
'melee/ft/kinds/ftCLink/ftclinkappeals.c':['ftCl_AppealS_Anim','ftCl_AppealS_IASA','ftCl_AppealS_Phys','ftCl_AppealS_Coll'],
'melee/ft/ftafterimage.c':['read_trail'],
}

CLOSURE_SOURCES={
'melee/ft/kinds/ftLink/ftlinkattackair.c':['ftLk_AttackAir_Enter','lwOnHit','lwDoAnimHitCapsule','lwOnAnim','ftLk_AttackAir_800EB3BC'],
'melee/ft/kinds/ftLink/ftlink.c':['ftLk_800EB334'],
'melee/ft/kinds/ftCLink/ftclink.c':['ftCl_Init_80149114','ftCl_Init_8014919C'],
'melee/ft/kinds/ftCommon/ftCo_AttackAir.c':['ftCo_AttackAir_Anim'],
'melee/ft/kinds/ftCommon/ftCo_Guard.c':['ftCo_80092ED8'],
'melee/ft/ftcoll.c':['ftColl_8007B1B8','ftColl_8007B064'],
'melee/lb/lbcollision.c':['lbColl_80008434','lbColl_80008440'],
'melee/ft/ftattacks4combo.c':['ftCo_800CECE8','ftCo_800CED30','ftCo_AttackS42_Anim','ftCo_AttackS42_IASA','ftCo_AttackS42_Phys','ftCo_AttackS42_Coll'],
'melee/ft/kinds/ftCommon/ftCo_AttackS4.c':['ftCo_AttackS4_Anim','ftCo_AttackS4_IASA','ftCo_AttackS4_Phys','ftCo_AttackS4_Coll'],
'melee/ft/native_fighter_module.c':['common_attribute'],
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
    extracted=[];inputs=[Path(__file__).resolve(),GROUNDED/'stock_test.c',ACCEPTED/'recipient.fms',HERE/'prepare.py',*sorted(HERE.glob('*.component.fms'))]
    for path,names in SOURCES.items():
        source=sdk/'src'/path;inputs.append(source)
        for n in names:extracted.append(body(source.read_text(encoding='utf-8'),n))
    (out/'cleanup_originals.h').write_text('\n\n'.join(extracted)+'\n',encoding='utf-8',newline='\n')
    common=[]
    source=sdk/'src/melee/ft/kinds/ftCommon/ftCo_0D8E.c';text=source.read_text(encoding='utf-8')
    layout=text[text.index('typedef struct {'):text.index('} ftCo_LinkCatchAttrs;')+len('} ftCo_LinkCatchAttrs;')]
    for path,names in COMMON_SOURCES.items():
        source=sdk/'src'/path;inputs.append(source)
        common.extend(body(source.read_text(encoding='utf-8'),n) for n in names)
    declarations=[b[:b.index('{')].strip()+';' for b in common]
    (out/'common_originals.h').write_text(layout+'\n'+'\n\n'.join(declarations+common)+'\n',encoding='utf-8',newline='\n')
    phase=[]
    for path,names in PHASE_SOURCES.items():
        source=sdk/'src'/path;inputs.append(source);phase.extend(body(source.read_text(encoding='utf-8'),n) for n in names)
    (out/'phase_originals.h').write_text('\n\n'.join(phase)+'\n',encoding='utf-8',newline='\n')
    inputs.extend([out/'phase_originals.h',HERE/'phase_test.inc.c'])
    closure=[]
    for path,names in CLOSURE_SOURCES.items():
        source=sdk/'src'/path;inputs.append(source);closure.extend(body(source.read_text(encoding='utf-8'),n) for n in names)
    declarations=[b[:b.index('{')].strip()+';' for b in closure]
    (out/'closure_originals.h').write_text('\n\n'.join(declarations+closure)+'\n',encoding='utf-8',newline='\n')
    inputs.extend([out/'closure_originals.h',HERE/'common_test.inc.c'])
    load=[]
    for path,names in {
        'melee/ft/kinds/ftLink/ftlink.c':['ftLk_Init_OnLoadForCLink','ftLk_Init_OnLoad','ftLk_Init_LoadSpecialAttrs'],
        'melee/ft/kinds/ftCLink/ftclink.c':['ftCl_Init_OnLoad'],
        'melee/ft/native_fighter_module.c':['copy_ext_attributes','reload_ext_attributes'],
    }.items():
        source=sdk/'src'/path;inputs.append(source);load.extend(body(source.read_text(encoding='utf-8'),n) for n in names)
    declarations=[b[:b.index('{')].strip()+';' for b in load]
    (out/'load_originals.h').write_text('\n\n'.join(declarations+load)+'\n',encoding='utf-8',newline='\n')
    inputs.extend([out/'load_originals.h',HERE/'load_test.inc.c'])
    native=[]
    for path,names in [('melee/ft/native_fighter_module.c',['article_signal_author','signal_article']),('melee/it/native_article.c',['nativeArticle_Creator'])]:
        source=sdk/'src'/path;inputs.append(source);native += [body(source.read_text(encoding='utf-8'),n) for n in names]
    (out/'dispatch_originals.h').write_text('\n\n'.join(native)+'\n',encoding='utf-8',newline='\n')
    item_source=sdk/'src/melee/it/item.c';inputs.append(item_source)
    item_symbols=['DestroyItemInline','ItemSwitch','func_8026A8EC_inline1','func_8026A8EC_inline3','Item_8026A8EC']
    (out/'item_removal_originals.h').write_text('\n\n'.join(body(item_source.read_text(encoding='utf-8'),n) for n in item_symbols)+'\n',encoding='utf-8',newline='\n')
    inputs.extend(sorted(ACCEPTED.glob('*.component.fms')))
    production=[sdk/'src'/n for n in ['melee/ft/native_callback_lifetime.c','melee/ft/native_fighter_parameters.c','melee/ft/native_fighter_trail.c','melee/ft/native_fighter_events.c','melee/it/native_article_refs.c','melee/it/native_article_resource.c','melee/it/native_article_model.c','melee/lb/native_model_joint.c']]
    inputs+=production+[out/'cleanup_originals.h',out/'common_originals.h',out/'dispatch_originals.h',out/'item_removal_originals.h']
    flags=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out)]
    recipient=out/'recipient';run('recipient_clone',[tool,'clone-stock',ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso','Link',recipient,'Independent cleanup recipient'])
    run('recipient_import',[tool,'import-parameters',recipient,ACCEPTED/'recipient.fms',recipient/'behavior.fms'])
    rec=recipient/'recipient.generated.c';run('recipient_generate',[tool,'script-c',recipient/'behavior.fms','cleanup_recipient',rec]);run('recipient_dll',flags+['-shared',rec,'-o',out/'recipient.dll'])
    inputs += [recipient/n for n in ['behavior.fms','fighter_parameters.fmp','recipient.generated.c']]
    results=[]
    for family in ['link','young_link']:
        project=out/family;generated=project/'fighter.generated.c';s=generated.read_text(encoding='utf-8')
        fields=sorted(set(re.findall(r'host->(\w+)\s*==\s*0',s)));(project/'unused_host.h').write_text('\n'.join('host.'+f+'=(__typeof__(host.'+f+'))unused_host;' for f in fields)+'\n',encoding='utf-8',newline='\n')
        fixture=project/'composition_test.c'
        test=(GROUNDED/'stock_test.c').read_text(encoding='utf-8').replace('"grounded.generated.c"','"fighter.generated.c"')
        test=test.replace('.fighter_root=family==Ft_Kind_Link?', '.joint_count=family==Ft_Kind_Link?75:79,.special_ground_state={344,350,356,358},.special_air_state={347,353,357,359},.fighter_root=family==Ft_Kind_Link?')
        test=test.replace('int main(int argc,char** argv)', 'static void phase_init(NativeFighterHost*);\nstatic void phase_matrix(void);\nstatic void common_init(NativeFighterHost*);\nstatic void common_matrix(void);\nstatic void load_init(NativeFighterHost*);\nstatic void load_matrix(void);\nint main(int argc,char** argv)')
        test=test.replace(' init_ground_host(&host);',' init_ground_host(&host);phase_init(&host);common_init(&host);load_init(&host);')
        test=test.replace('CHECK(part==FtPart_RThumbNb||part==FtPart_XRotN);','CHECK(part==FtPart_RThumbNb||part==FtPart_XRotN||part==FtPart_LThumbNb);')
        test=test.replace(' printf("{\\"assertions', ' phase_matrix();common_matrix();load_matrix();\n printf("{\\"assertions')
        test=test.replace('void ftAnim_8006EBA4(Fighter_GObj* g){(void)g;abort();}', 'void ftAnim_8006EBA4(Fighter_GObj* g){CHECK(creator_side(g)==current);event(40,0,0);}')
        test+='\n'+(HERE/'phase_test.inc.c').read_text(encoding='utf-8')+'\n'+(HERE/'common_test.inc.c').read_text(encoding='utf-8')+'\n'+(HERE/'load_test.inc.c').read_text(encoding='utf-8')
        fixture.write_text(test,encoding='utf-8',newline='\n')
        run(family+'_compile',flags+['-I'+str(project),fixture,*production,'-o',project/'grounded_test.exe'])
        result=run(family+'_matrix',[project/'grounded_test.exe',project/'fighter_parameters.fmp',family,out/'recipient.dll',recipient/'fighter_parameters.fmp']);phase_match=re.search(r'phase cases: lifecycle=(\d+) taunt=(\d+) trail=(\d+)',(out/(family+'_matrix.log')).read_text(encoding='utf-8'))
        if not phase_match:raise RuntimeError('missing executed phase census')
        closure_match=re.search(r'closure cases: dair=(\d+) shield=(\d+) combo=(\d+)',(out/(family+'_matrix.log')).read_text(encoding='utf-8'))
        if not closure_match:raise RuntimeError('missing executed closure census')
        load_match=re.search(r'load cases: (\d+)',(out/(family+'_matrix.log')).read_text(encoding='utf-8'))
        if not load_match:raise RuntimeError('missing executed load census')
        results.append(dict(fighter=family,load_cases=int(load_match[1]),dair_cases=int(closure_match[1]),shield_cases=int(closure_match[2]),combo_cases=int(closure_match[3]),**json.loads(result),lifecycle_cases=int(phase_match[1]),taunt_cases=int(phase_match[2]),trail_cases=int(phase_match[3])))
        inputs += [project/n for n in ['fighter.dat','fighter_parameters.fmp','behavior.fms','fighter.generated.c','unused_host.h','composition_test.c']]
    receipt={'snapshot':str(snap),'snapshot_sha256':sha(snap/'freeze.json'),'results':results,'input_sha256':{str(x):sha(x) for x in inputs},'phase_symbols':PHASE_SOURCES,'closure_symbols':CLOSURE_SOURCES,'symbols':SOURCES,'commands':commands,'normal_preparation_receipt_sha256':sha(out/'preparation.json'),'item_removal_symbols':item_symbols,'common_symbols':COMMON_SOURCES,'scope':'Dair entry/hit/animation, carried shield/recoil, S4 first/second continuation, load/reload, ordinary lifecycle, Young Link taunt and native sword-trail reader plus full assembled special/article/lifecycle module exercised through actual common Catch input/entry/frame/exit, CatchPull/CatchWait and victim CaptureWait bodies; native Link/CLink policy versus generated eight Catch hooks, grounded article mode changes and full immutable-author cleanup composition; separate ordinary author and recipient DLLs compiled','controlled_boundaries':['HSD fixed graph allocation/free and model-descriptor construction; original OnLoad and actual copy/reload bodies execute, clip72 length and model graph insertion have matched controlled boundary calls; native article registration kind set checked against immutable module metadata','Dair uses actual capsule enabling/victim-list clearing; shared set-damage scaling and input acceptance return controlled values identically in both paths; common motion boundary records state/frame and does not run the whole transition engine', 'ordinary held-item heavy/hold kind queried from controlled data; actual Fighter_OnItem/Knockback inline bodies execute with animation presentation boundaries recorded','Milk creation allocates matched fixed Item storage; constructor/state0 engine setup is not executed; actual original Appeal entry and four phase bodies versus generated module decisions','sword owned scalar/byte values supplied from normal-imported FMP to original SwordAttrs; actual read_trail and nativeFighterTrail_Valid run, GX draw excluded','Actual Item_8026A8EC: controlled hitbox/global-item counts/effects/audio cleanup; HSD free unlinks/poisons fixed fixture storage','live-list scene construction and immutable Definition lookup','common motion entry, animation availability, held-item throw priority, registration, capture flags/effects, sound and segment collision result controlled; real common bodies retain ordering', 'Hookshot creation boundary populates matched fixed graphs and declared state, then compares native versus generated transition/scaling/removal paths; on_created/held or flight solvers are not executed', 'semantic bone/world-coordinate and live-root-scale reads controlled, with instance scale1.25 distinct from root scale2'],'production_linked':[str(x) for x in production],'limits':['no gameplay/default registration/full fighter independence; Bow fighter phases, Spin/Bomb and article flight solvers are not exercised by this matrix; decoded clip/HSD installation are controlled boundaries','stock stale raw pointer or owner userdata destruction excluded from strict comparisons; generated safe rejection checked separately','original Hookshot leaves borrowed part/capture/hitbox dangling; authored resource cleanup invalidates them safely, classified as defined safety extension','stock fresh resource graphs configured directly; no whole tether constructor/solver claim; node/joint solvers are not run', 'native DA190 is executed and proven a no-op for Link/CLink; authored false follow leaves accessory1 NULL instead of retaining a no-op pointer', 'missing resource capture anchor safely falls back to the common owned bone; original missing item endpoint is undefined and excluded from strict entry comparison', 'frozen compiler generated discarded result/equality expressions and original gobj==NULL no-op generate warnings; logs preserve all diagnostics']}
    manifest=json.loads((snap/'freeze.json').read_text(encoding='utf-8'))
    for name,expected in manifest.items():
        if name.startswith(('sdk/','sdk\\','compiler/','compiler\\')) or name.replace('\\','/')=='bin/native_content_tool.exe':
            if sha(snap/name)!=expected:raise RuntimeError('frozen input changed: '+name)
    (out/'acceptance.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps({'receipt':str(out/'acceptance.json'),'results':results}))
if __name__=='__main__':main()
