vim9script

# Config {{{1

const TIMEOUT: number = 50

# Init {{{1
# These items must come first; we use them to define the next constants.
var cmds: list<string>
# CURLY_BLOCK {{{2

# TODO: `{` alone on a line is not necessarily the start of a block.
# It  could be  a dictionary  if the  previous line  ends with  a binary/ternary
# operator.   This  can  cause  an   issue  whenever  we  use  `CURLY_BLOCK`  or
# `LINE_CONTINUATION_AT_END`.
const CURLY_BLOCK: string = '^\s*{\s*$'
  .. '\|' .. '^.*\s=>\s\+{\s*$'
  .. '\|' ..  '^\%(\s*\|.*|\s*\)\%(com\%[mand]\|au\%[tocmd]\).*\s{\s*$'

# OPERATOR {{{2

const OPERATOR: string = '\%(^\|\s\)\%([-+*/%]\|\.\.\|||\|&&\|??\|?\|<<\|>>\|\%([=!]=\|[<>]=\=\|[=!]\~\|is\|isnot\)[?#]\=\)\%(\s\|$\)\@=\%(\s*[|<]\)\@!'
  # assignment operators
  .. '\|' .. '\s\%([-+*/%]\|\.\.\)\==\%(\s\|$\)\@='
  # support `:` when used inside conditional operator `?:`
  .. '\|' .. '\%(\s\|^\):\%(\s\|$\)'
# }}}2

# COMMENT {{{2

# Technically, `"\s` is wrong.
# In Vim9, a string might appear at the start of the line.
# To be sure, we should also inspect the syntax.
# But in practice, `"\s` at the start of a line is unlikely to be anything other
# than a legacy comment.
const COMMENT: string = '^\s*\%(#\|"\\\=\s\)'

# KEY_IN_LITERAL_DICT {{{2

const KEY_IN_LITERAL_DICT: string = '^\s*\%(\w\|-\)\+:\%(\s\|$\)'

# START_MIDDLE_END {{{2

const START_MIDDLE_END: dict<list<string>> = {
  if: ['if', 'else\|elseif', 'endif'],
  else: ['if', 'else\|elseif', 'endif'],
  elseif: ['if', 'else\|elseif', 'endif'],
  endif: ['if', 'else\|elseif', 'endif'],
  for: ['for', '', 'endfor'],
  endfor: ['for', '', 'endfor'],
  while: ['while', '', 'endwhile'],
  endwhile: ['while', '', 'endwhile'],
  try: ['try', 'catch\|finally', 'endtry'],
  catch: ['try', 'catch\|finally', 'endtry'],
  finally: ['try', 'catch\|finally', 'endtry'],
  endtry: ['try', 'catch\|finally', 'endtry'],
  def: ['def', '', 'enddef'],
  enddef: ['def', '', 'enddef'],
  function: ['fu\%[nction]', '', 'endf\%[unction]'],
  endfunction: ['fu\%[nction]', '', 'endf\%[unction]'],
  augroup: ['aug\%[roup]\%(\s\+[eE][nN][dD]\)\@!\s\+\S\+', '', 'aug\%[roup]\s\+[eE][nN][dD]'],
}->map((_, kwds: list<string>) => kwds->map((_, kwd: string) => $'\%(^\||\)\s*\%({kwd->printf('\C\<\%%(%s\)\>')}\)'))

# LINE_CONTINUATION_AT_START {{{2

const LINE_CONTINUATION_AT_START: string = '^\s*\%('
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

# LINE_CONTINUATION_AT_END {{{2

const LINE_CONTINUATION_AT_END: string = '\%('
  .. ','
  .. '\|' .. OPERATOR
  .. '\|' .. '\s=>'
  .. '\|' .. '[[(]'
  # `{` is ambiguous.
  # It can be the start of a dictionary or a block.
  # We only want to match the former.
  .. '\|' .. $'^\%({CURLY_BLOCK}\)\@!.*{{'
  .. '\)\s*$'

# STARTS_WITH_BACKSLASH {{{2

const STARTS_WITH_BACKSLASH: string = '^\s*\%(\\\|"\\\s\)'

# DECLARES_HEREDOC {{{2

const DECLARES_HEREDOC: string = '^\%(\s*\%(#\|"\s\)\)\@!.*\%('
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
  else
  elseif
  for
  while
  try
  catch
  finally
  fu\%[nction]\%x28\@!
  \%(export\s\+\)\=def
  aug\%[roup]\%(\s\+[eE][nN][dD]\)\@!\s\+\S\+
