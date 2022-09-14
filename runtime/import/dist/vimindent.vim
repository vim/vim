vim9script

# Config {{{1

const TIMEOUT: number = 100

# Init {{{1
# These items must come first; we use them to define the next constants.
var cmds: list<string>
# CURLY_BLOCK {{{2

# TODO: `{` alone on a line is not necessarily the start of a block.
# It  could be  a dictionary  if the  previous line  ends with  a binary/ternary
# operator.   This  can  cause  an   issue  whenever  we  use  `CURLY_BLOCK`  or
# `LINE_CONTINUATION_AT_END`.
const CURLY_BLOCK: string = '^\s*{\s*$'
  .. '\|' .. '^.*\zs\s=>\s\+{\s*$'
  .. '\|' ..  '^\%(\s*\|.*|\s*\)\%(com\%[mand]\|au\%[tocmd]\).*\zs\s{\s*$'

# OPERATOR {{{2

const OPERATOR: string = '\%(^\|\s\)\%([-+*/%]\|\.\.\|||\|&&\|??\|?\|<<\|>>\|\%([=!]=\|[<>]=\=\|[=!]\~\|is\|isnot\)[?#]\=\)\%(\s\|$\)\@=\%(\s*[|<]\)\@!'
  # assignment operators
  .. '\|' .. '\s\%([-+*/%]\|\.\.\)\==\%(\s\|$\)\@='
  # support `:` when used inside conditional operator `?:`
  .. '\|' .. '\%(\s\|^\):\%(\s\|$\)'

# INLINE_COMMENT {{{2

const INLINE_COMMENT: string = '\%(#\|"\\\=\s\).*$'
# }}}2

# COMMENT {{{2

# Technically, `"\s` is wrong.
# In Vim9, a string might appear at the start of the line.
# To be sure, we should also inspect the syntax.
# But in practice, `"\s` at the start of a line is unlikely to be anything other
# than a legacy comment.
const COMMENT: string = $'^\s*{INLINE_COMMENT}'

# DICT_KEY_OR_FUNC_PARAM {{{2

const DICT_KEY_OR_FUNC_PARAM: string = '^\s*\%('
  .. '\%(\.\.\.\)\=\h[a-zA-Z0-9_]*'
  .. '\|'
  .. '\%(\w\|-\)\+'
  .. '\|'
  .. '"[^"]*"'
  .. '\|'
  .. "'[^']*'"
  .. '\|'
  .. '\[[^]]\+\]'
  .. '\)'
  .. ':\%(\s\|$\)'

# START_MIDDLE_END {{{2

const START_MIDDLE_END: dict<list<string>> = {
  if: ['if', 'el\%[se]\|elseif\=', 'en\%[dif]'],
  else: ['if', 'el\%[se]\|elseif\=', 'en\%[dif]'],
  elseif: ['if', 'el\%[se]\|elseif\=', 'en\%[dif]'],
  endif: ['if', 'el\%[se]\|elseif\=', 'en\%[dif]'],
  for: ['for', '', 'endfor\='],
  endfor: ['for', '', 'endfor\='],
  while: ['wh\%[ile]', '', 'endw\%[hile]'],
  endwhile: ['wh\%[ile]', '', 'endw\%[hile]'],
  try: ['try', 'cat\%[ch]\|fina\|finally\=', 'endt\%[ry]'],
  catch: ['try', 'cat\%[ch]\|finally\=', 'endt\%[ry]'],
  finally: ['try', 'cat\%[ch]\|finally\=', 'endt\%[ry]'],
  endtry: ['try', 'cat\%[ch]\|finally\=', 'endt\%[ry]'],
  def: ['\%(export\s\+\)\=def', '', 'enddef'],
  enddef: ['\%(export\s\+\)\=def', '', 'enddef'],
  function: ['fu\%[nction]', '', 'endf\%[unction]'],
  endfunction: ['fu\%[nction]', '', 'endf\%[unction]'],
  augroup: ['aug\%[roup]\%(\s\+[eE][nN][dD]\)\@!\s\+\S\+', '', 'aug\%[roup]\s\+[eE][nN][dD]'],
}->map((_, kwds: list<string>) =>
  kwds->map((_, kwd: string) => kwd == '' ? '' : $'\%(^\||\)\s*\%({kwd->printf('\C\<\%%(%s\)\>')}\)'))

