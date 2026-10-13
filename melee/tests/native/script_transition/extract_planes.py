"""Extract current native ground/air transition bodies for the test fixture."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
EXTRACTOR=Path(__file__).resolve().parents[1]/"script_kinetics/extract_primitives.py"
spec=importlib.util.spec_from_file_location("transition_body_extractor",EXTRACTOR)
reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
FUNCTIONS=["ftCommon_UnlockECB","ftCommon_8007D5D4","ftCommon_8007D6A4","ftCommon_8007D7FC"]


def extract(root,output):
    source=root/"src/melee/ft/ftcommon.c"
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text("\n\n".join(reference.body(source.read_text(),name) for name in FUNCTIONS)+"\n",newline="\n")
    return {"functions":FUNCTIONS,"inputs":{str(source):hashlib.sha256(source.read_bytes()).hexdigest()},
            "extracted_sha256":hashlib.sha256(output.read_bytes()).hexdigest()}


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root",type=Path,default=ROOT)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args();print(json.dumps(extract(args.source_root.resolve(),args.output.resolve())))
