"""Recorded capture, counter, reflector and worn-item interactions.

Only pad generation is reused from historical suites. The shared roster runner
creates fresh interpreted packages, checks explicit coverage and collects errors.
"""
import argparse
import json
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
from marth_roy_special_parity import (
    contact_recording, counter_recording, melee_counter_recording,
    peach_counter_recording,
)
from mewtwo_special_parity import recording

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    out = args.out_dir.resolve()
    if out.drive.upper() != "A:" or out.exists():
        parser.error("Use a fresh private A: output directory")
    out.mkdir(parents=True)
    cases = []

    def add(fighter, name, motions, extra):
        cases.append(dict(fighter=fighter, scenario=name,
                          recording=fighter + "-" + name + ".fmr",
                          arguments=[v for m in motions for v in ("--expect-motion", str(m))] + extra))

    for fighter, title in (("marth", "Marth"), ("roy", "Roy")):
        for mode in ("ground", "air"):
            name = "counter-" + mode + "-laser"
            counter_recording(out / (fighter + "-" + name + ".fmr"), mode, True)
            add(fighter, name, [371 if mode == "air" else 369, 370],
                ["--p2", "human:fox"])
        name = "counter-ground-melee"
        melee_counter_recording(out / (fighter + "-" + name + ".fmr"), title)
        add(fighter, name, [369, 370], ["--expect-damage-port", "2"])
        name = "cross-peach-counter"
        peach_counter_recording(out / (fighter + "-" + name + ".fmr"), title)
        add(fighter, name, [369, 370], ["--p2", "human:peach", "--expect-port-motion", "2", "359"])
        for up in (False, True):
            name = "up-contact" if up else "neutral-contact"
            contact_recording(out / (fighter + "-" + name + ".fmr"), up, fighter == "marth")
            add(fighter, name, [367] if up else [345, 347], ["--expect-damage-port", "2"])

    base = bytearray(BASE.read_bytes())
    struct.pack_into("<I", base, 16, 0x6D657774)
    targets = (
        ("shadow-air-short", "short-b", True, False, "idle", [346, 347]),
        ("shadow-shield", "short-b", False, False, "shield", [345]),
        ("shadow-reflector", "short-b", False, False, "reflect", [345]),
        ("disable-ground", "disable", False, False, "idle", [359]),
        ("disable-ground-left", "disable", False, True, "idle", [359]),
        ("disable-air-land", "disable", True, False, "idle", [360]),
        ("disable-shield", "disable", False, False, "shield", [359]),
        ("disable-target", "disable", False, False, "target", [359]),
        ("confusion-ground-capture", "confusion", False, False, "capture", [351]),
        ("confusion-ground-left-capture", "confusion", False, True, "capture", [351]),
        ("confusion-air-capture", "confusion", True, False, "capture", [352]),
        ("confusion-shield-capture", "confusion", False, False, "capture-shield", [351]),
        ("confusion-air-land", "confusion-land", True, False, "idle", [352, 351]),
        ("confusion-air-repeat", "confusion-repeat", True, False, "idle", [352]),
        ("confusion-reflect", "confusion-reflect", False, False, "confusion-reflect", [351]),
        ("throw-forward-right", "throw-forward", False, False, "grab-target", [212, 219]),
        ("throw-forward-left", "throw-forward", False, True, "grab-target", [212, 219]),
        ("worn-bunny-double-jump", "worn-normal", False, False, "hood", [27]),
        ("held-screw-double-jump", "worn-screw", False, False, "screw", [154, 155]),
        ("worn-bunny-screw-double-jump", "worn-both", False, False, "both", [154, 155]),
    )
    for name, variant, air, left, defense, motions in targets:
        (out / ("mewtwo-" + name + ".fmr")).write_bytes(recording(base, 700, variant, air, left, defense))
        extra = []
        if air:
            height = 10 if variant == "confusion-land" else 150 if variant == "confusion-repeat" else 120
            extra += ["--fixture-airborne", f"1,175,-30,{height}"]
        if variant == "confusion-repeat":
            extra += ["--fixture-airborne", "1,390,-30,120"]
            for begin, end in ((180, 220), (235, 280), (400, 450)):
                extra += ["--expect-motion-window", "1", "352", str(begin), str(end)]
        if defense.startswith("capture"):
            pos = "2,175,-18,145" if air else "2,140,-77,6" if left else "2,140,-48,6"
            extra += ["--fixture-airborne", pos, "--expect-port-motion", "2", "304" if air else "303",
                      "--expect-port-motion", "2", "38", "--expect-damage-port", "2"]
        if defense == "grab-target":
            extra += ["--fixture-airborne", "2,140,-72,6" if left else "2,140,-48,6",
                      "--expect-port-motion", "2", "239", "--expect-damage-port", "2",
                      "--expect-item-kind", "112"]
        if defense == "confusion-reflect":
            extra += ["--p2", "human:mario", "--fixture-airborne", "2,120,0,6",
                      "--expect-item-kind", "48", "--expect-item-direction-change", "48", "2"]
        if variant in ("short-b", "disable"):
            extra += ["--expect-item-kind", "110" if variant == "disable" else "112"]
        if defense == "reflect":
            extra += ["--expect-item-owner-transfer", "112", "1", "2"]
        if defense == "shield":
            extra += ["--expect-port-motion", "2", "181"]
        if variant == "disable" and defense in ("shield", "target"):
            extra += ["--fixture-airborne", "2,140,-35,6"]
        if defense == "target":
            extra += ["--expect-port-motion", "2", "300", "--expect-damage-port", "2"]
        if variant.startswith("worn-"):
            kinds = (31, 30) if defense == "both" else (31,) if defense == "hood" else (30,)
            for kind in kinds:
                tick = 100 if kind == 31 else 170 if defense == "both" else 130
                extra += ["--fixture-item", f"{kind},{tick},-60,3", "--expect-item-kind", str(kind),
                          "--expect-item-state", str(kind), "2", "1", "261", "270"]
        add("mewtwo", name, motions, extra)

    manifest = dict(cases=cases, input_files=list(map(str, (Path(__file__), BASE,
        ROOT / "tests/native/marth_roy_special_parity.py", ROOT / "tests/native/mewtwo_special_parity.py"))),
        boundaries=["Capture and aerial placement use explicit offline fixtures.",
                    "Worn items are explicitly spawned, then collected through controller input.",
                    "Every selected case requires actual move or interaction coverage."])
    (out / "matrix.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(dict(matrix=str(out / "matrix.json"), cases=len(cases))))


if __name__ == "__main__":
    main()