# STARTS_WITH_LINE_CONTINUATION {{{2

const STARTS_WITH_LINE_CONTINUATION: string = '^\s*\%('
  .. '\\'
  .. '\|' .. '[#"]\\ '
  .. '\|' .. OPERATOR
  .. '\|' .. '->\s*\h'
  .. '\|' .. '\.\h'  # dict member
  .. '\|' .. '|'
  # TODO: `}` at the start of a line is not necessarily a continuation line.
  # Could be the end of a block.
  .. '\|' .. '[]})]'
  .. '\)'

# STARTS_WITH_RANGE {{{2

const STARTS_WITH_RANGE: string = '^\s*:\S'

# LINE_CONTINUATION_AT_END {{{2

const LINE_CONTINUATION_AT_END: string = '\%('
  .. ','
  .. '\|' .. OPERATOR
  .. '\|' .. '\s=>'
  .. '\|' .. '[[(]'
  # `{` is ambiguous.
  # It can be the start of a dictionary or a block.
  # We only want to match the former.
  .. '\|' .. $'^\%({CURLY_BLOCK}\)\@!.*\zs{{'
  .. '\)\s*\%(\s#.*\)\=$'

# STARTS_WITH_BACKSLASH {{{2

const STARTS_WITH_BACKSLASH: string = '^\s*\%(\\\|[#"]\\ \)'

# ASSIGNS_HEREDOC {{{2

const ASSIGNS_HEREDOC: string = '^\%(\s*\%(#\|"\s\)\)\@!.*\%('
  .. '\s=<<\s\+\%(\%(trim\|eval\)\s\)\{,2}\s*'
  .. '\)\zs\L\S*$'

# STARTS_BLOCK {{{2

# All of these will be used at the start of a line (or after a bar).
# NOTE: Don't replace `\%x28` with `(`.{{{
#
# Otherwise, the paren would be unbalanced which might cause syntax highlighting
# issues much  later in the  code of the  current script (sometimes,  the syntax
# highlighting plugin fails  to correctly recognize a heredoc which  is far away
# and/or not displayed because inside a fold).
# }}}
cmds =<< trim END
  if
  el\%[se]
  elseif\=
  for
  wh\%[ile]
  try
  cat\%[ch]
  fina\|finally\=
  fu\%[nction]\%x28\@!
  \%(export\s\+\)\=def
  aug\%[roup]\%(\s\+[eE][nN][dD]\)\@!\s\+\S\+
END
const STARTS_BLOCK: string = '^\s*\%(' .. cmds->join('\|') .. '\)\>'

# ENDS_BLOCK_OR_CLAUSE {{{2

cmds =<< trim END
  en\%[dif]
  el\%[se]
  endfor\=
  endw\%[hile]
  endt\%[ry]
  fina\|finally\=
  enddef
  endfu\%[nction]
  aug\%[roup]\s\+[eE][nN][dD]
END

# delimiter around `:catch` pattern (typically a slash)
var delimiter: string = '[^-+*/%.:# \t[:alnum:]\"|]\@=.\|->\@!\%(=\s\)\@!\|[+*/%]\%(=\s\)\@!'
const ENDS_BLOCK_OR_CLAUSE: string = '^\s*\%(' .. cmds->join('\|') .. $'\)\s*\%(|\|$\|{INLINE_COMMENT}\)'
  .. $'\|^\s*cat\%[ch]\%(\s\+\({delimiter}\).*\1\)\=\s*\%(|\|$\|{INLINE_COMMENT}\)'
  .. $'\|^\s*elseif\=\s\+\%({OPERATOR}\)\@!'

# ENDS_BLOCK {{{2

