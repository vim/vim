vim9script
# Ex command ranges
# VIM_TEST_SETUP hi link vimRangeSeparator Todo
# VIM_TEST_SETUP hi link vimRangeOffset Type


:'<,'>print
:'(,')print
:'{,'}print
:'[,']print

echo | :'<,'>print
echo | :'(,')print
echo | :'{,'}print
echo | :'[,']print

# bare mark ranges

 :'a
: 'a
:'a
:'k
:'z
:'A
:'K
:'Z
:'0
:'9
:'[
:']
:'{
:'}
:'(
:')
:'<
:'>
:'`
:''
:'"
:'^
:'.

echo |:'a
echo| :'a
echo | :'a
echo | :'k
echo | :'z
echo | :'A
echo | :'K
echo | :'Z
echo | :'0
echo | :'9
echo | :'[
echo | :']
echo | :'{
echo | :'}
echo | :'(
echo | :')
echo | :'<
echo | :'>
echo | :'`
echo | :''
echo | :'"
echo | :'^
echo | :'.


:1,10:print

# range (with whitespace)

:1 match
:2 match
:3 match

:42,42 append
    text
.
:42,42 abbreviate
:42,42 abclear
:42,42 aboveleft
:42,42 all
:42,42 amenu
:42,42 anoremenu
:42,42 args
:42,42 argadd
:42,42 argdedupe
:42,42 argdelete
:42,42 argedit
:42,42 argdo
:42,42 argglobal
:42,42 arglocal
:42,42 argument
:42,42 ascii
:42,42 autocmd
:42,42 augroup Foo
:42,42 augroup END
:42,42 aunmenu
:42,42 buffer
:42,42 bNext
:42,42 ball
:42,42 badd
:42,42 balt
:42,42 bdelete
:42,42 behave mswin
:42,42 behave xterm
:42,42 belowright
:42,42 bfirst
:42,42 blast
:42,42 bmodified
:42,42 bnext
:42,42 botright
:42,42 bprevious
:42,42 brewind
:42,42 break
:42,42 breakadd
:42,42 breakdel
:42,42 breaklist
:42,42 browse
:42,42 bufdo
:42,42 buffers
:42,42 bunload
:42,42 bwipeout
:42,42 change
    text
.
:42,42 cNext
:42,42 cNfile
:42,42 cabbrev
:42,42 cabclear
:42,42 cabove
:42,42 caddbuffer
:42,42 caddexpr
:42,42 caddfile
:42,42 cafter
:42,42 call
:42,42 catch
:42,42 cbefore
:42,42 cbelow
:42,42 cbottom
:42,42 cbuffer
:42,42 cc
:42,42 cclose
:42,42 cd
:42,42 cdo
:42,42 cfdo
:42,42 center
:42,42 cexpr
:42,42 cfile
:42,42 cfirst
:42,42 cgetbuffer
:42,42 cgetexpr
:42,42 cgetfile
:42,42 changes
:42,42 chdir
:42,42 checkpath
:42,42 checktime
:42,42 chistory
:42,42 clast
:42,42 clearjumps
:42,42 clist
:42,42 close
:42,42 cmap
:42,42 cmapclear
:42,42 cmenu
:42,42 cnext
:42,42 cnewer
:42,42 cnfile
:42,42 cnoremap
:42,42 cnoreabbrev
:42,42 cnoremenu
:42,42 copy
:42,42 colder
:42,42 colorscheme
:42,42 command
:42,42 comclear
:42,42 compiler
:42,42 continue
:42,42 confirm
:42,42 const
:42,42 copen
:42,42 cprevious
:42,42 cpfile
:42,42 cquit
:42,42 crewind
:42,42 cscope
:42,42 cstag
:42,42 cunmap
:42,42 cunabbrev
:42,42 cunmenu
:42,42 cwindow
:42,42 delete
:42,42 debug
:42,42 debuggreedy
:42,42 def
:42,42 defcompile
:42,42 defer
:42,42 delcommand
:42,42 delfunction
:42,42 delmarks
:42,42 diffupdate
:42,42 diffget
:42,42 diffoff
:42,42 diffpatch
:42,42 diffput
:42,42 diffsplit
:42,42 diffthis
:42,42 digraphs
:42,42 display
:42,42 disassemble
:42,42 djump
:42,42 dl
:42,42 dlist
:42,42 doautocmd
:42,42 doautoall
:42,42 dp
:42,42 drop
:42,42 dsearch
:42,42 dsplit
:42,42 edit
:42,42 earlier
:42,42 echo
:42,42 echoconsole
:42,42 echoerr
:42,42 echohl
:42,42 echomsg
:42,42 echon
:42,42 echowindow
:42,42 else
:42,42 elseif
:42,42 emenu
:42,42 enddef
:42,42 endif
:42,42 endfor
:42,42 endfunction
:42,42 endtry
:42,42 endwhile
:42,42 enew
:42,42 eval
:42,42 ex
:42,42 execute
:42,42 exit
:42,42 exusage
:42,42 file
:42,42 files
:42,42 filetype
:42,42 filter
:42,42 find
:42,42 final
:42,42 finally
:42,42 finish
:42,42 first
:42,42 fixdel
:42,42 fold
:42,42 foldclose
:42,42 folddoopen
:42,42 folddoclosed
:42,42 foldopen
:42,42 for foo in bar | endfor
:42,42 function
:42,42 global/.../
:42,42 goto
:42,42 grep
:42,42 grepadd
:42,42 gui
:42,42 gvim
:42,42 hardcopy
:42,42 help
:42,42 helpclose
:42,42 helpfind
:42,42 helpgrep
:42,42 helptags
:42,42 highlight
:42,42 hide
:42,42 history
:42,42 horizontal
:42,42 insert
    text
