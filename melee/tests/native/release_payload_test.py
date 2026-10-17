"""Check release completeness, build-directory isolation and publication gates.

Synthetic files exercise packaging only. Native startup and Workshop acceptance
are recorded separately against an extracted installation of the actual build.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
PREPARE = ROOT/'tools/prepare_native_release.py'
PUBLISH = ROOT/'tools/publish_native_release.py'
CHANNEL = 'https://freak-melee-releases.sndrec32exe.workers.dev/3ShgcrpBLwN2237YIwwm6kX0/latest.json'


class ReleasePayload(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='freak-release-test-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.build = self.root/'build'
        self.output = self.root/'prepared'
        self.build.mkdir()
        assets = ['ui/freak_shell.rml', 'ui/docs/index.md', 'ui/mario_full.fms',
                  'ui/yoshi_wip.fms', *[f'ui/fonts/{name}.bin' for name in
                  ['menu-heading','result-name','result-winner','notice','css-tile']]]
        for name in ['melee_native.exe','native_content_tool.exe','SDL3.dll',
                     'native_fighter_test_fixture.dll','private.iso', *assets, 'ui/retired.fms']:
            path = self.build/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(('synthetic '+name).encode())
        # The two executable dependency lists can name the same DLL.
        (self.build/'native_release_runtime.txt').write_text(str(self.build/'SDL3.dll')+'\n'+str(self.build/'SDL3.dll')+'\n')
        (self.build/'native_release_ui.txt').write_text('\n'.join(assets)+'\n')

    def prepare(self):
        return subprocess.run([sys.executable, str(PREPARE), '--build', str(self.build),
                               '--output', str(self.output), '--release', 'test-a', '--channel', CHANNEL],
                              capture_output=True, text=True)

    def publish(self):
        return subprocess.run([sys.executable, str(PUBLISH), str(self.output), '--dry-run'],
                              capture_output=True, text=True)

    def test_complete_payload_matches_both_archives_and_excludes_build_junk(self):
        result = self.prepare()
        self.assertEqual(result.returncode, 0, result.stderr)
        manifest = json.loads((self.output/'latest.json').read_text())
        with zipfile.ZipFile(self.output/manifest['archive']) as archive:
            names = archive.namelist()
            self.assertIn('native_content_tool.exe', names)
            self.assertEqual(names.count('SDL3.dll'), 1)
            self.assertNotIn('native_fighter_test_fixture.dll', names)
            self.assertNotIn('ui/retired.fms', names)
            self.assertNotIn('private.iso', names)
            self.assertEqual(set(names), {entry['path'] for entry in manifest['files']})
            for entry in manifest['files']:
                content = archive.read(entry['path'])
                self.assertEqual(len(content), entry['size'])
                self.assertEqual(hashlib.sha256(content).hexdigest(), entry['sha256'])
            with zipfile.ZipFile(self.output/'freak-melee-setup-test-a.zip') as setup:
                for name in names:
                    self.assertEqual(setup.read('versions/test-a/'+name), archive.read(name))
        self.assertEqual(self.publish().returncode, 0)

    def test_missing_workshop_tool_prevents_any_release(self):
        (self.build/'native_content_tool.exe').unlink()
        result = self.prepare()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('native_content_tool.exe', result.stderr)
        self.assertFalse(self.output.exists())

    def test_declared_missing_template_prevents_any_release(self):
        (self.build/'ui/yoshi_wip.fms').unlink()
        result = self.prepare()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('ui/yoshi_wip.fms', result.stderr)
        self.assertFalse(self.output.exists())

    def test_missing_cmake_dependency_list_prevents_guessing(self):
        (self.build/'native_release_runtime.txt').unlink()
        result = self.prepare()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('runtime dependency list', result.stderr)
        self.assertFalse(self.output.exists())

    def test_publisher_rejects_update_without_workshop_tool_even_with_matching_hashes(self):
        self.assertEqual(self.prepare().returncode, 0)
        manifest_path = self.output/'latest.json'
        manifest = json.loads(manifest_path.read_text())
        archive_path = self.output/manifest['archive']
        with zipfile.ZipFile(archive_path) as archive:
            content = {name: archive.read(name) for name in archive.namelist() if name != 'native_content_tool.exe'}
        with zipfile.ZipFile(archive_path, 'w') as archive:
            for name, data in content.items():
                archive.writestr(name, data)
        manifest['files'] = [entry for entry in manifest['files'] if entry['path'] != 'native_content_tool.exe']
        manifest['size'] = archive_path.stat().st_size
        manifest['sha256'] = hashlib.sha256(archive_path.read_bytes()).hexdigest()
        manifest_path.write_text(json.dumps(manifest))
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Workshop tool', result.stderr)

    def test_publisher_rejects_setup_without_workshop_tool(self):
        self.assertEqual(self.prepare().returncode, 0)
        path = self.output/'freak-melee-setup-test-a.zip'
        with zipfile.ZipFile(path) as archive:
            content = {name: archive.read(name) for name in archive.namelist()
                       if name != 'versions/test-a/native_content_tool.exe'}
        with zipfile.ZipFile(path, 'w') as archive:
            for name, data in content.items():
                archive.writestr(name, data)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('installer is incomplete', result.stderr)

    def test_publisher_rejects_corrupt_setup_asset(self):
        self.assertEqual(self.prepare().returncode, 0)
        path = self.output/'freak-melee-setup-test-a.zip'
        with zipfile.ZipFile(path) as archive:
            content = {name: archive.read(name) for name in archive.namelist()}
        content['versions/test-a/ui/fonts/notice.bin'] = b'corrupt font'
        with zipfile.ZipFile(path, 'w') as archive:
            for name, data in content.items():
                archive.writestr(name, data)
        result = self.publish()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Friend installer file differs', result.stderr)


if __name__ == '__main__':
    unittest.main()
