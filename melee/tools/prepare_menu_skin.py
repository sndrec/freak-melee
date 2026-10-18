"""Extract reusable menu border coverage from the user's verified Melee disc."""
import argparse
from pathlib import Path
import re

from prepare_menu_fonts import disc_file
from prepare_menu_lettering import MenuArchive
from prepare_native_assets import extract_dol


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1]
    config = (source / 'config/GALE01/config.yml').read_text()
    extract_dol(args.iso, re.search(r'hash: ([0-9a-f]{40})', config).group(1))
    menu = MenuArchive(disc_file(args.iso, 'MnMaAll.usd'))
    border = menu.joint_image('MenMainPanel_Top_joint', 77, coverage=True)
    if border.size != (48, 32):
        raise ValueError('Unexpected stock panel border dimensions')
    # Keep the border's coverage; the component supplies its own interior fill.
    data = bytearray()
    for by in range(0, 32, 8):
        for bx in range(0, 48, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    a = border.getpixel((bx + x, by + y)) >> 4
                    b = border.getpixel((bx + x + 1, by + y)) >> 4
                    data.append(a << 4 | b)
    args.output.mkdir(parents=True, exist_ok=True)
    path = args.output / 'menu-panel.bin'
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)
    border.save(path.with_suffix('.png'))


if __name__ == '__main__':
    main()