.
:42,42 iabbrev
:42,42 iabclear
:42,42 if
:42,42 ijump
:42,42 ilist
:42,42 imap
:42,42 imapclear
:42,42 imenu
:42,42 import
:42,42 inoremap
:42,42 inoreabbrev
:42,42 inoremenu
:42,42 intro
:42,42 isearch
:42,42 isplit
:42,42 iunmap
:42,42 iunabbrev
:42,42 iunmenu
:42,42 join
:42,42 jumps
:42,42 k
:42,42 keepalt
:42,42 keepmarks
:42,42 keepjumps
:42,42 keeppatterns
:42,42 lNext
:42,42 lNfile
:42,42 list
:42,42 labove
:42,42 laddexpr
:42,42 laddbuffer
:42,42 laddfile
:42,42 lafter
:42,42 last
:42,42 language
:42,42 later
:42,42 lbefore
:42,42 lbelow
:42,42 lbottom
:42,42 lbuffer
:42,42 lcd
:42,42 lchdir
:42,42 lclose
:42,42 lcscope
:42,42 ldo
:42,42 lfdo
:42,42 left
:42,42 leftabove
:42,42 legacy
:42,42 let
:42,42 lexpr
:42,42 lfile
:42,42 lfirst
:42,42 lgetbuffer
:42,42 lgetexpr
:42,42 lgetfile
:42,42 lgrep
:42,42 lgrepadd
:42,42 lhelpgrep
:42,42 lhistory
:42,42 ll
:42,42 llast
:42,42 llist
:42,42 lmake
:42,42 lmap
:42,42 lmapclear
:42,42 lnext
:42,42 lnewer
:42,42 lnfile
:42,42 lnoremap
# :42,42 loadkeymap
:42,42 loadview
:42,42 lockmarks
:42,42 lockvar
:42,42 lolder
:42,42 lopen
:42,42 lprevious
:42,42 lpfile
:42,42 lrewind
:42,42 ls
:42,42 ltag
:42,42 lunmap
:42,42 lua
:42,42 luado
:42,42 luafile
:42,42 lvimgrep
:42,42 lvimgrepadd
:42,42 lwindow
:42,42 move
:42,42 mark
:42,42 make
:42,42 map
:42,42 mapclear
:42,42 marks
:42,42 match
:42,42 menu
:42,42 menutranslate
:42,42 messages
:42,42 mkexrc
:42,42 mksession
:42,42 mkspell
:42,42 mkvimrc
:42,42 mkview
:42,42 mode
:42,42 mzscheme
:42,42 mzfile
:42,42 nbclose
:42,42 nbkey
:42,42 nbstart
:42,42 next
:42,42 new
:42,42 nmap
:42,42 nmapclear
:42,42 nmenu
:42,42 nnoremap
:42,42 nnoremenu
:42,42 noautocmd
:42,42 noremap
:42,42 nohlsearch
:42,42 noreabbrev
:42,42 noremenu
:42,42 normal
:42,42 noswapfile
:42,42 number
:42,42 nunmap
:42,42 nunmenu
:42,42 oldfiles
:42,42 open
:42,42 omap
:42,42 omapclear
:42,42 omenu
:42,42 only
:42,42 onoremap
:42,42 onoremenu
:42,42 options
:42,42 ounmap
:42,42 ounmenu
:42,42 ownsyntax
:42,42 packadd
:42,42 packloadall
:42,42 pclose
:42,42 pedit
:42,42 perl
:42,42 print
:42,42 profdel
:42,42 profile
:42,42 promptfind
:42,42 promptrepl
:42,42 perldo
:42,42 pop
:42,42 popup
:42,42 ppop
:42,42 preserve
:42,42 previous
:42,42 psearch
:42,42 ptag
:42,42 ptNext
:42,42 ptfirst
:42,42 ptjump
:42,42 ptlast
:42,42 ptnext
:42,42 ptprevious
:42,42 ptrewind
:42,42 ptselect
:42,42 put
:42,42 pwd
:42,42 py3
:42,42 python3
:42,42 py3do
:42,42 py3file
:42,42 python
:42,42 pydo
:42,42 pyfile
:42,42 pyx
:42,42 pythonx
:42,42 pyxdo
:42,42 pyxfile
:42,42 quit
:42,42 quitall
:42,42 qall
:42,42 read
:42,42 recover
:42,42 redo
:42,42 redir
:42,42 redraw
:42,42 redrawstatus
:42,42 redrawtabline
:42,42 registers
:42,42 resize
:42,42 retab
:42,42 return
:42,42 rewind
:42,42 right
:42,42 rightbelow
:42,42 ruby
:42,42 rubydo
:42,42 rubyfile
:42,42 rundo
:42,42 runtime
:42,42 rviminfo
:42,42 substitute
:42,42 sNext
:42,42 sandbox
:42,42 sargument
:42,42 sall
:42,42 saveas
:42,42 sbuffer
:42,42 sbNext
:42,42 sball
:42,42 sbfirst
:42,42 sblast
:42,42 sbmodified
:42,42 sbnext
:42,42 sbprevious
:42,42 sbrewind
:42,42 scriptnames
:42,42 scriptencoding
:42,42 scriptversion
:42,42 scscope
:42,42 set
:42,42 setfiletype
:42,42 setglobal
:42,42 setlocal
:42,42 sfind
:42,42 sfirst
:42,42 shell
:42,42 simalt
:42,42 sign
:42,42 silent
:42,42 sleep
:42,42 sleep!
:42,42 slast
:42,42 smagic
:42,42 smap
:42,42 smapclear
:42,42 smenu
:42,42 smile
:42,42 snext
:42,42 snomagic
:42,42 snoremap
:42,42 snoremenu
:42,42 sort
:42,42 source
:42,42 spelldump
:42,42 spellgood
:42,42 spellinfo
:42,42 spellrare
:42,42 spellrepall
:42,42 spellundo
:42,42 spellwrong
:42,42 split
:42,42 sprevious
:42,42 srewind
:42,42 stop
:42,42 stag
:42,42 startinsert
:42,42 startgreplace
:42,42 startreplace
:42,42 stopinsert
:42,42 stjump
:42,42 stselect
:42,42 sunhide
:42,42 sunmap
:42,42 sunmenu
:42,42 suspend
:42,42 sview
:42,42 swapname
:42,42 syntax
:42,42 syntime
:42,42 syncbind
:42,42 t
:42,42 tNext
:42,42 tabNext
:42,42 tabclose
:42,42 tabdo
:42,42 tabedit
:42,42 tabfind
:42,42 tabfirst
:42,42 tablast
:42,42 tabmove
:42,42 tabnew
:42,42 tabnext
:42,42 tabonly
:42,42 tabprevious
:42,42 tabrewind
:42,42 tabs
:42,42 tab
:42,42 tag
:42,42 tags
:42,42 tcd
:42,42 tchdir
:42,42 tcl
:42,42 tcldo
:42,42 tclfile
:42,42 tearoff
:42,42 terminal
:42,42 tfirst
:42,42 throw
:42,42 tjump
:42,42 tlast
:42,42 tlmenu
:42,42 tlnoremenu
:42,42 tlunmenu
:42,42 tmapclear
:42,42 tmap
:42,42 tmenu
:42,42 tnext
:42,42 tnoremap
:42,42 topleft
:42,42 tprevious
:42,42 trewind
:42,42 try
:42,42 tselect
:42,42 tunmap
:42,42 tunmenu
:42,42 undo
:42,42 undojoin
:42,42 undolist
:42,42 unabbreviate
:42,42 unabbreviate
:42,42 unhide
:42,42 unlet
:42,42 unlockvar
:42,42 unmap
:42,42 unmenu
:42,42 unsilent
:42,42 update
:42,42 vglobal/.../
:42,42 version
:42,42 verbose
:42,42 vertical
:42,42 vim9cmd
:42,42 vimgrep
:42,42 vimgrepadd
:42,42 visual
:42,42 viusage
:42,42 view
:42,42 vmap
:42,42 vmapclear
:42,42 vmenu
:42,42 vnew
:42,42 vnoremap
:42,42 vnoremenu
:42,42 vsplit
:42,42 vunmap
:42,42 vunmenu
:42,42 windo
:42,42 write
:42,42 wNext
:42,42 wall
:42,42 while
:42,42 winsize
:42,42 wincmd
:42,42 winpos
:42,42 wnext
:42,42 wprevious
:42,42 wq
:42,42 wqall
:42,42 wundo
:42,42 wviminfo
:42,42 xit
:42,42 xall
:42,42 xmapclear
:42,42 xmap
:42,42 xmenu
:42,42 xrestore
:42,42 xnoremap
:42,42 xnoremenu
:42,42 xunmap
:42,42 xunmenu
:42,42 yank
:42,42 z


