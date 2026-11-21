"""Forward two local netplay clients through one repeatable UDP fault point.

Configure both games' remote peer port to this proxy's listen port. The game
expects packets to come from that same port, so two one-way proxies cannot be
substituted here.
"""

import argparse
import heapq
import random
import select
import socket
import time


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--listen", type=int, required=True)
    parser.add_argument("--host-port", type=int, required=True)
    parser.add_argument("--guest-port", type=int, required=True)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--drop-percent", type=float, default=0.0)
    parser.add_argument("--delay-ms", type=float, default=0.0)
    parser.add_argument("--jitter-ms", type=float, default=0.0)
    parser.add_argument("--burst-at-frame", type=int, default=120,
                        help="First input sample of the deterministic loss burst")
    parser.add_argument("--burst-frames", type=int, default=0,
                        help="Drop every send for this many successive input samples")
    parser.add_argument("--burst-from", choices=("host", "guest", "both"), default="host")
    args = parser.parse_args()
    if not all(0 < port < 65536 for port in
               (args.listen, args.host_port, args.guest_port)):
        parser.error("UDP ports must be between 1 and 65535")
    if len({args.listen, args.host_port, args.guest_port}) != 3:
        parser.error("listen, host, and guest ports must differ")
    if not (0 <= args.drop_percent <= 100):
        parser.error("drop percentage must be between 0 and 100")
    if args.delay_ms < 0 or args.jitter_ms < 0:
        parser.error("delay and jitter must be nonnegative")
    if args.burst_at_frame < 0 or args.burst_frames < 0:
        parser.error("burst start and length must be nonnegative")

    rng = random.Random(args.seed)
    pending: list[tuple[float, int, bytes, tuple[str, int]]] = []
    received = dropped = forwarded = dropped_in_burst = 0
    next_report = time.monotonic() + 10
    host = ("127.0.0.1", args.host_port)
    guest = ("127.0.0.1", args.guest_port)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", args.listen))
    sock.setblocking(False)
    print(
        f"listening={args.listen} host={args.host_port} guest={args.guest_port} seed={args.seed} "
        f"drop={args.drop_percent}% delay={args.delay_ms}ms jitter={args.jitter_ms}ms "
        f"burst={args.burst_from}:{args.burst_at_frame}+{args.burst_frames}",
        flush=True,
    )
    try:
        while True:
            now = time.monotonic()
            if now >= next_report:
                print(f"received={received} dropped={dropped} "
                      f"forwarded={forwarded} pending={len(pending)} burst_dropped={dropped_in_burst}",
                      flush=True)
                next_report = now + 10
            while pending and pending[0][0] <= now:
                _, _, packet, destination = heapq.heappop(pending)
                sock.sendto(packet, destination)
                forwarded += 1
            timeout = min(0.1, max(0.0, pending[0][0] - now)) if pending else 0.1
            readable, _, _ = select.select([sock], [], [], timeout)
            if not readable:
                continue
            try:
                packet, source = sock.recvfrom(2048)
            except ConnectionResetError:
                # Windows can surface ICMP from an exited client on this socket.
                continue
            if source == host:
                destination = guest
                direction = "host"
            elif source == guest:
                destination = host
                direction = "guest"
            else:
                continue
            received += 1
            if (args.burst_frames and args.burst_from in (direction, "both")
                    and len(packet) >= 48 and packet[:4] in (b"FMIN", b"FMI2")):
                sample = int.from_bytes(packet[40:48], "little")
                if args.burst_at_frame <= sample < args.burst_at_frame + args.burst_frames:
                    dropped += 1
                    dropped_in_burst += 1
                    continue
            if rng.random() * 100 < args.drop_percent:
                dropped += 1
                continue
            delay = (args.delay_ms + rng.uniform(0, args.jitter_ms)) / 1000.0
            heapq.heappush(pending, (time.monotonic() + delay, received,
                                     packet, destination))
    except KeyboardInterrupt:
        pass
    finally:
        print(
            f"received={received} dropped={dropped} forwarded={forwarded} "
            f"pending={len(pending)} burst_dropped={dropped_in_burst}",
            flush=True,
        )
        sock.close()


if __name__ == "__main__":
    main()
