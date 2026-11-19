"""Review announcement paging, updates, withdrawal and article actions in a hidden client."""
from argparse import ArgumentParser
from hashlib import sha256
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlsplit
from uuid import uuid4
import json
import os
import threading
import time

from review_native_community import wait_for
from review_native_css_room import Client


def main():
    parser = ArgumentParser(description=__doc__)
    for name in ("exe", "iso", "profile", "output"):
        parser.add_argument(f"--{name}", type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    now = int(time.time()*1000)
    article = {"id": str(uuid4()), "title": "Announcement article", "ticker": "Read the update",
               "published_at": now, "updated_at": now, "revision": 1,
               "body": "# Latest changes\n\nRead the [Workshop guide](https://example.com/workshop).\n\n- Make it yours\n- Share it"}
    notices = [{"id": str(uuid4()), "delivery_id": str(uuid4()), "title": f"Notice {i}",
                "body": f"Announcement number {i}.", "news_id": article["id"] if i == 64 else None,
                "published_at": now-i, "expires_at": None, "revision": 1} for i in range(130)]
    state = {"enabled": False, "removed": set(), "empty": False, "malformed": False, "pages": []}

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args): pass

        def do_GET(self):
            path = urlsplit(self.path)
            active = [n for n in notices if n["delivery_id"] not in state["removed"] and
                      (not n["expires_at"] or n["expires_at"] > int(time.time()*1000))] if state["enabled"] else []
            def page(before=None):
                rows = active
                if before:
                    stamp, key = before.split(",")
                    rows = [n for n in rows if (n["published_at"], n["id"]) < (int(stamp), key)]
                entries = rows[:64]
                return {"entries": entries, "next": f"{entries[-1]['published_at']},{entries[-1]['id']}" if len(rows)>64 else None}
            if path.path == "/v1/feed":
                values = page()
                result = {"headline": None if state["empty"] else {k:v for k,v in article.items() if k!="body"},
                          "announcements": values["entries"], "announcements_next": values["next"]}
                if state["malformed"]: result = {"headline": "invalid"}
            elif path.path == "/v1/announcements":
                before = parse_qs(path.query)["before"][0]
                state["pages"].append(before)
                result = page(before)
            elif path.path.startswith("/v1/announcements/"):
                result = next((n for n in active if n["delivery_id"] == path.path.rsplit("/",1)[-1]), None)
            elif path.path == "/v1/news/" + article["id"]: result = article
            elif path.path == "/v1/news": result = {"entries": [{k:v for k,v in article.items() if k!="body"}], "next": None}
            else: result = None
            if result is None:
                self.send_error(404); return
            data = json.dumps(result).encode()
            etag = '"' + sha256(data).hexdigest() + '"'
            self.send_response(304 if self.headers.get("If-None-Match") == etag else 200)
            self.send_header("ETag", etag)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            if self.headers.get("If-None-Match") != etag: self.wfile.write(data)

    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
    os.environ.update(FREAK_MELEE_COMMUNITY_ENDPOINT=f"http://127.0.0.1:{server.server_port}",
                      SDL_AUDIODRIVER="dummy", FREAK_MELEE_MENU_MOTION="0")
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), args.output/"client",
                    "Announcement review", window_size=(1200, 800))
    checks, shots = [], []
    try:
        client.click("nav-5")
        wait_for(client, "hub-news", "headline=Announcement article")
        seen = client.data/"content/seen-announcements.txt"
        seen.write_text("".join(n["delivery_id"]+"\n" for i,n in enumerate(notices) if i not in (64,129)))
        sent_at = int(time.time()*1000)
        for notice, key in zip(notices, sorted((n["id"] for n in notices), reverse=True)):
            notice.update(id=key, published_at=sent_at)
        state["enabled"] = True
        wait_for(client, "hub-notice", "title=Notice 64")
        assert state["pages"], "No older announcement page requested"
        shots.append(str(client.screenshot()))
        checks.append("unseen notice beyond newest 64")
        wait_for(client, "hub-notice", "ready=1")
        client.command("ui-key", "x")
        wait_for(client, "hub-news", "article="+article["id"])
        # A second pending notice may temporarily cover the article.
        wait_for(client, "hub-notice", "title=Notice 129")
        assert len(set(state["pages"])) == 2
        notices[129].update(title="Edited while visible", body="The updated body is visible too.", revision=2)
        wait_for(client, "hub-notice", "title=Edited while visible", timeout=45)
        assert "The updated body" in client.inspect("hub-notice")
        checks += ["article action", "third announcement page", "edit active notice"]
        state["removed"].add(notices[129]["delivery_id"])
        wait_for(client, "hub-notice", "count=0", timeout=45)
        assert notices[129]["delivery_id"] not in seen.read_text()
        assert notices[64]["delivery_id"] in seen.read_text()
        checks.append("withdraw without marking seen")
        wait_for(client, "hub-news", "busy=0")
        client.element("news-link-0-2")
        shots.append(str(client.screenshot()))
        checks.append("formatted article with clickable web link")
        # A new delivery appears once; expiry removes an already-visible notice.
        expiring = {**notices[0], "delivery_id": str(uuid4()), "title": "Expires soon",
                    "published_at": int(time.time()*1000), "expires_at": int(time.time()*1000)+45000}
        notices.insert(0, expiring)
        wait_for(client, "hub-notice", "title=Expires soon", timeout=40)
        wait_for(client, "hub-notice", "count=0", timeout=50)
        assert expiring["delivery_id"] not in seen.read_text()
        checks.append("expiry without marking seen")
        state["malformed"] = True
        wait_for(client, "hub-news", "offline=1", timeout=40)
        assert "headline=Announcement article" in client.inspect("hub-news")
        checks.append("malformed feed preserves last valid headline")
        state.update(malformed=False, empty=True)
        wait_for(client, "hub-news", "offline=0", timeout=40)
        assert "headline= notices=" in client.inspect("hub-news")
        checks.append("valid empty headline clears ticker")
    finally:
        client.stop(); server.shutdown(); server.server_close(); thread.join(timeout=5)
        (args.output/"review.json").write_text(json.dumps({"checks": checks, "screenshots": shots,
                                                         "older_page_requests": state["pages"]}, indent=2))
    print("PASS native announcements, linked article and feed validity review")


if __name__ == "__main__": main()
