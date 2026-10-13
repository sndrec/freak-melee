"""Build private recordings for current interpreted special follow-through.

Reuses source-backed pad schedules, not their historical package/C backends.
The shared roster runner prepares fresh source packages and collects all errors.
"""
import argparse
import json
from pathlib import Path
import struct
import sys

sys.dont_write_bytecode = True
from marth_roy_special_parity import charge_recording, chain_recording
from marth_roy_closure_parity import expectations
from mewtwo_special_parity import recording as mewtwo_recording

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / "tests/native/parity_inputs"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    out = args.out_dir.resolve()
    if out.drive.upper() != "A:" or out.exists():
        parser.error("Use a fresh private A: output directory")
    out.mkdir(parents=True)
    cases = []
    pins = [Path(__file__), INPUTS / "README.md",
            INPUTS / "drmario-special-neutral-ground-inputs.fmr"]
    pins += [ROOT / "tests/native" / name for name in (
        "marth_roy_special_parity.py", "marth_roy_closure_parity.py",
        "mewtwo_special_parity.py")]

    def add(fighter, name, motions, arguments=(), recording=None):
        cases.append(dict(fighter=fighter, scenario=name, recording=recording or name + ".fmr",
                          arguments=["--expect-motion", str(motions[0])] +
                          [value for motion in motions[1:]
                           for value in ("--expect-motion", str(motion))] + list(arguments)))

    for mode, charges in (("ground", ("short", "full", "turn")),
                          ("air", ("short",))):
        for charge in charges:
            for facing in ("right", "left"):
                name = "rollout-" + charge + "-" + mode + "-" + facing
                original = INPUTS / ("purin-" + name + "-inputs.fmr")
                (out / (name + ".fmr")).write_bytes(original.read_bytes())
                pins.append(original)
                motions = [358 if mode == "air" else 350]
                if charge == "full": motions.append(349)
                if charge == "turn": motions.append(351)
                extra = []
                if mode == "ground" and charge != "turn" and facing == "right":
                    motions.append(362)
                    extra = ["--expect-damage-port", "2"]
                add("jigglypuff", name, motions, extra)

    for name, motions in (
        ("held-release", [343, 344, 347, 351]),
        ("max-charge", [343, 344, 347, 351]),
        ("misfire", [343, 344, 348, 351]),
        ("air-held-release", [349, 350, 344, 347, 351]),
    ):
        name = "green-missile-" + name
        original = INPUTS / ("luigi-" + name + ".fmr")
        raw = bytearray(original.read_bytes())
        if name == "green-missile-misfire":
            # Seed 3's historical sample-270 press now rolls a normal launch.
            # Advance P1's press/release nine frames within its preceding idle:
            # current stock RNG tracing identifies this natural misfire window.
            # Keep the original seed; neither runtime overrides the RNG result.
            original_bytes = bytes(raw)
            samples = (len(raw) - 24) // 80
            for frame in range(261, samples):
                src = 24 + (frame + 9) * 80 + 8
                dst = 24 + frame * 80 + 8
                raw[dst:dst + 16] = original_bytes[src:src + 16] if frame + 9 < samples else bytes(16)
        (out / (name + ".fmr")).write_bytes(raw)
        pins.append(original)
        # Retain the historical two-stock match setup for the seeded case.
        extra = ["--stocks", "2"] if name == "green-missile-misfire" else []
        add("luigi", name, motions, extra)

    base = bytearray((INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes())
    struct.pack_into("<I", base, 16, 0x6D657774)
    for variant, motions in (
        ("short-b", [341, 345]), ("short-a", [341, 345]),
        ("cancel-restart", [341, 344, 345]), ("roll-restart", [341, 345]),
        ("full-release", [343, 345]), ("full-store", [343, 344, 345]),
    ):
        name = "shadow-" + variant
        (out / (name + ".fmr")).write_bytes(mewtwo_recording(base, 700, variant))
        add("mewtwo", name, motions, ["--expect-item-kind", "112"])
    for air in (False, True):
        for direction, ground in (("right", [353, 354, 355]),
                                  ("up", [353, 357, 358]),
                                  ("diagonal", [353, 357, 358])):
            name = "teleport-" + ("air-" if air else "ground-") + direction
            (out / (name + ".fmr")).write_bytes(mewtwo_recording(
                base, 700, "teleport-" + direction, airborne=air))
            extra = ["--fixture-airborne", "1,175,-30,120"] if air else []
            add("mewtwo", name, [356, 357, 358] if air else ground, extra)

    # Charge/release and every grounded/airborne Dancing Blade route use
    # source-backed pad timing and motion contracts, independently of VM.
    for mode, facing, duration, charge in (("ground", "left", 210, "full"),
                                           ("ground", "right", 60, "partial"),
                                           ("air", "right", 1, "short")):
        name = "neutral-" + mode + "-" + facing + "-" + charge
        for fighter in ("marth", "roy"):
            clip = fighter + "-" + name + ".fmr"
            # Roy holds for seven seconds at 30 charge ticks/sec, after startup;
            # Marth's four-second charge completes within the shorter schedule.
            hold = 260 if fighter == "roy" and charge == "full" else duration
            charge_recording(out / clip, mode, facing, hold)
            add(fighter, name, expectations(name, fighter)["motions"], recording=clip)
    for mode in ("ground", "air"):
        for route in ("high", "neutral", "low"):
            name = "side-" + mode + "-" + route
            chain_recording(out / (name + ".fmr"), mode, route)
            for fighter in ("marth", "roy"):
                add(fighter, name, expectations(name, fighter)["motions"])
    manifest = dict(cases=cases, input_files=list(map(str, pins)),
                    boundaries=["Pad schedules reuse source-backed move contracts.",
                                "Airborne Teleport uses explicit offline placement, not callback injection.",
                                "Fresh interpreted packages are prepared by the shared roster runner."])
    (out / "matrix.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(dict(matrix=str(out / "matrix.json"), cases=len(cases))))


if __name__ == "__main__":
    main()
