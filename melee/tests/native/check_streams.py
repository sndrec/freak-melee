"""Decode all music metadata reachable from the original disc's HPS files."""
import pathlib, struct, subprocess, sys, tempfile
with tempfile.TemporaryDirectory(prefix="melee-stream-") as directory:
    fixture = pathlib.Path(directory) / "headers.bin"
    with open(sys.argv[1], "rb") as source, fixture.open("wb") as out:
        source.seek(0x424)
        offset, size = struct.unpack(">II", source.read(8))
        source.seek(offset)
        fst = source.read(size)
        count = struct.unpack_from(">I", fst, 8)[0]
        names = fst[count * 12:]
        for i in range(1, count):
            name, offset, size = struct.unpack_from(">III", fst, i * 12)
            if name >> 24: continue
            start = name & 0xFFFFFF
            if not names[start:names.index(0, start)].endswith(b".hps"): continue
            source.seek(offset)
            header = source.read(128)
            chunks, seen, cursor = [], set(), 128
            while cursor != 0xFFFFFFFF and cursor not in seen:
                assert cursor >= 128 and cursor + 32 <= size
                seen.add(cursor)
                source.seek(offset + cursor)
                chunk = source.read(32)
                length, end, next_offset = struct.unpack_from(">III", chunk)
                assert cursor + 32 + length <= size
                chunks.append(chunk)
                cursor = next_offset
            out.write(header)
            out.write(struct.pack(">I", len(chunks)))
            out.write(b"".join(chunks))
    subprocess.run([sys.argv[2], str(fixture)], check=True)
