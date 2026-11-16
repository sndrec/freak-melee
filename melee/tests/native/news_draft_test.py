"""Check draft creation and retries against the community's actual SQL schema."""
import importlib.util
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest
from unittest.mock import patch
from uuid import uuid4


ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('news_draft', ROOT / 'tools/save_native_news_draft.py')
draft = importlib.util.module_from_spec(spec)
spec.loader.exec_module(draft)


class NewsDraft(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='freak-news-test-')
        self.addCleanup(temporary.cleanup)
        self.receipt = Path(temporary.name) / 'news.json'
        self.db = sqlite3.connect(':memory:')
        self.addCleanup(self.db.close)
        self.db.row_factory = sqlite3.Row
        self.db.executescript((ROOT / 'services/community/migrations/0001_community.sql').read_text())
        self.writes = 0

        def execute(sql, write=False):
            self.writes += int(write)
            result = self.db.execute(sql)
            rows = [dict(row) for row in result] if result.description else []
            self.db.commit()
            return rows

        mock = patch.object(draft, 'execute', side_effect=execute)
        mock.start()
        self.addCleanup(mock.stop)

    def save(self, title='Release notes', body='Only patch notes.'):
        return draft.save_draft('release-test', title, body, self.receipt)

    def test_creates_draft_with_exact_unicode_and_statement_looking_text(self):
        text = "# What's new?\n\n'); DELETE FROM news; --\nUnicode: 🎮 / café / 中文"
        result = self.save(body=text)
        self.assertFalse(result['published'])
        self.assertIsNone(result['published_at'])
        self.assertEqual(result['ticker'], '')
        self.assertFalse(result['archived'])
        self.assertTrue(result['body_matches'])
        row = self.db.execute('SELECT * FROM news').fetchone()
        self.assertEqual(row['body'], text)
        self.assertEqual(row['revision'], 1)
        self.assertEqual(self.db.execute('SELECT count(*) FROM announcements').fetchone()[0], 0)
        self.assertEqual(json.loads(self.receipt.read_text()), result)

    def test_repeat_keeps_one_record_and_does_not_write_again(self):
        a, b = self.save(), self.save()
        self.assertEqual(a['id'], b['id'])
        self.assertEqual(b['revision'], 1)
        self.assertEqual(self.writes, 1)
        self.assertEqual(self.db.execute('SELECT count(*) FROM news').fetchone()[0], 1)

    def test_preserves_owner_edits_and_publication_even_without_receipt(self):
        a = self.save()
        self.db.execute("UPDATE news SET title='My title',body='Personal notes',ticker='Edited',"
                        'published_at=123,archived=1,revision=2')
        self.receipt.unlink()
        b = self.save(body='Different generated notes')
        self.assertEqual(a['id'], b['id'])
        self.assertEqual(b['title'], 'My title')
        self.assertEqual(b['revision'], 2)
        self.assertTrue(b['published'])
        self.assertTrue(b['archived'])
        self.assertFalse(b['body_matches'])
        self.assertEqual(self.writes, 1)
        self.assertEqual(self.db.execute('SELECT body FROM news').fetchone()[0], 'Personal notes')

    def test_reuses_earlier_manually_created_article(self):
        key = str(uuid4())
        self.db.execute("INSERT INTO news(id,title,ticker,body,updated_at) VALUES(?,?,'',?,1)",
                        (key, 'Release notes', 'Only patch notes.'))
        a = self.save()
        self.assertEqual(a['id'], key)
        self.assertEqual(self.writes, 0)
        self.assertTrue(a['body_matches'])

    def test_duplicate_title_is_an_error_with_no_write(self):
        self.save()
        self.db.execute("INSERT INTO news(id,title,ticker,body,updated_at) VALUES(?,'Release notes','','Other',1)",
                        (str(uuid4()),))
        with self.assertRaisesRegex(RuntimeError, 'duplicate titles'):
            self.save()
        self.assertEqual(self.writes, 1)

    def test_limits_and_receipt_identity_are_checked_before_writing(self):
        for body in ['', '\0', 'a' * 32001, '🎮' * 16001]:
            with self.assertRaises(ValueError):
                self.save(body=body)
        with self.assertRaises(ValueError):
            draft.save_draft("'; DROP TABLE news;", 'Title', 'Body', self.receipt)
        self.save()
        with self.assertRaisesRegex(ValueError, 'another draft'):
            draft.save_draft('another-release', 'Title', 'Body', self.receipt)
        self.assertEqual(self.writes, 1)


class WranglerOutput(unittest.TestCase):
    def test_file_import_may_return_no_json_and_article_text_stays_out_of_command(self):
        sql = 'INSERT INTO news VALUES(' + draft.sql_text("'; & echo do-not-run") + ');'

        def run(args, **kwargs):
            path = Path(args[args.index('--file') + 1])
            self.assertEqual(path.read_text(), sql)
            self.assertNotIn(sql, args)
            self.assertIn('--remote', args)
            return subprocess.CompletedProcess(args, 0, stdout='', stderr='')

        # Use the real transport here, rather than the SQLite substitute above.
        with patch.object(draft.subprocess, 'run', side_effect=run):
            self.assertEqual(draft.execute(sql, write=True), [])


if __name__ == '__main__':
    unittest.main()
