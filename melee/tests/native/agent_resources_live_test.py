"""Run resource edits/history against an actual isolated hidden Workshop."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time
from agent_live_test import close_window
from agent_resources_test import resource_checks,snapshot


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','tool','iso','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--control',type=Path)
    args=parser.parse_args();root=args.output.resolve();root.mkdir(parents=True,exist_ok=False)
    profile=root/'userdata';profile.mkdir();windows=Path(os.environ['SystemRoot'])
    control_id='agent-resources-'+str(os.getpid())
    env=dict(os.environ,LOCALAPPDATA=str(profile),PATH=str(windows/'System32')+';'+str(windows),
        FREAK_MELEE_MENU_MOTION='0',FREAK_MELEE_CONTROL_ID=control_id)
    for service in ('CHAT','SERVICE','COMMUNITY'):env['FREAK_MELEE_'+service+'_ENDPOINT']='http://127.0.0.1:1'
    def call(name,arguments=None,expected=True):
        result=subprocess.run([str(args.tool.resolve()),'agent','call',name,'--stdin'],input=json.dumps(arguments or {}),
            env=env,cwd=root,capture_output=True,encoding='utf-8',timeout=90,creationflags=subprocess.CREATE_NO_WINDOW)
        assert result.stdout,(name,result.returncode,result.stderr)
        reply=json.loads(result.stdout);assert reply['ok']==expected,(name,arguments,reply)
        return reply['result'] if expected else reply['error']
    mario=next(row for row in call('stock.list',dict(query='Mario'))['fighters'] if row['name']=='Mario')
    project=profile/'Freak Melee/projects/Resource Mario'
    call('project.clone_stock',dict(iso=str(args.iso.resolve()),fighter_id=mario['fighter_id'],title='Resource Mario',output=str(project)))
    content=profile/'Freak Melee/content';content.mkdir(parents=True,exist_ok=True)
    (content/'startup.txt').write_text('12 0 1\n\n');(content/'hub-profile.txt').write_text('Resource review\n\n')
    process=None
    try:
        with (root/'stdout.log').open('w') as out,(root/'stderr.log').open('w') as err:
            process=subprocess.Popen([str(args.exe.resolve()),'--iso',str(args.iso.resolve()),'--hidden','--mute'],
                env=env,cwd=root,stdout=out,stderr=err,creationflags=subprocess.CREATE_NO_WINDOW)
        (root/'pid.txt').write_text(str(process.pid));deadline=time.monotonic()+120
        while time.monotonic()<deadline:
            assert process.poll() is None,('Game exited',process.returncode)
            matches=[row for row in call('instances.list')['instances'] if row['process_id']==process.pid and row['available']]
            if matches:break
            time.sleep(.2)
        else:raise AssertionError('No resource test endpoint')
        target=dict(instance=matches[0]['instance']);state=call('session.inspect',target)
        call('session.open',dict(target,revision=state['revision'],project=str(project)))
        def settle():
            deadline=time.monotonic()+120
            while time.monotonic()<deadline:
                state=call('session.inspect',target)
                if state['editing_allowed'] and state['preview_ready'] and not state['preview_loading'] and state['requested_preview_package_id']==state['loaded_preview_package_id']:return state
                assert process.poll() is None,('Game exited',process.returncode)
                time.sleep(.15)
            raise AssertionError(('Preview did not settle',state))
        settle();checks=resource_checks(call,target,project,root/'inputs',live=True)
        if args.control:
            state=call('session.inspect',target)
            disk=snapshot(project)
            previous=call('session.resources.read',dict(target,query='ui/stock-icon.png'))['resources']
            result=subprocess.run([str(args.control.resolve()),'--id',control_id,'workshop-image','stock',str(root/'inputs/token.png')],
                env=env,cwd=root,capture_output=True,text=True,timeout=10,creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.returncode==0,result.stderr
            deadline=time.monotonic()+10
            while time.monotonic()<deadline:
                rows=call('session.resources.read',dict(target,query='ui/stock-icon.png'))['resources']
                if rows and rows[0]['width']==8:break
                time.sleep(.1)
            else:raise AssertionError('Ordinary HUD image import did not reach resource operation')
            now=call('session.inspect',target);assert now['dirty']
            refreshed=call('session.preview.refresh',dict(target,revision=now['revision']))
            loaded=settle();assert loaded['loaded_preview_package_id']==refreshed['package_id']
            assert loaded['preview_matches_fighter'] and snapshot(project)==disk
            checks['actual_resource_preview']=True
            call('session.undo',dict(target,revision=loaded['revision']))
            assert call('session.resources.read',dict(target,query='ui/stock-icon.png'))['resources']==previous
            checks['hud_import_history']=True
        state=call('session.inspect',target);saved=call('session.save',dict(target,revision=state['revision']))
        built=call('session.build',dict(target,revision=saved['revision']));assert built['package_id']
        (root/'result.json').write_text(json.dumps(dict(status='PASS',**checks),indent=2))
        print('PASS: real resource edits, atomic failure, active-workspace history, HUD import, Save and build')
    finally:
        if process and process.poll() is None:close_window(process.pid);process.wait(timeout=30)


if __name__=='__main__':main()
