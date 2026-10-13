"""Convert a local DSP coefficient ROM table to native signed coefficients."""
import argparse, pathlib, struct
p = argparse.ArgumentParser()
p.add_argument("--input", type=pathlib.Path, required=True)
p.add_argument("--output", type=pathlib.Path, required=True)
a = p.parse_args()
data = a.input.read_bytes()
if len(data) != 4096: raise ValueError("Expected 4096-byte DSP coefficient table")
# Melee leaves AX's coefficient selector at zero: the first 128 four-tap phases.
words = struct.unpack(">512h", data[:1024])
text = "/* Generated from local DSP coefficients; do not commit. */\n"
text += "\n".join(", ".join(map(str, words[i:i+8])) + "," for i in range(0, 512, 8)) + "\n"
a.output.parent.mkdir(parents=True, exist_ok=True)
if not a.output.exists() or a.output.read_text() != text: a.output.write_text(text)
