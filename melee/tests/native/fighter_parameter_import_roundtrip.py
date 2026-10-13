"""Copy stock constants bit-exactly into owned values and package them."""

import hashlib
import pathlib
import re
import struct
import sys
import tempfile

from fighter_script_bundle_roundtrip import run, portrait, bundled_id


def attributes(dat, root_name, field_offset=4):
    data_size, relocations, roots = struct.unpack_from(">III", dat, 4)
    root_table = 32 + data_size + relocations * 4
    externs = struct.unpack_from(">I", dat, 16)[0]
    strings = root_table + (roots + externs) * 8
    for index in range(roots):
        target, name = struct.unpack_from(">II", dat, root_table + index * 8)
        name = dat[strings + name:].split(b"\0", 1)[0].decode()
        if name == root_name:
            return 32 + struct.unpack_from(">I", dat, 32 + target + field_offset)[0]
    raise AssertionError(root_name)


def main():
    tool, iso, source_path, reflector_path = sys.argv[1:]
    source = pathlib.Path(source_path).read_text()
    declarations = re.findall(r"^attribute (float|int) (\w+) (\d+)$",
                              source, re.MULTILINE)
    defaults = re.findall(r"^parameter (float|int|bool) (\w+) = (.+)$",
                          source, re.MULTILINE)
    with tempfile.TemporaryDirectory(prefix="freak-parameters-") as temp:
        root = pathlib.Path(temp)
        project, cache = root / "Luigi", root / "cache"
        converted, image = root / "owned.fms", root / "portrait.png"
        portrait(image)
        run(tool, "clone-stock", iso, "Luigi", project, "Owned Luigi")
        run(tool, "import-parameters", project, source_path, converted)
        assert not re.search(r"^attribute ", converted.read_text(), re.MULTILINE)
        params = project / "fighter_parameters.fmp"
        values = dict((name, (typ, value)) for typ, name, value in
                      re.findall(r"^(float|int|bool) ([\w.]+) = (.+)$",
                                 params.read_text(), re.MULTILINE))
        dat = (project / "fighter.dat").read_bytes()
        base = attributes(dat, "ftDataLuigi")
        assert declarations
        assert {name for name in values if "." not in name} == \
               {name for _, name, _ in declarations + defaults}
        for typ, name, value in defaults:
            actual_type, actual = values[name]
            assert actual_type == typ
            if typ == "float":
                assert struct.pack(">f", float(actual)) == struct.pack(">f", float(value)), name
            elif typ == "int":
                assert int(actual) == int(value), name
            else:
                assert actual == value, name
        for typ, name, offset in declarations:
            value_type, value = values[name]
            assert typ == value_type
            bits = struct.pack(">f", float(value)) if typ == "float" else \
                   struct.pack(">i", int(value))
            assert bits == dat[base + int(offset):base + int(offset) + 4], name
        run(tool, "script-build", project, converted)
        first = bundled_id(run(tool, "--root", cache, "bundle-project",
                               project, image, iso))
        module = project / "module.dll"
        module_hash = hashlib.sha256(module.read_bytes()).digest()
        text = params.read_text()
        changed = re.sub(r"^float missile_launch_speed = .+$",
                         "float missile_launch_speed = 3.25", text,
                         flags=re.MULTILINE)
        assert changed != text
        params.write_text(changed)
        second = bundled_id(run(tool, "--root", cache, "bundle-project",
                                project, image, iso))
        assert first != second
        assert hashlib.sha256(module.read_bytes()).digest() == module_hash
        run(tool, "--root", cache, "verify-package", second, iso)
        params.write_text(changed.replace("missile_launch_speed =",
                                          "renamed_speed =", 1))
        run(tool, "--root", cache, "package-project", project, image,
            succeeds=False)
    # The reflector descriptor ends in a byte, unlike the word-valued tuning.
    # Exercise the real source layout and own the flag as an editable bool.
    with tempfile.TemporaryDirectory(prefix="freak-reflector-parameters-") as temp:
        root = pathlib.Path(temp)
        project, converted = root / "Fox", root / "owned.fms"
        run(tool, "clone-stock", iso, "Fox", project, "Owned Fox")
        run(tool, "import-parameters", project, reflector_path, converted)
        dat = (project / "fighter.dat").read_bytes()
        base = attributes(dat, "ftDataFox")
        declarations = re.findall(r"^attribute (float|int|bool) (\w+) (\d+)$",
                                  pathlib.Path(reflector_path).read_text(), re.MULTILINE)
        values = dict((name, (typ, value)) for typ, name, value in
                      re.findall(r"^(float|int|bool) ([\w.]+) = (.+)$",
                          (project / "fighter_parameters.fmp").read_text(), re.MULTILINE))
        defaults = re.findall(r"^parameter (float|int|bool) (\w+) = (.+)$",
                              pathlib.Path(reflector_path).read_text(), re.MULTILINE)
        assert len(declarations) == 47
        assert {name for name in values if "." not in name} == \
               {name for _, name, _ in declarations + defaults}
        for typ, name, value in defaults:
            actual_type, actual = values[name]
            assert actual_type == typ
            if typ == "float":
                assert struct.pack(">f", float(actual)) == struct.pack(">f", float(value)), name
            elif typ == "int":
                assert int(actual) == int(value), name
            else:
                assert actual == value, name
        article_declarations = {
            "It_Kind_Fox_Laser": (0, {"lifetime": 0, "maximum_length": 4}),
            "It_Kind_Fox_Illusion": (2, {"trail_lifetime": 0, "fade_lifetime": 4}),
        }
        # Read the original raw article table, independent of the C importer.
        data_size, relocations, roots, externs = struct.unpack_from(">IIII", dat, 4)
        root_table = 32 + data_size + relocations * 4
        strings = root_table + (roots + externs) * 8
        for i in range(roots):
            target, name = struct.unpack_from(">II", dat, root_table + i * 8)
            if dat[strings + name:].split(b"\0", 1)[0] == b"ftDataFox":
                article_table = 32 + struct.unpack_from(">I", dat, 32 + target + 72)[0]
                break
        for kind, (slot, fields) in article_declarations.items():
            descriptor = 32 + struct.unpack_from(">I", dat, article_table + slot * 4)[0]
            tuning = 32 + struct.unpack_from(">I", dat, descriptor + 4)[0]
            for field, offset in fields.items():
                typ, value = values[kind + "." + field]
                assert typ == "float"
                assert struct.pack(">f", float(value)) == dat[tuning + offset:tuning + offset + 4]
        assert len(values) == 51 + len(defaults)
        owned_source = converted.read_text()
        assert "article It_Kind_Falco_" not in owned_source
        assert "item.attr." not in owned_source
        assert not re.search(r"^    attribute ", owned_source, re.MULTILINE)
        for typ, name, offset in declarations:
            value_type, value = values[name]
            assert value_type == typ
            bits = bytes([value == "true"]) if typ == "bool" else \
                   struct.pack(">f", float(value)) if typ == "float" else \
                   struct.pack(">i", int(value))
            assert bits == dat[base + int(offset):base + int(offset) + len(bits)], name
        assert not re.search(r"^attribute ", converted.read_text(), re.MULTILINE)
        run(tool, "script-build", project, converted)
        generated = (project / "behavior.generated.c").read_text()
        assert "x4_specialAttributes" not in generated
        # Packed colors and other unsigned byte fields become ordinary owned
        # integers, including unaligned source offsets and values above one.
        byte_source = root / "bytes.fms"
        byte_project = root / "ByteFox"
        byte_converted = root / "bytes_owned.fms"
        run(tool, "clone-stock", iso, "Fox", byte_project, "Owned Bytes")
        byte_source.write_text("\n".join(
            f"attribute byte channel_{i} {i}" for i in range(16)) +
            "\naction ReadBytes:\n    on load:\n        velocity.x = attrs.channel_1\n")
        run(tool, "import-parameters", byte_project, byte_source, byte_converted)
        byte_values = dict(re.findall(r"^int (\w+) = (\d+)$",
            (byte_project / "fighter_parameters.fmp").read_text(), re.MULTILINE))
        assert len(byte_values) == 16
        assert any(dat[base + i] > 1 for i in range(16))
        for i in range(16):
            assert int(byte_values[f"channel_{i}"]) == dat[base + i]
        run(tool, "script-build", byte_project, byte_converted)
        byte_module = (byte_project / "behavior.generated.c").read_text()
        assert "x4_specialAttributes" not in byte_module
        (byte_project / "fighter_parameters.fmp").unlink()
        for declaration in ("attribute byte beyond 65536", "attribute byte negative -1",
                            "attribute bool not_boolean 0"):
            if declaration.endswith("0") and dat[base] <= 1:
                declaration = f"attribute bool not_boolean {next(i for i in range(16) if dat[base+i] > 1)}"
            byte_source.write_text(declaration + "\n")
            rejected = root / "rejected.fms"
            run(tool, "import-parameters", byte_project, byte_source, rejected, succeeds=False)
            assert not rejected.exists()
            assert not (byte_project / "fighter_parameters.fmp").exists()
    # Every currently translated article family uses the same ownership path.
    examples = pathlib.Path(source_path).parent
    for fighter, template, slots in (
        ("Mario", "mario_full.fms", {"It_Kind_Mario_Fire": 0}),
        ("DrMario", "drmario_full.fms", {"It_Kind_DrMario_Vitamin": 1}),
        ("Falco", "falco_wip.fms", {"It_Kind_Falco_Laser": 0, "It_Kind_Falco_Phantasm": 3}),
    ):
        with tempfile.TemporaryDirectory(prefix="freak-article-values-") as temp:
            root = pathlib.Path(temp)
            project, converted = root / fighter, root / "owned.fms"
            run(tool, "clone-stock", iso, fighter, project, "Owned " + fighter)
            if fighter == "Falco":
                invalid = root / "invalid.fms"
                invalid.write_text((examples / template).read_text().replace(
                    "    attribute float lifetime 0", "    attribute float lifetime 1024"))
                run(tool, "import-parameters", project, invalid, converted, succeeds=False)
                assert not converted.exists()
                assert not (project / "fighter_parameters.fmp").exists()
            run(tool, "import-parameters", project, examples / template, converted)
            dat = (project / "fighter.dat").read_bytes()
            table = attributes(dat, "ftDataDrmario" if fighter == "DrMario" else "ftData" + fighter, 72)
            values = dict((name, (typ, value)) for typ, name, value in
                re.findall(r"^(float|int|bool) ([\w.]+) = (.+)$",
                    (project / "fighter_parameters.fmp").read_text(), re.MULTILINE))
            for kind, body in re.findall(r"^article (\w+) states \d+:\n(.*?)(?=^article |\Z)",
                    (examples / template).read_text(), re.MULTILINE | re.DOTALL):
                if kind not in slots:
                    continue
                descriptor = 32 + struct.unpack_from(">I", dat, table + slots[kind] * 4)[0]
                tuning = 32 + struct.unpack_from(">I", dat, descriptor + 4)[0]
                for typ, name, offset in re.findall(r"^    attribute (float|int|bool) (\w+) (\d+)$", body, re.MULTILINE):
                    actual_type, value = values[kind + "." + name]
                    assert actual_type == typ
                    bits = bytes([value == "true"]) if typ == "bool" else \
                        struct.pack(">f", float(value)) if typ == "float" else \
                        struct.pack(">i", int(value))
                    assert bits == dat[tuning + int(offset):tuning + int(offset) + len(bits)]
            run(tool, "script-build", project, converted)
            assert "x4_specialAttributes" not in (project / "behavior.generated.c").read_text()
    # Template defaults are owned parameters, with no guessed DAT address.
    with tempfile.TemporaryDirectory(prefix="freak-literal-parameters-") as temp:
        root = pathlib.Path(temp)
        project, converted = root / "Mario", root / "owned.fms"
        source = root / "defaults.fms"
        run(tool, "clone-stock", iso, "Mario", project, "Owned Defaults")
        valid = ("parameter float threshold = 0.550000012\n"
                 "parameter int charges = 3\n"
                 "parameter bool charged = true\n"
                 "action Init:\n    on load:\n        velocity.x = attrs.threshold\n")
        for invalid in (valid + "parameter float threshold = 1\n",
                        valid.replace("charges = 3", "charges = 2147483648"),
                        valid.replace("charged = true", "charged = 2"),
                        valid.replace("threshold = 0.550000012", "threshold = nan")):
            source.write_text(invalid)
            run(tool, "import-parameters", project, source, converted, succeeds=False)
            assert not converted.exists()
            assert not (project / "fighter_parameters.fmp").exists()
        source.write_text(valid)
        run(tool, "import-parameters", project, source, converted)
        params = (project / "fighter_parameters.fmp").read_text()
        assert "int charges = 3" in params and "bool charged = true" in params
        value = re.search(r"float threshold = (.+)", params)[1]
        assert struct.pack(">f", float(value)) == struct.pack(">f", 0.55)
        assert "parameter " not in converted.read_text()
        run(tool, "script-build", project, converted)
    print("Owned fighter/article defaults match source bits; parameter edits "
          "bind without rebuilding; schema edits rejected")



if __name__ == "__main__":
    main()
