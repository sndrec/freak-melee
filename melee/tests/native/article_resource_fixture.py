"""Extract the actual runtime slot-release body for resource retirement checks."""
from pathlib import Path
import re
import sys


def extract(source, output):
    text = Path(source).read_text(encoding="utf-8")
    context = re.search(
        r"typedef struct NativeFighterLoadContext\s*\{.*?\}\s*NativeFighterLoadContext;",
        text, re.S)
    if context is None:
        raise ValueError("Missing actual runtime load context declaration")
    start = text.index("static void release_slot(NativeFighterSlot* slot)")
    opening = text.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    # NativeFighterSlot remains the controlled storage boundary in the harness;
    # the context schema and retirement body come directly from production.
    declarations = context.group(0) + "\n\n" + (
        "static NativeFighterLoadContext load_contexts[NATIVE_CALLBACK_DISPATCH_LIMIT];\n"
        "static unsigned load_context_count;\n\n")
    Path(output).write_text(declarations + text[start:end] + "\n",
                           encoding="utf-8", newline="\n")


if __name__ == "__main__":
    extract(*sys.argv[1:])
