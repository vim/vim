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
  .. '\|' .. '^\%(\s*\|.*|\s*\)com\%[mand].*\s{\s*$'
  .. '\|' .. '^\%(\s*\|.*|\s*\)au\%[tocmd].*\s{\s*$'

# OPERATOR {{{2

const OPERATOR: string = '\%(^\|\s\)\%([-+*/%]\|\.\.\|||\|&&\|??\|?\|<<\|>>\|\%([=!]=\|[<>]=\=\|[=!]\~\|is\|isnot\)[?#]\=\)\%(\s\|$\)\@=\%(\s*[|<]\)\@!'
  # assignment operators
  .. '\|' .. '\s\%([-+*/%]\|\.\.\)\==\%(\s\|$\)\@='
  # support `:` when used inside conditional `?:` operator
  .. '\|' .. '\%(\s\|^\):\%(\s\|$\)'
# }}}2

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
}->map((_, kwds: list<string>) => kwds->map((_, kwd: string) => kwd->printf('\C\<%s\>')))

# LINE_CONTINUATION_AT_START {{{2

const LINE_CONTINUATION_AT_START: string = '^\s*\%('
  .. '\\'
  .. '\|' .. '[#"]\\ '
  .. '\|' .. OPERATOR
  .. '\|' .. '->\s*\h'
  .. '\|' .. '\.\h' # dict member
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

const STARTS_WITH_BACKSLASH: string = '^\s*\\'

# DECLARES_HEREDOC {{{2

const DECLARES_HEREDOC: string = '^\%(\s*\%(#\|"\s\)\)\@!.*\%('
          .. '\s=<<\s\+\%(trim\s\)\=\s*'
  .. '\|' .. '\s=<<\s\+\%(.*\<eval\>\)\@=\%(\%(trim\|eval\)\s\)\{1,2}\s*'
  .. '\)\zs\L\S*$'

# STARTS_BLOCK {{{2

# All of these will be used at the start of a line (or after a bar).
# NOTE: Don't replace `\%x28` with `(`.{{{
#
# Otherwise, the paren would be unbalanced which might cause syntax highlighting
# issues much  later in the  code of the  current script (sometimes,  the syntax
# highlighting plugin fails  to correctly recognize a heredoc which  is far away
# and/or not displayed because inside a fold).
#}}}
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

# Shortest non-ambiguous end of block commands.
cmds =<< trim END
  endif
  else
  elseif
  endfor
  endwhile
  endtry
  catch
  finally
  enddef
  endfu
  aug\%[roup]\s\+[eE][nN][dD]
END
const ENDS_BLOCK: string = '^\s*\%(' .. cmds->join('\|') .. '\)'

# CLOSING_BRACKET {{{2

const CLOSING_BRACKET: string = '[]})]'

# STARTS_WITH_CLOSING_BRACKET {{{2

const STARTS_WITH_CLOSING_BRACKET: string = '^\s*[]})]'

# IS_SINGLE_OPEN_BRACKET {{{2

const IS_SINGLE_OPEN_BRACKET: string = '^\s*[[{(]\s*$'
# }}}1
# Interface {{{1
export def Expr(): number #{{{2
  if v:lnum == 1
    return 0
  endif

  var line_A: dict<any> = {text: getline(v:lnum), lnum: v:lnum}
  var line_B: dict<any>

  # at the start of a heredoc
  if line_A.text =~ DECLARES_HEREDOC
    b:vimindent_heredoc = {
      lnum: v:lnum,
      startindent: indent(v:lnum),
      endmarker: line_A.text->matchstr(DECLARES_HEREDOC),
    }
    autocmd_add([{
      cmd: 'unlet! b:vimindent_heredoc',
      event: 'ModeChanged',
      group: 'VimIndentHereDoc',
      once: true,
      pattern: '*:n',
      replace: true,
    }])
  elseif exists('b:vimindent_heredoc') && !empty(b:vimindent_heredoc)
    return line_A.text->HereDocIndent()
  endif

  line_B.lnum = prevnonblank(v:lnum - 1)
  line_B.text = getline(line_B.lnum)

  var base_ind: number

  if line_B.text =~ IS_SINGLE_OPEN_BRACKET
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

