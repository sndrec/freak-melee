"""Replay identical inputs against stock and a packaged clone, then compare traces."""

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--inputs", type=Path, required=True)
    parser.add_argument("--stock-fighter", required=True)
    parser.add_argument("--package-id", required=True)
    parser.add_argument("--p2-package-id",
                        help="also replace the stock --p2 fighter with this independent package")
    parser.add_argument("--content-root", type=Path, required=True)
    parser.add_argument("--stage", default="final-destination")
    parser.add_argument("--stock-color", type=int, default=0)
    parser.add_argument("--clone-skin")
    parser.add_argument("--stocks", type=int, default=1)
    parser.add_argument("--trace-geometry-frame", type=int,
                        help="Dump live joints/materials/hitboxes at one replay sample")
    parser.add_argument("--fixture-item", action="append", default=[],
                        help="Offline common-item spawn: kind,sample,x,y")
    parser.add_argument("--fixture-airborne", action="append", default=[],
                        help="Offline airborne fighter placement: port,sample,x,y")
    parser.add_argument("--real-time", action="store_true",
                        help="Pace simulation normally for audio-dependent stage behavior")
    parser.add_argument("--show-window", action="store_true",
                        help="Show game windows for an explicitly requested visual inspection")
    parser.add_argument("--p2", default="human:zelda")
    parser.add_argument("--p1-type", choices=("human", "cpu"), default="human")
    parser.add_argument("--cpu-level", type=int, choices=range(1, 10), default=9)
    parser.add_argument("--exe", type=Path,
                        default=ROOT / "build/native/melee_native.exe")
    parser.add_argument("--out-dir", type=Path,
                        default=ROOT / ".local/parity-runs")
    parser.add_argument("--expect-attachment", action="store_true")
    parser.add_argument("--independent", action="store_true",
                        help="verify P1 uses its custom behavior identity; normalize only "
                             "that identity when comparing to stock")
    parser.add_argument("--expect-motion", type=int, action="append", default=[])
    parser.add_argument("--expect-port-motion", type=int, nargs=2,
                        action="append", default=[], metavar=("PORT", "MOTION"))
    parser.add_argument("--expect-damage-port", type=int, action="append",
                        default=[])
    parser.add_argument("--expect-item-kind", type=int, action="append",
                        default=[])
    parser.add_argument("--expect-item-state", type=int, nargs=5, action="append", default=[])
    parser.add_argument("--expect-item-direction-change", type=int, nargs=2, action="append", default=[])
    parser.add_argument("--expect-motion-window", type=int, nargs=4, action="append", default=[])
    parser.add_argument("--expect-item-owner-transfer", type=int, nargs=3,
                        action="append", default=[],
                        metavar=("KIND", "FROM", "TO"))
    parser.add_argument("--expect-item-event", nargs=2, action="append",
                        default=[], metavar=("EVENT", "KIND"))
    parser.add_argument("--expect-item-removed-on-damage", type=int, nargs=2,
                        action="append", default=[], metavar=("KIND", "PORT"))
    args = parser.parse_args()
    clone_p2 = args.p2
    if args.p2_package_id:
        parts = args.p2.split(":")
        if not args.independent or len(parts) not in (2, 3) or parts[0] not in ("human", "cpu") or parts[1] == "custom":
            parser.error("scripted P2 needs --independent and a reference --p2 human/cpu:STOCK[:LEVEL]")
        clone_p2 = ":".join((parts[0], "custom", *parts[2:]))
    for path in (args.iso, args.inputs, args.exe):
        if not path.is_file():
            parser.error(f"missing file: {path}")
    if not args.content_root.is_dir():
        parser.error(f"missing content store: {args.content_root}")
    args.out_dir.mkdir(parents=True, exist_ok=True)
    common = [str(args.exe.resolve()), "--iso", str(args.iso.resolve()),
              "--mute", "--match", "--stage", args.stage,
              "--mode", "stock", "--stocks", str(args.stocks), "--time", "1",
              "--items", "off", "--trace-actions",
              "--trace-fighter-state", "--replay-inputs",
              str(args.inputs.resolve())]
    if not args.real_time:
        common.append("--fast")
    if not args.show_window:
        common.append("--hidden")
    if args.trace_geometry_frame is not None:
        common.extend(("--trace-geometry-frame", str(args.trace_geometry_frame)))
    for fixture in args.fixture_item:
        common.extend(("--fixture-item", fixture))
    for fixture in args.fixture_airborne:
        common.extend(("--fixture-airborne", fixture))
    logs = []
    for label, fighter, extra in (
        ("stock", args.stock_fighter, ["--p1-color", str(args.stock_color)]),
        ("clone", "custom", ["--p1-fighter", args.package_id,
                             "--content-root", str(args.content_root.resolve())]),
    ):
        if label == "clone" and args.clone_skin:
            extra = extra + ["--p1-skin", args.clone_skin]
        if label == "clone" and args.p2_package_id:
            extra = extra + ["--p2-fighter", args.p2_package_id]
        log = args.out_dir / f"{args.inputs.stem}-{label}.log"
        with log.open("wb") as output:
            try:
                player = f"{args.p1_type}:{fighter}"
                if args.p1_type == "cpu": player += f":{args.cpu_level}"
                result = subprocess.run(common + ["--p1", player, "--p2", clone_p2 if label == "clone" else args.p2] +
                                        extra, cwd=ROOT, stdout=output,
                                        stderr=subprocess.STDOUT, timeout=90,
                                        creationflags=subprocess.CREATE_NO_WINDOW)
            except subprocess.TimeoutExpired:
                print(f"{label} run timed out; inspect {log}", file=sys.stderr)
                return 1
        logs.append(log)
        if result.returncode:
            print(f"{label} run exited {result.returncode}; inspect {log}",
                  file=sys.stderr)
            return 1
    compare = [sys.executable, str(ROOT / "tools/compare_fighter_parity.py"),
               str(logs[0]), str(logs[1]), "--ports", "1", "2"]
    if args.p2_package_id:
        compare.extend(("--clone-ports", "1", "2"))
    if args.independent:
        compare.extend(("--independent-ports", "1", "2") if args.p2_package_id else ("--independent-ports", "1"))
    if args.expect_attachment:
        compare.append("--expect-attachment")
    for value in args.expect_motion:
        compare.extend(("--expect-motion", str(value)))
    for port, motion in args.expect_port_motion:
        compare.extend(("--expect-port-motion", str(port), str(motion)))
    for value in args.expect_damage_port:
        compare.extend(("--expect-damage-port", str(value)))
    for value in args.expect_item_kind:
        compare.extend(("--expect-item-kind", str(value)))
    for values in args.expect_item_state:
        compare.extend(("--expect-item-state", *map(str, values)))
    for values in args.expect_item_direction_change:
        compare.extend(("--expect-item-direction-change", *map(str, values)))
    for values in args.expect_motion_window:
        compare.extend(("--expect-motion-window", *map(str, values)))
    for kind, source, target in args.expect_item_owner_transfer:
        compare.extend(("--expect-item-owner-transfer", str(kind),
                        str(source), str(target)))
    for event, kind in args.expect_item_event:
        compare.extend(("--expect-item-event", event, kind))
    for kind, port in args.expect_item_removed_on_damage:
        compare.extend(("--expect-item-removed-on-damage", str(kind),
                        str(port)))
    result = subprocess.run(compare, cwd=ROOT)
    if result.returncode:
        print(f"Trace logs: {logs[0]} and {logs[1]}", file=sys.stderr)
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
