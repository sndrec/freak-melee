"""Build the private envelope graph used by the live weighted-morph review."""
import struct


def weighted_shape(asset, joints, mesh):
    size, count = struct.unpack_from('>II', asset, 4)
    data = bytearray(asset[32:32+size])
    links = set(struct.unpack_from(f'>{count}I', asset, 32+size))
    tail = asset[32+size+count*4:]
    word = lambda at: struct.unpack_from('>I', data, at)[0]

    def allocate(count):
        data.extend(b'\0'*(-len(data)%4))
        at = len(data)
        data.extend(b'\0'*count)
        return at

    def link(slot, target):
        struct.pack_into('>I', data, slot, target)
        links.add(slot)

    polygon = word(mesh+12)
    old_shape = word(polygon+20)
    shape = allocate(32)
    data[shape:shape+28] = data[old_shape:old_shape+28]
    flags = struct.unpack_from('>H', data, shape)[0]
    struct.pack_into('>H', data, shape, flags | 0x8000)
    for member in (8, 12, 20, 24):
        link(shape+member, word(old_shape+member))
    link(polygon+20, shape)

    root = int(joints[0][2], 16)
    deform = next(int(address, 16) for _, _, address in joints[1:]
                  if word(int(address, 16)+4) & 1 and word(int(address, 16)+56))
    # Deliberately pose this bone away from its saved inverse bind. Distinct
    # per-vertex weights must deform the drawn quad and its picking surface.
    angle = struct.unpack_from('>f', data, deform+28)[0]
    struct.pack_into('>f', data, deform+28, angle+.35)
    if not word(root+56):
        bind = allocate(48)
        struct.pack_into('>12f', data, bind, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0)
        link(root+56, bind)
    table = allocate(20)
    for vertex in range(4):
        weights = allocate(24)
        link(table+vertex*4, weights)
        link(weights, root)
        link(weights+8, deform)
        influence = .2+vertex*.15
        struct.pack_into('>f', data, weights+4, 1-influence)
        struct.pack_into('>f', data, weights+12, influence)

    source_attrs = word(polygon+8)
    assert [word(source_attrs+i*24) for i in range(3)] == [9, 10, 255]
    attributes = allocate(96)
    struct.pack_into('>III', data, attributes, 0, 1, 0)  # Direct PNMTX index.
    for i in range(2):
        at = attributes+(i+1)*24
        data[at:at+24] = data[source_attrs+i*24:source_attrs+(i+1)*24]
        link(at+20, word(source_attrs+i*24+20))
    struct.pack_into('>I', data, attributes+72, 255)

    # Parent fixture is one indexed triangle list with matching position/normal
    # indices. Preserve its corner order; select the palette by logical vertex.
    original = word(polygon+16)
    assert data[original] == 0x90
    corners = struct.unpack_from('>H', data, original+1)[0]
    commands = bytearray(data[original:original+3])
    for i in range(corners):
        corner = data[original+3+i*4:original+7+i*4]
        vertex, normal = struct.unpack('>HH', corner)
        assert vertex < 4 and normal == vertex
        commands.append(vertex*3)
        commands.extend(corner)
    commands.extend(b'\0'*(-len(commands)%32))
    display = allocate(len(commands))
    data[display:display+len(commands)] = commands
    batch = allocate(24)
    link(batch+8, attributes)
    struct.pack_into('>HH', data, batch+12, 0x2000, len(commands)//32)
    link(batch+16, display)
    link(batch+20, table)
    link(shape+28, batch)
    result = bytearray(asset[:32])+data+struct.pack(f'>{len(links)}I', *sorted(links))+tail
    struct.pack_into('>III', result, 0, len(result), len(data), len(links))
    return result
