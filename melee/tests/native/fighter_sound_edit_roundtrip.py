"""Edit a reachable original Mario sound command without changing its shape."""

import hashlib
import pathlib
import subprocess
import sys
import tempfile


def run(tool, *args, succeeds=True):
    result = subprocess.run([tool, *map(str, args)], capture_output=True,
                            text=True, check=False)
    if (result.returncode == 0) != succeeds:
        raise AssertionError(
            f"unexpected exit {result.returncode}: {args}\n"
            f"{result.stdout}\n{result.stderr}"
        )
    return result.stdout


def main():
    tool, iso = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="freak-sound-edit-") as temp:
        project = pathlib.Path(temp) / "Mario Sounds"
        run(tool, "clone-stock", iso, "Mario", project, "Mario Sounds")
        dat = project / "fighter.dat"
        before = dat.read_bytes()
        original = hashlib.sha256(before).digest()
        assert "0x3FCC sound-effect 166 behavior=1" in run(
            tool, "project-actions", project, 46)
        assert "behavior=1 id=166 volume=127 pan=64" in run(
            tool, "project-sound", project, 46, "0x3FCC")

        run(tool, "project-sound", project, 47, "0x3FCC", 167, 100, 50,
            succeeds=False)
        run(tool, "project-sound", project, 46, "0x3FCC", 167, 256, 50,
            succeeds=False)
        run(tool, "project-event", project, 46, "0x3FCC", 167,
            succeeds=False)
        assert dat.read_bytes() == before

        run(tool, "project-sound", project, 46, "0x3FCC", 167, 100, 50)
        after = dat.read_bytes()
        changed = [i for i, (old, new) in enumerate(zip(before, after))
                   if old != new]
        assert changed == [32 + 0x3FCC + 7,
                           32 + 0x3FCC + 10,
                           32 + 0x3FCC + 11], changed
        assert "behavior=1 id=167 volume=100 pan=50" in run(
            tool, "project-sound", project, 46, "0x3FCC")
        assert "0x3FCC sound-effect 167 behavior=1" in run(
            tool, "project-actions", project, 46)
        run(tool, "project-undo", project)
        assert hashlib.sha256(dat.read_bytes()).digest() == original

    print("Mario sound command source, three-byte edit and undo passed")


if __name__ == "__main__":
    main()
