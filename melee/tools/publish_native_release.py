"""Upload a prepared release to R2, publishing latest.json only after verification."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import tempfile
import zipfile
from pathlib import Path


BUCKET = "freak-melee-releases"
CHANNEL = "https://freak-melee-releases.sndrec32exe.workers.dev/3ShgcrpBLwN2237YIwwm6kX0/latest.json"
ROOT = Path(__file__).resolve().parents[1]
WORKER = ROOT / "services" / "releases"
WRANGLER = WORKER / "node_modules" / ".bin" / "wrangler.cmd"
RELEASE_ID = re.compile(r"[a-zA-Z0-9._-]{1,64}\Z")


def digest(path: Path) -> str:
    result = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def wrangler(*args: str) -> None:
    subprocess.run([str(WRANGLER), *args], cwd=WORKER, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("release_dir", type=Path,
                        help="Directory produced by prepare_native_release.py")
    parser.add_argument("--dry-run", action="store_true",
                        help="Validate the prepared release without uploading")
    args = parser.parse_args()

    release_dir = args.release_dir.resolve(strict=True)
    manifest_path = release_dir / "latest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    release = manifest.get("release")
    if not isinstance(release, str) or not RELEASE_ID.fullmatch(release) or \
            release in (".", ".."):
        parser.error("Invalid release ID")
    archive_name = f"freak-melee-{release}.zip"
    if manifest.get("archive") != archive_name:
        parser.error("Archive name does not match release ID")
    archive = release_dir / archive_name
    setup_name = f"freak-melee-setup-{release}.zip"
    setup = release_dir / setup_name
    if archive.stat().st_size != manifest.get("size") or \
            digest(archive) != manifest.get("sha256"):
        parser.error("Prepared ZIP does not match latest.json")
    with zipfile.ZipFile(setup) as installer:
        required = {"Freak Melee.bat", "freak_launcher.ps1", "channel.txt",
                    "state.json", f"versions/{release}/melee_native.exe",
                    f"versions/{release}/ui/freak_shell.rml"}
        if not required.issubset(installer.namelist()) or \
                installer.read("channel.txt").decode("utf-8").strip() != CHANNEL or \
                any(name.lower().endswith(".iso") for name in installer.namelist()):
            parser.error("Friend installer is incomplete, has the wrong channel, or includes an ISO")
    entries = manifest.get("files")
    if not isinstance(entries, list) or not entries:
        parser.error("Prepared manifest has no files")
    with zipfile.ZipFile(archive) as package:
        names = package.namelist()
        if len(names) != len(set(names)) or len(names) != len(entries):
            parser.error("Prepared ZIP has missing or duplicate files")
        if {entry.get("path") for entry in entries if isinstance(entry, dict)} != set(names):
            parser.error("Prepared manifest does not list the ZIP exactly")
        for entry in entries:
            if not isinstance(entry, dict) or entry.get("path") not in names or \
                    not isinstance(entry.get("size"), int) or \
                    not isinstance(entry.get("sha256"), str):
                parser.error("Prepared manifest has an invalid file entry")
            name = entry["path"]
            if name.startswith("/") or "\\" in name or ":" in name or \
                    any(part in (".", "..", "") for part in name.split("/")):
                parser.error("Prepared manifest has an invalid file path")
            member = package.getinfo(name)
            if member.file_size != entry["size"]:
                parser.error(f"Prepared file size differs: {name}")
            file_hash = hashlib.sha256()
            with package.open(member) as source:
                for block in iter(lambda: source.read(1024 * 1024), b""):
                    file_hash.update(block)
            if file_hash.hexdigest() != entry["sha256"]:
                parser.error(f"Prepared file hash differs: {name}")
    if args.dry_run:
        print(f"Validated {archive_name}; no upload performed.")
        return
    if not WRANGLER.is_file():
        parser.error("Install services/releases dependencies with npm install first")

    with tempfile.TemporaryDirectory(prefix="freak-release-") as directory:
        for filename, source, expected in (
            (archive_name, archive, manifest["sha256"]),
            (setup_name, setup, digest(setup)),
        ):
            object_path = f"{BUCKET}/{filename}"
            wrangler("r2", "object", "put", object_path, "--file", str(source),
                     "--content-type", "application/zip", "--remote")
            verification = Path(directory) / filename
            wrangler("r2", "object", "get", object_path, "--file",
                     str(verification), "--remote")
            if digest(verification) != expected:
                raise SystemExit(f"Remote {filename} verification failed; latest.json was not changed")
    wrangler("r2", "object", "put", f"{BUCKET}/latest.json", "--file",
             str(manifest_path), "--content-type", "application/json",
             "--cache-control", "no-store", "--remote")
    print(f"Published {release} after verifying the remote ZIP.")


if __name__ == "__main__":
    main()
