"""Extract the original executable and embedded byte-only font atlases.

The source symbols define atlas locations; no copyrighted data is stored here.
Other native asset decoding is separate from this build-time extraction.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


FONT_ATLASES = (
    ("HSD_DebugFontAtlas", "debug_font", 56),
    ("HSD_SisLib_FontAtlas", "sislib_font", 512),
)


def read_exact(stream, offset, size):
    stream.seek(offset)
    data = stream.read(size)
    if len(data) != size:
        raise ValueError(f"Truncated ISO: need {size} bytes at 0x{offset:X}")
    return data


def extract_dol(iso_path, expected_hash):
    with iso_path.open("rb") as stream:
        header = read_exact(stream, 0, 0x440)
        if header[:6] != b"GALE01" or header[7] != 2:
            raise ValueError("Expected the USA v1.02 Melee disc (GALE01 revision 2)")
        if struct.unpack_from(">I", header, 0x1C)[0] != 0xC2339F3D:
            raise ValueError("Expected an uncompressed GameCube ISO")
        dol_offset = struct.unpack_from(">I", header, 0x420)[0]
        dol_header = read_exact(stream, dol_offset, 0x100)
        offsets = struct.unpack_from(">18I", dol_header, 0)
        addresses = struct.unpack_from(">18I", dol_header, 0x48)
        sizes = struct.unpack_from(">18I", dol_header, 0x90)
        sections = [(o, a, s) for o, a, s in zip(offsets, addresses, sizes) if s]
        if not sections or any(o < 0x100 for o, _, _ in sections):
            raise ValueError("Invalid DOL section table")
        extent = max(o + s for o, _, s in sections)
        if dol_offset + extent > iso_path.stat().st_size:
            raise ValueError("DOL sections extend beyond the disc image")
        dol = read_exact(stream, dol_offset, extent)
    actual_hash = hashlib.sha1(dol).hexdigest()
    if actual_hash != expected_hash:
        raise ValueError(f"Original main.dol SHA-1 mismatch: {actual_hash}")
    return dol, sections


def extract_symbol(dol, sections, symbols, name):
    match = re.search(
        rf"^{re.escape(name)} = \.data:0x([0-9A-Fa-f]+);.*size:0x([0-9A-Fa-f]+)",
        symbols,
        re.MULTILINE,
    )
    if not match:
        raise ValueError(f"No sized data symbol for {name}")
    address, size = (int(value, 16) for value in match.groups())
    for offset, base, length in sections:
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return dol[start:start + size]
    raise ValueError(f"{name} does not fit within a DOL section")


def write_if_changed(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    config = (root / "config/GALE01/config.yml").read_text()
    expected_hash = re.search(r"^hash: ([0-9a-f]{40})$", config, re.MULTILINE)
    if not expected_hash:
        raise ValueError("Missing reference executable hash in config.yml")
    dol, sections = extract_dol(args.iso, expected_hash.group(1))
    symbols = (root / "config/GALE01/symbols.txt").read_text()
    report = {"dol_sha1": expected_hash.group(1), "fonts": {}}
    outputs = {args.output / "main.dol": dol}
    for symbol, filename, glyph_size in FONT_ATLASES:
        data = extract_symbol(dol, sections, symbols, symbol)
        if len(data) % glyph_size:
            raise ValueError(f"{symbol} has a partial glyph")
        # Both atlas structures contain byte arrays only, so preserve byte order.
        lines = ["/* Extracted from the verified original main.dol. */"]
        for start in range(0, len(data), glyph_size):
            lines.append("{{")
            for row in range(start, start + glyph_size, 16):
                values = data[row:min(row + 16, start + glyph_size)]
                lines.append("    " + ", ".join(f"0x{x:02X}" for x in values) + ",")
            lines.append("}},")
        outputs[args.output / "include/sysdolphin/baselib" / (filename + ".inc")] = (
            "\n".join(lines) + "\n"
        ).encode("ascii")
        report["fonts"][symbol] = {
            "bytes": len(data),
            "glyphs": len(data) // glyph_size,
            "sha256": hashlib.sha256(data).hexdigest(),
        }
    outputs[args.output / "extraction.json"] = (json.dumps(report, indent=2) + "\n").encode()
    # Validate every input before producing any outputs.
    for path, data in outputs.items():
        write_if_changed(path, data)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
