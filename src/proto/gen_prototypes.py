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

_DIR_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b')
_IF0_RE = re.compile(r'^\s*#\s*if\s+0\b')
_IFNDEF_PROTO_RE = re.compile(r'^\s*#\s*ifndef\s+PROTO\b')        # drop whole group
_IF_NOTDEF_PROTO_RE = re.compile(r'^\s*#\s*!defined\s*\(\s*PROTO\s*\)')

def _ends_with_bs(line: str) -> bool:
    return line.rstrip("\r\n").rstrip().endswith("\\")

def rewrite_conditionals_first_branch(text: str) -> str:
    """Keep only the first branch of #if/#ifdef/#ifndef groups, remove others."""
    lines = text.splitlines(keepends=True)
    out: List[str] = []
    i, n = 0, len(lines)

    def _collect_group(start: int) -> Tuple[int, List[Tuple[str,int,int]]]:
        h = start
        while True:
            h += 1
            if h >= n or not _ends_with_bs(lines[h - 1]):
                break
        depth = 1
        j = h
        marks: List[Tuple[str,int]] = [("if", start)]
        while j < n and depth > 0:
            m = _DIR_RE.match(lines[j])
            if m:
                kw = m.group(1)
                if kw in ("if", "ifdef", "ifndef"):
                    depth += 1
                elif kw == "endif":
                    depth -= 1
                    if depth == 0:
                        break
                elif depth == 1 and kw in ("elif", "else"):
                    marks.append((kw, j))
            j += 1
        else:
            return n, [("text", start, n)]

        bodies: List[Tuple[str,int,int]] = []
        header_positions = [pos for _, pos in marks] + [j]
        tags = [tag for tag, _ in marks]
        for idx, tag in enumerate(tags):
            header = header_positions[idx]
            body_s = header + 1
            while body_s < n and _ends_with_bs(lines[body_s - 1]):
                body_s += 1
            next_header = header_positions[idx + 1]
            body_e = next_header
            bodies.append((tag, body_s, body_e))
        return j + 1, bodies

    while i < n:
        line = lines[i]
        m = _DIR_RE.match(line)
        if not m:
            out.append(line); i += 1; continue
        kw = m.group(1)
        if kw in ("if", "ifdef", "ifndef"):
            if _IF0_RE.match(line) or _IFNDEF_PROTO_RE.match(line) or _IF_NOTDEF_PROTO_RE.match(line):
                group_end, _ = _collect_group(i)
                i = group_end
                continue
            group_end, bodies = _collect_group(i)
            # always keep first body regardless of actual condition
            if bodies:
                keep_s, keep_e = bodies[0][1], bodies[0][2]
                kept_text = "".join(lines[keep_s:keep_e])
                kept_rewritten = rewrite_conditionals_first_branch(kept_text)
                out.append(kept_rewritten)
            i = group_end
        else:
            # skip elif/else/endif
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
