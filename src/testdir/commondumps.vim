vim9script

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


# Script-local functions
#
# Fold the difference part and the bottom part when the top and the bottom
# parts are identical.
def FoldDumpDiffCopy()
  try
    normal mc
    # Shape the pattern after get_separator() from "terminal.c".
    const separator: string = '^\(=\+\)\=\s\S.*\.dump\s\1$'
    const start_lnum: number = search(separator, 'eW', (line('$') / 2))
    if start_lnum > 0
      const end_lnum: number = search(separator, 'eW')
      if end_lnum > 0 && getline((start_lnum + 1), (end_lnum - 1))
	  ->filter((_: number, line: string) => line !~ '^\s\+$')
	  ->empty()
	setlocal foldenable foldmethod=manual
	exec 'normal ' .. start_lnum .. 'GzfG'
      endif
    endif
  finally
    normal `c
  endtry
enddef

# Render a loaded screendump file or the difference of a loaded screendump
# file and its namesake file from the "dumps" directory.
def Render()
  const failed_fname: string = bufname()
  try
    setlocal suffixesadd=.dump
    const dumps_fname: string = findfile(
			fnamemodify(failed_fname, ':p:t'),
			fnamemodify(failed_fname, ':p:h:h') .. '/dumps')
    if filereadable(dumps_fname)
      term_dumpdiff(failed_fname, dumps_fname)
      FoldDumpDiffCopy()
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
    cmd:	'Render()',
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