def g:GetVimIndent(): number #{{{2
# for backward compatibility
  return Expr(v:lnum)
enddef
# }}}1
# Core {{{1
def Offset( #{{{2
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
    if line_B->ClosesBlock(line_A)
      return 0
    # Do it twice for a line continuation in the block header itself, so that we
    # can easily distinguish the  end of the block header from  the start of the
    # block body.
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

def HereDocIndent(line: string): number #{{{2
  # at the end of a heredoc
  if line =~ $'^\s*{b:vimindent_heredoc.endmarker}$'
    var ind: number = b:vimindent_heredoc.startindent
    unlet! b:vimindent_heredoc
    return ind
  # inside a heredoc
  else
    return b:vimindent_heredoc.startindent + shiftwidth()
  endif
enddef
# }}}1
# Util {{{1
def FindStart( #{{{2
    start: string,
    middle: string,
    end: string,
    ): number

  return searchpair(start->escape('[]'), middle, end->escape('[]'),
    'bnW', (): bool => InCommentOrString(), 0, TIMEOUT)
enddef

def GetBlockStartKeyword(line: string): string #{{{2
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

def MatchingOpenBracket(line: dict<any>): number #{{{2
  var end: string = line.text->matchstr(CLOSING_BRACKET)
  var start: string = {']': '[', '}': '{', ')': '('}[end]
  cursor(line.lnum, 1)
  return FindStart(start, '', end)
enddef

def FirstLinePreviousCommand(line: dict<any>): list<any> #{{{2
  var text: string = line.text
  var lnum: number = line.lnum

  while lnum > 1
    var line_above: string = getline(lnum - 1)

    if text =~ STARTS_WITH_CLOSING_BRACKET
      var n: number = MatchingOpenBracket({text: text, lnum: lnum})

      if n <= 0 || text =~ '^\s*}' && IsBlock(n)
        break
      endif

      lnum = n
      text = getline(lnum)
      continue

    elseif text->IsFirstLineOfCommand({text: line_above, lnum: lnum - 1})
      break
    endif

    --lnum
    text = line_above
  endwhile

  return [text, lnum]
enddef

def ClosesBlock(line_A: dict<any>, line_B: dict<any>): bool #{{{2
  var kwd: string = GetBlockStartKeyword(line_A.text)
  if !START_MIDDLE_END->has_key(kwd)
    return false
  endif

  var [start: string, middle: string, end: string] = START_MIDDLE_END[kwd]
  var block_end: number = searchpair(start, middle, end,
    'nW', (): bool => InCommentOrString(), 0, TIMEOUT)

  return block_end < line_B.lnum
enddef

def HasLineContinuationAtEnd(line: dict<any>): bool #{{{2
  var col: number = line.text->matchend(LINE_CONTINUATION_AT_END)
  return col >= 0 && !InCommentOrString(line.lnum, col)
enddef

def IsFirstLineOfCommand(line_A: string, line_B: dict<any>): bool #{{{2
  return line_A !~ KEY_IN_LITERAL_DICT
    && line_A !~ LINE_CONTINUATION_AT_START
    && !line_B->HasLineContinuationAtEnd()
enddef

def IsBlock(lnum: number): bool #{{{2
  var line: string = getline(lnum)
  if line =~ '^\s*{\s*$'
      && prevnonblank(lnum - 1)->getline() !~ LINE_CONTINUATION_AT_END
    return true
  endif

  return line =~ '=>\s\+{\s*$'
    || line =~ '^\%(\s*\|.*|\s*\)com\%[mand].*\s{\s*$'
    || line =~ '^\%(\s*\|.*|\s*\)au\%[tocmd].*\s{\s*$'
enddef

def InCommentOrString(lnum = line('.'), col = col('.')): bool #{{{2
  if !has('syntax_items')
    if getline(lnum) =~ '^\s*#'
      return true
    endif
    return false
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
