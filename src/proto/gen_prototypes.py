#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
import sys
from pathlib import Path
from typing import List, Tuple
from clang.cindex import Index, CursorKind, StorageClass, TokenKind

SHOW_DIAGS = os.environ.get("GENPROTO_SHOW_DIAGS", "") not in ("", "0", "false", "no")
DEBUG_LOG  = os.environ.get("GENPROTO_DEBUG", "") not in ("", "0", "false", "no")
STUB_PROTO_H = os.environ.get("GENPROTO_STUB_PROTO_H", "1") not in ("", "0", "false", "no")

# Preprocessor directive detection
_DIR_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b')
_IF0_RE = re.compile(r'^\s*#\s*if\s+0\s*(//.*)?$')

# Keep legacy behavior: drop whole group for #ifndef PROTO and #if !defined(PROTO)
_IFNDEF_PROTO_RE   = re.compile(r'^\s*#\s*ifndef\s+PROTO\s*(//.*)?$')
_IF_NOTDEF_PROTO_RE = re.compile(r'^\s*#\s*if\s*!defined\s*\(\s*PROTO\s*\)\s*(//.*)?$')

# Helpers to detect defined(PROTO) inside an expression line
_DEFINED_CALL_RE      = re.compile(r'defined\s*\(\s*PROTO\s*\)')
_NOT_DEFINED_CALL_RE  = re.compile(r'!+\s*defined\s*\(\s*PROTO\s*\)')

def _ends_with_bs(line: str) -> bool:
    return line.rstrip("\r\n").rstrip().endswith("\\")

def _has_proto_defined_from_argv(argv: List[str]) -> bool:
    """Return True if -DPROTO (or -DPROTO=...) appears in clang args."""
    for a in argv:
        if a == "-DPROTO" or a.startswith("-DPROTO="):
            return True
    return False

def _eval_condition_with_defined_proto(cond: str, proto_is_defined: bool) -> bool:
    """
    Evaluate a simple C preprocessor condition only when it contains defined(PROTO).
    - Replace defined(PROTO)/!defined(PROTO) based on proto_is_defined
    - Map C logical operators (&&, ||, !) to Python (and, or, not)
    - Treat unknown identifiers as 0 (false)
    - Prevent tokens like 'True(' / 'False(' / '0(' turning into call syntax
    On any parsing error, return True (fail-open -> keep the branch).
    """
    s = cond

    # Normalize spacing
    s = s.strip()

    # Replace !defined(PROTO) first, then defined(PROTO)
    if proto_is_defined:
        s = re.sub(r'!+\s*defined\s*\(\s*PROTO\s*\)', "False", s)
        s = re.sub(r'defined\s*\(\s*PROTO\s*\)', "True", s)
    else:
        s = re.sub(r'!+\s*defined\s*\(\s*PROTO\s*\)', "True", s)
        s = re.sub(r'defined\s*\(\s*PROTO\s*\)', "False", s)

    # Translate C logical ops to Python
    # Replace defined(PROTO) and !defined(PROTO) depending on -DPROTO
    if proto_is_defined:
        s = re.sub(r'!defined\s*\(\s*PROTO\s*\)', "False", s)
        s = re.sub(r'defined\s*\(\s*PROTO\s*\)', "True", s)
    else:
        s = re.sub(r'!defined\s*\(\s*PROTO\s*\)', "True", s)
        s = re.sub(r'defined\s*\(\s*PROTO\s*\)', "False", s)

    # Any other defined(MACRO) should be considered unknown
    s = re.sub(r'!defined\s*\(\s*[A-Za-z_]\w*\s*\)', "True", s)
    s = re.sub(r'defined\s*\(\s*[A-Za-z_]\w*\s*\)', "False", s)

    # Translate C logical operators to Python
    s = s.replace("&&", " and ")
    s = s.replace("||", " or ")
    # Replace '!' with ' not ' but do not touch '!='
    s = re.sub(r'(?<![=!])!', " not ", s)

    # Replace remaining identifiers (macros) with 0 (false)
    # Keep True/False and numbers as-is
    s = re.sub(r'\b(?!True\b|False\b)[A-Za-z_]\w*', "0", s)

    # Prevent literals followed by '(' from looking like a call: True(  False(  0(
    s = re.sub(r'\b(True|False|0)\s*\(', r'(\1) and (', s)

    # Safety: collapse excessive whitespace
    s = re.sub(r'\s+', " ", s).strip()

    try:
        return bool(eval(s, {}, {}))
    except Exception:
        # Fail-open to avoid accidentally dropping code
        if DEBUG_LOG:
            print(f"[warn] condition eval failed: {cond!r} -> {s!r}", file=sys.stderr)
        return True

