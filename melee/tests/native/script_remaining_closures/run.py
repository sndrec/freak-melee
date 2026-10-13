"""Dependency census using the shared lexer and actual compiled descriptor table."""
import argparse
from collections import Counter,defaultdict
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('closure_binding_audit',ROOT/'tools/audit_native_script_bindings.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
audit.subprocess=SimpleNamespace(run=lambda *args,**kwargs:subprocess.run(*args,**kwargs,creationflags=subprocess.CREATE_NO_WINDOW))


def build(compiler,out,stem,units,extra=()):
    command=[str(compiler),'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1',
        '-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/melee/ft'),
        '-I'+str(ROOT/'extern/dolphin/include'),*extra,*map(str,units),'-o',str(out/(stem+'.exe'))]
    result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
    (out/(stem+'-build.log')).write_text(result.stdout+result.stderr);result.check_returncode()
    return out/(stem+'.exe')


def suite(name):
    if name.startswith(('item.resource','item.link','item.unlink','item.signal','articles.signal')):return 'resource_link_signal'
    if name.startswith(('owner.','creator.','holder.','recipient.','partner.','other.','victim.','targets.','capture.','cpu.target','family.')):return 'retained_receiver_target_capture'
    if name.startswith(('articles.','items.','item.create','item.spawn_child','item.attach','item.attachment','item.activate','item.change_state','item.spawn_food','item.interact','item.check_owner','item.overlaps_owner')):return 'article_spawn_attach_holdable_lifecycle'
    if name.startswith(('effect.','effects.','item.joint','item.bone','item.dynamic_joint','item.spawn_effect','item.spawn_attached_effect','bones.','model.','model_','graphs.','pose.','visuals.','costume.','attachment.','color.')):return 'presentation_graph_effect'
    if name.startswith(('callbacks.','cue.','event.','signal.','hitlag.')):return 'event_callback_identity'
    if name.startswith(('cpu.','player.','stage.','world.','targets.')):return 'planner_world_player_queries'
    if name.startswith(('collision.','floor.','surface.','terrain.','wall.','ceiling.')):return 'collision_stage_geometry'
    if name.startswith(('guard.','shield.','reflector.','absorber.','hitbox.','hitboxes.','hurtboxes.','damage.','combat.')):return 'combat_guard_capture_policy'
    if name.startswith(('item.','position.','velocity.','facing')):return 'item_self_remaining'
    if name.startswith(('state.','physics.','controls.','motion.','animation.','movement.','walk.','jumps.','input.','fighter.','air.','ground.','action.','stats.','facing.')):return 'fighter_entry_control_policy'
    if name.startswith(('sound.','audio.')):return 'audio_policy'
    return 'math_or_unclassified'


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--compiler',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    observed=[ROOT/'src/melee/ft/native_script_bindings.c',ROOT/'src/melee/ft/native_script_bindings.h',ROOT/'src/melee/ft/native_script_ir.h']
    observed += [ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in ('events','source','program','statement','expression','value')]
    before={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in observed}
    lexer=build(args.compiler,args.output,'lexer',[ROOT/'tests/native/script_binding_inventory/lexer_census.c',ROOT/'src/melee/ft/native_script_expression.c'])
    # Compile the actual immutable table/macros and Descriptor body, avoiding
    # unrelated runtime leaves rather than supplying fake successful bindings.
    binding_source=(ROOT/'src/melee/ft/native_script_bindings.c').read_text()
    first=binding_source.index('#define F ');last=binding_source.index('\nstatic bool error(',first)
    catalog_source=args.output/'catalog_actual.c'
    catalog_source.write_text('#include <melee/ft/native_script_bindings.h>\n#include <melee/ft/native_script_program.h>\n#include <melee/ft/native_script_binding_item_resources.h>\n#include <melee/ft/native_script_binding_spawn.h>\n'+binding_source[first:last])
    frontend=[ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in ('events','source','program','statement','expression','value')]
    executable=build(args.compiler,args.output,'catalog',[HERE/'catalog.c',catalog_source,ROOT/'src/melee/ft/native_script_binding_item_resources_catalog.c', ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c',*frontend])
    run=subprocess.run([str(executable)],cwd=ROOT,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW);run.check_returncode()
    dumped=json.loads(run.stdout);catalog=dumped['catalog'];payload_names={entry['property'] for entry in dumped['event_payloads']};by_path=defaultdict(list)
    for descriptor in catalog:by_path[descriptor['path']].append(descriptor)
    paths=sorted((ROOT/'examples').rglob('*.fms'))
    manifest=audit.audit(lexer,paths)
    groups=defaultdict(lambda:{'uses':0,'operations':Counter(),'mario':Counter(),'rook':Counter(),'first_examples':{}})
    statuses=Counter();all_occurrences=[];cross_owned=[]
    # Overload names select these actual alternative catalog descriptors in Resolve.
    overloads={'state.enter':{2:'state.enter_frame',3:'state.enter_speed'},
        'bones.root.rotate_x':{2:'bones.root.rotate_x_joint'},
        'bones.root.rotate_y':{2:'bones.root.rotate_y_joint'},
        'bones.root.rotate_z':{2:'bones.root.rotate_z_joint'},
        'bones.world_position':{7:'bones.world_position_offset'}}
    for file in manifest['files']:
        for use in file['occurrences']:
            if use['classification']!='native' and use['operation'].startswith(('owner.','creator.','holder.','recipient.')):
                cross_owned.append(use)
            if use['classification']!='native':continue
            name=use['operation'];canonical=name.replace('()','')
            descriptors=by_path.get(canonical,[])
            calls=[step for step in use['chain'] if step['kind']=='call']
            mode=use['mode'];context={'fighter':1,'article':2,'effect':4}.get(use['context'],0)
            if calls and canonical in overloads:
                alternative=overloads[canonical].get(len(calls[-1]['arguments']))
                if alternative:descriptors=by_path.get(alternative,[])
            compatible=[entry for entry in descriptors if entry['contexts']&context]
            if name in payload_names:status='event_payload_presence_only'
            elif use['context']=='recipient':status='receiver_context_missing'
            elif any(step['kind']=='index' or (step['kind']=='call' and i+1<len(use['chain'])) for i,step in enumerate(use['chain'])):status='selector_chain_requires_semantic_contract'
            elif not descriptors:status='catalog_path_missing'
            elif not compatible:status='catalog_context_missing'
            elif mode=='call' and not any(entry['kind']==2 and entry['arguments']==len(calls[-1]['arguments']) for entry in compatible):status='catalog_call_shape_mismatch'
            elif mode=='write' and not any(entry['flags']&2 for entry in compatible):status='catalog_write_missing'
            else:status='catalog_presence_only'
            statuses[status]+=1;use=dict(use,current_catalog_status=status,closure_suite=suite(name));all_occurrences.append(use)
            if status not in ('catalog_presence_only','event_payload_presence_only'):
                group=groups[use['closure_suite']];group['uses']+=1;group['operations'][name]+=1
                if use['path']=='examples/mario_full.fms':group['mario'][name]+=1
                if 'rook_workshop/' in use['path']:group['rook'][name]+=1
                group['first_examples'].setdefault(name,{'path':use['path'],'line':use['line'],'chain':use['chain']})
    report={'scope':'All authored examples FMS; balanced shared-lexer chains vs actual catalog names/contexts/access/arity. No type/body/package/game acceptance.',
        'summary':manifest['summary'],'catalog_descriptor_count':len(catalog),'current_catalog_statuses':dict(statuses),
        'groups':{name:dict(group,operations=dict(group['operations']),mario=dict(group['mario']),rook=dict(group['rook'])) for name,group in groups.items()},
        'occurrences':all_occurrences,'catalog':catalog,'event_payloads':dumped['event_payloads'],'cross_owned_accesses':cross_owned,
        'unparsed_statements':{file['path']:file['unparsed_statements'] for file in manifest['files'] if file['unparsed_statements']}}
    units=[HERE/'catalog.c',Path(__file__).resolve(),ROOT/'tools/audit_native_script_bindings.py',ROOT/'src/melee/ft/native_script_bindings.c',
        ROOT/'src/melee/ft/native_script_bindings.h',ROOT/'src/melee/ft/native_script_ir.h',*frontend,ROOT/'src/melee/ft/native_script_binding_item_resources_catalog.c', ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
    report['source_sha256']={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in [*units,*paths]}
    for path,digest in before.items():
        if report['source_sha256'][path]!=digest:raise RuntimeError('Source changed during census: '+path)
    (args.output/'closures.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'files':len(paths),'native_uses':sum(statuses.values()),'catalog_statuses':dict(statuses),
        'groups':{name:{'uses':group['uses'],'distinct':len(group['operations']),'mario':len(group['mario']),'rook':len(group['rook'])} for name,group in groups.items()},
        'unparsed_statements':manifest['summary']['unparsed_statements']},indent=2))


if __name__=='__main__':main()
