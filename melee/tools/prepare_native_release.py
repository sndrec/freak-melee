"""Package one complete native build and a standalone friend installation."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import zipfile
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--release", required=True)
    parser.add_argument("--channel", help="HTTPS URL of the hosted release manifest")
    args = parser.parse_args()

    release = args.release
    if not release or len(release) > 64 or any(
        char not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-"
        for char in release
    ) or release in (".", ".."):
        parser.error("release must be a safe, nonempty folder name (up to 64 characters)")
    if args.channel and not args.channel.startswith("https://"):
        parser.error("channel must be an HTTPS manifest URL")

    build = args.build.resolve(strict=True)
    payload = [build / "melee_native.exe", *sorted(build.glob("*.dll"))]
    payload += sorted(path for path in (build / "ui").rglob("*") if path.is_file())
    if not (build / "melee_native.exe").is_file() or not (build / "ui/freak_shell.rml").is_file():
        parser.error("build must contain melee_native.exe and ui/freak_shell.rml")
    if not list(build.glob("*.dll")):
        parser.error("build has no runtime DLLs")
    scripts = Path(__file__).resolve().parent
    payload = [(source, source.relative_to(build).as_posix()) for source in payload]
    payload.append((scripts / "freak_launcher.ps1", "freak_launcher.ps1"))

    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    archive_name = f"freak-melee-{release}.zip"
    archive_path = output / archive_name
    setup_path = output / f"freak-melee-setup-{release}.zip"
    install = output / "install"
    if archive_path.exists() or setup_path.exists() or \
            (output / "latest.json").exists() or install.exists():
        raise SystemExit(f"Refusing to overwrite an existing release in {output}")
    files = []
    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=3) as archive:
        for source, name in payload:
            files.append({"path": name, "size": source.stat().st_size,
                          "sha256": sha256(source)})
            archive.write(source, name)

    manifest = {"release": release, "archive": archive_name,
                "size": archive_path.stat().st_size, "sha256": sha256(archive_path),
                "files": files}
    (output / "latest.json").write_text(json.dumps(manifest, indent=2) + "\n",
                                         encoding="utf-8")

    version = install / "versions" / release
    version.mkdir(parents=True)
    for source, name in payload:
        destination = version / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    (install / "state.json").write_text(
        json.dumps({"current": release, "previous": None, "blocked": None}, indent=2) + "\n",
        encoding="utf-8",
    )
    shutil.copy2(scripts / "freak_launcher.ps1", install / "freak_launcher.ps1")
    shutil.copy2(scripts / "Freak Melee.bat", install / "Freak Melee.bat")
    (install / "channel.txt").write_text((args.channel or "") + "\n", encoding="utf-8")
    with zipfile.ZipFile(setup_path, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=3) as archive:
        for source in sorted(path for path in install.rglob("*") if path.is_file()):
            archive.write(source, source.relative_to(install).as_posix())
    print(f"Release: {archive_path}\nManifest: {output / 'latest.json'}"
          f"\nFriend installation: {install}\nFriend download: {setup_path}")
    print("The ISO is intentionally absent."
          + (" Updates use the configured channel." if args.channel else
             " Set a channel URL to enable updates."))


if __name__ == "__main__":
    main()
