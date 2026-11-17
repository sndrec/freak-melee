"""Exercise a real Mario draft's encoded key, native evaluation and disk undo."""

import hashlib
import pathlib
import re
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


def sample(tool, project, frame):
    output = run(tool, "project-animation-sample", project, 295, 0, frame)
    return float(output.split("value ", 1)[1])


def digest(data):
    return hashlib.sha256(data).digest()


def main():
    tool, iso = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="freak-animation-edit-") as temp:
        project = pathlib.Path(temp) / "Mario Key Test"
        run(tool, "clone-stock", iso, "Mario", project, "Mario Key Test")
        bank = project / "animation.dat"
        original = bank.read_bytes()
        initial = sample(tool, project, 11)
        assert abs(initial - 0.118713379) < 1e-7

        run(tool, "project-animation", project, 295, 0, 5, 0.25)
        edited = bank.read_bytes()
        assert len(edited) == len(original)
        changed = [i for i, (a, b) in enumerate(zip(original, edited))
                   if a != b]
        assert changed == [403636, 403637], changed
        assert sample(tool, project, 11) == 0.25
        assert abs(sample(tool, project, 11.5) - 0.178985596) < 1e-7

        run(tool, "project-animation", project, 295, 0, 5, 0.251,
            succeeds=False)
        run(tool, "project-animation", project, 295, 0, 1, 0.25,
            succeeds=False)
        assert digest(bank.read_bytes()) == digest(edited)

        run(tool, "project-animation-undo", project)
        assert digest(bank.read_bytes()) == digest(original)
        assert sample(tool, project, 11) == initial

        before_curve = sample(tool, project, 1)
        run(tool, "project-animation-tangent", project, 295, 0, 0, 0.25)
        tangent_edit = bank.read_bytes()
        changed = [i for i, (a, b) in enumerate(zip(original, tangent_edit))
                   if a != b]
        assert changed == [403619, 403620], changed
        assert abs(before_curve - -0.0168457031) < 1e-7
        assert abs(sample(tool, project, 1) - 0.0458526611) < 1e-7
        run(tool, "project-animation-tangent", project, 295, 0, 0, 0.251,
            succeeds=False)
        run(tool, "project-animation-tangent", project, 295, 0, 2, 0.25,
            succeeds=False)
        assert digest(bank.read_bytes()) == digest(tangent_edit)
        run(tool, "project-animation-undo", project)
        assert digest(bank.read_bytes()) == digest(original)

        before_move = run(tool, "project-animation", project, 295, 0).splitlines()[1:]
        other_track = run(tool, "project-animation", project, 295, 1).splitlines()[1:]
        run(tool, "project-animation-move", project, 295, 0, 5, 12)
        moved = bank.read_bytes()
        assert moved != original
        # Retiming re-encodes and relocates the stream. Verify key semantics
        # and unrelated tracks rather than the old two-byte patch layout.
        expected = [line.replace("5 frame=11 ", "5 frame=12 ", 1)
                    if line.startswith("5 frame=") else line for line in before_move]
        assert run(tool, "project-animation", project, 295, 0).splitlines()[1:] == expected
        assert run(tool, "project-animation", project, 295, 1).splitlines()[1:] == other_track
        keys = run(tool, "project-animation", project, 295, 0)
        assert "5 frame=12" in keys and "6 frame=13" in keys
        assert abs(sample(tool, project, 11) - 0.0593566895) < 1e-7
        run(tool, "project-animation-move", project, 295, 0, 5, 13,
            succeeds=False)
        run(tool, "project-animation-move", project, 295, 0, 1, 4,
            succeeds=False)
        assert digest(bank.read_bytes()) == digest(moved)
        run(tool, "project-animation-undo", project)
        assert digest(bank.read_bytes()) == digest(original)
        remaining_at_13 = sample(tool, project, 13)
        run(tool, "project-animation-delete", project, 295, 0, 5)
        without_key = run(tool, "project-animation", project, 295, 0)
        assert "frame=12" not in without_key
        assert sample(tool, project, 13) == remaining_at_13
        run(tool, "project-animation-undo", project)
        assert digest(bank.read_bytes()) == digest(original)
        run(tool, "project-animation-delete", project, 295, 0, 0)
        assert "frame=0 " not in run(tool, "project-animation", project, 295, 0)
        assert sample(tool, project, 13) == remaining_at_13
        run(tool, "project-animation-undo", project)
        assert digest(bank.read_bytes()) == digest(original)
        run(tool, "project-animation-insert", project, 295, 0, 11, 0.25)
        with_key = run(tool, "project-animation", project, 295, 0)
        assert "frame=11" in with_key
        run(tool, "project-animation-undo", project)
        assert digest(bank.read_bytes()) == digest(original)
        info = run(tool, "project-animation", project, 295)
        original_frames = float(info.split("frames=", 1)[1].split()[0])
        run(tool, "project-animation-frames", project, 295,
            original_frames + 2)
        extended = bank.read_bytes()
        assert extended != original
        assert f"frames={original_frames + 2:g}" in run(
            tool, "project-animation", project, 295)
        run(tool, "project-animation-frames", project, 295, 1,
            succeeds=False)
        assert bank.read_bytes() == extended
        run(tool, "project-animation-undo", project)
        assert digest(bank.read_bytes()) == digest(original)
        # Pose authoring must create missing channels without shifting another
        # bone's meaning or changing existing tracks/other clips.
        listing = run(tool, "project-animation", project, 295)
        track_ids = {(int(j), int(c)): int(t) for t, j, c in
                     re.findall(r"(\d+) joint=(\d+) channel=(\d+)", listing)}
        joint, channel = next((0, c) for c in (5, 6, 7, 8, 9, 10)
                              if (0, c) not in track_ids)
        old_pair = next(pair for pair, index in track_ids.items() if index == 0)
        run(tool, "project-animation-key-bone", project, 295,
            joint, channel, 10, 2.5, 0.75)
        updated = run(tool, "project-animation", project, 295)
        after_ids = {(int(j), int(c)): int(t) for t, j, c in
                     re.findall(r"(\d+) joint=(\d+) channel=(\d+)", updated)}
        assert len(after_ids) == len(track_ids) + 1
        def at(track, frame):
            output = run(tool, "project-animation-sample", project, 295, track, frame)
            return float(output.split("value ", 1)[1])
        assert at(after_ids[joint, channel], 0) == 0.75
        assert at(after_ids[joint, channel], 10) == 2.5
        assert at(after_ids[old_pair], 11) == initial
        run(tool, "project-animation-key-bone", project, 295,
            joint, channel, 10, 3.5, 0.75)
        assert at(after_ids[joint, channel], 10) == 3.5
        assert at(after_ids[old_pair], 11) == initial
        # A new constant track at frame zero must actually drive FObj; a lone
        # linear endpoint has not initialized its interpolation state yet.
        another = next(c for c in (1, 2, 3, 5, 6, 7, 8, 9, 10)
                       if (joint, c) not in after_ids)
        run(tool, "project-animation-key-bone", project, 295,
            joint, another, 0, 1.25, 0.75)
        final_ids = {(int(j), int(c)): int(t) for t, j, c in
                     re.findall(r"(\d+) joint=(\d+) channel=(\d+)",
                                run(tool, "project-animation", project, 295))}
        assert at(final_ids[joint, another], 0) == 1.25
        assert at(final_ids[joint, another], 10) == 1.25
        before_rejected = bank.read_bytes()
        run(tool, "project-animation-key-bone", project, 295,
            joint, 4, 10, 3.5, 0.75, succeeds=False)
        assert bank.read_bytes() == before_rejected
    print("Animation value, tangent, timing, deletion, length, new bone channels and undo passed")


if __name__ == "__main__":
    main()
