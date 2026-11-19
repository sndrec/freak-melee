"""Offline DAT/figatree editing. No game or Blender dependency."""
import math
import struct


class Dat:
    def __init__(self, raw):
        size, data_size, reloc_count, root_count, ref_count = struct.unpack_from('>5I', raw)
        if size != len(raw):
            raise ValueError('DAT size mismatch')
        self.header = raw[:32]
        self.data = bytearray(raw[32:32 + data_size])
        p = 32 + data_size
        self.relocs = list(struct.unpack_from(f'>{reloc_count}I', raw, p))
        p += 4 * reloc_count
        self.symbols = raw[p:]
        self.root_count = root_count
        self.ref_count = ref_count
        strings = 8 * (root_count + ref_count)
        self.roots = {}
        for i in range(root_count):
            offset, name = struct.unpack_from('>II', self.symbols, 8 * i)
            start = strings + name
            self.roots[self.symbols[start:self.symbols.index(0, start)].decode()] = offset

    def u32(self, offset):
        return struct.unpack_from('>I', self.data, offset)[0]

    def word(self, offset, value):
        struct.pack_into('>I', self.data, offset, value)

    def pointer(self, offset, target):
        self.word(offset, target)
        if target and offset not in self.relocs:
            self.relocs.append(offset)

    def append(self, raw):
        self.data += b'\0' * (-len(self.data) % 4)
        offset = len(self.data)
        self.data += raw
        return offset

    def bytes(self):
        self.data += b'\0' * (-len(self.data) % 4)
        relocs = sorted(set(self.relocs))
        size = 32 + len(self.data) + 4 * len(relocs) + len(self.symbols)
        return (struct.pack('>5I', size, len(self.data), len(relocs),
                            self.root_count, self.ref_count) + self.header[20:32] +
                self.data + struct.pack(f'>{len(relocs)}I', *relocs) + self.symbols)

    def string(self, offset):
        return self.data[offset:self.data.index(0, offset)].decode()


def disc_file(path, name):
    with open(path, 'rb') as f:
        f.seek(0x424)
        offset, size = struct.unpack('>II', f.read(8))
        f.seek(offset)
        fst = f.read(size)
        count = struct.unpack_from('>I', fst, 8)[0]
        strings = count * 12
        for i in range(1, count):
            label, offset, size = struct.unpack_from('>III', fst, i * 12)
            if label >> 24:
                continue
            start = strings + label
            if fst[start:fst.index(0, start)].decode() == name:
                f.seek(offset)
                return f.read(size)
    raise ValueError(f'Disc file missing: {name}')


def animation_bank(raw):
    clips = {}
    p = 0
    while p < len(raw):
        if raw[p] == 255:
            p += 1
            continue
        size = struct.unpack_from('>I', raw, p)[0]
        if size < 32 or p + size > len(raw):
            raise ValueError(f'Invalid animation member at {p}')
        member = raw[p:p + size]
        dat = Dat(member)
        if len(dat.roots) != 1:
            raise ValueError('Expected one figatree per member')
        name = next(iter(dat.roots))
        if name in clips:
            raise ValueError(f'Duplicate clip {name}')
        clips[name] = member
        p += size
    return clips


def joints(dat, root):
    result = []
    def walk(offset, parent=-1):
        while offset:
            index = len(result)
            if any(n['offset'] == offset for n in result):
                raise ValueError('Instanced/cyclic skeleton needs explicit mapping')
            values = struct.unpack_from('>9f', dat.data, offset + 20)
            result.append(dict(offset=offset, parent=parent,
                               rotation=values[:3], scale=values[3:6], translation=values[6:]))
            walk(dat.u32(offset + 8), index)
            offset = dat.u32(offset + 12)
    walk(root)
    return result


def packed(value):
    result = bytearray()
    while value >= 128:
        result.append((value & 127) | 128)
        value >>= 7
    result.append(value)
    return result


def float_track(channel, values):
    if not values or not all(math.isfinite(v) for v in values):
        raise ValueError('Invalid animation samples')
    # One-key packets avoid the separate packet-count varint encoding.
    constant = max(values) - min(values) < 1e-7
    stream = bytearray()
    if constant:
        # A lone linear key leaves HSD without an interpolation segment.
        # Give constant tracks two identical endpoints, including one-sample poses.
        keys = [(values[0], max(1, len(values) - 1)), (values[-1], 0)]
    else:
        keys = [(value, 1 if i + 1 < len(values) else 0)
                for i, value in enumerate(values)]
    for value, wait in keys:
        stream += bytes([2]) + struct.pack('<f', value) + packed(wait)
    return dict(channel=channel, start=0, value_format=0, slope_format=0, stream=bytes(stream))


def figatree(name, nodes, frames, tree_type=1, flags=0, share_streams=False):
    data = bytearray(struct.pack('>IIfII', tree_type, flags, frames, 20, 0))
    data += bytes(len(n) for n in nodes) + b'\xff'
    data += b'\0' * (-len(data) % 4)
    tracks = len(data)
    struct.pack_into('>I', data, 16, tracks)
    data += b'\0' * (12 * sum(map(len, nodes)))
    reloc = [12, 16]
    streams = {}
    i = 0
    for node in nodes:
        for track in node:
            stream = track['stream']
            if len(stream) > 65535:
                raise ValueError('Track too large')
            offset = tracks + i * 12
            stream_offset = streams.get(stream) if share_streams else None
            if stream_offset is None:
                stream_offset = len(data)
                data += stream
                if share_streams:
                    streams[stream] = stream_offset
            struct.pack_into('>HHBBBBI', data, offset, len(stream), track['start'],
                             track['channel'], track['value_format'], track['slope_format'], 0, stream_offset)
            reloc.append(offset + 8)
            i += 1
    data += b'\0' * (-len(data) % 4)
    tail = struct.pack(f'>{len(reloc)}I', *reloc) + struct.pack('>II', 0, 0) + name.encode() + b'\0'
    size = 32 + len(data) + len(tail)
    return struct.pack('>5I12x', size, len(data), len(reloc), 1, 0) + data + tail


def read_figatree(raw):
    dat = Dat(raw)
    root = next(iter(dat.roots.values()))
    frames = struct.unpack_from('>f', dat.data, root + 8)[0]
    counts = dat.u32(root + 12)
    track = dat.u32(root + 16)
    nodes = []
    while dat.data[counts] != 255:
        node = []
        for _ in range(dat.data[counts]):
            length, start, channel, vf, sf, _, stream = struct.unpack_from('>HHBBBBI', dat.data, track)
            node.append(dict(channel=channel, start=start, value_format=vf, slope_format=sf,
                             stream=bytes(dat.data[stream:stream+length])))
            track += 12
        nodes.append(node)
        counts += 1
    return frames, nodes


def rebuild_bank(fighter, root, clips, action_count, demo_count):
    bank = bytearray()
    locations = {}
    for name, clip in clips.items():
        locations[name] = (len(bank), len(clip))
        bank += clip
        bank += b'\xff' * (-len(bank) % 32)
    for table, count in [(fighter.u32(root + 12), action_count),
                         (fighter.u32(root + 20), demo_count)]:
        for i in range(count):
            at = table + 24*i
            if not fighter.u32(at + 8):
                continue
            name = fighter.string(fighter.u32(at))
            # Demo entries can refer to a separate victory/ending bank.
            if name not in locations and table == fighter.u32(root + 20):
                continue
            offset, size = locations[name]
            fighter.word(at + 4, offset)
            fighter.word(at + 8, size)
    return bytes(bank)
