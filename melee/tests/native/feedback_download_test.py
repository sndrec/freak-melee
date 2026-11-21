"""Exercise the feedback review CLI against isolated SQL and object storage."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest
from unittest.mock import patch
from uuid import uuid4

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import download_native_feedback as feedback


class FeedbackDownload(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.output = Path(temporary.name)
        self.db = sqlite3.connect(':memory:')
        self.addCleanup(self.db.close)
        self.db.row_factory = sqlite3.Row
        for file in sorted((ROOT / 'services/community/migrations').glob('*.sql')):
            self.db.executescript(file.read_text())
        self.objects = {}
        self.interrupt_next_delete = False
        self.page_sizes = []
        mock = patch.object(feedback, 'wrangler', side_effect=self.command)
        mock.start()
        self.addCleanup(mock.stop)

    def command(self, *args):
        if args[0] == 'd1':
            sql = args[args.index('--command') + 1]
            if sql.startswith('SELECT'):
                self.page_sizes.append(int(sql.rsplit(' LIMIT ', 1)[1]))
            rows = [dict(row) for row in self.db.execute(sql)]
            return json.dumps([{'results': rows}])
        key = args[3].split('/', 1)[1]
        if args[2] == 'delete':
            if self.interrupt_next_delete:
                self.interrupt_next_delete = False
                raise RuntimeError('Interrupted after SQL commit')
            self.objects.pop(key, None)
        elif args[2] == 'get':
            Path(args[args.index('--file') + 1]).write_bytes(self.objects[key])
        else:
            self.fail('Unexpected storage command')

    def add_report(self, key):
        prefix = f'feedback/{key}/{uuid4()}'
        image, context = prefix + '/screen.png', prefix + '/context.bin'
        self.objects.update({image: b'screen', context: b'context'})
        self.db.execute('INSERT INTO feedback VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)', (
            key, 'now', 'test build', 17, image, context, '[]',
            hashlib.sha256(b'screen').hexdigest(), hashlib.sha256(b'context').hexdigest(),
            6, 7, 1, 1, 'new'))
        return image, context

    def test_pages_tied_timestamps_and_preserves_previous_index_on_failure(self):
        keys = sorted((str(uuid4()) for _ in range(19)), reverse=True)
        records = {k: self.add_report(k) for k in keys}
        argv = ['feedback', '--all', '--output', str(self.output)]
        with patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
            feedback.main()
            index = self.output / 'index.json'
            previous = index.read_bytes()
            self.assertEqual([row['id'] for row in json.loads(previous)], keys)
            self.assertEqual(self.page_sizes, [8, 8, 8])
            local = self.output / keys[-1] / 'context.json'
            local.write_bytes(b'old local file')
            self.objects[records[keys[-1]][1]] = b'interrupted'
            with self.assertRaisesRegex(RuntimeError, 'verification'):
                feedback.main()
            self.assertEqual(index.read_bytes(), previous)
            self.assertEqual(local.read_bytes(), b'old local file')
            self.assertEqual(list(local.parent.glob('*.download')), [])

    def test_retry_after_deletion_preserves_later_submission_with_same_id(self):
        key = str(uuid4())
        first = self.add_report(key)
        self.interrupt_next_delete = True
        with self.assertRaisesRegex(RuntimeError, 'Interrupted'):
            feedback.remove_report(key, 'test-bucket', self.output)
        second = self.add_report(key)
        with contextlib.redirect_stdout(io.StringIO()):
            feedback.remove_report(key, 'test-bucket', self.output)
        self.assertEqual([(r[0], r[1]) for r in self.db.execute('SELECT screenshot_key, context_key FROM feedback')], [second])
        self.assertTrue(all(k in self.objects for k in second))
        self.assertTrue(all(k not in self.objects for k in first))
        self.assertTrue(json.loads((self.output / key / 'removal.json').read_text())['deleted'])


if __name__ == '__main__':
    unittest.main()
