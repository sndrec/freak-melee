"""A local action script can gain a command without changing its neighbors."""

import argparse
import datetime
import hashlib
import json
import pathlib
import shutil
import uuid
import re
import struct
import subprocess
import sys
import zlib


OUTPUT = None
COMMANDS = []


def run(tool, *args, succeeds=True):
    command = [str(tool)] + list(map(str, args))
    if not args or args[0] != "--root":
        command[1:1] = ["--root", str(OUTPUT / "scratch cache")]
    result = subprocess.run(command, cwd=pathlib.Path(tool).parent, capture_output=True,
                            text=True, check=False, timeout=120,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    (OUTPUT / f"command-{len(COMMANDS):02d}.log").write_text(result.stdout + result.stderr)
    COMMANDS.append({"command": command, "exit": result.returncode, "expected_success": succeeds})
    (OUTPUT / "commands.json").write_text(json.dumps(COMMANDS, indent=2) + "\n")
    if (result.returncode == 0) != succeeds or (not succeeds and result.returncode != 1):
        raise AssertionError(f"exit {result.returncode}: {args}\n"
                             f"{result.stdout}\n{result.stderr}")
    return result.stdout


def portrait(path):
    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data +
                struct.pack(">I", zlib.crc32(kind + data)))

    rows = b"".join(b"\x00" + b"\x70\x50\x20\xff" * 16
                    for _ in range(16))
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n" +
        chunk(b"IHDR", struct.pack(">IIBBBBB", 16, 16, 8, 6, 0, 0, 0)) +
        chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def inserted_offset(output):
    match = re.search(r"at source 0x([0-9A-F]+)", output)
    assert match is not None, output
    return int(match.group(1), 16)


