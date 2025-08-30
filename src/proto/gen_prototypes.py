#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Prototype generator:
- Use libclang (clang.cindex) ONLY to detect function definitions in a .c file.
- Build the prototype text "as written" from original source tokens:
  * keep author's spelling (e.g., 'unsigned' vs 'unsigned int')
  * remove comments and newlines
  * tighten spaces: no space after '(', before ')', before ',', collapse spaces
  * collapse spaces after '*' (e.g., '* const' -> '*const', '*   *' -> '**')
  * remove 'UNUSED' and 'UNUSED(...)'
- Preprocess conditionals in-memory:
  * keep '#if 0 ... #endif' blocks intact (stay disabled)
  * remove all other conditional-directive lines (#if/#ifdef/#ifndef/#elif/#else/#endif)
- Output one proto/<stem>.pro per input .c file.
"""

from __future__ import annotations
import os
import re
import sys
from pathlib import Path
from typing import List, Tuple
from clang.cindex import Index, CursorKind, StorageClass, TokenKind

# ---------------------- Config via env ----------------------
SHOW_DIAGS     = os.environ.get("GENPROTO_SHOW_DIAGS", "") not in ("", "0", "false", "no")
INCLUDE_STATIC = os.environ.get("GENPROTO_INCLUDE_STATIC", "") not in ("", "0", "false", "no")

# Extra clang flags can be passed after the filename on CLI, or via make
# (e.g., -I, -D). We simply forward sys.argv[2:].
# -----------------------------------------------------------

_DIRECTIVE_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b')
_IF0_RE       = re.compile(r'^\s*#\s*if\s+0\b')

def _ends_with_backslash(line: str) -> bool:
    return line.rstrip("\r\n").rstrip().endswith("\\")

def rewrite_conditionals_keep_if0(text: str) -> str:
    """
    Keep '#if 0 ... #endif' blocks verbatim; remove all other conditional-directive lines.
    Also remove any backslash-continued condition lines that follow '#if' or '#elif'.
    Newlines are preserved to keep locations stable.
    """
    lines = text.splitlines(keepends=True)
    out: List[str] = []
    i = 0
    n = len(lines)
    keep0_depth = 0

    while i < n:
        line = lines[i]
        m = _DIRECTIVE_RE.match(line)

        if keep0_depth > 0:
            # Inside kept '#if 0' region: keep directives as-is and track depth.
            if m:
                kw = m.group(1)
                if kw in ("if", "ifdef", "ifndef"):
                    keep0_depth += 1
                elif kw == "endif":
                    keep0_depth -= 1
                out.append(line)
                i += 1
            else:
                out.append(line)
                i += 1
            continue

        if not m:
            out.append(line)
            i += 1
            continue

        kw = m.group(1)

        if kw == "if":
            if _IF0_RE.match(line):
                # keep '#if 0' line (and any continued condition lines)
                keep0_depth = 1
                out.append(line)
                i += 1
                while i < n and _ends_with_backslash(lines[i - 1]):
                    out.append(lines[i])   # keep continuation line
                    i += 1
            else:
                # drop '#if ...' line and ALL its continuation lines
                out.append("\n" if line.endswith("\n") else "")
                i += 1
                while i < n and _ends_with_backslash(lines[i - 1]):
                    out.append("\n" if lines[i].endswith("\n") else "")
                    i += 1
        elif kw in ("elif",):
            # drop '#elif ...' and its continuation lines
            out.append("\n" if line.endswith("\n") else "")
            i += 1
            while i < n and _ends_with_backslash(lines[i - 1]):
                out.append("\n" if lines[i].endswith("\n") else "")
                i += 1
        elif kw in ("ifdef", "ifndef", "else", "endif"):
            # drop these directive lines (no continuations in practice)
            out.append("\n" if line.endswith("\n") else "")
            i += 1
        else:
            out.append(line)
            i += 1

    return "".join(out)

# ---------------------- Formatting helpers ----------------------

def collapse_star_spaces(s: str) -> str:
    """Collapse spaces after '*' and between consecutive stars; keep pre-star space."""
    return re.sub(r"\*\s+", "*", s)

def fix_paren_and_comma_spacing(s: str) -> str:
    # normalize spaces after/before parens
    s = re.sub(r"\(\s+", "(", s)   # after '('
    s = re.sub(r"\s+\)", ")", s)   # before ')'
    # remove space before '(' when preceded by word or ')'
    # but DO NOT tighten when it starts a function-pointer declarator: " (*"
    s = re.sub(r"(?<=[\w\)])\s+\((?!\*)", "(", s)
    # commas
    s = re.sub(r"\s+,", ",", s)
    s = re.sub(r",(?=[^\s),])", ", ", s)
    # array brackets
    s = re.sub(r"\s+\[", "[", s)   # before '['
    s = re.sub(r"\[\s+", "[", s)   # after '['
    s = re.sub(r"\s+\]", "]", s)   # before ']'
    return s

def strip_comments(s: str) -> str:
    """Remove C block comments and C++ line comments."""
    s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
    s = re.sub(r"//[^\n]*", " ", s)
    return s

def strip_unused_macros(s: str) -> str:
    """Remove UNUSED and UNUSED(...) from a signature string."""
    s = re.sub(r"\bUNUSED\b\s*\([^)]*\)", " ", s)
    s = re.sub(r"\bUNUSED\b", " ", s)
    return s

def normalize_ws(s: str) -> str:
    return re.sub(r"\s+", " ", s).strip()

# ---------------------- Token slicing ----------------------

def tokens_for_function_header(c) -> List[str]:
    """
    Return token spellings forming: <return-specifiers+name+params up to final ')'>.
    We:
      - find the name token, then the following '(' that starts parameter list
      - collect tokens from start of extent up to the matching ')'
    Comments and preprocessor lines are not returned by libclang tokens.
    """
    # Collect tokens and drop comments so they never reach the join step.
    toks = [t for t in c.get_tokens() if t.kind != TokenKind.COMMENT]
    if not toks:
        return []
    # Find name token index
    name_idx = None
    for i, t in enumerate(toks):
        if t.spelling == c.spelling and t.kind == TokenKind.IDENTIFIER:
            name_idx = i
            break
    if name_idx is None:
        return []

    # Find '(' that starts the parameter list (first '(' after name)
    lpar_idx = None
    for i in range(name_idx + 1, len(toks)):
        if toks[i].spelling == '(':
            lpar_idx = i
            break
    if lpar_idx is None:
        return []

    # Match to the corresponding ')', counting nested parentheses
    depth = 0
    rpar_idx = None
    for i in range(lpar_idx, len(toks)):
        sp = toks[i].spelling
        if sp == '(':
            depth += 1
        elif sp == ')':
            depth -= 1
            if depth == 0:
                rpar_idx = i
                break
    if rpar_idx is None:
        return []

    # Start from the first token of the extent (should be decl specifiers)
    start_idx = 0
    # But tighten to the nearest token line above the name, to avoid dragging previous statements.
    # If token API provides locations, use them to chop to the same/previous line of the name.
    try:
        name_line = toks[name_idx].location.line
        # move start forward until token is on name_line or the previous non-empty line
        # here: choose earliest token with line >= name_line - 1
        for i, t in enumerate(toks):
            if t.location.line >= name_line - 1:
                start_idx = i
                break
    except Exception:
        pass

    return [t.spelling for t in toks[start_idx:rpar_idx + 1]]

def join_tokens_minimal(tokens: List[str]) -> str:
    """
    Join tokens with single spaces, then run cleanups.
    We do not try to be clever during join; post-fixes handle punctuation.
    """
    if not tokens:
        return ""
    # Tokens already exclude comments; join directly.
    s = " ".join(tokens)
    s = strip_unused_macros(s)
    s = normalize_ws(s)
    s = collapse_star_spaces(s)
    s = fix_paren_and_comma_spacing(s)
    return s

# ---------------------- libclang traversal ----------------------

def parse_with_clang(path: Path, text: str, args: List[str]):
    index = Index.create()
    tu = index.parse(
        path.as_posix(),
        args=args or ["-std=c99"],
        unsaved_files=[(path.as_posix(), text)],
        options=0
    )
    if SHOW_DIAGS:
        for d in getattr(tu, "diagnostics", []):
            print(d, file=sys.stderr)
    return tu

def in_this_file(cur, src_path: Path) -> bool:
    try:
        f = cur.location.file
        return bool(f) and Path(str(f.name)).name == src_path.name
    except Exception:
        return False

def collect_function_defs(tu, src_path: Path):
    """Yield FUNCTION_DECL cursors that are definitions belonging to src_path."""
    def walk(node):
        if node.kind == CursorKind.FUNCTION_DECL and in_this_file(node, src_path):
            if not INCLUDE_STATIC and node.storage_class == StorageClass.STATIC:
                return
            if node.is_definition():
                yield node
            return
        for ch in node.get_children():
            yield from walk(ch)
    yield from walk(tu.cursor)

# ---------------------- Main per-file pipeline ----------------------

def process_one_file(src: Path, clang_args: List[str]) -> List[str]:
    original = src.read_text(encoding="utf-8", errors="ignore")
    rewritten = rewrite_conditionals_keep_if0(original)

    tu = parse_with_clang(src, rewritten, clang_args)

    protos: List[str] = []
    for fn in collect_function_defs(tu, src):
        header_tokens = tokens_for_function_header(fn)
        header = join_tokens_minimal(header_tokens)
        if not header:
            continue
        # Ensure we end with a proper prototype
        protos.append(f"{header};")
    return protos

# ---------------------- CLI ----------------------

def main():
    """
    Entry point:
    - Read a single .c source file
    - Rewrite conditionals (keep only '#if 0 ... #endif' blocks as-is)
    - Parse with libclang (unsaved_files)
    - Collect FUNCTION_DECL definitions that belong to this file
    - Rebuild prototypes from original tokens (comments already dropped)
    - Tighten spacing (but keep function-pointer '(*' spacing intact via fix_paren_and_comma_spacing)
    - Deduplicate identical prototypes while preserving order
    - Write proto/<stem>.pro
    """
    if len(sys.argv) < 2:
        print("Usage: gen_prototypes.py source.c [clang-args...]", file=sys.stderr)
        sys.exit(1)

    src_path = Path(sys.argv[1])
    clang_args = sys.argv[2:]
    if not src_path.exists():
        print(f"not found: {src_path}", file=sys.stderr)
        sys.exit(2)

    # Load original source and rewrite conditionals for parsing
    try:
        original = src_path.read_text(encoding="utf-8", errors="ignore")
    except Exception as e:
        print(f"failed to read {src_path}: {e}", file=sys.stderr)
        sys.exit(2)

    rewritten = rewrite_conditionals_keep_if0(original)

    # Parse the rewritten buffer with libclang
    try:
        tu = parse_with_clang(src_path, rewritten, clang_args)
    except Exception as e:
        print(f"libclang parse failed: {e}", file=sys.stderr)
        sys.exit(3)

    # Collect prototypes from FUNCTION_DECL definitions
    protos: List[str] = []
    for fn in collect_function_defs(tu, src_path):
        toks = tokens_for_function_header(fn)       # tokens (comments removed)
        header = join_tokens_minimal(toks)          # minimal join + spacing rules
        if not header:
            continue
        # Final punctuation tightening (idempotent)
        header = collapse_star_spaces(header)
        header = fix_paren_and_comma_spacing(header)
        protos.append(f"{header};")

    # Deduplicate identical prototypes (preserve first occurrence order)
    seen = set()
    unique_protos: List[str] = []
    for p in protos:
        if p not in seen:
            unique_protos.append(p)
            seen.add(p)

    # Write out proto/<stem>.pro
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
        print(f"failed to write {out_file}: {e}", file=sys.stderr)
        sys.exit(4)

    print(f"Generated {out_file.as_posix()} with {len(unique_protos)} prototypes.")

if __name__ == "__main__":
    main()
