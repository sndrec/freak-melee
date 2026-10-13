"""Actual source-only install/export/re-clone/edit/build proof in private caches."""
import argparse
import datetime
import hashlib
import json
import pathlib
import re
import shutil
import struct
import subprocess
import uuid
import zlib


def portrait(path):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    rows = (b"\x00" + b"\x70\x50\x20\xff" * 16) * 16
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 16, 16, 8, 6, 0, 0, 0)) +
                     chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("tool", type=pathlib.Path)
    parser.add_argument("iso", type=pathlib.Path)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    root = (args.output or pathlib.Path("A:/FreakMelee-test-archives") /
            ("source-installed-clone-" + stamp + "-" + uuid.uuid4().hex[:8])).resolve()
    if root.drive.upper() != "A:" or root.exists():
        parser.error("a fresh private A: output directory is required")
    root.mkdir(parents=True)
    tool_source = args.tool.resolve()
    tool_dir = root / "tool"
    tool_dir.mkdir()
    tool = tool_dir / tool_source.name
    shutil.copy2(tool_source, tool)
    for dll in tool_source.parent.glob("*.dll"):
        shutil.copy2(dll, tool_dir / dll.name)
    source = args.source.resolve()
    source_hash = digest(source)
    tool_hash = digest(tool)
    commands = []
    checks = 0

    def check(value, message):
        nonlocal checks
        checks += 1
        if not value:
            raise AssertionError(message)

    def run(*words):
        command = [str(tool)] + list(map(str, words))
        if not words or words[0] != "--root":
            command[1:1] = ["--root", str(root / "scratch cache")]
        result = subprocess.run(command, cwd=tool_dir, capture_output=True, text=True,
                                timeout=120, creationflags=subprocess.CREATE_NO_WINDOW)
        index = len(commands)
        (root / f"command-{index:02d}.log").write_text(result.stdout + result.stderr)
        commands.append({"command": command, "exit": result.returncode})
        (root / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        check(result.returncode == 0, f"exit {result.returncode}: {words}\n{result.stdout}\n{result.stderr}")
        return result.stdout

    def package_id(output):
        match = re.search(r"Cached source fighter ([0-9a-f]{64})", output)
        check(match is not None, output)
        return match.group(1)

    author, recipient = root / "author project", root / "recipient project"
    author_store, recipient_store = root / "author store", root / "recipient store"
    image, exported = root / "portrait.png", root / "fighter.fmc"
    portrait(image)
    run("clone-stock", args.iso.resolve(), "Mario", author, "Mario Author")
    run("script-build", author, source)
    run("project-attribute", author, "weight", 110)
    first = package_id(run("--root", author_store, "package-project", author, image))
    run("--root", author_store, "verify-package", first)
    run("--root", author_store, "accept", first)
    run("--root", author_store, "export", first, exported)
    check(exported.read_bytes()[:4] == b"FMSC", "ordinary export must be the canonical source packet")
    payload_names = ["fighter.dat", "animation.dat", "costume.dat", "assets.fma", "behavior.fms", "cpu_profile.fcp"]
    if (author / "fighter_parameters.fmp").exists():
        payload_names.append("fighter_parameters.fmp")
    original_files = {name: digest(author / name) for name in payload_names}
    check(original_files["behavior.fms"] == source_hash, "complete source was not published exactly")
    check(author.resolve().is_relative_to(root) and author.resolve() != root, "author draft escaped private output")
    shutil.rmtree(author)
    check(not author.exists(), "author draft still exists")
    check(first in run("--root", recipient_store, "cache", exported), "cache did not preserve packet identity")
    run("--root", recipient_store, "accept", first)
    run("--root", recipient_store, "clone-installed", first, recipient, "Mario Recipient")
    for name in payload_names:
        check(digest(recipient / name) == original_files[name], "cloned payload changed: " + name)
    check(digest(recipient / "portrait.png") == digest(image), "portrait did not survive install/re-clone")
    check("Mario Recipient" in run("inspect-project", recipient), "recipient title was not retained")

    # Only the copied packet and recipient draft remain. No author draft, donor
    # behavior lookup or ISO is supplied to these validation/publication routes.
    package_hash = digest(exported)
    cache_before = {str(p.relative_to(recipient_store)): digest(p) for p in recipient_store.rglob("*.fmc")}
    cpu = recipient / "cpu_profile.fcp"
    cpu_before = digest(cpu)
    lines = cpu.read_text().splitlines()
    changed = False
    for i, line in enumerate(lines):
        if line.startswith("distance "):
            lines[i] = "distance 9" if line == "distance 10" else "distance 10"
            changed = True
    check(changed, "CPU profile has no distance field")
    cpu.write_bytes(("\n".join(lines) + "\n").encode())
    check(digest(cpu) != cpu_before, "CPU profile edit was ineffective")
    run("project-attribute", recipient, "weight", 125)
    check(digest(exported) == package_hash, "recipient edit changed the exported author packet")
    run("--root", recipient_store, "verify-package", first)
    run("source-check", recipient, recipient / "behavior.fms")
    run("script-build", recipient, recipient / "behavior.fms")
    check(digest(recipient / "assets.fma") == original_files["assets.fma"], "rebuild replaced the owned facts")
    check(digest(recipient / "behavior.fms") == source_hash, "rebuild changed complete source bytes")
    second = package_id(run("--root", recipient_store, "package-project", recipient, image))
    check(second != first, "edited recipient must have a distinct packet identity")
    run("--root", recipient_store, "verify-package", second)
    run("--root", recipient_store, "accept", second)
    check(all(digest(recipient_store / name) == expected for name, expected in cache_before.items()),
          "recipient rebuild changed an accepted cached packet")
    check(digest(exported) == package_hash and digest(source) == source_hash, "accepted packet or input source changed")
    check(digest(tool) == tool_hash, "fixture changed its copied tool")
    receipt = {"status": "PASS", "checks": checks, "commands": commands, "source_sha256": source_hash,
               "tool_sha256": tool_hash, "first_id": first, "second_id": second, "payload_sha256": original_files,
               "scope": "Actual retail Mario clone, complete source candidate, source packet/cache/private install/export/re-clone and independent DAT/CPU edit/rebuild; no game, global build or user-store install"}
    (root / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"status": "PASS source installed clone", "checks": checks, "receipt": str(root / "receipt.json")}))


if __name__ == "__main__":
    main()
