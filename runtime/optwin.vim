  " These commands create the option window.
  "
" Maintainer:	Bram Moolenaar <Bram@vim.org> (Restorer <restorers@users.sf.org>)
" Last Change:	26 Aug 2020

" If there already is an option window, jump to that one.
let buf = bufnr('option-window')
if buf >= 0
  let winids = win_findbuf(buf)
  if len(winids) > 0
    if win_gotoid(winids[0]) == 1
      finish
    endif
  endif
endif

" Make sure the '<' flag is not included in 'cpoptions', otherwise <CR> would
" not be recognized.  See ":help 'cpoptions'".
let s:cpo_save = &cpo
set cpo&vim

" function to be called when <CR> is hit in the option-window
fun! <SID>CR()

  " If on a continued comment line, go back to the first comment line
  let lnum = search("^[^\t]", 'bWcn')
  let line = getline(lnum)

  " <CR> on a "set" line executes the option line
  if match(line, "^ \tset ") >= 0

    " For a local option: go to the previous window
    " If this is a help window, go to the window below it
    let thiswin = winnr()
    let local = <SID>Find(lnum)
    if local >= 0
      exe line
      call <SID>Update(lnum, line, local, thiswin)
    endif

  " <CR> on a "option" line shows help for that option
  elseif match(line, "^[a-z]") >= 0
    let name = substitute(line, '\([^\t]*\).*', '\1', "")
    exe "help '" . name . "'"

  " <CR> on an index line jumps to the group
  elseif match(line, '^ \=[0-9]') >= 0
    exe "norm! /" . line . "\<CR>zt"
  endif
endfun

" function to be called when <Space> is hit in the option-window
fun! <SID>Space()

  let lnum = line(".")
  let line = getline(lnum)

  " <Space> on a "set" line refreshes the option line
  if match(line, "^ \tset ") >= 0

    " For a local option: go to the previous window
    " If this is a help window, go to the window below it
    let thiswin = winnr()
    let local = <SID>Find(lnum)
    if local >= 0
      call <SID>Update(lnum, line, local, thiswin)
    endif

  endif
endfun

" find the window in which the option applies
" returns 0 for global option, 1 for local option, -1 for error
fun! <SID>Find(lnum)
    if getline(a:lnum - 1) =~ "(local to"
      let local = 1
      let thiswin = winnr()
      wincmd p
      if exists("b:current_syntax") && b:current_syntax == "help"
	wincmd j
	if winnr() == thiswin
	  wincmd j
	endif
      endif
    else
      let local = 0
    endif
    if local && (winnr() == thiswin || (exists("b:current_syntax")
	\ && b:current_syntax == "help"))
      echo "Don't know in which window"
      let local = -1
    endif
    return local
endfun

" Update a "set" line in the option window
fun! <SID>Update(lnum, line, local, thiswin)
  " get the new value of the option and update the option window line
  if match(a:line, "=") >= 0
    let name = substitute(a:line, '^ \tset \([^=]*\)=.*', '\1', "")
  else
    let name = substitute(a:line, '^ \tset \(no\)\=\([a-z]*\).*', '\2', "")
  endif
  if name == "pt" && &pt =~ "\x80"
    let val = <SID>PTvalue()
  else
    let val = escape(eval('&' . name), " \t\\\"|")
  endif
  if a:local
    exe a:thiswin . "wincmd w"
  endif
  if match(a:line, "=") >= 0 || (val != "0" && val != "1")
    call setline(a:lnum, " \tset " . name . "=" . val)
  else
    if val
      call setline(a:lnum, " \tset " . name . "\tno" . name)
    else
      call setline(a:lnum, " \tset no" . name . "\t" . name)
    endif
  endif
  set nomodified
endfun

" Reset 'title' and 'icon' to make it work faster.
" Reset 'undolevels' to avoid undo'ing until the buffer is empty.
let s:old_title = &title
let s:old_icon = &icon
let s:old_sc = &sc
let s:old_ru = &ru
let s:old_ul = &ul
set notitle noicon nosc noru ul=-1

" If the current window is a help window, try finding a non-help window.
" Relies on syntax highlighting to be switched on.
let s:thiswin = winnr()
while exists("b:current_syntax") && b:current_syntax == "help"
  wincmd w
  if s:thiswin == winnr()
    break
  endif
endwhile

" Open the window.  $OPTWIN_CMD is set to "tab" for ":tab options".
exe $OPTWIN_CMD . ' new option-window'
setlocal ts=15 tw=0 noro buftype=nofile


" "Restorer: подготовка
" "Restorer: preparation

if exists("did_optwin_trans")
  unlet did_optwin_trans
endif
if exists("g:optwin_trans_ok")
  unlet g:optwin_trans_ok
endif

" effective? faster?
" unlet! did_optwin_trans
" unlet! g:optwin_trans_ok

let s:banner=[]
let s:contents=[]
let s:optdesc={}

let s:contlen_ne = 0
let s:optdesclen_ne = 0
let s:optwin_fl = ""
let s:contlen_en = 0
let s:optdesclen_en = 0

" "Restorer: поехали!


if exists("v:lang")
  let s:lang = v:lang
  if '^en' !~? s:lang 
" "Restorer: скопировал из menu.vim
" "Restorer: copy-paste from menu.vim
    " When the language does not include the charset add 'encoding'
    if s:lang =~ '^\a\a$\|^\a\a_\a\a$'
      let s:lang = s:lang . '.' . &enc
    endif
    " We always use a lowercase name.
    " Change "iso-8859" to "iso_8859" and "iso8859" to "iso_8859", some
    " systems appear to use this.
    " Change spaces to underscores.
    let s:lang = substitute(tolower(s:lang), '\.iso-', ".iso_", "")
    let s:lang = substitute(s:lang, '\.iso8859', ".iso_8859", "")
    let s:lang = substitute(s:lang, " ", "_", "g")
    " Remove "@euro", otherwise "LC_ALL=de_DE@euro gvim" will show English menus
    let s:lang = substitute(s:lang, "@euro", "", "")
    " Change "iso_8859-1" and "iso_8859-15" to "latin1", we always use the
    " same menu file for them.
    let s:lang = substitute(s:lang, 'iso_8859-15\=$', "latin1", "")
    exe "runtime! lang/optwin_" . s:lang . ".vim"

    if !exists("did_optwin_trans")
      " There is no exact match, try matching with a wildcard added
      " (e.g. find menu_de_de.iso_8859-1.vim if s:lang == de_DE).
      let s:lang = substitute(s:lang, '\.[^.]*', "", "")
      exe "runtime! lang/optwin_" . s:lang . "[^a-z]*vim"

      if !exists("did_optwin_trans") && s:lang =~ '_'
        " If the language includes a region try matching without that region.
        " (e.g. find menu_de.vim if s:lang == de_DE).
        let langonly = substitute(s:lang, '_.*', "", "")
        exe "runtime! lang/optwin_" . langonly . "[^a-z]*vim"
      endif

      if !exists("did_optwin_trans") && strlen($LANG) > 1 && '^en' !~ s:lang
        " On windows locale names are complicated, try using $LANG, it might
        " have been set by set_init_1().  But don't do this for "en" or "en_us".
        " But don't match "slovak" when $LANG is "sl".
        exe "runtime! lang/optwin_" . tolower($LANG) . "[^a-z]*vim"
      endif
    endif
" "Restorer: так как перевод может не соответствовать актуальному состоянию
" английской версии, то прежде чем что‐то делать требуется это проверить.
" Считываем размер массивов из английской версии, сравниваем, и при положительном
" результате загружаем перевод.
" "Restorer: since the translation may not correspond to the current state of
" the English version, you need to check it before doing anything.  We read the
" size of arrays from the English version, compare them, and load the
" translation if the result is positive. 
    if exists("did_optwin_trans")
      let s:contlen_ne = g:contlen
      let s:optdesclen_ne = g:optdesclen
      let s:optwin_fl = g:optwin_fl
      unlet g:contlen
      unlet g:optdesclen
      unlet g:optwin_fl
      unlet did_optwin_trans
      exe "runtime lang/optwin_en.latin1.vim"
      let s:contlen_en = g:contlen
      let s:optdesclen_en = g:optdesclen
      unlet g:contlen
      unlet g:optdesclen
      unlet g:optwin_fl
      unlet did_optwin_trans
      if (s:contlen_en == s:contlen_ne) && (s:optdesclen_en == s:optdesclen_ne)
	let g:optwin_trans_ok = 2
	exe "runtime lang/" . s:optwin_fl
	let s:banner = copy(g:banner)
	unlet g:banner
	let s:contents = copy(g:contents)
	unlet g:contents
	let s:optdesc = copy(g:optdesc)
	unlet g:optdesc
	unlet g:optwin_trans_ok
	unlet did_optwin_trans
	unlet g:optwin_fl
      endif
    endif
  endif
endif

" "Restorer: если ни одна из предыдущих проверок и действий не дали результата,
" то загружаем английский вариант
" "Restorer: if none of the previous checks and actions were successful, then
" load the English version 
if 0 == len(s:banner) || 0 == len(s:contents) || 0 == len(s:optdesc)
  let g:optwin_trans_ok = 1
  exe "runtime lang/optwin_en.latin1.vim"
  let s:banner = copy(g:banner)
  unlet g:banner
  let s:contents = copy(g:contents)
  unlet g:contents
  let s:optdesc = copy(g:optdesc)
  unlet g:optdesc
  unlet g:optwin_trans_ok
  unlet did_optwin_trans g:optwin_fl
endif

"НАДО: сделать, наверное, сообщение пользователю, что у него устаревшая версия
"перевода или вообще отсутствует. А может и не надо.
"TODO:

" Insert help and a "set" command for each option.
call append(0, s:banner)

" These functions are called often below.  Keep them fast!

" Init a local binary option
fun! <SID>BinOptionL(name)
  let val = getwinvar(winnr('#'), '&' . a:name)
  call append("$", substitute(substitute(" \tset " . val . a:name . "\t" .
	\!val . a:name, "0", "no", ""), "1", "", ""))
endfun

" Init a global binary option
fun! <SID>BinOptionG(name, val)
  call append("$", substitute(substitute(" \tset " . a:val . a:name . "\t" .
	\!a:val . a:name, "0", "no", ""), "1", "", ""))
endfun

" Init a local string option
fun! <SID>OptionL(name)
  let val = escape(getwinvar(winnr('#'), '&' . a:name), " \t\\\"|")
  call append("$", " \tset " . a:name . "=" . val)
endfun

" Init a global string option
fun! <SID>OptionG(name, val)
  call append("$", " \tset " . a:name . "=" . escape(a:val, " \t\\\"|"))
endfun

let s:idx = 1
let s:lnum = line("$")
call append("$", "")

fun! <SID>Header(nrcnts)
  let itmcnt = s:contents[a:nrcnts]
  let line = s:idx . '.' . " " . itmcnt
  if a:nrcnts < 10
    let line = " " . line
  endif
  call append("$", "")
  call append("$", line)
  call append("$", "")
  call append(s:lnum, line)
  let s:idx = s:idx + 1
  let s:lnum = s:lnum + 1
endfun

" "Restorer: печать наименования параметра и краткого пояснения к нему. Будет
" печатать столько строк, сколько содержится в описании к этому параметру
" "Restorer: print the parameter name and a brief explanation. It will print as
" many lines as the description for this parameter contains 
function! <SID>PrtOptDesc(optname)
  let desc = s:optdesc[a:optname]
  let desc0 = desc[0]
  let desc0 = a:optname . desc0
  let desc[0] = desc0
  call append("$", desc)
endfunction

" Get the value of 'pastetoggle'.  It could be a special key.
fun! <SID>PTvalue()
  redir @a
  silent set pt
  redir END
  return substitute(@a, '[^=]*=\(.*\)', '\1', "")
endfun

" Restore the previous value of 'cpoptions' here, it's used below.
let &cpo = s:cpo_save

" List of all options, organized by function.
" The text should be sufficient to know what the option is used for.

call <SID>Header(1)
call <SID>PrtOptDesc("compatible")
call <SID>BinOptionG("cp", &cp)
call <SID>PrtOptDesc("cpoptions")
call <SID>OptionG("cpo", &cpo)
call <SID>PrtOptDesc("insertmode")
call <SID>BinOptionG("im", &im)
call <SID>PrtOptDesc("paste")
call <SID>BinOptionG("paste", &paste)
call <SID>PrtOptDesc("pastetoggle")
if &pt =~ "\x80"
  call append("$", " \tset pt=" . <SID>PTvalue())
else
  call <SID>OptionG("pt", &pt)
endif
call <SID>PrtOptDesc("runtimepath")
call <SID>OptionG("rtp", &rtp)
call <SID>PrtOptDesc("packpath")
call <SID>OptionG("pp", &pp)
call <SID>PrtOptDesc("helpfile")
call <SID>OptionG("hf", &hf)


call <SID>Header(2)
call <SID>PrtOptDesc("whichwrap")
call <SID>OptionL("ww")
call <SID>PrtOptDesc("startofline")
call <SID>BinOptionG("sol", &sol)
call <SID>PrtOptDesc("paragraphs")
call <SID>OptionG("para", &para)
call <SID>PrtOptDesc("sections")
call <SID>OptionG("sect", &sect)
call <SID>PrtOptDesc("path")
call <SID>OptionG("pa", &pa)
call <SID>PrtOptDesc("cdpath")
call <SID>OptionG("cd", &cd)
if exists("+autochdir")
  call <SID>PrtOptDesc("autochdir")
  call <SID>BinOptionG("acd", &acd)
endif
call <SID>PrtOptDesc("wrapscan")
call <SID>BinOptionG("ws", &ws)
call <SID>PrtOptDesc("incsearch")
call <SID>BinOptionG("is", &is)
call <SID>PrtOptDesc("magic")
call <SID>BinOptionG("magic", &magic)
call <SID>PrtOptDesc("regexpengine")
call <SID>OptionG("re", &re)
call <SID>PrtOptDesc("ignorecase")
call <SID>BinOptionG("ic", &ic)
call <SID>PrtOptDesc("smartcase")
call <SID>BinOptionG("scs", &scs)
call <SID>PrtOptDesc("casemap")
call <SID>OptionG("cmp", &cmp)
call <SID>PrtOptDesc("maxmempattern")
call append("$", " \tset mmp=" . &mmp)
call <SID>PrtOptDesc("define")
call <SID>OptionG("def", &def)
if has("find_in_path")
  call <SID>PrtOptDesc("include")
  call <SID>OptionL("inc")
  call <SID>PrtOptDesc("includeexpr")
  call <SID>OptionL("inex")
