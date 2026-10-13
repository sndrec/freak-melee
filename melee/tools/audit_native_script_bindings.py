"""Source-backed binding census for the direct FMS frontend.

This is audit tooling, not a second language parser or a runtime compiler. The
token stream comes from native_script_expression.c's production lexer. Walking
balanced postfix chains preserves selector arguments; it cannot establish type
acceptance. Unknown statements, signatures and ownership remain explicit.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests/native/script_binding_inventory"


def read_contracts():
    return json.loads((FIXTURE / "contracts.json").read_text(encoding="utf-8"))


def source_evidence():
    """Index literal source references; these are candidates, never signatures."""
    literals = defaultdict(list)
    for path in sorted((ROOT / "tools").glob("native_fighter*.c")):
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            for literal in re.findall(r'"((?:\\.|[^"\\])*)"', line):
                match = re.match(r"\.?([A-Za-z_][A-Za-z0-9_.]*)(?:\(| = |$)", literal.lstrip())
                if not match:
                    continue
                name = match[1]
                literals[name].append({
                    "path": path.relative_to(ROOT).as_posix(), "line": line_number,
                    "text": line.strip(),
                })
    return literals


def family_evidence(name):
    families = (("item.resource()", "tools/native_fighter_article_resources.c"),
                ("articles.ref()", "tools/native_fighter_script_article_refs.c"),
                ("hitbox.current()", "tools/native_fighter_script_terrain.c"),
                ("hitbox.previous()", "tools/native_fighter_script_terrain.c"),
                ("item.dynamic_joint()", "tools/native_fighter_item_operations.c"),
                ("item.command", "tools/native_fighter_item_script.c"),
                ("owner.command", "tools/native_fighter_item_script.c"),
                ("bones.rotate_", "tools/native_fighter_script_primitives.c"),
                ("holder.held_frame", "tools/native_fighter_item_statements.c"))
    for prefix, path in families:
        if name.startswith(prefix):
            return [{"path": path, "line": 1,
                     "text": "family parser candidate; exact signature unresolved"}]
    return []


def legacy_type_hints():
    content = (ROOT/"tools/native_fighter_script_types.c").read_text()
    hints = {}
    for array, kind in (("integers", "Int"), ("booleans", "Bool")):
        match = re.search(r"static const char\* " + array + r"\[\] = \{(.*?)\};", content, re.S)
        if match:
            for name in re.findall(r'"([A-Za-z_][A-Za-z0-9_.]*)"', match[1]):
                hints[name] = kind
    return hints


def matching(tokens):
    pairs, stack = {}, []
    for index, token in enumerate(tokens):
        text = token["text"]
        if text in ("(", "["):
            stack.append((text, index))
        elif text in (")", "]"):
            if not stack or stack[-1][0] != (")" == text and "(" or "["):
                raise ValueError(f"unmatched {text}")
            _, first = stack.pop()
            pairs[first] = index
    if stack:
        raise ValueError("unclosed selector/argument list")
    return pairs


def argument_spans(tokens, pairs, first, last):
    spans, start, index = [], first, first
    while index < last:
        text = tokens[index]["text"]
        if text in ("(", "["):
            index = pairs[index] + 1
        elif text == ",":
            spans.append((start, index))
            start = index + 1
            index += 1
        else:
            index += 1
    if start < last:
        spans.append((start, last))
    return spans


def spelling(tokens):
    return "".join(token["text"] for token in tokens)


def postfix_chains(tokens):
    """Inventory balanced token chains, including nested argument expressions."""
    pairs = matching(tokens)
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token["kind"] != "name":
            index += 1
            continue
        start, cursor = index, index + 1
        segments = [{"kind": "name", "text": token["text"]}]
        nested = []
        while cursor < len(tokens):
            text = tokens[cursor]["text"]
            if text == "." and cursor + 1 < len(tokens) and tokens[cursor+1]["kind"] == "name":
                segments.append({"kind": "member", "text": tokens[cursor+1]["text"]})
                cursor += 2
            elif text in ("(", "["):
                end = pairs[cursor]
                spans = argument_spans(tokens, pairs, cursor+1, end)
                args = [spelling(tokens[a:b]) for a, b in spans]
                segments.append({"kind": "call" if text == "(" else "index", "arguments": args})
                nested.extend(tokens[a:b] for a, b in spans)
                cursor = end + 1
            else:
                break
        yield {"segments": segments, "start": start, "end": cursor, "nested": False,
               "spelling": spelling(tokens[start:cursor])}
        for args in nested:
            for chain in postfix_chains(args):
                chain["nested"] = True
                yield chain
        index = cursor


def semantic_name(chain):
    segments = chain["segments"]
    result = ""
    for index, segment in enumerate(segments):
        if segment["kind"] == "name":
            result += segment["text"]
        elif segment["kind"] == "member":
            result += "." + segment["text"]
        elif segment["kind"] == "call" and index + 1 < len(segments):
            result += "()"
        elif segment["kind"] == "index":
            result += "[]"
    return result


def statement_tokens(tokens):
    """Split only statement framing; expression/selector lexing is shared."""
    if not tokens:
        return []
    head = tokens[0]["text"]
    if head in ("if", "elif", "while"):
        pairs = matching(tokens)
        index = 1
        while index < len(tokens):
            if tokens[index]["text"] in ("(", "["):
                index = pairs[index] + 1
            elif tokens[index]["text"] == ":":
                return [("read", tokens[1:index])] + statement_tokens(tokens[index+1:])
            else:
                index += 1
        raise ValueError("conditional has no colon")
    if head in ("else", "pass", "return", "break", "continue"):
        if head == "return":
            return [("read", tokens[1:])]
        if head == "else" and len(tokens) > 2:
            return statement_tokens(tokens[2:])
        return []
    pairs = matching(tokens)
    index = 0
    while index < len(tokens):
        if tokens[index]["text"] in ("(", "["):
            index = pairs[index]+1
        elif tokens[index]["text"] == "=":
            left = tokens[:index]
            if head == "let":
                left = left[2:] if len(left)>2 and left[1]["text"] in ("float", "bool", "int") else left[1:]
            return [("write", left), ("read", tokens[index+1:])]
        else:
            index += 1
    if head == "let":
        raise ValueError("local declaration has no initializer")
    return [("read", tokens)]


def symbols(lines):
    helpers, tables, state, attrs = defaultdict(set), defaultdict(set), defaultdict(dict), {}
    owner = "fighter"
    for line in lines:
        text = line.strip()
        if line and not line[0].isspace():
            if text.startswith(("article ", "item ")):
                owner = "article:" + text.split()[1]
            elif text.startswith("model_effect "):
                owner = "effect:" + text.split()[1]
            else:
                owner = "fighter"
        function = re.match(r"(?:export )?func (\w+)\(", text)
        if function:
            helpers[owner].add(function[1])
        table = re.match(r"table (bool|int|float) (\w+)", text)
        if table:
            tables[owner].add(table[2])
        field = re.match(r"state (bool|int|float) (\w+)", text)
        if field:
            state[owner][field[2]] = field[1]
        attribute = re.match(r"(?:attribute|parameter) (bool|int|float) (\w+)", text)
        if attribute:
            attrs[attribute[2]] = attribute[1]
    return helpers, tables, state, attrs


def audit_source(path, lexer, contracts, evidence):
    content = path.read_text(encoding="utf-8")
    lines = content.splitlines()
    helpers, tables, states, attrs = symbols(lines)
    lexed = subprocess.run([str(lexer), str(path)], check=True, capture_output=True, text=True)
    by_line = defaultdict(list)
    lexical = json.loads(lexed.stdout)
    lexical_errors = {item["line"]: item["error"] for item in lexical["errors"]}
    for token in lexical["tokens"]:
        by_line[token["line"]].append(token)
    occurrences, unresolved = [], []
    owner, body_indent, body_name, context = "fighter", None, "", "fighter"
    local_names = {}
    for number, line in enumerate(lines, 1):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        indent = len(line) - len(line.lstrip())
        if body_indent is not None and indent <= body_indent:
            body_indent = None
        if indent == 0:
            owner = "article:"+stripped.split()[1] if stripped.startswith(("article ", "item ")) else "effect:"+stripped.split()[1] if stripped.startswith("model_effect ") else "fighter"
        if re.match(r"(?:export )?func |on ", stripped):
            body_indent, body_name = indent, stripped
            context = "article" if owner.startswith("article:") else "effect" if owner.startswith("effect:") else "recipient" if stripped.startswith("export ") else "fighter"
            local_names = {name: indent+1 for name in re.findall(r"\b(?:float|int|bool) (\w+)", stripped)}
            continue
        if body_indent is None:
            continue
        tokens = by_line[number]
        local_names = {name: depth for name, depth in local_names.items() if indent >= depth}
        for index, token in enumerate(tokens):
            if token["text"] == "let" and index+1 < len(tokens):
                name_index = index+2 if tokens[index+1]["text"] in ("float", "int", "bool") else index+1
                if name_index < len(tokens):
                    local_names[tokens[name_index]["text"]] = indent
        try:
            if number in lexical_errors:
                raise ValueError(lexical_errors[number])
            chunks = statement_tokens(tokens)
            for mode, expression in chunks:
                for chain in postfix_chains(expression):
                    name = semantic_name(chain)
                    root = chain["segments"][0]["text"]
                    called = chain["segments"][-1]["kind"] == "call"
                    if root in local_names:
                        continue
                    # Scalar locals/enums are not native operations. Their exact
                    # type validation belongs to the production frontend.
                    if len(chain["segments"]) == 1:
                        if root not in ("frame", "airborne", "grounded", "facing") and root not in helpers[owner] and root not in helpers["fighter"]:
                            continue
                    classification = "native"
                    if root in helpers[owner] or root in helpers["fighter"]:
                        classification = "authored_helper"
                    elif root == "table":
                        classification = "authored_table"
                    elif root in ("attrs", "state") and not called:
                        classification = "owned_parameter" if root == "attrs" else "authored_state"
                    elif name.startswith(("item.state.", "owner.state.", "creator.state.")):
                        classification = "authored_state"
                    elif name.startswith("item.params."):
                        classification = "owned_parameter"
                    elif name.startswith("item.attr."):
                        classification = "owned_article_attribute"
                    elif root[0].isupper() or root.startswith("It_Kind_"):
                        classification = "constant"
                    contract = contracts.get(name)
                    if classification == "native" and contract is None and name.startswith("item.resource()."):
                        contract = contracts.get("item.resource()." + name.split("item.resource().", 1)[1])
                    key = name.replace("()", "").replace("[]", "")
                    candidate = evidence.get(key, [])
                    if not candidate and "." in key:
                        parts = key.split(".")
                        for first in range(1, len(parts)):
                            candidate = evidence.get(".".join(parts[first:]), [])
                            if candidate:
                                break
                    if not candidate:
                        candidate = family_evidence(name)
                    entry = {"path": path.relative_to(ROOT).as_posix(), "line": number,
                             "context": context, "owner": owner, "body": body_name,
                             "operation": name, "mode": "call" if called else "read" if chain["nested"] else mode,
                             "classification": classification, "chain": chain["segments"],
                             "spelling": chain["spelling"]}
                    if contract:
                        entry["contract_name"] = name
                        entry["context_matches_catalog"] = context in contract["contexts"]
                        entry["access_matches_catalog"] = entry["mode"] in contract["access"]
                        if called:
                            args = chain["segments"][-1]["arguments"]
                            entry["outputs"] = [{"argument": i, "destination": args[i]}
                                for i in contract.get("output_arguments", []) if i < len(args)]
                    elif classification == "native":
                        entry["contract_status"] = "signature_context_and_invalidation_unresolved"
                        entry["source_candidates"] = candidate[:8]
                    elif classification == "authored_state":
                        field = chain["segments"][-1].get("text")
                        state_owner = owner if name.startswith("item.state.") else "fighter"
                        entry["declared_type"] = states[state_owner].get(field, "unresolved")
                    elif classification == "owned_parameter":
                        entry["declared_type"] = attrs.get(chain["segments"][-1].get("text"), "unresolved")
                    occurrences.append(entry)
        except ValueError as error:
            unresolved.append({"path": path.relative_to(ROOT).as_posix(), "line": number,
                               "statement": stripped, "error": str(error)})
    return {"path": path.relative_to(ROOT).as_posix(),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "helpers": {key: sorted(value) for key, value in helpers.items()},
            "tables": {key: sorted(value) for key, value in tables.items()},
            "state": dict(states), "parameters": attrs,
            "occurrences": occurrences, "unparsed_statements": unresolved}


def audit(lexer, paths):
    contracts, evidence = read_contracts(), source_evidence()
    hints = legacy_type_hints()
    files = [audit_source(path, lexer, contracts, evidence) for path in paths]
    operations = defaultdict(lambda: {"contexts": set(), "modes": set(), "uses": 0, "contract_status": "unresolved"})
    for item in files:
        for occurrence in item["occurrences"]:
            if occurrence["classification"] != "native":
                continue
            operation = operations[occurrence["operation"]]
            operation["contexts"].add(occurrence["context"])
            operation["modes"].add(occurrence["mode"])
            operation["uses"] += 1
            if "contract_name" in occurrence:
                operation["contract_status"] = contracts[occurrence["contract_name"]]["status"]
    output_operations = {key: dict(value, contexts=sorted(value["contexts"]), modes=sorted(value["modes"]))
                         for key, value in sorted(operations.items())}
    for key, value in output_operations.items():
        value["script_signature"] = {"contract_name": key} if key in contracts else "unresolved"
        if key in hints:
            value["legacy_result_hint"] = hints[key]
            value["legacy_result_hint_source"] = "tools/native_fighter_script_types.c"
    checked_sources = sorted({path for contract in contracts.values() for path in contract["sources"]})
    source_hashes = {path: hashlib.sha256((ROOT/path).read_bytes()).hexdigest() for path in checked_sources}
    return {"scope": "selected *_full.fms templates and Rook workshop source",
            "method": "shared production lexer; balanced postfix census; source-evidence contracts",
            "production_type_acceptance": False, "contracts": contracts, "files": files,
            "operations": output_operations, "contract_source_sha256": source_hashes,
            "summary": {"files": len(files), "native_operations": len(operations),
                        "native_occurrences": sum(value["uses"] for value in operations.values()),
                        "contract_statuses": dict(Counter(value["contract_status"] for value in operations.values())),
                        "unparsed_statements": sum(len(item["unparsed_statements"]) for item in files)}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lexer", type=Path, required=True, help="shared lexer census executable")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("sources", nargs="*", type=Path)
    args = parser.parse_args()
    paths = args.sources or sorted((ROOT/"examples").glob("*_full.fms")) + sorted((ROOT/"examples/rook_workshop").rglob("*.fms"))
    manifest = audit(args.lexer, paths)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest["summary"], indent=2))


if __name__ == "__main__":
    main()
