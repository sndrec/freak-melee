"""Compose Samus authoring components for interpreter preparation.

The component composition omits activation for focused fixtures. The published
Workshop template adds independence after composing all fighter, grapple,
combat-article and lifecycle decisions; it never delegates to a stock special.
"""
from pathlib import Path
import hashlib
import json
import re
ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
INPUTS = [ROOT/'examples/samus_wip.fms',
    ROOT/'tests/native/samus_tether_composition/fighter.component.fms',
    ROOT/'tests/native/samus_grapple_article_composition/article.component.fms',
    HERE/'fighter.constructor.fms', HERE/'constructor.component.fms',
    ROOT/'tests/native/samus_tether_composition/ground.component.fms',
    HERE/'controls.component.fms',
    ROOT/'tests/native/samus_tether_composition/lifecycle.component.fms']

def chunks(source):
    starts = list(re.finditer(r'^(?:article|func|export func|action|move|state|attribute|parameter)\s', source, re.M))
    return [source[m.start():starts[i+1].start() if i+1<len(starts) else len(source)]
            for i,m in enumerate(starts)]

def compose():
    base, fighter, article, constructor_fighter, constructor_article, ground, controls, lifecycle = [p.read_text(encoding='utf-8') for p in INPUTS]
    # Extend the existing lifecycle events, rather than registering duplicate
    # named hooks or falling back to native Samus union resets.
    old_cleanup='        shot_remove()\n        state.shot_charge = 0\n        spin_clear()'
    if base.count(old_cleanup)!=2:
        raise ValueError('Samus damage/death2 event bodies changed; review composition')
    base=base.replace(old_cleanup,'        Samus_DamageDeathCleanup()')
    load='        fighter.copy_ext_attributes()\n        state.shot_handle = -1'
    death='        state.missile_serial = 0\n        state.spin_effect = false'
    if base.count(load)!=1 or base.count(death)!=1:
        raise ValueError('Samus load/death event bodies changed; review composition')
    base=base.replace(load,load+'\n        state.tether_handle = -1\n        state.tether_code_sequence = 0')
    base=base.replace(death,death+'\n        state.tether_handle = -1\n        state.tether_code_sequence = 0')
    # Compose the actual node/model constructor and common Catch signals into
    # the owned solver, never the older controlled Article boundary fixture.
    declarations, callbacks = constructor_article.split('    on created:', 1)
    declarations = declarations[declarations.index('    model_animation '):]
    article = article.replace('    func Dispose():', declarations+'\n    func Dispose():', 1)
    article = article.replace('    on signal hitlag_update:', '    on created:'+callbacks+'\n    on signal hitlag_update:', 1)
    control_fields, control_callbacks = controls.split('    on signal extend_air:', 1)
    control_fields = control_fields[control_fields.index('    state '):]
    article = article.replace('    func Dispose():', control_fields+'\n    func Dispose():', 1)
    article = article.replace('    state 0 anim -1:', '    on signal extend_air:'+control_callbacks+'\n    state 0 anim -1:', 1)
    pieces = chunks(base) + [c for c in chunks(fighter) if not c.startswith('func Tether_Create(')]
    pieces += chunks(constructor_fighter) + chunks(ground) + chunks(lifecycle) + chunks(article)
    def order(s):
        return 0 if s.startswith(('state ', 'attribute ', 'parameter ')) else 1 if s.startswith(('func ', 'export func ')) else 2 if s.startswith('article ') else 3
    return '# Samus interpreter authoring candidate. No independent activation.\n' + '\n'.join(sorted(pieces,key=order))

def inventory(source):
    article = source[source.index('article It_Kind_Samus_GBeam '):]
    article = article[:re.search(r'^(?:article|action|move) ', article[len('article '):], re.M).start()+len('article ')] if re.search(r'^(?:article|action|move) ',article[len('article '):],re.M) else article
    fields = re.findall(r'^    state (int|float|bool) (\w+)(?:\[(\d+)\])?$',article,re.M)
    offset=0
    for kind, _, count in fields:
        width=1 if kind=='bool' else 4
        offset=(offset+width-1)//width*width
        offset+=width*int(count or 1)
    return {'article_fields':len(fields),'article_state_bytes':offset,
        'article_helpers':len(re.findall(r'^    (?:export )?func ',article,re.M)),
        'inputs':{str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in INPUTS}}

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('output',type=Path)
    p.add_argument('--independent',action='store_true',help='Write the full independent Workshop template')
    p.add_argument('--check-template',action='store_true',help='Check the shipped independent template against its maintained components')
    args=p.parse_args();source=compose()
    if args.independent or args.check_template:
        source = ('# Independent Samus Workshop template.\n'
                  '# Maintained components: tests/native/samus_grapple_constructor/compose.py.\n'
                  '# Fighter decisions, four articles, grapple resources and lifecycle are script-owned.\n'
                  'fighter:\n    behavior independent\n\n' + source.split('\n',1)[1])
    if args.check_template:
        if args.output.read_text(encoding='utf-8') != source:
            p.error('Workshop template differs from its maintained source components; regenerate with --independent')
    else:
        args.output.write_text(source,encoding='utf-8',newline='\n')
    print(json.dumps(inventory(source),indent=2))
