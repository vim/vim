" Vim filetype plugin file
" Language:	    generic Changelog file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/ftplugin/pcp/changelog/
" Latest Revision:  2004-04-25
" arch-tag:	    b00e2974-c559-4477-b7b2-3ef3f4061bdb
" Variables:
"   g:changelog_timeformat -
"	description: the timeformat used in ChangeLog entries.
"	default: "%Y-%m-%d".
"   g:changelog_username -
"	description: the username to use in ChangeLog entries
"	default: try to deduce it from environment variables and system	files.
" Local Mappings:
"   <Leader>o -
"	adds a new changelog entry for the current user for the current date.
" Global Mappings:
"   <Leader>o -
"	switches to the ChangeLog buffer opened for the current directory, or
"	opens it in a new buffer if it exists in the current directory.  Then
"	it does the same as the local <Leader>o described above.
" Notes:
"   run 'runtime ftplugin/changelog.vim' to enable the global mapping for
"   changelog files.
" TODO:
"  should we perhaps open the ChangeLog file even if it doesn't exist already?
"  Problem is that you might end up with ChangeLog files all over the place.

" If 'filetype' isn't "changelog", we must have been to add ChangeLog opener
if &filetype == "changelog"
  " Only do this when not done yet for this buffer
  if exists("b:did_ftplugin")
    finish
  endif

  " Don't load another plugin for this buffer
  let b:did_ftplugin = 1

  let cpo_save = &cpo
  set cpo-=C

  " The format of the date-time field (should have been called dateformat)
  if !exists("g:changelog_timeformat")
    let g:changelog_timeformat = "%Y-%m-%d"
  endif

  " Try to figure out a reasonable username of the form:
  " Full Name <user@host>
  if !exists("g:changelog_username")
    if exists("$EMAIL_ADDRESS")
      let g:changelog_username = $EMAIL_ADDRESS
    elseif exists("$EMAIL")
      let g:changelog_username = $EMAIL
    else
      " Get the users login name
      let login = system('whoami')
      if v:shell_error
	let login = 'unknown'
      else
	let newline = stridx(login, "\n")
	if newline != -1
	  let login = strpart(login, 0, newline)
	endif
      endif

      " Try to full name from gecos field in /etc/passwd
      if filereadable('/etc/passwd')
	let name = substitute(
	      \system('cat /etc/passwd | grep ^`whoami`'),
	      \'^\%([^:]*:\)\{4}\([^:]*\):.*$', '\1', '')
      endif

      " If there is no such file, or there was some other problem try
      " others
      if !filereadable('/etc/passwd') || v:shell_error
	" Maybe the environment has something of interest
	if exists("$NAME")
	  let name = $NAME
	else
	  " No? well, use the login name and capitalize first
	  " character
	  let name = toupper(login[0]) . strpart(login, 1)
	endif
      endif

      " Only keep stuff before the first comma
      let comma = stridx(name, ',')
      if comma != -1
	let name = strpart(name, 0, comma)
      endif

      " And substitute & in the real name with the login of our user
      let amp = stridx(name, '&')
      if amp != -1
	let name = strpart(name, 0, amp) . toupper(login[0]) .
	      \strpart(login, 1) . strpart(name, amp + 1)
      endif

      " Get our hostname
      let hostname = system("hostname")
      if v:shell_error
	let hostname = 'unknownhost'
      else
	let newline = stridx(hostname, "\n")
	if newline != -1
	  let hostname = strpart(hostname, 0, newline)
	endif
      endif

      " And finally set the username
      let g:changelog_username = name.'  <'.login.'@'.hostname.'>'
    endif
  endif

  " Format used for new date-entries
  if !exists("g:changelog_new_date_format")
    let g:changelog_new_date_format = "%d  %u\n\n\t* %c\n\n"
  endif

  " Format used for new entries to current date-entry
  if !exists("g:changelog_new_entry_format")
    let g:changelog_new_entry_format = "\t* %c"
  endif

  if !exists("g:changelog_date_entry_search")
    let g:changelog_date_entry_search = '^\s*%d\_s*%u'
  endif

  " Substitutes specific items in new date-entry formats and search strings
  " Can be done with substitute of course, but unclean, and need \@! then
  function! s:substitute_items(str, date, user)
    let str = a:str
    let i = stridx(str, '%')
    while i != -1
      let char = str[i + 1]
      if char == '%'
	let middle = '%'
      elseif char == 'd'
	let middle = a:date
      elseif char == 'u'
	let middle = a:user
      elseif char == 'c'
	let middle = '{cursor}'
      else
	let middle = char
      endif
      let str = strpart(str, 0, i) . middle . strpart(str, i + 2)
      let i = stridx(str, '%')
    endwhile
    return str
  endfunction

  function! s:position_cursor()
    if search('{cursor}') > 0
      let pos = line('.')
      let line = getline(pos)
      let cursor = stridx(line, '{cursor}')
      call setline(pos, substitute(line, '{cursor}', '', ''))
    endif
    startinsert!
  endfunction

  " Internal function to create a new entry in the ChangeLog
  function! s:new_changelog_entry()
    " Deal with 'paste' option
    let save_paste = &paste
    let &paste = 1
    1
    " Look for an entry for today by our user
    let date = strftime(g:changelog_timeformat)
    let search = s:substitute_items(g:changelog_date_entry_search, date,
	  \g:changelog_username)
    if search(search) > 0
      " Ok, now we look for the end of the date-entry, and add an entry
      let pos = nextnonblank(line('.') + 1)
      let line = getline(pos)
      while line =~ '^\s\+\S\+'
	let pos = pos + 1
	let line = getline(pos)
      endwhile
      let insert = s:substitute_items(g:changelog_new_entry_format,
	    \'', '')
      execute "normal! ".(pos - 1)."Go".insert
      execute pos
    else
      " Flag for removing empty lines at end of new ChangeLogs
      let remove_empty = line('$') == 1

      " No entry today, so create a date-user header and insert an entry
      let todays_entry = s:substitute_items(g:changelog_new_date_format,
	    \date, g:changelog_username)
      " Make sure we have a cursor positioning
      if stridx(todays_entry, '{cursor}') == -1
	let todays_entry = todays_entry.'{cursor}'
      endif

      " Now do the work
      execute "normal! i".todays_entry
      if remove_empty
	while getline('$') == ''
	  $delete
	endwhile
      endif

      1
    endif

    call s:position_cursor()

    " And reset 'paste' option
    let &paste = save_paste
  endfunction

  if exists(":NewChangelogEntry") != 2
    map <buffer> <silent> <Leader>o <Esc>:call <SID>new_changelog_entry()<CR>
    command! -nargs=0 NewChangelogEntry call s:new_changelog_entry()
  endif

  let b:undo_ftplugin = "setl com< tw< fo< et< ai<"

  if &textwidth == 0
    setlocal textwidth=78
  endif
  setlocal comments=
  setlocal formatoptions+=t
  setlocal noexpandtab
  setlocal autoindent

  let &cpo = cpo_save
else
  " Add the Changelog opening mapping
  nmap <silent> <Leader>o :call <SID>open_changelog()<CR>

  function! s:open_changelog()
    if filereadable('ChangeLog')
      if bufloaded('ChangeLog')
	let buf = bufnr('ChangeLog')
	execute "normal! \<C-W>t"
	while winbufnr(winnr()) != buf
	  execute "normal! \<C-W>w"
	endwhile
      else
	split ChangeLog
      endif

      if exists("g:mapleader")
	execute "normal " . g:mapleader . "o"
      else
	execute "normal \\o"
      endif
      startinsert!
    endif
  endfunction
endif

" vim: set sts=2 sw=2:
