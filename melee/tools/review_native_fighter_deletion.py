"""Exercise confirmed deletion, recovery, draft-only and paired fighters.

The source profile should contain one installed, editable single fighter.
Only copies under the new output directory are changed.
"""
from argparse import ArgumentParser
from pathlib import Path
import ctypes
import os
import re
import shutil
import time

from review_native_css_room import Client


def choose(client, region, value):
    pattern = rf"\[hub-inspect\] {region} origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+) enabled=1 value=(\d+)"
    text = client.inspect("hub-regions")
    for x, y, width, height, index in re.findall(pattern, text):
        if int(index) == value:
            client.command("ui-click", round(float(x)+float(width)/2), round(float(y)+float(height)/2))
            time.sleep(.3)
            return
    raise AssertionError(text)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    os.environ["SDL_AUDIODRIVER"] = "dummy"
    os.environ["FREAK_MELEE_MENU_MOTION"] = "0"
    os.environ["FREAK_MELEE_COMMUNITY_ENDPOINT"] = "http://127.0.0.1:1"
    exe, iso, profile = args.exe.resolve(), args.iso.resolve(), args.profile.resolve()
    originals = list((profile/"Freak Melee/projects").iterdir())
    assert len(originals) == 1 and originals[0].is_dir(), "Use a one-fighter review profile"
    original = originals[0].name
    client = Client(exe, iso, profile, args.output/"locked", "Deletion review")
    try:
        client.click("nav-5")
        client.click("copy-fighter")
        projects = client.data/"projects"
        generated = [path for path in projects.iterdir() if path.name != original]
        assert len(generated) == 1
        draft = generated[0].name
        choose(client, "fighter", 1)
        client.click("delete-fighter")
        client.screenshot()
        client.click("delete-cancel")
        assert (projects/draft).is_dir()
        client.click("delete-fighter")
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.CreateFileW.restype = ctypes.c_void_p
        kernel.CloseHandle.argtypes = [ctypes.c_void_p]
        handle = kernel.CreateFileW(str(projects/draft/"behavior.fms"), 0x80000000, 0, None, 3, 0, None)
        assert handle not in (None, ctypes.c_void_p(-1).value)
        try:
            client.click("delete-confirm")
            assert (client.data/"content/delete-fighter.json").exists()
            assert (projects/original).is_dir()
        finally:
            kernel.CloseHandle(handle)
    finally:
        client.stop()
    recovered = Client(exe, iso, client.folder/"appdata", args.output/"recovered", "Deletion review")
    try:
        recovered.click("nav-5")
        assert not (recovered.data/"projects"/draft).exists()
        assert not (recovered.data/"projects/.delete-pending").exists()
        assert not (recovered.data/"content/delete-fighter.json").exists()
        assert (recovered.data/"projects"/original).exists()
        installed = list((recovered.data/"content/installed").glob("*.fmi"))
        assert len(installed) == 1
        package = installed[0].stem
        recovered.click("delete-fighter")
        recovered.click("delete-confirm")
        assert not (recovered.data/"projects"/original).exists()
        assert not (recovered.data/"content/installed"/f"{package}.fmi").exists()
        assert (recovered.data/"content/cache"/f"{package}.fmc").exists()
        recovered.click("first-clone")
        choose(recovered, "template", 19)  # Zelda / Sheik family
        recovered.click("create-clone")
        recovered.element("delete-fighter")
        paired = list((recovered.data/"projects").iterdir())
        assert len(paired) == 1 and (paired[0]/"family.txt").is_file()
        assert (paired[0]/"partner").is_dir()
        family_path = paired[0]
        recovered.click("delete-fighter")
        recovered.click("delete-confirm")
        assert not family_path.exists()
        assert not list((recovered.data/"content/installed").glob("*.fmi"))
        assert "first-clone" in recovered.inspect("hub-regions")
        recovered.screenshot()
    finally:
        recovered.stop()
    draft_seed = args.output/"draft-seed"
    shutil.copytree(profile, draft_seed)
    copy = draft_seed/"Freak Melee/projects/draft-only"
    shutil.copytree(draft_seed/"Freak Melee/projects"/original, copy)
    (copy/"installed.id").unlink()
    draft_client = Client(exe, iso, draft_seed, args.output/"draft-only", "Deletion review")
    try:
        draft_client.click("nav-5")
        choose(draft_client, "fighter", 1)
        before = sorted(path.name for path in (draft_client.data/"content/installed").glob("*.fmi"))
        draft_client.click("delete-fighter")
        draft_client.click("delete-confirm")
        assert not (draft_client.data/"projects/draft-only").exists()
        assert (draft_client.data/"projects"/original).exists()
        assert sorted(path.name for path in (draft_client.data/"content/installed").glob("*.fmi")) == before
    finally:
        draft_client.stop()
    print("PASS cancellation, locked-file recovery, installed, draft-only and paired-family deletion; unrelated fighters and cache retained")


if __name__ == "__main__":
    main()
