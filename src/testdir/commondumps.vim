vim9script

# Generate the small ASCII letters (for marking).
def SmallAlphaLetters(): list<string>
  return range(97, (97 + 25))->map((_: number, n: number) => nr2char(n, true))
enddef

# Query the paired position for the cursor line and use it, if available, for
# mark "`".
def TryChangingLastJumpMark(marks: dict<list<number>>)
  const pos: list<number> = get(marks, line('.'), [])
  if !empty(pos)
    setpos("'`", pos)
  endif
enddef

# Fold the difference part and the bottom part when the top and the bottom
# parts are identical; otherwise, create a fold for the difference part and
# manage marks for disparate lines: dynamically set mark "`" to pair such
# lines and initially set "`a", "`b", etc. marks for difference part lines.
def FoldAndMarkDumpDiffParts(letters: list<string>)
  defer call('setpos', ['.', getpos('.')])
  # Shape the pattern after get_separator() from "terminal.c".
  const separator: string = '^\(=\+\)\=\s\S.*\.dump\s\1$'
  const start_lnum: number = search(separator, 'eW', (line('$') / 2))
  if start_lnum > 0
    const end_lnum: number = search(separator, 'eW')
    if end_lnum > 0
      # Collect [0, line_nr, col_nr, 0] lists (matching the first non-blank
      # column) from the difference part "bs" and assemble corresponding lists
      # for the top part "as" and the bottom part "cs".
      const parts: list<list<list<number>>> =
				getline((start_lnum + 1), (end_lnum - 1))
	  ->map((idx: number, s: string) =>
	      [matchlist(s, '\(^\s*\S\)')->get(1, '')->strwidth(), idx])
	  ->reduce(((as: list<number>, bs: list<number>, cs: list<number>) =>
	      (xs: list<list<list<number>>>, pair: list<number>) => {
		  const col_nr: number = pair[0]
		  if col_nr != 0
		    const idx: number = pair[1]
		    xs[0]->add([0, get(as, idx, as[-1]), col_nr, 0])
		    xs[1]->add([0, bs[idx], col_nr, 0])
		    xs[2]->add([0, get(cs, idx, cs[-1]), col_nr, 0])
		  endif
		  return xs
		})(range(1, (start_lnum - 1)),
		    range((start_lnum + 1), (end_lnum - 1)),
		    range((end_lnum + 1), line('$'))),
	      [[], [], []])
      if empty(parts[1])
	setlocal foldenable foldmethod=manual
	exec 'normal ' .. start_lnum .. 'GzfG'
      else
	setlocal nofoldenable foldmethod=manual
	exec ':' .. start_lnum .. ',' .. end_lnum .. 'fold'
	var marks: dict<list<number>> = {}
	for idx in range(parts[1]->len())
	  if !empty(letters)
	    setpos(("'" .. remove(letters, 0)), parts[1][idx])
	  endif
	  # Point "bs" to "cs", "cs" to "as", "as" to "cs".
	  marks[parts[1][idx][1]] = parts[2][idx]
	  marks[parts[2][idx][1]] = parts[0][idx]
	  marks[parts[0][idx][1]] = parts[2][idx]
	endfor
	autocmd_add([{
	  replace:	true,
	  group:	'viewdumps',
	  event:	'CursorMoved',
	  bufnr:	bufnr(),
	  cmd:		printf('TryChangingLastJumpMark(%s)', string(marks)),
	}])
      endif
    endif
  endif
enddef

# See below on how to configure the git difftool extension

# Extend "git difftool" with the capability for loading screendump files.
if v:progname =~? '\<g\=vimdiff$'
  # Let "(g)vimdiff" render other files.
  if [argv(0), argv(1)]
      ->filter((_: number, fname: string) =>
	  fname =~? '^\%(/dev/null\|.\+\.dump\)$')
      ->len() == 2
    try
      if argv(0) ==? '/dev/null'
	term_dumpload(argv(1))
      elseif argv(1) ==? '/dev/null'
	term_dumpload(argv(0))
      else
	term_dumpdiff(argv(0), argv(1))
	FoldAndMarkDumpDiffParts(SmallAlphaLetters())
      endif
    finally
      silent bwipeout 1 2
    endtry
  endif
  # Always stop from further sourcing this script for "(g)vimdiff".
  finish
endif

