"""Extract actual Parasol phase bodies from the immutable source SDK."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

def body(text, name):
    m = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{', text)
    if not m: raise ValueError(name)
    # Alternate preprocessor switch branches contain two textual opening braces.
    # Stock definitions terminate at the first column-zero closing brace.
    end = re.search(r'(?m)^}', text[m.end():])
    if not end: raise ValueError(name + ' definition end')
    return text[m.start():m.end()+end.end()]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    for n in ('sdk', 'output', 'generated', 'parameters', 'dat'):
        p.add_argument('--'+n, type=Path, required=True)
    a = p.parse_args(); blocks = []; sources = {}
    names = ['ftPe_SpecialHi_8011D424', 'ftPe_8011D518', 'ftPe_8011D598', 'ensureUnkItem',
             'ftPe_SpecialHi_8011D620', 'ftPe_SpecialHi_8011D650',
             'notUsingParasol', 'ftPe_SpecialHi_NotActive', 'doEnter',
             'ftPe_SpecialHi_Enter', 'ftPe_SpecialAirHi_Enter', 'checkCmdVar2',
             'ftPe_SpecialHi_8011DD8C', 'doColl', 'ftPe_SpecialHi_8011E064']
    for prefix in ('SpecialHiStart', 'SpecialAirHiStart', 'SpecialHiEnd', 'SpecialAirHiEnd'):
        names += ['ftPe_'+prefix+'_'+suffix for suffix in ('Anim', 'IASA', 'Phys', 'Coll')]
    groups = {
        'src/melee/ft/kinds/ftPeach/ftpeachspecialhi.c': names,
        'src/melee/it/kinds/itpeachparasol.c': ['it_802BDA40', 'itPeachParasol_Logic60_Destroyed', 'it_802BDB94', 'it_802BDBF8', 'it_802BDC18', 'it_802BDC38', 'itPeachParasol_Logic60_PickedUp', 'itPeachparasol_UnkMotion2_Anim', 'it_802BDD3C', 'it_802BDD40', 'it_802BDDB4', 'itPeachParasol_Logic60_EvtUnk'],
        'src/melee/ft/ftmovement.c': ['ftCommon_Fall'],
    }
    for path, functions in groups.items():
        text = (a.sdk/path).read_text(); entries = {}
        for name in functions:
            b = body(text, name); blocks.append(b)
            entries[name] = {'line': text[:text.index(b)].count('\n')+1,
                             'sha256': hashlib.sha256(b.encode()).hexdigest()}
        sources[path] = {'sha256': hashlib.sha256((a.sdk/path).read_bytes()).hexdigest(), 'bodies': entries}
    # Preserve the actual state table used by the literal animation length helper.
    text = (a.sdk/'src/melee/it/kinds/itpeachparasol.c').read_text()
    table = text[text.index('ItemStateTable it_803F74F8'):]
    table = table[:table.index('};')+2]
    (a.output/'actual.inc').write_text(table+'\n\n'+'\n\n'.join(blocks)+'\n', newline='\n')
    original_path = Path(__file__).resolve().parent.parent/'held_lifecycle_protocol/original54.inc'
    original = original_path.read_text()
    manifest_path = original_path.with_name('original54_manifest.json')
    manifest = json.loads(manifest_path.read_text())
    assert hashlib.sha256(original_path.read_bytes()).hexdigest() == manifest['original_inc_sha256']
    entries = []
    for name in ('ft_8008A348', 'fn_800D2890', 'ftCo_Landing_Enter'):
        entries.append(re.sub(r'\b'+name+r'\b', 'original_'+name, body(original,name)))
    for path,name in [('src/melee/ft/ft_08A1.c','ft_8008A348'),('src/melee/ft/ft_0D27.c','ftCo_Mushroom_SelectAirResume'),('src/melee/ft/kinds/ftCommon/ftCo_Landing.c','ftCo_Landing_Enter')]:
        raw=(a.sdk/path).read_bytes();b=body(raw.decode(),name);entries.append(b)
        sources[path]={'sha256':hashlib.sha256(raw).hexdigest(),'scope':'complete actual entry','body_sha256':hashlib.sha256(b.encode()).hexdigest()}
    def timer(text):
        start=text.index('        if (fp->x2221_b4) {',text.index('void Fighter_8006A360('));end=text.index('{',start)+1;depth=1
        while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
        return text[start:end]
    for name,text in [('original_timer',original),('candidate_timer',(a.sdk/'src/melee/ft/fighter.c').read_text())]:
        entries.append('static void '+name+'(Fighter_GObj* gobj) { Fighter* fp=gobj->user_data; uint32_t callback_generation=(uint32_t)fp->x8_spawnNum;\n'+timer(text)+'\n}')
    sources['src/melee/ft/fighter.c']={'sha256':hashlib.sha256((a.sdk/'src/melee/ft/fighter.c').read_bytes()).hexdigest(),'scope':'exact held-timer if-block; surrounding ticking engine excluded'}
    sources[str(original_path)]={'sha256':hashlib.sha256(original_path.read_bytes()).hexdigest(),'scope':'immutable previously accepted original54 entry/timer source; manifest validated'}
    (a.output/'entries.inc').write_text('\n\n'.join(entries)+'\n',newline='\n')
    gen = a.generated.read_text(); values = []
    for line in a.parameters.read_text().splitlines():
        typ, name, eq, value = line.split(); assert eq == '='
        values.append(struct.unpack('<I', struct.pack('<f', float(value)))[0] if typ == 'float' else int(value,0)&0xffffffff)
    count, schema = re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL',gen).groups()
    assert int(count) == len(values)
    (a.output/'parameters.inc').write_text('static const uint32_t parameter_bits[]={'+','.join('UINT32_C(%d)'%v for v in values)+'};\nstatic const uint64_t parameter_schema='+schema+'ULL;\n', newline='\n')
    hosts = sorted(set(re.findall(r'host->(\w+) == 0', gen)))
    (a.output/'hosts.inc').write_text('\n'.join('host.'+h+' = (__typeof__(host.'+h+'))unexpected;' for h in hosts)+'\n', newline='\n')
    raw = a.dat.read_bytes(); size,relocs,public,external = struct.unpack_from('>4I',raw,4)
    symbols=32+size+4*relocs; strings=symbols+8*(public+external)
    roots={raw[strings+struct.unpack_from('>I',raw,symbols+8*i+4)[0]:].split(b'\0',1)[0].decode():struct.unpack_from('>I',raw,symbols+8*i)[0] for i in range(public)}
    attr=struct.unpack_from('>I',raw,32+roots['ftDataPeach']+4)[0]
    words=struct.unpack_from('>44I',raw,32+attr)
    (a.output/'attributes.inc').write_text('static const uint32_t dat_attribute_bits[]={'+','.join('UINT32_C(%d)'%v for v in words)+'};\n',newline='\n')
    (a.output/'census.json').write_text(json.dumps({'source_hashes':sources,'required_hosts':hosts,'parameter_count':len(values),'extracted_bodies':sum(map(len,groups.values()))},indent=2)+'\n',newline='\n')

if __name__ == '__main__': main()
