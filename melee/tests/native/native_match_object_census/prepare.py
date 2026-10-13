"""Prepare an untimed hidden/muted four-CPU census command; never launches."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[3]


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--content-root',type=Path,required=True)
    p.add_argument('--fighter')
    p.add_argument('--stage',default='battlefield')
    p.add_argument('--samples',type=int,default=2000)
    p.add_argument('--seed',type=lambda text:int(text,0),default=0x12345678)
    args=p.parse_args();out=args.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('fresh A: output required')
    if not 1<=args.samples<=100000 or not 0<=args.seed<=0xFFFFFFFF:p.error('invalid sample count/seed')
    out.mkdir(parents=True)
    pad=struct.pack('<HbbbbBBBBbxI',0,0,0,0,0,0,0,0,0,0,0)
    sample=struct.pack('<Q',0)+pad*4+struct.pack('<II',0,0)
    assert len(sample)==80
    replay=out/'inputs.fmr'
    replay.write_bytes(struct.pack('<8sQII',b'FMREPLAY',0,args.seed,1)+sample*args.samples)
    command=[str(ROOT/'build/native/melee_native.exe'),'--iso',
        str(ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso'),'--match',
        '--stage',args.stage,'--mode','stock','--stocks','99','--time','0','--items','off',
        '--hidden','--mute','--fast','--object-census','--replay-inputs',str(replay),
        '--content-root',str(args.content_root.resolve())]
    for port in range(1,5):
        command += [f'--p{port}','cpu:custom:9' if args.fighter else 'cpu:iceclimbers:9']
        if args.fighter:command += [f'--p{port}-fighter',args.fighter]
    receipt={'status':'PREPARED','timing':'UNTIMED','command':command,
        'environment':{'FREAK_MELEE_OBJECT_CENSUS':str(out/'census.log')},
        'replay_sha256':hashlib.sha256(replay.read_bytes()).hexdigest(),
        'seed':args.seed,'samples':args.samples,
        'launch_contract':'Root launch only: CREATE_NO_WINDOW, stdout/stderr redirected, hidden/muted CLI, bounded timeout',
        'limits':['one chosen scene/stage workload does not establish global capacities',
                  'typed allocator/class payload counts omit dynamic satellites, globals, backing allocator metadata and assets',
                  'no accepted timing or allocation-consumer wiring']}
    (out/'command.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'status':'PREPARED','receipt':str(out/'receipt.json')}))


if __name__=='__main__':main()