endif


call <SID>Header(3)
call <SID>PrtOptDesc("tagbsearch")
call <SID>BinOptionG("tbs", &tbs)
call <SID>PrtOptDesc("taglength")
call append("$", " \tset tl=" . &tl)
call <SID>PrtOptDesc("tags")
call <SID>OptionG("tag", &tag)
call <SID>PrtOptDesc("tagcase")
call <SID>OptionG("tc", &tc)
call <SID>PrtOptDesc("tagrelative")
call <SID>BinOptionG("tr", &tr)
call <SID>PrtOptDesc("tagstack")
call <SID>BinOptionG("tgst", &tgst)
call <SID>PrtOptDesc("showfulltag")
call <SID>BinOptionG("sft", &sft)
if has("eval")
  call <SID>PrtOptDesc("tagfunc")
  call <SID>OptionL("tfu")
endif
if has("cscope")
  call <SID>PrtOptDesc("cscopeprg")
  call <SID>OptionG("csprg", &csprg)
  call <SID>PrtOptDesc("cscopetag")
  call <SID>BinOptionG("cst", &cst)
  call <SID>PrtOptDesc("cscopetagorder")
  call append("$", " \tset csto=" . &csto)
  call <SID>PrtOptDesc("cscopeverbose")
  call <SID>BinOptionG("csverb", &csverb)
  call <SID>PrtOptDesc("cscopepathcomp")
  call append("$", " \tset cspc=" . &cspc)
  call <SID>PrtOptDesc("cscopequickfix")
  call <SID>OptionG("csqf", &csqf)
  call <SID>PrtOptDesc("cscoperelative")
  call <SID>BinOptionG("csre", &csre)
endif


call <SID>Header(4)
call <SID>PrtOptDesc("scroll")
call <SID>OptionL("scr")
call <SID>PrtOptDesc("scrolloff")
call append("$", " \tset so=" . &so)
call <SID>PrtOptDesc("wrap")
call <SID>BinOptionL("wrap")
call <SID>PrtOptDesc("linebreak")
call <SID>BinOptionL("lbr")
call <SID>PrtOptDesc("breakindent")
call <SID>BinOptionL("bri")
call <SID>PrtOptDesc("breakindentopt")
call <SID>OptionL("briopt")
call <SID>PrtOptDesc("breakat")
call <SID>OptionG("brk", &brk)
call <SID>PrtOptDesc("showbreak")
call <SID>OptionG("sbr", &sbr)
call <SID>PrtOptDesc("sidescroll")
call append("$", " \tset ss=" . &ss)
call <SID>PrtOptDesc("sidescrolloff")
call append("$", " \tset siso=" . &siso)
call <SID>PrtOptDesc("display")
call <SID>OptionG("dy", &dy)
call <SID>PrtOptDesc("fillchars")
call <SID>OptionG("fcs", &fcs)
call <SID>PrtOptDesc("cmdheight")
call append("$", " \tset ch=" . &ch)
call <SID>PrtOptDesc("columns")
call append("$", " \tset co=" . &co)
call <SID>PrtOptDesc("lines")
call append("$", " \tset lines=" . &lines)
call <SID>PrtOptDesc("window")
call append("$", " \tset window=" . &window)
call <SID>PrtOptDesc("lazyredraw")
call <SID>BinOptionG("lz", &lz)
if has("reltime")
  call <SID>PrtOptDesc("redrawtime")
  call append("$", " \tset rdt=" . &rdt)
endif
call <SID>PrtOptDesc("writedelay")
call append("$", " \tset wd=" . &wd)
call <SID>PrtOptDesc("list")
call <SID>BinOptionL("list")
call <SID>PrtOptDesc("listchars")
call <SID>OptionG("lcs", &lcs)
call <SID>PrtOptDesc("number")
call <SID>BinOptionL("nu")
call <SID>PrtOptDesc("relativenumber")
call <SID>BinOptionL("rnu")
if has("linebreak")
  call <SID>PrtOptDesc("numberwidth")
  call <SID>OptionL("nuw")
endif
if has("conceal")
  call <SID>PrtOptDesc("conceallevel")
  call <SID>OptionL("cole")
call <SID>PrtOptDesc("concealcursor")
  call <SID>OptionL("cocu")
endif


call <SID>Header(5)
call <SID>PrtOptDesc("background")
call <SID>OptionG("bg", &bg)
call <SID>PrtOptDesc("filetype")
call <SID>OptionL("ft")
if has("syntax")
  call <SID>PrtOptDesc("syntax")
  call <SID>OptionL("syn")
  call <SID>PrtOptDesc("synmaxcol")
  call <SID>OptionL("smc")
endif
call <SID>PrtOptDesc("highlight")
call <SID>OptionG("hl", &hl)
call <SID>PrtOptDesc("hlsearch")
call <SID>BinOptionG("hls", &hls)
call <SID>PrtOptDesc("wincolor")
call <SID>OptionL("wcr")
if has("termguicolors")
  call <SID>PrtOptDesc("termguicolors")
  call <SID>BinOptionG("tgc", &tgc)
endif
if has("syntax")
  call <SID>PrtOptDesc("cursorcolumn")
  call <SID>BinOptionL("cuc")
  call <SID>PrtOptDesc("cursorline")
  call <SID>BinOptionL("cul")
  call <SID>PrtOptDesc("cursorlineopt")
  call <SID>OptionL("culopt")
  call <SID>PrtOptDesc("colorcolumn")
  call <SID>OptionL("cc")
  call <SID>PrtOptDesc("spell")
  call <SID>BinOptionL("spell")
  call <SID>PrtOptDesc("spelllang")
  call <SID>OptionL("spl")
  call <SID>PrtOptDesc("spellfile")
  call <SID>OptionL("spf")
  call <SID>PrtOptDesc("spellcapcheck")
  call <SID>OptionL("spc")
  call <SID>PrtOptDesc("spelloptions")
  call <SID>OptionL("spo")
  call <SID>PrtOptDesc("spellsuggest")
  call <SID>OptionG("sps", &sps)
  call <SID>PrtOptDesc("mkspellmem")
  call <SID>OptionG("msm", &msm)
endif


call <SID>Header(6)
call <SID>PrtOptDesc("laststatus")
call append("$", " \tset ls=" . &ls)
if has("statusline")
  call <SID>PrtOptDesc("statusline")
  call <SID>OptionG("stl", &stl)
