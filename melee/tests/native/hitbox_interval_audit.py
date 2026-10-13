"""Audit duration conversion and edits against independently decoded DATs."""

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess

from action_event_timing_audit import action_traces


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--tool", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    results = []

    def run(*command, succeeds=True):
        result = subprocess.run(
            [str(args.tool), *map(str, command)], capture_output=True, text=True
        )
        assert (result.returncode == 0) == succeeds, result.stdout + result.stderr
        return result.stdout

    original = action_traces(args.project / "fighter.dat")
    prototype = next(payload for _, op, _, payload in original(72)[0] if op == 11)

    def hit(slot=0, group=0):
        return [
            (prototype[0] & ~((7 << 23) | (7 << 20))) | (slot << 23) | (group << 20),
            *prototype[1:],
        ]

    def fixture(name, words):
        project = args.output / name
        shutil.copytree(args.project, project, dirs_exist_ok=True)
        path = project / "fighter.dat"
        bank = bytearray(path.read_bytes())
        original_events, complete = action_traces(path)(72)
        assert complete
        start = original_events[0][0]
        end = original_events[-1][0] + 4
        script = struct.pack(f">{len(words)}I", *words)
        assert len(script) <= end - start
        bank[32 + start : 32 + start + len(script)] = script
        path.write_bytes(bank)
        return project

    def convert(project):
        before = action_traces(project / "fighter.dat")(72)[0]
        run("project-hitbox-intervals", project, 72)
        after, complete = action_traces(project / "fighter.dat")(72)
        assert complete
        expected = []
        for i, (_, op, frame, payload) in enumerate(before):
            if op != 11:
                continue
            slot = payload[0] >> 23 & 7
            end = next(
                t
                for _, code, t, p in before[i + 1 :]
                if code == 16
                or code == 15
                and p[0] & 0x3FFFFFF == slot
                or code == 11
                and p[0] >> 23 & 7 == slot
            )
            expected.append((frame, slot, (end - frame) * 256, payload[1:]))
        actual = [
            (frame, p[0] >> 23 & 7, p[5] & 0x3FFFFFFF, p[1:5])
            for _, op, frame, p in after
            if op == 59
        ]
        assert expected == actual, (expected, actual)
        assert not any(op in (11, 15, 16) for _, op, _, _ in after)
        assert Counter(
            (op, t, p) for _, op, t, p in before if op >= 9 and op not in (11, 15, 16)
        ) == Counter((op, t, p) for _, op, t, p in after if op >= 9 and op != 59)
        assert Counter(p for _, op, _, p in before if op == 3) == Counter(
            p for _, op, _, p in after if op == 3
        )
        unchanged = (project / "fighter.dat").read_bytes()
        run("project-hitbox-intervals", project, 72)
        assert unchanged == (project / "fighter.dat").read_bytes()
        return after

    wait = lambda frames: (1 << 26) | frames
    clear = 16 << 26
    project = fixture("adjacent", [wait(4), *hit(), wait(3), *hit(), wait(3), clear, 0])
    after = convert(project)
    assert all(not p[5] & 0x80000000 for _, op, _, p in after if op == 59)
    results.append("adjacent intervals preserve history")
    first = next(o for o in after if o[1] == 59)
    source = first[0]
    run("project-hitbox", project, 72, hex(source), "duration256", 512)
    run("project-hitbox", project, 72, hex(source), "duration256", 1024, succeeds=False)
    run("project-hitbox", project, 72, hex(source), "duration256", 0, succeeds=False)
    before_reject = (project / "fighter.dat").read_bytes()
    run("project-move-event", project, 72, hex(source), 2, succeeds=False)
    assert before_reject == (project / "fighter.dat").read_bytes()
    text = run("project-move-event", project, 72, hex(source), -1)
    moved = int(re.search(r"source 0x([0-9A-F]+)", text).group(1), 16)
    edited = next(
        o for o in action_traces(project / "fighter.dat")(72)[0] if o[0] == moved
    )
    assert edited[2] == 3 and edited[3][5] & 0x3FFFFFFF == 512
    run("project-hitbox", project, 72, hex(moved), "duration256", 1)
    run("project-hitbox", project, 72, hex(moved), "refresh_hit_eligibility", 1)
    edited = next(
        o for o in action_traces(project / "fighter.dat")(72)[0] if o[0] == moved
    )
    assert edited[3][5] & 0x80000000 and edited[3][5] & 0x3FFFFFFF == 1
    results.append(
        "duration, fractional frames, move endpoint, refresh, atomic overlap rejection"
    )

    project = fixture("refresh", [*hit(), wait(2), clear, *hit(), wait(2), clear, 0])
    after = convert(project)
    assert [bool(p[5] & 0x80000000) for _, op, _, p in after if op == 59] == [
        False,
        True,
    ]
    results.append("clear and same-frame respawn refreshes eligibility")

    project = fixture(
        "loop", [wait(10), (3 << 26) | 7, *hit(), wait(2), clear, wait(2), 4 << 26, 0]
    )
    after = convert(project)
    source = next(o[0] for o in after if o[1] == 59)
    run("project-move-event", project, 72, hex(source), 1)
    edited = [o for o in action_traces(project / "fighter.dat")(72)[0] if o[1] == 59]
    assert [o[2] for o in edited] == [11, 15, 19, 23, 27, 31, 35]
    assert all(o[3][5] & 0x3FFFFFFF == 512 for o in edited)
    results.append("all seven loop pulses move with their endpoints")

    for name, words in (
        ("unresolved", [*hit(), 8 << 26, clear, 0]),
        ("unclosed", [*hit(), 0]),
        ("group-inheritance", [*hit(), wait(1), *hit(1), wait(2), clear, 0]),
        (
            "updated-group-inheritance",
            [*hit(), wait(1), *hit(), *hit(1), wait(2), clear, 0],
        ),
        ("zero-length", [*hit(), clear, 0]),
    ):
        project = fixture(name, words)
        before = (project / "fighter.dat").read_bytes()
        run("project-hitbox-intervals", project, 72)
        assert before == (project / "fighter.dat").read_bytes(), name
    results.append("unresolved lifetimes remain unchanged")

    project = fixture("new-hitbox", [*hit(), wait(2), clear, 0])
    source = next(
        o[0] for o in action_traces(project / "fighter.dat")(72)[0] if o[1] == 11
    )
    run("project-insert-at-frame", project, 72, 6, 72, hex(source))
    inserted = next(
        o for o in action_traces(project / "fighter.dat")(72)[0] if o[1] == 59
    )
    assert inserted[2] == 6 and inserted[3][5] & 0x3FFFFFFF == 256
    run("project-insert-at-frame", project, 5, 12, 72, hex(inserted[0]))
    empty = action_traces(project / "fighter.dat")(5)[0]
    assert any(
        op == 59 and frame == 12 and p[5] & 0x3FFFFFFF == 256
        for _, op, frame, p in empty
    )
    results.append(
        "new and copied hitboxes own their duration, including empty actions"
    )

    project = args.output / "stock-ganon"
    shutil.copytree(args.project, project, dirs_exist_ok=True)
    report = run("project-hitbox-intervals", project)
    assert not report.startswith("Converted 0 ")
    actions = [
        int(line.split()[0]) for line in run("project-actions", project).splitlines()
    ]
    current = action_traces(project / "fighter.dat")
    converted_actions = 0
    for action in actions:
        before, _ = original(action)
        after, _ = current(action)
        if any(op == 59 for _, op, _, _ in after):
            converted_actions += 1
            assert Counter(
                (op, t, p)
                for _, op, t, p in before
                if op >= 9 and op not in (11, 15, 16)
            ) == Counter((op, t, p) for _, op, t, p in after if op >= 9 and op != 59)
        else:
            assert before == after
    results.append(
        f"stock Ganondorf: {converted_actions} actions converted; other actions preserved"
    )
    (args.output / "results.json").write_text(json.dumps(results, indent=2))
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
