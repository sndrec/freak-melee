"""Check F2 capture and return in startup and every Workshop workspace."""
from argparse import ArgumentParser
from pathlib import Path
import json
import os
import time

from PIL import Image
from review_native_css_room import Client
from review_native_feedback import wait_visible
from review_native_outfit_matches import wait


def capture(client, output, workspace, mode=None, page=None):
    client.command('ui-key','f2');wait_visible(client,'feedback-image')
    contexts=list((client.data/'content/feedback').glob('*.context.json'))
    assert len(contexts)==1
    original=contexts[0].read_bytes();data=json.loads(original)
    assert data['workspace']==workspace,data
    if mode is not None:assert data['work_mode']==mode,data['work_mode']
    if page is not None:assert data['hub_page']==page,data['hub_page']
    image=contexts[0].with_name(contexts[0].name.replace('.context.json','.png'))
    def ready():
        try:
            with Image.open(image) as picture:
                picture.verify()
            return True
        except (OSError,ValueError):return False
    wait(ready,'captured PNG ready',20)
    output.with_suffix('.context.json').write_bytes(original)
    output.with_suffix('.png').write_bytes(image.read_bytes())
    with Image.open(image) as picture:
        assert picture.size==(data['image_width'],data['image_height'])
    # Repeated activation must retain the same report and original context.
    client.command('ui-key','f2');time.sleep(.15)
    assert contexts[0].read_bytes()==original
    client.command('ui-key','escape');wait_visible(client,'feedback-keep')
    client.command('ui-key','enter')
    wait(lambda:'absent' in client.inspect('feedback-document') or 'visible=0' in client.inspect('feedback-document'),
         'return from feedback',10)
    assert not contexts[0].exists()
    return dict(workspace=workspace,mode=mode,bytes=len(original),fighters=len(data['fighters']),items=len(data['items']),
                scripted_fighters=sum('script' in fighter for fighter in data['fighters']),
                image=str(output.with_suffix('.png')))


def main():
    parser=ArgumentParser(description=__doc__)
    for key in ('exe','iso','profile','output'):parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    os.environ.update(FREAK_MELEE_COMMUNITY_ENDPOINT='http://127.0.0.1:1',FREAK_MELEE_SERVICE_ENDPOINT='http://127.0.0.1:1',
                      SDL_AUDIODRIVER='dummy',FREAK_MELEE_MENU_MOTION='0')
    empty=output/'empty-profile';empty.mkdir()
    receipt=dict(status='RUNNING',captures=[])
    clients=[]
    try:
        first=Client(args.exe.resolve(),args.iso.resolve(),empty,output/'startup','',write_profile=False,window_size=(1100,760))
        clients.append(first);first.element('setup-next')
        receipt['captures'].append(capture(first,output/'startup-welcome','Hub',page=17))
        assert 'enabled=1' in first.inspect('setup-next')
        first.stop();clients.remove(first)
        workshop=Client(args.exe.resolve(),args.iso.resolve(),args.profile.resolve(),output/'workshop','Workshop feedback review',
                        window_size=(1440,900))
        clients.append(workshop);workshop.click('nav-5');workshop.click('edit-fighter')
        wait(lambda:'Draft preview ready.' in workshop.inspect('workshop-editor-status'),'frozen Workshop preview',65)
        for key,mode in [('action','Action Editor'),('script','Scripting'),('animation','Animation'),('model','Model')]:
            workshop.click('workshop-mode-'+key)
            receipt['captures'].append(capture(workshop,output/('workshop-'+key),'Workshop',mode=mode))
            assert 'visible=1' in workshop.inspect('workshop-mode-'+key)
        workshop.stop();clients.remove(workshop)
        fighter=next((args.profile.resolve()/'Freak Melee/content/installed').glob('*.fmi')).stem
        match_args=['--match','--stage','battlefield','--mode','time','--time','2','--items','very-high']
        for port in range(1,5):
            match_args += ['--p'+str(port),'human:custom' if port==1 else 'cpu:custom:9','--p'+str(port)+'-fighter',fighter]
        game=Client(args.exe.resolve(),args.iso.resolve(),args.profile.resolve(),output/'gameplay','Gameplay feedback review',
                    launch_args=match_args,window_size=(1280,800))
        clients.append(game);time.sleep(6)
        for attempt in range(8):
            sample=capture(game,output/f'four-player-{attempt}','Game')
            receipt['captures'].append(sample)
            assert sample['fighters']==4 and sample['scripted_fighters']==4,sample
            assert sample['bytes']<8*1024*1024,sample
            if sample['items']:break
            time.sleep(6)
        else:raise AssertionError('No items present in the four-player capture budget review')
        receipt['status']='PASS'
    except Exception as error:
        receipt.update(status='FAIL',error=repr(error));raise
    finally:
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))
        for client in clients:client.stop()
    print('PASS F2 startup, all Workshop workspaces and four scripted fighters with items; immutable capture and clean return')


if __name__=='__main__':main()
