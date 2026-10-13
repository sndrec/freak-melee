"""Read-only callback/caller closure census for the assembled Game & Watch."""
import argparse
import hashlib
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--snapshot', type=Path, required=True)
    ap.add_argument('--candidate', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    snap, sdk = a.snapshot.resolve(), a.snapshot.resolve() / 'sdk'
    text = a.candidate.read_text()
    freeze = json.loads((snap / 'freeze.json').read_text())
    source_hashes = {}
    def source(path):
        digest = sha(sdk / path)
        assert freeze[str((sdk / path).relative_to(snap))] == digest
        source_hashes[path] = digest
        return (sdk / path).read_text(encoding='utf-8', errors='replace')

    stock = source('src/melee/ft/kinds/ftGameWatch/ftgamewatch.c')
    motion_table = re.search(r'MotionState ftGw_Init_MotionStateTable\[[^\]]+\]\s*=\s*\{(.*?)\n\};', stock, re.S)[1]
    rows = []
    for name, index, block in re.findall(r'// ftGw_MS_(\w+) = (\d+)\n(.*?)(?=\n    \},)', motion_table, re.S):
        tokens = [v.strip() for v in block.split(',') if v.strip()]
        rows.append({'name': name, 'motion': int(index),
                     'original_callbacks': tokens[-5:],
                     'authored_state_slots': re.findall(r'on state_' + index + r'_(\w+):', text),
                     'common_policy': re.findall(r'^motion_policy ' + index + r' common (\d+)$', text, re.M)})
    assert len(rows) == 40 and [r['motion'] for r in rows] == list(range(341,381))
    # A special move supplies both ground/air slots once across its family;
    # explicit per-state duplicates are not required for each Judge selection.
    hooks = sorted(set(re.findall(r'^    on (\w+):', text, re.M)))
    callers = []
    for folder in ('src/melee/ft', 'src/melee/it'):
        for path in sorted((sdk / folder).rglob('*.c')):
            relative = path.relative_to(sdk).as_posix()
            if '/ftGameWatch/' in relative or path.name.startswith('itgamewatch'):
                continue
            raw = path.read_text(encoding='utf-8', errors='replace')
            hits = [(line, value.strip()) for line, value in enumerate(raw.splitlines(), 1)
                    if re.search(r'Ft_Kind_GameWatch|\bftGw_|\bu\.gw\.', value)]
            if not hits:
                continue
            source(relative)
            if 'Kirby' in relative or 'kirby' in path.name:
                category = 'excluded Kirby copy'
            elif path.name == 'ftcpuattack.c':
                category = 'excluded Kirby case; inspect case ownership, not union spelling'
            elif path.name in ('ftaction.c', 'ft_0881.c'):
                category = 'BLOCKER: muted sound channel exception needs generic immutable policy'
            elif path.name == 'ftdata.c':
                category = 'copied metadata or generated exclusive callbacks'
            elif path.name == 'ftlib.c':
                category = 'stock CE64 palette helpers replaced by direct typed creator helpers'
            elif path.name == 'ftcharacter_assets.c':
                category = 'asset decoder schema; copied base owns equivalent immutable layout'
            elif path.name.startswith('ftCo_Attack'):
                category = 'generic entry replacement present; new normal phase agreement pending'
            elif path.name == 'ftCo_Landing.c':
                category = 'owned landing_action clears judge_impulse_used after common setup'
            elif path.name == 'it_3F2F.c':
                category = 'stock item catalog; owned ten article logics replace this family'
            elif path.name in ('native_fighter_article_bindings.c', 'native_fighter_project.c', 'native_fighter_runtime.c'):
                category = 'registration/import metadata; no authored gameplay delegation'
            else:
                category = 'REVIEW source path'
            callers += [{'source':relative,'line':line,'text':value,'classification':category}
                        for line,value in hits]
    assert not any(c['classification'] == 'REVIEW source path' for c in callers), callers
    result = {'status':'SOURCE CENSUS; not activation or full-fighter acceptance',
              'snapshot':str(snap),'freeze_sha256':sha(snap/'freeze.json'),
              'candidate':str(a.candidate.resolve()),'candidate_sha256':sha(a.candidate),
              'source_sha256':source_hashes,'motion_rows':rows,'declared_hooks':hooks,
              'article_kinds':re.findall(r'^article (\w+) states (\d+):',text,re.M),
              'global_helper_count':len(re.findall(r'^func ',text,re.M)),
              'callers':callers,
              'cpu':'No GameWatch-specific CPU branch in ftcpuattack.c; lone u.gw read belongs to Kirby case. Normal clone must copy retail CPU profile.',
              'taunt':'No GameWatch-specific taunt policy; common AppealS plus copied animations.',
              'remaining':['Normal project clone/import/DLL/package ownership gate after real base registry is rebuilt.',
                           'Actual original/generated new normal entry and all twelve phase comparison.',
                           'Muted sound channel exceptions at ftaction/ft_0881.',
                           'Real drawn gameplay/renderer acceptance; prior source fixtures do not prove pixels.']}
    assert len(result['article_kinds']) == 10
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n',newline='\n')
    print(json.dumps({'receipt':str(a.output),'motions':len(rows),'articles':len(result['article_kinds']),
                      'global_helpers':result['global_helper_count'],'caller_references':len(callers)}))


if __name__ == '__main__':
    main()
