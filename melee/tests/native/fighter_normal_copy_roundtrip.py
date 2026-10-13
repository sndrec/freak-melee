"""Copy a real Mario normal's subactions and clip between independent drafts."""

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


def digest(path):
    return hashlib.sha256(path.read_bytes()).digest()


def main():
    tool, iso = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="freak-normal-copy-") as temp:
        root = pathlib.Path(temp)
        source = root / "Mario Source"
        destination = root / "Mario Destination"
        customized = root / "Mario Different Weight"
        group_destination = root / "Mario Group Destination"
        incompatible = root / "Mario Incompatible"
        aerial_source = root / "Mario Aerial Source"
        aerial_destination = root / "Mario Aerial Destination"
        for project in (source, destination, customized, group_destination,
                        incompatible, aerial_source, aerial_destination):
            run(tool, "clone-stock", iso, "Mario", project, project.name)
        stock_fighter = digest(destination / "fighter.dat")
        stock_animation = digest(destination / "animation.dat")

        run(tool, "project-event", source, 46, "0x3F90", 8)
        run(tool, "project-animation", source, 46, 4, 0, 0.25)
        assert digest(source / "fighter.dat") != stock_fighter
        assert digest(source / "animation.dat") != stock_animation

        run(tool, "project-copy-normal", source, source, 46,
            succeeds=False)
        run(tool, "project-copy-normal", source, destination, 295,
            succeeds=False)
        assert digest(destination / "fighter.dat") == stock_fighter
        assert digest(destination / "animation.dat") == stock_animation

        # An action with a different command opcode is not the same source
        # graph, even if both fighter DATs remain valid HSD archives.
        bad_dat = incompatible / "fighter.dat"
        changed = bytearray(bad_dat.read_bytes())
        first_command = 32 + 0x3F8C
        changed[first_command] = (changed[first_command] & 3) | (3 << 2)
        bad_dat.write_bytes(changed)
        before_bad = digest(bad_dat)
        run(tool, "project-copy-normal", source, incompatible, 46,
            succeeds=False)
        assert digest(bad_dat) == before_bad

        run(tool, "project-copy-normal", source, destination, 46)
        fighter = destination / "fighter.dat"
        animation = destination / "animation.dat"
        assert digest(fighter) == digest(source / "fighter.dat")
        assert digest(animation) == digest(source / "animation.dat")
        assert "0x3F90 hitbox 8 id=0" in run(
            tool, "project-actions", destination, 46)
        assert "0 frame=0 op=4 value=0.25" in run(
            tool, "project-animation", destination, 46, 4)

        run(tool, "project-attribute", customized, "weight", 101)
        customized_fighter = digest(customized / "fighter.dat")
        run(tool, "project-copy-normal", source, customized, 46)
        assert "weight: 101" in run(tool, "project-attribute",
                                    customized, "weight")
        assert "0x3F90 hitbox 8 id=0" in run(
            tool, "project-actions", customized, 46)
        assert "0 frame=0 op=4 value=0.25" in run(
            tool, "project-animation", customized, 46, 4)
        run(tool, "project-undo", customized)
        run(tool, "project-animation-undo", customized)
        assert digest(customized / "fighter.dat") == customized_fighter
        assert digest(customized / "animation.dat") == stock_animation

        # The copy owns its bytes: later source edits cannot change it.
        copied_fighter = digest(fighter)
        run(tool, "project-event", source, 46, "0x3F90", 9)
        assert digest(fighter) == copied_fighter
        assert digest(source / "fighter.dat") != copied_fighter

        run(tool, "project-undo", destination)
        run(tool, "project-animation-undo", destination)
        assert digest(fighter) == stock_fighter
        assert digest(animation) == stock_animation

        # A jab copy is one transaction over its three stock action variants.
        run(tool, "project-event", source, 47, "0x4008", 7)
        run(tool, "project-event", source, 48, "0x40C4", 1000)
        run(tool, "project-animation", source, 47, 4, 0, 0.25)
        run(tool, "project-copy-normal-group", source,
            group_destination, "jab")
        assert digest(group_destination / "fighter.dat") == digest(
            source / "fighter.dat")
        assert digest(group_destination / "animation.dat") == digest(
            source / "animation.dat")
        assert "0x4008 hitbox 7 id=0" in run(
            tool, "project-actions", group_destination, 47)
        assert "0x40C4 hitbox-scale 1000 id=0" in run(
            tool, "project-actions", group_destination, 48)
        run(tool, "project-copy-normal-group", source, source, "jab",
            succeeds=False)
        run(tool, "project-copy-normal-group", source, destination,
            "unknown", succeeds=False)
        run(tool, "project-undo", group_destination)
        run(tool, "project-animation-undo", group_destination)
        assert digest(group_destination / "fighter.dat") == stock_fighter
        assert digest(group_destination / "animation.dat") == stock_animation

        # These are also three-variant stock normals with independent clips.
        run(tool, "project-copy-normal-group", source,
            destination, "forward-tilt")
        run(tool, "project-copy-normal-group", source,
            destination, "forward-smash")
        assert digest(destination / "fighter.dat") == stock_fighter
        assert digest(destination / "animation.dat") == stock_animation
        # An aerial is not complete without its separate landing action.
        run(tool, "project-event", aerial_source, 72, "0x49F0", 7)
        run(tool, "project-event", aerial_source, 77, "0x4AD0", 5)
        run(tool, "project-animation", aerial_source, 77, 11, 0, -1.5)
        run(tool, "project-copy-normal-group", aerial_source,
            aerial_destination, "down-air")
        assert digest(aerial_destination / "fighter.dat") == digest(
            aerial_source / "fighter.dat")
        assert digest(aerial_destination / "animation.dat") == digest(
            aerial_source / "animation.dat")
        assert "0x49F0 hitbox 7 id=0" in run(
            tool, "project-actions", aerial_destination, 72)
        assert "0x4AD0 hitbox 5 id=0" in run(
            tool, "project-actions", aerial_destination, 77)
        assert "0 frame=0 op=4 value=-1.5" in run(
            tool, "project-animation", aerial_destination, 77, 11)
        run(tool, "project-undo", aerial_destination)
        run(tool, "project-animation-undo", aerial_destination)
        assert digest(aerial_destination / "fighter.dat") == stock_fighter
        assert digest(aerial_destination / "animation.dat") == stock_animation
        for name in ("neutral-air", "forward-air", "back-air", "up-air"):
            run(tool, "project-copy-normal-group", aerial_source,
                aerial_destination, name)
        assert digest(aerial_destination / "fighter.dat") == stock_fighter
        assert digest(aerial_destination / "animation.dat") == stock_animation

    print("Independent Mario normal and aerial-landing copies passed")


if __name__ == "__main__":
    main()
