"""Save an unpublished News draft using the owner's existing Wrangler login."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
from uuid import NAMESPACE_URL, UUID, uuid5


SERVICE = Path(__file__).resolve().parents[1] / 'services/community'
WRANGLER = SERVICE / 'node_modules/.bin' / ('wrangler.cmd' if os.name == 'nt' else 'wrangler')
ADMIN_URL = 'https://freak-melee-community.sndrec32exe.workers.dev/admin/'


def field(value, maximum):
    # Match the admin service's UTF-16 length limit and control-character rules.
    if len(value.encode('utf-16-le')) // 2 > maximum or not value.strip() or \
            re.search(r'[\x00-\x08\x0b\x0c\x0e-\x1f]', value):
        raise ValueError(f'Text must contain 1–{maximum} characters and no control characters.')
    return value.strip()


def sql_text(value):
    # All article text stays in a SQL file. Hex literals also keep quotes,
    # Unicode and statement-looking text intact when Wrangler parses the file.
    return "CAST(X'" + value.encode('utf-8').hex() + "' AS TEXT)"


def execute(sql, write=False):
    args = [str(WRANGLER), 'd1', 'execute', 'COMMUNITY', '--remote', '--json']
    with tempfile.TemporaryDirectory(prefix='freak-news-') as folder:
        if write:
            path = Path(folder) / 'draft.sql'
            path.write_text(sql, encoding='utf-8')
            args += ['--file', str(path), '--yes']
        else:
            args += ['--command', sql]
        result = subprocess.run(args, cwd=SERVICE, capture_output=True, text=True,
                                encoding='utf-8', errors='replace', timeout=120)
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())
    # Wrangler's remote file import can finish without JSON on stdout. The
    # following SELECT verifies the actual stored article in either case.
    if write:
        return []
    response = json.loads(result.stdout)
    entries = response if isinstance(response, list) else [response]
    if not entries or any(entry.get('success') is not True for entry in entries):
        raise RuntimeError('Wrangler did not confirm the database operation.')
    return [row for entry in entries for row in entry.get('results', [])]


def save_draft(key, title, body, receipt_path):
    if not re.fullmatch(r'[A-Za-z0-9._/-]{1,128}', key):
        raise ValueError('Use a stable draft key of 1–128 letters, digits, dots, slashes, underscores or hyphens.')
    title, body = field(title, 120), field(body, 32000)
    record_id = str(uuid5(NAMESPACE_URL, ADMIN_URL + 'news-draft/' + key))
    if receipt_path.is_file():
        previous = json.loads(receipt_path.read_text(encoding='utf-8'))
        if previous.get('key') != key or previous.get('admin_url') != ADMIN_URL or not previous.get('id'):
            raise ValueError('This receipt belongs to another draft.')
        record_id = str(UUID(previous['id']))
    receipt_path.parent.mkdir(parents=True, exist_ok=True)
    # Fail before a remote write when the requested receipt cannot be saved.
    with tempfile.TemporaryFile(dir=receipt_path.parent):
        pass

    query = f'SELECT * FROM news WHERE id={sql_text(record_id)} OR title={sql_text(title)} LIMIT 2'
    rows = execute(query)
    if not rows:
        execute('INSERT INTO news(id,title,ticker,body,published_at,updated_at,revision,archived) '
                f'SELECT {sql_text(record_id)},{sql_text(title)},\'\',{sql_text(body)},'
                f'NULL,{time.time_ns() // 1000000},1,0 '
                f'WHERE NOT EXISTS (SELECT 1 FROM news WHERE title={sql_text(title)}) '
                'ON CONFLICT(id) DO NOTHING;', write=True)
        rows = execute(query)
    if len(rows) != 1:
        raise RuntimeError('Expected one saved article. Resolve duplicate titles before retrying.')
    row = rows[0]
    matches = row['title'] == title and row['body'] == body
    receipt = dict(key=key, id=row['id'], title=row['title'], admin_url=ADMIN_URL,
                   revision=row['revision'], published=row['published_at'] is not None,
                   published_at=row['published_at'], archived=bool(row['archived']),
                   ticker=row['ticker'], body_matches=matches,
                   body_sha256=hashlib.sha256(row['body'].encode('utf-8')).hexdigest(),
                   verified_at_ms=time.time_ns() // 1000000)
    pending = receipt_path.with_suffix(receipt_path.suffix + '.tmp')
    pending.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    pending.replace(receipt_path)
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--key', required=True, help='Stable identifier; use the release ID for patch notes')
    parser.add_argument('--title', required=True)
    parser.add_argument('--body-file', required=True, type=Path, help='UTF-8 Markdown file')
    parser.add_argument('--receipt', required=True, type=Path, help='Save and reuse this article receipt')
    args = parser.parse_args()
    if not WRANGLER.is_file():
        parser.error('Run npm ci and wrangler login in services/community first.')
    try:
        # A 32,000 UTF-16-unit article fits in 128 KiB even with a UTF-8 BOM.
        if args.body_file.stat().st_size > 128 * 1024:
            raise ValueError('The body file is too large for a News article.')
        receipt = save_draft(args.key, args.title, args.body_file.read_text(encoding='utf-8-sig'),
                             args.receipt.resolve())
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as error:
        parser.exit(1, f'News draft: {error}\n')
    print(json.dumps(receipt, indent=2))
    if not receipt['body_matches'] or receipt['published'] or receipt['archived'] or receipt['ticker']:
        print('Existing article preserved. Review the receipt before continuing publication.')
    else:
        print('Verified server-side News draft; unpublished, unscheduled and without ticker text.')


if __name__ == '__main__':
    main()