# range

:1match
:2match
:3match

:42,42append
    text
.
:42,42abbreviate     # no range					
:42,42abclear        # no range
:42,42aboveleft      # no range
:42,42all
:42,42amenu
:42,42anoremenu
:42,42args           # no range
:42,42argadd
:42,42argdedupe      # no range
:42,42argdelete
:42,42argedit
:42,42argdo
:42,42argglobal      # no range
:42,42arglocal       # no range
:42,42argument
:42,42ascii          # no range
:42,42autocmd        # no range
:42,42augroup Foo    # no range
:42,42augroup END    # no range
:42,42aunmenu        # no range
:42,42buffer
:42,42bNext
:42,42ball
:42,42badd           # no range
:42,42balt           # no range
:42,42bdelete
:42,42behave mswin   # no range
:42,42behave xterm   # no range
:42,42belowright     # no range
:42,42bfirst         # no range
:42,42blast
:42,42bmodified
:42,42bnext
:42,42botright       # no range
:42,42bprevious
:42,42brewind
:42,42break          # no range
:42,42breakadd       # no range
:42,42breakdel       # no range
:42,42breaklist      # no range
:42,42browse         # no range
:42,42bufdo
:42,42buffers        # no range
:42,42bunload
:42,42bwipeout
:42,42change
    text
