"""Exercise the production bank decoder with every original ISO sound-bank header."""
import pathlib
import struct
import subprocess
import sys
import tempfile

iso, executable = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix="melee-sfx-") as directory:
    fixture = pathlib.Path(directory) / "headers.bin"
    with open(iso, "rb") as source, fixture.open("wb") as out:
        source.seek(0x424)
        offset, size = struct.unpack(">II", source.read(8))
        source.seek(offset)
        fst = source.read(size)
        count = struct.unpack_from(">I", fst, 8)[0]
        names = fst[count * 12:]
        for i in range(1, count):
            name, offset, size = struct.unpack_from(">III", fst, i * 12)
            if name >> 24:
                continue
            start = name & 0xFFFFFF
            name = names[start:names.index(0, start)]
            if not name.endswith(b".ssm"):
                continue
            source.seek(offset)
            header = source.read(16)
            header_size = struct.unpack_from(">I", header)[0]
            assert header_size + 16 <= size
            out.write(header)
            out.write(source.read(header_size))
    subprocess.run([executable, str(fixture)], check=True)
