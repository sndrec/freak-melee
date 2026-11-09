#!/usr/bin/env python3
"""Compare two Freak Melee playtest reports at their first shared confirmed split."""

import argparse
from collections import defaultdict
from pathlib import Path
import struct


FIGHTER_GROUPS = (
    "movement", "input", "collision", "animation", "hitboxes", "damage",
    "script", "attachments", "pose", "relations",
)


def read_report(path):
    frames = defaultdict(dict)
    columns = {"hurtbox-count": ["count"]}
    session = None
    peer = None
    for line in path.read_text(encoding="utf-8").splitlines():
        label, *values = line.split() or [""]
        if label.startswith("session="):
            session, peer = label[8:], values[0].removeprefix("peer=")
        elif label in {"[hitbox-columns]", "[hurtbox-columns]"}:
            columns[label[1:-9]] = values[3:]
        elif label in {"[digest]", "[tick]", "[inputs]", "[consumed]", "[state-groups]"}:
            frames[int(values[0])][label] = values[1:]
        elif label == "[fighter-state]" and values[0] != "columns:":
            frame, actor = map(int, values[:2])
            frames[frame].setdefault("fighters", {})[actor] = values[2:]
        elif label in {"[hitbox]", "[hurtbox]", "[hurtbox-count]"}:
            frame, actor, slot = map(int, values[:3])
            kind = label[1:-1]
            fields = columns[kind]
            if len(fields) != len(values[3:]):
                raise ValueError(f"{path}: malformed {kind} row at frame {frame}")
            frames[frame].setdefault("capsules", {})[actor, kind, slot] = dict(
                zip(fields, values[3:])
            )
    if session is None or peer is None:
        raise ValueError(f"{path}: missing report session or peer")
    return session, peer, frames


def describe_word(field, word):
    value = int(word, 16)
    if field.endswith(":f32"):
        number, = struct.unpack("<f", value.to_bytes(4, "little"))
        return f"{number:.9g} (0x{value:08x})"
    return f"{value} (0x{value:x})"


def compare(left, right):
    session, local_peer, local = left
    other_session, remote_peer, remote = right
    if session != other_session or local_peer == remote_peer:
        raise ValueError("Use two distinct peers from the same match.")
    print(f"Session {session}, peer {local_peer} vs peer {remote_peer}")
    shared = sorted(local.keys() & remote.keys())
    confirmed = [
        frame for frame in shared
        if all(history[frame].get("[inputs]", [""])[0] == "confirmed=f"
               and "[digest]" in history[frame] for history in (local, remote))
    ]
    split = next((frame for frame in confirmed
                  if local[frame]["[digest]"] != remote[frame]["[digest]"]), None)
    if split is None:
        print(f"No checksum mismatch across {len(confirmed)} shared confirmed frames.")
        return
    last_agreed = max((frame for frame in confirmed if frame < split), default=None)
    print(f"First shared confirmed split: frame {split}; last shared agreement: {last_agreed}")
    a, b = local[split], remote[split]
    print("Checksums:", *a["[digest]"], "vs", *b["[digest]"])
    print("Confirmed inputs:", "equal" if a["[inputs]"] == b["[inputs]"] else "differ")
    for frame, peer in ((a, local_peer), (b, remote_peer)):
        if "[tick]" in frame:
            print(f"Peer {peer} tick:", *frame["[tick]"])
        if "[consumed]" in frame:
            print(f"Peer {peer} consumed:", *frame["[consumed]"])
    if "[state-groups]" in a and "[state-groups]" in b:
        for av, bv in zip(a["[state-groups]"], b["[state-groups]"]):
            if av != bv:
                print(f"First differing cumulative state group: {av} vs {bv}")
                break
    fighters_a, fighters_b = a.get("fighters", {}), b.get("fighters", {})
    if fighters_a.keys() != fighters_b.keys():
        print(f"Fighter rows differ: {sorted(fighters_a)} vs {sorted(fighters_b)}")
    for actor in sorted(fighters_a.keys() & fighters_b.keys()):
        af, bf = fighters_a[actor], fighters_b[actor]
        if af == bf:
            continue
        if af[:3] != bf[:3]:
            print(f"Actor {actor} port/kind/action: {'/'.join(af[:3])} vs {'/'.join(bf[:3])}")
        group = next((label for label, av, bv in zip(FIGHTER_GROUPS, af[3:], bf[3:])
                      if av != bv), "none")
        print(f"Actor {actor}, port {af[0]}, kind {af[1]}, action {af[2]}:"
              f" first differing cumulative fighter group = {group}")
        ac, bc = a.get("capsules", {}), b.get("capsules", {})
        common = sorted(key for key in ac.keys() & bc.keys() if key[0] == actor)
        differences = 0
        for key in common:
            _, kind, slot = key
            for field, av in ac[key].items():
                bv = bc[key].get(field)
                if bv is None or av == bv:
                    continue
                differences += 1
                name = field.removesuffix(":f32")
                print(f"  {kind} {slot} {name}: {describe_word(field, av)}"
                      f" vs {describe_word(field, bv)}")
        if group == "hitboxes" and differences == 0:
            status = "no field differences in shared rows" if common else "no shared rows"
            print(f"  Capsule detail: {status}.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path, nargs=2)
    args = parser.parse_args()
    try:
        compare(*(read_report(path) for path in args.report))
    except (OSError, ValueError, KeyError, IndexError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
