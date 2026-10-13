"""Compose immutable Float/SideB evidence and the unregistered Parasol policies."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent

def main():
    original = HERE.parent / "peach_float_sideb_composition/candidate.fms"
    base = original.read_text()
    travel = (HERE / "travel.fms").read_text()
    article = (HERE / "parasol.fms").read_text()
    base = base.replace("# Float/SideB require generic50 compilation; Parasol travel/common protocol remains inherited.\n# Partial Parasol table exists solely for accepted resource callbacks.", "# Composed travel/held policies require immutable55; constructor/render remain controlled.")
    base = base.replace("base ftDataPeach 341 30", "base ftDataPeach 341 30\n\nmotion_policy 369 common 144\nmotion_policy 370 common 146")
    assert base.count("callbacks.track_hitlag()") == 2
    base = base.replace("callbacks.track_hitlag()", "callbacks.pre_hitlag(pause_parasol)\n                callbacks.post_hitlag(resume_parasol)", 1)
    base = base.replace("callbacks.track_hitlag()", "callbacks.pre_hitlag(pause_parasol)\n    callbacks.post_hitlag(resume_parasol)", 1)
    start = base.index("article It_Kind_Peach_Parasol states 3:")
    end = base.index("article It_Kind_Peach_Turnip", start)
    base = base[:start] + article + "\n" + base[end:]
    cut = travel.index("func pause_parasol")
    first_function = min(base.index("export func "), base.index("func "))
    base = base[:first_function] + travel[:cut] + base[first_function:] + "\n" + travel[cut:]
    (HERE / "candidate.fms").write_text(base, newline="\n")
    inputs = [original, HERE / "travel.fms", HERE / "parasol.fms", Path(__file__)]
    (HERE / "components.json").write_text(json.dumps({str(x): hashlib.sha256(x.read_bytes()).hexdigest() for x in inputs}, indent=2) + "\n", newline="\n")
    print("Assembled", len(base.splitlines()), "lines; accepted inputs unchanged")

if __name__ == "__main__":
    main()