.
:42,42cNext
:42,42cNfile
:42,42cabbrev        # no range
:42,42cabclear       # no range
:42,42cabove
:42,42caddbuffer
:42,42caddexpr       # no range
:42,42caddfile       # no range
:42,42cafter
:42,42call
:42,42catch          # no range
:42,42cbefore
:42,42cbelow
:42,42cbottom        # no range
:42,42cbuffer
:42,42cc
:42,42cclose         # no range
:42,42cd             # no range
:42,42cdo
:42,42cfdo
:42,42center
:42,42cexpr          # no range
:42,42cfile          # no range
:42,42cfirst
:42,42cgetbuffer
:42,42cgetexpr       # no range
:42,42cgetfile       # no range
:42,42changes        # no range
:42,42chdir          # no range
:42,42checkpath      # no range
:42,42checktime
:42,42chistory
:42,42clast
:42,42clearjumps     # no range
:42,42clist          # no range
:42,42close
:42,42cmap           # no range
:42,42cmapclear      # no range
:42,42cmenu
:42,42cnext
:42,42cnewer
:42,42cnfile
:42,42cnoremap       # no range
:42,42cnoreabbrev    # no range
:42,42cnoremenu
:42,42copy
:42,42colder
:42,42colorscheme    # no range
:42,42command        # no range
:42,42comclear       # no range
:42,42compiler       # no range
:42,42continue       # no range
:42,42confirm        # no range
:42,42const          # no range
:42,42copen
:42,42cprevious
:42,42cpfile
:42,42cquit
:42,42crewind
:42,42cscope         # no range
:42,42cstag          # no range
:42,42cunmap         # no range
:42,42cunabbrev      # no range
:42,42cunmenu        # no range
:42,42cwindow
:42,42delete
:42,42debug          # no range
:42,42debuggreedy
:42,42def            # no range
:42,42defcompile     # no range
:42,42defer          # no range
:42,42delcommand     # no range
:42,42delfunction    # no range
:42,42delmarks       # no range
:42,42diffupdate     # no range
:42,42diffget
:42,42diffoff        # no range
:42,42diffpatch      # no range
:42,42diffput
:42,42diffsplit      # no range
:42,42diffthis       # no range
:42,42digraphs       # no range
:42,42display        # no range
:42,42disassemble    # no range
:42,42djump
:42,42dl
:42,42dlist
:42,42doautocmd      # no range
:42,42doautoall      # no range
:42,42dp
:42,42drop           # no range
:42,42dsearch
:42,42dsplit
:42,42edit           # no range
:42,42earlier        # no range
:42,42echo           # no range
:42,42echoconsole    # no range
:42,42echoerr        # no range
:42,42echohl         # no range
:42,42echomsg        # no range
:42,42echon          # no range
:42,42echowindow     # no range
:42,42else           # no range
:42,42elseif         # no range
:42,42emenu
:42,42enddef         # no range
:42,42endif          # no range
:42,42endfor         # no range
:42,42endfunction    # no range
:42,42endtry         # no range
:42,42endwhile       # no range
:42,42enew           # no range
:42,42eval           # no range
:42,42ex             # no range
:42,42execute        # no range
:42,42exit
:42,42exusage        # no range
:42,42file
:42,42files          # no range
:42,42filetype       # no range
:42,42filter         # no range
:42,42find
:42,42final          # no range
:42,42finally        # no range
:42,42finish         # no range
:42,42first          # no range
:42,42fixdel         # no range
:42,42fold
:42,42foldclose
:42,42folddoopen
:42,42folddoclosed
:42,42foldopen
:42,42for            # no range
:42,42function       # no range
:42,42global/.../
:42,42goto
:42,42grep
:42,42grepadd
:42,42gui            # no range
:42,42gvim           # no range
:42,42hardcopy
:42,42help           # no range
:42,42helpclose      # no range
:42,42helpfind       # no range
:42,42helpgrep       # no range
:42,42helptags       # no range
:42,42highlight      # no range
:42,42hide
:42,42history        # no range
:42,42horizontal     # no range
:42,42insert
    text
