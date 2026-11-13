"""Check feedback neutral input in two hidden, muted loopback peers using an installed scripted fighter."""
from argparse import ArgumentParser
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import secrets
import socket
import time

from review_native_css_room import Client


def state(client):
    text = client.inspect("netplay-state")
    frame = int(re.search(r"\[netplay-state\] frame=(\d+)", text)[1])
    players = {int(p): (float(x), float(y)) for p, x, y in re.findall(
        r"\[netplay-player\] port=(\d+).*? x=([\d.-]+) y=([\d.-]+)", text)}
    assert "failed=0" in text, text
    return frame, players


def main():
    parser = ArgumentParser(description=__doc__)
    for name in ("exe", "iso", "profile", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=False)
    profile = args.profile.resolve()
    fighter = next((profile / "Freak Melee/content/installed").glob("*.fmi")).stem
    ports = []
    for _ in range(2):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
            probe.bind(("127.0.0.1", 0))
            ports.append(probe.getsockname()[1])
    common = ["--match", "--stage", "battlefield", "--p1", "human:custom", "--p1-fighter", fighter,
              "--p2", "human:custom", "--p2-fighter", fighter, "--p3", "off", "--p4", "off",
              "--netplay-session", str(secrets.randbits(63)), "--netplay-input-delay", "2", "--netplay-delay", "6"]
    clients = []

    def start(i):
        client = Client(args.exe.resolve(), args.iso.resolve(), profile, root / str(i), f"Feedback peer {i}",
            launch_args=(*common, "--netplay-local-peer", str(i), "--netplay-local-mask", str(5 if i == 0 else 10),
                "--netplay-listen", str(ports[i]), "--netplay-peer", f"{1-i},127.0.0.1,{ports[1-i]},{10 if i == 0 else 5}"))
        clients.append(client)
        return client

    result = {"status": "RUNNING"}
    try:
        with ThreadPoolExecutor(max_workers=2) as pool:
            a, b = list(pool.map(start, range(2)))
        time.sleep(3)
        before = state(b)
        a.command("ui-key", "f2")
        time.sleep(.3)
        context = next((a.data / "content/feedback").glob("*.context.json"))
        data = json.loads(context.read_bytes())
        assert data["online"] and any("script" in fighter for fighter in data["fighters"])
        (root / "capture.json").write_bytes(context.read_bytes())
        first = state(b)
        a.command("hold", 0, "sx=80")
        # Direct UDP assigns physical ports by global slot. Lobby sessions
        # have their own local-seat mapping; this fixture controls P2 on port 1.
        b.command("hold", 1, "sx=40")
        time.sleep(.65)
        during = state(b)
        assert during[0] - first[0] >= 20, (first, during)
        assert abs(during[1][0][0] - first[1][0][0]) < .1, (first, during)
        assert during[1][1][0] > first[1][1][0] + 1, (first, during)
        b.command("hold", 1, "sx=0")
        a.command("ui-key", "escape")
        time.sleep(.4)
        after = state(b)
        assert after[1][0][0] > during[1][0][0] + 1, (during, after)
        traces = [client.inspect("netplay-state") for client in (a, b)]
        assert any(int(re.search(r"restores=(\d+)", text)[1]) > 0 for text in traces), traces
        result.update(status="PASS", before=before, during_start=first, during_end=during, after=after,
                      limits=["Loopback UDP with six-frame artificial outbound delay; remote Internet play is separate."])
    except Exception as error:
        result.update(status="FAIL", error=str(error))
        raise
    finally:
        for client in clients:
            client.stop()
        (root / "review.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps(result))


if __name__ == "__main__":
    main()
