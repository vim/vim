" Vim filetype plugin file
" Language:         generic Changelog file
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2007-05-06
" Variables:
"   g:changelog_timeformat (deprecated: use g:changelog_dateformat instead) -
"       description: the timeformat used in ChangeLog entries.
"       default: "%Y-%m-%d".
"   g:changelog_dateformat -
"       description: the format sent to strftime() to generate a date string.
"       default: "%Y-%m-%d".
"   g:changelog_username -
"       description: the username to use in ChangeLog entries
"       default: try to deduce it from environment variables and system files.
" Local Mappings:
"   <Leader>o -
"       adds a new changelog entry for the current user for the current date.
" Global Mappings:
"   <Leader>o -
"       switches to the ChangeLog buffer opened for the current directory, or
"       opens it in a new buffer if it exists in the current directory.  Then
"       it does the same as the local <Leader>o described above.
" Notes:
"   run 'runtime ftplugin/changelog.vim' to enable the global mapping for
"   changelog files.
" TODO:
"  should we perhaps open the ChangeLog file even if it doesn't exist already?
"  Problem is that you might end up with ChangeLog files all over the place.

" If 'filetype' isn't "changelog", we must have been to add ChangeLog opener
if &filetype == 'changelog'
  if exists('b:did_ftplugin')
    finish
  endif
  let b:did_ftplugin = 1

  let s:cpo_save = &cpo
  set cpo&vim

  " Set up the format used for dates.
  if !exists('g:changelog_dateformat')
    if exists('g:changelog_timeformat')
      let g:changelog_dateformat = g:changelog_timeformat
    else
      let g:changelog_dateformat = "%Y-%m-%d"
    endif
  endif

  " Try to figure out a reasonable username of the form:
  "   Full Name <user@host>.
  if !exists('g:changelog_username')
    if exists('$EMAIL') && $EMAIL != ''
      let g:changelog_username = $EMAIL
    elseif exists('$EMAIL_ADDRESS') && $EMAIL_ADDRESS != ''
      " This is some Debian junk if I remember correctly.
      let g:changelog_username = $EMAIL_ADDRESS
    else
      " Get the users login name.
      let login = system('whoami')
      if v:shell_error
        let login = 'unknown'
      else
        let newline = stridx(login, "\n")
        if newline != -1
          let login = strpart(login, 0, newline)
        endif
      endif

      " Try to get the full name from gecos field in /etc/passwd.
      if filereadable('/etc/passwd')
        for line in readfile('/etc/passwd')
          if line =~ '^' . login
            let name = substitute(line,'^\%([^:]*:\)\{4}\([^:]*\):.*$','\1','')
            " Only keep stuff before the first comma.
            let comma = stridx(name, ',')
            if comma != -1
              let name = strpart(name, 0, comma)
            endif
            " And substitute & in the real name with the login of our user.
            let amp = stridx(name, '&')
            if amp != -1
              let name = strpart(name, 0, amp) . toupper(login[0]) .
                       \ strpart(login, 1) . strpart(name, amp + 1)
            endif
          endif
        endfor
      endif

      " If we haven't found a name, try to gather it from other places.
      if !exists('name')
        " Maybe the environment has something of interest.
        if exists("$NAME")
          let name = $NAME
        else
          " No? well, use the login name and capitalize first
          " character.
          let name = toupper(login[0]) . strpart(login, 1)
        endif
      endif

      " Get our hostname.
      let hostname = system('hostname')
      if v:shell_error
        let hostname = 'localhost'
      else
        let newline = stridx(hostname, "\n")
        if newline != -1
          let hostname = strpart(hostname, 0, newline)
        endif
      endif

      " And finally set the username.
      let g:changelog_username = name . '  <' . login . '@' . hostname . '>'
    endif
  endif

  " Format used for new date entries.
  if !exists('g:changelog_new_date_format')
    let g:changelog_new_date_format = "%d  %u\n\n\t* %c\n\n"
  endif

  " Format used for new entries to current date entry.
  if !exists('g:changelog_new_entry_format')
    let g:changelog_new_entry_format = "\t* %c"
  endif

  " Regular expression used to find a given date entry.
  if !exists('g:changelog_date_entry_search')
    let g:changelog_date_entry_search = '^\s*%d\_s*%u'
  endif

  " Regular expression used to find the end of a date entry
  if !exists('g:changelog_date_end_entry_search')
    let g:changelog_date_entry_search = '^\s*$'
  endif


  " Substitutes specific items in new date-entry formats and search strings.
  " Can be done with substitute of course, but unclean, and need \@! then.
  function! s:substitute_items(str, date, user)
    let str = a:str
    let middles = {'%': '%', 'd': a:date, 'u': a:user, 'c': '{cursor}'}
    let i = stridx(str, '%')
    while i != -1
      let inc = 0
      if has_key(middles, str[i + 1])
        let mid = middles[str[i + 1]]
        let str = strpart(str, 0, i) . mid . strpart(str, i + 2)
        let inc = strlen(mid)
      endif
      let i = stridx(str, '%', i + 1 + inc)
    endwhile
    return str
  endfunction

  " Position the cursor once we've done all the funky substitution.
  function! s:position_cursor()
    if search('{cursor}') > 0
      let lnum = line('.')
      let line = getline(lnum)
      let cursor = stridx(line, '{cursor}')
      call setline(lnum, substitute(line, '{cursor}', '', ''))
    endif
    startinsert!
  endfunction

  " Internal function to create a new entry in the ChangeLog.
  function! s:new_changelog_entry()
    " Deal with 'paste' option.
    let save_paste = &paste
    let &paste = 1
    call cursor(1, 1)
    " Look for an entry for today by our user.
    let date = strftime(g:changelog_dateformat)
    let search = s:substitute_items(g:changelog_date_entry_search, date,
                                  \ g:changelog_username)
    if search(search) > 0
      " Ok, now we look for the end of the date entry, and add an entry.
      call cursor(nextnonblank(line('.') + 1), 1)
      if search(g:changelog_date_end_entry_search, 'W') > 0
        let p = line('.') - 1
      else
        let p = line('.')
      endif
      let ls = split(s:substitute_items(g:changelog_new_entry_format, '', ''),
                   \ '\n')
      call append(p, ls)
      call cursor(p + 1, 1)
    else
      " Flag for removing empty lines at end of new ChangeLogs.
      let remove_empty = line('$') == 1

      " No entry today, so create a date-user header and insert an entry.
      let todays_entry = s:substitute_items(g:changelog_new_date_format,
                                          \ date, g:changelog_username)
      " Make sure we have a cursor positioning.
      if stridx(todays_entry, '{cursor}') == -1
        let todays_entry = todays_entry . '{cursor}'
      endif

      " Now do the work.
      call append(0, split(todays_entry, '\n'))
      
      " Remove empty lines at end of file.
      if remove_empty
        $-/^\s*$/-1,$delete
      endif

      " Reposition cursor once we're done.
      call cursor(1, 1)
    endif

    call s:position_cursor()

    " And reset 'paste' option
    let &paste = save_paste
  endfunction

  if exists(":NewChangelogEntry") != 2
    map <buffer> <silent> <Leader>o <Esc>:call <SID>new_changelog_entry()<CR>
    command! -nargs=0 NewChangelogEntry call s:new_changelog_entry()
  endif

  let b:undo_ftplugin = "setl com< fo< et< ai<"

  setlocal comments=
  setlocal formatoptions+=t
  setlocal noexpandtab
  setlocal autoindent

  if &textwidth == 0
    setlocal textwidth=78
    let b:undo_ftplugin .= " tw<"
  endif

  let &cpo = s:cpo_save
  unlet s:cpo_save
else
  " Add the Changelog opening mapping
  nmap <silent> <Leader>o :call <SID>open_changelog()<CR>

  function! s:open_changelog()
    if !filereadable('ChangeLog')
      return
    endif
    let buf = bufnr('ChangeLog')
    if buf != -1
      if bufwinnr(buf) != -1
        execute bufwinnr(buf) . 'wincmd w'
      else
        execute 'sbuffer' buf
      endif
    else
      split ChangeLog
    endif

    call s:new_changelog_entry()
  endfunction
endif
