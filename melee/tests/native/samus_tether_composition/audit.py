"""Read actual normal-clone DAT assets and frozen Samus caller/source contracts."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Dat:
    def __init__(self, path):
        self.path = path
        self.blob = path.read_bytes()
        length, self.size, relocations, roots, references = struct.unpack_from(">5I", self.blob)
        assert length == len(self.blob)
        table = 32 + self.size
        self.relocations = set(struct.unpack_from(">I", self.blob, table + i * 4)[0] for i in range(relocations))
        table += relocations * 4
        strings = table + (roots + references) * 8
        self.roots = {}
        for i in range(roots):
            offset, name = struct.unpack_from(">2I", self.blob, table + i * 8)
            end = self.blob.index(0, strings + name)
            self.roots[self.blob[strings + name:end].decode("ascii")] = offset
        self.externals = {}
        for i in range(references):
            link, name = struct.unpack_from(">2I", self.blob, table + (roots + i) * 8)
            end = self.blob.index(0, strings + name)
            symbol = self.blob[strings + name:end].decode("ascii")
            while link != 0xFFFFFFFF:
                assert link not in self.externals
                self.externals[link] = symbol
                link = self.word(link)

    def word(self, offset):
        assert 0 <= offset <= self.size - 4
        return struct.unpack_from(">I", self.blob, 32 + offset)[0]

    def pointer(self, offset):
        value = self.word(offset)
        assert value == 0 or offset in self.relocations
        assert value == 0 or value < self.size
        return value

    def reference(self, offset):
        if offset in self.externals:
            return dict(kind="external", symbol=self.externals[offset], relocation_chain_word=self.word(offset))
        value = self.pointer(offset)
        return dict(kind="local" if value else "null", target=value)

    def count_joints(self, root):
        seen = set()
        def visit(offset):
            while offset:
                assert offset not in seen
                seen.add(offset)
                if not self.word(offset + 4) & (1 << 12):
                    visit(self.pointer(offset + 8))
                offset = self.pointer(offset + 12)
        visit(root)
        return len(seen)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", required=True, type=Path)
    parser.add_argument("--project", required=True, type=Path)
    parser.add_argument("--common", type=Path, default=ROOT / ".local/mewtwo_common_stock.dat")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    snapshot, project, output = args.snapshot.resolve(), args.project.resolve(), args.output.resolve()
    if output.exists() or snapshot == output or snapshot in output.parents or HERE in output.parents:
        parser.error("fresh output outside the immutable snapshot and source namespace required")
    manifest_path = snapshot / "freeze.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    for name, value in manifest.items():
        if name.replace("\\", "/").startswith("sdk/") and sha(snapshot / name) != value:
            raise RuntimeError("changed frozen input: " + name)
    fighter = Dat(project / "fighter.dat")
    root = fighter.roots["ftDataSamus"]
    items = fighter.pointer(root + 0x48)
    article = fighter.pointer(items + 3 * 4)
    attributes = fighter.pointer(article + 4)
    roots = [{"attribute_offset": offset, "source_root": fighter.pointer(attributes + offset)} for offset in range(0x64, 0x74, 4)]
    bundles = []
    for offset in range(0x74, 0xB0, 4):
        reference = fighter.reference(attributes + offset)
        array = reference.get("target", 0)
        graph = fighter.pointer(array) if array else 0
        bundles.append(dict(attribute_offset=offset, reference=reference, source_pointer_array=array, first_graph=graph,
                            source_declared_indirection=1))
    throw = fighter.pointer(items + 4 * 4)
    throw_array = fighter.pointer(throw + 4)
    throw_graphs = [fighter.pointer(throw_array + i * 4) for i in range(4)]
    costume = Dat(project / "costume.dat")
    costume_root = next(value for name, value in costume.roots.items() if name.endswith("_Share_joint"))
    physical = costume.count_joints(costume_root)
    common = Dat(args.common.resolve())
    common_root = common.roots["ftLoadCommonData"]
    table = common.pointer(common_root + 16)
    descriptor = common.pointer(table + 13 * 4)
    inverse, mapping, count = [common.word(descriptor + i * 4) for i in range(3)]
    semantics = list(common.blob[32 + mapping:32 + mapping + 54])
    raw_to_semantic = list(common.blob[32 + inverse:32 + inverse + count])
    assert physical == count == 60 and semantics[51] == 255 and raw_to_semantic[51] == 255
    sdk = snapshot / "sdk"
    paths = ["ft/kinds/ftSamus/ftsamus.c", "ft/kinds/ftSamus/types.h", "ft/kinds/ftSamus/inlines.h",
             "ft/kinds/ftSamus/ftsamusspecialn.c", "ft/kinds/ftSamus/ftsamusspecials.c", "it/kinds/itsamusgrapple.c",
             "it/itCharItems.h", "it/itarticle_data.c", "ft/ftcharacter_assets.c", "lb/types.h",
             "ft/kinds/ftCommon/ftCo_Throw.c", "ft/kinds/ftCommon/ftCo_0A01.c",
             "it/native_article_resource.h", "ft/native_fighter_runtime.h"]
    inputs = [Path(__file__).resolve(), manifest_path, project / "fighter.dat", project / "costume.dat", args.common.resolve()]
    references = []
    for relative in paths:
        path = sdk / "src/melee" / relative
        inputs.append(path)
        references.append(dict(path=relative, sha256=sha(path)))
    sites = []
    for path in sorted((sdk / "src/melee").rglob("*.c")):
        relative = path.relative_to(sdk / "src").as_posix()
        if "/ftKirby/" in relative or "/ftSamus/" in relative or "/itsamus" in relative:
            continue
        found = [dict(line=i + 1, text=line.strip()) for i, line in enumerate(path.read_text(encoding="utf-8").splitlines())
                 if re.search(r"\bFt_Kind_Samus\b|\bftSs_\w+|\bu\.ss\b|\bmv\.ss\b", line)]
        if found:
            inputs.append(path)
            sites.append(dict(path=relative, sha256=sha(path), sites=found))
    output.mkdir(parents=True)
    record = dict(snapshot=str(snapshot), snapshot_sha256=sha(manifest_path),
                  normal_project=str(project), fighter_root=root, item_table=items,
                  grapple_article_source=article, grapple_attribute_source=attributes,
                  article_model_roots=roots, article_animation_pointer_arrays=bundles,
                  throw_accessory=dict(source_descriptor=throw, joint=fighter.pointer(throw),
                                       animation_array=throw_array, animation_graphs=throw_graphs,
                                       base_joint_animation=fighter.pointer(throw + 8), base_material_animation=fighter.pointer(throw + 12)),
                  rig=dict(physical_joint_count=physical, runtime_part_count=count, semantic_map=semantics,
                           raw51_semantic=raw_to_semantic[51], semantic51_joint=semantics[51],
                           endpoint_reservation=139, reservation_adds_no_physical_joint=True),
                  exact_source_inputs=references, common_named_samus_sites=sites,
                  input_sha256={str(path):sha(path) for path in inputs},
                  limits=["Read-only original DAT relocation evidence; no engine import/model-render execution.",
                          "Slot4 is a typed accessory descriptor, not an article or plain joint graph.",
                          "Array dereference only reads index0, exactly as original grapple callbacks; unresolved external channels are reported separately.",
                          "Kind-name census is a review inventory; absence/presence alone does not prove caller independence."])
    (output / "inventory.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(output / "inventory.json")


if __name__ == "__main__":
    main()
