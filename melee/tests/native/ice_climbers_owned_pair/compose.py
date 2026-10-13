"""Assemble unregistered full motion programs; accepted component files stay read-only."""
import importlib.util,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
LINKED=HERE.parent/'ice_climbers_linked'
ROPE=HERE.parent/'ice_climbers_rope'
ACCEPTED=HERE.parent/'ice_climbers_composition'

def source(file):
    return file.read_text(encoding='utf-8')

def compose(member):
    if member not in ('popo','nana'):raise ValueError('Unknown member')
    # Replace the accepted partial death hook with the complete owned lifecycle.
    fighter=re.sub(r'(?m)^    on death:\n(?:        .*\n|\n)*','',source(ACCEPTED/'fighter.fms'))
    # The original Ice/Blizzard notification is full Init_8011F060. Resolve
    # the accepted standalone cleanup subset directly to the full owned policy.
    fighter=re.sub(r'(?m)^func cleanup_standalone\(\):\n(?:    .*\n|\n)*','',fighter)
    fighter=fighter.replace('cleanup_standalone','linked_full_cleanup')
    pieces=[fighter,source(LINKED/'solo.fms')]
    pieces += [source(LINKED/(name+'.fms')) for name in ('paired_exports','paired_hitlag','primary_cleanup','primary_squall')]
    pieces += [source(ROPE/'primary.fms'),source(HERE/'squall_collision.fms')]
    # Both actual tables bind the same primary and secondary phases. The old
    # standalone programs had distinct same-named fall tuning; give the secondary
    # field its own meaningful name when both policies share one program.
    pieces += [source(LINKED/'secondary_squall.fms').replace('linked_squall_fall_lag','linked_secondary_squall_fall_lag'),source(LINKED/'secondary_belay.fms')]
    lifecycle=source(HERE/'lifecycle.fms').replace('MEMBER_SPAWN','primary_spawn_offset' if member=='popo' else 'secondary_spawn_offset')
    if member=='nana':lifecycle=re.sub(r'(?m)^    on death:\n(?:        .*\n|\n)*','',lifecycle)
    pieces += [lifecycle]
    if member=='nana':pieces.append(source(HERE/'secondary_death.fms'))
    pieces += [source(ACCEPTED/'ice.fms'),source(ACCEPTED/'blizzard.fms'),source(ROPE/'rope.fms')]
    text='\n'.join(pieces)
    text=re.sub(r'(?m)^base .+\n','',text)
    states=re.findall(r'(?m)^state (?:int|float|bool) .+$',text)
    if len(states)!=len(set(states)):raise ValueError('Duplicate fighter state declarations')
    text=re.sub(r'(?m)^state (?:int|float|bool) .+\n','',text)
    if member=='nana':
        # Preserve actual NULL special entry hooks. Primary phases remain owned.
        for hook in ('special_s','special_air_s','special_hi','special_air_hi'):
            text=re.sub(r'(?m)^    on '+hook+r':\n(?:        .*\n|\n)*','',text)
    return '# Unregistered full motion candidate; load flags, voice and follower foundation pending.\nbase ftData'+('Popo' if member=='popo' else 'Nana')+' 341 26\n'+'\n'.join(states)+'\n'+text

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('member',choices=('popo','nana'));p.add_argument('output',type=Path);x=p.parse_args()
    x.output.write_text(compose(x.member),encoding='utf-8')
