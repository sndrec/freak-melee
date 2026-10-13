"""Frozen source/full generated follower policy console comparison; never starts Melee."""
import argparse, hashlib, json, re, struct, subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
CPU = 'src/melee/ft/kinds/ftCommon/ftCo_0A01.c'
PREFIX = r'^(?:static\s+)?(?:inline\s+)?(?:bool|void|float|f32|s32|u8|int|int32_t|uint32_t|Fighter\*)\s+'
SYMBOLS = ['ftCo_800A3200', 'isInTeeter', 'isInCaptureWait', 'inlineK0',
           'inlineJ0', 'isPopoUpB', 'ftCo_800B0CA8', 'ftCo_800B0E98',
           'ftCo_800B101C_inline0', 'ftCo_800A7AAC', 'inlineI3',
           'ftCo_800B0760', 'ftCo_CpuUpdateNanaFollow', 'ftCo_800B101C',
    'ftCo_800B0AF4']
ENUMS = {
    'common_wait': 'ftCo_MS_Wait', 'common_motion_count': 'ftCo_MS_Count',
    'common_cliff_catch': 'ftCo_MS_CliffCatch', 'common_cliff_wait': 'ftCo_MS_CliffWait',
    'common_capture_wait_hi': 'ftCo_MS_CaptureWaitHi', 'common_capture_wait_lw': 'ftCo_MS_CaptureWaitLw',
    'common_shouldered_wait': 'ftCo_MS_ShoulderedWait', 'common_shouldered_turn': 'ftCo_MS_ShoulderedTurn',
    'common_teeter': 'ftCo_MS_Ottotto', 'common_teeter_wait': 'ftCo_MS_OttottoWait',
    'common_capture_kirby': 'ftCo_MS_CaptureKirby', 'common_capture_wait_kirby': 'ftCo_MS_CaptureWaitKirby',
    'common_rebirth': 'ftCo_MS_Rebirth', 'common_rebirth_wait': 'ftCo_MS_RebirthWait',
    'common_warp_jump': 'ftCo_MS_WarpStarJump', 'common_warp_fall': 'ftCo_MS_WarpStarFall',
    'common_parasol_fall': 'ftCo_MS_ItemParasolFall', 'common_parasol_damage_fall': 'ftCo_MS_ItemParasolDamageFall',
    'common_squat': 'ftCo_MS_Squat', 'common_squat_wait': 'ftCo_MS_SquatWait',
    'common_landing': 'ftCo_MS_Landing', 'common_landing_special': 'ftCo_MS_LandingFallSpecial',
    'common_landing_air_n': 'ftCo_MS_LandingAirN', 'common_landing_air_lw': 'ftCo_MS_LandingAirLw',
    'common_flower_ground': 'ftCo_MS_FireFlowerShoot', 'common_flower_air': 'ftCo_MS_FireFlowerShootAir',
    'follower_blizzard_ground': 'ftPp_MS_SpecialLw', 'follower_blizzard_air': 'ftPp_MS_SpecialAirLw',
    'follower_belay_first': 'ftPp_MS_SpecialHi_0', 'follower_belay_last': 'ftPp_MS_SpecialHi_5',
}

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def body(text, name):
    m = re.search(PREFIX + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text, re.M)
    if not m: raise ValueError('Missing actual definition: ' + name)
    depth, end = 1, m.end()
    while depth: depth += (text[end] == '{') - (text[end] == '}'); end += 1
    return text[m.start():end]

def enum_values(sdk):
    values = {}
    for name, kind in [('ftCommon', 'ftCommon'), ('ftPopo', 'ftPopo')]:
        text = (sdk / ('src/melee/ft/kinds/' + name + '/forward.h')).read_text(encoding='utf-8')
        entries = re.search(r'typedef enum ' + kind + r'_MotionState\s*\{(.*?)\}', text, re.S).group(1)
        value = -1
        for entry in entries.split(','):
            entry = re.sub(r'/\*.*?\*/|//[^\n]*', '', entry, flags=re.S).strip()
            if not entry: continue
            token, *expr = entry.split('=', 1); token = token.strip()
            if not expr: value += 1
            elif re.fullmatch(r'-?\d+', expr[0].strip()): value = int(expr[0])
            elif expr[0].strip() in values: value = values[expr[0].strip()]
            else:
                left, right = map(str.strip, expr[0].split('-')); value = values[left] - values[right]
            values[token] = value
    return values

def compose(parameters, route):
    # Reuse actual accepted helper/article sources directly, preserving their
    # namespace. Only this namespace's new hooks are appended to the one move.
    component = HERE.parent / 'ice_climbers_composition'
    fighter = (component / 'fighter.fms').read_text(encoding='utf-8').replace('\r\n', '\n')
    prefix = fighter.split('\nmove Climbers:', 1)[0]
    prefix = re.sub(r'(?m)^base .+\n', '', prefix)
    follower = (HERE / 'follower.fms').read_text(encoding='utf-8')
    helper, hooks = follower.split('\nmove FollowerPolicy:', 1)
    capture = (HERE / 'capture.fms').read_text(encoding='utf-8').split('move PrimaryCapturePolicy:', 1)[1]
    articles = '\n'.join((component / (x + '.fms')).read_text(encoding='utf-8') for x in ['ice', 'blizzard'])
    probe = (HERE / 'query_fixture.fms').read_text(encoding='utf-8')
    return (('base ftDataTest 341 26\n' if route else '') + parameters + '\n' + prefix + '\n' + helper + '\n' + probe + '\n' + articles +
            ('\nmove OwnedFollower:\n' if route else '\naction OwnedFollower:\n') + hooks + capture +
            ('\n    on state_341_physics:\n        pass\n' if route else '\n    on load:\n        pass\n'))

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot', type=Path, required=True)
    p.add_argument('--reference', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--prepare-only', action='store_true')
    a = p.parse_args(); snapshot, ref, out = a.snapshot.resolve(), a.reference.resolve(), a.output.resolve()
    out.mkdir(parents=True, exist_ok=False); sdk = snapshot / 'sdk'
    receipt = {'status': 'WIP', 'scope': 'source-only owned follower; controlled engine boundaries', 'inputs': {}, 'operations': []}
    def verify(base, relative):
        path = base / relative
        manifest = {k.replace('\\', '/'): v for k, v in json.loads((base / 'freeze.json').read_text(encoding='utf-8')).items()}
        assert sha(path) == manifest[relative]
        receipt['inputs'][str(path)] = sha(path); return path
    for base in [snapshot, ref]: receipt['inputs'][str(base / 'freeze.json')] = sha(base / 'freeze.json')
    old = verify(ref, 'sdk/' + CPU).read_text(encoding='utf-8')
    original_names = SYMBOLS + ['inlineM0', 'ftCo_800B0918', 'ftCo_CpuDataShouldAct', 'ftCo_CpuShouldAct', 'ftCo_800A1F3C']
    functions = [body(old, x) for x in original_names]
    # The destination setter stays the actual complete source body; its stage
    # adjustment child is an explicit controlled boundary in the C fixture.
    (out / 'stock_bodies.h').write_text('\n\n'.join(functions), encoding='utf-8', newline='\n')
    receipt['original_bodies'] = {x: {'path': CPU, 'snapshot': str(ref), 'body_sha256': hashlib.sha256(y.encode('utf-8')).hexdigest()} for x, y in zip(original_names, functions)}
    capture = verify(ref, 'sdk/src/melee/ft/ftcpuattack.c').read_text(encoding='utf-8')
    capture_current = verify(snapshot, 'sdk/src/melee/ft/ftcpuattack.c').read_text(encoding='utf-8')
    (out / 'capture_body.h').write_text(body(capture, 'ftCo_800B683C').replace('ftCo_800B683C', 'stock_capture') + '\n' + body(capture_current, 'ftCo_800B683C'), encoding='utf-8', newline='\n')
    current = verify(snapshot, 'sdk/' + CPU).read_text(encoding='utf-8')
    (out / 'common_bodies.h').write_text(body(current, 'ftCo_CpuPrepareCombatPlan') + '\n' + body(current, 'ftCo_CpuPrepareDestinationUpdate'), encoding='utf-8', newline='\n')
    target = verify(snapshot, 'sdk/src/melee/ft/native_fighter_target.c').read_text(encoding='utf-8')
    targets = ['IsFighter', 'Position', 'Motion', 'PositionDelta', 'Damage', 'Facing', 'Sleeping', 'FloorLine', 'CpuBodyHalfWidth', 'Airborne']
    (out / 'target_bodies.h').write_text('\n'.join(body(target, 'nativeFighterTarget_' + x) for x in targets), encoding='utf-8', newline='\n')
    special = verify(ref, 'sdk/src/melee/ft/kinds/ftPopo/ftpopospeciallw.c').read_text(encoding='utf-8')
    (out / 'blizzard_entry.h').write_text(body(special, 'ftPp_SpecialLw_Enter'), encoding='utf-8', newline='\n')
    receipt['original_bodies']['ftPp_SpecialLw_Enter'] = {'path': 'src/melee/ft/kinds/ftPopo/ftpopospeciallw.c', 'snapshot': str(ref), 'body_sha256': hashlib.sha256(body(special, 'ftPp_SpecialLw_Enter').encode('utf-8')).hexdigest()}
    receipt['original_bodies']['ftCo_800B683C'] = {'path': 'src/melee/ft/ftcpuattack.c', 'snapshot': str(ref), 'body_sha256': hashlib.sha256(body(capture, 'ftCo_800B683C').encode('utf-8')).hexdigest()}
    receipt['current_bodies'] = {}
    for name, text, path in [('ftCo_CpuPrepareCombatPlan', current, CPU), ('ftCo_CpuPrepareDestinationUpdate', current, CPU), ('ftCo_800B683C', capture_current, 'src/melee/ft/ftcpuattack.c')] + [('nativeFighterTarget_' + x, target, 'src/melee/ft/native_fighter_target.c') for x in targets]:
        receipt['current_bodies'][name] = {'path': path, 'snapshot': str(snapshot), 'body_sha256': hashlib.sha256(body(text, name).encode('utf-8')).hexdigest()}
    values = enum_values(sdk)
    parameters = '\n'.join('parameter int ' + name + ' = ' + str(values[token]) for name, token in ENUMS.items()) + '\nparameter int follower_target_member = 0\n'
    assert parameters == (HERE / 'parameters.fms').read_text(encoding='utf-8'), 'Tracked enum/member parameters differ from frozen declarations'
    (out / 'parameters.fms').write_text(parameters, encoding='utf-8', newline='\n')
    for name in ['fighter', 'ice', 'blizzard']:
        dependency = HERE.parent / 'ice_climbers_composition' / (name + '.fms')
        receipt['inputs'][str(dependency)] = sha(dependency)
    for route in [0, 1]:
        text = compose(parameters, route)
        lines, tuning, scope = [], [], None
        for line in text.splitlines():
            if line.startswith('article '): scope = line.split()[1]
            if line.startswith(('move ', 'action ')): scope = None
            m = re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)', line)
            if m:
                indent, kind, name, value = m.groups()
                tuning.append(kind + ' ' + (scope + '.' if indent else '') + name + ' = ' + value)
            else: lines.append(line)
        (out / ('owned_' + str(route) + '.fms')).write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')
        (out / ('owned_' + str(route) + '.fmp')).write_text('\n'.join(tuning) + '\n', encoding='utf-8', newline='\n')
    receipt['inputs'][str(ROOT / 'CMakeLists.txt')] = sha(ROOT / 'CMakeLists.txt')
    cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
    sources = re.search(r'set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+(.*?)\)', cmake, re.S).group(1).split()
    source_paths = [verify(snapshot, 'sdk/' + x) for x in sources]
    # Explicit one-field fixture overlay, not a mutated SDK nor an alternate
    # engine operation. The parent-approved future production mapping is direct.
    original = verify(snapshot, 'sdk/tools/native_fighter_script_primitives.c')
    text = original.read_text(encoding='utf-8')
    anchor = '{ "ground.walk_speed", "fighter->co_attrs.walk_max_vel" },'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '\n        { "ground.mid_walk_point", "fighter->co_attrs.mid_walk_point" },')
    anchor = '{ "targets.motion", "target_motion" },'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '{ "targets.is_fighter", "target_is_fighter" },\n        ' + anchor)
    overlay = out / 'primitives.c'; overlay.write_text(text, encoding='utf-8', newline='\n')
    source_paths = [overlay if x == original else x for x in source_paths]
    clang = verify(snapshot, 'compiler/clang.exe')
    common = [str(clang), '-std=c11', '-O2', '-fms-extensions', '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS', '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include'), '-I' + str(sdk / 'tools'), '-I' + str(out)]
    commands = [('generator_build', common + [str(HERE / 'generate.c'), *map(str, source_paths), str(sdk / 'src/melee/ft/native_fighter_parameters.c'), '-o', str(out / 'generate.exe')])]
    for route in [0, 1]:
        commands.append(('generate_' + str(route), [str(out / 'generate.exe'), str(out / ('owned_' + str(route) + '.fms')), str(out / ('owned_' + str(route) + '.fmp')), str(out / ('owned_' + str(route) + '.inc'))]))
    if not a.prepare_only:
        for route in [0, 1]:
            commands += [('build_' + str(route), common + ['-DOWNED_ROUTE=' + str(route), str(HERE / 'compare.c'), str(sdk / 'src/melee/ft/native_cpu_history.c'), '-o', str(out / ('compare_' + str(route) + '.exe'))]), ('run_' + str(route), [str(out / ('compare_' + str(route) + '.exe'))])]
    for name, command in commands:
        r = subprocess.run(command, cwd=ROOT, text=True, encoding='utf-8', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        (out / (name + '.log')).write_text(r.stdout, encoding='utf-8', newline='\n')
        receipt['operations'].append({'name': name, 'command': command, 'exit_code': r.returncode, 'output': r.stdout})
        receipt['status'] = 'FAIL' if r.returncode else 'WIP'
        (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8', newline='\n')
        print(name, r.returncode, r.stdout)
        if r.returncode: raise RuntimeError(out / (name + '.log'))
        if name.startswith('generate_'):
            route = name[-1]; code = (out / ('owned_' + route + '.inc')).read_text(encoding='utf-8')
            host_names = sorted(set(re.findall(r'host->(\w+) == 0', code)))
            (out / ('host_' + route + '.inc')).write_text('\n'.join('host.' + x + ' = (__typeof__(host.' + x + '))unused;' for x in host_names if x != 'sound_channels'), encoding='utf-8', newline='\n')
            schema = re.search(r'schema != (0x[0-9a-f]+)ULL', code).group(1)
            bits = []
            for line in (out / ('owned_' + route + '.fmp')).read_text(encoding='utf-8').splitlines():
                kind, _, _, value = line.split()
                bits.append(struct.unpack('<I', struct.pack('<f', float(value)))[0] if kind == 'float' else int(value) & 0xffffffff)
            (out / ('parameters_' + route + '.inc')).write_text('static const uint32_t tuning[] = {' + ','.join(str(x) + 'U' for x in bits) + '};\nstatic const uint64_t tuning_schema = ' + schema + 'ULL;\n', encoding='utf-8', newline='\n')
    for x in HERE.iterdir():
        if x.is_file(): receipt['inputs'][str(x)] = sha(x)
    for x in out.iterdir():
        if x.suffix in ['.fms', '.fmp', '.inc', '.h']: receipt['inputs'][str(x)] = sha(x)
    receipt['overlay_sha256'] = sha(overlay)
    receipt['results'] = {}
    pattern = r'PASS (\d+) whole-phase pairs, (\d+) eligibility pairs, (\d+) threshold pairs, (\d+) checks, (\d+) authored safety cases; max_float_error ([0-9.eE+-]+)'
    for operation in receipt['operations']:
        if operation['name'].startswith('run_'):
            match = re.search(pattern, operation['output'])
            assert match, 'Missing full comparison summary'
            receipt['results'][operation['name']] = dict(zip(['whole_phase_pairs', 'eligibility_pairs', 'threshold_pairs', 'checks', 'authored_safety_cases'], map(int, match.groups()[:5])), max_float_error=float(match.group(6)))
    receipt['status'] = 'GENERATED' if a.prepare_only else 'PASS'
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(out / 'receipt.json')

if __name__ == '__main__': main()
