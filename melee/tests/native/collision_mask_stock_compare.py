"""Extract actual mask dataflow bodies; original bool means MSL int, not _Bool.
No game/build is launched. CMake may compile collision_mask_stock_test.c with
--output as its include directory. --iso optionally verifies exact retail code.
"""
from pathlib import Path
import argparse, hashlib, json, re, struct, subprocess

ORIGINAL_REF = "500911adf1817c06fdbf887da787eca7be3ff60a"
SHARED = ["it_8026DFB0", "it_8026E058", "it_8026E_inline",
          "it_8026E15C_inline1", "it_8026E15C_inline2", "it_8026E15C",
          "it_8026E248_inline", "it_8026DE98_outline", "it_8026DE98_outline2",
          "it_8026DE98_outline3", "it_8026DE98_outline4", "it_8026E248",
          "it_8026E32C_inline", "it_8026E32C", "it_8026E414", "it_8026E4D0",
          "it_8026E5A0", "it_8026E664", "it_8026E71C", "it_8026E7E0"]
TURNIP = ["itPeachturnip_UnkMotion3_Coll"]
RETAIL = {'itPeachturnip_UnkMotion3_Coll': {'address': 2150357092, 'size': 104, 'sha256': 'dbe11093b12271e77e67d28b163fbf2da1c30e3074478f0b32213a2fb334d1da'}, 'it_8026DFB0': {'address': 2150031280, 'size': 168, 'sha256': '272a4f44c81c6ad4acf674bc384bbc696125f5072cb2cc7d7bd26e2044cf2bb4'}, 'it_8026E058': {'address': 2150031448, 'size': 156, 'sha256': 'cfc3783a33a9dbd029977d13c13dce47192fc32b0c791a225eb7bfa09f0b6b78'}, 'it_8026E15C': {'address': 2150031708, 'size': 236, 'sha256': '081d1031472fa3d9bf2a4e0cadd5855b174dadd3673a0e73d0e4cb57950cb0f1'}, 'it_8026E248': {'address': 2150031944, 'size': 228, 'sha256': '884bc4c447b9e60be7b02031b95c4e4e8433cefa24f95cd85d73dbd4301c8c89'}, 'it_8026E32C': {'address': 2150032172, 'size': 232, 'sha256': '81bc9efc6b13c1b833c6e92dbf28aed51954f60a752bdfad514e15d774e80427'}, 'it_8026E414': {'address': 2150032404, 'size': 188, 'sha256': '5ef478b701a95fd327db6aa1188a4d44376bd32d18b2814ba5482a1c98c1dcb8'}, 'it_8026E4D0': {'address': 2150032592, 'size': 208, 'sha256': 'e6246473040589403d9e3f55f34a28397b3e4e157dabf11e18932711f84f8181'}, 'it_8026E5A0': {'address': 2150032800, 'size': 196, 'sha256': '744c12abce78c99aaa15396c58f58273fed200ef74b110cc1ac95d24ba08dcf6'}, 'it_8026E664': {'address': 2150032996, 'size': 184, 'sha256': '6c2d9ee439ee1737c1fac0dfcbadded39a6524eaa85c253b4df5c7e6131dfb37'}, 'it_8026E71C': {'address': 2150033180, 'size': 196, 'sha256': 'b4c5aaaec1fe424c4fe292993e8dd079482623a0c00d0e015447589e96420a39'}, 'it_8026E7E0': {'address': 2150033376, 'size': 228, 'sha256': 'e6f4fde998e05f56102a64022ff62e93b7b55c871f945d9a83a8ecf7618d637e'}}

def body(source, name):
    match = re.search(r"(?m)^(?:static\s+)?(?:inline\s+)?(?:bool|void|s32)\s+" + name +
                      r"\([^;{}]*\)\s*\{", source)
    if not match:
        raise ValueError("missing production body " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--iso", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    all_names = SHARED + TURNIP
    old, current, receipts = [], [], {}
    for path, names in [("src/melee/it/itgroundcoll.c", SHARED),
                        ("src/melee/it/kinds/itpeachturnip.c", TURNIP)]:
        source = (args.root/path).read_text(encoding="utf-8")
        original = subprocess.check_output(["git", "show", ORIGINAL_REF+":"+path],
                                           cwd=args.root, text=True)
        receipts[path] = {"original_sha256": hashlib.sha256(original.encode()).hexdigest(),
                          "candidate_sha256": hashlib.sha256(source.encode()).hexdigest()}
        for name in names:
            baseline = body(original, name)
            # MSL/stdbool.h uses typedef int bool. Replace only the type token
            # and function identities; statements/order/math remain actual C.
            baseline = re.sub(r"\bbool\b", "int", baseline)
            for function in all_names:
                baseline = re.sub(r"\b"+function+r"\b", "original_"+function, baseline)
            old.append(baseline)
            current.append(body(source, name))
    (args.output/"collision_mask_original.inc").write_text("\n\n".join(old)+"\n")
    (args.output/"collision_mask_candidate.inc").write_text("\n\n".join(current)+"\n")
    retail_verified = False
    if args.iso:
        with args.iso.open("rb") as stream:
            stream.seek(0x420)
            dol = struct.unpack(">I", stream.read(4))[0]
            stream.seek(dol)
            header = stream.read(0x100)
            offsets = struct.unpack_from(">18I", header, 0)
            addresses = struct.unpack_from(">18I", header, 0x48)
            sizes = struct.unpack_from(">18I", header, 0x90)
            for name, record in RETAIL.items():
                address, size = record["address"], record["size"]
                section = next(i for i in range(18) if addresses[i] <= address and
                               address+size <= addresses[i]+sizes[i])
                stream.seek(dol+offsets[section]+address-addresses[section])
                assert hashlib.sha256(stream.read(size)).hexdigest() == record["sha256"], name
        retail_verified = True
    (args.output/"collision_mask_source.json").write_text(json.dumps({
        "original_ref": ORIGINAL_REF, "sources": receipts,
        "original_bool": "MSL typedef int bool", "body_count": len(all_names),
        "retail_function_count": len(RETAIL), "retail_verified": retail_verified}, indent=2)+"\n")
    print(f"Extracted {len(all_names)} actual original/candidate bodies; retail verified={retail_verified}")

if __name__ == "__main__":
    main()
