"""Extract stock resource-count inputs and record rounded-f32 scale census.

No DAT/assets are copied. The receipt records input/source hashes and the
explicit fixed-pool policy, not a promise for arbitrary custom attributes.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
from extract_native_animation import extract_clip


def f32(value):
    try:
        return struct.unpack(">f", struct.pack(">f", value))[0]
    except OverflowError:
        return math.copysign(math.inf, value)


def derived_count(count, segment, blend, coefficient, scale, doubled):
    scale = f32(scale)
    length = f32(segment * scale)
    if length <= 0 or not math.isfinite(length):
        return None
    if scale > 1:
        a = f32(count * length)
        b = f32(count * segment)
        total = f32(f32(blend * a) + f32(f32(1 - blend) * b))
    else:
        total = f32(count * length)
    value = f32(f32(total * coefficient) / length)
    if not math.isfinite(value):
        return None
    return int(value) * (2 if doubled else 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    entries = [("Link", "PlLk.dat", "ftDataLink", 2, 0x4C, False),
               ("Young Link", "PlCl.dat", "ftDataClink", 2, 0x4C, False),
               ("Samus", "PlSs.dat", "ftDataSamus", 3, 0x5C, True)]
    scales = sorted(set([2.0 ** n for n in range(-149, 128)] +
                        [0.1, 0.5, 0.99, 1.0, 1.00001, 1.5, 2.0, 8.0]))
    receipt = {"collections": 6, "nodes_per_collection": 90,
               "policy": "reject count<2, count>declared capacity, invalid/nonfinite arithmetic; never clamp",
               "scope": "retail attributes; sampled f32 positive scales including subnormal/extreme scales",
               "fighters": [], "source_sha256": {}}
    for name, dat, root_name, slot, coefficient_offset, can_double in entries:
        data, root = extract_clip(args.iso, dat, root_name)
        u32 = lambda offset: struct.unpack_from(">I", data, offset + 32)[0]
        flt = lambda offset: struct.unpack_from(">f", data, offset + 32)[0]
        article = u32(u32(root + 72) + slot * 4)
        attrs = u32(article + 4)
        inputs = dict(count=u32(attrs + 12), segment=flt(attrs + 16),
                      blend=flt(attrs + 8), ground=flt(attrs + coefficient_offset),
                      air=flt(attrs + coefficient_offset + 4))
        cases = []
        for scale in scales:
            for mode in ("ground", "air"):
                for doubled in (False, True) if can_double else (False,):
                    count = derived_count(inputs["count"], inputs["segment"],
                                          inputs["blend"], inputs[mode], scale, doubled)
                    cases.append(dict(scale=scale, mode=mode, doubled=doubled,
                                      count=count, accepted=count is not None and 2 <= count <= 90))
        valid = [c["count"] for c in cases if c["count"] is not None]
        small = [c["count"] for c in cases if c["scale"] < 1 and c["count"] is not None]
        receipt["fighters"].append(dict(name=name, dat=dat, root=root_name,
            dat_sha256=hashlib.sha256(data).hexdigest(), attrs_offset=attrs,
            inputs=inputs, maximum=max(valid), small_scale_maximum=max(small), cases=cases))
    for path in ("src/melee/it/kinds/itlinkhookshot.c",
                 "src/melee/it/kinds/itsamusgrapple.c",
                 "src/melee/it/native_article_resource.c",
                 "src/melee/it/native_article_resource.h"):
        receipt["source_sha256"][path] = hashlib.sha256((REPO / path).read_bytes()).hexdigest()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    output = args.out_dir / "receipt.json"
    output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"receipt": str(output.resolve()), "maxima":
          {f["name"]: [f["maximum"], f["small_scale_maximum"]] for f in receipt["fighters"]}}))


if __name__ == "__main__":
    main()
