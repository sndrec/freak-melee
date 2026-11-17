"""Verify agent reports against the real F2 uploader in one hidden, muted client."""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import time
from urllib.request import urlopen

from agent_live_test import close_window


def serve(root):
    root.mkdir(parents=True, exist_ok=True)
    attempts = {}
    reports = {}
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_): pass
        def response(self, status, body):
            encoded = json.dumps(body).encode()
            self.send_response(status)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(encoded)))
            self.end_headers()
            self.wfile.write(encoded)
        def do_GET(self):
            if self.path == '/stop':
                self.server.running = False
                self.response(200, {})
            else:
                self.response(404, {'error': 'Test endpoint only receives feedback.'})
        def do_POST(self):
            assert self.path == '/v1/feedback', self.path
            data = self.rfile.read(int(self.headers['Content-Length']))
            assert data[:8] == b'FMREPORT' and len(data) >= 20
            notes_size, image_size, context_size = struct.unpack_from('<III', data, 8)
            assert len(data) == 20 + notes_size + image_size + context_size
            notes = json.loads(data[20:20 + notes_size])
            image = data[20 + notes_size:20 + notes_size + image_size]
            context = json.loads(data[20 + notes_size + image_size:])
            assert image.startswith(b'\x89PNG\r\n\x1a\n')
            assert notes['id'] == context['id']
            assert notes['capture_at'] == context['captured_at']
            assert context['agent_report']['author'] == 'agent'
            assert context['agent_report']['category'] in ('bug', 'feature_request')
            assert 'Agent ' in notes['pins'][0]['text']
            for pin in notes['pins']:
                assert 0 <= pin['x'] <= 1 and 0 <= pin['y'] <= 1
                assert len(pin['text'].encode('utf-16-le')) // 2 <= 2048
            key = notes['id']
            if key in reports: assert reports[key] == data, 'Retry changed the captured bytes'
            else: reports[key] = data
            attempts[key] = attempts.get(key, 0) + 1
            (root / (key + '.report')).write_bytes(data)
            (root / 'attempts.json').write_text(json.dumps(attempts), encoding='utf-8')
            if attempts[key] == 1:
                self.response(503, {'error': 'Intentional local retry check'})
            else:
                self.response(201 if attempts[key] == 2 else 200, {'id': key, 'stored': True})
    server = HTTPServer(('127.0.0.1', 0), Handler)
    server.running = True
    print(server.server_port, flush=True)
    while server.running: server.handle_request()
    server.server_close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--server', type=Path)
    for field in ('exe', 'tool', 'iso', 'output'):
        parser.add_argument('--' + field, type=Path)
    args = parser.parse_args()
    if args.server:
        serve(args.server)
        return
    assert args.exe and args.tool and args.iso and args.output
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    userdata = output / 'userdata'
    userdata.mkdir()
    windows = Path(os.environ['SystemRoot'])
    env = dict(os.environ, LOCALAPPDATA=str(userdata),
               PATH=str(windows / 'System32') + ';' + str(windows))
    for service in ('CHAT', 'SERVICE'):
        env['FREAK_MELEE_' + service + '_ENDPOINT'] = 'http://127.0.0.1:1'
    with (output / 'server-errors.log').open('w') as errors:
        server = subprocess.Popen([sys.executable, str(Path(__file__).resolve()), '--server', str(output / 'received')],
            stdout=subprocess.PIPE, stderr=errors, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
    # The private helper prints its ephemeral port before accepting requests.
    port = int(server.stdout.readline())
    env['FREAK_MELEE_COMMUNITY_ENDPOINT'] = f'http://127.0.0.1:{port}'
    process = None
    try:
        def call(name, arguments=None, expected=True):
            result = subprocess.run([str(args.tool.resolve()), 'agent', 'call', name, '--stdin'],
                input=json.dumps(arguments or {}), capture_output=True, encoding='utf-8',
                env=env, cwd=output, timeout=30, creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.stdout, (name, result.returncode, result.stderr)
            reply = json.loads(result.stdout)
            assert reply['ok'] == expected, (name, reply)
            return reply['result'] if expected else reply['error']
        with (output / 'stdout.log').open('w') as stdout, (output / 'stderr.log').open('w') as stderr:
            process = subprocess.Popen([str(args.exe.resolve()), '--iso', str(args.iso.resolve()), '--hidden', '--mute'],
                env=env, cwd=output, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        target = None
        for _ in range(300):
            assert process.poll() is None, process.returncode
            for instance in call('instances.list')['instances']:
                if instance['process_id'] == process.pid and instance['available']:
                    target = {'instance': instance['instance']}
                    break
            if target: break
            time.sleep(.2)
        assert target
        state = call('session.inspect', target)
        assert state['project'] is None
        assert call('feedback.list', target)['reports'] == []
        request = dict(target, task_id='packaged-report-test', issue_key='generic-article-capability',
            category='feature_request', summary='LOCAL TEST: generic article capability',
            operation='No preview open', expected='A reusable capability is available', observed='No such operation was found',
            capability='A generic authoring operation', alternatives='Inspected tool and scripting references',
            reproduction='Run the packaged authoring guide', pins=[{'x': .2, 'y': .4, 'text': 'Local test annotation'}])
        report = call('feedback.prepare', dict(request, revision=state['revision']))
        selected = dict(target, report_id=report['id'])
        for _ in range(100):
            report = call('feedback.inspect', selected)
            if report['attachments_ready']: break
            time.sleep(.1)
        assert report['attachments_ready'], report
        assert not report['fighter_files_attached']
        assert report['agent_report']['captured_workshop']['revision'] == state['revision']
        paths = [Path(report[field]) for field in ('screenshot', 'context', 'notes')]
        original = [path.read_bytes() for path in paths]
        context = json.loads(original[1])
        assert context['agent_report']['category'] == 'feature_request'
        assert context['agent_report']['captured_workshop']['build_identity'] == state['build_identity']
        assert call('feedback.prepare', dict(request, summary='Must preserve first report'))['duplicate']
        assert [path.read_bytes() for path in paths] == original
        assert call('feedback.inspect', dict(target, report_id='../../../not-a-report-xxxxxxxxxxxxxx'), expected=False)['code'] == 'unknown_report'
        assert call('feedback.submit', selected)['status'] == 'sending'
        for _ in range(100):
            report = call('feedback.status', selected)
            if report['status'] == 'retry': break
            time.sleep(.1)
        assert report['status'] == 'retry', report
        assert 'Intentional local retry check' in report['error']
        assert [path.read_bytes() for path in paths] == original
        # The next process recovers the same report without taking a new
        # screenshot or relying on the first process's capture ID or receipt.
        close_window(process.pid)
        process.wait(timeout=20)
        assert process.returncode == 0
        with (output / 'restarted-stdout.log').open('w') as stdout, (output / 'restarted-stderr.log').open('w') as stderr:
            process = subprocess.Popen([str(args.exe.resolve()), '--iso', str(args.iso.resolve()), '--hidden', '--mute'],
                env=env, cwd=output, stdout=stdout, stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
        target = None
        for _ in range(300):
            assert process.poll() is None, process.returncode
            for instance in call('instances.list')['instances']:
                if instance['process_id'] == process.pid and instance['available']:
                    target = {'instance': instance['instance']}
                    break
            if target: break
            time.sleep(.2)
        assert target
        request['instance'] = target['instance']
        selected = dict(target, report_id=report['id'])
        assert call('feedback.prepare', request)['id'] == report['id']
        assert [path.read_bytes() for path in paths] == original
        paths[2].write_bytes(original[2] + b' ')
        assert call('feedback.submit', selected, expected=False)['code'] == 'capture_changed'
        paths[2].write_bytes(original[2])
        call('feedback.submit', selected)
        for _ in range(100):
            report = call('feedback.status', selected)
            if report['stored']: break
            time.sleep(.1)
        assert report['stored'], report
        assert call('feedback.submit', selected)['stored']
        attempts = json.loads((output / 'received/attempts.json').read_text())
        assert attempts == {report['id']: 2}, attempts
        assert [path.read_bytes() for path in paths] == original
        assert call('feedback.list', target)['reports'][0]['id'] == report['id']
        (output / 'report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
        # Exercise a second category separately from the first report's retry.
        bug = call('feedback.prepare', dict(request, issue_key='generic-bug', category='bug'))
        assert bug['id'] != report['id'] and bug['agent_report']['category'] == 'bug'
        for _ in range(100):
            current = call('feedback.inspect', dict(target, report_id=bug['id']))
            if current['attachments_ready']: break
            time.sleep(.1)
        assert current['attachments_ready']
        assert call('feedback.discard', dict(target, report_id=bug['id']))['status'] == 'discarded_local'
        assert call('feedback.discard', selected)['status'] == 'discarded_local'
        assert call('feedback.list', target)['reports'] == []
        assert call('session.inspect', target)['revision'] == state['revision']
        print('PASS: capture-time agent bug/feature reports, F2 binary upload, failed retry, sealed bytes, duplicate suppression and local cleanup')
    finally:
        try:
            if process:
                if process.poll() is None:
                    close_window(process.pid)
                    process.wait(timeout=20)
                assert process.returncode == 0, process.returncode
        finally:
            urlopen(f'http://127.0.0.1:{port}/stop', timeout=3).close()
            server.wait(timeout=5)
            assert server.returncode == 0, server.returncode


if __name__ == '__main__': main()