def main():
    global OUTPUT
    parser = argparse.ArgumentParser()
    parser.add_argument("tool", type=pathlib.Path)
    parser.add_argument("iso", type=pathlib.Path)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    root = (args.output or pathlib.Path("A:/FreakMelee-test-archives") /
            ("source-action-duplicate-" + stamp + "-" + uuid.uuid4().hex[:8])).resolve()
    if root.drive.upper() != "A:" or root.exists():
        parser.error("a fresh private A: output directory is required")
    root.mkdir(parents=True)
    OUTPUT = root
    tool_source = args.tool.resolve()
    tool_dir = root / "tool"
    tool_dir.mkdir()
    tool = tool_dir / tool_source.name
    shutil.copy2(tool_source, tool)
    for dll in tool_source.parent.glob("*.dll"):
        shutil.copy2(dll, tool_dir / dll.name)
    iso, fixture = args.iso.resolve(), args.source.resolve()
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    inputs = {str(path): sha(path) for path in [tool, fixture, pathlib.Path(__file__).resolve()]}
    project = root / "Mario Action"
    image = root / "portrait.png"
    portrait(image)
    run(tool, "clone-stock", iso, "Mario", project, "Mario Action")
    run(tool, "script-build", project, fixture)
    assert (project / "behavior.fms").read_bytes() == fixture.read_bytes()
    dat = project / "fighter.dat"
    before = dat.read_bytes()
    other_jab = run(tool, "project-actions", project, 47)
    down_air = run(tool, "project-actions", project, 72)
    original = run(tool, "project-actions", project, 46)
    assert "0x3F90 hitbox 3 id=0" in original
    run(tool, "project-event", project, 46, "0x3F8C", 4)
    assert "0x3F8C at-frame 4" in run(tool, "project-actions",
                                       project, 46)
    assert run(tool, "project-actions", project, 47) == other_jab
    before = dat.read_bytes()

    bad = run(tool, "project-insert-command", project, 46,
              "0xFFFFFF", 46, "0x3F90", succeeds=False)
    assert not bad and dat.read_bytes() == before

    first = inserted_offset(run(tool, "project-insert-command",
                                project, 46, "0x3F90", 46, "0x3F90"))
    once = dat.read_bytes()
    assert once[32 + 0x3F8C:32 + 0x4000] == before[32 + 0x3F8C:32 + 0x4000]
    assert f"0x{first:X} hitbox 3 id=3" in run(
        tool, "project-actions", project, 46)
    assert run(tool, "project-actions", project, 47) == other_jab
    assert run(tool, "project-actions", project, 72) == down_air

    second = inserted_offset(run(tool, "project-insert-command",
                                 project, 46, hex(first), 46, hex(first)))
    # Four live hitbox slots exist. When all IDs are occupied, copying a
    # command reuses its source ID instead of writing outside the fighter.
    assert f"0x{second:X} hitbox 3 id=3" in run(
        tool, "project-actions", project, 46)
    run(tool, "project-hitbox", project, 46, hex(second), "damage", 14)
    assert "id=3" in run(tool, "project-hitbox", project, 46, hex(second))
    valid_hitbox = dat.read_bytes()
    run(tool, "project-hitbox", project, 46, hex(second), "id", 4,
        succeeds=False)
    assert dat.read_bytes() == valid_hitbox
    assert "damage=14" in run(tool, "project-hitbox", project, 46,
                              hex(second))
    assert run(tool, "project-actions", project, 47) == other_jab
    assert run(tool, "project-actions", project, 72) == down_air

    effect = inserted_offset(run(tool, "project-insert-command",
                                 project, 46, hex(second), 295,
                                 "0x34C0"))
    assert f"0x{effect:X} spawn-effect 1013 bone=0" in run(
        tool, "project-actions", project, 46)
    assert run(tool, "project-actions", project, 47) == other_jab
    inserted_jab = run(tool, "project-actions", project, 46)
    bad_delete = run(tool, "project-delete-command", project, 46,
                     "0xFFFFFF", succeeds=False)
    assert not bad_delete and run(tool, "project-actions", project,
                                  46) == inserted_jab

    package = run(tool, "--root", root / "store", "package-project",
                  project, image)
    package_id = re.search(r"Cached source fighter ([0-9a-f]{64})", package)
    assert package_id is not None, package
    run(tool, "--root", root / "store", "verify-package",
        package_id.group(1))

    run(tool, "project-delete-command", project, 46, hex(effect))
    assert "spawn-effect 1013 bone=0" not in run(
        tool, "project-actions", project, 46)
    assert run(tool, "project-actions", project, 47) == other_jab

    shared_wait = run(tool, "project-actions", project, 6)
    branch = inserted_offset(run(tool, "project-insert-command",
                                 project, 2, "0x3250", 2, "0x3250"))
    branched_action = run(tool, "project-actions", project, 2)
    assert f"0x{branch:X} wait 2" in branched_action
    assert branched_action.count("wait ") == 4
    assert run(tool, "project-actions", project, 6) == shared_wait
    run(tool, "project-delete-command", project, 2, hex(branch))
    assert run(tool, "project-actions", project, 2).count("wait ") == 3
    assert run(tool, "project-actions", project, 6) == shared_wait
    shared_script = run(tool, "project-actions", project, 2)
    assert not run(tool, "project-delete-command", project, 2,
                   "0x3250", succeeds=False)
    assert run(tool, "project-actions", project, 2) == shared_script
    local = inserted_offset(run(tool, "project-localize-command",
                                project, 6, "0x3250"))
    run(tool, "project-event", project, 6, hex(local), 5)
    assert f"0x{local:X} wait 5" in run(tool, "project-actions", project, 6)
    assert run(tool, "project-actions", project, 2) == shared_script
    package = run(tool, "--root", root / "store", "package-project",
                  project, image)
    package_id = re.search(r"Cached source fighter ([0-9a-f]{64})", package)
    assert package_id is not None, package
    run(tool, "--root", root / "store", "verify-package",
        package_id.group(1))

    assert all(sha(pathlib.Path(path)) == digest for path, digest in inputs.items())
    receipt = {"status": "PASS", "commands": COMMANDS, "input_sha256": inputs,
               "publication_checks": 2, "scope": "Actual retail Mario action command edits, unchanged neighbors, relocation, failed-edit preservation and validated complete FMS/source packet import; no game or user installation"}
    (root / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"status": "PASS source action duplicate", "commands": len(COMMANDS), "receipt": str(root / "receipt.json")}))


if __name__ == "__main__":
    main()
