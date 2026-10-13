"""Prepare unchanged original held-tether bodies and a pending-candidate inventory.

This prepares source inputs; it does not compile the future DSL or claim parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

REPO = Path(__file__).resolve().parents[2]
SYMBOLS = ["itLinkHookshot_ApplyOwnerScale", "it_802A2474", "it_802A24A0",
           "link_fighter_compare", "it_802A2568_inline", "it_link_get_joint",
           "it_link_get_joint_c", "it_link_lerp", "it_link_attr_math",
           "it_802A2568", "it_802A2B10", "it_802A2BA4", "fn_802A2E4C",
           "itLinkhookshot_UnkMotion0_Phys", "it_802A3C98", "it_802A40D0",
           "it_802A49B0", "it_802A6944", "it_802A6A78_get_next",
           "it_802A6A78_get_pos", "it_802A6A78_normalize_diff_rev",
           "it_802A6A78_normalize_diff", "it_802A6A78", "it_802A6F80",
           "it_802A7168", "itLinkHookshot_Logic20_PickedUp_inline",
           "itLinkHookshot_Logic20_PickedUp", "it_802A7AAC", "it_802A7AF0",
           "it_802A2EE4_inline_alt_mtx_first", "it_802A7B34_6944_inline",
           "it_802A7B34", "it_802A7D40"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise ValueError(f"missing actual body {name}")
    opening = text.index("{", match.start())
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def inventory(candidate):
    lines = candidate.read_text(encoding="utf-8").splitlines()
    functions = []
    for i, line in enumerate(lines):
        match = re.fullmatch(r"( *)(?:export )?func (\w+)\((.*)\):", line)
        if not match:
            continue
        indent, name, arguments = len(match[1]), match[2], match[3]
        end = i + 1
        while end < len(lines):
            current = lines[end]
            if current.strip() and not current.lstrip().startswith("#") and len(current)-len(current.lstrip()) <= indent:
                break
            end += 1
        source_lines = [s for s in lines[i+1:end] if s.strip() and not s.lstrip().startswith("#")]
        argument_count = len(arguments.split(",")) if arguments else 0
        locals_count = sum(bool(re.match(r"\s*let \w+ \w+", s)) for s in source_lines)
        functions.append({"name": name, "context": "item" if indent else "fighter",
                          "line": i+1, "body_statements": len(source_lines),
                          "arguments": argument_count, "declared_locals_plus_arguments": locals_count+argument_count})
    return {"path": str(candidate), "sha256": digest(candidate), "lines": len(lines), "functions": functions}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=REPO)
    parser.add_argument("--candidate", type=Path, default=REPO / ".local/link-tether-candidate/held_tether.pending.fms")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root, output = args.source_root.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = root / "src/melee/it/kinds/itlinkhookshot.c"
    layout = root / "src/melee/it/itCharItems.h"
    originals = source.read_text(encoding="utf-8")
    # Definition order is retained in the extracted header. Parent fixture supplies
    # forward declarations/controlled engine boundaries when compiling these bodies.
    pieces = [(originals.index(body(originals, name)), name, body(originals, name)) for name in SYMBOLS]
    pieces.sort()
    extracted = "// Unchanged actual stock bodies; forward declarations belong to the consuming fixture.\n" + "\n\n".join(text for _, _, text in pieces) + "\n"
    if len(extracted.splitlines()) >= 2000:
        raise RuntimeError("split original fixture before it grows past 2000 lines")
    header = output / "link_tether_held_originals.h"
    header.write_text(extracted, encoding="utf-8", newline="\n")
    candidate = inventory(args.candidate.resolve())
    if any(f["body_statements"] > 64 or f["declared_locals_plus_arguments"] > 16 for f in candidate["functions"]):
        raise RuntimeError("candidate helper exceeds existing per-body/local limits")
    receipt = {"scope": "source extraction and candidate bounds; no generated-C/source parity claimed",
               "source_sha256": {str(source): digest(source), str(layout): digest(layout), str(Path(__file__).resolve()): digest(Path(__file__).resolve())},
               "extracted_sha256": digest(header), "original_symbols": SYMBOLS,
               "candidate": candidate, "pending": ["article helper/accessory/probe compiler integration", "actual generated-C vs original comparison", "full tether lifecycle", "gameplay"]}
    (output / "inputs.json").write_text(json.dumps(receipt, indent=2)+"\n", encoding="utf-8", newline="\n")
    print(json.dumps({"receipt": str(output / "inputs.json"), "originals": len(SYMBOLS),
                      "helpers": len(candidate["functions"]), "maximum_body_statements": max(f["body_statements"] for f in candidate["functions"]),
                      "maximum_locals_and_arguments": max(f["declared_locals_plus_arguments"] for f in candidate["functions"])}))


if __name__ == "__main__":
    main()
