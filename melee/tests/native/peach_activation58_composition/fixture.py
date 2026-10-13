"""Extract complete original taunt bodies; optional explicitly separate guarded source."""
import hashlib
import re
from pathlib import Path


def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
                      re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match:
        raise ValueError('function definition missing: ' + name)
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


def extract(sdk, output, guard_source=None):
    groups = {
        'src/melee/ft/kinds/ftCommon/ftCo_AppealS.c': [
            'ftCo_800DE9B8', 'ftCo_800DE9D8', 'ftCo_800DEA28', 'ftCo_800DEAE8', 'ftCo_800DEBD0'],
        'src/melee/ft/kinds/ftPeach/ftpeach.c': ['ftPe_Init_8011B93C'],
        'src/melee/ft/ftparts.c': ['ftParts_80074A74', 'ftParts_80074A4C'],
        'src/melee/ft/native_fighter_visuals.c': ['nativeFighterVisuals_PreviousGroup'],
    }
    blocks, sources = [], {}
    for relative, names in groups.items():
        path = sdk / relative
        text = path.read_text()
        functions = {}
        for name in names:
            original = body(text, name)
            functions[name] = {'line': text[:text.index(original)].count('\n') + 1,
                               'sha256': hashlib.sha256(original.encode()).hexdigest()}
            blocks.append(original)
        sources[relative] = {'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'functions': functions}
    if guard_source:
        path = Path(guard_source)
        text = path.read_text()
        original = body(text, 'ftCo_800DEA28')
        assert 'nativeCallbackSource_Snapshot' in original and 'nativeCallbackSource_Alive' in original
        # Rename only the definition to coexist with the unchanged frozen58
        # body. The guarded full body and all engine calls remain untouched.
        blocks.append(original.replace('void ftCo_800DEA28(', 'void guarded_taunt_entry(', 1))
        sources['separate_guarded_source'] = {'path': str(path.resolve()),
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
            'body_sha256': hashlib.sha256(original.encode()).hexdigest(),
            'line': text[:text.index(original)].count('\n') + 1}
        blocks.append('#define HAVE_GUARDED_ENTRY 1')
    (output / 'taunt_bodies.h').write_text('\n\n'.join(blocks) + '\n', encoding='utf-8', newline='\n')
    return sources
