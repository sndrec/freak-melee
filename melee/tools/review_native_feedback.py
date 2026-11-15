"""Review F2 annotation, immutable attachments, resize and upload retry in a hidden, muted client."""
from argparse import ArgumentParser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import hashlib
import json
import os
import re
from pathlib import Path
import struct
import subprocess
import threading
import time

from PIL import Image
from review_native_css_room import Client
from review_native_release import close_window


def wait_visible(client, name, timeout=30):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        text = client.inspect(name)
        if "visible=1" in text:
            return text
        time.sleep(.15)
    raise AssertionError(f"{name} did not appear: {text}")


def picture(client, output):
    # The ordinary screenshot slot belongs to the original F2 capture. Use
    # the existing full-interface video recorder to inspect the annotation UI.
    stream = output.with_suffix(".ppm")
    client.command("video-start", stream)
    time.sleep(.12)
    client.command("video-stop")
    time.sleep(.25)
    with Image.open(stream) as image:
        image.save(output)
    stream.unlink()


def main():
    parser = ArgumentParser(description=__doc__)
    for name in ("exe", "iso", "profile", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    packets = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def answer(self, code, data):
            body = json.dumps(data).encode()
            self.send_response(code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            self.answer(200, {"headline": None, "announcements": [], "announcements_next": None})

        def do_POST(self):
            assert self.path == "/v1/feedback"
            body = self.rfile.read(int(self.headers["Content-Length"]))
            packets.append(body)
            (output / f"request-{len(packets)}.bin").write_bytes(body)
            a, b, c = struct.unpack_from("<III", body, 8)
            assert body[:8] == b"FMREPORT" and len(body) == 20 + a + b + c
            metadata = json.loads(body[20:20+a])
            if len(packets) == 1:
                self.answer(503, {"error": "Review fixture: please retry the same report."})
            else:
                self.answer(201, {"id": metadata["id"], "stored": True})

    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    previous = os.environ.get("FREAK_MELEE_COMMUNITY_ENDPOINT")
    os.environ["FREAK_MELEE_COMMUNITY_ENDPOINT"] = f"http://127.0.0.1:{server.server_port}"
    client = None
    result = {"status": "RUNNING"}
    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output / "client", "Feedback review")
        time.sleep(1)
        client.command("ui-key", "f2")
        wait_visible(client, "feedback-image")
        contexts = list((client.data / "content/feedback").glob("*.context.json"))
        assert len(contexts) == 1
        context = contexts[0].read_bytes()
        captured = json.loads(context)
        assert captured["source_revision"] and captured["workspace"] == "Hub"
        revision = subprocess.check_output(["git", "-C", str(Path(__file__).resolve().parents[2]), "rev-parse", "HEAD"], text=True).strip()
        assert captured["source_revision"].removesuffix("-modified") == revision
        assert captured["hub_page"] >= 0 and isinstance(captured["menu_transition"], bool)
        # Notes, resize and the failed upload must never regenerate context.
        client.command("ui-click", 700, 420)
        for line in range(12):
            client.command("ui-text", f"Line {line}: a long feedback note that wraps across lines. ")
        client.command("ui-text", "LAST LINE VISIBLE")
        wait_visible(client, "feedback-bubble")
        scroll = re.search(r"scroll=[\d.]+,([\d.]+)", client.inspect("feedback-note"))
        assert scroll and float(scroll[1]) > 0, "Long notes must scroll to the caret"
        picture(client, output / "note-bubble.png")
        client.command("ui-key", "home", "ctrl")
        assert "scroll=0.0,0.0" in client.inspect("feedback-note")
        client.command("ui-key", "end", "ctrl")
        client.command("ui-key", "enter")
        time.sleep(.25)
        client.command("ui-click", 950, 500)
        client.command("ui-text", "Temporary note")
        client.click("feedback-remove")
        assert "absent" in client.inspect("feedback-pin-1")
        client.resize(1000, 750)
        time.sleep(.3)
        client.click("feedback-pin-0")
        client.command("ui-key", "a", "ctrl")
        client.command("ui-text", "Edited after resizing; preserve the original frame.")
        client.command("ui-key", "enter")
        time.sleep(.25)
        picture(client, output / "resized-pins.png")
        assert contexts[0].read_bytes() == context
        client.command("ui-key", "enter")
        wait_visible(client, "feedback-dialog")
        picture(client, output / "send-confirmation.png")
        client.click("feedback-send")
        wait_visible(client, "feedback-retry")
        assert contexts[0].read_bytes() == context
        client.click("feedback-retry")
        wait_visible(client, "feedback-close")
        assert len(packets) == 2 and packets[0] == packets[1]
        body = packets[1]
        a, b, c = struct.unpack_from("<III", body, 8)
        metadata = json.loads(body[20:20+a])
        assert body[20+a+b:] == context
        assert metadata["capture_at"] == captured["captured_at"] and metadata["id"] == captured["id"]
        assert metadata["build"] == captured["build"] and metadata["build"]
        assert metadata["context_complete"] is True
        assert len(metadata["pins"]) == 1 and metadata["pins"][0]["text"].startswith("Edited after resizing")
        assert abs(metadata["pins"][0]["x"] - 700/captured["image_width"]) < .002
        assert abs(metadata["pins"][0]["y"] - 420/captured["image_height"]) < .002
        screen = output / "captured.png"
        screen.write_bytes(body[20+a:20+a+b])
        with Image.open(screen) as image:
            assert image.size == (captured["image_width"], captured["image_height"])
        assert not contexts[0].exists(), "Accepted local attachments should be removed"
        client.click("feedback-close")
        assert "visible=0" in client.inspect("feedback-document") or "absent" in client.inspect("feedback-document")
        # Escape closes note editing first and asks before discarding a report.
        client.command("ui-key", "f2")
        wait_visible(client, "feedback-image")
        client.command("ui-click", 600, 350)
        client.command("ui-text", "Keep this until discard is confirmed.")
        client.command("ui-key", "escape")
        assert "visible=1" in client.inspect("feedback-document")
        client.command("ui-key", "escape")
        wait_visible(client, "feedback-keep")
        client.click("feedback-keep")
        assert "visible=1" in client.inspect("feedback-document")
        client.click("feedback-exit")
        wait_visible(client, "feedback-send")
        assert "Discard feedback" in client.inspect("feedback-send")
        client.command("ui-key", "enter")
        assert "visible=0" in client.inspect("feedback-document") or "absent" in client.inspect("feedback-document")
        assert not list((client.data / "content/feedback").glob("*.context.json"))
        result.update(status="PASS", id=metadata["id"], screenshot_bytes=b, context_bytes=c,
                      report_sha256=hashlib.sha256(body).hexdigest(), capture=captured["captured_at"],
                      limits=["Local HTTP fixture; hosted Access/R2 delivery and two-peer neutral input are separate checks."])
    except Exception as error:
        result.update(status="FAIL", error=str(error))
        raise
    finally:
        if client and client.process.poll() is None:
            close_window(client.process.pid)
            client.process.wait(timeout=20)
        server.shutdown()
        if previous is None:
            os.environ.pop("FREAK_MELEE_COMMUNITY_ENDPOINT", None)
        else:
            os.environ["FREAK_MELEE_COMMUNITY_ENDPOINT"] = previous
        (output / "review.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps(result))


if __name__ == "__main__":
    main()
