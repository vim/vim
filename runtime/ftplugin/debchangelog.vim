" Vim filetype plugin file
" Language:	Debian Changelog
" Maintainer:	Michael Piefel <piefel@informatik.hu-berlin.de>
" Last Change:	23 March 2004

if exists("g:did_changelog_ftplugin")
  finish
endif

" Don't load another plugin (this is global)
let g:did_changelog_ftplugin = 1

" Helper functions returning various data.
" Returns full name, either from $DEBFULLNAME or debianfullname.
" TODO Is there a way to determine name from anywhere else?
function <SID>FullName()
    if exists("$DEBFULLNAME")
	return $DEBFULLNAME
    elseif exists("g:debianfullname")
	return g:debianfullname
    else
	return "Your Name"
    endif
endfunction

" Returns email address, from $DEBEMAIL, $EMAIL or debianemail.
function <SID>Email()
    if exists("$DEBEMAIL")
	return $DEBEMAIL
    elseif exists("$EMAIL")
	return $EMAIL
    elseif exists("g:debianemail")
	return g:debianfullemail
    else
	return "your@email.address"
    endif
endfunction

" Returns date in RFC822 format.
function <SID>Date()
    let savelang = v:lc_time
    execute "language time C"
    let dateandtime = strftime("%a, %d %b %Y %X %z")
    execute "language time " . savelang
    return dateandtime
endfunction

function <SID>WarnIfNotUnfinalised()
    if match(getline("."), " -- [[:alpha:]][[:alnum:].]")!=-1
	echohl WarningMsg
	echo "The entry has not been unfinalised before editing."
	echohl None
	return 1
    endif
    return 0
endfunction

function <SID>Finalised()
    let savelinenum = line(".")
    normal 1G
    call search("^ -- ")
    if match(getline("."), " -- [[:alpha:]][[:alnum:].]")!=-1
	let returnvalue = 1
    else
	let returnvalue = 0
    endif
    execute savelinenum
    return returnvalue
endfunction

" These functions implement the menus
function NewVersion()
    " The new entry is unfinalised and shall be changed
    amenu disable Changelog.New\ Version
    amenu enable Changelog.Add\ Entry
    amenu enable Changelog.Close\ Bug
    amenu enable Changelog.Set\ Distribution
    amenu enable Changelog.Set\ Urgency
    amenu disable Changelog.Unfinalise
    amenu enable Changelog.Finalise
    call append(0, substitute(getline(1),'-\([[:digit:]]\+\))', '-Ü\1)', ''))
    call append(1, "")
    call append(2, "")
    call append(3, " -- ")
    call append(4, "")
    call Distribution("unstable")
    call Urgency("low")
    normal 1G
    call search(")")
    normal h
    normal 
    call setline(1, substitute(getline(1),'-Ü\([[:digit:]]\+\))', '-\1)', ''))
    call AddEntry()
endfunction

function AddEntry()
    normal 1G
    call search("^ -- ")
    normal kk
    call append(".", "  * ")
    normal jjj
    let warn=<SID>WarnIfNotUnfinalised()
    normal kk
    if warn
	echohl MoreMsg
	call input("Hit ENTER")
	echohl None
    endif
    startinsert!
endfunction

function CloseBug()
    normal 1G
    call search("^ -- ")
    let warn=<SID>WarnIfNotUnfinalised()
    normal kk
    call append(".", "  *  (closes: #" . input("Bug number to close: ") . ")")
    normal j^ll
    startinsert
endfunction

function Distribution(dist)
    call setline(1, substitute(getline(1), ") [[:lower:] ]*;", ") " . a:dist . ";", ""))
endfunction

function Urgency(urg)
    call setline(1, substitute(getline(1), "urgency=.*$", "urgency=" . a:urg, ""))
endfunction

function <SID>UnfinaliseMenu()
    " This means the entry shall be changed
    amenu disable Changelog.New\ Version
    amenu enable Changelog.Add\ Entry
    amenu enable Changelog.Close\ Bug
    amenu enable Changelog.Set\ Distribution
    amenu enable Changelog.Set\ Urgency
    amenu disable Changelog.Unfinalise
    amenu enable Changelog.Finalise
endfunction

function Unfinalise()
    call <SID>UnfinaliseMenu()
    normal 1G
    call search("^ -- ")
    call setline(".", " -- ")
endfunction

function <SID>FinaliseMenu()
    " This means the entry should not be changed anymore
    amenu enable Changelog.New\ Version
    amenu disable Changelog.Add\ Entry
    amenu disable Changelog.Close\ Bug
    amenu disable Changelog.Set\ Distribution
    amenu disable Changelog.Set\ Urgency
    amenu enable Changelog.Unfinalise
    amenu disable Changelog.Finalise
endfunction

function Finalise()
    call <SID>FinaliseMenu()
    normal 1G
    call search("^ -- ")
    call setline(".", " -- " . <SID>FullName() . " <" . <SID>Email() . ">  " . <SID>Date())
endfunction


function <SID>MakeMenu()
    amenu &Changelog.&New\ Version			:call NewVersion()<CR>
    amenu Changelog.&Add\ Entry				:call AddEntry()<CR>
    amenu Changelog.&Close\ Bug				:call CloseBug()<CR>
    menu Changelog.-sep-				<nul>

    amenu Changelog.Set\ &Distribution.&unstable	:call Distribution("unstable")<CR>
    amenu Changelog.Set\ Distribution.&frozen		:call Distribution("frozen")<CR>
    amenu Changelog.Set\ Distribution.&stable		:call Distribution("stable")<CR>
    menu Changelog.Set\ Distribution.-sep-		<nul>
    amenu Changelog.Set\ Distribution.frozen\ unstable	:call Distribution("frozen unstable")<CR>
    amenu Changelog.Set\ Distribution.stable\ unstable	:call Distribution("stable unstable")<CR>
    amenu Changelog.Set\ Distribution.stable\ frozen	:call Distribution("stable frozen")<CR>
    amenu Changelog.Set\ Distribution.stable\ frozen\ unstable	:call Distribution("stable frozen unstable")<CR>

    amenu Changelog.Set\ &Urgency.&low			:call Urgency("low")<CR>
    amenu Changelog.Set\ Urgency.&medium		:call Urgency("medium")<CR>
    amenu Changelog.Set\ Urgency.&high			:call Urgency("high")<CR>

    menu Changelog.-sep-				<nul>
    amenu Changelog.U&nfinalise				:call Unfinalise()<CR>
    amenu Changelog.&Finalise				:call Finalise()<CR>

    if <SID>Finalised()
	call <SID>FinaliseMenu()
    else
	call <SID>UnfinaliseMenu()
    endif
endfunction

augroup changelogMenu
au BufEnter * if &filetype == "debchangelog" | call <SID>MakeMenu() | endif
au BufLeave * if &filetype == "debchangelog" | aunmenu Changelog | endif
augroup END

" Debian changelogs are not supposed to have any other text width,
" so the user cannot override this setting
setlocal tw=78
setlocal comments=f:* 
let b:undo_ftplugin = "setlocal tw< comments<"
