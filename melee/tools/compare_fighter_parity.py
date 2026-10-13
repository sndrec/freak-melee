"""Compare stock and clone traces produced with the same --replay-inputs file.

Use --trace-fighter-state --trace-actions for both runs. A cloned fighter's
runtime kind differs by design, so action lines ignore that field only on
declared clone ports. Fully independent ports additionally validate and normalize
the behavior identity. Fighter and item digests remain exact.
"""

import argparse
import re
import struct
from pathlib import Path


FIGHTER = re.compile(r"^\[fighter\] sample (\d+) P(\d+) (.*)$")
ACTION = re.compile(r"^\[action\] sample (\d+) P(\d+) (.*)$")
DAMAGE = re.compile(r"^\[damage\] sample (\d+) P(\d+) (.*)$")
COMPANION = re.compile(r"^\[companion\] sample (\d+) P(\d+) (.*)$")
COMPANION_ACTION = re.compile(r"^\[companion-action\] sample (\d+) P(\d+) (.*)$")
COMPANION_DAMAGE = re.compile(r"^\[companion-damage\] sample (\d+) P(\d+) (.*)$")
BODY_CONTACT = re.compile(r"^\[body-contact\] sample (\d+) P(\d+) (.*)$")
ITEM = re.compile(r"^\[item\] sample (\d+) I(\d+) (.*)$")
RESOURCE = re.compile(r"^\[item-resources?\] sample (\d+) I(\d+) (.*)$")
ITEM_EVENT = re.compile(r"^\[item-event\] sample (\d+) (.*)$")
RNG = re.compile(r"^\[rng\] sample (\d+) (.*)$")
END = re.compile(r"^\[replay\] End after (\d+) controller samples\.")


