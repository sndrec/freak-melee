"""Sparse editable poses and duration-owned events for Glim's custom moves."""

import math
import struct

from fighter_animation_io import Dat, figatree, joints, packed


def track(channel, points):
    stream = bytearray()
    for i, (frame, value) in enumerate(points):
        wait = points[i + 1][0] - frame if i + 1 < len(points) else 0
        stream += bytes([2]) + struct.pack("<f", value) + packed(wait)
    return dict(
        channel=channel, start=0, value_format=0, slope_format=0, stream=bytes(stream)
    )


def pose_clip(symbol, rig, duration, poses):
    # Required engine helper joints retain world position and facing. Deform
    # joints receive rest channels plus the explicitly authored pose deltas.
    nodes = []
    channels = (1, 2, 3, 5, 6, 7, 8, 9, 10)
    for index, joint in enumerate(rig):
        if index < 4:
            nodes.append([])
            continue
        baseline = (*joint["rotation"], *joint["translation"], *joint["scale"])
        node = []
        for channel, value in zip(channels, baseline):
            points = poses.get((index, channel))
            values = (
                [
                    (frame, value + delta if channel < 8 else value * delta)
                    for frame, delta in points
                ]
                if points
                else [(0, value), (duration, value)]
            )
            node.append(track(channel, values))
        nodes.append(node)
    return figatree(symbol, nodes, duration)


def hitbox(
    start,
    end,
    damage,
    radius,
    angle,
    growth=95,
    base=25,
    slot=0,
    bone=4,
    group=0,
    refresh=False,
    element=0,
):
    header = (59 << 26) | (slot << 23) | (group << 20) | (bone << 11) | damage
    words = [
        header,
        round(radius / 0.003906) << 16,
        0,
        (angle << 23) | (growth << 14) | 2,
        (base << 23) | (element << 18) | (1 << 7) | (1 << 2) | 3,
        ((end - start) * 256) | (1 << 30) | (int(refresh) << 31),
    ]
    return start, words


def install_events(fighter, root, action, events, interrupt=None):
    if interrupt is not None:
        events = [*events, (interrupt, [23 << 26])]
    words = []
    frame = 0
    for when, command in sorted(events, key=lambda event: event[0]):
        if when > frame:
            words.append((1 << 26) | (when - frame))
        words.extend(command)
        frame = when
    words.append(0)
    code = struct.pack(f">{len(words)}I", *words)
    table = fighter.u32(root + 12)
    # Command graphs precede the action table. Retarget every actual DAT
    # relocation and symbol after insertion; raw payload words stay unchanged.
    original = bytes(fighter.data)
    fighter.data[table:table] = code
    old_relocations = list(fighter.relocs)
    fighter.relocs.clear()
    for slot in old_relocations:
        target = struct.unpack_from(">I", original, slot)[0]
        moved_slot = slot + len(code) if slot >= table else slot
        moved_target = target + len(code) if target >= table else target
        fighter.pointer(moved_slot, moved_target)
    symbols = bytearray(fighter.symbols)
    for i in range(fighter.root_count + fighter.ref_count):
        offset = struct.unpack_from(">I", symbols, i * 8)[0]
        if offset >= table:
            struct.pack_into(">I", symbols, i * 8, offset + len(code))
    fighter.symbols = symbols
    fighter.pointer(table + len(code) + action * 24 + 12, table)
    return root + len(code) if root >= table else root


