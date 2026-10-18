"""Check UTF-8 editing, glyph selection and measured widths in one native hub.

The game is hidden and muted, using an isolated profile and no public chat.
"""
import argparse
from pathlib import Path
import re

from review_native_css_room import Client


def metrics(client):
    text = client.inspect('hub-fonts')
    rows = re.findall(r'font=(\d+) glyphs=(\d+) pairs=(\d+) atlas=(\d+)x(\d+) first=U\+([0-9A-F]+) width=([\d.]+)', text)
    assert len(rows) == 7, text
    return [(int(face), int(count), int(pairs), int(code, 16), float(width))
            for face, count, pairs, w, h, code, width in rows]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                    output / 'client', 'Lettering Review', window_size=(960, 540))
    evidence = []
    try:
        client.click('your-profile'); client.element('profile-bio')
        cases = [('é', 0xE9), ('e\u0301', 0xE9), ('Å', 0xC5), ('A\u030A', 0xC5),
                 ('č', 0x10D), ('c\u030C', 0x10D), ('🦊', 0xFFFD)]
        widths = []
        for text, expected in cases:
            client.enter('profile-bio', text)
            values = metrics(client)
            assert all(count > 300 and pairs > 150 and first == expected and width > 0
                       for face, count, pairs, first, width in values), (ascii(text), values)
            widths.append([value[-1] for value in values])
            evidence.append((ascii(text), values))
        assert widths[0] == widths[1] and widths[2] == widths[3] and widths[4] == widths[5]
        bio = 'Pokémon — café, Zoë, Łódź, Ångström… “Hello!”'
        client.enter('profile-bio', bio + 'é')
        client.command('ui-key', 'backspace')
        client.click('save-bio')
        saved = (client.data / 'content/hub-profile.txt').read_text(encoding='utf-8')
        assert saved.splitlines()[1] == bio, repr(saved)
        client.screenshot()
        client.command('ui-key', 'escape'); client.element('local-practice')
        client.click('nav-6'); client.element('display-name')
        client.enter('display-name', 'Zoë — Łódź')
        client.click('save-profile'); client.screenshot()
        client.click('your-profile'); client.element('profile-bio')
        client.screenshot()
        assert (client.data / 'content/hub-profile.txt').read_text(encoding='utf-8').splitlines() == ['Zoë — Łódź', bio]
        client.command('ui-key', 'escape'); client.element('display-name')
        assert client.process.poll() is None
        (output / 'passed.txt').write_text('Seven native font faces: precomposed/decomposed accents agree, unsupported scalar fallback, UTF-8 backspace, profile save and return passed.\n')
        print((output / 'passed.txt').read_text().strip(), flush=True)
    finally:
        (output / 'metrics.txt').write_text(repr(evidence), encoding='utf-8')
        client.stop()


if __name__ == '__main__':
    main()
