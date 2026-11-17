"""Check the installed reference against its actual packaged file graph."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('workshop_docs', ROOT/'tools/prepare_workshop_docs.py')
docs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(docs)


class WorkshopDocs(unittest.TestCase):
    def test_real_reference_is_self_contained(self):
        with tempfile.TemporaryDirectory(prefix='freak-reference-') as temporary:
            output = Path(temporary)/'ui/docs'
            pages, examples = docs.prepare(output)
            self.assertGreaterEqual(pages, 88)
            self.assertGreaterEqual(examples, 13)
            self.assertTrue((output/'holdable_items.md').is_file())
            self.assertTrue((output.parent/'mario_full.fms').is_file())
            self.assertTrue((output.parent/'fox_falco_blaster.fms').is_file())
            for file in output.glob('*.md'):
                for line, prose in docs.prose_lines(file.read_text(encoding='utf-8')):
                    if not prose:
                        continue
                    for label, href in docs.LINK.findall(line):
                        self.assertNotIn('../../tests/', href)
                        if href.startswith(docs.REPOSITORY):
                            self.assertIn('developer reference, online', label)
            docs.check_tree(output)

    def test_missing_files_sections_and_unlisted_payloads_fail(self):
        with tempfile.TemporaryDirectory(prefix='freak-reference-gates-') as temporary:
            root = Path(temporary)
            index = root/'index.md'
            index.write_text('# Overview\n\n[Details](other.md#details)\n', encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'unbundled file'):
                docs.check_tree(root)
            other = root/'other.md'
            other.write_text('# Other\n\n## Something else\n', encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'unknown section'):
                docs.check_tree(root)
            other.write_text('# Other\n\n## Details\n', encoding='utf-8')
            docs.check_tree(root)
            with self.assertRaisesRegex(ValueError, 'unbundled file'):
                docs.check_tree(root, [index])

    def test_code_examples_are_not_hyperlinks_or_headings(self):
        text = '# Visible\n```text\n# Hidden\n[Example](missing.md)\n```\n## Visible\n'
        self.assertEqual(docs.anchors(text), {'visible', 'visible-1'})
        with tempfile.TemporaryDirectory(prefix='freak-reference-code-') as temporary:
            root = Path(temporary)
            (root/'index.md').write_text(text, encoding='utf-8')
            docs.check_tree(root)

    def test_empty_reference_is_not_successful(self):
        with tempfile.TemporaryDirectory(prefix='freak-reference-empty-') as temporary:
            with self.assertRaisesRegex(ValueError, 'missing index'):
                docs.check_tree(temporary)


if __name__ == '__main__':
    unittest.main()
