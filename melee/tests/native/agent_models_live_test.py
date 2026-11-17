"""Verify model authoring and whole-asset history in the actual hidden Workshop."""
import argparse
import json
import os
import re
from pathlib import Path
import subprocess
import time

from agent_live_test import close_window, snapshot
from agent_models_test import model_checks


def view_checks(call,target,project,settled):
    baseline=snapshot(project)
    original=call('session.models.view',target)
    changed=call('session.models.select',dict(target,revision=original['revision'],context='meshes',
        mesh=0,bones=[],hidden_meshes=[0,1],bones_visible=False))
    assert changed['workspace']=='model' and changed['transient'] and not changed['saved']
    view=changed['model_view']
    assert view['visible'] and view['mesh']==0 and view['bones']==[] and view['active_bone']==-1
    assert view['hidden_meshes']==[0,1] and not view['bones_drawn']
    same=call('session.models.select',dict(target,revision=changed['revision'],context='meshes',
        mesh=0,bones=[],hidden_meshes=[0,1],bones_visible=False))
    assert same['revision']==changed['revision']
    assert call('session.models.select',dict(target,revision=original['revision'],hidden_meshes=[]),expected=False)['code']=='revision_conflict'
    for fields in [dict(context='materials',bones=[1,1]),dict(context='materials',bones=[511]),dict(hidden_meshes=[0,0])]:
        call('session.models.select',dict(target,revision=same['revision'],**fields),expected=False)
        assert call('session.models.view',target)['model_view']==view
    clips=call('session.actions.list',dict(target,query='AttackAirLw'))['actions']
    clip=next(row['id'] for row in clips if row['name']=='AttackAirLw')
    clip_ref=dict(bank='action',index=clip)
    original_info=call('session.animations.read',dict(target,view='info',clip=clip_ref))
    selected=call('session.models.select',dict(target,revision=same['revision'],context='skeleton',mode='animation',
        bones=[5,6],active_bone=6,hidden_meshes=[],action_clip=clip,frame=10,bones_visible=False))
    assert selected['model_view']['bones_drawn'] and selected['model_view']['bones']==[5,6]
    def preview(frame,position=None):
        deadline=time.monotonic()+30
        while time.monotonic()<deadline:
            response=call('session.animations.preview',dict(target,bones=[5,6]),expected=None)
            if response.get('error'):
                assert response['error']['code']=='preview_pending',response
            elif response['frame']==frame and response['clip']['index']==clip:
                if position is None or all(abs(a-b)<1e-5 for a,b in zip(response['bones'][0]['position'],position)):
                    return response
            time.sleep(.1)
        raise AssertionError(('Actor did not apply the requested model pose',response))
    preview(10)
    playing=call('session.models.select',dict(target,revision=selected['revision'],playing=True))
    time.sleep(.2)
    running=call('session.models.view',target)
    assert running['revision']==playing['revision'] and running['model_view']['playing']
    paused=call('session.models.select',dict(target,revision=running['revision'],playing=False))
    pause_frame=paused['model_view']['frame'];time.sleep(.1)
    assert call('session.models.view',target)['model_view']['frame']==pause_frame
    bones=call('session.models.read',dict(target,view='bones',limit=200))['bones']
    position=list(bones[5]['position']);position[0]+=2
    posed=call('session.models.pose',dict(target,revision=paused['revision'],changes=[
        dict(bone=5,position=position,rotation=[.2,.1,0],scale=[1.2,1,1]),dict(bone=6,rotation=[0,.25,0])]))
    assert posed['model_view']['mode']=='pose' and not posed['dirty']
    actual=preview(0,position)
    assert abs(actual['bones'][0]['scale'][0]-1.2)<1e-6
    detailed=call('session.models.view',dict(target,include_pose=True))['model_view']
    for changes in [[dict(bone=5,position=[0,0,0]),dict(bone=511,scale=[1,1,1])],
                    [dict(bone=5,position=[0,0,0]),dict(bone=5,rotation=[0,0,0])]]:
        call('session.models.pose',dict(target,revision=posed['revision'],changes=changes),expected=False)
        unchanged=call('session.models.view',dict(target,include_pose=True))
        assert unchanged['revision']==posed['revision'] and unchanged['model_view']['temporary_pose']==detailed['temporary_pose']
    reset=call('session.models.pose',dict(target,revision=posed['revision'],reset=True))
    preview(0,bones[5]['position'])
    # Model playback uses the edited draft tree and its full native frame range.
    lengthened=call('session.animations.edit',dict(target,revision=reset['revision'],summary='Long Model preview check',
        operation='duration',clip=dict(bank='action',index=clip),frames=1200.5))
    long_pose=call('session.models.select',dict(target,revision=lengthened['revision'],mode='animation',action_clip=clip,frame=1100))
    assert long_pose['model_view']['frame']==1100
    preview(1100)
    clamped=call('session.models.select',dict(target,revision=long_pose['revision'],frame=65535))
    assert clamped['model_view']['frame']==1200
    preview(1200)
    edit_mode=call('session.workspace.select',dict(target,revision=clamped['revision'],workspace='animation'))
    restored=call('session.undo',dict(target,revision=edit_mode['revision']))
    returned=call('session.models.select',dict(target,revision=restored['revision'],mode='edit',frame=0,hidden_meshes=[],bones_visible=True))
    settled()
    # Animation history conservatively leaves its unsaved marker set after Undo.
    # Compare the restored clip and disk instead of treating that marker as data.
    restored_info=call('session.animations.read',dict(target,view='info',clip=clip_ref))
    for field in ('clip','frames','track_count','binding_flags'):
        assert restored_info[field]==original_info[field],(field,original_info,restored_info)
    assert snapshot(project)==baseline
    return dict(selection=True,transient_pose=True,actual_pose=True,long_clip_preview=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'tool', 'iso', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--control',type=Path,help='Optional developer driver for actual Model UI checks')
    args = parser.parse_args()
    root = args.output.resolve(); root.mkdir(parents=True, exist_ok=False)
    profile = root / 'userdata'; profile.mkdir()
    windows = Path(os.environ['SystemRoot'])
    env = dict(os.environ, LOCALAPPDATA=str(profile),
        PATH=str(windows / 'System32') + ';' + str(windows), FREAK_MELEE_MENU_MOTION='0')
    control_id='agent-model-ui-'+str(os.getpid())
    env['FREAK_MELEE_CONTROL_ID']=control_id
    for service in ('CHAT', 'SERVICE', 'COMMUNITY'):
        env['FREAK_MELEE_' + service + '_ENDPOINT'] = 'http://127.0.0.1:1'
    def call(method, arguments=None, expected=True):
        result = subprocess.run([str(args.tool.resolve()), 'agent', 'call', method, '--stdin'],
            input=json.dumps(arguments or {}), env=env, cwd=root, capture_output=True,
            encoding='utf-8', timeout=90, creationflags=subprocess.CREATE_NO_WINDOW)
        assert result.stdout, (method, result.returncode, result.stderr)
        reply = json.loads(result.stdout)
        if expected is None: return reply['result'] if reply['ok'] else {'error':reply['error']}
        assert reply['ok'] == expected, (method, arguments, reply)
        return reply['result'] if expected else reply['error']
    def ui_checks(target):
        def control(*parts):
            result=subprocess.run([str(args.control.resolve()),'--id',control_id,*map(str,parts)],
                env=env,cwd=root,capture_output=True,text=True,timeout=10,creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.returncode==0,result.stderr
        def click(element):
            log=root/'stderr.log'
            for _ in range(30):
                start=log.stat().st_size;control('ui-inspect',element);time.sleep(.1)
                with log.open(encoding='utf-8',errors='replace') as stream:
                    stream.seek(start);text=stream.read(2*1024*1024)
                found=re.search(r'\[ui-inspect\] '+re.escape(element)+r':?[^\n]*origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)',text)
                if found:
                    x,y,width,height=map(float,found.groups())
                    if width>0 and height>0 and 'visible=0' not in found[0]:
                        control('ui-click',round(x+width/2),round(y+height/2));time.sleep(.1);return
            raise AssertionError(('UI control unavailable',element))
        def view():return call('session.models.view',target)
        state=view()
        call('session.models.select',dict(target,revision=state['revision'],context='meshes',hidden_meshes=[]))
        click('workshop-model-eye-0');assert view()['model_view']['hidden_meshes']==[0]
        click('workshop-model-eye-0');assert view()['model_view']['hidden_meshes']==[]
        click('workshop-model-context-skeleton')
        click('workshop-model-joint-5');assert view()['model_view']['bones']==[5]
        click('workshop-model-rig-pose')
        click('workshop-model-pos-0');control('ui-key','a','ctrl');control('ui-text','3.125');control('ui-key','enter')
        time.sleep(.1)
        posed=call('session.models.view',dict(target,include_pose=True))
        bone=next(row for row in posed['model_view']['temporary_pose'] if row['bone']==5)
        assert bone['position'][0]==3.125,bone
        actual=call('session.animations.preview',dict(target,bones=[5]))
        assert actual['bones'][0]['position'][0]==3.125,actual
        state=view();call('session.models.pose',dict(target,revision=state['revision'],reset=True))
        click('workshop-model-rig-animation');click('workshop-model-play')
        assert view()['model_view']['playing']
        click('workshop-model-play');assert not view()['model_view']['playing']
        state=view();call('session.models.select',dict(target,revision=state['revision'],mode='edit',frame=0))
        return True
    stock = next(row for row in call('stock.list', {'query': 'Mario'})['fighters'] if row['name'] == 'Mario')
    project = profile / 'Freak Melee/projects/Model Mario'
    call('project.clone_stock', dict(fighter_id=stock['fighter_id'], iso=str(args.iso.resolve()),
        output=str(project), title='Model Mario'))
    content = profile / 'Freak Melee/content'; content.mkdir(parents=True, exist_ok=True)
    (content / 'startup.txt').write_text('12 0 1\n\n')
    (content / 'hub-profile.txt').write_text('Agent model review\n\n')
    baseline = snapshot(project)
    process = None
    try:
        with (root / 'stdout.log').open('w') as out, (root / 'stderr.log').open('w') as err:
            process = subprocess.Popen([str(args.exe.resolve()), '--iso', str(args.iso.resolve()), '--hidden', '--mute'],
                cwd=root, env=env, stdout=out, stderr=err, creationflags=subprocess.CREATE_NO_WINDOW)
        (root / 'pid.txt').write_text(str(process.pid))
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            assert process.poll() is None, ('Game exited', process.returncode, root)
            matches = [row for row in call('instances.list')['instances'] if row['process_id'] == process.pid and row['available']]
            if matches: break
            time.sleep(.2)
        else: raise AssertionError('No live endpoint')
        target = {'instance': matches[0]['instance']}
        state = call('session.inspect', target)
        call('session.open', dict(target, project=str(project), revision=state['revision']))
        def settled():
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline:
                assert process.poll() is None, ('Game exited', process.returncode, root)
                state = call('session.inspect', target)
                if (not state['preview_loading'] and state['editing_allowed'] and state['preview_scene'] == 'practice'
                    and state['preview_ready'] and state['requested_preview_package_id']
                    and state['loaded_preview_package_id'] == state['requested_preview_package_id']):
                    return state
                time.sleep(.15)
            raise AssertionError(('Preview did not settle', state))
        settled()
        views=view_checks(call,target,project,settled)
        views['actual_ui']=ui_checks(target) if args.control else False
        result = model_checks(call, target, project, live=True, settled=settled)
        state = settled()
        assert state['dirty'] and snapshot(project) == baseline
        saved = call('session.save', dict(target, revision=state['revision']))
        assert saved['saved'] and not saved['dirty'] and snapshot(project) != baseline
        loaded = call('models.read', dict(project=str(project), view='info'))
        assert loaded['meshes'] == result['meshes']
        built = call('session.build', dict(target, revision=saved['revision']))
        assert built['package_id']
        # History remains valid after Save/build; changing the source identity
        # must not cause labels, embedded images or size-changing data to be lost.
        call('session.undo', dict(target, revision=built['revision']))
        state = settled()
        assert state['dirty']
        call('session.redo', dict(target, revision=state['revision']))
        state = settled()
        assert not state['dirty']
        (root / 'result.json').write_text(json.dumps(dict(status='PASS', **views, **result), indent=2))
        print('PASS: real model edits, full-asset Undo/Redo, private live ownership, preview reload and saved build')
    finally:
        if process and process.poll() is None:
            close_window(process.pid)
            process.wait(timeout=30)


if __name__ == '__main__': main()
