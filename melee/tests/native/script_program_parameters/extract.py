"""Extract complete native creator capture/getter bodies; no semantic rewriting."""
from pathlib import Path

def extract(root: Path, out: Path):
    text = (root / 'src/melee/it/native_item_owner.c').read_text()
    bodies = []
    for signature in ['void nativeItem_CaptureCreator(', 'Fighter_GObj* nativeItem_Creator(']:
        start = text.index(signature)
        opening = text.index('{', start)
        depth = 1
        end = opening + 1
        while depth:
            if text[end] == '{':
                depth += 1
            elif text[end] == '}':
                depth -= 1
            end += 1
        bodies.append(text[start:end])
    (out / 'actual_owner.inc').write_text('\n\n'.join(bodies) + '\n', encoding='utf-8')
