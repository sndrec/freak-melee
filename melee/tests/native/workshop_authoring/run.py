"""Exercise authoring transactions against a disposable copy of a real draft.

Usage: python run.py --tool native_content_tool.exe --project DRAFT --halo halo.glb
                    --output DIRECTORY
The source draft is read-only. Each run creates a fresh output subdirectory.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time


class Dat:
    def __init__(self, path):
        self.raw = Path(path).read_bytes()
        _, size, relocs, roots, externs = struct.unpack_from(">5I", self.raw)
        self.data = self.raw[32:32 + size]
        table = 32 + size + relocs * 4
        strings = table + (roots + externs) * 8
        self.roots = {}
        for i in range(roots):
            offset, name = struct.unpack_from(">2I", self.raw, table + i * 8)
            end = self.raw.index(0, strings + name)
            self.roots[self.raw[strings + name:end].decode()] = offset

    def u32(self, offset):
        return struct.unpack_from(">I", self.data, offset)[0]

    def u16(self, offset):
        return struct.unpack_from(">H", self.data, offset)[0]

    def action(self, index):
        root = next(iter(self.roots.values()))
        record = self.u32(root + 12) + index * 24
        name = self.u32(record)
        end = self.data.index(0, name)
        return (self.data[name:end].decode(), self.u32(record + 4),
                self.u32(record + 8), self.data[record + 12:record + 24])

    def model(self):
        meshes, materials, joints = [], [], []

        def walk(joint):
            while joint:
                joints.append(joint)
                if not self.u32(joint + 4) & ((1 << 5) | (1 << 14)):
                    mesh = self.u32(joint + 16)
                    while mesh:
                        meshes.append(mesh)
                        mat = self.u32(mesh + 8)
                        if mat and mat not in materials:
                            materials.append(mat)
                        mesh = self.u32(mesh + 4)
                walk(self.u32(joint + 8))
                joint = self.u32(joint + 12)

        joint = next(v for k, v in self.roots.items() if k.endswith("_joint") and "matanim" not in k.lower())
        walk(joint)
        return joints, meshes, materials

    def material_animations(self):
        result = {}
        def walk(joint, anim):
            while joint and anim:
                mesh, mat = self.u32(joint + 16), self.u32(anim + 8)
                while mesh:
                    result[mesh] = (self.u32(mat + 4), self.u32(mat + 8)) if mat else (0, 0)
                    mesh = self.u32(mesh + 4)
                    mat = self.u32(mat) if mat else 0
                walk(self.u32(joint + 8), self.u32(anim))
                joint, anim = self.u32(joint + 12), self.u32(anim + 4)
        joint = next(v for k, v in self.roots.items() if k.endswith("_joint") and "matanim" not in k.lower())
        anim = next(v for k, v in self.roots.items() if "matanim" in k.lower())
        walk(joint, anim)
        return result

    def textures(self):
        result = []
        for mesh in self.model()[1]:
            material = self.u32(mesh + 8)
            texture = self.u32(material + 8) if material else 0
            while texture:
                result.append((mesh, texture))
                texture = self.u32(texture + 4)
        return result

    def selectors(self, costume):
        """Resolve numeric reference tables to object identities."""
        meshes, textures = costume.model()[1], costume.textures()
        model = self.u32(next(iter(self.roots.values())) + 8)
        visibility, rows = self.u32(model + 4), self.u32(model + 12)
        groups, count = self.u32(model), self.u32(model + 8)
        result = {}
        # The clone's installed costume uses variant zero; other variant rows
        # can share these arrays and are preserved by the editor's remapping.
        for layer in (0, 1, 3):
            lookup = self.u32(visibility + layer * 4)
            if not lookup:
                continue
            for group in range(groups):
                variants, array = self.u32(lookup + group * 8), self.u32(lookup + group * 8 + 4)
                for variant in range(variants):
                    size, indices = self.u32(array + variant * 8), self.u32(array + variant * 8 + 4)
                    for i in range(size):
                        index = self.data[indices + i]
                        result[(layer, group, variant, i)] = meshes[index] if index < len(meshes) else ("extra", index - len(meshes))
        indices = self.u32(rows)
        for i in range(count):
            index = self.u16(indices + i * 2)
            result[("texture", i)] = textures[index] if index < len(textures) else ("extra", index - len(textures))
        return result


def glb_pair(source, destination, invalid=False):
    raw = source.read_bytes()
    size, kind = struct.unpack_from("<II", raw, 12)
    assert kind == 0x4e4f534a
    data = json.loads(raw[20:20 + size])
    mesh = copy.deepcopy(data["meshes"][0])
    data["meshes"][0]["name"] = "First halo"
    mesh["name"] = "Second halo"
    if invalid:
        mesh["primitives"][0]["attributes"].pop("NORMAL")
    data["meshes"].append(mesh)
    data["materials"][0]["name"] = "Halo coating"
    data["nodes"].append({"mesh": len(data["meshes"]) - 1})
    data["scenes"][0]["nodes"].append(len(data["nodes"]) - 1)
    encoded = json.dumps(data).encode()
    encoded += b" " * (-len(encoded) % 4)
    rest = raw[20 + size:]
    out = struct.pack("<5I", 0x46546c67, 2, 20 + len(encoded) + len(rest),
                      len(encoded), 0x4e4f534a) + encoded + rest
    destination.write_bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("tool", "project", "halo", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    out = args.output / time.strftime("run-%Y%m%d-%H%M%S")
    out.mkdir(parents=True)
    receipts = []

    def command(*words, fails=False):
        result = subprocess.run([str(args.tool.resolve()), *map(str, words)],
                                capture_output=True, text=True, timeout=120)
        receipts.append({"command": list(map(str, words)), "exit": result.returncode,
                         "stdout": result.stdout, "stderr": result.stderr})
        assert (result.returncode != 0) == fails, receipts[-1]
        return result.stdout

    def draft(name):
        target = out / name
        shutil.copytree(args.project, target, ignore=shutil.ignore_patterns(
            "*.previous", "installed.id"))
        return target

    def hashes(path):
        return {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                for p in path.iterdir() if p.is_file() and not p.name.endswith(".previous")}

    clips = draft("clips")
    original = Dat(clips / "fighter.dat")
    source = original.action(72)
    destination = original.action(69)
    command("project-clip-copy", clips, "action", 72, 69, "Independent")
    changed = Dat(clips / "fighter.dat")
    assert changed.action(72) == source
    assert changed.action(69)[3] == destination[3]
    assert changed.action(69)[1] != source[1]
    # Value edits must stay in the new clip, despite shared source data.
    before = command("project-animation-sample", clips, 72, 0, 0)
    command("project-animation", clips, 69, 0, 0, "0.25")
    assert command("project-animation-sample", clips, 72, 0, 0) == before
    # Make a fresh copy for saved undo and unrelated attribute preservation.
    undo = draft("undo")
    attr = command("project-attribute", undo, "gravity")
    command("project-clip-copy", undo, "action", 72, 69, "UndoClip")
    command("project-attribute", undo, "gravity", "0.125")
    command("project-animation-undo", undo)
    assert Dat(undo / "fighter.dat").action(69) == destination
    assert "0.125" in command("project-attribute", undo, "gravity")
    command("project-clip-create", clips, 69, "NewMotion", 30)
    assert "frames=30" in command("project-animation", clips, 69)
    command("project-animation-insert", clips, 69, 0, 15, "0.5")
    assert "value 0.5" in command("project-animation-sample", clips, 69, 0, 15)
    old = hashes(clips)
    command("project-clip-create", clips, 69, "Bad name", 30, fails=True)
    assert hashes(clips) == old

    names = draft("names")
    original_hash = hashlib.sha256((names / "fighter.dat").read_bytes()).digest()
    joints, meshes, materials = Dat(names / "costume.dat").model()
    for category, key, label in (("bone", joints[4], "Body"),
                                 ("mesh", meshes[0], "Costume"),
                                 ("material", materials[0], "Cloth"),
                                 ("action", 72, "Comet kick")):
        command("project-label", names, category, key, label)
    command("project-hide-action", names, 69, 1)
    text = (names / "workshop_names.txt").read_text()
    assert all(s in text for s in ("Body", "Costume", "Cloth", "Comet kick", "hidden\t69"))
    command("inspect-project", names)
    assert hashlib.sha256((names / "fighter.dat").read_bytes()).digest() == original_hash

    valid, invalid = out / "two.glb", out / "bad-second.glb"
    glb_pair(args.halo, valid)
    glb_pair(args.halo, invalid, True)
    old = hashes(names)
    original_costume = Dat(names / "costume.dat")
    original_selectors = Dat(names / "fighter.dat").selectors(original_costume)
    original_animations = original_costume.material_animations()
    command("project-import-meshes", names, invalid, 4, 1, "rigid", fails=True)
    assert hashes(names) == old
    original_gravity = command("project-attribute", names, "gravity")
    command("project-attribute", names, "gravity", "0.125")
    command("project-import-meshes", names, valid, 4, 1, "rigid")
    imported = Dat(names / "costume.dat")
    assert len(imported.model()[1]) == len(meshes) + 2
    assert Dat(names / "fighter.dat").selectors(imported) == original_selectors
    animated = imported.material_animations()
    assert all(animated[m] == value for m, value in original_animations.items())
    command("project-undo", names)
    assert command("project-attribute", names, "gravity") == original_gravity
    assert Dat(names / "fighter.dat").selectors(imported) == original_selectors
    text = (names / "workshop_names.txt").read_text()
    assert all(s in text for s in ("First halo", "Second halo", "Halo coating", "Body"))
    command("inspect-project", names)

    textures = draft("textures")
    output = command("project-texture-sequences", textures, 2)
    sequence, count = map(int, re.search(r"offset (\d+) layer \d+ images (\d+)", output).groups())
    original = Dat(textures / "costume.dat")
    table = original.u32(sequence + 12)
    entries = [original.u32(table + i * 4) for i in range(count)]
    command("project-texture-image", textures, 2, sequence, 1, args.project / "portrait.png")
    changed = Dat(textures / "costume.dat")
    assert changed.data[sequence:sequence + 24] == original.data[sequence:sequence + 24]
    assert changed.u32(table + 4) != entries[1]
    assert all(changed.u32(table + i * 4) == entries[i] for i in range(count) if i != 1)
    # The sampler, UV transforms and render properties stay intact.
    _, _, mats = changed.model()
    material = next(i for i, mat in enumerate(mats) if changed.u32(mat + 8))
    texture = changed.u32(mats[material] + 8)
    assert texture
    sampler = changed.data[texture:texture + 76]
    command("project-model-texture", textures, material, args.project / "portrait.png")
    assert Dat(textures / "costume.dat").data[texture:texture + 76] == sampler
    material = next(i for i, mat in enumerate(mats) if not changed.u32(mat + 8))
    old = Dat(textures / "fighter.dat").selectors(Dat(textures / "costume.dat"))
    command("project-model-texture", textures, material, args.project / "portrait.png")
    assert Dat(textures / "fighter.dat").selectors(Dat(textures / "costume.dat")) == old
    old = hashes(textures)
    command("project-texture-image", textures, 2, sequence, count,
            args.project / "portrait.png", fails=True)
    assert hashes(textures) == old
    command("source-audit", names, names / "behavior.fms")
    command("source-audit", clips, clips / "behavior.fms")
    (out / "receipt.json").write_text(json.dumps({"passed": True, "operations": receipts}, indent=2))
    print(f"Authoring batch passed: {out / 'receipt.json'}")


if __name__ == "__main__":
    main()
