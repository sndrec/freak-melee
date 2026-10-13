"""Selectable-stage native owner migration cohort, old/new rendered CPU matches."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/"native_snapshot_census"))
from trace_interval import compare_interval
from run import OWNERS

OWNER_COUNT=len(OWNERS)

ROOT=Path(__file__).resolve().parents[3]
STAGES=['battlefield','final-destination','dream-land','green-greens','jungle-japes',
        'yoshis-story','yoshis-island','flat-zone','yoshis-island-64','kongo-jungle-64',
        'fountain-of-dreams','rainbow-cruise','great-bay','hyrule-temple','onett',
        'pokemon-stadium','brinstar','brinstar-depths','fourside','mushroom-kingdom',
        'mushroom-kingdom-ii','poke-floats','big-blue','icicle-mountain',
        'princess-peachs-castle','kongo-jungle','corneria','mute-city','venom']

def main():
    p=argparse.ArgumentParser();p.add_argument('--baseline',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--samples',type=int,default=2000)
    p.add_argument('--stage',choices=STAGES,action='append')
    p.add_argument('--snapshot-census',action='store_true',help='require explicit shared-stage Save/Validate at every sampled boundary')
    p.add_argument('--rewind',action='store_true',help='test an eight-tick older own component capture and recover the complete current endpoint')
    p.add_argument('--self-restore',action='store_true',help='require current-boundary Load and byte-identical recapture for all census owners')
    p.add_argument('--fighters',nargs=4,default=['ganondorf','zelda','iceclimbers','mrgameandwatch'])
    p.add_argument('--expect-baseline-green-import-failure',action='store_true',
                   help='require the previous GrGr descriptor failure; candidate must complete normally')
    p.add_argument('--resimulate',action='store_true',help='repeat eight rendered input ticks from compact state and require exact broad endpoint / intermediate traces')
    args=p.parse_args()
    if args.resimulate:args.rewind=True
    if args.rewind:args.self_restore=True
    if args.self_restore and not args.snapshot_census:p.error('--self-restore requires --snapshot-census')
    out=args.output.resolve();baseline=args.baseline.resolve()
    if out.exists():p.error('fresh output directory required')
    if not 1<=args.samples<=100000 or not baseline.is_file():p.error('invalid samples/baseline')
    out.mkdir(parents=True);content=out/'content';content.mkdir()
    candidate=ROOT/'build/native/melee_native.exe'
    iso=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso'
    comparer=ROOT/'tools/compare_fighter_parity.py'
    spec=importlib.util.spec_from_file_location('comparer',comparer)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    replay=out/'inputs.fmr';pad=struct.pack('<HbbbbBBBBbxI',0,0,0,0,0,0,0,0,0,0,0)
    sample=struct.pack('<Q',0)+pad*4+struct.pack('<II',0,0)
    replay.write_bytes(struct.pack('<8sQII',b'FMREPLAY',0,0x12345678,1)+sample*args.samples)
    sources=[ROOT/'tests/native/native_snapshot_census/trace_interval.py',ROOT/'src/melee/it/item.c',ROOT/'src/melee/it/native_item_pool.c',ROOT/'src/melee/it/native_item_link_pool.c',
        *sorted((ROOT/'src/melee/it').glob('native_article_resource*.[ch]')),
        *sorted((ROOT/'src/melee/it/kinds').glob('*.c')),
             ROOT/'src/melee/it/native_item_pool.h',ROOT/'src/melee/it/native_item_link_pool.h',ROOT/'src/melee/gr/ground.c',ROOT/'src/melee/gr/native_ground_pool.c',
             ROOT/'src/melee/gr/native_ground_pool.h',
             ROOT/'src/sysdolphin/baselib/native_gobj_pool_storage.c',
             ROOT/'src/sysdolphin/baselib/native_gobj_snapshot.h',
             ROOT/'src/sysdolphin/baselib/gobj.c',
             ROOT/'src/sysdolphin/baselib/pobj.c',
             ROOT/'src/sysdolphin/baselib/tobj.c',
             ROOT/'src/sysdolphin/baselib/gobjproc.c',
             ROOT/'src/sysdolphin/baselib/initialize.c',
             ROOT/'src/sysdolphin/baselib/object.c',
             ROOT/'src/sysdolphin/baselib/objalloc.c',
             ROOT/'src/sysdolphin/baselib/class.c',
             ROOT/'src/sysdolphin/baselib/native_class_snapshot.h',
             ROOT/'src/sysdolphin/baselib/id.c',
             ROOT/'src/sysdolphin/baselib/native_id_snapshot.h',
             ROOT.parent/'aurora/lib/dolphin/os/OSAlloc.cpp',
             ROOT.parent/'aurora/include/dolphin/os/OSHeapSnapshot.h',
             ROOT/'src/sysdolphin/baselib/native_objalloc_snapshot.h',
             ROOT/'src/melee/gm/native_match_object_census.c',
             ROOT/'src/melee/gm/native_match_snapshot_pool.c',
             ROOT/'src/melee/mp/mpcoll.c',comparer,Path(__file__).resolve()]
    sources += [ROOT/'src/melee/gm'/name for name in
                ['gmvs.c','gm_1879.c','gm_1798.c','gm_186E.c','gm_1832.c','gmregtyfall.c']]
    sources += [ROOT/'src/melee/ef'/n for n in ['efdata.c','eflib.c','eflib.h','native_effect_control_snapshot.c','native_effect_control_snapshot.h']]
    sources += [ROOT/'src/sysdolphin/baselib'/n for n in ['particle.c','generator.c','psappsrt.c','psdisp.c','psdisp_state.c','psdisp_state_internal.h','particle_state.c','particle_state_internal.h','native_particle_control_snapshot.c','native_particle_control_snapshot.h']]
    sources += sorted((ROOT/'src/melee/mp').glob('native_collision*.[ch]'))
    sources += [ROOT/'src/melee/mp'/name for name in ['mplib.c','mpisland.c']]
    sources += [ROOT/'src/melee/gr/grdynamicattr.c']
    sources += [ROOT/'src/melee/gm'/name for name in
                ['native_semantic_state.c','native_semantic_state.h',
                 'native_replay_state.c','native_replay_item_fields.c',
                 'native_replay_item_fields.h']]
    sources += [ROOT/'src/melee/vi'/name for name in
                ['vi0102.c','vi0401.c','vi0501.c','vi0502.c','vi0601.c','vi0801.c','vi1101.c','vi1201v1.c','vi1201v2.c']]
    sources += [ROOT/'src/melee/gr'/name for name in
                ['stage_state.c','stage_state_internal.h','native_stage_snapshot.c',
                 'native_stage_snapshot.h','grlib.c','stage.c','types.h']]
    sources += [ROOT/'src/melee/gm'/name for name in ['gm_1A45.c','native_snapshot_census.c','native_snapshot_census.h','native_match_snapshot.h','native_match_snapshot_capture.c','native_match_snapshot_load.c','native_snapshot_actor_graph.c','native_snapshot_actor_graph.h','native_snapshot_class_graph.c','native_snapshot_class_graph.h','native_snapshot_endpoint.c','native_snapshot_endpoint.h','native_rollback_regions.c','native_rollback_regions.h']]
    sources += [ROOT/'src/melee/gr'/name for name in
                ['stage_hazard_state.c','stage_hazard_state_internal.h','grrcruise.c','grrcruise_data.h','grizumi.c','native_stage_hazard_snapshot.c',
                 'native_stage_hazard_snapshot.h','grcorneria.c','grvenom.c','grgreens.c','grkraid.c','grzebes.c','grkongo.c','grkongo.static.h','gricemt.c','gricemt_data.h']]
    sources += [ROOT/'src/melee/gr'/name for name in ['native_green_grid.c','native_green_grid_snapshot.c','native_green_grid.h']]
    sources += [ROOT/'src/melee/lb'/name for name in ['lb_00F9.c','dynamics_state.c','dynamics_state_internal.h','native_dynamics_snapshot.c','native_dynamics_snapshot.h','types.h']]
    sources += [ROOT/'src/melee/ft/ftdynamics.c']
    sources += [ROOT/'src/melee/lb/lb_00F9.h']
    sources += [ROOT/'src/melee/gr/grpura.c',ROOT/'src/melee/gr/grbattle.c']
    sources += [ROOT/'src/melee/gr/grbigblue.c',ROOT/'src/melee/gr/grbigblue.static.h']
    sources += [ROOT/'src/melee/gr'/n for n in ['grzakogenerator.c','grzakogenerator.h','stage_generator_state.c','stage_generator_state.h','native_stage_generator_snapshot.c','native_stage_generator_snapshot.h','stage_spawn_state.c','stage_spawn_state.h','native_stage_spawn_snapshot.c','native_stage_spawn_snapshot.h']]
    sources += [ROOT/'src/sysdolphin/baselib'/n for n in ['lobj.c','lobj_state.c','lobj_state_internal.h','native_light_control_snapshot.c','native_light_control_snapshot.h']]
    sources += [ROOT/'src/melee/lb'/n for n in ['lb_00F9.c','lb_020A.c']]
    sources += [ROOT/'src/melee/gm'/n for n in ['native_replay.c','native_replay.h','native_replay_state.c']]
    sources += [ROOT/'src/sysdolphin/baselib'/n for n in ['audio_rollback.c','audio_rollback.h','axdriver.c']]
    sources += [ROOT/'src/melee/gm'/n for n in ['native_texture_identity_snapshot.c','native_texture_identity_snapshot.h']]
    sources += [ROOT.parent/'aurora/lib/dolphin/gx/GXTexture.cpp']
    sources += [ROOT/n for n in ['src/melee/gm/gmvs.c','src/melee/gm/gmvs_state.c','src/melee/gm/gmvs_state_internal.h','src/melee/gm/native_versus_control_snapshot.c','src/melee/gm/native_versus_control_snapshot.h','src/melee/it/itspawn.c','src/melee/it/itspawn_state.c','src/melee/it/itspawn_state_internal.h','src/melee/if/ifstatus.c','src/melee/if/ifstatus_state.c','src/melee/if/ifstatus_state_internal.h']]
    sources += [ROOT/'src/melee/gm/gmregtyfall.c']
    sources += sorted((ROOT/'src/melee/vi').glob('*.c'))
    sources += [ROOT/'src/melee/ft/ftparts_render.c',
                ROOT/'src/sysdolphin/baselib/native_render_phase.h',
                ROOT/'tests/native/fighter_parts_test.c']
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    inputs={str(path):sha(path) for path in [baseline,candidate,iso,replay]+sources}
    runtime_files=list(baseline.parent.glob('*.dll'))+list(candidate.parent.glob('*.dll'))
    runtime_files += [p for folder in [baseline.parent/'ui',candidate.parent/'ui']
                      for p in folder.rglob('*') if p.is_file()]
    inputs.update({str(path):sha(path) for path in runtime_files})
    receipt={'status':'RUNNING','inputs':inputs,'samples':args.samples,'fighters':args.fighters,'self_restore':args.self_restore,'rewind':args.rewind,'cases':[],
             'item_field_trace':os.environ.get('FREAK_MELEE_ITEM_FIELDS'),
             'skip_model_geometry':os.environ.get('FREAK_MELEE_SKIP_MODEL_RESIM'),
             'skip_gpu_output':os.environ.get('FREAK_MELEE_NO_GPU_RESIM'),
             'limits':['rendered CPU gameplay, not every stage interaction or pixel comparison',
                       'normal callbacks retained; fast pacing can differ for audio-clock stage behavior',
                       'no complete explicit match state, savestate timing or multi-PC claim']}
    path=out/'receipt.json'
    for stage in args.stage or STAGES:
        case={'stage':stage,'runs':[]};traces={}
        for label,exe in [('baseline',baseline),('candidate',candidate)]:
            log=out/f'{stage}-{label}.log';census=out/f'{stage}-{label}-census.log'
            command=[str(exe),'--iso',str(iso),'--match','--stage',stage,'--mode','stock',
                     '--stocks','99','--time','0','--items','medium','--hidden','--mute','--fast',
                     '--object-census','--trace-fighter-state','--trace-actions','--replay-inputs',
                     str(replay),'--content-root',str(content)]
            for port,fighter in enumerate(args.fighters,1):
                command += [f'--p{port}',f'cpu:{fighter}:9']
            env=os.environ.copy();env['FREAK_MELEE_OBJECT_CENSUS']=str(census)
            env['FREAK_MELEE_SNAPSHOT_SELF_RESTORE']='1' if args.self_restore and label=='candidate' else '0'
            env['FREAK_MELEE_SNAPSHOT_REWIND']='1' if args.rewind and label=='candidate' else '0'
            env['FREAK_MELEE_SNAPSHOT_RESIMULATE']='1' if args.resimulate and label=='candidate' else '0'
            env['FREAK_MELEE_SNAPSHOT_RESIMULATION_RAW']=str(out/f'{stage}-resim-raw.log')
            diagnostic=out/f'{stage}-{label}-snapshots.log'
            if args.snapshot_census and label=='candidate':
                command.append('--snapshot-census')
                env['FREAK_MELEE_SNAPSHOT_CENSUS']=str(diagnostic)
            errors=[]
            with log.open('wb') as stream:
                try:
                    result=subprocess.run(command,cwd=ROOT/'build/native',env=env,
                        stdout=stream,stderr=subprocess.STDOUT,timeout=240,
                        creationflags=subprocess.CREATE_NO_WINDOW)
                    code=result.returncode
                except subprocess.TimeoutExpired:
                    code=None;errors.append('timeout; task-owned child force-killed')
            text=log.read_text(encoding='utf-8',errors='replace')
            if code!=0:errors.append(f'exit {code}')
            if f'[replay] End after {args.samples} controller samples.' not in text:errors.append('replay incomplete')
            if '[replay] Nonfinite' in text or '[script] Fault' in text:errors.append('nonfinite/source fault')
            if not census.is_file():errors.append('census missing')
            else:
                counts=census.read_text(encoding='utf-8',errors='replace')
                if re.search(r'errors=[1-9]',counts):errors.append('census owner/topology error')
                if label=='candidate':
                    for owner in ['ground_owner','item_owner']:
                        if 'name='+owner not in counts:errors.append(owner+' not observed')
            if args.snapshot_census and label=='candidate':
                diag=diagnostic.read_text(errors='replace') if diagnostic.exists() else ''
                if args.resimulate:
                    comparisons=re.findall(r'^\[snapshot-census\] resim-compare owner=(\S+) match=(\d+) bytes=(\d+) first_difference=(\d+)$',diag,re.M)
                    summary=re.search(r'^\[snapshot-census\] resim-end ticks=(\d+) compared=(\d+) expected=(\d+) endpoint_restored=(\d+)$',diag,re.M)
                    audio=re.search(r'^\[snapshot-census\] resim-audio reused=(\d+) new=(\d+) canceled=(\d+)$',diag,re.M)
                    if not audio or any(int(value) for value in audio.groups()[1:]):errors.append('same-input resimulation changed audio starts')
                    raw_end=re.search(r'^\[snapshot-census\] resim-raw differing_bytes=(\d+)$',diag,re.M)
                    if len(comparisons)!=OWNER_COUNT or any(row[1]!='1' for row in comparisons):errors.append('compact resimulation owner mismatch')
                    if not summary or tuple(map(int,summary.groups()))!=(8,OWNER_COUNT,OWNER_COUNT,1):errors.append('compact resimulation endpoint recovery incomplete')
                    if not raw_end or int(raw_end.group(1))!=0:errors.append('resimulation broad endpoint differs')
                    if '[snapshot-census] resim-null-load restored=1 replay_rewound=1' not in diag:errors.append('null-report compact reset or replay rewind absent')
                    case['resimulation']=dict(compared=len(comparisons),summary=summary.groups() if summary else [],raw_differing_bytes=int(raw_end.group(1)) if raw_end else None,audio=list(map(int,audio.groups())) if audio else [])
                summaries=re.findall(r'^\[snapshot-census\] summary owner=stage (.+)$',diag,re.M)
                counts=dict(re.findall(r'(\w+)=([\w.]+)',summaries[0])) if len(summaries)==1 else {}
                expected=args.samples//120
                if not expected or any(int(counts.get(k,0))!=expected for k in ('attempts','saved','validated')):
                    errors.append('shared stage snapshot coverage incomplete')
                case['stage_snapshot']=counts
                hazard_summaries=re.findall(r'^\[snapshot-census\] summary owner=stage-hazards (.+)$',diag,re.M)
                hazard_counts=dict(re.findall(r'(\w+)=([\w.]+)',hazard_summaries[0])) if len(hazard_summaries)==1 else {}
                if any(int(hazard_counts.get(k,0))!=expected for k in ('attempts','saved','validated')):
                    errors.append('private hazard snapshot coverage incomplete')
                case['hazard_snapshot']=hazard_counts
                grid_summaries=re.findall(r'^\[snapshot-census\] summary owner=green-grid (.+)$',diag,re.M)
                grid_counts=dict(re.findall(r'(\w+)=([\w.]+)',grid_summaries[0])) if len(grid_summaries)==1 else {}
                if any(int(grid_counts.get(k,0))!=expected for k in ('attempts','saved','validated')):
                    errors.append('Green Greens grid snapshot coverage incomplete')
                case['grid_snapshot']=grid_counts
                dynamics_summaries=re.findall(r'^\[snapshot-census\] summary owner=dynamics (.+)$',diag,re.M)
                dynamics_counts=dict(re.findall(r'(\w+)=([\w.]+)',dynamics_summaries[0])) if len(dynamics_summaries)==1 else {}
                if any(int(dynamics_counts.get(k,0))!=expected for k in ('attempts','saved','validated')):
                    errors.append('secondary dynamics/wind snapshot coverage incomplete')
                case['dynamics_snapshot']=dynamics_counts
                generator_summary=re.findall(r'^\[snapshot-census\] summary owner=stage-generator (.+)$',diag,re.M)
                generator_counts=dict(re.findall(r'(\w+)=([\w.]+)',generator_summary[0])) if len(generator_summary)==1 else {}
                if any(int(generator_counts.get(k,0))!=expected for k in ('attempts','saved','validated')):
                    errors.append('stage generator snapshot coverage incomplete')
                case['generator_snapshot']=generator_counts
                spawn_summary=re.findall(r'^\[snapshot-census\] summary owner=stage-spawns (.+)$',diag,re.M)
                spawn_counts=dict(re.findall(r'(\w+)=([\w.]+)',spawn_summary[0])) if len(spawn_summary)==1 else {}
                if any(int(spawn_counts.get(k,0))!=expected for k in ('attempts','saved','validated')):
                    errors.append('stage spawn configuration coverage incomplete')
                case['spawn_snapshot']=spawn_counts
                case['snapshot_log']=str(diagnostic)
                if '[snapshot-census] aggregate-buffer-tests refused=1 metadata_unchanged=1' not in diag:
                    errors.append('shared aggregate preflight refusal proof absent')
                if args.self_restore:
                    summaries=re.findall(r'^\[snapshot-census\] summary owner=(\S+) (.+)$',diag,re.M)
                    names={n for n,_ in summaries}
                    if len(names)!=OWNER_COUNT:errors.append('self-restore owner set incomplete')
                    for name,fields in summaries:
                        fields=dict(re.findall(r'(\w+)=([\w.]+)',fields))
                        if any(int(fields.get(k,0))!=expected for k in ('attempts','saved','validated')):
                            errors.append('self-restore capture coverage incomplete: '+name)
                    loaded=re.findall(r'^\[snapshot-census\] self-restore-load owner=(\S+) restored=(\d+)(?: elapsed_us=[\d.]+)?$',diag,re.M)
                    compared=re.findall(r'^\[snapshot-census\] self-restore-compare owner=(\S+) match=(\d+) bytes=(\d+)$',diag,re.M)
                    total=expected*OWNER_COUNT
                    if len(loaded)!=total+(OWNER_COUNT if args.rewind else 0) or {n for n,_ in loaded}!=names or any(v!='1' for _,v in loaded):errors.append('same-boundary Load coverage incomplete')
                    if len(compared)!=total or {n for n,_,_ in compared}!=names or any(v!='1' for _,v,_ in compared):errors.append('same-boundary recapture equality incomplete')
                    summary=re.search(r'^\[snapshot-census\] self-restore-summary restored=(\d+) compared=(\d+) expected=(\d+)$',diag,re.M)
                    if not summary or any(int(v)!=total for v in summary.groups()):errors.append('self-restore summary incomplete')
                    actor_graphs=re.findall(r'^\[snapshot-census\] actor-graph valid=(\d+) fighters=(\d+) ground=(\d+) items=(\d+)$',diag,re.M)
                    if len(actor_graphs)!=expected+(1 if args.rewind else 0) or any(valid!='1' for valid,_,_,_ in actor_graphs):errors.append('candidate actor graph validation incomplete')
                    class_graphs=re.findall(r'^\[snapshot-census\] class-graph valid=(\d+) blocks=(\d+) types=(\d+)$',diag,re.M)
                    if len(class_graphs)!=expected+(1 if args.rewind else 0) or any(valid!='1' for valid,_,_ in class_graphs):
                        errors.append('candidate class graph validation incomplete')
                    case['self_restore_checks']=dict(class_graphs=len(class_graphs),actor_graphs=len(actor_graphs),loaded=len(loaded),compared=len(compared),expected=total)
                    if args.rewind:
                        rewind_summary=re.search(r'^\[snapshot-census\] rewind-summary loaded=(\d+) compared=(\d+) endpoint_restores=(\d+) pending=(\d+)$',diag,re.M)
                        raw=re.search(r'^\[snapshot-census\] rewind-raw-summary differing_bytes=(\d+)$',diag,re.M)
                        activity=re.search(r'^\[snapshot-census\] rewind-object-activity created=(\d+) destroyed=(\d+) reused=(\d+)$',diag,re.M)
                        comparisons=re.findall(r'^\[snapshot-census\] rewind-compare owner=(\S+) match=(\d+) bytes=(\d+)$',diag,re.M)
                        if not rewind_summary or tuple(map(int,rewind_summary.groups()))!=(OWNER_COUNT,OWNER_COUNT,1,0):errors.append('older restore/recapture or endpoint recovery incomplete')
                        if not raw or not activity:errors.append('rewind raw coverage/activity absent')
                        if len(comparisons)!=OWNER_COUNT or {n for n,_,_ in comparisons}!=names or any(v!='1' for _,v,_ in comparisons):errors.append('older candidate recapture mismatch')
                        coverage=re.search(r'^\[snapshot-census\] rewind-coverage-summary total=(\d+) pool_state=(\d+) inactive_pool=(\d+) unclassified=(\d+)$',diag,re.M)
                    if not coverage:errors.append('pool coverage accounting absent')
                    else:
                        total,required,inactive,unknown=map(int,coverage.groups())
                        if not raw or total!=int(raw.group(1)) or total!=required+inactive+unknown:
                            errors.append('pool coverage accounting inconsistent')
                        if required:errors.append('restored pool state differs from target oracle')
                    pool_rows=re.findall(r'^\[snapshot-census\] rewind-pool-coverage owner=(\S+) state=(\d+) free_payload=(\d+) stride_padding=(\d+) free_tail=(\d+)$',diag,re.M)
                    if len(pool_rows)!=8 or {row[0] for row in pool_rows}!={'gobj','gobj-proc','fighter','ground','item','item-link','aobj','fobj'}:
                        errors.append('pool coverage owner set incomplete')
                    case['pool_coverage']=dict(summary=list(map(int,coverage.groups())) if coverage else [],owners=pool_rows)
                    case['rewind_checks']=dict(compared=len(comparisons),raw_differing_bytes=int(raw.group(1)) if raw else None,
                            object_activity=dict(zip(['created','destroyed','reused'],map(int,activity.groups()))) if activity else {})
            trace_log=log
            if args.resimulate and label=='candidate':
                trace_log,interval_errors=compare_interval(module,log,out,stage);errors+=interval_errors
            traces[label]=module.read_trace(trace_log,[1,2,3,4],[])
            if not traces[label][0]['fighter']:errors.append('fighter trace missing')
            case['runs'].append({'label':label,'command':command,'exit':code,'errors':errors,
                                 'log':str(log),'census':str(census)})
        expected_baseline_failure=args.expect_baseline_green_import_failure and stage=='green-greens'
        if expected_baseline_failure:
            baseline_text=Path(case['runs'][0]['log']).read_text(encoding='utf-8',errors='replace')
            confirmed=(case['runs'][0]['exit']==3 and
                'Native stage import failed: /GrGr.dat status=6 offset=59024' in baseline_text and
                not traces['baseline'][0]['fighter'])
            case['baseline_import_failure_confirmed']=confirmed
            if confirmed:case['runs'][0]['errors']=[]
            else:case['runs'][0]['errors'].append('expected baseline GrGr importer failure not reproduced')
        differences=[]
        for name,values in traces['baseline'][0].items():
            if expected_baseline_failure:continue
            other=traces['candidate'][0][name]
            if values!=other:
                first=next((i for i,(a,b) in enumerate(zip(values,other)) if a!=b),min(len(values),len(other)))
                differences.append({'family':name,'first_entry':first,
                    'baseline':values[first] if first<len(values) else None,
                    'candidate':other[first] if first<len(other) else None})
        case['trace_differences']=differences
        case['evidence']='candidate gameplay plus known baseline importer failure' if expected_baseline_failure else 'old/new exact traces'
        case['status']='PASS' if not differences and all(not r['errors'] for r in case['runs']) else 'FAIL'
        receipt['cases'].append(case);path.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
        print(json.dumps({'stage':stage,'status':case['status'],'differences':len(differences)}),flush=True)
    receipt['unchanged_inputs']=all(sha(Path(name))==expected for name,expected in inputs.items())
    receipt['status']='PASS' if receipt['unchanged_inputs'] and all(c['status']=='PASS' for c in receipt['cases']) else 'FAIL'
    path.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'status':receipt['status'],'receipt':str(path)}),flush=True)
    raise SystemExit(receipt['status']!='PASS')

if __name__=='__main__':main()
