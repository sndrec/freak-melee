"""Pin copied stock callback slots and actual Ness motion handlers to a snapshot."""
import hashlib
import json
from pathlib import Path
import re


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def initializer(text, name):
    match = re.search(r'\b' + re.escape(name) + r'\s*(?:\[[^\]]*\])?\s*=\s*\{', text)
    if not match:
        if re.search(r'\b' + re.escape(name) + r'\s*\[[^\]]*\]\s*;', text):
            return ''  # Actual static-storage table has implicit zero initialization.
        raise RuntimeError('Missing callback table ' + name)
    begin = match.end()
    cursor, depth = begin, 1
    while depth:
        depth += (text[cursor] == '{') - (text[cursor] == '}')
        cursor += 1
    return text[begin:cursor-1]


def entries(text):
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    result, start, depth = [], 0, 0
    for i, c in enumerate(text):
        depth += (c in '{([') - (c in '})]')
        if c == ',' and depth == 0:
            result.append(text[start:i].strip())
            start = i + 1
    if text[start:].strip():
        result.append(text[start:].strip())
    return result


def census(snapshot, output):
    sdk = snapshot / 'sdk'
    source = sdk / 'src/melee/ft/ftdata.c'
    text = source.read_text()
    copy_body = text[text.index('void ftData_AssignStockCallbacks('):]
    tables = list(dict.fromkeys(re.findall(r'COPY\((ft\w+)\)', copy_body)))
    tables.append('ftData_UnkCallbackPairs0')
    slots = {}
    for table in tables:
        values = entries(initializer(text, table))
        slots[table] = values[8] if len(values) > 8 else '0 (implicit initializer)'
    model_arrays = entries(initializer(text, 'ftData_UnkIntBoolFunc0'))
    models = [entries(array.strip()[1:-1])[8] for array in model_arrays]
    if models != ['NULL', 'NULL']:
        raise RuntimeError('Ness acquired an extra model callback: '+str(models))
    source_ness = sdk / 'src/melee/ft/kinds/ftNess/ftness.c'
    text_ness = source_ness.read_text()
    motions = []
    for match in re.finditer(r'//\s*(ftNs_MS_\w+)\s*=\s*(\d+)\s*\n(.*?)\n    \}',
                             text_ness, re.S):
        fields = entries(match[3])
        if len(fields) != 8:
            raise RuntimeError('Unexpected MotionState shape ' + match[1])
        motions.append({'motion': int(match[2]), 'name': match[1],
                        'animation': fields[3], 'interrupt': fields[4],
                        'physics': fields[5], 'collision': fields[6],
                        'camera': fields[7]})
    if len(motions) != 36 or sum(m[k] != 'NULL' for m in motions for k in
                              ['animation', 'interrupt', 'physics', 'collision']) != 142:
        raise RuntimeError('Ness stock motion census changed')
    paths = ['src/melee/ft/ftdata.c', 'src/melee/ft/kinds/ftNess/ftness.c',
             'src/melee/ft/kinds/ftNess/ftness_clone.c',
             'src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c',
             'src/melee/ft/ft_0BEC.c', 'src/melee/ft/ft_0BF0.c',
             'src/melee/ft/ftcpuattack.c',
             'src/melee/ft/kinds/ftCommon/ftCo_0A01.c']
    result = {'snapshot': str(snapshot), 'original_inputs': {p: sha(sdk/p) for p in paths},
              'copied_stock_tables': slots, 'motion_states': motions,
              'nonnull_gameplay_phases': 142,
              'intentionally_null_phases': ['356 interrupt', '357 interrupt'],
              'authored_empty_over_original_null': ['356 interrupt', '357 interrupt'],
              'extra_model_events': {'ftData_UnkIntBoolFunc0': models},
              'metadata': 'demo roots, names and immutable item asset registration stay owned metadata'}
    # Hash the complete original Ness bodies and the shared callers whose kind
    # decisions are covered by this assembly. Kirby copy remains outside scope.
    complete=list((sdk/'src/melee/ft/kinds/ftNess').glob('ftness*.c'))
    complete+=list((sdk/'src/melee/it/kinds').glob('itness*.c'))
    for path in complete:
        result['original_inputs'][path.relative_to(sdk).as_posix()]=sha(path)
    result['common_decision_contracts']={
        'extended_smashes':'authored forward/up/down_smash_enter and all seven phases',
        'aerial_jump':'authored aerial_jump/JumpPhysics, generic Begin/DriftFrom/adjusted Y',
        'worn_item_routes':'authored worn_item_jump_sound=false; shared ItemScrew/multi-jump routes',
        'incapacitated_presentation':'authored presentation_item; common KO/ice/sleep initialization',
        'cpu':'authored attack/travel/recovery/special_candidate/projectile_hit',
        'article_registration':'11 immutable owned asset slots, 11 generated article logics',
        'copy_fighter':'Kirby copy decisions excluded',
    }
    (output/'census.json').write_text(json.dumps(result, indent=2) + '\n')
    return result
