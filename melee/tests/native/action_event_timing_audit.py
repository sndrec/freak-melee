"""Check timeline retiming against independently decoded DAT execution."""

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess


def action_traces(path):
    bank = path.read_bytes()
    _, data_size, relocation_count, root_count, external_count = struct.unpack_from(
        ">5I", bank
    )
    roots = 32 + data_size + relocation_count * 4
    strings = roots + (root_count + external_count) * 8
    root = next(
        offset
        for offset, name in (
            struct.unpack_from(">2I", bank, roots + i * 8) for i in range(root_count)
        )
        if bank[strings + name :].split(b"\0")[0].startswith(b"ftData")
    )
    widths_source = Path(__file__).resolve().parents[2] / "src/melee/ft/ftaction_data.c"
    widths = [
        int(x)
        for x in re.search(
            r"action_widths\[49\] = \{([^}]+)", widths_source.read_text()
        )
        .group(1)
        .split(",")
        if x.strip()
    ]

    def word(offset):
        return struct.unpack_from(">I", bank, 32 + offset)[0]

    table = word(root + 12)

    def trace(action):
        offset = word(table + action * 24 + 12)
        if not offset:
            return [], True
        frame, stack, events = 0, [], []
        for _ in range(8192):
            value = word(offset)
            opcode = value >> 26
            width = (
                6
                if opcode == 59
                else (
                    2 if opcode in (5, 7) else 1 if opcode < 10 else widths[opcode - 10]
                )
            )
            payload = tuple(word(offset + i * 4) for i in range(width))
            if opcode == 1:
                frame += value & 0x3FFFFFF
            elif opcode == 2:
                frame = value & 0x3FFFFFF
            if frame > 1000:
                return events, False
            events.append((offset, opcode, frame, payload))
            next_offset = offset + width * 4
            if opcode == 0:
                return events, True
            if opcode == 3:
                stack.append([next_offset, value & 0x3FFFFFF, True])
            elif opcode == 4:
                stack[-1][1] -= 1
                if stack[-1][1]:
                    offset = stack[-1][0]
                    continue
                stack.pop()
            elif opcode in (5, 7):
                if not payload[1]:
                    return events, True
                if opcode == 5:
                    stack.append([next_offset, 0, False])
                offset = payload[1]
                continue
            elif opcode == 6:
                offset = stack.pop()[0]
                continue
            elif opcode == 8:
                return events, False
            offset = next_offset
        return events, False

    return trace


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--tool",
        type=Path,
        default=Path(__file__).resolve().parents[2]
        / "build/native/native_content_tool.exe",
    )
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    original = action_traces(args.project / "fighter.dat")
    listing = subprocess.check_output(
        [str(args.tool), "project-actions", str(args.project)], text=True
    )
    actions = [int(line.split()[0]) for line in listing.splitlines()]
    results = []

    def move(name, action, source, delta):
        project = args.output / name
        shutil.copytree(args.project, project, dirs_exist_ok=True)
        before, complete = original(action)
        assert complete
        output = subprocess.check_output(
            [
                str(args.tool),
                "project-move-event",
                str(project),
                str(action),
                hex(source),
                str(delta),
            ],
            text=True,
        )
        moved_source = int(re.search(r"source 0x([0-9A-F]+)", output).group(1), 16)
        current = action_traces(project / "fighter.dat")
        after, complete = current(action)
        assert complete
        expected = Counter(
            (opcode, frame + (delta if offset == source else 0), payload)
            for offset, opcode, frame, payload in before
            if opcode >= 9
        )
        actual = Counter(
            (opcode, frame, payload)
            for offset, opcode, frame, payload in after
            if opcode >= 9
        )
        assert expected == actual, (name, expected - actual, actual - expected)
        assert Counter(
            payload for _, opcode, _, payload in before if opcode == 3
        ) == Counter(payload for _, opcode, _, payload in after if opcode == 3)
        assert all(current(i) == original(i) for i in actions if i != action)
        assert any(offset == moved_source for offset, _, _, _ in after)
        results.append({"case": name, "passed": True})
        return project, after, moved_source

    down_air, _ = original(72)
    source = next(offset for offset, opcode, _, _ in down_air if opcode == 11)
    project, split, moved_source = move("split-simultaneous-hitboxes", 72, source, 2)
    subprocess.check_output(
        [
            str(args.tool),
            "project-move-event",
            str(project),
            "72",
            hex(moved_source),
            "-2",
        ],
        text=True,
    )
    merged, complete = action_traces(project / "fighter.dat")(72)
    assert complete
    assert sum(opcode == 1 for _, opcode, _, _ in merged) < sum(
        opcode == 1 for _, opcode, _, _ in split
    )
    assert Counter(
        (opcode, frame, payload) for _, opcode, frame, payload in merged if opcode >= 9
    ) == Counter(
        (opcode, frame, payload)
        for _, opcode, frame, payload in down_air
        if opcode >= 9
    )
    results.append({"case": "merge-shared-timer", "passed": True})

    for action in actions:
        events, complete = original(action)
        loop_start = next(
            (i for i, (_, opcode, _, _) in enumerate(events) if opcode == 3), None
        )
        if complete and loop_start is not None:
            source = next(
                offset
                for offset, opcode, _, _ in events[loop_start + 1 :]
                if opcode >= 9
            )
            move("loop-all-repetitions", action, source, 1)
            rejected = args.output / "reject-loop-boundary"
            shutil.copytree(args.project, rejected, dirs_exist_ok=True)
            before = (rejected / "fighter.dat").read_bytes()
            result = subprocess.run(
                [
                    str(args.tool),
                    "project-move-event",
                    str(rejected),
                    str(action),
                    hex(source),
                    "999",
                ],
                capture_output=True,
            )
            assert result.returncode != 0
            assert (rejected / "fighter.dat").read_bytes() == before
            results.append({"case": "reject-loop-boundary-atomically", "passed": True})
            break
    else:
        raise AssertionError("Fixture has no editable finite loop")
    for action in actions:
        events, complete = original(action)
        source = next(
            (
                offset
                for offset, opcode, frame, _ in events
                if opcode >= 9 and frame == 0
            ),
            None,
        )
        if (
            complete
            and source is not None
            and not any(opcode == 3 for _, opcode, _, _ in events)
        ):
            move("frame-zero-without-existing-timer", action, source, 3)
            break
    else:
        raise AssertionError("Fixture has no editable frame-zero event")
    (args.output / "results.json").write_text(json.dumps(results, indent=2))
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