const ENDS_BLOCK: string = '^\s*\%('
  .. 'en\%[dif]'
  .. '\|' .. 'endfor\='
  .. '\|' .. 'endw\%[hile]'
  .. '\|' .. 'endt\%[ry]'
  .. '\|' .. 'enddef'
  .. '\|' .. 'endfu\%[nction]'
  .. '\|' .. 'aug\%[roup]\s\+[eE][nN][dD]'
  .. '\|' .. '[]})]'
  .. $'\)\s*\%(|\|$\|{INLINE_COMMENT}\)'

# CLOSING_BRACKET {{{2

const CLOSING_BRACKET: string = '[]})]'

# STARTS_WITH_CLOSING_BRACKET {{{2

const STARTS_WITH_CLOSING_BRACKET: string = '^\s*[]})]'

# STARTS_FUNCTION {{{2

const STARTS_FUNCTION: string = '^\s*\%(export\s\+\)\=def\>'

# OPENING_BRACKET_AT_END {{{2

const OPENING_BRACKET_AT_END: string = '[[{(]\s*$'
# }}}1
# Interface {{{1
export def Expr(lnum: number): number # {{{2
  # line which is indented
  var line_A: dict<any> = {text: getline(lnum), lnum: lnum}
  # line above, on which we'll base the indent of line A
  var line_B: dict<any>

  # at the start of a heredoc
  if line_A.text =~ ASSIGNS_HEREDOC && !exists('b:vimindent_heredoc')
    b:vimindent_heredoc = {
      startlnum: lnum,
      startindent: Indent(lnum),
      endmarker: line_A.text->matchstr(ASSIGNS_HEREDOC),
      trim: line_A.text =~ '.*\s\%(trim\%(\s\+eval\)\=\)\s\+\L\S*$',
    }
    # invalidate the cache so that it's not used for the next `=` normal command
    autocmd_add([{
      cmd: 'unlet! b:vimindent_heredoc',
      event: 'ModeChanged',
      group: 'VimIndentHereDoc',
      once: true,
      pattern: '*:n',
      replace: true,
    }])
  elseif exists('b:vimindent_heredoc')
    return line_A.text->HereDocIndent()
  endif

  # Don't move this block before the heredoc code.
  # A heredoc might be assigned on the very first line.
  if lnum == 1
    return 0
  endif

  if line_A.text =~ COMMENT
    return CommentIndent()
  endif

  line_B = PrevCodeLine(lnum)

  if line_B.text =~ CURLY_BLOCK
    return Indent(line_B.lnum) + shiftwidth()

  elseif line_A.text =~ STARTS_WITH_CLOSING_BRACKET
    var open_bracket: number = MatchingOpenBracket(line_A)
    if open_bracket <= 0
      return -1
    endif
    if getline(open_bracket) =~ STARTS_BLOCK
      return Indent(open_bracket) + 2 * shiftwidth()
    else
      return Indent(open_bracket)
    endif

  elseif line_A.text =~ STARTS_WITH_BACKSLASH
    if line_B.text =~ STARTS_WITH_BACKSLASH
      return Indent(line_B.lnum)
    else
      return Indent(line_B.lnum) + get(g:, 'vim_indent_cont', shiftwidth() * 3)
    endif

  elseif line_A.text =~ DICT_KEY_OR_FUNC_PARAM
      || line_B.text =~ DICT_KEY_OR_FUNC_PARAM
    var start: number = FindStart('(', '', ')')
    # function param
    if start > 0 && getline(start) =~ STARTS_FUNCTION
      return Indent(start) + 2 * shiftwidth()
    # dictionary key
    else
      start = FindStart('{', '', '}')
      var offset: number
      if line_A.text =~ DICT_KEY_OR_FUNC_PARAM
        # indent a dictionary key at the start of a line
        offset = shiftwidth()
      else
        # Indent a dictionary value at the start of a line twice.
        # Once for  the key relative  to the  dictionary start, another  for the
        # value relative to the key.
        offset = 2 * shiftwidth()
      endif
      return Indent(start) + offset
    endif

  elseif line_A.text =~ ENDS_BLOCK_OR_CLAUSE
      && !line_B->EndsWithLineContinuation()
    var kwd: string = GetBlockStartKeyword(line_A.text)
    if !START_MIDDLE_END->has_key(kwd)
      return -1
    endif
    var [start: string, middle: string, end: string] = START_MIDDLE_END[kwd]
    var block_start = FindStart(start, middle, end)
    if block_start > 0
      return Indent(block_start)
    else
      return -1
    endif
  endif

  var base_ind: number
  if line_A.text->IsFirstLineOfCommand(line_B)
    line_A.isfirst = true
    var [cmd: string, n: number] = line_B->FirstLinePreviousCommand()
    line_B = {text: cmd, lnum: n}
    base_ind = Indent(n)

    if line_B->EndsWithCurlyBlock()
        && !line_A->IsInThisBlock(line_B.lnum)
      return base_ind
    endif

  else
    line_A.isfirst = false
    base_ind = Indent(line_B.lnum)

    var line_C: dict<any> = PrevCodeLine(line_B.lnum)
    if !line_B.text->IsFirstLineOfCommand(line_C) || line_C.lnum <= 0
      # indent items in multiline nested list/dictionary
      if line_B->EndsWithOpeningBracket()
        return base_ind + shiftwidth()
      endif
      return base_ind
    endif
  endif

  var ind: number = base_ind + Offset(line_A, line_B)
  return [ind, 0]->max()
enddef

def g:GetVimIndent(): number # {{{2
  # for backward compatibility
  return Expr(v:lnum)
enddef
# }}}1
# Core {{{1
def Offset( # {{{2
    # we indent this line ...
    line_A: dict<any>,
    # ... relatively to this line
    line_B: dict<any>,
    ): number

  # increase indentation inside a block
  if line_B.text =~ STARTS_BLOCK || line_B->EndsWithCurlyBlock()
    # But don't indent if the line starting the block also closes it.
    if line_B->AlsoClosesBlock()
      return 0
    # Indent twice for  a line continuation in the block  header itself, so that
    # we can easily  distinguish the end of  the block header from  the start of
    # the block body.
    elseif line_B->EndsWithLineContinuation()
        && !line_A.isfirst
        || line_A.text =~ STARTS_WITH_LINE_CONTINUATION
      return 2 * shiftwidth()
    else
      return shiftwidth()
    endif

  # increase indentation of  a line if it's the continuation  of a command which
  # started on a previous line
  elseif !line_A.isfirst
      && (line_B->EndsWithLineContinuation()
      || line_A.text =~ STARTS_WITH_LINE_CONTINUATION)
    return shiftwidth()
  endif

  return 0
enddef

def HereDocIndent(line: string): number # {{{2
  # at the end of a heredoc
  if line =~ $'^\s*{b:vimindent_heredoc.endmarker}$'
    # `END` must be at the very start of the line if the heredoc is not trimmed
    if !b:vimindent_heredoc.trim
      return 0
    endif

    var ind: number = b:vimindent_heredoc.startindent
    # invalidate the cache so that it's not used for the next heredoc
    unlet! b:vimindent_heredoc
    return ind
  endif

  # In a non-trimmed heredoc, all of leading whitespace is semantic.
  # Leave it alone.
  if !b:vimindent_heredoc.trim
    return -1
  endif

  # In a trimmed heredoc, *some* of the leading whitespace is semantic.
  # We want to preserve  it, so we can't just indent  relative to the assignment
  # line.  That's because we're dealing with data, not with code.
  # Instead, we need  to compute by how  much the indent of  the assignment line
  # was increased  or decreased.   Then, we  need to apply  that same  change to
  # every line inside the body.
  var offset: number
  if !b:vimindent_heredoc->has_key('offset')
    var old_startindent: number = b:vimindent_heredoc.startindent
    var new_startindent: number = Indent(b:vimindent_heredoc.startlnum)
    offset = new_startindent - old_startindent

    # If all the non-empty lines in  the body have a higher indentation relative
    # to the assignment, there is no need to indent them more.
    # But if  at least one of  them does have  the same indentation level  (or a
    # lower one), then we want to indent it further (and the whole block with it).
    # This way,  we can clearly distinguish  the heredoc block from  the rest of
    # the code.
    var end: number = search($'^\s*{b:vimindent_heredoc.endmarker}$', 'nW')
    var should_indent_more: bool = range(v:lnum, end - 1)
      ->indexof((_, lnum: number): bool => Indent(lnum) <= old_startindent && getline(lnum) != '') >= 0
    if should_indent_more
      offset += shiftwidth()
    endif

    b:vimindent_heredoc.offset = offset
    b:vimindent_heredoc.startindent = new_startindent
  endif

  return [0, Indent(v:lnum) + b:vimindent_heredoc.offset]->max()
