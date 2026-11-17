"""Build the installed reference and verify its local links and section anchors."""
import argparse
from collections import Counter
from pathlib import Path
import re
from urllib.parse import quote, unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'docs/scripting'
REPOSITORY = 'https://github.com/sndrec/freak-melee/blob/main/melee/'
LINK = re.compile(r'(!?\[[^\]\n]*\])\(([^)\n]+)\)')


def prose_lines(text):
    fence = None
    for line in text.splitlines(keepends=True):
        marker = re.match(r'^\s*(`{3,}|~{3,})', line)
        if marker:
            if fence is None:
                fence = marker[1][0]
            elif fence == marker[1][0]:
                fence = None
            yield line, False
        else:
            yield line, fence is None


def anchors(text):
    result = set(re.findall(r'<a\s+(?:id|name)=[\'"]([^\'"]+)', text))
    counts = Counter()
    for line, prose in prose_lines(text):
        heading = re.match(r'^#{1,6}\s+(.+?)\s*#*\s*$', line) if prose else None
        if not heading:
            continue
        title = re.sub(r'<[^>]*>', '', heading[1]).lower()
        title = re.sub(r'[^\w\-\s]', '', title).replace(' ', '-')
        number = counts[title]
        counts[title] += 1
        result.add(title + (f'-{number}' if number else ''))
    return result


def sources():
    documents = {path.resolve(): 'ui/docs/' + path.name for path in sorted(SOURCE.glob('*.md'))}
    documents[(ROOT / 'docs/native_fighter_items.md').resolve()] = 'ui/docs/holdable_items.md'
    examples = {}
    for document in documents:
        for line, prose in prose_lines(document.read_text(encoding='utf-8')):
            if not prose:
                continue
            for _, href in LINK.findall(line):
                link = urlsplit(href)
                if link.scheme or link.netloc or not link.path:
                    continue
                target = (document.parent / unquote(link.path)).resolve()
                if target.is_relative_to(ROOT / 'examples') and target.suffix == '.fms':
                    if not target.is_file():
                        raise ValueError(f'{document.name}: missing example {href}')
                    examples[target] = 'ui/' + target.name
    return documents, examples


def render(document, documents, examples):
    def rewrite(match):
        label, href = match.groups()
        link = urlsplit(href)
        if link.scheme or link.netloc or not link.path:
            return match[0]
        target = (document.parent / unquote(link.path)).resolve()
        mapped = documents.get(target) or examples.get(target)
        if mapped:
            # All guide pages occupy ui/docs and examples occupy ui.
            relative = Path(mapped).name if mapped.startswith('ui/docs/') else '../' + Path(mapped).name
            return f'{label}({relative}' + (f'#{link.fragment}' if link.fragment else '') + ')'
        if not target.is_relative_to(ROOT) or not target.is_file():
            raise ValueError(f'{document.name}: unresolved local reference {href}')
        if target.is_relative_to(ROOT / 'tests'):
            raise ValueError(f'{document.name}: replace test-fixture link with an authoring explanation: {href}')
        url = REPOSITORY + quote(target.relative_to(ROOT).as_posix(), safe='/')
        if link.fragment:
            url += '#' + link.fragment
        return label[:-1] + ' (developer reference, online)](' + url + ')'

    return ''.join(LINK.sub(rewrite, line) if prose else line
                   for line, prose in prose_lines(document.read_text(encoding='utf-8')))


def check_tree(directory, allowed_files=None):
    directory = Path(directory).resolve()
    allowed = {Path(path).resolve() for path in allowed_files} if allowed_files is not None else None
    errors = []
    if not (directory / 'index.md').is_file():
        raise ValueError('Workshop reference is missing index.md.')
    for path in sorted(directory.glob('*.md')):
        if allowed is not None and path.resolve() not in allowed:
            continue
        for line, prose in prose_lines(path.read_text(encoding='utf-8')):
            if not prose:
                continue
            for _, href in LINK.findall(line):
                link = urlsplit(href)
                if link.scheme or link.netloc:
                    continue
                target = (path.parent / unquote(link.path)).resolve() if link.path else path
                if not target.is_file() or (allowed is not None and target not in allowed):
                    errors.append(f'{path.name}: unbundled file {href}')
                elif link.fragment and target.suffix == '.md' and unquote(link.fragment) not in anchors(target.read_text(encoding='utf-8')):
                    errors.append(f'{path.name}: unknown section {href}')
    if errors:
        raise ValueError('\n'.join(errors))


def prepare(output):
    output = Path(output).resolve()
    if output.name != 'docs' or output.parent.name != 'ui':
        raise ValueError('Output must be the installation\'s ui/docs directory.')
    output.mkdir(parents=True, exist_ok=True)
    package = output.parent.parent
    documents, examples = sources()
    for source, relative in documents.items():
        (package / relative).write_text(render(source, documents, examples), encoding='utf-8', newline='\n')
    for source, relative in examples.items():
        (package / relative).write_bytes(source.read_bytes())
    allowed = [package / relative for relative in [*documents.values(), *examples.values()]]
    check_tree(output, allowed)
    return len(documents), len(examples)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    choice = parser.add_mutually_exclusive_group(required=True)
    choice.add_argument('--output', type=Path)
    choice.add_argument('--check', type=Path)
    choice.add_argument('--list', action='store_true')
    args = parser.parse_args()
    if args.list:
        documents, examples = sources()
        print('\n'.join(sorted(set(documents.values()) | set(examples.values()))))
    elif args.check:
        check_tree(args.check)
        print('Workshop reference links and sections verified.')
    else:
        documents, examples = prepare(args.output)
        print(f'Prepared {documents} Workshop reference pages and {examples} local examples.')


if __name__ == '__main__':
    main()
