"""Source parser/event-signature census; explicitly not body or package acceptance."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def callback_fields(header,name):
    text=re.sub(r'/\*.*?\*/|//[^\n]*','',header,flags=re.S)
    match=re.search(r'(?:typedef )?struct '+name+r'\s*\{(.*?)\n}',text,re.S)
    if not match:return []
    fields=[]
    for declaration in match[1].split(';'):
        declaration=declaration.strip()
        direct=re.search(r'\(\*(\w+)\)',declaration)
        alias=re.search(r'\b(?:HSD_GObjEvent|HSD_GObjPredicate|Fighter_ItemEvent|EF_UpdateFn)\s+(\w+)\s*$',declaration)
        if direct or alias:fields.append((direct or alias).group(1))
    return fields


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--source',type=Path,default=ROOT/'examples/mario_full.fms')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    units=[ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in ('source','expression','program','statement','value','events')]
    command=[str(args.compiler),'-std=c11','-O2','-Wall','-Wextra','-Werror','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),str(HERE/'census.c'),*map(str,units),'-o',str(args.output/'census.exe')]
    build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
    (args.output/'build.log').write_text(build.stdout+build.stderr);build.check_returncode()
    run=subprocess.run([str(args.output/'census.exe'),str(args.source)],cwd=ROOT,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
    (args.output/'run.log').write_text(run.stdout+run.stderr);run.check_returncode();report=json.loads(run.stdout)
    program_path=ROOT/'src/melee/ft/native_script_fighter_program.c';program=program_path.read_text()
    hooks=set(re.findall(r'strcmp\(event->name,"([^"]+)"\)',program))
    extra=re.search(r'static const char\* const names\[NativeScriptFighterCallback_Count\]=\{(.*?)\}',program,re.S)
    if extra:hooks.update(re.findall(r'"([^"]+)"',extra[1]))
    for callback in report['callbacks']:
        if callback['owner']:closure='entity_lifecycle'
        elif callback['argument_count'] or callback['result']!='void':closure='fighter_scheduler_abi'
        elif callback['event_kind'] in (1,2):closure='motion_scheduler_class'
        elif callback['event_kind']==0 and callback['event_name'] in hooks:closure='fighter_scheduler_class'
        else:closure='fighter_hook_dispatch'
        callback['current_scheduler_category']=closure
    header_path=ROOT/'src/melee/ft/native_fighter_runtime.h';header=header_path.read_text()
    structures=['NativeFighterDefinition','NativeFighterArticleLogic','NativeFighterProjectile','NativeFighterArticleSignal',
                'NativeFighterCallbackExport','NativeFighterModelEffect','NativeItemDefinition']
    report['native_callback_fields']={name:callback_fields(header,name) for name in structures}
    motion_path=ROOT/'src/melee/ft/types.h';motion=motion_path.read_text()
    report['native_callback_fields']['MotionState']=callback_fields(motion,'MotionState')
    item_path=ROOT/'src/melee/it/kinds/types.h';item=item_path.read_text()
    report['native_callback_fields']['ItemStateTable']=callback_fields(item,'ItemStateTable')
    report['native_callback_fields']['ItemLogicTable']=callback_fields(item,'ItemLogicTable')
    report['summary']={'declarations':dict(Counter(item['kind'] for item in report['declarations'])),
        'callback_contexts':dict(Counter(item['context'] for item in report['callbacks'])),
        'scheduler_categories':dict(Counter(item['current_scheduler_category'] for item in report['callbacks']))}
    paths=[HERE/'census.c',Path(__file__).resolve(),args.source,*units,program_path,header_path,motion_path,item_path]
    report['source_sha256']={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    report['scope']='Actual Source parser/ProgramIndex/event signatures and native declaration inventory only; no body, metadata, package or game acceptance.'
    (args.output/'census.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['summary'],indent=2))


if __name__=='__main__':main()
