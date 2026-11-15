"""Download feedback notes, screenshots and diagnostics using the owner's Wrangler login."""
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('--id', help='Download one report UUID')
    selection.add_argument('--all', action='store_true', help='Download every matching report')
    parser.add_argument('--status', choices=['new', 'investigating', 'resolved'])
    parser.add_argument('--limit', type=int, default=1, help='Newest reports to download (default: 1)')
    parser.add_argument('--output', type=Path, default=SERVICE.parents[2] / '.local/feedback')
    args = parser.parse_args()
    if args.limit < 1:
        parser.error('--limit must be positive')
    if not WRANGLER.is_file():
        parser.error('Install services/community dependencies with npm ci, then run wrangler login there.')
    config = json.loads((SERVICE / 'wrangler.jsonc').read_text())
    bucket = next(b['bucket_name'] for b in config['r2_buckets'] if b['binding'] == 'REPORTS')
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