def author_animations(project, info):
    fighter = Dat((project / "fighter.dat").read_bytes())
    root = fighter.roots[info["root"]]
    bank = bytearray((project / "animation.dat").read_bytes())
    costume = Dat((project / "costume.dat").read_bytes())
    rig = joints(costume, costume.roots[info["joint"]])
    table = fighter.u32(root + 12)
    symbols = {
        fighter.string(fighter.u32(table + i * 24)): i
        for i in range(info["actions"])
        if fighter.u32(table + i * 24)
    }

    def action_name(name):
        return next(
            index
            for symbol, index in symbols.items()
            if f"_ACTION_{name}_figatree" in symbol
        )

    def motion_action(motion):
        return info["motions"][motion - info["first"]][0]

    def author(action, duration, poses, events=(), interrupt=None, name=None):
        nonlocal root
        at = fighter.u32(root + 12) + action * 24
        symbol = fighter.string(fighter.u32(at))
        if name:
            symbol = f"PlyGlim_Share_ACTION_{name}_figatree"
            fighter.pointer(at, fighter.append(symbol.encode() + b"\0"))
        clip = pose_clip(symbol, rig, duration, poses)
        bank.extend(b"\xff" * (-len(bank) % 32))
        fighter.word(at + 4, len(bank))
        fighter.word(at + 8, len(clip))
        bank.extend(clip)
        if "_ACTION_AttackAir" in symbol:
            last_hit = max(
                (start + (words[5] & 0x1FFFFFFF) / 256 for start, words in events),
                default=10,
            )
            events = [*events, (3, [(19 << 26) | 1]), (int(last_hit + 4), [19 << 26])]
        root = install_events(fighter, root, action, events, interrupt)

    # The halo is attached to HeadN; a small head tilt makes it readable even
    # when the body compresses. Angles are radians, scales are multipliers.
    breathe = {
        (4, 9): [(0, 1), (20, 1.035), (40, 1)],
        (6, 3): [(0, -0.05), (20, 0.05), (40, -0.05)],
    }
    author(action_name("Wait"), 40, breathe)
    charge = {
        (4, 8): [(0, 1), (6, 0.92), (12, 0.85)],
        (4, 9): [(0, 1), (6, 1.1), (12, 1.18)],
        (6, 3): [(0, 0), (6, -0.08), (12, 0.08)],
    }
    burst = {
        (4, 8): [(0, 0.9), (4, 1.22), (9, 1.05), (22, 1)],
        (4, 9): [(0, 1.1), (4, 0.85), (9, 1), (22, 1)],
        (27, 3): [(0, 0.1), (4, -0.7), (10, -0.25), (22, 0)],
        (31, 3): [(0, -0.1), (4, 0.7), (10, 0.25), (22, 0)],
    }
    for motion in (346, 354):
        author(
            motion_action(motion),
            48,
            charge,
            name=f"LanternCharge{'Air' if motion == 354 else 'Ground'}",
        )
    author(
        motion_action(348),
        22,
        burst,
        [hitbox(4, 10, 5, 7.5, 55, 105, 28, element=2)],
        name="LanternFlash",
    )
    skip = {
        (4, 3): [(0, 0), (4, -0.35), (8, -0.5), (15, -0.35), (28, 0)],
        (4, 8): [(0, 1), (5, 1.3), (14, 1.2), (28, 1)],
        (4, 9): [(0, 1), (5, 0.8), (14, 0.9), (28, 1)],
    }
    for motion in (363, 364):
        author(
            motion_action(motion),
            28,
            skip,
            [hitbox(5, 15, 7, 5, 60, 80, 25)],
            name=f"FireflySkip{'Air' if motion == 364 else 'Ground'}",
        )
    up = {
        (4, 8): [(0, 1), (3, 1.12), (5, 0.85), (12, 1), (36, 1)],
        (4, 9): [(0, 1), (3, 0.82), (5, 1.25), (12, 1), (36, 1)],
        (6, 3): [(0, 0), (8, 0.2), (18, -0.2), (28, 0.1), (36, 0)],
    }
    for motion in (365, 366):
        author(
            motion_action(motion),
            36,
            up,
            [hitbox(4, 11, 6, 5.5, 85, 85, 30)],
            name=f"Updraft{'Air' if motion == 366 else 'Ground'}",
        )
    ward = {
        (4, 8): [(0, 1), (3, 1.1), (13, 1.1), (22, 1)],
        (4, 9): [(0, 1), (3, 0.9), (13, 0.9), (22, 1)],
        (27, 3): [(0, 0), (3, -0.5), (13, -0.5), (22, 0)],
        (31, 3): [(0, 0), (3, 0.5), (13, 0.5), (22, 0)],
    }
    for motion in (369, 370):
        author(
            motion_action(motion),
            22,
            ward,
            name=f"WickWard{'Air' if motion == 370 else 'Ground'}",
        )
    author(
        action_name("AttackAirN"),
        38,
        {(4, 3): [(0, 0), (6, 0), (14, math.pi), (24, math.tau), (38, math.tau)]},
        [hitbox(6, 16, 6, 5.2, 65), hitbox(16, 26, 3, 5.2, 65)],
        interrupt=30,
    )
    author(
        action_name("AttackAirF"),
        35,
        {
            (4, 3): [(0, 0), (7, 0.3), (11, -0.3), (20, -0.15), (35, 0)],
            (27, 3): [(0, 0), (8, -0.2), (11, -1.1), (20, -0.5), (35, 0)],
        },
        [hitbox(10, 16, 9, 4.5, 45, 100, 25, bone=30)],
        interrupt=27,
    )
    author(
        action_name("AttackAirLw"),
        42,
        {
            (4, 8): [(0, 1), (8, 1.18), (12, 0.9), (25, 1), (42, 1)],
            (4, 9): [(0, 1), (8, 0.8), (12, 1.25), (25, 1), (42, 1)],
        },
        [hitbox(10, 14, 10, 5, 270, 85, 15), hitbox(14, 20, 5, 5, 65, 80, 20)],
        interrupt=32,
    )
    author(
        action_name("AttackS4"),
        49,
        {
            (4, 3): [(0, 0), (9, 0.3), (13, -0.5), (20, -0.2), (49, 0)],
            (4, 8): [(0, 1), (9, 0.9), (13, 1.2), (20, 1), (49, 1)],
        },
        [hitbox(13, 18, 14, 6.5, 50, 103, 25, element=2)],
        interrupt=38,
    )
    (project / "animation.dat").write_bytes(bank)
    (project / "fighter.dat").write_bytes(fighter.bytes())