# CONSIDER ALTERNATIVES FOR ENABLING THE ABOVE EXTENSION.
#
# For convenience, it is assumed that there is a defined "$VIM_FORK_PATHNAME"
# environment variable holding an absolute pathname for the root directory of
# this repository.
#
#
# A. USE Git FOR CONFIGURATION.
#
# Define the following Git variables with "git config --edit --local" (where
# the "vimdumps" name is arbitrary):
#
# ------------------------------------------------------------------------------
# [diff]
#	tool = vimdumps
# [difftool.vimdumps]
#	cmd = vimdiff -S "${VIM_FORK_PATHNAME:?}"/src/testdir/commondumps.vim -o -- "$LOCAL" "$REMOTE"
# ------------------------------------------------------------------------------
#
# Rendered screendump files (among other files) between revisions can now be
# compared, two at a time, by using "git difftool", e.g.:
#	git difftool 50423ab8~1 50423ab8
#	git difftool 50423ab8~1 50423ab8 -- '**/*.dump'
#
# The raw files can also be examined:
#	:all
#
#
# B. USE Bash FOR CONFIGURATION (on Debian GNU/Linux).
#
# 1. Make an alias that sources this file, e.g.:
#	alias git_vimdiff="git difftool -x 'vimdiff -S "${VIM_FORK_PATHNAME:?}"/vim/src/testdir/commondumps.vim -o --'"
#
# 2. Enable programmable completion for the alias, e.g.:
#	cat ~/.local/share/bash-completion/completions/git_vimdiff
#
# ------------------------------------------------------------------------------
# ## Consider (un)setting "$BASH_COMPLETION_USER_DIR" and/or "$XDG_DATA_HOME" so
# ## that this file can be found and sourced; look for these variables in the
# ## "/usr/share/bash-completion/bash_completion" script.
# ##
# ## Look for __git_complete() examples in the header comment of the sourced
# ## "/usr/share/bash-completion/completions/git" script.
# [ -r /usr/share/bash-completion/completions/git ] &&
# . /usr/share/bash-completion/completions/git &&
# __git_complete git_vimdiff _git_difftool
# ------------------------------------------------------------------------------
#
# Rendered screendump files (among other files) between revisions can now be
# compared, two at a time, by using the alias, e.g.:
#	git_vimdiff 50423ab8~1 50423ab8
#	git_vimdiff 50423ab8~1 50423ab8 -- '**/*.dump'
#
# The raw files can also be examined:
#	:all

# Render a loaded screendump file or the difference of a loaded screendump
# file and its namesake file from the "dumps" directory.
def Render(letters: list<string>)
  const failed_fname: string = bufname()
  try
    setlocal suffixesadd=.dump
    const dumps_fname: string = findfile(
			fnamemodify(failed_fname, ':p:t'),
			fnamemodify(failed_fname, ':p:h:h') .. '/dumps')
    if filereadable(dumps_fname)
      term_dumpdiff(failed_fname, dumps_fname)
      FoldAndMarkDumpDiffParts(letters)
    else
      term_dumpload(failed_fname)
    endif
  finally
    exec 'bwipeout ' .. failed_fname
  endtry
enddef

# Public functions
#
# Search for the "failed" directory in the passed _subtreedirname_ directories
# (usually "\<src\>" or "\<syntax\>") and, if found, select its passed _count_
# occurrence, add all its "*.dump" files to the argument list and list them;
# also define a BufRead autocommand that would invoke "Render()" for every
# "*.dump" file.
def g:Init(subtreedirname: string, count: number)
  # Support sourcing this script from any directory in the direct path that
  # leads to the project's root directory.
  const failed_path: string = finddir('failed', getcwd() .. '/**', -1)
    ->filter(((cwdpath: string, parentdirname: string) =>
			(_: number, dirpath: string) =>
	cwdpath =~ parentdirname || dirpath =~ parentdirname)(
			getcwd(),
			subtreedirname))
    ->get(count, '') .. '/'
  var error: string = null_string
  if failed_path == '/'
    error = 'No such directory: "failed"'
  else
    const failed_fnames: string = failed_path .. readdir(failed_path,
			(fname: string) => fname =~ '^.\+\.dump$')
      ->join(' ' .. failed_path)
    if failed_fnames =~ 'failed/$'
      error = 'No such file: "*.dump"'
    else
      exec ':0argedit ' .. failed_fnames
      buffers
    endif
  endif
  autocmd_add([{
    replace:	true,
    group:	'viewdumps',
    event:	'BufRead',
    pattern:	'*.dump',
    cmd:	printf('Render(%s)', string(SmallAlphaLetters())),
  }])
  # Unconditionally help, in case a list of filenames is passed to the
  # command, the first terminal window with its BufRead event.
  silent doautocmd viewdumps BufRead
  if error != null_string
    # Instead of sleeping, fill half a window with blanks and prompt
    # hit-enter.
    echom error .. repeat("\x20",
	(winwidth(0) * (winheight(0) / 2) - strlen(error)))
  endif
enddef

# vim:fdm=syntax:sw=2:ts=8:noet:nosta:
