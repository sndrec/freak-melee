"""Check a Mario pose transfer against the native evaluator and disk undo."""

import math
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


def sample(tool, project, action, track, frame):
    output = run(tool, "project-animation-sample", project, action, track,
                 frame)
    return float(output.split("value ", 1)[1])


def main():
    tool, iso = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="freak-pose-transfer-") as temp:
        project = pathlib.Path(temp) / "Mario Pose Test"
        run(tool, "clone-stock", iso, "Mario", project, "Mario Pose Test")
        bank = project / "animation.dat"

        # Down air and back air use the same Mario skeleton. Give the source's
        # root translation a representable value distinct from the destination
        # so the default root-preservation policy is tested rather than assumed.
        run(tool, "project-animation-insert", project, 72, 5, 16, 6)
        before_paste = bank.read_bytes()
        source_rotation = sample(tool, project, 72, 3, 16)
        source_limb = sample(tool, project, 72, 28, 16)
        target_rotation = sample(tool, project, 70, 3, 0)
        target_limb = sample(tool, project, 70, 28, 0)
        target_root = sample(tool, project, 70, 5, 0)
        assert sample(tool, project, 72, 5, 16) == 6
        assert target_root != 6
        assert not math.isclose(target_rotation, source_rotation,
                                abs_tol=0.01)
        assert not math.isclose(target_limb, source_limb, abs_tol=0.01)

        result = run(tool, "project-pose-paste", project, 72, 16, 70, 0)
        assert "Pasted 161 channels; 0 source channels" in result
        assert bank.read_bytes() != before_paste
        assert math.isclose(sample(tool, project, 70, 3, 0),
                            source_rotation, abs_tol=0.0005)
        assert math.isclose(sample(tool, project, 70, 28, 0),
                            source_limb, abs_tol=0.0005)
        assert sample(tool, project, 70, 5, 0) == target_root
        assert sample(tool, project, 72, 5, 16) == 6

        pasted = bank.read_bytes()
        run(tool, "project-pose-paste", project, 72, 16, 70, 99,
            succeeds=False)
        assert bank.read_bytes() == pasted
        run(tool, "project-animation-undo", project)
        assert bank.read_bytes() == before_paste
        assert sample(tool, project, 70, 3, 0) == target_rotation
        assert sample(tool, project, 70, 28, 0) == target_limb
        assert sample(tool, project, 72, 5, 16) == 6
    print("Mario pose transfer, root preservation, rejection and disk undo passed")


if __name__ == "__main__":
    main()