endif
call <SID>PrtOptDesc("equalalways")
call <SID>BinOptionG("ea", &ea)
call <SID>PrtOptDesc("eadirection")
call <SID>OptionG("ead", &ead)
call <SID>PrtOptDesc("winheight")
call append("$", " \tset wh=" . &wh)
call <SID>PrtOptDesc("winminheight")
call append("$", " \tset wmh=" . &wmh)
call <SID>PrtOptDesc("winfixheight")
call <SID>BinOptionL("wfh")
call <SID>PrtOptDesc("winfixwidth")
call <SID>BinOptionL("wfw")
call <SID>PrtOptDesc("winwidth")
call append("$", " \tset wiw=" . &wiw)
call <SID>PrtOptDesc("winminwidth")
call append("$", " \tset wmw=" . &wmw)
call <SID>PrtOptDesc("helpheight")
call append("$", " \tset hh=" . &hh)
if has("quickfix")
  call <SID>PrtOptDesc("previewpopup")
  call append("$", " \tset pvp=" . &pvp)
  call <SID>PrtOptDesc("previewheight")
  call append("$", " \tset pvh=" . &pvh)
  call <SID>PrtOptDesc("previewwindow")
  call <SID>BinOptionL("pvw")
endif
call <SID>PrtOptDesc("hidden")
call <SID>BinOptionG("hid", &hid)
call <SID>PrtOptDesc("switchbuf")
call <SID>OptionG("swb", &swb)
call <SID>PrtOptDesc("splitbelow")
call <SID>BinOptionG("sb", &sb)
call <SID>PrtOptDesc("splitright")
call <SID>BinOptionG("spr", &spr)
call <SID>PrtOptDesc("scrollbind")
call <SID>BinOptionL("scb")
call <SID>PrtOptDesc("scrollopt")
call <SID>OptionG("sbo", &sbo)
call <SID>PrtOptDesc("cursorbind")
call <SID>BinOptionL("crb")
if has("terminal")
  call <SID>PrtOptDesc("termwinsize")
  call <SID>OptionL("tws")
  call <SID>PrtOptDesc("termwinkey")
  call <SID>OptionL("twk")
  call <SID>PrtOptDesc("termwinscroll")
  call <SID>OptionL("twsl")
  if has('win32')
    call <SID>PrtOptDesc("termwintype")
    call <SID>OptionG("twt", &twt)
  endif
  if exists("&winptydll")
    call <SID>PrtOptDesc("winptydll")
    call <SID>OptionG("winptydll", &winptydll)
  endif
endif


call <SID>Header(7)
call <SID>PrtOptDesc("showtabline")
call append("$", " \tset stal=" . &stal)
call <SID>PrtOptDesc("tabpagemax")
call append("$", " \tset tpm=" . &tpm)
call <SID>PrtOptDesc("tabline")
call <SID>OptionG("tal", &tal)
if has("gui")
  call <SID>PrtOptDesc("guitablabel")
  call <SID>OptionG("gtl", &gtl)
  call <SID>PrtOptDesc("guitabtooltip")
  call <SID>OptionG("gtt", &gtt)
endif


call <SID>Header(8)
call <SID>PrtOptDesc("term")
call <SID>OptionG("term", &term)
call <SID>PrtOptDesc("ttytype")
call <SID>OptionG("tty", &tty)
call <SID>PrtOptDesc("ttybuiltin")
call <SID>BinOptionG("tbi", &tbi)
call <SID>PrtOptDesc("ttyfast")
call <SID>BinOptionG("tf", &tf)
call <SID>PrtOptDesc("weirdinvert")
call <SID>BinOptionG("wiv", &wiv)
call <SID>PrtOptDesc("esckeys")
call <SID>BinOptionG("ek", &ek)
call <SID>PrtOptDesc("scrolljump")
call append("$", " \tset sj=" . &sj)
call <SID>PrtOptDesc("ttyscroll")
call append("$", " \tset tsl=" . &tsl)
if has("gui") || has("win32")
  call <SID>PrtOptDesc("guicursor")
  call <SID>OptionG("gcr", &gcr)
endif
if has("title")
  let &title = s:old_title
  call <SID>PrtOptDesc("title")
  call <SID>BinOptionG("title", &title)
  set notitle
  call <SID>PrtOptDesc("titlelen")
  call append("$", " \tset titlelen=" . &titlelen)
  call <SID>PrtOptDesc("titlestring")
  call <SID>OptionG("titlestring", &titlestring)
  call <SID>PrtOptDesc("titleold")
  call <SID>OptionG("titleold", &titleold)
  let &icon = s:old_icon
  call <SID>PrtOptDesc("icon")
  call <SID>BinOptionG("icon", &icon)
  set noicon
  call <SID>PrtOptDesc("iconstring")
  call <SID>OptionG("iconstring", &iconstring)
endif
if has("win32")
  call <SID>PrtOptDesc("restorescreen")
  call <SID>BinOptionG("rs", &rs)
endif


call <SID>Header(9)
call <SID>PrtOptDesc("mouse")
call <SID>OptionG("mouse", &mouse)
if has("gui")
  call <SID>PrtOptDesc("mousefocus")
  call <SID>BinOptionG("mousef", &mousef)
endif
call <SID>PrtOptDesc("scrollfocus")
call <SID>BinOptionG("scf", &scf)
if has("gui")
  call <SID>PrtOptDesc("mousehide")
  call <SID>BinOptionG("mh", &mh)
endif
call <SID>PrtOptDesc("mousemodel")
call <SID>OptionG("mousem", &mousem)
call <SID>PrtOptDesc("mousetime")
call append("$", " \tset mouset=" . &mouset)
call <SID>PrtOptDesc("ttymouse")
call <SID>OptionG("ttym", &ttym)
if has("mouseshape")
  call <SID>PrtOptDesc("mouseshape")
  call <SID>OptionG("mouses", &mouses)
endif


if has("gui")
  call <SID>Header(10)
  call <SID>PrtOptDesc("guifont")
  call <SID>OptionG("gfn", &gfn)
  if has("xfontset")
    call <SID>PrtOptDesc("guifontset")
    call <SID>OptionG("gfs", &gfs)
  endif
  call <SID>PrtOptDesc("guifontwide")
  call <SID>OptionG("gfw", &gfw)
  if has("mac")
    call <SID>PrtOptDesc("antialias")
    call <SID>BinOptionG("anti", &anti)
  endif
  call <SID>PrtOptDesc("guioptions")
  call <SID>OptionG("go", &go)
  if has("gui_gtk")
    call <SID>PrtOptDesc("toolbar")
    call <SID>OptionG("tb", &tb)
    if has("gui_gtk2")
      call <SID>PrtOptDesc("toolbariconsize")
      call <SID>OptionG("tbis", &tbis)
    endif
    call <SID>PrtOptDesc("guiheadroom")
    call append("$", " \tset ghr=" . &ghr)
  endif
  if has("directx")
    call <SID>PrtOptDesc("renderoptions")
    call <SID>OptionG("rop", &rop)
  endif
  call <SID>PrtOptDesc("guipty")
  call <SID>BinOptionG("guipty", &guipty)
  if has("browse")
    call <SID>PrtOptDesc("browsedir")
    call <SID>OptionG("bsdir", &bsdir)
  endif
  if has("multi_lang")
    call <SID>PrtOptDesc("langmenu")
    call <SID>OptionG("langmenu", &lm)
  endif
  call <SID>PrtOptDesc("menuitems")
  call append("$", " \tset mis=" . &mis)
  if has("winaltkeys")
    call <SID>PrtOptDesc("winaltkeys")
    call <SID>OptionG("wak", &wak)
  endif
  call <SID>PrtOptDesc("linespace")
  call append("$", " \tset lsp=" . &lsp)
  if has("balloon_eval") || has("balloon_eval_term")
    call <SID>PrtOptDesc("balloondelay")
    call append("$", " \tset bdlay=" . &bdlay)
    if has("balloon_eval")
      call <SID>PrtOptDesc("ballooneval")
      call <SID>BinOptionG("beval", &beval)
    endif
    if has("balloon_eval_term")
      call <SID>PrtOptDesc("balloonevalterm")
      call <SID>BinOptionG("bevalterm", &beval)
    endif
    if has("eval")
      call <SID>PrtOptDesc("balloonexpr")
      call append("$", " \tset bexpr=" . &bexpr)
    endif
  endif
  if exists("+macatsui")
    call <SID>PrtOptDesc("macatsui")
    call <SID>OptionG("macatsui", &macatsui)
  endif