def read_trace(path, ports, clone_ports, independent_ports=(), cloned=False):
    entries = {"fighter": [], "action": [], "damage": [], "item": [],
               "item_event": [], "rng": [], "body_contact": [], "resource": [],
               "companion": [], "companion_action": [], "companion_damage": []}
    end_sample = None
    missing_item_visual = False
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        for name, pattern in (("fighter", FIGHTER), ("action", ACTION),
                              ("damage", DAMAGE), ("body_contact", BODY_CONTACT),
                              ("companion", COMPANION),
                              ("companion_action", COMPANION_ACTION),
                              ("companion_damage", COMPANION_DAMAGE)):
            match = pattern.match(line)
            if match is None or int(match.group(2)) not in ports:
                continue
            state = match.group(3)
            if name in ("action", "companion_action") and int(match.group(2)) in clone_ports:
                if int(match.group(2)) in independent_ports:
                    identity = re.match(r"^kind=(\d+) behavior=(\d+) ", state)
                    if identity is None:
                        raise ValueError("Independent comparisons require kind and behavior traces")
                    kind, behavior = map(int, identity.groups())
                    if behavior != kind or not (34 <= kind < 128 if cloned else 0 <= kind < 33):
                        raise ValueError(f"Invalid independent fighter identity in {path}: {state}")
                    state = re.sub(r" behavior=\d+ ", " ", state, count=1)
                state = re.sub(r"^kind=\d+ ", "", state)
            entries[name].append((int(match.group(1)), int(match.group(2)),
                                  state))
            break
        item_match = ITEM.match(line)
        if item_match is not None:
            if " visual=" not in item_match.group(3):
                missing_item_visual = True
            entries["item"].append((int(item_match.group(1)),
                                    int(item_match.group(2)),
                                    item_match.group(3)))
        event_match = ITEM_EVENT.match(line)
        if event_match is not None:
            entries["item_event"].append((int(event_match.group(1)),
                                          event_match.group(2)))
        resource_match = RESOURCE.match(line)
        if resource_match is not None:
            entries["resource"].append((int(resource_match.group(1)),
                int(resource_match.group(2)), resource_match.group(3)))
        rng_match = RNG.match(line)
        if rng_match is not None:
            entries["rng"].append((int(rng_match.group(1)), rng_match.group(2)))
        match = END.match(line)
        if match is not None:
            end_sample = int(match.group(1))
    return entries, end_sample, missing_item_visual


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("stock", type=Path)
    parser.add_argument("clone", type=Path)
    parser.add_argument("--ports", type=int, nargs="+", default=[1],
                        help="player ports to compare (default: 1)")
    parser.add_argument("--clone-ports", type=int, nargs="+", default=[1],
                        help="ports whose runtime kind may differ (default: 1)")
    parser.add_argument("--independent-ports", type=int, nargs="+", default=[],
                        help="clone ports with fully authored behavior; verify custom/custom "
                             "identity and normalize only kind/behavior on these ports")
    parser.add_argument("--expect-item-kind", type=int, action="append",
                        default=[], help="require a spawned item kind in both runs")
    parser.add_argument("--expect-item-state", type=int, nargs=5,
                        action="append", default=[], metavar=("KIND", "STATE", "OWNER", "BEGIN", "END"),
                        help="require a KIND item in STATE owned by OWNER during [BEGIN, END)")
    parser.add_argument("--expect-item-direction-change", type=int, nargs=2,
                        action="append", default=[], metavar=("KIND", "OWNER"),
                        help="require a continuous item's horizontal velocity to reverse while retaining OWNER")
    parser.add_argument("--expect-item-owner-transfer", type=int, nargs=3,
                        action="append", default=[], metavar=("KIND", "FROM", "TO"),
                        help="require a continuously traced item of KIND to change "
                             "fighter owner from FROM to TO in both runs")
    parser.add_argument("--expect-item-event", nargs=2, action="append",
                        default=[], metavar=("EVENT", "KIND"),
                        help="require an item callback event for KIND in both runs")
    parser.add_argument("--expect-item-removed-on-damage", type=int, nargs=2,
                        action="append", default=[], metavar=("KIND", "PORT"),
                        help="require a PORT-owned item of KIND to disappear "
                             "on a damage sample for PORT in both runs")
    parser.add_argument("--expect-attachment", action="store_true",
                        help="require a rendered P1 attachment in both runs")
    parser.add_argument("--expect-motion", type=int, action="append", default=[],
                        help="require a P1 motion transition in both runs")
    parser.add_argument("--expect-port-motion", type=int, nargs=2,
                        action="append", default=[], metavar=("PORT", "MOTION"),
                        help="require a motion transition for a compared port")
    parser.add_argument("--expect-motion-window", type=int, nargs=4,
                        action="append", default=[], metavar=("PORT", "MOTION", "BEGIN", "END"),
                        help="require a motion change or animation-frame restart during [BEGIN, END)")
    parser.add_argument("--expect-damage-port", type=int, action="append",
                        default=[], help="require damage to this port in both runs")
    args = parser.parse_args()
    ports = set(args.ports)
    if not ports or any(port < 1 or port > 4 for port in ports):
        parser.error("ports must be between 1 and 4")
    clone_ports = set(args.clone_ports)
    if not clone_ports or not clone_ports <= ports:
        parser.error("clone ports must be a nonempty subset of compared ports")
    independent_ports = set(args.independent_ports)
    if not independent_ports <= clone_ports:
        parser.error("independent ports must be a subset of clone ports")

    try:
        stock, stock_end, stock_missing_visual = read_trace(
            args.stock, ports, clone_ports, independent_ports)
        clone, clone_end, clone_missing_visual = read_trace(
            args.clone, ports, clone_ports, independent_ports, cloned=True)
    except ValueError as error:
        print(error)
        return 1
    if stock_end is None or clone_end is None or stock_end != clone_end:
        print(f"Incomplete or unequal replay: stock={stock_end}, clone={clone_end}")
        return 1
    if args.expect_attachment:
        for label, entries in (("stock", stock), ("clone", clone)):
            if not any(port == 1 and (match := re.search(r"attachment=([a-f0-9]{16})", state))
                       and match.group(1) != "cbf29ce484222325"
                       for _, port, state in entries["fighter"]):
                print(f"{label} never rendered the expected P1 attachment")
                return 1
    if not stock["fighter"] or not clone["fighter"]:
        print("Both logs need --trace-fighter-state output")
        return 1
    if not stock["rng"] or not clone["rng"]:
        print("Both logs need RNG trace output from the current client")
        return 1
    if stock_missing_visual or clone_missing_visual:
        print("Both logs need item visual digests from the current client")
        return 1
    for kind in args.expect_item_kind:
        pattern = re.compile(rf"^kind={kind} ")
        if not any(pattern.match(item[2]) for item in stock["item"]) or \
           not any(pattern.match(item[2]) for item in clone["item"]):
            print(f"Expected item kind {kind} was not spawned in both runs")
            return 1
    for kind, state, owner, begin, end in args.expect_item_state:
        if owner not in range(5) or not 0 <= begin < end:
            parser.error("item state requires owner 0..4 and a nonempty nonnegative sample interval")
        pattern = re.compile(rf"^kind={kind} state={state} .*?\bowner=P{owner}(?: |$)")
        for label, trace in (("stock", stock), ("clone", clone)):
            if not any(begin <= sample < end and pattern.search(value)
                       for sample, _, value in trace["item"]):
                print(f"Expected kind {kind} state {state} owner P{owner} "
                      f"during [{begin}, {end}) in {label} trace")
                return 1
    for kind, owner in args.expect_item_direction_change:
        if owner not in range(5):
            parser.error("item direction change owner must be 0..4")
        pattern = re.compile(rf"^kind={kind} .*?\bowner=P{owner} .*?vel=\(([-+\d.eE]+),")
        for label, trace in (("stock", stock), ("clone", clone)):
            previous, found = {}, False
            for sample, index, value in trace["item"]:
                match = pattern.search(value)
                if match is None:
                    # Do not carry observations across another owner or kind.
                    previous.pop(index, None)
                    continue
                speed = float(match.group(1))
                last = previous.get(index)
                if last and sample == last[0] + 1 and last[1] * speed < 0:
                    found = True
                    break
                previous[index] = sample, speed
            if not found:
                print(f"Expected continuously traced kind {kind} to reverse "
                      f"horizontal velocity retaining P{owner} in {label} trace")
                return 1
    for kind, source, target in args.expect_item_owner_transfer:
        if source not in range(1, 5) or target not in range(1, 5) or source == target:
            parser.error("item owner transfer ports must be distinct and between 1 and 4")
        pattern = re.compile(rf"^kind={kind} .* owner=P([0-4])(?: |$)")
        for label, trace in (("stock", stock), ("clone", clone)):
            previous = {}
            found = False
            for sample, index, state in trace["item"]:
                match = pattern.match(state)
                if match is None:
                    continue
                owner = int(match.group(1))
                if previous.get(index) == (sample - 1, source) and owner == target:
                    found = True
                    break
                previous[index] = (sample, owner)
            if not found:
                print(f"Expected item kind {kind} to transfer P{source} -> "
                      f"P{target} in {label} trace")
                return 1
    for event, kind in args.expect_item_event:
        try:
            kind = int(kind)
        except ValueError:
            parser.error("item event KIND must be an integer")
        expected = f"kind={kind} event={event}"
        if not all(any(state == expected for _, state in trace["item_event"])
                   for trace in (stock, clone)):
            print(f"Expected item event {expected} in both runs")
            return 1
    for kind, port in args.expect_item_removed_on_damage:
        if port not in ports:
            parser.error("expected item removal port must be in compared ports")
        pattern = re.compile(rf"^kind={kind} .* owner=P{port}(?: |$)")
        for label, trace in (("stock", stock), ("clone", clone)):
            by_sample = {}
            for sample, index, state in trace["item"]:
                if pattern.match(state):
                    by_sample.setdefault(sample, set()).add(index)
            damage_samples = {sample for sample, target, _ in trace["damage"]
                              if target == port}
            if not any(by_sample.get(sample - 1, set()) -
                       by_sample.get(sample, set())
                       for sample in damage_samples):
                print(f"Expected P{port} item kind {kind} to disappear "
                      f"on damage in {label} trace")
                return 1
    expected_motions = [(1, motion) for motion in args.expect_motion]
    expected_motions.extend(args.expect_port_motion)
    for expected_port, motion in expected_motions:
        if expected_port not in ports:
            parser.error("expected motion port must be in compared ports")
        pattern = re.compile(rf"(?:^| )motion={motion}(?: |$)")
        if not all(any(port == expected_port and pattern.search(state)
                       for _, port, state in trace["action"])
                   for trace in (stock, clone)):
            print(f"Expected P{expected_port} motion {motion} was not reached in both runs")
            return 1
    for port, motion, begin, end in args.expect_motion_window:
        if port not in ports or not 0 <= begin < end:
            parser.error("motion window requires a compared port and nonempty nonnegative sample interval")
        pattern = re.compile(r"(?:^| )motion=(-?\d+) frame=([0-9a-f]{8})(?: |$)")
        for label, trace in (("stock", stock), ("clone", clone)):
            previous, found = None, False
            # The action channel logs ID changes, omitting a same-ID restart.
            # Read the sampled animation clock so repeated moves are observable.
            for sample, actor, value in trace["fighter"]:
                if actor != port:
                    continue
                match = pattern.search(value)
                if match is None:
                    continue
                current = int(match.group(1)), struct.unpack(">f", bytes.fromhex(match.group(2)))[0]
                restart = previous is None or current[0] != previous[0] or current[1] < previous[1]
                if begin <= sample < end and current[0] == motion and restart:
                    found = True
                    break
                previous = current
            if not found:
                print(f"Expected P{port} motion {motion} entry during "
                      f"[{begin}, {end}) in {label} trace")
                return 1
    for port in args.expect_damage_port:
        if port not in ports:
            parser.error("expected damage port must be in compared ports")
        if not all(any(target == port for _, target, _ in trace["damage"])
                   for trace in (stock, clone)):
            print(f"Expected damage to P{port} was not recorded in both runs")
            return 1
    first_difference = None
    for name in stock:
        left, right = stock[name], clone[name]
        for index in range(max(len(left), len(right))):
            a = left[index] if index < len(left) else None
            b = right[index] if index < len(right) else None
            if a != b:
                sample = a[0] if a is not None else b[0]
                if first_difference is None or sample < first_difference[0]:
                    first_difference = (sample, name, index, a, b)
                break
    if first_difference is not None:
        sample, name, index, a, b = first_difference
        print(f"First difference at sample {sample} ({name} entry {index}):\n"
              f"  stock: {a}\n  clone: {b}")
        return 1
    print(f"Match: {stock_end} controller samples, "
          f"{len(stock['fighter'])} fighter states, "
          f"{len(stock['action'])} actions, {len(stock['item'])} item states, "
          f"{len(stock['item_event'])} item events, "
          f"{len(stock['damage'])} damage events, "
          f"{len(stock['rng'])} RNG states")
    if stock["resource"]:
        print(f"Resources match: {len(stock['resource'])} count/node records")
    if stock["companion"]:
        print(f"Companions match: {len(stock['companion'])} states, "
              f"{len(stock['companion_action'])} actions, "
              f"{len(stock['companion_damage'])} damage events")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
