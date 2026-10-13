"""Compare an exact recorded interval with its separately bracketed resimulation."""
def compare_interval(module,log,out,prefix,depth=8,rounds=1,target=240):
    text=log.read_text(errors='replace');errors=[]
    marks=[f'[snapshot-census] resimulation-{name}\n' for name in
           ['reference-start','reference-end','trace-start','trace-end']]
    if any(text.count(mark)!=rounds for mark in marks):
        return log,['original/resimulation interval boundaries absent']
    cursor=0;pieces=[]
    for interval in range(rounds):
        positions=[text.index(mark,cursor) for mark in marks]
        if positions!=sorted(positions):return log,['resimulation interval boundaries out of order']
        expected=out/f'{prefix}-expected-interval-{interval}.log';repeated=out/f'{prefix}-resimulation-trace-{interval}.log'
        expected.write_text(text[positions[0]+len(marks[0]):positions[1]])
        repeated.write_text(text[positions[2]+len(marks[2]):positions[3]])
        original,_,_=module.read_trace(expected,{1,2,3,4},set())
        replayed,_,_=module.read_trace(repeated,{1,2,3,4},set())
        if original!=replayed:errors.append(f'intermediate resimulation traces differ at interval {interval}')
        start=target+120*interval
        if [row[0] for row in original['rng']]!=list(range(start,start+depth)):
            errors.append(f'interval {interval} lacks {depth} distinct input/RNG samples')
        pieces.append(text[cursor:positions[2]])
        cursor=positions[3]+len(marks[3])
    continued=out/f'{prefix}-continued-trace.log'
    continued.write_text(''.join(pieces)+text[cursor:])
    return continued,errors


def compare_correction(module, log, reference, out, prefix, depth=8,target=240):
    """Keep predicted continuation separate from two corrected replay passes."""
    text = log.read_text(errors='replace')
    names = ['resimulation-reference-start', 'resimulation-reference-end',
             'correction-trace-first-start', 'correction-trace-first-end',
             'correction-trace-second-start', 'correction-trace-second-end']
    marks = [f'[snapshot-census] {name}\n' for name in names]
    if any(text.count(mark) != 1 for mark in marks):
        return log, ['correction interval boundaries absent']
    positions = [text.index(mark) for mark in marks]
    if positions != sorted(positions):
        return log, ['correction interval boundaries out of order']
    traces = []
    for index, label in enumerate(['predicted', 'corrected-first', 'corrected-second']):
        begin, end = 2 * index, 2 * index + 1
        path = out / f'{prefix}-{label}-interval.log'
        path.write_text(text[positions[begin] + len(marks[begin]):positions[end]])
        traces.append(module.read_trace(path, {1, 2, 3, 4}, set())[0])
    fresh = module.read_trace(reference, {1, 2, 3, 4}, set())[0]
    expected = {kind: [row for row in rows if target <= row[0] < target+depth]
                for kind, rows in fresh.items()}
    errors = []
    for index, trace in enumerate(traces[1:], 1):
        if trace != expected:
            mismatched = [kind for kind in expected if trace[kind] != expected[kind]]
            errors.append(f'correction pass {index} differs from fresh corrected trace: {mismatched}')
        if [row[0] for row in trace['rng']] != list(range(target, target+depth)):
            errors.append(f'correction pass {index} lacks {depth} distinct samples')
    if traces[0]['fighter'] == traces[1]['fighter']:
        errors.append('corrected inputs did not change fighter state')
    continued = out / f'{prefix}-continued-trace.log'
    continued.write_text(text[:positions[2]] + text[positions[5] + len(marks[5]):])
    return continued, errors