endif

if has("printer")
  call <SID>Header(11)
  call <SID>PrtOptDesc("printoptions")
  call <SID>OptionG("popt", &popt)
  call <SID>PrtOptDesc("printdevice")
  call <SID>OptionG("pdev", &pdev)
  if has("postscript")
    call <SID>PrtOptDesc("printexpr")
    call <SID>OptionG("pexpr", &pexpr)
  endif
  call <SID>PrtOptDesc("printfont")
  call <SID>OptionG("pfn", &pfn)
  call <SID>PrtOptDesc("printheader")
  call <SID>OptionG("pheader", &pheader)
  if has("postscript")
    call <SID>PrtOptDesc("printencoding")
    call <SID>OptionG("penc", &penc)
  endif
  call <SID>PrtOptDesc("printmbcharset")
  call <SID>OptionG("pmbcs", &pmbcs)
  call <SID>PrtOptDesc("printmbfont")
  call <SID>OptionG("pmbfn", &pmbfn)
endif

call <SID>Header(12)
call <SID>PrtOptDesc("terse")
call <SID>BinOptionG("terse", &terse)
call <SID>PrtOptDesc("shortmess")
call <SID>OptionG("shm", &shm)
call <SID>PrtOptDesc("showcmd")
let &sc = s:old_sc
call <SID>BinOptionG("sc", &sc)
set nosc
call <SID>PrtOptDesc("showmode")
call <SID>BinOptionG("smd", &smd)
call <SID>PrtOptDesc("ruler")
let &ru = s:old_ru
call <SID>BinOptionG("ru", &ru)
set noru
if has("statusline")
  call <SID>PrtOptDesc("rulerformat")
  call <SID>OptionG("ruf", &ruf)
endif
call <SID>PrtOptDesc("report")
call append("$", " \tset report=" . &report)
call <SID>PrtOptDesc("verbose")
call append("$", " \tset vbs=" . &vbs)
call <SID>PrtOptDesc("verbosefile")
call <SID>OptionG("vfile", &vfile)
call <SID>PrtOptDesc("more")
call <SID>BinOptionG("more", &more)
if has("dialog_con") || has("dialog_gui")
  call <SID>PrtOptDesc("confirm")
  call <SID>BinOptionG("cf", &cf)
endif
call <SID>PrtOptDesc("errorbells")
call <SID>BinOptionG("eb", &eb)
call <SID>PrtOptDesc("visualbell")
call <SID>BinOptionG("vb", &vb)
call <SID>PrtOptDesc("belloff")
call <SID>OptionG("belloff", &belloff)
if has("multi_lang")
  call <SID>PrtOptDesc("helplang")
  call <SID>OptionG("hlg", &hlg)
endif


call <SID>Header(13)
call <SID>PrtOptDesc("selection")
call <SID>OptionG("sel", &sel)
call <SID>PrtOptDesc("selectmode")
call <SID>OptionG("slm", &slm)
if has("clipboard")
  call <SID>PrtOptDesc("clipboard")
  call <SID>OptionG("cb", &cb)
endif
call <SID>PrtOptDesc("keymodel")
call <SID>OptionG("km", &km)


call <SID>Header(14)
call <SID>PrtOptDesc("undolevels")
call append("$", " \tset ul=" . s:old_ul)
call <SID>PrtOptDesc("undofile")
call <SID>BinOptionG("udf", &udf)
call <SID>PrtOptDesc("undodir")
call <SID>OptionG("udir", &udir)
call <SID>PrtOptDesc("undoreload")
call append("$", " \tset ur=" . &ur)
call <SID>PrtOptDesc("modified")
call <SID>BinOptionL("mod")
call <SID>PrtOptDesc("readonly")
call <SID>BinOptionL("ro")
call <SID>PrtOptDesc("modifiable")
call <SID>BinOptionL("ma")
call <SID>PrtOptDesc("textwidth")
call <SID>OptionL("tw")
call <SID>PrtOptDesc("wrapmargin")
call <SID>OptionL("wm")
call <SID>PrtOptDesc("backspace")
call append("$", " \tset bs=" . &bs)
call <SID>PrtOptDesc("comments")
call <SID>OptionL("com")
call <SID>PrtOptDesc("formatoptions")
call <SID>OptionL("fo")
call <SID>PrtOptDesc("formatlistpat")
call <SID>OptionL("flp")
if has("eval")
  call <SID>PrtOptDesc("formatexpr")
  call <SID>OptionL("fex")
endif
if has("insert_expand")
  call <SID>PrtOptDesc("complete")
  call <SID>OptionL("cpt")
  call <SID>PrtOptDesc("completeopt")
  call <SID>OptionG("cot", &cot)
  if exists("+completepopup")
    call <SID>PrtOptDesc("completepopup")
    call <SID>OptionG("cpp", &cpp)
  endif
  call <SID>PrtOptDesc("pumheight")
  call <SID>OptionG("ph", &ph)
  call <SID>PrtOptDesc("pumwidth")
  call <SID>OptionG("pw", &pw)
  call <SID>PrtOptDesc("completefunc")
  call <SID>OptionL("cfu")
  call <SID>PrtOptDesc("omnifunc")
  call <SID>OptionL("ofu")
  call <SID>PrtOptDesc("dictionary")
  call <SID>OptionG("dict", &dict)
  call <SID>PrtOptDesc("thesaurus")
  call <SID>OptionG("tsr", &tsr)
endif
call <SID>PrtOptDesc("infercase")
call <SID>BinOptionL("inf")
if has("digraphs")
  call <SID>PrtOptDesc("digraph")
  call <SID>BinOptionG("dg", &dg)
endif
call <SID>PrtOptDesc("tildeop")
call <SID>BinOptionG("top", &top)
call <SID>PrtOptDesc("operatorfunc")
call <SID>OptionG("opfunc", &opfunc)
call <SID>PrtOptDesc("showmatch")
call <SID>BinOptionG("sm", &sm)
call <SID>PrtOptDesc("matchtime")
call append("$", " \tset mat=" . &mat)
call <SID>PrtOptDesc("matchpairs")
call <SID>OptionL("mps")
call <SID>PrtOptDesc("joinspaces")
call <SID>BinOptionG("js", &js)
call <SID>PrtOptDesc("nrformats")
call <SID>OptionL("nf")


