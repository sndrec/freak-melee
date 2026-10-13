from pathlib import Path
import struct, subprocess, shutil, json, re
import argparse

parser = argparse.ArgumentParser(
    description="Verify action insertion without altering existing event timing or other actions."
)
parser.add_argument(
    "--project",
    type=Path,
    required=True,
    help="A complete stock Ganondorf Workshop draft",
)
parser.add_argument(
    "--output", type=Path, required=True, help="Directory for independent test copies"
)
parser.add_argument(
    "--tool",
    type=Path,
    default=Path(__file__).resolve().parents[2]
    / "build/native/native_content_tool.exe",
)
args = parser.parse_args()
review = args.output
review.mkdir(parents=True, exist_ok=True)
base = args.project
tool = args.tool

widths = [
    int(x)
    for x in re.search(
        r"action_widths\[49\] = \{([^}]+)",
        (
            Path(__file__).resolve().parents[2] / "src/melee/ft/ftaction_data.c"
        ).read_text(),
    )
    .group(1)
    .replace("\n", " ")
    .split(",")
    if x.strip()
]


def traces(bank):
    b = bank.read_bytes()
    _, data, nr, nroot, nex = struct.unpack_from(">5I", b)
    roots = 32 + data + nr * 4
    strings = roots + (nroot + nex) * 8

    def u(at):
        return struct.unpack_from(">I", b, 32 + at)[0]

    root = next(
        struct.unpack_from(">2I", b, roots + i * 8)[0]
        for i in range(nroot)
        if b[strings + struct.unpack_from(">2I", b, roots + i * 8)[1] :]
        .split(b"\0")[0]
        .startswith(b"ftData")
    )
    table = u(root + 12)

    def trace(action):
        at = u(table + action * 24 + 12)
        if not at:
            return []
        frame = 0
        stack = []
        out = []
        for _ in range(8192):
            raw = u(at)
            op = raw >> 26
            w = 2 if op in (5, 7) else 1 if op < 10 else widths[op - 10]
            payload = [u(at + i * 4) for i in range(w)]
            if op == 1:
                frame += raw & 0x3FFFFFF
            elif op == 2:
                frame = raw & 0x3FFFFFF
            if frame > 1000:
                return out
            out.append((op, frame, tuple(payload[:1] if op in (5, 7) else payload)))
            nxt = at + w * 4
            if op == 0:
                return out
            if op == 3:
                stack.append([nxt, raw & 0x3FFFFFF, True])
            elif op == 4:
                stack[-1][1] -= 1
                if stack[-1][1]:
                    at = stack[-1][0]
                    continue
                stack.pop()
            elif op in (5, 7):
                target = u(at + 4)
                if not target:
                    return out
                if op == 5:
                    stack.append([nxt, 0, False])
                at = target
                continue
            elif op == 6:
                at = stack.pop()[0]
                continue
            elif op == 8:
                return out
            at = nxt
        return out

    return trace


original = traces(base / "fighter.dat")
listing = subprocess.check_output([str(tool), "project-actions", str(base)]).decode()
indices = [int(line.split()[0]) for line in listing.splitlines()]
loop = next(
    (
        i
        for i in indices
        if any(
            op == 3 and (payload[0] & 0x3FFFFFF) > 1 for op, _, payload in original(i)
        )
    ),
    None,
)
cases = [
    ("before-absolute", 68, 3),
    ("split-relative", 68, 8),
    ("after-end", 68, 100),
    ("empty-action", 5, 12),
]
if loop is not None:
    cases.append(("loop", loop, 2))
results = []
for name, action, frame in cases:
    target = review / ("insert-" + name)
    shutil.copytree(base, target, dirs_exist_ok=True)
    result = subprocess.run(
        [
            str(tool),
            "project-insert-at-frame",
            str(target),
            str(action),
            str(frame),
            "68",
            "0x4DA8",
        ],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, (name, result.stderr)
    current = traces(target / "fighter.dat")
    old = [x for x in original(action) if x[0] not in (0, 1, 2)]
    now = [x for x in current(action) if x[0] not in (0, 1, 2)]
    remaining = iter(now)
    assert all(any(next_event == event for next_event in remaining) for event in old), (
        name,
        "existing event timing changed",
    )
    assert any(op == 19 and f == frame for op, f, p in now), (
        name,
        "new event missing at requested frame",
    )
    assert all(current(i) == original(i) for i in indices if i != action), (
        name,
        "another action changed",
    )
    results.append({"case": name, "action": action, "frame": frame, "passed": True})
target = review / "edit-variable"
shutil.copytree(base, target, dirs_exist_ok=True)
result = subprocess.run(
    [str(tool), "project-event", str(target), "68", "0x4DA8", "2"],
    capture_output=True,
    text=True,
)
assert result.returncode == 0, ("edit-variable", result.stderr)
current = traces(target / "fighter.dat")
assert any(
    op == 19 and payload[0] & 0xFFFFFF == 2 for op, frame, payload in current(68)
)
assert all(current(i) == original(i) for i in indices if i != 68)
results.append({"case": "edit-variable-with-empty-actions", "passed": True})
(review / "insert-results.json").write_text(json.dumps(results, indent=2))
print(json.dumps(results, indent=2))
