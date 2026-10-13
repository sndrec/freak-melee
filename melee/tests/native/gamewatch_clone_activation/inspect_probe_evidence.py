"""Offline package clip identity and observed trace representation evidence.

Does not launch games, alter logs, or convert a failed strict comparison to PASS.
"""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--snapshot', required=True, type=Path)
    ap.add_argument('--prepared', required=True, type=Path)
    ap.add_argument('--chef-logs', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    a = ap.parse_args()
    snap, project = a.snapshot.resolve(), a.prepared.resolve() / 'project'
    if a.output.exists():
        ap.error('fresh evidence output required')
    sys.path.insert(0, str(snap / 'sdk/tools'))
    from fighter_animation_io import Dat
    dat = Dat((project / 'fighter.dat').read_bytes())
    animations = (project / 'animation.dat').read_bytes()
    root = dat.roots['ftDataGamewatch']
    own, borrowed = [], []
    for bank, pointer, count in [('action', 12, 323), ('demo', 20, 14)]:
        table = dat.u32(root + pointer)
        for action in range(count):
            row = table + action * 24
            size = dat.u32(row + 8)
            if not size:
                continue
            flags = dat.u32(row + 16)
            kind, offset = flags & 63, dat.u32(row + 4)
            name = dat.string(dat.u32(row))
            clip = animations[offset:offset + size]
            assert len(clip) == size
            entry = dict(bank=bank, action=action, name=name, animation_source_kind=kind,
                         raw_flags=hex(flags), offset=offset, size=size,
                         clip_sha256=hashlib.sha256(clip).hexdigest())
            (own if kind == 24 else borrowed).append(entry)
    assert len(own) == 278 and len(borrowed) == 4
    assert [r['action'] for r in borrowed] == [262, 263, 264, 265]
    assert all(r['animation_source_kind'] == 33 for r in borrowed)
    expected = ['ThrowF', 'ThrowB', 'ThrowHi', 'ThrowLw']
    assert all('Taro_Share_ACTION_TGamewatch' + n in r['name']
               for n, r in zip(expected, borrowed))

    # Both recorded velocity components identify variant2 from the imported
    # constructor recipe. The only final Chef core field is its private union
    # variant; invert the actual one-word FNV operation without changing traces.
    stock = (a.chef_logs / 'chef-ground-stock.log').read_text()
    clone = (a.chef_logs / 'chef-ground-clone.log').read_text()
    pattern = r'\[item\] sample 168 I0 kind=122 state=0 core=([0-9a-f]+)'
    old, new = [int(re.search(pattern, text)[1], 16) for text in (stock, clone)]
    prime, mask = 1099511628211, (1 << 64) - 1
    inverse = pow(prime, -1, 1 << 64)
    pre = ((old * inverse) & mask) ^ 2
    observed_word = pre ^ ((new * inverse) & mask)
    assert observed_word == 0x3f800000
    assert ((pre ^ observed_word) * prime) & mask == new
    generated = (project / 'behavior.generated.c').read_text()
    assert '{ "variant", 0, 1, 2, 4, false }' in generated
    assert 'scripted_article_set_state_int(item, 0, (int32_t)(item->xDD4_itemVar.native_article.creation_value))' in generated
    inputs = [Path(__file__), project / 'fighter.dat', project / 'animation.dat',
              project / 'behavior.generated.c', project / 'fighter_parameters.fmp',
              a.chef_logs / 'chef-ground-stock.log', a.chef_logs / 'chef-ground-clone.log']
    sources = ['src/melee/ft/ftdata.c', 'src/melee/ft/ftanim.c',
               'src/melee/ft/ftparts_table.c', 'src/melee/ft/native_fighter_runtime.c',
               'src/melee/gm/native_semantic_state.c', 'src/melee/it/types.h',
               'src/melee/it/itCharItems.h', 'src/melee/it/kinds/itgamewatchchef.c']
    inputs += [snap / 'sdk' / p for p in sources]
    result = dict(status='OFFLINE DIAGNOSIS; original strict Chef comparison remains failed',
                  snapshot=str(snap), freeze_sha256=sha(snap / 'freeze.json'),
                  own_clip_count=len(own), borrowed_clips=borrowed,
                  borrowed_classification='Four package-owned universal Taro thrown-victim animations, tagged Ft_Kind_None33; no donor fighter kind. Retargeting uses common semantic part tables; no stock fighter callback fallback.',
                  chef=dict(sample=168, stock_variant=2, scripted_variant_type='Int',
                            scripted_variant_state_offset=0, recovered_trace_word=hex(observed_word),
                            trace_word_meaning='native_article.attachment_facing float1.0 overlaps private stock variant word',
                            stock_core=f'{old:016x}', clone_core=f'{new:016x}',
                            model_evidence='food2 velocity(.6,1.2), identical pose/visual digests at first spawn; constructor source agreement is separate accepted evidence'),
                  source_sha256={str(p.resolve()): sha(p) for p in inputs})
    a.output.write_text(json.dumps(result, indent=2) + '\n', newline='\n')
    print(a.output)


if __name__ == '__main__':
    main()
