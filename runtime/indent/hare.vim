vim9script

# Vim indent file.
# Language:    Hare
# Maintainer:  Amelia Clarke <selene@perilune.dev>
# Last Change: 2025 Sep 06
# Upstream:    https://git.sr.ht/~sircmpwn/hare.vim

if exists('b:did_indent')
  finish
endif
b:did_indent = 1

# L0 -> Don't unindent lines that look like C labels.
# :0 -> Don't indent `case` in match and switch expressions. This only affects
#       lines containing `:` (that isn't part of `::`).
# +0 -> Don't indent continuation lines.
# (s -> Indent one level inside parens.
# u0 -> Don't indent additional levels inside nested parens.
# U1 -> Don't treat `(` any differently if it is at the start of a line.
# m1 -> Indent lines starting with `)` the same as the matching `(`.
# j1 -> Indent blocks one level inside parens.
# J1 -> Indent structs and unions correctly.
# *0 -> Don't search for unclosed C-style block comments.
# #1 -> Don't unindent lines starting with `#`.
setlocal cinoptions=L0,:0,+0,(s,u0,U1,m1,j1,J1,*0,#1
setlocal cinscopedecls=
setlocal indentexpr=GetHareIndent()
setlocal indentkeys=0{,0},0),0],!^F,o,O,e,0=case
setlocal nolisp
b:undo_indent = 'setl cino< cinsd< inde< indk< lisp<'

# Calculates the indentation for the current line, using the value computed by
# cindent and manually fixing the cases where it behaves incorrectly.
def GetHareIndent(): number
  # Get the preceding lines of context and the value computed by cindent.
  const line = getline(v:lnum)
  const [plnum, pline] = PrevNonBlank(v:lnum - 1)
  const [pplnum, ppline] = PrevNonBlank(plnum - 1)
  const pindent = indent(plnum)
  const ppindent = indent(pplnum)
  const cindent = cindent(v:lnum) / shiftwidth() * shiftwidth()

  # If this line is a comment, don't try to align it with a comment at the end
  # of the previous line.
  if line =~ '^\s*//' && getline(plnum) =~ '\s*//.*$'
    return -1
  endif

  # Indent `case`.
  if line =~ '^\s*case\>'
    # If the previous line was also a `case`, use the same indent.
    if pline =~ '^\s*case\>'
      return pindent
    endif

    # If the previous line started the block, use the same indent.
    if pline =~ '{$'
      return pindent
    endif

    # If the current line contains a `:` that is not part of `::`, use the
    # computed cindent.
    if line =~ '\v%(%(::)*)@>:'
      return cindent
    endif

    # Unindent after a multi-line `case`.
    if pline =~ '=>$'
      return pindent - shiftwidth() * GetValue('hare_indent_case', 2)
    endif

    # If the previous line closed a set of parens, search for the previous
    # `case` within the same block and use the same indent. This fixes issues
    # with `case` not being correctly unindented after a function call
    # continuation line:
    #
    #   case let err: fs::error =>
    #           fmt::fatalf("Unable to open {}: {}",
    #                   os::args[1], fs::strerror(err));
    #           case // <-- cindent tries to unindent by only one shiftwidth
    if pline =~ ');$'
      const case = PrevMatchInBlock('^\s*case\>', plnum - 1)
      if case > 0
        return indent(case)
      endif
    endif

    # If cindent would indent the same or more than the previous line, unindent.
    if cindent >= pindent
      return pindent - shiftwidth()
    endif

    # Otherwise, use the computed cindent.
    return cindent
  endif

  # Indent after `case`.
  if line !~ '^\s*}'
    # If the previous `case` started and ended on the same line, indent.
    if pline =~ '^\s*case\>.*;$'
      return pindent + shiftwidth()
    endif

    # Indent after a single-line `case`.
    if pline =~ '^\s*case\>.*=>$'
      return pindent + shiftwidth()
    endif

    # Indent inside a multi-line `case`.
    if pline =~ '^\s*case\>' && pline !~ '=>'
      return pindent + shiftwidth() * GetValue('hare_indent_case', 2)
    endif

    # Indent after a multi-line `case`.
    if pline =~ '=>$'
      return pindent - shiftwidth() * (GetValue('hare_indent_case', 2) - 1)
    endif

    # Don't unindent while inside a `case` body.
    if ppline =~ '=>$' && pline =~ ';$'
      return pindent
    endif

    # Don't unindent if the previous line ended a block. This fixes a very
    # peculiar edge case where cindent would try to unindent after a block, but
    # only if it is the first expression within a `case` body:
    #
    #   case =>
    #           if (foo) {
    #                   bar();
    #           };
    #   | <-- cindent tries to unindent by one shiftwidth
    if pline =~ '};$' && cindent < pindent
      return pindent
    endif

    # If the previous line closed a set of parens, and cindent would try to
    # unindent more than one level, search for the previous `case` within the
    # same block. If that line didn't contain a `:` (excluding `::`), indent one
    # level more. This fixes an issue where cindent would unindent too far when
    # there was no `:` after a `case`:
    #
    #   case foo =>
    #           bar(baz,
    #                   quux);
    #   | <-- cindent tries to unindent by two shiftwidths
    if pline =~ ').*;$' && cindent < pindent - shiftwidth()
      const case = PrevMatchInBlock('^\s*case\>', plnum - 1)
      if case > 0 && GetTrimmedLine(case) !~ '\v%(%(::)*)@>:'
        return indent(case) + shiftwidth()
      endif
    endif
  endif

  # If the previous line ended with `=`, indent.
  if pline =~ '=$'
    return pindent + shiftwidth()
  endif

  # If the previous line opened an array literal, indent.
  if pline =~ '[$'
    return pindent + shiftwidth()
  endif

  # If the previous line started a binding expression, indent.
  if pline =~ '\v<%(const|def|let|type)$'
    return pindent + shiftwidth()
  endif

  # Indent continuation lines.
  if !TrailingParen(pline)
    # If this line closed an array and cindent would indent the same amount as
    # the previous line, unindent.
    if line =~ '^\s*]' && cindent == pindent
      return cindent - shiftwidth()
    endif

    # If the previous line closed an array literal, use the same indent. This
    # fixes an issue where cindent would try to indent an additional level after
    # an array literal containing indexing or slicing expressions, but only
    # inside a block:
    #
    #   export fn main() void = {
    #           const foo = [
    #                   bar[..4],
    #                   baz[..],
    #                   quux[1..],
    #           ];
    #                   | <-- cindent tries to indent by one shiftwidth
    if pline =~ '^\s*];$' && cindent > pindent
      return pindent
    endif

    # Don't indent any further if the previous line closed an enum, struct, or
    # union.
    if pline =~ '^\s*},$' && cindent > pindent
      return pindent
    endif

    # If the previous line started a binding expression, and the first binding
    # was on the same line, indent.
    if pline =~ '\v<%(const|def|let|type)>.{-}\=.*,$'
      return pindent + shiftwidth()
    endif

    # Use the original indentation after a single continuation line.
    if pline =~ '[,;]$' && ppline =~ '=$'
      return ppindent
    endif

    # Don't unindent within a binding expression.
    if pline =~ ',$' && ppline =~ '\v<%(const|def|let|type)$'
      return pindent
    endif
  endif

  # If the previous line had an unclosed `if` or `for` condition, indent twice.
  if pline =~ '\v<%(if|for)>'
    const cond = match(pline, '\v%(if|for)>[^(]*\zs\(')
    if cond != -1 && TrailingParen(pline, cond)
      return pindent + shiftwidth() * 2
    endif
  endif

  # Optionally indent unclosed `match` and `switch` conditions an extra level.
  if pline =~ '\v<%(match|switch)>'
    const cond = match(pline, '\v<%(match|switch)>[^(]*\zs\(')
    if cond != -1 && TrailingParen(pline, cond)
      return pindent + shiftwidth()
        * GetValue('hare_indent_match_switch', 1, 1, 2)
    endif
  endif

  # Otherwise, use the computed cindent.
  return cindent
