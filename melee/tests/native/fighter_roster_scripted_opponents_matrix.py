"""Reuse controller interactions with both sides independently scripted.

Most opponent choices match the established stock cohort. Mario additionally
replaces its Zelda capture/throw/Disable targets; these require fresh coverage.
"""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--from-matrix", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()
    source = args.from_matrix.resolve()
    out = args.out_dir.resolve()
    if out.drive.upper() != "A:" or out.exists():
        parser.error("Use a fresh private A: output directory")
    original = json.loads(source.read_text())
    by_case = {(row["fighter"], row["scenario"]): row for row in original["cases"]}
    cases = []
    selected = []
    for fighter in ("marth", "roy"):
        for name, opponent in (("counter-ground-laser", "fox"),
                               ("counter-air-laser", "fox"),
                               ("cross-peach-counter", "peach"),
                               ("counter-ground-melee", "mario")):
            selected.append((fighter, name, opponent))
    # Identical package IDs must still produce two independently owned actors.
    selected.append(("marth", "counter-ground-melee", "marth"))
    for name in ("confusion-reflect", "confusion-ground-capture",
                 "confusion-ground-left-capture", "confusion-air-capture",
                 "confusion-shield-capture", "throw-forward-right", "throw-forward-left",
                 "disable-target", "disable-shield"):
        selected.append(("mewtwo", name, "mario"))
    out.mkdir(parents=True)
    pins = [Path(__file__), source]
    pins += [(source.parent / value).resolve() for value in original.get("input_files", [])]
    for fighter, name, opponent in selected:
        row = dict(by_case[fighter, name])
        recording = (source.parent / row["recording"]).resolve()
        pins.append(recording)
        scenario = name + "-source-" + opponent
        filename = fighter + "-" + scenario + ".fmr"
        data = bytearray(recording.read_bytes())
        if fighter == "roy" and name == "counter-ground-melee" and opponent == "mario":
            # Mario's tilt reaches Roy before the stock guard opens at frame 8.
            # Delay only the opponent's press; keep Roy's counter input intact.
            source_pad = 24 + 180 * 80 + 24
            target_pad = 24 + 186 * 80 + 24
            if any(data[target_pad:target_pad + 16]) or not any(data[source_pad:source_pad + 16]):
                raise ValueError("Roy/Mario fixture no longer has the expected isolated press")
            data[target_pad:target_pad + 16] = data[source_pad:source_pad + 16]
            data[source_pad:source_pad + 16] = bytes(16)
        (out / filename).write_bytes(data)
        arguments = list(row["arguments"])
        if "--p2" in arguments:
            arguments[arguments.index("--p2") + 1] = "human:" + opponent
        else:
            arguments += ["--p2", "human:" + opponent]
        row.update(scenario=scenario, recording=filename, arguments=arguments, opponent=opponent)
        cases.append(row)
    manifest = dict(cases=cases, input_files=list(map(str, pins)),
                    boundaries=["Both source programs use fresh ordinary independent packages.",
                                "Mario replaces Zelda in capture/throw/Disable setups.",
                                "Roy/Mario tilt input is delayed six frames to reach the active counter guard.",
                                "Original offline positioning fixtures are retained.",
                                "The shared runner demands two live fault-free source actor reports."])
    (out / "matrix.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(dict(matrix=str(out / "matrix.json"), cases=len(cases))))


if __name__ == "__main__":
    main()
