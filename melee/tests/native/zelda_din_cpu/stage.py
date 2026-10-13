"""Retail DAT parameter staging for an unregistered paired-form candidate."""
import re
import struct


def stage(source, dat, top, output):
    word = lambda offset: struct.unpack_from('>I', dat, 32 + offset)[0]
    attributes, articles = word(top + 4), word(top + 72)
    slots = {'It_Kind_Zelda_DinFire': 0, 'It_Kind_Zelda_DinFire_Explode': 1}
    owned, parameters, article = [], [], None
    for line in source.splitlines():
        if line.startswith('article '):
            article = line.split()[1]
        literal = re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)', line)
        imported = re.fullmatch(r'(\s*)attribute (float|int|bool) (\w+) (\d+)', line)
        if not literal and not imported:
            owned.append(line.replace('item.attr.', 'item.params.'))
            continue
        if literal:
            indent, kind, name, value = literal.groups()
            offset = None
            bits = struct.unpack('>I', struct.pack('>f', float(value)))[0] if kind == 'float' else (
                int(value.lower() == 'true') if kind == 'bool' else int(value, 0) & 0xffffffff)
        else:
            indent, kind, name, offset = imported.groups()
            offset = int(offset)
            base = word(word(articles + slots[article] * 4) + 4) if indent else attributes
            bits = dat[32 + base + offset] if kind == 'bool' else word(base + offset)
        value = ('true' if bits else 'false') if kind == 'bool' else repr(struct.unpack(
            '>f' if kind == 'float' else '>i', struct.pack('>I', bits))[0])
        parameters.append({'name': article + '.' + name if indent else name,
                           'type': kind, 'bits': bits, 'value': value,
                           'offset': offset, 'article': article if indent else None})
        owned.append('')
    (output/'owned.fms').write_text('\n'.join(owned).rstrip()+'\n', newline='\n')
    (output/'fighter_parameters.fmp').write_text(''.join(
        f"{p['type']} {p['name']} = {p['value']}\n" for p in parameters), newline='\n')
    return parameters
