vim9script

# Support sourcing this script from this directory or any other directory in
# the direct path that leads to the project's root directory.
const failed_path: string = finddir('failed', getcwd() .. '/**', -1)
  ->filter(((cwdpath: string) => (_: number, dirpath: string) =>
	cwdpath =~ '\<syntax\>' || dirpath =~ '\<syntax\>')(getcwd()))
  ->get(-1, '') .. '/'
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

# THE FOLLOWING SETTINGS PERTAIN TO "input/" FILES THAT ARE LIKELY TO BE
# LOADED SIDE BY SIDE WHENEVER BATCHES OF NEW SCREENDUMPS ARE GENERATED.

# Match "LC_ALL=C" of Makefile.
language C

# Match the settings from term_util.vim#RunVimInTerminal().
set t_Co=256 background=light
hi Normal ctermfg=NONE ctermbg=NONE

# Match the settings from runtest.vim#Xtestscript#SetUpVim().
set display=lastline ruler scrolloff=5 t_ZH= t_ZR=

# Anticipate non-Latin-1 characters in "input/" files.
set encoding=utf-8 termencoding=utf-8

autocmd_add([{
  replace:	true,
  group:	'viewdumps',
  event:	'BufRead',
  pattern:	'*.dump',
  cmd:		'Render()',
}])

# Unconditionally help, in case a list of filenames is passed to the command,
# the first terminal window with its BufRead event.
silent doautocmd viewdumps BufRead

if error != null_string
  # Instead of sleeping, fill half a window with blanks and prompt hit-enter.
  echom error .. repeat("\x20", (winwidth(0) * (winheight(0) / 2) - strlen(error)))
  error = null_string
endif

# vim:fdm=syntax:sw=2:ts=8:noet:nolist:nosta:
