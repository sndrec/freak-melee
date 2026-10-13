"""Verify complete direct program assembly and transactional inactive banks."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
NO_WINDOW=getattr(subprocess,"CREATE_NO_WINDOW",0)
UNITS=["native_script_source.c","native_script_expression.c","native_script_statement.c",
       "native_script_program.c","native_script_bytecode.c","native_script_value.c","native_script_vm.c",
       "native_script_events.c","native_script_event_payload.c","native_script_bindings.c", "native_script_binding_item_resources_catalog.c", "native_script_binding_spawn_catalog.c",
       "native_script_binding_effect_resolve.c","native_script_binding_spawn_resolve.c","native_script_constants.c","native_script_moves.c","native_script_move_lowering.c",
       "native_script_receiver.c","native_script_binding_receiver.c","native_script_runtime.c","native_callback_lifetime.c",
       "native_script_math.c","native_script_storage.c","native_script_execution.c","native_script_compile.c", "native_script_binding_item_resources_resolve.c",
       "native_script_binding_article_reference_resolve.c","native_script_binding_signal_resolve.c",
       "native_script_binding_item_model_resolve.c","native_script_model_metadata.c",
       "native_script_pose_metadata.c","native_script_hitbox_metadata.c","native_script_metadata.c","native_fighter_pose_path.c"]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    executable=args.output/"test.exe"
    command=[str(args.compiler),"-std=c11","-Wall","-Wextra","-Werror","-O2","-ffp-contract=off",
             "-fuse-ld=lld","-Wl,/errorlimit:0","-Wno-array-parameter","-D_USE_MATH_DEFINES","-DMELEE_NATIVE=1","-D_CRT_SECURE_NO_WARNINGS","-I",str(ROOT/"src"),"-I",str(ROOT/"extern/dolphin/include"),
             str(HERE/"test.c"),str(HERE/"native_boundaries.c")]
    boundaries=sorted(HERE.parent.glob("script_bindings_unavailable_*.c"))
    boundaries += [HERE.parent/name for name in ["script_receiver_unavailable_identity.c",
        "script_receiver_unavailable_pool.c","script_receiver_unavailable_registry.c"]]
    supporting=[ROOT/"src/melee/gm/native_match_pool.c",ROOT/"src/sysdolphin/baselib/dat.c",
                ROOT/"src/melee/it/native_article_animation_demand.c",ROOT/"src/melee/it/native_article_animation_texture.c"]
    command += list(map(str,boundaries+supporting))
    command += [str(ROOT/"src/melee/ft"/unit) for unit in UNITS]+["-o",str(executable)]
    paths=[HERE/"test.c",HERE/"native_boundaries.c",HERE/"probe.fms",Path(__file__).resolve()]
    paths += [ROOT/"src/melee/ft"/unit for unit in UNITS]+boundaries+supporting
    paths += list((ROOT/"src").rglob("*.h"))+list((ROOT/"extern/dolphin/include").rglob("*.h"))
    paths += [args.compiler.resolve()]
    hashes=lambda: {str(path.resolve()):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    before=hashes()
    build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,creationflags=NO_WINDOW)
    (args.output/"build.log").write_text(build.stdout+build.stderr);build.check_returncode()
    run=subprocess.run([str(executable),str(HERE/"probe.fms")],cwd=ROOT,capture_output=True,text=True,creationflags=NO_WINDOW)
    (args.output/"run.log").write_text(run.stdout+run.stderr);run.check_returncode()
    receipt=json.loads(run.stdout)
    if hashes()!=before:
        raise RuntimeError("Fixture inputs changed during verification")
    receipt.update({"scope":"actual complete compile assembly and immutable publication; controlled runtime property boundaries",
                    "compiler":command,"game_acceptance":False,"whole_package_acceptance":False,
                    "source_sha256":before,"build_exit":build.returncode,"run_exit":run.returncode})
    (args.output/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print(json.dumps({key:value for key,value in receipt.items() if key not in ("compiler","source_sha256")},indent=2))


if __name__=="__main__":main()
