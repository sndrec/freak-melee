"""Read-only normal-project DAT and original caller inventory for the Peach gate."""
import hashlib
import re
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Dat:
    def __init__(self, path):
        self.raw = path.read_bytes()
        self.size, reloc, public, external = struct.unpack_from('>4I', self.raw, 4)
        table = 32 + self.size + 4 * reloc
        strings = table + 8 * (public + external)
        self.roots = {}
        for i in range(public):
            offset, name = struct.unpack_from('>2I', self.raw, table + 8 * i)
            self.roots[self.raw[strings + name:].split(b'\0', 1)[0].decode()] = offset

    def word(self, offset):
        assert 0 <= offset <= self.size - 4, offset
        return struct.unpack_from('>I', self.raw, 32 + offset)[0]

    def joints(self, root):
        visited = set()

        def visit(offset):
            if not offset:
                return
            assert offset not in visited, ('cyclic/repeated joint descriptor', offset)
            visited.add(offset)
            visit(self.word(offset + 8))
            visit(self.word(offset + 12))

        visit(root)
        return len(visited)


SLOTS = {'It_Kind_Peach_Explode': (0, 98), 'It_Kind_Peach_Turnip': (1, 99),
         'It_Kind_Peach_Parasol': (2, 103), 'It_Kind_Peach_Toad': (3, 104),
         'It_Kind_Peach_ToadSpore': (4, 111)}


def inspect(snapshot, project, candidate, parameter_text):
    sdk = snapshot / 'sdk'
    fighter_path = project / 'fighter.dat'
    dat = Dat(fighter_path)
    root = dat.roots['ftDataPeach']
    attrs, table = dat.word(root + 4), dat.word(root + 72)
    params = {}
    for line in parameter_text.read_text().splitlines():
        typ, name, equal, value = line.split()
        assert equal == '='
        bits = struct.unpack('<I', struct.pack('<f', float(value)))[0] if typ == 'float' else int(value, 0) & 0xffffffff
        params[name] = (typ, bits)
    source = candidate.read_text()
    attributes = []
    active = None
    for line in source.splitlines():
        if line.startswith('article '):
            active = line.split()[1]
        match = re.fullmatch(r'(\s*)attribute (float|int) (\w+) (\d+)', line)
        if not match:
            continue
        indent, typ, name, offset = match.groups()
        offset = int(offset)
        base = dat.word(dat.word(table + 4 * SLOTS[active][0]) + 4) if indent else attrs
        qualified = active + '.' + name if indent else name
        assert params[qualified] == (typ, dat.word(base + offset)), qualified
        attributes.append({'name': qualified, 'DAT_data_offset': base + offset,
                           'type': typ, 'word': dat.word(base + offset)})
    assert len(attributes) == 67
    articles = []
    for name, (slot, kind) in SLOTS.items():
        article = dat.word(table + slot * 4)
        assert article
        model, states = dat.word(article + 16), dat.word(article + 12)
        # Explode/Spore are intentionally effect-only: the retail model
        # descriptor exists with a NULL graph. Do not invent a joint root.
        assert model and states
        articles.append({'name': name, 'kind': kind, 'slot': slot, 'article_offset': article,
                         'common_attributes_offset': dat.word(article),
                         'special_attributes_offset': dat.word(article + 4),
                         'state_descriptors_offset': states,
                         'first_state_animation_offset': dat.word(states),
                         'first_state_matanim_offset': dat.word(states + 4),
                         'model_descriptor_offset': model,
                         'joint_descriptor_offset': dat.word(model),
                         'model_bone_count_source_word': dat.word(model + 4),
                         'joint_descriptor_preorder_count': dat.joints(dat.word(model)),
                         'attachment_index_word': dat.word(model + 8)})
    costume_path = project / 'costume.dat'
    costume = Dat(costume_path)
    joint = costume.roots['PlyPeach5K_Share_joint']
    assert costume.joints(joint) == 114
    assert 'PlyPeach5K_Share_matanim_joint' in costume.roots
    # Full immutable source inventory. Not every mention is a gameplay decision:
    # roster registration, semantic tracing and asset import are classified in
    # caller_closure.md instead of being silently described as authored policy.
    mentions = []
    for path in sorted((sdk / 'src/melee').rglob('*.c')):
        if '/kinds/ftPeach/' in path.as_posix() or path.name.startswith('itpeach'):
            continue
        text = path.read_text(encoding='utf-8', errors='replace')
        lines = [{'line': i, 'text': line.strip()} for i, line in enumerate(text.splitlines(), 1)
                 if re.search(r'\b(?:Ft_Kind_Peach|ftPe_\w+)\b', line)]
        if lines:
            mentions.append({'path': path.relative_to(sdk).as_posix(), 'sha256': sha(path), 'mentions': lines})
    original = (sdk / 'src/melee/ft/kinds/ftPeach/ftpeach.c').read_text()
    rows = re.findall(r'\{\s*// ftPe_MS_(\w+) = (\d+)\s*\n(.*?)\n\s*\}', original, re.S)
    assert len(rows) == 30
    phases = []
    for name, motion, fields in rows:
        values = [v.strip() for v in fields.split(',') if v.strip()]
        assert len(values) == 8
        bindings = []
        for field, callback in zip(('animation', 'interrupt', 'physics', 'collision'), values[3:7]):
            common = callback.startswith('ftCo_')
            present = 'on state_' + motion + '_' + field + ':' in source
            if callback != 'NULL':
                assert common or present, (motion, field, callback)
            bindings.append({'phase': field, 'original': callback, 'authored_handler': present,
                             'explicit_common_policy': common and int(motion) in (369, 370)})
        phases.append({'name': name, 'motion': int(motion), 'bindings': bindings})
    return {'snapshot': snapshot.name, 'raw_attribute_words_verified': attributes,
            'normal_article_assets': articles,
            'rig': {'costume_joint_descriptor': joint, 'physical_count': 114,
                    'runtime_count': 114, 'semantic_count': 53, 'omission_count': 0},
            'motion_rows': phases, 'original_source_mentions': mentions,
            'inputs': {str(p): sha(p) for p in [fighter_path, costume_path, parameter_text,
                       candidate, HERE / 'audit.py', sdk / 'src/melee/it/types.h',
                       sdk / 'src/sysdolphin/baselib/jobj.h']}}