.
:42,42iabbrev        # no range
:42,42iabclear       # no range
:42,42if             # no range
:42,42ijump
:42,42ilist
:42,42imap           # no range
:42,42imapclear      # no range
:42,42imenu
:42,42import         # no range
:42,42inoremap       # no range
:42,42inoreabbrev    # no range
:42,42inoremenu
:42,42intro          # no range
:42,42isearch
:42,42isplit
:42,42iunmap         # no range
:42,42iunabbrev      # no range
:42,42iunmenu        # no range
:42,42join
:42,42jumps          # no range
:42,42k
:42,42keepalt        # no range
:42,42keepmarks      # no range
:42,42keepjumps      # no range
:42,42keeppatterns   # no range
:42,42lNext
:42,42lNfile
:42,42list
:42,42labove
:42,42laddexpr       # no range
:42,42laddbuffer
:42,42laddfile       # no range
:42,42lafter
:42,42last           # no range
:42,42language       # no range
:42,42later          # no range
:42,42lbefore
:42,42lbelow
:42,42lbottom        # no range
:42,42lbuffer
:42,42lcd            # no range
:42,42lchdir         # no range
:42,42lclose
:42,42lcscope        # no range
:42,42ldo
:42,42lfdo
:42,42left
:42,42leftabove      # no range
:42,42legacy         # no range
:42,42let            # no range
:42,42lexpr          # no range
:42,42lfile          # no range
:42,42lfirst
:42,42lgetbuffer
:42,42lgetexpr       # no range
:42,42lgetfile       # no range
:42,42lgrep
:42,42lgrepadd
:42,42lhelpgrep      # no range
:42,42lhistory
:42,42ll
:42,42llast
:42,42llist          # no range
:42,42lmake          # norange
:42,42lmap           # no range
:42,42lmapclear      # no range
:42,42lnext
:42,42lnewer
:42,42lnfile
:42,42lnoremap       # no range
# :42,42loadkeymap   # no range
:42,42loadview       # no range
:42,42lockmarks      # no range
:42,42lockvar        # no range
:42,42lolder
:42,42lopen
:42,42lprevious
:42,42lpfile
:42,42lrewind
:42,42ls             # no range
:42,42ltag           # no range
:42,42lunmap         # no range
:42,42lua
:42,42luado
:42,42luafile
:42,42lvimgrep
:42,42lvimgrepadd
:42,42lwindow
:42,42move
:42,42mark
:42,42make           # norange
:42,42map            # no range
:42,42mapclear       # no range
:42,42marks          # no range
:42,42match
:42,42menu
:42,42menutranslate  # no range
:42,42messages
:42,42mkexrc         # no range
:42,42mksession      # no range
:42,42mkspell        # no range
:42,42mkvimrc        # no range
:42,42mkview         # no range
:42,42mode           # no range
:42,42mzscheme
:42,42mzfile
:42,42nbclose        # no range
:42,42nbkey          # no range
:42,42nbstart        # no range
:42,42next
:42,42new
:42,42nmap           # no range
:42,42nmapclear      # no range
:42,42nmenu
:42,42nnoremap       # no range
:42,42nnoremenu
:42,42noautocmd      # no range
:42,42noremap        # no range
:42,42nohlsearch     # no range
:42,42noreabbrev     # no range
:42,42noremenu
:42,42normal
:42,42noswapfile     # no range
:42,42number
:42,42nunmap         # no range
:42,42nunmenu        # no range
:42,42oldfiles       # no range
:42,42open
:42,42omap           # no range
:42,42omapclear      # no range
:42,42omenu
:42,42only
:42,42onoremap       # no range
:42,42onoremenu
:42,42options        # no range
:42,42ounmap         # no range
:42,42ounmenu        # no range
:42,42ownsyntax      # no range
:42,42packadd        # no range
:42,42packloadall    # no range
:42,42pclose         # no range
:42,42pedit          # no range
:42,42perl
:42,42print
:42,42profdel        # no range
:42,42profile        # no range
:42,42promptfind     # no range
:42,42promptrepl     # no range
:42,42perldo
:42,42pop
:42,42popup          # no range
:42,42ppop
:42,42preserve       # no range
:42,42previous
:42,42psearch
:42,42ptag
:42,42ptNext
:42,42ptfirst
:42,42ptjump         # no range
:42,42ptlast         # no range
:42,42ptnext
:42,42ptprevious
:42,42ptrewind
:42,42ptselect       # no range
:42,42put
:42,42pwd            # no range
:42,42py3
:42,42python3
:42,42py3do
:42,42py3file
:42,42python
:42,42pydo
:42,42pyfile
:42,42pyx
:42,42pythonx
:42,42pyxdo
:42,42pyxfile
:42,42quit
:42,42quitall        # no range
:42,42qall           # no range
:42,42read
:42,42recover        # no range
:42,42redo           # no range
:42,42redir          # no range
:42,42redraw         # no range
:42,42redrawstatus   # no range
:42,42redrawtabline  # no range
:42,42registers      # no range
:42,42resize
:42,42retab
:42,42return         # no range
:42,42rewind         # no range
:42,42right
:42,42rightbelow     # no range
:42,42ruby
:42,42rubydo
:42,42rubyfile
:42,42rundo          # no range
:42,42runtime        # no range
:42,42rviminfo       # no range
:42,42substitute
:42,42sNext
:42,42sandbox        # no range
:42,42sargument
:42,42sall
:42,42saveas         # no range
:42,42sbuffer
:42,42sbNext
:42,42sball
:42,42sbfirst        # no range
:42,42sblast         # no range
:42,42sbmodified
:42,42sbnext
:42,42sbprevious
:42,42sbrewind       # no range
:42,42scriptnames
:42,42scriptencoding # no range
:42,42scriptversion  # no range
:42,42scscope        # no range
:42,42set            # no range
:42,42setfiletype    # no range
:42,42setglobal      # no range
:42,42setlocal       # no range
:42,42sfind
:42,42sfirst         # no range
:42,42shell          # no range
:42,42simalt         # no range
:42,42sign
:42,42silent         # no range
:42,42sleep
:42,42sleep!
:42,42slast          # no range
:42,42smagic
:42,42smap           # no range
:42,42smapclear      # no range
:42,42smenu
:42,42smile          # no range
:42,42snext
:42,42snomagic
:42,42snoremap       # no range
:42,42snoremenu
:42,42sort
:42,42source
:42,42spelldump      # no range
:42,42spellgood
:42,42spellinfo      # no range
:42,42spellrare
:42,42spellrepall    # no range
:42,42spellundo
:42,42spellwrong
:42,42split
:42,42sprevious
:42,42srewind        # no range
:42,42stop           # no range
:42,42stag
:42,42startinsert    # no range
:42,42startgreplace  # no range
:42,42startreplace   # no range
:42,42stopinsert     # no range
:42,42stjump         # no range
:42,42stselect       # no range
:42,42sunhide
:42,42sunmap         # no range
:42,42sunmenu        # no range
:42,42suspend        # no range
:42,42sview
:42,42swapname       # no range
:42,42syntax         # no range
:42,42syntime        # no range
:42,42syncbind       # no range
:42,42t
:42,42tNext
:42,42tabNext
:42,42tabclose
:42,42tabdo
:42,42tabedit
:42,42tabfind
:42,42tabfirst       # no range
:42,42tablast        # no range
:42,42tabmove
:42,42tabnew
:42,42tabnext
:42,42tabonly
:42,42tabprevious
:42,42tabrewind      # no range
:42,42tabs           # no range
:42,42tab            # no range
:42,42tag
:42,42tags           # no range
:42,42tcd            # no range
:42,42tchdir         # no range
:42,42tcl
:42,42tcldo
:42,42tclfile
:42,42tearoff
:42,42terminal
:42,42tfirst
:42,42throw          # no range
:42,42tjump          # no range
:42,42tlast          # no range
:42,42tlmenu
:42,42tlnoremenu
:42,42tlunmenu       # no range
:42,42tmapclear      # no range
:42,42tmap           # no range
:42,42tmenu
:42,42tnext
:42,42tnoremap       # no range
:42,42topleft        # no range
:42,42tprevious
:42,42trewind
:42,42try            # no range
:42,42tselect        # no range
:42,42tunmap         # no range
:42,42tunmenu        # no range
:42,42undo
:42,42undojoin       # no range
:42,42undolist       # no range
:42,42unabbreviate   # no range
:42,42unabbreviate   # no range
:42,42unhide
:42,42unlet          # no range
:42,42unlockvar      # no range
:42,42unmap          # no range
:42,42unmenu         # no range
:42,42unsilent       # no range
:42,42update
:42,42vglobal/.../
:42,42version        # no range
:42,42verbose
:42,42vertical       # no range
:42,42vim9cmd        # no range
:42,42vimgrep
:42,42vimgrepadd
:42,42visual         # no range
:42,42viusage        # no range
:42,42view           # no range
:42,42vmap           # no range
:42,42vmapclear      # no range
:42,42vmenu
:42,42vnew
:42,42vnoremap       # no range
:42,42vnoremenu
:42,42vsplit
:42,42vunmap         # no range
:42,42vunmenu        # no range
:42,42windo
:42,42write
:42,42wNext
:42,42wall           # no range
:42,42while          # no range
:42,42winsize        # no range
:42,42wincmd
:42,42winpos
:42,42wnext
:42,42wprevious
:42,42wq
:42,42wqall          # no range
:42,42wundo          # no range
:42,42wviminfo       # no range
:42,42xit
:42,42xall           # no range
:42,42xmapclear      # no range
:42,42xmap           # no range
:42,42xmenu
:42,42xrestore       # no range
:42,42xnoremap       # no range
:42,42xnoremenu
:42,42xunmap         # no range
:42,42xunmenu        # no range
:42,42yank
:42,42z

