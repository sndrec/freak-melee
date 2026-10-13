"""Unchanged Peach constructor/caller bodies; run against a pinned SDK root."""
from pathlib import Path
import argparse, hashlib, json, re, struct

def body(text, name):
    m = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not m: raise ValueError(name)
    i, depth = m.end(), 1
    while depth:
        depth += (text[i] == '{') - (text[i] == '}'); i += 1
    return text[m.start():i]

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root', required=True, type=Path)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--parameters', type=Path)
    p.add_argument('--generated', type=Path)
    a = p.parse_args(); a.output.mkdir(parents=True, exist_ok=True)
    sources = {
        'src/melee/it/kinds/inlines.h': ['Item_InitSpawn'],
        'src/melee/it/kinds/itpeachturnip.c': ['it_802BD32C', 'it_802BD4AC', 'itPeachTurnip_Logic56_PickedUp', 'itPeachTurnip_Logic56_Destroyed'],
        'src/melee/ft/kinds/ftPeach/ftpeachspeciallw.c': ['pickVeg', 'getVeg', 'setupVeg', 'spawnVeg', 'ftPe_SpecialLw_UnsetVeg', 'ftPe_SpecialLw_8011CFA0'],
    }
    blocks, receipts = [], {}
    for path, names in sources.items():
        text = (a.root/path).read_text()
        receipts[path] = {'sha256': hashlib.sha256((a.root/path).read_bytes()).hexdigest(), 'bodies': {}}
        for name in names:
            b = body(text, name)
            receipts[path]['bodies'][name] = {'line': text[:text.index(b)].count('\n')+1, 'sha256': hashlib.sha256(b.encode()).hexdigest()}
            blocks.append('// Actual '+path+'\n'+b)
    path = 'src/melee/ft/inlines.h'
    text = (a.root/path).read_text()
    receipts[path] = {'sha256':hashlib.sha256((a.root/path).read_bytes()).hexdigest(),
        'compiled_header_body':'ftCheckThrowB0', 'body_sha256':hashlib.sha256(body(text,'ftCheckThrowB0').encode()).hexdigest()}
    (a.output/'peach_composition_actual.inc').write_text('\n\n'.join(blocks)+'\n')
    (a.output/'source.json').write_text(json.dumps({'body_count':len(blocks), 'sources':receipts,
       'scope':'actual source constructor/caller composition, explicit engine allocation/attachment/scale boundaries; no game'}, indent=2)+'\n')
    if a.parameters or a.generated:
        if not a.parameters or not a.generated: p.error('parameters and generated are required together')
        values=[]
        for line in a.parameters.read_text().splitlines():
            kind, name, equals, value=line.split()
            assert equals=='='
            values.append(struct.unpack('<I',struct.pack('<f',float(value)))[0]
                if kind=='float' else int(value)&0xffffffff)
        schema=re.search(r'schema != (0x[0-9a-f]+)ULL',a.generated.read_text())[1]
        assert len(values)==28
        (a.output/'parameters.inc').write_text('static const uint32_t fixture_parameter_bits[] = {'+
            ','.join(hex(v)+'u' for v in values)+'};\nstatic const uint64_t fixture_parameter_schema = '+schema+'ULL;\n')

if __name__ == '__main__': main()
