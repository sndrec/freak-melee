"""Download or remove reviewed feedback using the owner's Wrangler login."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from tempfile import NamedTemporaryFile
from uuid import UUID


SERVICE = Path(__file__).resolve().parents[1] / 'services/community'
WRANGLER = SERVICE / 'node_modules/wrangler/bin/wrangler.js'


def wrangler(*args):
    result = subprocess.run(['node', str(WRANGLER), *args], cwd=SERVICE,
                            capture_output=True, text=True, encoding='utf-8', errors='replace')
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())
    return result.stdout


def reports(report_id, status, limit, before=None):
    where = []
    if report_id:
        where.append("id='" + str(UUID(report_id)) + "'")
    if status:
        where.append("status='" + status + "'")
    if before:
        timestamp, key = before
        quoted = str(key).replace("'", "''")
        where.append(f"(created_at<{int(timestamp)} OR (created_at={int(timestamp)} AND id<'{quoted}'))")
    sql = 'SELECT * FROM feedback'
    if where:
        sql += ' WHERE ' + ' AND '.join(where)
    sql += ' ORDER BY created_at DESC,id DESC'
    if limit:
        sql += f' LIMIT {limit}'
    config = json.loads((SERVICE / 'wrangler.jsonc').read_text())
    database = next(db['database_name'] for db in config['d1_databases'] if db['binding'] == 'COMMUNITY')
    result = json.loads(wrangler('d1', 'execute', database, '--remote', '--json', '--command', sql))
    return result[0]['results']


def report_pages(report_id, status, remaining):
    before = None
    page_size = 1 if report_id else 8
    while remaining is None or remaining > 0:
        count = page_size if remaining is None else min(page_size, remaining)
        rows = reports(report_id, status, count, before)
        yield from rows
        if report_id or len(rows) < count:
            return
        before = rows[-1]['created_at'], rows[-1]['id']
        if remaining is not None:
            remaining -= len(rows)


def attachment(bucket, key, path, size, digest):
    def valid(candidate):
        return candidate.is_file() and candidate.stat().st_size == size and \
            hashlib.sha256(candidate.read_bytes()).hexdigest() == digest
    if valid(path):
        return
    with NamedTemporaryFile(dir=path.parent, suffix='.download', delete=False) as file:
        pending = Path(file.name)
    try:
        wrangler('r2', 'object', 'get', bucket + '/' + key, '--remote', '--file', str(pending))
        if not valid(pending):
            raise RuntimeError('Downloaded attachment failed size/hash verification: ' + str(path))
        pending.replace(path)
    finally:
        pending.unlink(missing_ok=True)


def validate_attachment_keys(report_id, row):
    prefix = f'feedback/{report_id}/'
    image = row['screenshot_key']
    attempt = image.removeprefix(prefix).removesuffix('/screen.png')
    if (row['id'] != report_id or str(UUID(attempt)) != attempt or
            image != f'{prefix}{attempt}/screen.png' or
            row['context_key'] != f'{prefix}{attempt}/context.bin'):
        raise ValueError('Attachment keys do not belong to this report upload.')


def save_removal(receipt, row, deleted):
    file = NamedTemporaryFile(dir=receipt.parent, suffix='.json', mode='w', encoding='utf-8', delete=False)
    pending = Path(file.name)
    try:
        with file:
            json.dump({'report': row, 'deleted': deleted}, file, indent=2)
            file.write('\n')
        pending.replace(receipt)
    finally:
        pending.unlink(missing_ok=True)


def remove_report(report_id, bucket, output):
    key = str(UUID(report_id))
    rows = reports(key, None, 1)
    folder = output / key
    folder.mkdir(parents=True, exist_ok=True)
    receipt = folder / 'removal.json'
    saved = json.loads(receipt.read_text(encoding='utf-8')) if receipt.is_file() else None
    if saved and not saved['deleted']:
        row = saved['report']
    elif rows:
        row = rows[0]
        validate_attachment_keys(key, row)
        # Keep the attachment keys for retry if deletion is interrupted after
        # the visible database record has been removed.
        save_removal(receipt, row, False)
    elif saved:
        print('Report already removed: ' + key)
        return
    else:
        raise RuntimeError('Report is absent; no local removal receipt is available.')
    config = json.loads((SERVICE / 'wrangler.jsonc').read_text())
    database = next(db['database_name'] for db in config['d1_databases'] if db['binding'] == 'COMMUNITY')
    validate_attachment_keys(key, row)
    wrangler('d1', 'execute', database, '--remote', '--json', '--command',
             f"DELETE FROM feedback WHERE id='{key}' AND screenshot_key='{row['screenshot_key']}' "
             f"AND context_key='{row['context_key']}'")
    for field in ('screenshot_key', 'context_key'):
        wrangler('r2', 'object', 'delete', bucket + '/' + row[field], '--remote', '--force')
    current = reports(key, None, 1)
    if current and current[0]['screenshot_key'] == row['screenshot_key']:
        raise RuntimeError('Report is still present after deletion.')
    save_removal(receipt, row, True)
    print('Removed report and both attachments: ' + key)
    if current:
        print('A later submission with the same ID is still available for review.')


def download_report(row, bucket, output):
    report_id = str(UUID(row['id']))
    validate_attachment_keys(report_id, row)
    folder = output / report_id
    folder.mkdir(exist_ok=True)
    notes = json.loads(row['notes_json'])
    for field, name in [('screenshot', 'screen.png'), ('context', 'context.json')]:
        attachment(bucket, row[field + '_key'], folder / name,
                   row[field + '_size'], row[field + '_sha256'])
    (folder / 'report.json').write_text(json.dumps({**row, 'notes': notes}, indent=2) + '\n', encoding='utf-8')
    summary = [f"# Feedback {report_id}", '']
    for label, field in [('Captured', 'capture_at'), ('Build', 'build'), ('Status', 'status')]:
        summary.extend('    ' + line for line in f"{label}: {row[field]}".splitlines())
    summary.extend(['', '![Screenshot](screen.png)', ''])
    for pin in notes:
        summary.extend([f"## Pin {pin['id']:d} ({pin['x']:.4f}, {pin['y']:.4f})", ''])
        summary.extend('    ' + line for line in pin['text'].splitlines())
        summary.append('')
    (folder / 'notes.md').write_text('\n'.join(summary), encoding='utf-8')
    print(folder)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('--id', help='Download one report UUID')
    selection.add_argument('--all', action='store_true', help='Download every matching report')
    parser.add_argument('--status', choices=['new', 'investigating', 'resolved'])
    parser.add_argument('--limit', type=int, default=1, help='Newest reports to download (default: 1)')
    parser.add_argument('--output', type=Path, default=SERVICE.parents[2] / '.local/feedback')
    parser.add_argument('--delete', action='store_true', help='Remove one fully reviewed report and its attachments; requires --id')
    args = parser.parse_args()
    if args.limit < 1:
        parser.error('--limit must be positive')
    if args.delete and not args.id:
        parser.error('--delete requires a single --id')
    if not WRANGLER.is_file():
        parser.error('Install services/community dependencies with npm ci, then run wrangler login there.')
    config = json.loads((SERVICE / 'wrangler.jsonc').read_text())
    bucket = next(b['bucket_name'] for b in config['r2_buckets'] if b['binding'] == 'REPORTS')
    if args.delete:
        remove_report(args.id, bucket, args.output.resolve())
        return
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    downloaded = 0
    index = NamedTemporaryFile(dir=output, suffix='.json', mode='w', encoding='utf-8', delete=False)
    pending = Path(index.name)
    try:
        with index:
            index.write('[\n')
            for row in report_pages(args.id, args.status, None if args.all else args.limit):
                try:
                    download_report(row, bucket, output)
                except ValueError:
                    print('Skipping a feedback record with invalid metadata.', file=sys.stderr)
                    continue
                if downloaded:
                    index.write(',\n')
                json.dump(row, index, indent=2)
                downloaded += 1
            index.write('\n]\n')
        pending.replace(output / 'index.json')
    finally:
        pending.unlink(missing_ok=True)
    print(f'Downloaded {downloaded} report(s); remote reports unchanged.')


if __name__ == '__main__':
    main()
