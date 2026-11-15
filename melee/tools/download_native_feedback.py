"""Download or remove reviewed feedback using the owner's Wrangler login."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from uuid import UUID


SERVICE = Path(__file__).resolve().parents[1] / 'services/community'
WRANGLER = SERVICE / 'node_modules/.bin' / ('wrangler.cmd' if os.name == 'nt' else 'wrangler')


def wrangler(*args):
    result = subprocess.run([str(WRANGLER), *args], cwd=SERVICE,
                            capture_output=True, text=True, encoding='utf-8', errors='replace')
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())
    return result.stdout


def reports(report_id, status, limit):
    where = []
    if report_id:
        where.append("id='" + str(UUID(report_id)) + "'")
    if status:
        where.append("status='" + status + "'")
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


def attachment(bucket, key, path, size, digest):
    def valid(candidate):
        return candidate.is_file() and candidate.stat().st_size == size and \
            hashlib.sha256(candidate.read_bytes()).hexdigest() == digest
    if valid(path):
        return
    pending = path.with_suffix(path.suffix + '.download')
    wrangler('r2', 'object', 'get', bucket + '/' + key, '--remote', '--file', str(pending))
    if not valid(pending):
        raise RuntimeError('Downloaded attachment failed size/hash verification: ' + str(path))
    pending.replace(path)


def remove_report(report_id, bucket, output):
    key = str(UUID(report_id))
    rows = reports(key, None, 1)
    folder = output / key
    folder.mkdir(parents=True, exist_ok=True)
    receipt = folder / 'removal.json'
    if rows:
        row = rows[0]
        # Keep the attachment keys for retry if deletion is interrupted after
        # the visible database record has been removed.
        receipt.write_text(json.dumps({'report': row, 'deleted': False}, indent=2) + '\n', encoding='utf-8')
    elif receipt.is_file():
        row = json.loads(receipt.read_text(encoding='utf-8'))['report']
    else:
        raise RuntimeError('Report is absent; no local removal receipt is available.')
    config = json.loads((SERVICE / 'wrangler.jsonc').read_text())
    database = next(db['database_name'] for db in config['d1_databases'] if db['binding'] == 'COMMUNITY')
    for field in ('screenshot_key', 'context_key'):
        if not row[field].startswith('feedback/' + key + '/'):
            raise RuntimeError('Attachment key does not belong to this report.')
    wrangler('d1', 'execute', database, '--remote', '--json', '--command',
             "DELETE FROM feedback WHERE id='" + key + "'")
    for field in ('screenshot_key', 'context_key'):
        wrangler('r2', 'object', 'delete', bucket + '/' + row[field], '--remote', '--force')
    if reports(key, None, 1):
        raise RuntimeError('Report is still present after deletion.')
    receipt.write_text(json.dumps({'report': row, 'deleted': True}, indent=2) + '\n', encoding='utf-8')
    print('Removed report and both attachments: ' + key)


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
    rows = reports(args.id, args.status, 0 if args.all else args.limit)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    for row in rows:
        folder = output / str(UUID(row['id']))
        folder.mkdir(exist_ok=True)
        notes = json.loads(row['notes_json'])
        for field, name in [('screenshot', 'screen.png'), ('context', 'context.json')]:
            attachment(bucket, row[field + '_key'], folder / name,
                       row[field + '_size'], row[field + '_sha256'])
        (folder / 'report.json').write_text(json.dumps({**row, 'notes': notes}, indent=2) + '\n', encoding='utf-8')
        summary = [f"# Feedback {row['id']}", '', f"Captured: {row['capture_at']}",
                   f"Build: {row['build']}", f"Status: {row['status']}", '', '![Screenshot](screen.png)', '']
        for pin in notes:
            summary.extend([f"## Pin {pin['id']} ({pin['x']:.4f}, {pin['y']:.4f})", '', pin['text'], ''])
        (folder / 'notes.md').write_text('\n'.join(summary), encoding='utf-8')
        print(folder)
    (output / 'index.json').write_text(json.dumps(rows, indent=2) + '\n', encoding='utf-8')
    print(f'Downloaded {len(rows)} report(s); remote reports unchanged.')


if __name__ == '__main__':
    main()