call <SID>Header(15)
call <SID>PrtOptDesc("tabstop")
call <SID>OptionL("ts")
call <SID>PrtOptDesc("shiftwidth")
call <SID>OptionL("sw")
if has("vartabs")
  call <SID>PrtOptDesc("vartabstop")
  call <SID>OptionL("vts")
  call <SID>PrtOptDesc("varsofttabstop")
  call <SID>OptionL("vsts")
endif
call <SID>PrtOptDesc("smarttab")
call <SID>BinOptionG("sta", &sta)
call <SID>PrtOptDesc("softtabstop")
call <SID>OptionL("sts")
call <SID>PrtOptDesc("shiftround")
call <SID>BinOptionG("sr", &sr)
call <SID>PrtOptDesc("expandtab")
call <SID>BinOptionL("et")
call <SID>PrtOptDesc("autoindent")
call <SID>BinOptionL("ai")
if has("smartindent")
  call <SID>PrtOptDesc("smartindent")
  call <SID>BinOptionL("si")
endif
if has("cindent")
  call <SID>PrtOptDesc("cindent")
  call <SID>BinOptionL("cin")
  call <SID>PrtOptDesc("cinoptions")
  call <SID>OptionL("cino")
  call <SID>PrtOptDesc("cinkeys")
  call <SID>OptionL("cink")
  call <SID>PrtOptDesc("cinwords")
  call <SID>OptionL("cinw")
  call <SID>PrtOptDesc("indentexpr")
  call <SID>OptionL("inde")
  call <SID>PrtOptDesc("indentkeys")
  call <SID>OptionL("indk")
endif
call <SID>PrtOptDesc("copyindent")
call <SID>BinOptionL("ci")
call <SID>PrtOptDesc("preserveindent")
call <SID>BinOptionL("pi")
if has("lispindent")
  call <SID>PrtOptDesc("lisp")
  call <SID>BinOptionL("lisp")
  call <SID>PrtOptDesc("lispwords")
  call <SID>OptionL("lw")
endif


if has("folding")
  call <SID>Header(16)
  call <SID>PrtOptDesc("foldenable")
  call <SID>BinOptionL("fen")
  call <SID>PrtOptDesc("foldlevel")
  call <SID>OptionL("fdl")
  call <SID>PrtOptDesc("foldlevelstart")
  call append("$", " \tset fdls=" . &fdls)
  call <SID>PrtOptDesc("foldcolumn")
  call <SID>OptionL("fdc")
  call <SID>PrtOptDesc("foldtext")
  call <SID>OptionL("fdt")
  call <SID>PrtOptDesc("foldclose")
  call <SID>OptionG("fcl", &fcl)
  call <SID>PrtOptDesc("foldopen")
  call <SID>OptionG("fdo", &fdo)
  call <SID>PrtOptDesc("foldminlines")
  call <SID>OptionL("fml")
  call <SID>PrtOptDesc("commentstring")
  call <SID>OptionL("cms")
  call <SID>PrtOptDesc("foldmethod")
  call <SID>OptionL("fdm")
  call <SID>PrtOptDesc("foldexpr")
  call <SID>OptionL("fde")
  call <SID>PrtOptDesc("foldignore")
  call <SID>OptionL("fdi")
  call <SID>PrtOptDesc("foldmarker")
  call <SID>OptionL("fmr")
  call <SID>PrtOptDesc("foldnestmax")
  call <SID>OptionL("fdn")
endif


if has("diff")
  call <SID>Header(17)
  call <SID>PrtOptDesc("diff")
  call <SID>BinOptionL("diff")
  call <SID>PrtOptDesc("diffopt")
  call <SID>OptionG("dip", &dip)
  call <SID>PrtOptDesc("diffexpr")
  call <SID>OptionG("dex", &dex)
  call <SID>PrtOptDesc("patchexpr")
  call <SID>OptionG("pex", &pex)
endif


call <SID>Header(18)
call <SID>PrtOptDesc("maxmapdepth")
call append("$", " \tset mmd=" . &mmd)
call <SID>PrtOptDesc("remap")
call <SID>BinOptionG("remap", &remap)
call <SID>PrtOptDesc("timeout")
call <SID>BinOptionG("to", &to)
call <SID>PrtOptDesc("ttimeout")
call <SID>BinOptionG("ttimeout", &ttimeout)
call <SID>PrtOptDesc("timeoutlen")
call append("$", " \tset tm=" . &tm)
call <SID>PrtOptDesc("ttimeoutlen")
call append("$", " \tset ttm=" . &ttm)


call <SID>Header(19)
call <SID>PrtOptDesc("modeline")
call <SID>BinOptionL("ml")
call <SID>PrtOptDesc("modelineexpr")
call <SID>BinOptionG("mle", &mle)
call <SID>PrtOptDesc("modelines")
call append("$", " \tset mls=" . &mls)
call <SID>PrtOptDesc("binary")
call <SID>BinOptionL("bin")
call <SID>PrtOptDesc("endofline")
call <SID>BinOptionL("eol")
call <SID>PrtOptDesc("fixendofline")
call <SID>BinOptionL("fixeol")
call <SID>PrtOptDesc("bomb")
call <SID>BinOptionL("bomb")
call <SID>PrtOptDesc("fileformat")
call <SID>OptionL("ff")
call <SID>PrtOptDesc("fileformats")
call <SID>OptionG("ffs", &ffs)
call <SID>PrtOptDesc("textmode")
call <SID>BinOptionL("tx")
call <SID>PrtOptDesc("textauto")
call <SID>BinOptionG("ta", &ta)
call <SID>PrtOptDesc("write")
call <SID>BinOptionG("write", &write)
call <SID>PrtOptDesc("writebackup")
call <SID>BinOptionG("wb", &wb)
call <SID>PrtOptDesc("backup")
call <SID>BinOptionG("bk", &bk)
call <SID>PrtOptDesc("backupskip")
call append("$", " \tset bsk=" . &bsk)
call <SID>PrtOptDesc("backupcopy")
call append("$", " \tset bkc=" . &bkc)
call <SID>PrtOptDesc("backupdir")
call <SID>OptionG("bdir", &bdir)
call <SID>PrtOptDesc("backupext")
call <SID>OptionG("bex", &bex)
call <SID>PrtOptDesc("autowrite")
call <SID>BinOptionG("aw", &aw)
call <SID>PrtOptDesc("autowriteall")
call <SID>BinOptionG("awa", &awa)
call <SID>PrtOptDesc("writeany")
call <SID>BinOptionG("wa", &wa)
call <SID>PrtOptDesc("autoread")
call <SID>BinOptionG("ar", &ar)
call <SID>PrtOptDesc("patchmode")
call <SID>OptionG("pm", &pm)
call <SID>PrtOptDesc("fsync")
call <SID>BinOptionG("fs", &fs)
call <SID>PrtOptDesc("shortname")
call <SID>BinOptionL("sn")
call <SID>PrtOptDesc("cryptmethod")
call <SID>OptionL("cm")


