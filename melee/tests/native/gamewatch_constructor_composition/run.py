"""All ten original/owned paper constructors; console only, immutable SDK62+."""
import argparse, hashlib, importlib.util, json, os, re, struct, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
from audit import ROWS, extract, sha
PICKUP = ['itGamewatchGreenhouse_PickedUp', 'itGameWatchManhole_Logic72_PickedUp',
          'itGamewatchFire_PickedUp', 'itGameWatchParachute_Logic74_PickedUp',
          'itGameWatchTurtle_Logic75_PickedUp', 'itGameWatchBreath_Logic76_PickedUp',
          'it_2725_Logic77_PickedUp', 'itGameWatchPanic_Logic78_PickedUp']

def compose(out, dat, root):
    path = HERE.parent / 'gamewatch_normals_composition/compose.py'
    spec = importlib.util.spec_from_file_location('accepted_normal_composition', path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    candidate = module.compose(out, compare=False)
    text = candidate.read_text()
    additions = (HERE / 'constructors.pending.fms').read_text()
    for kind, symbol, count, family in ROWS:
        pattern = r'(?ms)^article It_Kind_GameWatch_' + kind + r' states \d+:\n(.*?)(?=^article |\Z)'
        incoming = re.search(pattern, additions)[1]
        # The accepted Chef already declares its variant, so do not duplicate it.
        incoming = incoming.replace('    state int variant\n', '')
        match = re.search(pattern, text); assert match, kind
        block = match[1]
        if kind == 'Chef':
            block = block.replace('    state int variant\n', '')
            incoming = '    state int variant\n' + incoming
        block, n = re.subn(r'(?ms)^    on created:\n.*?(?=^    [^ #\n]|\Z)', '', block)
        assert n == 1, (kind, n)
        text = text[:match.start(1)] + incoming + block + text[match.end(1):]
    # Replace the old provisional spellings with the shipped generic contracts.
    text = text.replace('item.joint_world_position("0/0",', 'item.joint_world_position(int(0), "0",')
    text = text.replace('item.wind_at_coordinates(', 'effects.wind_at_position(')
    # "" addresses the primary root; "0" is its first child. The source
    # BB20/BB44 operations apply recursively to the actual primary root.
    # Correct the new assembly only; accepted inputs stay intact.
    text = text.replace('item.joint("0").visible', 'item.joint("").visible')
    helpers = (HERE / 'palette_helpers.fms').read_text()
    attrs = struct.unpack_from('>I', dat, 32 + root + 4)[0]
    palette = []
    for kind, name, offset in re.findall(r'^attribute (float|byte) (\w+) (\d+)$', helpers, re.M):
        at = 32 + attrs + int(offset)
        value = struct.unpack_from('>f', dat, at)[0] if kind == 'float' else dat[at]
        palette.append(('float' if kind == 'float' else 'int', name, value))
    helpers = re.sub(r'^attribute .*\n', '', helpers, flags=re.M)
    # Keep all global declarations before the first helper; all original callbacks
    # remain complete. No filtering of renderer/wind/state bodies.
    first = re.search(r'(?m)^(func |export func |move |action |article )', text).start()
    text = text[:first] + helpers + '\n' + text[first:]
    params = (out / 'fighter_parameters.fmp').read_text()
    params += ''.join(f'{kind} {name} = {value}\n' for kind, name, value in palette)
    items = struct.unpack_from('>I', dat, 32 + root + 0x48)[0]
    chef = struct.unpack_from('>I', dat, 32 + items + 8 * 4)[0]
    food = struct.unpack_from('>I', dat, 32 + chef + 4)[0]
    raw_food = [('food_wall_bounce', 4), ('food_active_lifetime', 8), ('food_inactive_lifetime', 12)]
    for i in range(5):
        for channel, name in enumerate(['horizontal', 'vertical', 'gravity', 'terminal', 'angular']):
            raw_food.append((f'food_{i}_{name}', 16 + 20 * i + 4 * channel))
    for name, offset in raw_food:
        value = struct.unpack_from('>f', dat, 32 + food + offset)[0]
        params, n = re.subn(r'(?m)^float ' + name + r' = .*$', f'float {name} = {value}', params)
        assert n == 1, name
    candidate.write_text(text, newline='\n')
    (out / 'fighter_parameters.fmp').write_text(params, newline='\n')
    return candidate

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--snapshot', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--iso', type=Path, default=ROOT.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso')
    a = ap.parse_args(); snap, out = a.snapshot.resolve(), a.output.resolve(); sdk = snap / 'sdk'
    if out.exists() or any(out == p or p in out.parents for p in [snap, HERE]):
        ap.error('fresh output outside immutable inputs required')
    frozen = json.loads((snap / 'freeze.json').read_text())
    def verify():
        for name, digest in frozen.items():
            if sha(snap / name) != digest: raise RuntimeError('changed snapshot: ' + name)
    verify()
    layout = (sdk / 'src/melee/ft/ftcharacter_assets.c').read_text()
    paper = re.search(r'\[Ft_Kind_GameWatch\] = \{(.*?)\},', layout, re.S)[1]
    if not re.search(r'byte_ranges\s*=\s*\{\{\s*(?:4|0x0?4),\s*(?:20|0x14)\s*\}\}', paper):
        ap.error('requires repaired stock Paper raw colors {4,20}; frozen61 is insufficient')
    out.mkdir(parents=True); commands, inputs = [], {}
    def run(stage, cmd):
        cmd = list(map(str, cmd)); result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=180)
        (out / (stage + '.log')).write_text(result.stdout + result.stderr, newline='\n')
        commands.append({'stage': stage, 'command': cmd, 'exit': result.returncode})
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', newline='\n')
        if result.returncode: raise RuntimeError(stage + ' failed: ' + result.stdout + result.stderr)
        return result.stdout
    sys.path.insert(0, str(sdk / 'tools'))
    from extract_native_animation import extract_clip
    dat, root = extract_clip(a.iso, 'PlGw.dat', 'ftDataGamewatch'); (out / 'PlGw.dat').write_bytes(dat)
    candidate = compose(out, dat, root)
    generated = out / 'behavior.generated.h'
    run('generate', [snap / 'bin/native_content_tool.exe', 'script-c', candidate, 'gamewatch_constructor_probe', generated])
    code = generated.read_text(); schema = re.search(r'schema != (0x[0-9a-fA-F]+)ULL', code)[1]
    values, names = [], []
    for line in (out / 'fighter_parameters.fmp').read_text().splitlines():
        kind, name, eq, value = line.split(); assert eq == '='
        names.append(name); values.append(struct.unpack('<I', struct.pack('<f', float(value)))[0] if kind == 'float' else int(value) & 0xFFFFFFFF)
    (out / 'parameters.h').write_text('#define FIXTURE_PARAMETER_COUNT ' + str(len(values)) + '\n#define FIXTURE_PARAMETER_SCHEMA ' + schema + 'ULL\nstatic uint32_t fixture_parameter_bits[]={' + ','.join(map(str, values)) + '};\nstatic const char* fixture_parameter_names[]={' + ','.join(json.dumps(n) for n in names) + '};\n', newline='\n')
    hosts = sorted(set(re.findall(r'host->([a-zA-Z0-9_]+) == 0', code)))
    (out / 'unused_hosts.h').write_text(''.join('host.' + h + '=(__typeof__(host.' + h + '))unused;\n' for h in hosts), newline='\n')
    bodies, source_bodies = [], []
    def source(relative, symbols):
        path = sdk / relative; inputs[str(path)] = sha(path); text = path.read_text()
        for name in symbols:
            body, line = extract(text, name)
            bodies.append(body)
            source_bodies.append({'source': relative, 'name': name, 'line': line,
                'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    for i, (kind, symbol, _, _) in enumerate(ROWS):
        source('src/melee/it/kinds/itgamewatch' + kind.lower() + '.c',
               ([PICKUP[i]] if i < len(PICKUP) else []) +
               ({'Judge': ['it_802C78B8'], 'Chef': ['it_802C84A0'], 'Rescue': ['it_802C8208']}.get(kind, [])) + [symbol])
    for relative, symbols in [
        ('src/melee/it/item.c', ['Item_8026AB54', 'Item_8026B074', 'Item_802694CC']),
        ('src/melee/it/it_2725.c', ['it_802742F4', 'it_80273670', 'HSD_JObjSetScale_2', 'it_80274594', 'it_80275158', 'it_80272A18', 'it_80272A3C']),
        ('src/melee/it/it_26B1.c', ['it_8026BB44', 'it_8026B3A8']),
        ('src/melee/it/itzako.c', ['it_8027CE64']),
        ('src/melee/it/itmaterial.c', ['it_80278574']),
        ('src/melee/ft/ftlib.c', ['ftLib_8008770C', 'ftLib_80087744', 'ftLib_800870BC', 'ftLib_800865C0', 'ftLib_800865CC', 'ftLib_80086EB4']),
        ('src/melee/ft/ftidentity.c', ['ftLib_80086960', 'ftLib_80086BE0']),
        ('src/melee/ft/kinds/ftGameWatch/ftgamewatch.c', ['ftGw_Init_8014A7F4', 'ftGw_Init_8014A814']),
        ('src/melee/ft/kinds/ftGameWatch/ftgamewatchattack11.c', ['ftGw_Attack11_ItemGreenhouse_CheckAttack11']),
        ('src/melee/db/dbitem.c', ['db_80225DD8']),
        ('src/melee/it/article_model_layers.c', ['articleModelLayers_ValidateItem', 'articleModelLayers_Bind']),
        ('src/melee/it/native_article_model.c', ['model_root', 'nativeArticleModel_Read', 'nativeArticleModel_Write']),
        ('src/melee/it/native_article.c', ['nativeArticle_Creator', 'article_logic', 'nativeArticle_Create']),
        ('src/sysdolphin/baselib/jobj.c', ['HSD_JObjGetDObj', 'HSD_JObjGetFlags', 'HSD_JObjSetFlags', 'HSD_JObjSetFlagsAll', 'HSD_JObjClearFlags', 'HSD_JObjClearFlagsAll', 'HSD_JObjSetMtxDirty'])]:
        source(relative, symbols)
    declarations = ''.join(b[:b.index('{')].strip() + ';\n' for b in bodies)
    (out / 'original.h').write_text(declarations + '\n'.join(bodies) + '\n', newline='\n')
    sources = [sdk / 'src/sysdolphin/baselib' / (n + '.c') for n in ['dat', 'dat_import', 'dat_material', 'dat_texture', 'dat_animation', 'dat_model', 'dat_scene', 'dat_geometry']]
    sources += [sdk / 'src/melee/it/itcommand_data.c', sdk / 'src/melee/it/itarticle_data.c', sdk / 'src/melee/it/article_model_layers.c', sdk / 'src/melee/lb/native_model_joint.c', sdk / 'src/melee/ft/native_callback_lifetime.c', HERE.parent / 'model_effect_absent_fixture.c']
    # Decoder functions come from the full unit; extracted runtime functions
    # would otherwise collide. Preserve its actual implementation, no wrappers.
    layer_source = sdk / 'src/melee/it/article_model_layers.c'
    actual = layer_source.read_text()
    actual = actual[:actual.index('bool articleModelLayers_ValidateItem')]
    actual = actual.replace('#include "article_model_layers.h"', '#include <melee/it/article_model_layers.h>').replace('#include "types.h"', '#include <melee/it/types.h>')
    (out / 'layer_decoder.c').write_text(actual, newline='\n')
    sources[sources.index(layer_source)] = out / 'layer_decoder.c'
    vswhere = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], capture_output=True, text=True, check=True).stdout.strip()
    flags = [snap / 'compiler/clang.exe', '-std=gnu11', '-O1', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections', '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt', '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include'), '-I' + str(sdk / 'src/melee/it/kinds'), '-I' + str(out), '-fuse-ld=lld', '-Wl,/OPT:REF']
    command = [*flags, HERE / 'compare.c', *sources, '-o', out / 'compare.exe']
    (out / 'module.c').write_text('#include "behavior.generated.h"\n', newline='\n')
    dll_command = [*flags, '-shared', out / 'module.c', '-o', out / 'behavior.dll']
    setup = Path(vs) / 'VC/Auxiliary/Build/vcvars64.bat'
    (out / 'compile.cmd').write_text('@echo off\ncall "' + str(setup) + '" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n' + subprocess.list2cmdline(list(map(str, dll_command))) + '\nif errorlevel 1 exit /b 1\n' + subprocess.list2cmdline(list(map(str, command))) + '\n', newline='\n')
    run('compile', ['cmd.exe', '/d', '/c', out / 'compile.cmd'])
    result = json.loads(run('run', [out / 'compare.exe', out / 'PlGw.dat', out / 'behavior.dll']).strip())
    verify()
    for path in sources + [HERE / 'compare.c', HERE / 'run.py', HERE / 'audit.py', HERE / 'README.md',
            HERE / 'constructors.pending.fms', HERE / 'palette_helpers.fms',
            candidate, generated, out / 'original.h', out / 'parameters.h',
            out / 'fighter_parameters.fmp', out / 'composition_inputs.json',
            out / 'module.c', out / 'compile.cmd']:
        inputs[str(path)] = sha(path)
    for name in ['compose.py', 'fighter.fms', 'fighter_parameters.fmp', 'props.fms']:
        path = HERE.parent / 'gamewatch_normals_composition' / name
        inputs[str(path)] = sha(path)
    receipt = {'status': 'PASS scoped constructor source comparison; unregistered', 'result': result,
        'snapshot': str(snap), 'freeze_manifest_sha256': sha(snap / 'freeze.json'),
        'dat_sha256': sha(out / 'PlGw.dat'), 'commands': commands, 'inputs': inputs,
        'source_bodies': source_bodies,
        'accepted_composition_inputs': json.loads((out / 'composition_inputs.json').read_text()),
        'outputs': {name: sha(out / name) for name in ['behavior.dll', 'compare.exe']},
        'boundaries': [
            'Actual full constructors/pickup/attach/CE64/palette/diffuse/layer/joint/lifetime bodies execute; separately loaded generated DLL owns all created and pickup callbacks.',
            'Allocation, HSD pose realization/animation/commands/attachment geometry and lookup tables are controlled. Real DAT article imports execute via existing shared Paper schema selectors; this is not normal copied-fighter clone/package validation.',
            'Synthetic inherited base/motion metadata; motion callbacks unexecuted. Other Host operations trap.',
            'Transfer cases prove paper palette vs current-owner tint/scale with equal controlled debug inputs; no mid-allocation retail reflection or arbitrary transferred debug claim.',
            'No GX/pixel/full renderer/gameplay/independence acceptance. See namespace README for the complete boundary census.']}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', newline='\n')
    print(json.dumps(result))
if __name__ == '__main__': main()