def rewrite_conditionals_first_branch(text: str) -> str:
    """
    Keep only the first branch of #if/#ifdef/#ifndef groups, remove others.
    Extensions:
      - Drop whole group for #if 0.
      - Drop whole group for #ifndef PROTO and #if !defined(PROTO).
      - If an '#if <expr>' contains defined(PROTO) anywhere in the expression,
        evaluate the condition: if False, drop the whole group; if True, keep only
        the first branch (same as legacy behavior).
    """
    lines = text.splitlines(keepends=True)
    out: List[str] = []
    i, n = 0, len(lines)

    def _collect_group(start: int) -> Tuple[int, List[Tuple[str, int, int]]]:
        """Collect a full #if...#endif group and return (end_index, bodies).
        bodies is a list of (tag, body_s, body_e) for each if/elif/else branch.
        """
        h = start
        # Skip backslash-continued header lines
        while True:
            h += 1
            if h >= n or not _ends_with_bs(lines[h - 1]):
                break
        depth = 1
        j = h
        marks: List[Tuple[str, int]] = [("if", start)]
        while j < n and depth > 0:
            m = _DIR_RE.match(lines[j])
            if m:
                kw2 = m.group(1)
                if kw2 in ("if", "ifdef", "ifndef"):
                    depth += 1
                elif kw2 == "endif":
                    depth -= 1
                    if depth == 0:
                        break
                elif depth == 1 and kw2 in ("elif", "else"):
                    marks.append((kw2, j))
            j += 1
        else:
            # Unterminated: keep as-is from start to EOF
            return n, [("text", start, n)]

        def _after_header_line(pos: int) -> int:
            """
            Return the first index after a possibly backslash-continued header line
            starting at `pos`. It consumes all continuation lines that belong to
            the directive header.
            """
            k = pos + 1
            # Consume lines as long as the previous line ends with a backslash
            while k < n and _ends_with_bs(lines[k - 1]):
                k += 1
            return k

        bodies: List[Tuple[str, int, int]] = []
        header_positions = [pos for _, pos in marks] + [j]
        tags = [tag for tag, _ in marks]
        for idx, tag in enumerate(tags):
            header = header_positions[idx]
            # Start body right after the whole (possibly multi-line) header
            body_s = _after_header_line(header)
            next_header = header_positions[idx + 1]
            body_e = next_header
            bodies.append((tag, body_s, body_e))
        return j + 1, bodies

    # Detect whether PROTO is defined from argv (clang args passed after src path)
    proto_is_defined = _has_proto_defined_from_argv(sys.argv[2:])

    while i < n:
        line = lines[i]
        m = _DIR_RE.match(line)
        if not m:
            out.append(line)
            i += 1
            continue

        kw = m.group(1)
        if kw in ("if", "ifdef", "ifndef"):
            # Hard drops first (legacy behavior)
            if _IF0_RE.match(line) or _IFNDEF_PROTO_RE.match(line) or _IF_NOTDEF_PROTO_RE.match(line):
                group_end, _ = _collect_group(i)
                i = group_end
                continue

            # If '#if <expr>' contains defined(PROTO), evaluate; otherwise, legacy behavior
            evaluate = (kw == "if") and (
                _DEFINED_CALL_RE.search(line) is not None or _NOT_DEFINED_CALL_RE.search(line) is not None
            )

            if evaluate:
                # Extract condition text after 'if'
                try:
                    cond_text = line.split("if", 1)[1]
                except Exception:
                    cond_text = ""
                keep_first = _eval_condition_with_defined_proto(cond_text, proto_is_defined)
                if not keep_first:
                    group_end, _ = _collect_group(i)
                    i = group_end
                    continue

            # Keep only the first branch
            group_end, bodies = _collect_group(i)
            if not bodies:
                i = group_end
                continue
            keep_s, keep_e = bodies[0][1], bodies[0][2]
            kept_text = "".join(lines[keep_s:keep_e])
            kept_rewritten = rewrite_conditionals_first_branch(kept_text)
            out.append(kept_rewritten)
            i = group_end
        else:
            # For 'elif/else/endif' lines encountered directly, output a blank line to preserve line count
            out.append("\n" if line.endswith("\n") else "")
            i += 1

    return "".join(out)

def collapse_star_spaces(s: str) -> str:
    return re.sub(r"\*\s+", "*", s)

def fix_spacing(s: str) -> str:
    s = re.sub(r"\(\s+", "(", s)
    s = re.sub(r"\s+\)", ")", s)
    s = re.sub(r"(?<=[\w\)])\s+\((?!\*)", "(", s)
    s = re.sub(r"\s+,", ",", s)
    s = re.sub(r",(?=[^\s),])", ", ", s)
    s = re.sub(r"\s+\[", "[", s)
    s = re.sub(r"\[\s+", "[", s)
    s = re.sub(r"\s+\]", "]", s)
    return s

def strip_unused_macros(s: str) -> str:
    s = re.sub(r"\bUNUSED\b\s*\([^)]*\)", " ", s)
    s = re.sub(r"\bUNUSED\b", " ", s)
    return s