enddef

def CommentIndent(): number # {{{2
  var line_B: dict<any>
  line_B.lnum = prevnonblank(v:lnum - 1)
  line_B.text = getline(line_B.lnum)
  if line_B.text =~ COMMENT
    return Indent(line_B.lnum)
  endif

  var next: number = NextCodeLine()
  if next == 0
    return 0
  endif
  var ind: number = next->Expr()
  # The previous `Expr()` might have set `b:vimindent_heredoc`.
  # Setting  the variable  too early  can cause  issues (e.g.  when indenting  2
  # commented lines above  a heredoc).  Let's make sure the  variable is not set
  # too early.
  unlet! b:vimindent_heredoc
  if getline(next) =~ ENDS_BLOCK
    return ind + shiftwidth()
  else
    return ind
  endif
enddef
# }}}1
# Util {{{1
def Indent(lnum: number): number # {{{2
  if lnum <= 0
    return 0
  endif
  return indent(lnum)
enddef

def PrevCodeLine(lnum: number): dict<any> # {{{2
  var n: number = prevnonblank(lnum - 1)
  var line: string = getline(n)
  while line =~ COMMENT && n > 1
    n = prevnonblank(n - 1)
    line = getline(n)
  endwhile
  # If we get back to the first line, we return 1 no matter what; even if it's a
  # commented line.   That should not  cause an issue  though.  We just  want to
  # avoid a  commented line above which  there is a  line of code which  is more
  # relevant.  There is nothing above the first line.
  return {lnum: n, text: line}
enddef

def NextCodeLine(): number # {{{2
  var last: number = line('$')
  if v:lnum == last
    return 0
  endif

  var lnum: number = v:lnum + 1
  while lnum <= last
    var line: string = getline(lnum)
    if line != '' && line !~ COMMENT
      return lnum
    endif
    ++lnum
  endwhile
  return 0
enddef

def FindStart( # {{{2
    start: string,
    middle: string,
    end: string,
    ): number

  return Find(start, middle, end, 'bnW')
enddef

def FindEnd( # {{{2
    start: string,
    middle: string,
    end: string,
    stopline = 0,
    ): number
  return Find(start, middle, end, 'nW', stopline)
enddef

def Find( # {{{2
    start: string,
    middle: string,
    end: string,
    flags: string,
    stopline = 0,
    ): number

  var s: string = start
  var e: string = end
  if start == '[' || start == ']'
    s = s->escape('[]')
  endif
  if end == '[' || end == ']'
    e = e->escape('[]')
  endif
  return searchpair(s, middle, e, flags, (): bool => InCommentOrString(), stopline, TIMEOUT)
enddef

def GetBlockStartKeyword(line: string): string # {{{2
  var kwd: string = line->matchstr('\l\+')
  # Need to call `fullcommand()` from legacy context:
  #     :vim9cmd echo fullcommand('end')
  #     E1065: Command cannot be shortened: end
  return FullCommand(kwd)
enddef

function FullCommand(kwd)
  return fullcommand(a:kwd)
endfunction

def MatchingOpenBracket(line: dict<any>): number # {{{2
  var end: string = line.text->matchstr(CLOSING_BRACKET)
  var start: string = {']': '[', '}': '{', ')': '('}[end]
  cursor(line.lnum, 1)
  return FindStart(start, '', end)
enddef

