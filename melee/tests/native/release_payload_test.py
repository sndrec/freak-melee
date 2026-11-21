"""Check release completeness, build-directory isolation and publication gates.

Synthetic files exercise packaging only. Native startup and Workshop acceptance
are recorded separately against an extracted installation of the actual build.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.primitives import hashes

ROOT = Path(__file__).resolve().parents[2]
PREPARE = ROOT/'tools/prepare_native_release.py'
PUBLISH = ROOT/'tools/publish_native_release.py'
CHANNEL = 'https://freak-melee-releases.sndrec32exe.workers.dev/3ShgcrpBLwN2237YIwwm6kX0/latest.json'
sys.path.insert(0, str(ROOT/'tools'))
import release_signer


class ReleasePayload(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_key = rsa.generate_private_key(65537, 3072)

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='freak-release-test-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.build = self.root/'build'
        self.output = self.root/'prepared'
        self.build.mkdir()
        assets = ['ui/freak_shell.rml', 'ui/docs/index.md', 'ui/mario_full.fms', 'ui/skins/menu-panel.bin',
                  'ui/portraits/tile-frame.png', 'disc/audio/first_startup.hps',
                  'ui/yoshi_wip.fms', *[f'ui/fonts/{name}.bin' for name in
                  ['menu-heading','result-name','result-winner','notice','css-tile','menu-option','menu-caption']]]
        for name in ['melee_native.exe','native_content_tool.exe','SDL3.dll',
                     'native_fighter_test_fixture.dll','private.iso', *assets, 'ui/retired.fms']:
            path = self.build/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(('synthetic '+name).encode())
        # The two executable dependency lists can name the same DLL.
        (self.build/'native_release_runtime.txt').write_text(str(self.build/'SDL3.dll')+'\n'+str(self.build/'SDL3.dll')+'\n')
        (self.build/'native_release_ui.txt').write_text('\n'.join(assets)+'\n')

    def prepare(self, signed=False):
        command = [sys.executable, str(PREPARE), '--build', str(self.build),
                   '--output', str(self.output), '--release', 'test-a', '--channel', CHANNEL]
        if signed:
            self.public = self.root/'disposable.pub'
            self.public.write_text(release_signer.modulus(self.test_key), encoding='ascii')
            command += ['--public-key', str(self.public)]
        result = subprocess.run(command, capture_output=True, text=True)
        if signed and result.returncode == 0:
            self.run_publisher_with_r2('--sign-only', {})
        return result

    def publish(self):
        return subprocess.run([sys.executable, str(PUBLISH), str(self.output), '--dry-run'],
                              capture_output=True, text=True)

    @unittest.skipUnless(shutil.which('powershell.exe'), 'Windows launcher')
    def test_launcher_verifies_download_and_installed_files(self):
        result = self.prepare(signed=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        install = self.root / 'install'
        install.mkdir()
        (install / 'versions').mkdir()
        launcher = install / 'freak_launcher.ps1'
        shutil.copyfile(self.output/'install/freak_launcher.ps1', launcher)
        state = install / 'state.json'
        state.write_text(json.dumps({'current': 'prior', 'previous': None, 'blocked': None}))
        channel = self.output / 'latest.json'

        def check():
            return subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive',
                                   '-ExecutionPolicy', 'Bypass', '-File', str(launcher),
                                   '-Channel', str(channel), '-CheckOnly'],
                                  capture_output=True, text=True, timeout=60)

        original = channel.read_bytes()
        # A local test manifest is selected explicitly on the command line.
        (install / 'channel.txt').write_text(str(channel))
        configured = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive',
                                     '-ExecutionPolicy', 'Bypass', '-File', str(launcher), '-CheckOnly'],
                                    capture_output=True, text=True, timeout=60)
        self.assertNotEqual(configured.returncode, 0)
        self.assertEqual(json.loads(state.read_text())['current'], 'prior')
        signature_path = self.output/release_signer.signature_name(original)
        saved_signature = signature_path.read_bytes()
        signature_path.unlink()
        self.assertEqual(check().returncode, 0)
        self.assertEqual(json.loads(state.read_text())['current'], 'prior')
        other_signer = rsa.generate_private_key(65537, 3072)
        signature_path.write_bytes(other_signer.sign(original, padding.PKCS1v15(), hashes.SHA256()))
        self.assertIn('signature is invalid', check().stdout)
        self.assertEqual(json.loads(state.read_text())['current'], 'prior')
        signature_path.write_bytes(saved_signature)
        mismatch = json.loads(original)
        mismatch['files'][0]['size'] += 1
        channel.write_text(json.dumps(mismatch))
        changed = channel.read_bytes()
        (self.output/release_signer.signature_name(changed)).write_bytes(
            (self.output/release_signer.signature_name(original)).read_bytes())
        self.assertIn('signature is invalid', check().stdout)
        self.assertEqual(json.loads(state.read_text())['current'], 'prior')
        target = install / 'versions/test-a'
        self.assertFalse(target.exists())
        (self.output/release_signer.signature_name(changed)).write_bytes(
            self.test_key.sign(changed, padding.PKCS1v15(), hashes.SHA256()))
        self.assertIn('Archive file list or sizes differ', check().stdout)
        self.assertFalse(target.exists())
        self.assertEqual(json.loads(state.read_text())['current'], 'prior')

        channel.write_bytes(original)
        result = check()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(state.read_text())['current'], 'test-a', result.stdout)
        self.assertEqual(json.loads(state.read_text())['latest_ms'], json.loads(original)['issued_ms'])
        self.assertEqual(check().returncode, 0)
        earlier = dict(json.loads(original), release='older-test', issued_ms=json.loads(original)['issued_ms'] - 1)
        earlier['archive'] = 'freak-melee-older-test.zip'
        shutil.copyfile(self.output/'freak-melee-test-a.zip', self.output/earlier['archive'])
        data = json.dumps(earlier).encode()
        channel.write_bytes(data)
        (self.output/release_signer.signature_name(data)).write_bytes(
            self.test_key.sign(data, padding.PKCS1v15(), hashes.SHA256()))
        result = check()
        self.assertIn('older release', result.stdout)
        self.assertEqual(json.loads(state.read_text())['current'], 'test-a')
        # An older launcher writes the new launcher to disk before it knows
        # about latest_ms. The new launcher also carries its own minimum.
        migrated = json.loads(state.read_text())
        del migrated['latest_ms']
        state.write_text(json.dumps(migrated))
        self.assertIn('older release', check().stdout)
        self.assertEqual(json.loads(state.read_text()), migrated)
        channel.write_bytes(original)
        self.assertEqual(check().returncode, 0)
        self.assertEqual(json.loads(state.read_text())['latest_ms'], json.loads(original)['issued_ms'])
        (target / 'melee_native.exe').write_bytes(b'incomplete local copy')
        self.assertNotEqual(check().returncode, 0)

    @unittest.skipUnless(shutil.which('powershell.exe'), 'Windows launcher')
    def test_update_redirects_require_https_before_request(self):
        # A local HTTP stub records destinations. No requests leave the process.
        script = r"""
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Net.Http
$ast = [Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path tools/freak_launcher.ps1).Path, [ref]$null, [ref]$null)
$fn = $ast.Find({ param($n) $n -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $n.Name -eq 'Get-UpdateResponse' }, $true)
. ([scriptblock]::Create($fn.Extent.Text))
$client = [pscustomobject]@{ Dest = '/archive'; Paths = [Collections.Generic.List[string]]::new() }
$client | Add-Member ScriptMethod GetAsync {
    param($uri, $completion, $token)
    $this.Paths.Add($uri.AbsoluteUri)
    $response = [Net.Http.HttpResponseMessage]::new([Net.HttpStatusCode]::OK)
    if ($uri.AbsolutePath -eq '/first') {
        $response.StatusCode = [Net.HttpStatusCode]::Redirect
        $response.Headers.Location = [uri]::new($this.Dest, [UriKind]::RelativeOrAbsolute)
    }
    $task = [Threading.Tasks.TaskCompletionSource[Net.Http.HttpResponseMessage]]::new()
    $task.SetResult($response)
    return $task.Task
}
$start = [uri]'https://updates.invalid/first'
$mode = [Net.Http.HttpCompletionOption]::ResponseHeadersRead
$token = [Threading.CancellationToken]::None
$response = Get-UpdateResponse $client $start $mode $token
if ([int]$response.StatusCode -ne 200 -or $client.Paths.Count -ne 2 -or
    $client.Paths[1] -ne 'https://updates.invalid/archive') { throw 'HTTPS redirect failed' }
