"""Stock comparisons for Bowser's owned breath/Flame in hidden drawn matches."""
import argparse, json, struct, subprocess, sys
from pathlib import Path
from fighter_script_bundle_roundtrip import run, portrait, bundled_id
ROOT = Path(__file__).resolve().parents[2]

def recording(path, mode, facing, duration, contact=False, repeat=False, defense=False):
    base = (ROOT/'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    frames = bytearray(); start = 230 if contact else 170 if mode == 'air' else 150
    count = 1200 if repeat else 850 if duration > 200 else 550
    for tick in range(count):
        sample = bytearray(base[24:104]); buttons,x,y=0,0,0
        if facing == 'left' and 100 <= tick < 102: x=-40
        if contact and 85 <= tick < (130 if defense else 151): x=100
        if mode == 'air' and 145 <= tick < 153: buttons=1<<10
        if start <= tick < start+duration: buttons=1<<9
        if repeat and 950 <= tick < 1010: buttons=1<<9
        struct.pack_into('<Hbb', sample, 8, buttons,x,y)
        if defense and 215 <= tick < 510: struct.pack_into('<Hbb', sample, 24, 1 << 9, 0, -80)
        frames.extend(sample)
    path.write_bytes(base[:24]+frames)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso',required=True,type=Path)
    parser.add_argument('--out-dir',required=True,type=Path)
    parser.add_argument('--case',action='append')
    args=parser.parse_args();batch=args.out_dir.resolve();batch.mkdir(parents=True,exist_ok=False)
    project,store,source,image=batch/'project',batch/'store',batch/'owned.fms',batch/'portrait.png'
    tool=str(ROOT/'build/native/native_content_tool.exe');portrait(image)
    run(tool,'clone-stock',args.iso,'Bowser',project,'Bowser breath comparison')
    run(tool,'import-parameters',project,ROOT/'examples/koopa_wip.fms',source)
    run(tool,'script-build',project,source)
    package=bundled_id(run(tool,'--root',store,'bundle-project',project,image,args.iso))
    run(tool,'--root',store,'accept',package,args.iso);(batch/'package.txt').write_text(package)
    cases=[(f'{mode}-{facing}-{label}',mode,facing,duration,False,False,'final-destination')
           for mode in ('ground','air') for facing in ('right','left')
           for label,duration in (('tap',1),('sustain',400))]
    cases += [('ground-recharge','ground','right',400,False,True,'final-destination'),
              ('ground-contact','ground','right',200,True,False,'final-destination'),
              ('ground-slope','ground','right',100,False,False,'yoshis-story'),
              ('ground-reflect','ground','right',200,True,False,'final-destination'),
              ('ground-absorb','ground','right',200,True,False,'final-destination')]
    results=[]
    for name,mode,facing,duration,contact,repeat,stage in cases:
        if args.case and name not in args.case: continue
        print('Comparing Bowser breath: '+name,flush=True)
        inputs=batch/(name+'.fmr');defense=name in ('ground-reflect','ground-absorb')
        recording(inputs,mode,facing,duration,contact,repeat,defense)
        command=[sys.executable,str(ROOT/'tools/run_fighter_parity.py'),'--iso',str(args.iso),
                 '--inputs',str(inputs),'--stock-fighter','bowser','--package-id',package,
                 '--content-root',str(store),'--stocks','2','--out-dir',str(batch/name),
                 '--expect-motion','341' if mode=='ground' else '344',
                 '--expect-item-kind','100','--stage',stage]
        if contact and not defense:command += ['--expect-damage-port','2']
        if name == 'ground-reflect':command += ['--p2','human:fox','--expect-item-owner-transfer','100','1','2']
        if name == 'ground-absorb':command += ['--p2','human:ness','--expect-item-event','absorb','100']
        result=subprocess.run(command,cwd=ROOT)
        results.append(dict(case=name,package=package,exit=result.returncode))
        (batch/'results.json').write_text(json.dumps(results,indent=2)+'\n')
    return int(not results or any(row['exit'] for row in results))

if __name__=='__main__':raise SystemExit(main())
