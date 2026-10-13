"""Compare complete verified real-game C/interpreted benchmark evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import statistics

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def read(directory):
    receipt = directory/'receipt.json'
    log = directory/'game.log'
    data = json.loads(receipt.read_text())
    if data['status'] != 'PASS' or data['exit_code'] != 0 or not data['inputs_unchanged']:
        raise ValueError('Incomplete or invalid game batch: '+str(directory))
    windows = data['measured_windows']
    if len(windows) != 64 or data['summary']['verified'] != 64:
        raise ValueError('Missing full 64-window verification')
    if any(w['actors'] != 8 or w['replay_ticks'] != 15 for w in windows):
        raise ValueError('Incorrect actual actor/depth workload')
    text = log.read_text(errors='replace')
    hashes = [(int(f),h) for f,h in re.findall(
        r'\[interpreter-evaluation\] endpoint=(\d+) semantic=([0-9a-f]{16})',text)]
    if len(hashes) != 64 or [f for f,h in hashes] != [w['endpoint'] for w in windows]:
        raise ValueError('Missing semantic endpoint evidence')
    modules, current = {}, None
    for line in text.splitlines():
        match = re.search(r'\[interpreter-evaluation\] module=.*-member([01])\.dll '
            r'entries=(\d+) native_calls=(\d+) interpreted_calls=(\d+) errors=(\d+)',line)
        if match:
            current = int(match[1])
            modules[current] = {'entries':int(match[2]), 'native_calls':int(match[3]),
                'interpreted_calls':int(match[4]), 'errors':int(match[5]), 'functions':{}}
            continue
        match = re.search(r'\[interpreter-evaluation\] function=(\w+) calls=(\d+)',line)
        if match and current is not None: modules[current]['functions'][match[1]] = int(match[2])
    if set(modules) != {0,1} or any(m['errors'] for m in modules.values()):
        raise ValueError('Missing modules or interpreter errors')
    if any(sum(m['functions'].values()) != m['entries'] for m in modules.values()):
        raise ValueError('Inconsistent function coverage census')
    means = {key:statistics.mean(w[key] for w in windows) for key in [
        'replay_simulation_ms','replay_draw_ms','replay_snapshot_begin_ms',
        'replay_session_end_ms','replay_input_ms','forward_simulation_ms',
        'forward_snapshot_begin_ms','forward_session_end_ms','forward_draw_ms']}
    return {'summary':data['summary'], 'means':means, 'modules':modules,
            'hashes':hashes, 'inputs':{str(receipt):sha(receipt),str(log):sha(log)}}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native',type=Path,required=True)
    parser.add_argument('--interpreted',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    native, interpreted = read(args.native.resolve()), read(args.interpreted.resolve())
    if native['hashes'] != interpreted['hashes']:
        raise ValueError('C/interpreted semantic timeline divergence; timings not accepted')
    for member in [0,1]:
        if native['modules'][member]['functions'] != interpreted['modules'][member]['functions']:
            raise ValueError('C/interpreted callback/helper coverage diverges')
    if not interpreted['modules'][1]['interpreted_calls']:
        raise ValueError('No interpreted nested fighter behavior executed')
    report = {'status':'PASS correctness and coverage for this eight-actor workload',
        'threshold_met':interpreted['summary']['deadline_misses']==0,
        'criterion':'15 actual rollback replay ticks plus one advancing simulation/draw tick within16.666667ms',
        'semantic_endpoints_matched':64, 'native':native, 'interpreted':interpreted,
        'observed_replay_simulation_delta_ms':interpreted['means']['replay_simulation_ms']-native['means']['replay_simulation_ms'],
        'limits':['Both variants miss the deadline; this does not approve production migration.',
            'Sequential process batches on a shared PC; differences are observed costs, not isolated statistical language overhead.',
            'Eight-fighter CPU battle covers14/523 authored function bodies; separate scalar/rope fixtures cover additional logic.',
            'All523 authored bodies are lowered with no native fallback, but unexecuted bodies lack real-game acceptance.',
            'Real snapshot/audio/draw costs retained; semantic endpoint/full replay hashes verify each measured window.',
            'No independent GPU fence/timestamp or sustained paced scheduling measurement.',
            'Evaluation prototype consumes typed C lowering; no final direct-FMS frontend or public sandbox claim.']}
    output=args.output.resolve()
    if output.exists(): parser.error('Use a fresh output file')
    output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'status':report['status'],'threshold_met':report['threshold_met'],
        'semantic_endpoints_matched':64,'replay_simulation_delta_ms':report['observed_replay_simulation_delta_ms']}))

if __name__=='__main__':main()
