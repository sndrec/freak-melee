"""Build shared lexer in an isolated output and check semantic audit boundaries."""
from pathlib import Path
import argparse
import importlib.util
import json
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compiler", default="clang")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    lexer = args.output / "lexer_census.exe"
    command = [args.compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
               "-I", str(ROOT/"src/melee/ft"), str(HERE/"lexer_census.c"),
               str(ROOT/"src/melee/ft/native_script_expression.c"), "-o", str(lexer)]
    subprocess.run(command, check=True)
    spec = importlib.util.spec_from_file_location("binding_audit", ROOT/"tools/audit_native_script_bindings.py")
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    probe = audit.audit(lexer, [HERE/"probe.fms"])
    assert not probe["files"][0]["unparsed_statements"]
    uses = probe["files"][0]["occurrences"]
    operations = probe["operations"]
    assert "unknown.operation" in operations
    assert operations["unknown.operation"]["contract_status"] == "unresolved"
    assert "item.nonexistent" not in operations and "item.fake" not in operations
    selected = [u for u in uses if u["operation"] == "item.resource().joint().translation.x"]
    assert len(selected) == 1
    calls = [s["arguments"] for s in selected[0]["chain"] if s["kind"] == "call"]
    assert calls == [["item.state.node"], ['"0/1"']]
    assert selected[0]["mode"] == "write"
    state = [u for u in uses if u["spelling"] == "item.state.node"]
    assert state and all(u["mode"] == "read" for u in state)
    assert any(u["classification"] == "authored_helper" and u["operation"] == "ArticleHelper" for u in uses)
    assert any(u["classification"] == "authored_table" and u["operation"] == "table.scale[]" for u in uses)
    assert any(u["context"] == "recipient" and u["operation"] == "position.x" for u in uses)
    assert not any(u["body"].startswith("func Shadow") and u["classification"] == "native" for u in uses)
    assert any(u["owner"] == "article:WorkshopItem" and u["context"] == "article" and u["operation"] == "facing" for u in uses)
    query = next(u for u in uses if u["operation"] == "targets.read_position")
    assert [out["destination"] for out in query["outputs"]] == ["state.x", "state.y", "state.z"]
    resource_query = next(u for u in uses if u["operation"] == "item.resource().read_line_speed")
    assert probe["contracts"][resource_query["contract_name"]]["output_policy"] != probe["contracts"][query["contract_name"]]["output_policy"]
    # Preserve mismatched delimiters as a reportable error rather than silently
    # extracting a misleading binding prefix from the damaged expression.
    try:
        audit.matching([{"text": "("}, {"text": "]"}])
    except ValueError:
        pass
    else:
        raise AssertionError("mismatched selector accepted")
    paths = sorted((ROOT/"examples").glob("*_full.fms")) + sorted((ROOT/"examples/rook_workshop").rglob("*.fms"))
    manifest = audit.audit(lexer, paths)
    assert manifest["summary"]["files"] == 16
    assert manifest["summary"]["unparsed_statements"] == 0
    assert manifest["production_type_acceptance"] is False
    assert "item.resource().joint().translation.x" in manifest["operations"]
    assert "articles.ref().exists" in manifest["operations"]
    assert manifest["summary"]["contract_statuses"]["unresolved"] > 0
    (args.output/"manifest.json").write_text(json.dumps(manifest, indent=2)+"\n")
    receipt = {"result": "pass", "scope": "shared lexer and structural binding census",
               "summary": manifest["summary"], "production_type_acceptance": False,
               "compiler": command, "probe_checks": ["selector argument preservation",
               "nested source reads", "helper/table distinction", "recipient context",
               "output destinations", "query publication policy distinction",
               "quoted/comment exclusion", "unknown binding remains unresolved",
               "mismatched delimiters rejected", "complete selected source census"]}
    (args.output/"receipt.json").write_text(json.dumps(receipt, indent=2)+"\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