$response.Dispose()
$client.Dest = 'http://updates.invalid/archive'
$client.Paths.Clear()
try {
    $null = Get-UpdateResponse $client $start $mode $token
    throw 'Expected insecure redirect to be declined'
} catch [Security.SecurityException] {
    if ($client.Paths.Count -ne 1) { throw 'Requested an insecure destination' }
}
exit 0
"""
        result = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive',
                                 '-Command', script], cwd=ROOT, capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_complete_payload_matches_both_archives_and_excludes_build_junk(self):
        result = self.prepare()
        self.assertEqual(result.returncode, 0, result.stderr)
        manifest = json.loads((self.output/'latest.json').read_text())
        with zipfile.ZipFile(self.output/manifest['archive']) as archive:
            names = archive.namelist()
            self.assertIn('native_content_tool.exe', names)
            self.assertEqual(archive.read('START_HERE.md'), (ROOT/'docs/scripting/agent_tools.md').read_bytes())
            self.assertIn('disc/audio/first_startup.hps', names)
            self.assertIn('ui/portraits/tile-frame.png', names)
            self.assertIn('licenses/monocypher.txt', names)
            for notice in (ROOT/'licenses').rglob('*'):
                if notice.is_file():
                    self.assertEqual(archive.read('licenses/' + notice.relative_to(ROOT/'licenses').as_posix()),
                                     notice.read_bytes())
            for notice in ['freak-melee.txt', 'sources.md', 'aurora.txt', 'cgltf.txt', 'core-math.txt']:
                self.assertIn('licenses/' + notice, names)
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

    def test_reference_cannot_link_to_an_unpackaged_build_file(self):
        (self.build/'ui/docs/index.md').write_text('[Old guide](old.md)\n')
        (self.build/'ui/docs/old.md').write_text('# Old guide\n')
        result = self.prepare()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('unbundled file old.md', result.stderr)

    def test_reference_cannot_link_to_a_missing_section(self):
        (self.build/'ui/docs/index.md').write_text('# Guide\n[Missing](#missing)\n')
        result = self.prepare()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('unknown section #missing', result.stderr)

    def test_source_build_without_optional_startup_track_can_be_packaged(self):
        listing = self.build/'native_release_ui.txt'
        listing.write_text('\n'.join(name for name in listing.read_text().splitlines()
                                     if name != 'disc/audio/first_startup.hps') + '\n')
        (self.build/'disc/audio/first_startup.hps').unlink()
        self.assertEqual(self.prepare().returncode, 0)
        self.assertEqual(self.publish().returncode, 0)
        with zipfile.ZipFile(self.output/'freak-melee-test-a.zip') as archive:
            self.assertNotIn('disc/audio/first_startup.hps', archive.namelist())

    def run_publisher_with_r2(self, mode, remote):
        spec = importlib.util.spec_from_file_location('release_publisher', PUBLISH)
        publisher = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(publisher)
        commands = []

        def wrangler(*args):
            commands.append(args)
            filename = Path(args[args.index('--file') + 1])
            key = args[3]
            if args[2] == 'put':
                remote[key] = filename.read_bytes()
            else:
                filename.write_bytes(remote[key])

        # The local fake replaces only external R2 commands. Archive and
        # manifest validation still run against the real packaging fixture.
        with patch.object(publisher, 'wrangler', wrangler), \
             patch.object(publisher, 'PUBLIC_KEY', self.public), \
             patch.object(release_signer, 'load_private', return_value=self.test_key), \
             patch.object(release_signer, 'signing_key_path', return_value=self.root/'unused-test-key'), \
             patch.object(publisher, 'WRANGLER', Path(__file__)), \
             patch.object(sys, 'argv', [str(PUBLISH), str(self.output), mode]):
            publisher.main()
        return commands

    def test_staging_keeps_live_manifest_and_activation_writes_it_last(self):
        self.assertEqual(self.prepare(signed=True).returncode, 0)
        remote = {'freak-melee-releases/latest.json': b'previous live manifest'}
        staged = self.run_publisher_with_r2('--stage-only', remote)
        self.assertEqual(remote['freak-melee-releases/latest.json'], b'previous live manifest')
        self.assertEqual(sum(command[2] == 'put' for command in staged), 3)
        activated = self.run_publisher_with_r2('--activate-only', remote)
        self.assertEqual([command[2] for command in activated], ['get', 'get', 'get', 'put'])
        self.assertEqual(activated[-1][3], 'freak-melee-releases/latest.json')
        self.assertEqual(remote['freak-melee-releases/latest.json'], (self.output/'latest.json').read_bytes())

    def test_activation_refuses_corrupt_staged_archive_without_changing_live_manifest(self):
        self.assertEqual(self.prepare(signed=True).returncode, 0)
        remote = {'freak-melee-releases/latest.json': b'previous live manifest'}
        self.run_publisher_with_r2('--stage-only', remote)
        remote['freak-melee-releases/freak-melee-test-a.zip'] = b'corrupted upload'
        with self.assertRaisesRegex(SystemExit, 'latest.json was not changed'):
            self.run_publisher_with_r2('--activate-only', remote)
        self.assertEqual(remote['freak-melee-releases/latest.json'], b'previous live manifest')

    def test_activation_requires_the_staged_signature(self):
        self.assertEqual(self.prepare(signed=True).returncode, 0)
        remote = {'freak-melee-releases/latest.json': b'previous live manifest'}
        self.run_publisher_with_r2('--stage-only', remote)
        name = release_signer.signature_name((self.output/'latest.json').read_bytes())
        remote['freak-melee-releases/' + name] = b'incomplete'
        with self.assertRaisesRegex(SystemExit, 'latest.json was not changed'):
            self.run_publisher_with_r2('--activate-only', remote)
        self.assertEqual(remote['freak-melee-releases/latest.json'], b'previous live manifest')

    @unittest.skipUnless(sys.platform == 'win32', 'Windows publishing key')
    def test_encrypted_backup_can_restore_the_same_publisher(self):
        path = self.root/'publisher.dpapi'
        pub = self.root/'publisher.pub'
        backup = self.root/'backup.pem'
        pub.write_text(release_signer.modulus(self.test_key))
        release_signer.store_private(self.test_key, path)
        password = 'temporary recovery test only'
        with patch.object(release_signer, 'signing_key_path', return_value=path), \
             patch.object(release_signer, 'PUBLIC_KEY', pub), \
             patch.object(sys, 'argv', ['release_signer.py', 'backup', str(backup)]), \
             patch.object(release_signer, 'getpass', return_value=password):
            release_signer.main()
            self.assertIn(b'BEGIN ENCRYPTED PRIVATE KEY', backup.read_bytes())
            with self.assertRaises(FileExistsError):
                release_signer.main()
            sys.argv[1] = 'restore'
            with self.assertRaises(FileExistsError):
                release_signer.main()
            path.unlink()
            with patch.object(release_signer, 'getpass', return_value='other'):
                with self.assertRaises(ValueError):
                    release_signer.main()
            self.assertFalse(path.exists())
            release_signer.main()
            signature = release_signer.sign(b'local test manifest', release_signer.public_modulus(pub))
            release_signer.verify(b'local test manifest', signature, release_signer.public_modulus(pub))

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
