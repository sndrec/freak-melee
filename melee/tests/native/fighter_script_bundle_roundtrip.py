"""Bind Mario and Ganondorf drafts into independent custom fighter slots."""

import pathlib
import re
import struct
import subprocess
import sys
import tempfile
import zlib
import ctypes
import hashlib


def run(tool, *args, succeeds=True):
    result = subprocess.run([tool, *map(str, args)], capture_output=True,
                            text=True, check=False,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    if (result.returncode == 0) != succeeds:
        raise AssertionError(
            f"unexpected exit {result.returncode}: {args}\n"
            f"{result.stdout}\n{result.stderr}"
        )
    return result.stdout


def portrait(path):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + \
               struct.pack(">I", zlib.crc32(kind + data))

    rows = b"".join(b"\x00" + b"\x80\x20\x20\xff" * 16
                    for _ in range(16))
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n" +
        chunk(b"IHDR", struct.pack(">IIBBBBB", 16, 16, 8, 6, 0, 0, 0)) +
        chunk(b"IDAT", zlib.compress(rows)) +
        chunk(b"IEND", b"")
    )


def bundled_id(output):
    found = re.search(r"not installed\): ([0-9a-f]{64})", output)
    if found is None:
        raise AssertionError(output)
    return found.group(1)


def digest(path):
    return hashlib.sha256(path.read_bytes()).digest()


def main():
    tool, iso, fixture, ganon_script = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="freak-script-bundle-") as temp:
        root = pathlib.Path(temp)
        project = root / "Mario Script"
        plain = root / "Mario Plain"
        store = root / "store"
        image = root / "portrait.png"
        portrait(image)

        run(tool, "clone-stock", iso, "Mario", project, "Mario Script")
        run(tool, "script-build", project, fixture)
        first = bundled_id(run(tool, "--root", store, "bundle-project",
                               project, image, iso))
        run(tool, "--root", store, "verify-package", first, iso)

        saved = project / "behavior.fms"
        module = project / "module.dll"
        working_script = digest(saved)
        working_module = digest(module)
        candidate = root / "candidate.fms"
        candidate.write_bytes(saved.read_bytes() + b"\n# next valid build\n")
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.CreateFileW.argtypes = (ctypes.c_wchar_p, ctypes.c_uint32,
                                          ctypes.c_uint32, ctypes.c_void_p,
                                          ctypes.c_uint32, ctypes.c_uint32,
                                          ctypes.c_void_p)
        kernel32.CreateFileW.restype = ctypes.c_void_p
        kernel32.CloseHandle.argtypes = (ctypes.c_void_p,)
        kernel32.CloseHandle.restype = ctypes.c_int
        # Prevent replacement of the saved script after the new DLL is built.
        # The previous working script and module must remain a matching pair.
        handle = kernel32.CreateFileW(str(saved), 0x80000000, 1, None,
                                      3, 0x80, None)
        if handle == ctypes.c_void_p(-1).value:
            raise OSError(ctypes.get_last_error(), "cannot lock script")
        try:
            run(tool, "script-build", project, candidate, succeeds=False)
        finally:
            if not kernel32.CloseHandle(handle):
                raise OSError(ctypes.get_last_error(), "cannot unlock script")
        assert digest(saved) == working_script
        assert digest(module) == working_module
        assert not list(project.glob("module.dll.backup-*"))
        assert bundled_id(run(tool, "--root", store, "bundle-project",
                              project, image, iso)) == first

        saved.write_bytes(saved.read_bytes() + b"\n# deliberately stale\n")
        run(tool, "--root", store, "package-project", project, image,
            succeeds=False)
        run(tool, "--root", store, "bundle-project", project, image, iso,
            succeeds=False)

        run(tool, "script-build", project, saved)
        second = bundled_id(run(tool, "--root", store, "bundle-project",
                                project, image, iso))
        assert second != first
        run(tool, "--root", store, "verify-package", second, iso)

        run(tool, "clone-stock", iso, "Mario", plain, "Mario Plain")
        bundled_id(run(tool, "--root", store, "bundle-project",
                       plain, image, iso))

        ganon = root / "Ganondorf Script"
        run(tool, "clone-stock", iso, "Ganon", ganon, "Ganondorf Script")
        owned_ganon = root / "ganon_owned.fms"
        run(tool, "import-parameters", ganon, ganon_script, owned_ganon)
        run(tool, "script-build", ganon, owned_ganon)
        ganon_id = bundled_id(run(tool, "--root", store, "bundle-project",
                                  ganon, image, iso))
        run(tool, "--root", store, "verify-package", ganon_id, iso)

    print("Mario and Ganondorf bundles bound twice; stale source rejected")


if __name__ == "__main__":
    main()
