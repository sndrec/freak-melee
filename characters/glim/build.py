"""Create Glim from the local ISO and the checked-in authoring sources."""

import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "melee/tools"))
from fighter_animation_io import Dat, joints
from animations import author_animations
from model import make_art


def facts(path):
    raw = path.read_bytes()
    cursor = 4
    strings = []
    for _ in range(7):
        length = raw[cursor]
        cursor += 1
        strings.append(raw[cursor : cursor + length].decode())
        cursor += length
    counts = struct.unpack_from("<9H", raw, cursor)
    cursor += 18 + 3 + 8 + 1
    byte_ranges = raw[cursor + 7]
    cursor += 8
    common, first, count = struct.unpack_from("<3I", raw, cursor)
    cursor += 12
    moves = [struct.unpack_from("<3I", raw, cursor + n * 12) for n in range(4)]
    cursor += 48 + byte_ranges * 4
    motions = [struct.unpack_from("<iIIB", raw, cursor + n * 13) for n in range(count)]
    return dict(
        root=strings[0],
        joint=strings[1],
        actions=counts[0],
        demos=counts[1],
        joints=counts[2],
        first=first,
        count=count,
        moves=moves,
        motions=motions,
    )


def assemble_source(info, project):
    owned = {341, 342, 348}
    for ground, air, _ in info["moves"]:
        owned.update((ground, air))
    policies = "\n".join(
        f"motion_policy {motion} common 0"
        for motion in range(info["first"], info["first"] + info["count"])
        if motion not in owned
    )
    state = (
        (HERE / "src/fighter_state.fms")
        .read_text()
        .replace("__MOTION_POLICIES__", policies)
    )
    (project / "fighter_state.fms").write_text(state)
    (project / "moves").mkdir(exist_ok=True)
    sources = [state]
    for path in sorted((HERE / "src").glob("[0-9]*.fms")):
        text = path.read_text()
        (project / "moves" / path.name).write_text(text)
        sources.append(text)
    (project / "behavior.fms").write_text("\n".join(sources))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--replace-draft", action="store_true")
    parser.add_argument("--no-install", action="store_true")
    args = parser.parse_args()
    tool = ROOT / "melee/build/native/native_content_tool.exe"
    project = HERE / "project"
    output = HERE / "build"
    output.mkdir(exist_ok=True)

    def run(*command):
        result = subprocess.run(
            [str(tool), *map(str, command)],
            cwd=tool.parent,
            capture_output=True,
            text=True,
            timeout=120,
        )
        print(result.stdout.strip())
        if result.returncode:
            raise RuntimeError(result.stderr or f"Failed: {command}")
        return result.stdout

    stock = output / "stock"
    if not stock.exists():
        run("clone-stock", args.iso.resolve(), "Jigglypuff", stock, "Glim")
    project.mkdir(exist_ok=True)
    for path in stock.iterdir():
        if path.is_file():
            shutil.copy2(path, project / path.name)
    info = facts(project / "assets.fma")
    assemble_source(info, project)
    for name, value in {
        "weight": 75,
        "gravity": 0.075,
        "terminal_velocity": 1.8,
        "dash_initial_velocity": 1.55,
        "dash_max_velocity": 1.5,
        "air_drift_max": 1.12,
        "max_jumps": 3,
    }.items():
        run("project-attribute", project, name, value)
    author_animations(project, info)
    costume = Dat((project / "costume.dat").read_bytes())
    rig = joints(costume, costume.roots[info["joint"]])
    make_art(output, rig)
    run("project-model-texture", project, 0, output / "glim_body.png")
    run("project-model-import", project, output / "halo.glb", 0, 6, "add", 1, "rigid")
    run("project-hitbox-intervals", project)
    run("source-audit", project, project / "behavior.fms")
    run("script-build", project, project / "behavior.fms")
    shutil.copy2(output / "portrait.png", project / "portrait.png")
    if not args.no_install:
        target = Path(os.environ["LOCALAPPDATA"]) / "Freak Melee/projects/Glim"
        if target.exists() and not args.replace_draft:
            raise RuntimeError(
                f"Preserved existing editable draft: {target}. Use --replace-draft explicitly."
            )
        if target.exists():
            # This exact authoring destination belongs to this recipe; preserve
            # the old draft before replacement so user edits remain recoverable.
            backup = output / "previous-draft"
            if backup.exists():
                raise RuntimeError(f"An earlier draft backup already exists: {backup}")
            shutil.copytree(target, backup)
        shutil.copytree(
            project,
            target,
            dirs_exist_ok=True,
            ignore=shutil.ignore_patterns("*.previous"),
        )
        print(f"Editable Workshop draft: {target}")


if __name__ == "__main__":
    main()