END
const STARTS_BLOCK: string = '^\s*\%(' .. cmds->join('\|') .. '\)\>'

# ENDS_BLOCK {{{2

cmds =<< trim END
  en\%[dif]
  el\%[se]
  elseif\=
  endfor\=
  endw\%[hile]
  endt\%[ry]
  cat\%[ch]
  fina\|finally\=
  enddef
  endfu\%[nction]
  aug\%[roup]\s\+[eE][nN][dD]
END
const ENDS_BLOCK: string = '^\s*\%(' .. cmds->join('\|') .. '\)\>'

# CLOSING_BRACKET {{{2

const CLOSING_BRACKET: string = '[]})]'

# STARTS_WITH_CLOSING_BRACKET {{{2

const STARTS_WITH_CLOSING_BRACKET: string = '^\s*[]})]'

# IS_SINGLE_OPEN_BRACKET {{{2

const IS_SINGLE_OPEN_BRACKET: string = '^\s*[[{(]\s*$'
# }}}1
# Interface {{{1
export def Expr(): number # {{{2
  var line_A: dict<any> = {text: getline(v:lnum), lnum: v:lnum}
  var line_B: dict<any>

  # at the start of a heredoc
  if line_A.text =~ DECLARES_HEREDOC
    b:vimindent_heredoc = {
      startlnum: v:lnum,
      startindent: indent(v:lnum),
      endmarker: line_A.text->matchstr(DECLARES_HEREDOC),
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
  if v:lnum == 1
    return 0
  endif

  line_B.lnum = prevnonblank(v:lnum - 1)
  line_B.text = getline(line_B.lnum)

  var base_ind: number

  if line_A.text =~ COMMENT && line_B.text =~ COMMENT
    return indent(line_B.lnum)

  elseif line_B.text =~ IS_SINGLE_OPEN_BRACKET
    return indent(line_B.lnum) + shiftwidth()

  elseif line_A.text =~ STARTS_WITH_CLOSING_BRACKET
    var open_bracket: number = MatchingOpenBracket(line_A)
    if open_bracket <= 0
      return -1
    endif
    if getline(open_bracket) =~ STARTS_BLOCK
      return indent(open_bracket) + 2 * shiftwidth()
    else
      return indent(open_bracket)
    endif

  elseif line_A.text =~ ENDS_BLOCK
    var kwd: string = GetBlockStartKeyword(line_A.text)
    var [start: string, middle: string, end: string] = START_MIDDLE_END[kwd]
    var block_start = FindStart(start, middle, end)
    if block_start > 0
      return indent(block_start)
    else
      return -1
    endif

  elseif line_A.text->IsFirstLineOfCommand(line_B)
    line_A.isfirst = true
    var [cmd: string, lnum: number] = line_B->FirstLinePreviousCommand()
    line_B = {text: cmd, lnum: lnum}
    base_ind = indent(lnum)

  else
    line_A.isfirst = false
    base_ind = indent(line_B.lnum)

    var line_C: dict<any>
    line_C.lnum = prevnonblank(line_B.lnum - 1)
    line_C.text = getline(line_C.lnum)

    if !line_B.text->IsFirstLineOfCommand(line_C) || line_C.lnum <= 0
      return base_ind
    endif
  endif

  var ind: number = base_ind + Offset(line_A, line_B)
  return [ind, 0]->max()
enddef

def g:GetVimIndent(): number # {{{2
  # for backward compatibility
  return Expr()
enddef
# }}}1
# Core {{{1
def Offset( # {{{2
    # we indent this line ...
    line_A: dict<any>,
    # ... relatively to this line
    line_B: dict<any>,
    ): number

  # to be backward compatible
  if line_A.text =~ STARTS_WITH_BACKSLASH
    return get(g:, 'vim_indent_cont', shiftwidth() * 3)

  # increase indentation inside a block
  elseif line_B.text =~ STARTS_BLOCK || line_B.text =~ CURLY_BLOCK
    # But don't indent if the line starting the block also closes it.
    if line_B->AlsoClosesBlock()
      return 0
    # Indent twice for  a line continuation in the block  header itself, so that
    # we can easily  distinguish the end of  the block header from  the start of
    # the block body.
    elseif line_B->HasLineContinuationAtEnd()
        && !line_A.isfirst
        || line_A.text =~ LINE_CONTINUATION_AT_START
      return 2 * shiftwidth()
    else
      return shiftwidth()
    endif

  # increase indentation of  a line if it's the continuation  of a command which
  # started on a previous line
  elseif !line_A.isfirst
      && (line_B->HasLineContinuationAtEnd()
      || line_A.text =~ LINE_CONTINUATION_AT_START)
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

  # To preserve relative line indentations in  the body of a trimmed heredoc, we
  # need to  compute the  offset which was  applied to the  indent level  of the
  # declaration line.
  var offset: number
  if !b:vimindent_heredoc->has_key('offset')
    var old_startindent: number = b:vimindent_heredoc.startindent
    var new_startindent: number = indent(b:vimindent_heredoc.startlnum)
    offset = new_startindent - old_startindent

    # Indent the body relatively to the declaration when it makes sense.
    # That is, if  we can find at  least one line in the  body whose indentation
    # level was equal (or lower) than the declaration.
    var end: number = search($'^\s*{b:vimindent_heredoc.endmarker}$', 'nW')
    var should_indent_more: bool = getline(v:lnum, end - 1)
      ->map((_, lnum: number) => indent(lnum))
      ->indexof((_, ind: number) => ind <= old_startindent) >= 0
    if should_indent_more
      offset += shiftwidth()
    endif

    b:vimindent_heredoc.offset = offset
    b:vimindent_heredoc.startindent = new_startindent
  endif

  return b:vimindent_heredoc.startindent + b:vimindent_heredoc.offset
enddef
# }}}1
# Util {{{1
def FindStart( # {{{2
    start: string,
    middle: string,
    end: string,
    ): number

  return searchpair(start->escape('[]'), middle, end->escape('[]'),
    'bnW', (): bool => InCommentOrString(), 0, TIMEOUT)
enddef

def GetBlockStartKeyword(line: string): string # {{{2
  var kwd: string = line->matchstr('\l\+')
  if kwd =~ '^aug'
    kwd = 'augroup'
  elseif kwd =~ '^fu'
    kwd = 'function'
  elseif kwd =~ '^endfu'
    kwd = 'endfunction'
  endif
  return kwd
enddef

def MatchingOpenBracket(line: dict<any>): number # {{{2
  var end: string = line.text->matchstr(CLOSING_BRACKET)
  var start: string = {']': '[', '}': '{', ')': '('}[end]
  cursor(line.lnum, 1)
  return FindStart(start, '', end)
enddef

def FirstLinePreviousCommand(line_A: dict<any>): list<any> # {{{2
  var line_B: dict<any> = line_A

  while line_B.lnum > 1
    var line_above: string = getline(line_B.lnum - 1)

    if line_B.text =~ STARTS_WITH_CLOSING_BRACKET
      var n: number = MatchingOpenBracket(line_B)

      if n <= 0 || line_B.text =~ '^\s*}' && IsBlock(n)
        break
      endif

      line_B.lnum = n
      line_B.text = getline(line_B.lnum)
      continue

    elseif line_B.text->IsFirstLineOfCommand({text: line_above, lnum: line_B.lnum - 1})
      break
    endif

    --line_B.lnum
    line_B.text = line_above
  endwhile

  return [line_B.text, line_B.lnum]
enddef

def AlsoClosesBlock(line_B: dict<any>): bool # {{{2
  # We know that `line_B` opens a block.
  # Let's see if it also closes that block.
  # It does if  we can't find the block  end after where we are  (which is right
  # below `line_B`).
  var kwd: string = GetBlockStartKeyword(line_B.text)
  if !START_MIDDLE_END->has_key(kwd)
    return false
  endif

  var [start: string, middle: string, end: string] = START_MIDDLE_END[kwd]
  var block_end: number = searchpair(start, middle, end,
    'nW', (): bool => InCommentOrString(), 0, TIMEOUT)

  return block_end <= 0
enddef

def HasLineContinuationAtEnd(line: dict<any>): bool # {{{2
  var col: number = line.text->matchend(LINE_CONTINUATION_AT_END)
  return col >= 0 && !InCommentOrString(line.lnum, col)
enddef

def IsFirstLineOfCommand(line_A: string, line_B: dict<any>): bool # {{{2
  return line_A !~ KEY_IN_LITERAL_DICT
    && line_A !~ LINE_CONTINUATION_AT_START
    && !line_B->HasLineContinuationAtEnd()
enddef

def IsBlock(lnum: number): bool # {{{2
  var line: string = getline(lnum)
  if line =~ '^\s*{\s*$'
      && prevnonblank(lnum - 1)->getline() !~ LINE_CONTINUATION_AT_END
    return true
  endif

  return line =~ CURLY_BLOCK
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