def normalize_ws(s: str) -> str:
    return re.sub(r"\s+", " ", s).strip()

def tokens_for_function_header(cur) -> List[str]:
    toks = [t for t in cur.get_tokens() if t.kind != TokenKind.COMMENT]
    if not toks:
        return []
    name_idx = None
    for i, t in enumerate(toks):
        if t.kind == TokenKind.IDENTIFIER and t.spelling == cur.spelling:
            name_idx = i
            break
    if name_idx is None:
        return []
    lpar_idx = None
    for i in range(name_idx + 1, len(toks)):
        if toks[i].spelling == '(':
            lpar_idx = i
            break
    if lpar_idx is None:
        return []
    depth, rpar_idx = 0, None
    for i in range(lpar_idx, len(toks)):
        if toks[i].spelling == '(':
            depth += 1
        elif toks[i].spelling == ')':
            depth -= 1
            if depth == 0:
                rpar_idx = i
                break
    if rpar_idx is None:
        return []
    return [t.spelling for t in toks[:rpar_idx + 1]]

def join_tokens(tokens: List[str]) -> str:
    s = " ".join(tokens)
    s = strip_unused_macros(s)
    s = normalize_ws(s)
    s = collapse_star_spaces(s)
    s = fix_spacing(s)
    return s

def in_this_file(cur, src_path: Path) -> bool:
    try:
        loc = cur.extent.start
        f = (loc.file or cur.location.file)
        if not f:
            return False
        cur_path = os.path.normcase(os.path.abspath(str(f.name)))
        src_abs = os.path.normcase(os.path.abspath(src_path.as_posix()))
        return cur_path == src_abs or os.path.basename(cur_path) == os.path.basename(src_abs)
    except Exception:
        return False

def collect_function_defs(tu, src_path: Path):
    def walk(node):
        if node.kind == CursorKind.FUNCTION_DECL and in_this_file(node, src_path):
            if node.storage_class != StorageClass.STATIC and node.is_definition():
                yield node
            return
        for ch in node.get_children():
            yield from walk(ch)
    yield from walk(tu.cursor)

def parse_with_clang(src_path: Path, text: str, args: List[str]):
    index = Index.create()
    src_abs = os.path.abspath(src_path.as_posix())
    unsaved = [(src_abs, text)]
    if STUB_PROTO_H:
        proto_h = os.path.abspath((src_path.parent / "proto.h").as_posix())
        if os.path.exists(proto_h):
            unsaved.append((proto_h, "/* stubbed */\n"))
    final_args = (args or []) + [
        "-std=c99", "-ferror-limit=0", "-w",
        "-Wno-implicit-function-declaration"
    ]
    tu = index.parse(src_abs, args=final_args, unsaved_files=unsaved, options=0)
    if SHOW_DIAGS:
        for d in getattr(tu, "diagnostics", []):
            print(d, file=sys.stderr)
    return tu

def main():
    if len(sys.argv) < 2:
        print("Usage: gen_prototypes.py source.c [clang-args...]", file=sys.stderr)
        sys.exit(1)
    src_path = Path(sys.argv[1])
    clang_args = sys.argv[2:]
    if not src_path.exists():
        print(f"not found: {src_path}", file=sys.stderr)
        sys.exit(2)
    try:
        original = src_path.read_text(encoding="utf-8", errors="ignore")
    except Exception as e:
        print(f"read error: {e}", file=sys.stderr)
        sys.exit(2)
    rewritten = rewrite_conditionals_first_branch(original)
    try:
        tu = parse_with_clang(src_path, rewritten, clang_args)
    except Exception as e:
        print(f"parse failed: {e}", file=sys.stderr)
        sys.exit(3)

    protos: List[str] = []
    for fn in collect_function_defs(tu, src_path):
        if DEBUG_LOG:
            try:
                sr = fn.extent.start
                er = fn.extent.end
                print(f"[def] {fn.spelling}  {sr.line}:{sr.column} - {er.line}:{er.column}", file=sys.stderr)
            except Exception:
                print(f"[def] {fn.spelling}", file=sys.stderr)
        toks = tokens_for_function_header(fn)
        header = join_tokens(toks)
        if header:
            protos.append(f"{header};")

    seen = set()
    unique_protos = []
    for p in protos:
        if p not in seen:
            unique_protos.append(p)
            seen.add(p)

    out_dir = Path("proto")
    out_dir.mkdir(parents=True, exist_ok=True)
    out_file = out_dir / (src_path.stem + ".pro")
    try:
        with out_file.open("w", encoding="utf-8") as f:
            f.write(f"/* {src_path.name} */\n")
            for p in unique_protos:
                f.write(p + "\n")
            f.write("/* vim: set ft=c : */\n")
    except Exception as e:
        print(f"write failed: {e}", file=sys.stderr)
        sys.exit(4)
    print(f"Generated {out_file.as_posix()} with {len(unique_protos)} prototypes.")

if __name__ == "__main__":
    main()
