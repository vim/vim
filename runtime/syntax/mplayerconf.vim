" Vim syntax file
" Language:	    mplayer(1) configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/mplayerconf/
" Latest Revision:  2004-05-22
" arch-tag:	    c20b9381-5858-4452-b866-54e2e1891229

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Set iskeyword since we need `-' (and potentially others) in keywords.
" For version 5.x: Set it globally
" For version 6.x: Set it locally
if version >= 600
  command -nargs=1 SetIsk setlocal iskeyword=<args>
else
  command -nargs=1 SetIsk set iskeyword=<args>
endif
SetIsk @,48-57,-
delcommand SetIsk

" Todo
syn keyword mplayerconfTodo	contained TODO FIXME XXX NOTE

" Comments
syn region mplayerconfComment   display matchgroup=mplayerconfComment start='#' end='$' contains=mplayerconfTodo

" PreProc
syn keyword mplayerconfPreProc  include

" Booleans
syn keyword mplayerconfBoolean  yes no

" Numbers
syn match   mplayerconfNumber   '\<\d\+\>'

" Options
syn keyword mplayerconfOption	hardframedrop nomouseinput bandwidth dumpstream
syn keyword mplayerconfOption	rtsp-stream-over-tcp tv overlapsub sub-bg-alpha
syn keyword mplayerconfOption	subfont-outline unicode format vo edl cookies
syn keyword mplayerconfOption	fps zrfd af-adv nosound audio-density
syn keyword mplayerconfOption	passlogfile vobsuboutindex
syn keyword mplayerconfOption   autoq autosync benchmark colorkey nocolorkey
syn keyword mplayerconfOption   edlout enqueue fixed-vo framedrop h
syn keyword mplayerconfOption   identify input lircconf list-options loop menu
syn keyword mplayerconfOption   menu-cfg menu-root nojoystick nolirc
syn keyword mplayerconfOption   nortc playlist quiet really-quiet shuffle skin
syn keyword mplayerconfOption   slave softsleep speed sstep use-stdin aid alang
syn keyword mplayerconfOption   audio-demuxer audiofile audiofile-cache
syn keyword mplayerconfOption   cdrom-device cache cdda channels chapter
syn keyword mplayerconfOption   cookies-file demuxer dumpaudio dumpfile
syn keyword mplayerconfOption   dumpvideo dvbin dvd-device dvdangle forceidx
syn keyword mplayerconfOption   frames hr-mp3-seek idx ipv4-only-proxy loadidx
syn keyword mplayerconfOption   mc mf ni nobps noextbased passwd prefer-ipv4
syn keyword mplayerconfOption   prefer-ipv6 rawaudio rawvideo
syn keyword mplayerconfOption   saveidx sb srate ss tskeepbroken tsprog tsprobe
syn keyword mplayerconfOption   user user-agent vid vivo dumpjacosub
syn keyword mplayerconfOption   dumpmicrodvdsub dumpmpsub dumpsami dumpsrtsub
syn keyword mplayerconfOption   dumpsub ffactor flip-hebrew font forcedsubsonly
syn keyword mplayerconfOption   fribidi-charset ifo noautosub osdlevel
syn keyword mplayerconfOption   sid slang spuaa spualign spugauss sub
syn keyword mplayerconfOption   sub-bg-color sub-demuxer sub-fuzziness
syn keyword mplayerconfOption   sub-no-text-pp subalign subcc subcp subdelay
syn keyword mplayerconfOption   subfile subfont-autoscale subfont-blur
syn keyword mplayerconfOption   subfont-encoding subfont-osd-scale
syn keyword mplayerconfOption   subfont-text-scale subfps subpos subwidth
syn keyword mplayerconfOption   utf8 vobsub vobsubid abs ao aofile aop delay
syn keyword mplayerconfOption   mixer nowaveheader aa bpp brightness contrast
syn keyword mplayerconfOption   dfbopts display double dr dxr2 fb fbmode
syn keyword mplayerconfOption   fbmodeconfig forcexv fs fsmode-dontuse fstype
syn keyword mplayerconfOption   geometry guiwid hue jpeg monitor-dotclock
syn keyword mplayerconfOption   monitor-hfreq monitor-vfreq monitoraspect
syn keyword mplayerconfOption   nograbpointer nokeepaspect noxv ontop panscan
syn keyword mplayerconfOption   rootwin saturation screenw stop-xscreensaver vm
syn keyword mplayerconfOption   vsync wid xineramascreen z zrbw zrcrop zrdev
syn keyword mplayerconfOption   zrhelp zrnorm zrquality zrvdec zrxdoff ac af
syn keyword mplayerconfOption   afm aspect flip lavdopts noaspect noslices
syn keyword mplayerconfOption   novideo oldpp pp pphelp ssf stereo sws vc vfm x
syn keyword mplayerconfOption   xvidopts xy y zoom vf vop audio-delay
syn keyword mplayerconfOption   audio-preload endpos ffourcc include info
syn keyword mplayerconfOption   noautoexpand noskip o oac of ofps ovc
syn keyword mplayerconfOption   skiplimit v vobsubout vobsuboutid
syn keyword mplayerconfOption   lameopts lavcopts nuvopts xvidencopts

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_mplayer_syn_inits")
  if version < 508
    let did_mplayer_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink mplayerconfTodo    Todo
  HiLink mplayerconfComment Comment
  HiLink mplayerconfPreProc PreProc
  HiLink mplayerconfBoolean Boolean
  HiLink mplayerconfNumber  Number
  HiLink mplayerconfOption  Keyword

  delcommand HiLink
endif

let b:current_syntax = "mplayerconf"

" vim: set sts=2 sw=2:
