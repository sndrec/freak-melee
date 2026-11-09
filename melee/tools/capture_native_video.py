"""Record the complete native client through its render output, without desktop capture."""
import secrets
import subprocess
import time
from pathlib import Path


class NativeVideo:
    def __init__(self, client, output, fps=60, ffmpeg='ffmpeg', realtime=True, sound=True):
        self.client = client
        self.output = Path(output).resolve()
        self.pipe = '\\\\.\\pipe\\freak-melee-video-' + secrets.token_hex(8)
        self.log = self.output.with_suffix('.encode.log').open('w')
        self.process = None
        self.audio_path = self.output.with_suffix('.wav') if sound else None
        start = client.stderr.stat().st_size
        self.log_start = start
        client.command('video-start', self.pipe)
        deadline = time.monotonic()+20
        while time.monotonic()<deadline:
            with client.stderr.open(errors='replace') as file:
                file.seek(start); text=file.read()
            if 'Listening '+self.pipe in text:
                command = [
                    ffmpeg, '-hide_banner', '-loglevel', 'warning', '-y',
                    '-f', 'image2pipe', '-framerate', str(fps), '-vcodec', 'ppm']
                if realtime: command += ['-use_wallclock_as_timestamps','1']
                command += ['-i', self.pipe, '-r', str(fps), '-fps_mode', 'cfr',
                    '-vf', 'pad=ceil(iw/2)*2:ceil(ih/2)*2',
                    '-an', '-c:v', 'libx264', '-preset', 'fast', '-crf', '17', '-pix_fmt', 'yuv420p',
                    '-movflags', '+faststart', str(self.output)]
                self.process = subprocess.Popen(command,
                    stdout=self.log, stderr=self.log, creationflags=subprocess.CREATE_NO_WINDOW)
                break
            if client.process.poll() is not None: break
            time.sleep(.05)
        if not self.process:
            self.log.close()
            raise RuntimeError('Native video pipe did not open. Check '+str(client.stderr))
        if self.audio_path: client.command('audio-start', str(self.audio_path))

    def close(self):
        if not self.process: return
        try:
            self.client.command('video-stop')
            if self.audio_path: self.client.command('audio-stop')
            self.process.wait(timeout=30)
            if self.process.returncode:
                raise RuntimeError('Video encoder failed: '+str(self.output.with_suffix('.encode.log')))
            with self.client.stderr.open(errors='replace') as file:
                file.seek(self.log_start); text=file.read()
            if 'frames; failed=true' in text:
                raise RuntimeError('Native video readback/write failed: '+str(self.client.stderr))
        finally:
            if self.process.poll() is None:
                self.process.terminate(); self.process.wait(timeout=10)
            self.log.close(); self.process=None

    def __enter__(self): return self
    def __exit__(self, *exception): self.close()