def FirstLinePreviousCommand(line: dict<any>): list<any> # {{{2
  var line_B: dict<any> = line

  while line_B.lnum > 1
    var line_above: dict<any> = PrevCodeLine(line_B.lnum)

    if line_B.text =~ STARTS_WITH_CLOSING_BRACKET
      var n: number = MatchingOpenBracket(line_B)

      if n <= 0
        break
      endif

      line_B.lnum = n
      line_B.text = getline(line_B.lnum)
      continue

    elseif line_B.text->IsFirstLineOfCommand(line_above)
      break
    endif

    line_B = line_above
  endwhile

  return [line_B.text, line_B.lnum]
enddef

def AlsoClosesBlock(line_B: dict<any>): bool # {{{2
  # We know that `line_B` opens a block.
  # Let's see if it also closes that block.
  var kwd: string = GetBlockStartKeyword(line_B.text)
  if !START_MIDDLE_END->has_key(kwd)
    return false
  endif

  var [start: string, middle: string, end: string] = START_MIDDLE_END[kwd]
  var pos: list<number> = getcurpos()
  cursor(line_B.lnum, 1)
  var block_end: number = FindEnd(start, middle, end, line_B.lnum)
  setpos('.', pos)

  return block_end > 0
enddef

def EndsWithLineContinuation(line: dict<any>): bool # {{{2
  # Technically, that's wrong.  A  line might start with a range  and end with a
  # line continuation symbol.  But it's unlikely.  And it's useful to assume the
  # opposite because it  prevents us from conflating a mark  with an operator or
  # the start of a list:
  #
  #              not a comparison operator
  #              v
  #     :'< mark <
  #     :'< mark [
  #              ^
  #              not the start of a list
  if line.text =~ STARTS_WITH_RANGE
    return false
  endif

  #                    that's not an arithmetic operator
  #                    v
  #     catch /pattern /
  if line.text =~ $'\<catch\s\+\({delimiter}\)[^\1]*\1\s*$'
    return false
  endif

  return NonCommentedMatchAtEnd(line, LINE_CONTINUATION_AT_END)
enddef

def EndsWithCurlyBlock(line: dict<any>): bool # {{{2
  return NonCommentedMatchAtEnd(line, CURLY_BLOCK)
enddef

def EndsWithOpeningBracket(line: dict<any>): bool # {{{2
  return NonCommentedMatchAtEnd(line, OPENING_BRACKET_AT_END)
enddef

def NonCommentedMatchAtEnd(line: dict<any>, pat: string): bool # {{{2
  var pos: list<number> = getcurpos()
  cursor(line.lnum, 1)
  var match_lnum: number = search(pat, 'cnW', line.lnum, TIMEOUT, (): bool => InCommentOrString())
  setpos('.', pos)
  return match_lnum > 0
enddef

def IsInThisBlock(line_A: dict<any>, lnum: number): bool # {{{2
  var pos: list<number> = getcurpos()
  cursor(lnum, [lnum, '$']->col())
  var end: number = FindEnd('{', '', '}')
  setpos('.', pos)

  return line_A.lnum <= end
enddef

def IsFirstLineOfCommand(line_A: string, line_B: dict<any>): bool # {{{2
  if line_A =~ STARTS_WITH_RANGE
    return true
  endif

  var line_A_is_good: bool = line_A !~ COMMENT
    && line_A !~ DICT_KEY_OR_FUNC_PARAM
    && line_A !~ STARTS_WITH_LINE_CONTINUATION
  var line_B_is_good: bool = line_B.text !~ DICT_KEY_OR_FUNC_PARAM
    && !line_B->EndsWithLineContinuation()

  return line_A_is_good && line_B_is_good
enddef

def IsBlock(lnum: number): bool # {{{2
  var line: string = getline(lnum)
  if line =~ '^\s*{\s*$' && !PrevCodeLine(lnum)->EndsWithLineContinuation()
    return true
  endif

  return {text: line, lnum: lnum}->EndsWithCurlyBlock()
enddef

def InCommentOrString(lnum = line('.'), col = col('.')): bool # {{{2
  if !has('syntax_items')
    return getline(lnum) =~ COMMENT
  endif

  for synID: number in synstack(lnum, col)
    if synIDattr(synID, 'name') =~ '\ccomment\|string\|heredoc'
      return true
    endif
  endfor

  return false
enddef
# }}}1
# vim:sw=2
