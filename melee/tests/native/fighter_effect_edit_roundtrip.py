"""Edit an original Mario effect spawn without rewriting unknown command bits."""

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
    with tempfile.TemporaryDirectory(prefix="freak-effect-edit-") as temp:
        project = pathlib.Path(temp) / "Mario Effects"
        run(tool, "clone-stock", iso, "Mario", project, "Mario Effects")
        dat = project / "fighter.dat"
        before = dat.read_bytes()
        original = hashlib.sha256(before).digest()
        assert "0x43E4 spawn-effect 1011 bone=33" in run(
            tool, "project-actions", project, 60)
        assert "gfx_id=1011 bone=33" in run(
            tool, "project-effect", project, 60, "0x43E4")

        run(tool, "project-effect", project, 59, "0x43E4", "offset_y", 256,
            succeeds=False)
        run(tool, "project-effect", project, 60, "0x43E4", "bone", 99,
            succeeds=False)
        run(tool, "project-effect", project, 60, "0x43E4", "range_x", 65536,
            succeeds=False)
        run(tool, "project-event", project, 60, "0x43E4", 1012,
            succeeds=False)
        assert dat.read_bytes() == before

        run(tool, "project-effect", project, 60, "0x43E4", "offset_y", 256)
        after = dat.read_bytes()
        changed = [i for i, (old, new) in enumerate(zip(before, after))
                   if old != new]
        assert changed == [32 + 0x43E4 + 10], changed
        assert "offset_raw=(0,256,0)" in run(
            tool, "project-effect", project, 60, "0x43E4")
        run(tool, "project-undo", project)
        assert hashlib.sha256(dat.read_bytes()).digest() == original

    print("Mario effect source, one-byte offset edit and undo passed")


if __name__ == "__main__":
    main()
