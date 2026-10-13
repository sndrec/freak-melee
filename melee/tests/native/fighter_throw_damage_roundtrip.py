"""Edit a source-linked original throw hitbox damage without changing its shape."""

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
    with tempfile.TemporaryDirectory(prefix="freak-throw-edit-") as temp:
        project = pathlib.Path(temp) / "Mario Throws"
        run(tool, "clone-stock", iso, "Mario", project, "Mario Throws")
        dat = project / "fighter.dat"
        before = dat.read_bytes()
        original = hashlib.sha256(before).digest()
        assert "0x5B48 throw-hitbox 9 id=0" in run(
            tool, "project-actions", project, 247)

        run(tool, "project-event", project, 248, "0x5B48", 15,
            succeeds=False)
        run(tool, "project-event", project, 247, "0x5B48", 0x800000,
            succeeds=False)
        assert dat.read_bytes() == before

        run(tool, "project-event", project, 247, "0x5B48", 15)
        after = dat.read_bytes()
        changed = [i for i, (old, new) in enumerate(zip(before, after))
                   if old != new]
        assert changed == [32 + 0x5B48 + 3], changed
        assert "0x5B48 throw-hitbox 15 id=0" in run(
            tool, "project-actions", project, 247)
        run(tool, "project-undo", project)
        assert hashlib.sha256(dat.read_bytes()).digest() == original

    print("Mario throw hitbox damage, action ownership and undo passed")


if __name__ == "__main__":
    main()
