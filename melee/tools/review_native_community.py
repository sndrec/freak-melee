"""Review native news against a controlled HTTP feed, in a hidden muted client."""
from argparse import ArgumentParser
from hashlib import sha256
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlsplit
from uuid import uuid4
import json
import os
import re
import threading
import time

from review_native_css_room import Client
from review_native_release import close_window


def wait_for(client, element, expected, timeout=40):
    deadline = time.monotonic() + timeout
    latest = ""
    while time.monotonic() < deadline:
        latest = client.inspect(element)
        if expected in latest:
            return latest
        time.sleep(.25)
    raise AssertionError(f"Missing {expected}: {latest}")


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    rows = [{"id": str(uuid4()), "title": f"Workshop update {27-i}",
             "ticker": "A long news headline keeps moving through the menu header, and pauses beneath the Melee hand.",
             "published_at": int(time.time()*1000)-i*1000, "updated_at": int(time.time()*1000), "revision": 1}
            for i in range(27)]
    state = {"headline": rows[0], "announcements": [], "conditional": 0}
    article_body = "# Make it yours\n\nRead the latest Workshop changes here.\n\n- Edit your fighters\n- Share them with friends\n\n" + "\n\n".join(
        f"Update {i}: Your fighter owns its moves, animations, scripts and model. Keep making things!" for i in range(20))

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def do_GET(self):
            path = urlsplit(self.path)
            if path.path == "/v1/feed":
                result = {"headline": state["headline"], "announcements": state["announcements"], "announcements_next": None}
            elif path.path == "/v1/news":
                older = bool(parse_qs(path.query).get("before"))
                result = {"entries": rows[25:] if older else rows[:25],
                          "next": None if older else f"{rows[24]['published_at']},{rows[24]['id']}"}
            elif path.path.startswith("/v1/news/"):
                row = next((r for r in rows if r["id"] == path.path.rsplit("/", 1)[-1]), None)
                if row is None:
                    self.send_error(404)
                    return
                result = {**row, "body": article_body}
            elif path.path.startswith("/v1/announcements/"):
                row = next((r for r in state["announcements"] if r["delivery_id"] == path.path.rsplit("/", 1)[-1]), None)
                if row is None:
                    self.send_error(404)
                    return
                result = row
            else:
                self.send_error(404)
                return
            data = json.dumps(result).encode()
            etag = '"' + sha256(data).hexdigest() + '"'
            if self.headers.get("If-None-Match") == etag:
                state["conditional"] += 1
                self.send_response(304)
                self.send_header("ETag", etag)
                self.end_headers()
                return
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("ETag", etag)
            self.end_headers()
            self.wfile.write(data)

    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    os.environ["FREAK_MELEE_COMMUNITY_ENDPOINT"] = f"http://127.0.0.1:{server.server_port}"
    os.environ["SDL_AUDIODRIVER"] = "dummy"
    os.environ["FREAK_MELEE_MENU_MOTION"] = "0"
    shots = []
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                    args.output/"online", "News review", window_size=(1440, 900))
    try:
        client.click("nav-5")
        wait_for(client, "hub-news", "headline=Workshop update 27")
        shots.append(str(client.screenshot()))
        client.click("news-ticker")
        wait_for(client, "hub-news", f"article={rows[0]['id']}")
        wait_for(client, "hub-news", "busy=0")
        client.click("news-down")
        assert "scroll=5" in client.inspect("hub-news")
        client.tap("down")
        assert "scroll=8" in client.inspect("hub-news")
        client.tap("cy=80")
        assert "scroll=5" in client.inspect("hub-news")
        shots.append(str(client.screenshot()))
        client.resize(960, 540)
        shots.append(str(client.screenshot()))
        client.resize(1440, 900)
        client.click("news-history")
        wait_for(client, "hub-news", "page-count=25")
        shots.append(str(client.screenshot()))
        client.click("news-down")
        assert "scroll=5" in client.inspect("hub-news")
        client.click("news-older")
        wait_for(client, "hub-news", "page-count=2")
        client.click("news-latest")
        wait_for(client, "hub-news", "page-count=25")
        client.click("news-back")
        client.element("edit-fighter")
        # The ordinary poll must publish a changed headline and new notice
        # without replacing the Workshop page or requiring a client restart.
        state["headline"] = {**rows[0], "title": "Updated while running", "ticker": "Fresh news", "revision": 2}
        delivery = str(uuid4())
        state["announcements"] = [{"id": str(uuid4()), "delivery_id": delivery, "title": "News review notice",
                                   "body": "This notice arrived while the client was open.", "news_id": rows[0]["id"],
                                   "published_at": int(time.time()*1000), "expires_at": None, "revision": 1}]
        wait_for(client, "hub-news", "headline=Updated while running")
        wait_for(client, "hub-notice", "title=News review notice")
        shots.append(str(client.screenshot()))
        wait_for(client, "hub-notice", "ready=1")
        client.command("ui-key", "return")
        wait_for(client, "hub-notice", "count=0")
        time.sleep(.5)
        assert delivery in (client.data/"content/seen-announcements.txt").read_text()
        wait_for(client, "hub-news", "feed=3", timeout=40)
        assert "count=0" in client.inspect("hub-notice")
        assert state["conditional"] > 0
    finally:
        close_window(client.process.pid)
        client.process.wait(timeout=20)
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)
    # Reuse this installation with the service now unavailable. Its last valid
    # headline and dismissed-delivery record must survive the restart.
    offline = Client(args.exe.resolve(), args.iso.resolve(), client.folder/"appdata",
                     args.output/"offline", "News review", window_size=(1100, 760))
    try:
        offline.click("nav-5")
        wait_for(offline, "hub-news", "offline=1")
        assert "headline=Updated while running" in offline.inspect("hub-news")
        assert "count=0" in offline.inspect("hub-notice")
        shots.append(str(offline.screenshot()))
    finally:
        close_window(offline.process.pid)
        offline.process.wait(timeout=20)
    (args.output/"review.json").write_text(json.dumps({"screenshots": shots, "conditional_reads": state["conditional"],
        "checks": ["standalone reader at two window sizes", "return to originating page", "ticker", "article scroll", "history pagination", "live update", "notice dismissal", "seen persistence", "offline cache"]}, indent=2))
    print("PASS native ticker, reader, live update, notice and offline-cache review")


if __name__ == "__main__":
    main()
