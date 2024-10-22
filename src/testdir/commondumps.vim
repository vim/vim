vim9script

# (Script-local.)
#
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
    else
      term_dumpload(failed_fname)
    endif
  finally
    exec 'bwipeout ' .. failed_fname
  endtry
enddef

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