enddef

# Returns a line, with any comments or whitespace trimmed from the end.
def GetTrimmedLine(lnum: number): string
  var line = getline(lnum)

  # Use syntax highlighting attributes when possible.
  if has('syntax_items')
    # If the last character is inside a comment, do a binary search to find the
    # beginning of the comment.
    const len = strlen(line)
    if synIDattr(synID(lnum, len, true), 'name') =~ 'Comment\|Todo'
      var min = 1
      var max = len
      while min < max
        const col = (min + max) / 2
        if synIDattr(synID(lnum, col, true), 'name') =~ 'Comment\|Todo'
          max = col
        else
          min = col + 1
        endif
      endwhile
      line = strpart(line, 0, min - 1)
    endif
    return substitute(line, '\s*$', '', '')
  endif

  # Otherwise, use a regex as a fallback.
  return substitute(line, '\s*//.*$', '', '')
enddef

# Returns the value of a configuration variable, clamped within the given range.
def GetValue(
  name: string,
  default: number,
  min: number = 0,
  max: number = default,
): number
  const n = get(b:, name, get(g:, name, default))
  return min([max, max([n, min])])
enddef

# Returns the line number of the previous match for a pattern within the same
# block. Returns 0 if nothing was found.
def PrevMatchInBlock(
  pattern: string,
  lnum: number,
  maxlines: number = 20,
): number
  var block = 0
  for n in range(lnum, lnum - maxlines, -1)
    if n < 1
      break
    endif

    const line = GetTrimmedLine(n)
    if line =~ '{$'
      block -= 1
      if block < 0
        break
      endif
    endif

    if line =~ pattern && block == 0
      return n
    endif

    if line =~ '^\s*}'
      block += 1
    endif
  endfor
  return 0
enddef

# Returns the line number and contents of the previous non-blank line, with any
# comments trimmed.
def PrevNonBlank(lnum: number): tuple<number, string>
  var plnum = prevnonblank(lnum)
  var pline = GetTrimmedLine(plnum)
  while plnum > 1 && pline !~ '[^[:blank:]]'
    plnum = prevnonblank(plnum - 1)
    pline = GetTrimmedLine(plnum)
  endwhile
  return (plnum, pline)
enddef

# Returns whether a line contains at least one unclosed `(`.
# XXX: Can still be fooled by parens inside rune and string literals.
def TrailingParen(line: string, start: number = 0): bool
  var total = 0
  for n in strpart(line, start)->filter((_, n) => n =~ '[()]')->reverse()
    if n == ')'
      total += 1
    else
      total -= 1
      if total < 0
        return true
      endif
    endif
  endfor
  return false
enddef

# vim: et sts=2 sw=2 ts=8 tw=80
