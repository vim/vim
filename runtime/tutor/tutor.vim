" Vim tutor support file
" Author: Eduardo F. Amatria <eferna1@platea.pntic.mec.es>
" Last Change:	2006 Mar 18

" This small source file is used for detecting if a translation of the
" tutor file exist, i.e., a tutor.xx file, where xx is the language.
" If the translation does not exist, or no extension is given,
" it defaults to the english version.

" It is invoked by the vimtutor shell script.

" 1. Build the extension of the file, if any:
let s:ext = ""
if strlen($xx) > 1
  let s:ext = "." . $xx
else
  let s:lang = ""
  " Check that a potential value has at least two letters.
  " Ignore "1043" and "C".
  if exists("v:lang") && v:lang =~ '\a\a'
    let s:lang = v:lang
  elseif $LC_ALL =~ '\a\a'
    let s:lang = $LC_ALL
  elseif $LANG =~ '\a\a'
    let s:lang = $LANG
  endif
  if s:lang != ""
    " Remove "@euro" (ignoring case), it may be at the end
    let s:lang = substitute(s:lang, '\c@euro', '', '')
    " On MS-Windows it may be German_Germany.1252 or Polish_Poland.1250.  How
    " about other languages?
    if s:lang =~ "German"
      let s:ext = ".de"
    elseif s:lang =~ "Polish"
      let s:ext = ".pl"
    elseif s:lang =~ "Slovak"
      let s:ext = ".sk"
    elseif s:lang =~ "Dutch"
      let s:ext = ".nl"
    else
      let s:ext = "." . strpart(s:lang, 0, 2)
    endif
  endif
endif

" The japanese tutor is available in two encodings, guess which one to use
" The "sjis" one is actually "cp932", it doesn't matter for this text.
if s:ext =~? '\.ja'
  if &enc =~ "euc"
    let s:ext = ".ja.euc"
  elseif &enc =~ "utf-8$"
    let s:ext = ".ja.utf-8"
  else
    let s:ext = ".ja.sjis"
  endif
endif

" The korean tutor is available in two encodings, guess which one to use
if s:ext =~? '\.ko'
  if &enc =~ "utf-8$"
    let s:ext = ".ko.utf-8"
  else
    let s:ext = ".ko.euc"
  endif
endif

" The Chinese tutor is available in two encodings, guess which one to use
" This segment is from the above lines and modified by
" Mendel L Chan <beos@turbolinux.com.cn> for Chinese vim tutorial
if s:ext =~? '\.zh'
  if &enc =~ 'big5\|cp950'
    let s:ext = ".zh.big5"
  else
    let s:ext = ".zh.euc"
  endif
endif

" The Polish tutor is available in two encodings, guess which one to use.
if s:ext =~? '\.pl'
  if &enc =~ 1250
    let s:ext = ".pl.cp1250"
  elseif &enc =~ "utf-8$"
    let s:ext = ".pl.utf-8"
  endif
endif

" The Turkish tutor is available in two encodings, guess which one to use
if s:ext =~? '\.tr'
  if &enc == "utf-8"
    let s:ext = ".tr.utf-8"
  elseif &enc == "iso-8859-9"
    let s:ext = ".tr.iso9"
  endif
endif

" The Greek tutor is available in two encodings, guess which one to use
if s:ext =~? '\.gr' && &enc =~ 737
  let s:ext = ".gr.cp737"
endif

" The Slovak tutor is available in two encodings, guess which one to use
if s:ext =~? '\.sk' && &enc =~ 1250
  let s:ext = ".sk.cp1250"
endif

" The Russian tutor is available in two encodings, guess which one to use.
" This segment is from the above lines and modified by
" Alexey I. Froloff <raorn@altlinux.org> for Russian vim tutorial
if s:ext =~? '\.ru' && &enc =~ 1251
  let s:ext = ".ru.cp1251"
endif

" Somehow ".ge" (Germany) is sometimes used for ".de" (Deutsch).
if s:ext =~? '\.ge'
  let s:ext = ".de"
endif

if s:ext =~? '\.en'
  let s:ext = ""
endif

" 2. Build the name of the file:
let s:tutorfile = "/tutor/tutor"
let s:tutorxx = $VIMRUNTIME . s:tutorfile . s:ext

" 3. Finding the file:
if filereadable(s:tutorxx)
  let $TUTOR = s:tutorxx
else
  let $TUTOR = $VIMRUNTIME . s:tutorfile
  echo "The file " . s:tutorxx . " does not exist.\n"
  echo "Copying English version: " . $TUTOR
  4sleep
endif

" 4. Making the copy and exiting Vim:
e $TUTOR
wq! $TUTORCOPY
