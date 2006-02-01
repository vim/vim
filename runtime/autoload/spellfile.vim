" Vim script to download a missing spell file
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2006 Feb 01

if !exists('g:spellfile_URL')
  let g:spellfile_URL = 'ftp://ftp.vim.org/pub/vim/unstable/runtime/spell'
endif
let s:spellfile_URL = ''    " Start with nothing so that s:donedict is reset.

" This function is used for the spellfile plugin.
function! spellfile#LoadFile(lang)
  " If the netrw plugin isn't loaded we silently skip everything.
  if !exists(":Nread")
    if &verbose
      echomsg 'spellfile#LoadFile(): Nread command is not available.'
    endif
    return
  endif

  " If the URL changes we try all files again.
  if s:spellfile_URL != g:spellfile_URL
    let s:donedict = {}
    let s:spellfile_URL = g:spellfile_URL
  endif

  " I will say this only once!
  if has_key(s:donedict, a:lang . &enc)
    if &verbose
      echomsg 'spellfile#LoadFile(): Tried this language/encoding before.'
    endif
    return
  endif
  let s:donedict[a:lang . &enc] = 1

  " Find spell directories we can write in.
  let dirlist = []
  let dirchoices = '&Cancel'
  for dir in split(globpath(&rtp, 'spell'), "\n")
    if filewritable(dir) == 2
      call add(dirlist, dir)
      let dirchoices .= "\n&" . len(dirlist)
    endif
  endfor
  if len(dirlist) == 0
    if &verbose
      echomsg 'spellfile#LoadFile(): There is no writable spell directory.'
    endif
    return
  endif

  let msg = 'Cannot find spell file for "' . a:lang . '" in ' . &enc
  let msg .= "\nDo you want me to try downloading it?"
  if confirm(msg, "&Yes\n&No", 2) == 1
    let enc = &encoding
    if enc == 'iso-8859-15'
      let enc = 'latin1'
    endif
    let fname = a:lang . '.' . enc . '.spl'

    " Split the window, read the file into a new buffer.
    new
    setlocal bin
    echo 'Downloading ' . fname . '...'
    exe 'Nread ' g:spellfile_URL . '/' . fname
    if getline(2) !~ 'VIMspell'
      " Didn't work, perhaps there is an ASCII one.
      g/^/d
      let fname = a:lang . '.ascii.spl'
      echo 'Could not find it, trying ' . fname . '...'
      exe 'Nread ' g:spellfile_URL . '/' . fname
      if getline(2) !~ 'VIMspell'
	echo 'Sorry, downloading failed'
	bwipe!
	return
      endif
    endif

    " Delete the empty first line and mark the file unmodified.
    1d
    set nomod

    let msg = "In which directory do you want to write the file:"
    for i in range(len(dirlist))
      let msg .= "\n" . (i + 1) . '. ' . dirlist[i]
    endfor
    let dirchoice = confirm(msg, dirchoices) - 2
    if dirchoice >= 0
      exe "write " . escape(dirlist[dirchoice], ' ') . '/' . fname

      " Also download the .sug file, if the user wants to.
      let msg = "Do you want me to try getting the .sug file?\n"
      let msg .= "This will improve making suggestions for spelling mistakes,\n"
      let msg .= "but it uses quite a bit of memory."
      if confirm(msg, "&No\n&Yes") == 2
	g/^/d
	let fname = substitute(fname, '\.spl$', '.sug', '')
	echo 'Downloading ' . fname . '...'
	exe 'Nread ' g:spellfile_URL . '/' . fname
	if getline(2) !~ 'VIMsug'
	  echo 'Sorry, downloading failed'
	else
	  1d
	  exe "write " . escape(dirlist[dirchoice], ' ') . '/' . fname
	endif
	set nomod
      endif
    endif

    bwipe
  endif
endfunc