call <SID>Header(20)
call <SID>PrtOptDesc("directory")
call <SID>OptionG("dir", &dir)
call <SID>PrtOptDesc("swapfile")
call <SID>BinOptionL("swf")
call <SID>PrtOptDesc("swapsync")
call <SID>OptionG("sws", &sws)
call <SID>PrtOptDesc("updatecount")
call append("$", " \tset uc=" . &uc)
call <SID>PrtOptDesc("updatetime")
call append("$", " \tset ut=" . &ut)
call <SID>PrtOptDesc("maxmem")
call append("$", " \tset mm=" . &mm)
call <SID>PrtOptDesc("maxmemtot")
call append("$", " \tset mmt=" . &mmt)


call <SID>Header(21)
call <SID>PrtOptDesc("history")
call append("$", " \tset hi=" . &hi)
call <SID>PrtOptDesc("wildchar")
call append("$", " \tset wc=" . &wc)
call <SID>PrtOptDesc("wildcharm")
call append("$", " \tset wcm=" . &wcm)
call <SID>PrtOptDesc("wildmode")
call <SID>OptionG("wim", &wim)
if has("wildoptions")
  call <SID>PrtOptDesc("wildoptions")
  call <SID>OptionG("wop", &wop)
endif
call <SID>PrtOptDesc("suffixes")
call <SID>OptionG("su", &su)
if has("file_in_path")
  call <SID>PrtOptDesc("suffixesadd")
  call <SID>OptionL("sua")
endif
if has("wildignore")
  call <SID>PrtOptDesc("wildignore")
  call <SID>OptionG("wig", &wig)
endif
call <SID>PrtOptDesc("fileignorecase")
call <SID>BinOptionG("fic", &fic)
call <SID>PrtOptDesc("wildignorecase")
call <SID>BinOptionG("wic", &wic)
if has("wildmenu")
  call <SID>PrtOptDesc("wildmenu")
  call <SID>BinOptionG("wmnu", &wmnu)
endif
call <SID>PrtOptDesc("cedit")
call <SID>OptionG("cedit", &cedit)
call <SID>PrtOptDesc("cmdwinheight")
call <SID>OptionG("cwh", &cwh)


call <SID>Header(22)
call <SID>PrtOptDesc("shell")
call <SID>OptionG("sh", &sh)
if has("amiga")
  call <SID>PrtOptDesc("shelltype")
  call append("$", " \tset st=" . &st)
endif
call <SID>PrtOptDesc("shellquote")
call <SID>OptionG("shq", &shq)
call <SID>PrtOptDesc("shellxquote")
call <SID>OptionG("sxq", &sxq)
call <SID>PrtOptDesc("shellxescape")
call <SID>OptionG("sxe", &sxe)
call <SID>PrtOptDesc("shellcmdflag")
call <SID>OptionG("shcf", &shcf)
call <SID>PrtOptDesc("shellredir")
call <SID>OptionG("srr", &srr)
call <SID>PrtOptDesc("shelltemp")
call <SID>BinOptionG("stmp", &stmp)
call <SID>PrtOptDesc("equalprg")
call <SID>OptionG("ep", &ep)
call <SID>PrtOptDesc("formatprg")
call <SID>OptionG("fp", &fp)
call <SID>PrtOptDesc("keywordprg")
call <SID>OptionG("kp", &kp)
call <SID>PrtOptDesc("warn")
call <SID>BinOptionG("warn", &warn)


if has("quickfix")
  call <SID>Header(23)
  call <SID>PrtOptDesc("errorfile")
  call <SID>OptionG("ef", &ef)
  call <SID>PrtOptDesc("errorformat")
  call <SID>OptionG("efm", &efm)
  call <SID>PrtOptDesc("makeprg")
  call <SID>OptionG("mp", &mp)
  call <SID>PrtOptDesc("shellpipe")
  call <SID>OptionG("sp", &sp)
  call <SID>PrtOptDesc("makeef")
  call <SID>OptionG("mef", &mef)
  call <SID>PrtOptDesc("grepprg")
  call <SID>OptionG("gp", &gp)
  call <SID>PrtOptDesc("grepformat")
  call <SID>OptionG("gfm", &gfm)
  call <SID>PrtOptDesc("makeencoding")
  call <SID>OptionG("menc", &menc)
  call <SID>PrtOptDesc("quickfixtextfunc")
  call <SID>OptionG("qftf", &qftf)
endif


if has("win32") || has("osfiletype")
  call <SID>Header(24)
  if has("osfiletype")
    call <SID>PrtOptDesc("osfiletype")
    call <SID>OptionL("oft")
  endif
  if has("win32")
    call <SID>PrtOptDesc("shellslash")
    call <SID>BinOptionG("ssl", &ssl)
    call <SID>PrtOptDesc("completeslash")
    call <SID>OptionG("csl", &csl)
  endif
endif


call <SID>Header(25)
call <SID>PrtOptDesc("isfname")
call <SID>OptionG("isf", &isf)
call <SID>PrtOptDesc("isident")
call <SID>OptionG("isi", &isi)
call <SID>PrtOptDesc("iskeyword")
call <SID>OptionL("isk")
call <SID>PrtOptDesc("isprint")
call <SID>OptionG("isp", &isp)
if has("textobjects")
  call <SID>PrtOptDesc("quoteescape")
  call <SID>OptionL("qe")
endif
if has("rightleft")
  call <SID>PrtOptDesc("rightleft")
  call <SID>BinOptionL("rl")
  call <SID>PrtOptDesc("rightleftcmd")
  call <SID>OptionL("rlc")
  call <SID>PrtOptDesc("revins")
  call <SID>BinOptionG("ri", &ri)
  call <SID>PrtOptDesc("allowrevins")
  call <SID>BinOptionG("ari", &ari)
  call <SID>PrtOptDesc("aleph")
  call append("$", " \tset al=" . &al)
  call <SID>PrtOptDesc("hkmap")
  call <SID>BinOptionG("hk", &hk)
  call <SID>PrtOptDesc("hkmapp")
  call <SID>BinOptionG("hkp", &hkp)
endif
if has("farsi")
  call <SID>PrtOptDesc("altkeymap")
  call <SID>BinOptionG("akm", &akm)
  call <SID>PrtOptDesc("fkmap")
  call <SID>BinOptionG("fk", &fk)
endif
if has("arabic")
  call <SID>PrtOptDesc("arabic")
  call <SID>BinOptionL("arab")
  call <SID>PrtOptDesc("arabicshape")
  call <SID>BinOptionG("arshape", &arshape)
  call <SID>PrtOptDesc("termbidi")
  call <SID>BinOptionG("tbidi", &tbidi)
endif
if has("keymap")
  call <SID>PrtOptDesc("keymap")
  call <SID>OptionL("kmp")
endif
if has("langmap")
  call <SID>PrtOptDesc("langmap")
  call <SID>OptionG("lmap", &lmap)
  call <SID>PrtOptDesc("langremap")
  call <SID>BinOptionG("lrm", &lrm)
endif
if has("xim")
  call <SID>PrtOptDesc("imdisable")
  call <SID>BinOptionG("imd", &imd)
endif
call <SID>PrtOptDesc("iminsert")
call <SID>OptionL("imi")
call <SID>PrtOptDesc("imstyle")
call <SID>OptionG("imst", &imst)
call <SID>PrtOptDesc("imsearch")
call <SID>OptionL("ims")
if has("xim")
  call <SID>PrtOptDesc("imcmdline")
  call <SID>BinOptionG("imc", &imc)
  call <SID>PrtOptDesc("imstatusfunc")
  call <SID>OptionG("imsf", &imsf)
  call <SID>PrtOptDesc("imactivatefunc")
  call <SID>OptionG("imaf", &imaf)
endif


call <SID>Header(26)
call <SID>PrtOptDesc("encoding")
call <SID>OptionG("enc", &enc)
call <SID>PrtOptDesc("fileencoding")
call <SID>OptionL("fenc")
call <SID>PrtOptDesc("fileencodings")
call <SID>OptionG("fencs", &fencs)
call <SID>PrtOptDesc("termencoding")
call <SID>OptionG("tenc", &tenc)
call <SID>PrtOptDesc("charconvert")
call <SID>OptionG("ccv", &ccv)
call <SID>PrtOptDesc("delcombine")
call <SID>BinOptionG("deco", &deco)
call <SID>PrtOptDesc("maxcombine")
call <SID>OptionG("mco", &mco)
if has("xim") && has("gui_gtk")
  call <SID>PrtOptDesc("imactivatekey")
  call <SID>OptionG("imak", &imak)
endif
call <SID>PrtOptDesc("ambiwidth")
call <SID>OptionG("ambw", &ambw)
call <SID>PrtOptDesc("emoji")
call <SID>BinOptionG("emo", &emo)


call <SID>Header(27)
call <SID>PrtOptDesc("virtualedit")
call <SID>OptionG("ve", &ve)
call <SID>PrtOptDesc("eventignore")
call <SID>OptionG("ei", &ei)
call <SID>PrtOptDesc("loadplugins")
call <SID>BinOptionG("lpl", &lpl)
call <SID>PrtOptDesc("exrc")
call <SID>BinOptionG("ex", &ex)
call <SID>PrtOptDesc("secure")
call <SID>BinOptionG("secure", &secure)
call <SID>PrtOptDesc("gdefault")
call <SID>BinOptionG("gd", &gd)
call <SID>PrtOptDesc("edcompatible")
call <SID>BinOptionG("ed", &ed)
if exists("+opendevice")
  call <SID>PrtOptDesc("opendevice")
  call <SID>BinOptionG("odev", &odev)
endif
if exists("+maxfuncdepth")
  call <SID>PrtOptDesc("maxfuncdepth")
  call append("$", " \tset mfd=" . &mfd)
endif
if has("mksession")
  call <SID>PrtOptDesc("sessionoptions")
  call <SID>OptionG("ssop", &ssop)
  call <SID>PrtOptDesc("viewoptions")
  call <SID>OptionG("vop", &vop)
  call <SID>PrtOptDesc("viewdir")
  call <SID>OptionG("vdir", &vdir)
endif
if has("viminfo")
  call <SID>PrtOptDesc("viminfo")
  call <SID>OptionG("vi", &vi)
  call <SID>PrtOptDesc("viminfofile")
  call <SID>OptionG("vif", &vif)
endif
if has("quickfix")
  call <SID>PrtOptDesc("bufhidden")
  call <SID>OptionL("bh")
  call <SID>PrtOptDesc("buftype")
  call <SID>OptionL("bt")
endif
call <SID>PrtOptDesc("buflisted")
call <SID>BinOptionL("bl")
call <SID>PrtOptDesc("debug")
if has("signs")
  call <SID>PrtOptDesc("signcolumn")
  call <SID>OptionL("scl")
endif
if has("mzscheme")
  call <SID>PrtOptDesc("mzquantum")
  call append("$", " \tset mzq=" . &mzq)
endif
if exists("&luadll")
  call <SID>PrtOptDesc("luadll")
  call <SID>OptionG("luadll", &luadll)
endif
if exists("&perldll")
  call <SID>PrtOptDesc("perldll")
  call <SID>OptionG("perldll", &perldll)
endif
if has('pythonx')
  call <SID>PrtOptDesc("pyxversion")
  call append("$", " \tset pyx=" . &wd)
endif
if exists("&pythondll")
  call <SID>PrtOptDesc("pythondll")
  call <SID>OptionG("pythondll", &pythondll)
endif
if exists("&pythonhome")
  call <SID>PrtOptDesc("pythonhome")
  call <SID>OptionG("pythonhome", &pythonhome)
endif
if exists("&pythonthreedll")
  call <SID>PrtOptDesc("pythonthreedll")
  call <SID>OptionG("pythonthreedll", &pythonthreedll)
endif
if exists("&pythonthreehome")
  call <SID>PrtOptDesc("pythonthreehome")
  call <SID>OptionG("pythonthreehome", &pythonthreehome)
endif
if exists("&rubydll")
  call <SID>PrtOptDesc("rubydll")
  call <SID>OptionG("rubydll", &rubydll)
endif
if exists("&tcldll")
  call <SID>PrtOptDesc("tcldll")
  call <SID>OptionG("tcldll", &tcldll)
endif
if exists("&mzschemedll")
  call <SID>PrtOptDesc("mzschemedll")
  call <SID>OptionG("mzschemedll", &mzschemedll)
  call <SID>PrtOptDesc("mzschemegcdll")
  call <SID>OptionG("mzschemegcdll", &mzschemegcdll)
endif

set cpo&vim
" go to first line
1

" reset 'modified', so that ":q" can be used to close the window
setlocal nomodified

if has("syntax")
  " Use Vim highlighting, with some additional stuff
  setlocal ft=vim
  syn match optwinHeader "^ \=[0-9].*"
  syn match optwinName "^[a-z]*\t" nextgroup=optwinComment
  syn match optwinComment ".*" contained
  syn match optwinComment "^\t.*"
  if !exists("did_optwin_syntax_inits")
    let did_optwin_syntax_inits = 1
    hi link optwinHeader Title
    hi link optwinName Identifier
    hi link optwinComment Comment
  endif
endif

" Install autocommands to enable mappings in option-window
noremap <silent> <buffer> <CR> <C-\><C-N>:call <SID>CR()<CR>
inoremap <silent> <buffer> <CR> <Esc>:call <SID>CR()<CR>
noremap <silent> <buffer> <Space> :call <SID>Space()<CR>

" Make the buffer be deleted when the window is closed.
setlocal buftype=nofile bufhidden=delete noswapfile

augroup optwin
  au! BufUnload,BufHidden option-window nested
	\ call <SID>unload() | delfun <SID>unload
augroup END

fun! <SID>unload()
  delfun <SID>CR
  delfun <SID>Space
  delfun <SID>Find
  delfun <SID>Update
  delfun <SID>OptionL
  delfun <SID>OptionG
  delfun <SID>BinOptionL
  delfun <SID>BinOptionG
  delfun <SID>Header
  delfun <SID>PrtOptDesc
  au! optwin
endfun

" Restore the previous value of 'title' and 'icon'.
let &title = s:old_title
let &icon = s:old_icon
let &ru = s:old_ru
let &sc = s:old_sc
let &cpo = s:cpo_save
let &ul = s:old_ul
unlet s:old_title s:old_icon s:old_ru s:old_sc s:cpo_save s:idx s:lnum s:old_ul
unlet s:banner s:contents s:optdesc s:contlen_ne s:optdesclen_ne
unlet s:contlen_en s:optdesclen_en s:optwin_fl

" vim: ts=8 sw=2 sts=2
