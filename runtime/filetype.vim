" Vim support file to detect file types
"
" Maintainer:		The Vim Project <https://github.com/vim/vim>
" Last Change:		2025 Nov 11
" Former Maintainer:	Bram Moolenaar <Bram@vim.org>

" If the filetype can be detected from extension or file name(the final path component),
" please update `ft_from_name` and `ft_from_ext` in `runtime/autoload/dist/ft.vim`.
" Otherwise add a new autocmd in this file.

" Listen very carefully, I will say this only once
if exists("did_load_filetypes")
  finish
endif
let did_load_filetypes = 1

" Line continuation is used here, remove 'C' from 'cpoptions'
let s:cpo_save = &cpo
set cpo&vim

augroup filetypedetect

" Ignored extensions
if exists("*fnameescape")
au BufNewFile,BufRead ?\+.orig,?\+.bak,?\+.old,?\+.new,?\+.dpkg-dist,?\+.dpkg-old,?\+.dpkg-new,?\+.dpkg-bak,?\+.rpmsave,?\+.rpmnew,?\+.pacsave,?\+.pacnew
	\ exe "doau filetypedetect BufRead " . fnameescape(expand("<afile>:r"))
au BufNewFile,BufRead *~
	\ let s:name = expand("<afile>") |
	\ let s:short = substitute(s:name, '\~\+$', '', '') |
	\ if s:name != s:short && s:short != "" |
	\   exe "doau filetypedetect BufRead " . fnameescape(s:short) |
	\ endif |
	\ unlet! s:name s:short
au BufNewFile,BufRead ?\+.in
	\ if expand("<afile>:t") != "configure.in" |
	\   exe "doau filetypedetect BufRead " . fnameescape(expand("<afile>:r")) |
	\ endif
elseif &verbose > 0
  echomsg "Warning: some filetypes will not be recognized because this version of Vim does not have fnameescape()"
endif

" Pattern used to match file names which should not be inspected.
" Currently finds compressed files.
if !exists("g:ft_ignore_pat")
  let g:ft_ignore_pat = '\.\(Z\|gz\|bz2\|zip\|tgz\)$'
endif

" Function used for patterns that end in a star: don't set the filetype if the
" file name matches ft_ignore_pat.
" When using this, the entry should probably be further down below with the
" other StarSetf() calls.
func s:StarSetf(ft)
  if expand("<amatch>") !~ g:ft_ignore_pat
    exe 'setf ' . a:ft
  endif
endfunc

" Vim help file, set ft explicitly, because 'modeline' might be off
au BufNewFile,BufRead */doc/*.txt
	\  if getline('$') =~ '\%(^\|\s\)vim:\%(.*\%(:\|\s\)\)\?\%(ft\|filetype\)=help\%(:\|\s\|$\)'
	\|   setf help
	\| endif

" Detect by name
au BufNewFile,BufRead *				call dist#ft#DetectFromName()

" Abaqus or Trasys
au BufNewFile,BufRead *.inp			call dist#ft#Check_inp()

" A2ps printing utility
au BufNewFile,BufRead */etc/a2ps.cfg,*/etc/a2ps/*.cfg,a2psrc,.a2psrc setf a2ps

" Ada (83, 9X, 95)
if has("vms")
  au BufNewFile,BufRead *.gpr,*.ada_m,*.adc	setf ada
else
  au BufNewFile,BufRead *.gpr			setf ada
endif

" Apache config file
au BufNewFile,BufRead .htaccess,*/etc/httpd/*.conf		setf apache
au BufNewFile,BufRead */etc/apache2/sites-*/*.com		setf apache

" ALSA configuration
au BufNewFile,BufRead .asoundrc,*/usr/share/alsa/alsa.conf,*/etc/asound.conf setf alsaconf

" APT config file
au BufNewFile,BufRead */.aptitude/config       setf aptconf
" more generic pattern far down

" Arch Inventory file
au BufNewFile,BufRead .arch-inventory,=tagging-method	setf arch

" Active Server Pages (with Visual Basic Script)
au BufNewFile,BufRead *.asa
	\ if exists("g:filetype_asa") |
	\   exe "setf " . g:filetype_asa |
	\ else |
	\   setf aspvbs |
	\ endif

" Active Server Pages (with Perl or Visual Basic Script)
au BufNewFile,BufRead *.asp
	\ if exists("g:filetype_asp") |
	\   exe "setf " . g:filetype_asp |
	\ elseif getline(1) . getline(2) . getline(3) =~? "perlscript" |
	\   setf aspperl |
	\ else |
	\   setf aspvbs |
	\ endif

" Grub (must be before pattern *.lst)
au BufNewFile,BufRead */boot/grub/menu.lst,*/boot/grub/grub.conf,*/etc/grub.conf setf grub

" Maxima, see:
" https://maxima.sourceforge.io/docs/manual/maxima_71.html#file_005ftype_005fmaxima
" Must be before the pattern *.mac.
" *.dem omitted - also used by gnuplot demos
" *.mc omitted - used by dist#ft#McSetf()
au BufNewFile,BufRead *.demo,*.dm{1,2,3,t},*.wxm,maxima-init.mac setf maxima

" Assembly (all kinds)
" *.lst is not pure assembly, it has two extra columns (address, byte codes)
" *.[sS], *.[aA] usually Assembly - GNU
au BufNewFile,BufRead *.asm,*.[sS],*.[aA],*.mac,*.lst	call dist#ft#FTasm()

" BASIC or Visual Basic
au BufNewFile,BufRead *.bas			call dist#ft#FTbas()
au BufNewFile,BufRead *.bi,*.bm			call dist#ft#FTbas()

" Batch file for MSDOS. See dist#ft#FTsys for *.sys
" *.cmd is close to a Batch file, but on OS/2 Rexx files and TI linker command files also use *.cmd.
" lnk: `/* comment */`, `// comment`, and `--linker-option=value`
" rexx: `/* comment */`, `-- comment`
au BufNewFile,BufRead *.cmd
	\  if join(getline(1, 20), "\n") =~ 'MEMORY\|SECTIONS\|\%(^\|\n\)--\S\|\%(^\|\n\)//'
	\|   setf lnk
	\| elseif getline(1) =~ '^/\*'
	\|   setf rexx
	\| else
	\|   setf dosbatch
	\| endif
" ABB RAPID or Batch file for MSDOS.
au BufNewFile,BufRead *.sys			call dist#ft#FTsys()
if has("fname_case")
  au BufNewFile,BufRead *.Sys,*.SYS		call dist#ft#FTsys()
endif
au BufNewFile,BufRead *.sysx			setf rapid
if has("fname_case")
  au BufNewFile,BufRead *.sysX,*.Sysx,*.SysX,*.SYSX,*.SYSx	setf rapid
endif

" Batch file for 4DOS
au BufNewFile,BufRead *.btm			call dist#ft#FTbtm()

" BIND configuration
" sudoedit uses namedXXXX.conf
au BufNewFile,BufRead named*.conf,rndc*.conf,rndc*.key	setf named

" BIND zone
au BufNewFile,BufRead *.db			call dist#ft#BindzoneCheck('')

" Blade
au BufNewFile,BufRead *.blade.php		setf blade

" Bitbake
au BufNewFile,BufRead *.bb,*.bbappend,*.bbclass,*/build/conf/*.conf,*/meta{-*,}/conf/*.conf,*/project-spec/configs/*.conf	setf bitbake

" Blkid cache file
au BufNewFile,BufRead */etc/blkid.tab,*/etc/blkid.tab.old   setf xml

" Bazel (https://bazel.build) and Buck2 (https://buck2.build/)
au BufNewFile,BufRead *.bzl,*.bxl,*.bazel,WORKSPACE,WORKSPACE.bzlmod	setf bzl
if has("fname_case")
  " There is another check for BUILD and BUCK further below.
  au BufNewFile,BufRead *.BUILD,BUILD,BUCK		setf bzl
endif

" Bundle config
au BufNewFile,BufRead */.bundle/config			setf yaml

" C or lpc
au BufNewFile,BufRead *.c			call dist#ft#FTlpc()
au BufNewFile,BufRead *.lpc,*.ulpc		setf lpc

" m17n database files. */m17n/* matches installed files, */.m17n.d/* matches
" per-user config files, */m17n-db/* matches the git repo. (must be before
" *.cs)
au BufNewFile,BufRead */{m17n,.m17n.d,m17n-db}/*.{ali,cs,dir,flt,fst,lnm,mic,mim,tbl} setf m17ndb

" Cdrdao TOC or LaTeX \tableofcontents files
au BufNewFile,BufRead *.toc
	\ if getline(1) =~# '\\contentsline' |setf tex|else|setf cdrtoc|endif

" Cdrdao config
au BufNewFile,BufRead */etc/cdrdao.conf,*/etc/defaults/cdrdao,*/etc/default/cdrdao,.cdrdao	setf cdrdaoconf

" Containers config files
au BufNewFile,BufRead */containers/containers.conf{,.d/*.conf}		setf toml
au BufNewFile,BufRead */containers/containers.conf.modules/*.conf	setf toml
au BufNewFile,BufRead */containers/registries.conf{,.d/*.conf}		setf toml
au BufNewFile,BufRead */containers/storage.conf				setf toml

" Cynlib
" .cc and .cpp files can be C++ or Cynlib.
au BufNewFile,BufRead *.cc
	\ if exists("cynlib_syntax_for_cc")|setf cynlib|else|setf cpp|endif
au BufNewFile,BufRead *.cpp
	\ if exists("cynlib_syntax_for_cpp")|setf cynlib|else|setf cpp|endif

" C++
if has("fname_case")
	au BufNewFile,BufRead *.C,*.H if !&fileignorecase | setf cpp | endif
endif

" .h files can be C, C++, Ch, Objective-C, or Objective-C++.
" Set g_filetype_h to set a different filetype
au BufNewFile,BufRead *.h			call dist#ft#FTheader()

" Changelog
au BufNewFile,BufRead changelog.Debian,changelog.dch,NEWS.Debian,NEWS.dch,*/debian/changelog
					\	setf debchangelog

au BufNewFile,BufRead [cC]hange[lL]og
	\  if getline(1) =~ '; urgency='
	\|   setf debchangelog
	\| else
	\|   setf changelog
	\| endif

au BufNewFile,BufRead NEWS
	\  if getline(1) =~ '; urgency='
	\|   setf debchangelog
	\| endif

" CHILL
au BufNewFile,BufRead *..ch			setf chill

" Changes for WEB and CWEB or CHILL
au BufNewFile,BufRead *.ch			call dist#ft#FTchange()

" Clang-format
au BufNewFile,BufRead .clang-format		setf yaml

" Clang-tidy
au BufNewFile,BufRead .clang-tidy		setf yaml

" Matplotlib
au BufNewFile,BufRead *.mplstyle,matplotlibrc	setf yaml

" Clever or dtd
au BufNewFile,BufRead *.ent			call dist#ft#FTent()

" Clipper, FoxPro, ABB RAPID or eviews
au BufNewFile,BufRead *.prg			call dist#ft#FTprg()
if has("fname_case")
  au BufNewFile,BufRead *.Prg,*.PRG			call dist#ft#FTprg()
endif

" Cmake
au BufNewFile,BufRead CMakeLists.txt,*.cmake,*.cmake.in		setf cmake

" Cmusrc
au BufNewFile,BufRead */.cmus/{autosave,rc,command-history,*.theme} setf cmusrc
au BufNewFile,BufRead */cmus/{rc,*.theme}			setf cmusrc

" Cobol
"   cobol or zope form controller python script? (heuristic)
au BufNewFile,BufRead *.cpy
	\ if getline(1) =~ '^##' |
	\   setf python |
	\ else |
	\   setf cobol |
	\ endif

" Dockerfile; Podman uses the same syntax with name Containerfile
" Also see Dockerfile.* below.
au BufNewFile,BufRead Containerfile,Dockerfile,dockerfile,*.[dD]ockerfile	setf dockerfile

" Enlightenment configuration files
au BufNewFile,BufRead *enlightenment/*.cfg	setf c

" Eterm
au BufNewFile,BufRead *Eterm/*.cfg		setf eterm

" Elixir or Euphoria
au BufNewFile,BufRead *.ex call dist#ft#ExCheck()

" Elixir
au BufNewFile,BufRead mix.lock,*.exs setf elixir
au BufNewFile,BufRead *.eex,*.leex setf eelixir

" Euphoria 3 or 4
au BufNewFile,BufRead *.eu,*.ew,*.exu,*.exw  call dist#ft#EuphoriaCheck()
if has("fname_case")
   au BufNewFile,BufRead *.EU,*.EW,*.EX,*.EXU,*.EXW  call dist#ft#EuphoriaCheck()
endif

" Execline (s6) scripts
au BufNewFile,BufRead *s6*/\(up\|down\|run\|finish\)    setf execline

" Faust
au BufNewFile,BufRead *.dsp				call dist#ft#FTdsp()

" Modula-3 configuration language (must be before *.cfg and *makefile)
au BufNewFile,BufRead *.quake,cm3.cfg		setf m3quake
au BufNewFile,BufRead m3makefile,m3overrides	setf m3build

" Many tools written in Python use dosini as their config
" like setuptools, pudb, coverage, pypi, gitlint, oelint-adv, pylint, bpython, mypy
" (must be before *.cfg)
au BufNewFile,BufRead {.,}pylintrc,*/bpython/config,*/mypy/config			setf dosini

" Many tools written in Python use toml as their config, like black
au BufNewFile,BufRead black
	\  if getline(1) =~ 'tool.back'
	\|   setf toml
	\| endif

" LXQt's programs use dosini as their config
au BufNewFile,BufRead */{lxqt,screengrab}/*.conf	setf dosini

" Quake
au BufNewFile,BufRead *baseq[2-3]/*.cfg,*id1/*.cfg	setf quake
au BufNewFile,BufRead *quake[1-3]/*.cfg			setf quake

" LaTeX packages use LaTeX as their configuration, such as:
" ~/.texlive/texmf-config/tex/latex/hyperref/hyperref.cfg
" ~/.texlive/texmf-config/tex/latex/docstrip/docstrip.cfg
au BufNewFile,BufRead */tex/latex/**.cfg		setf tex

" Configure files
au BufNewFile,BufRead *.cfg			call dist#ft#FTcfg()
if has("fname_case")
  au BufNewFile,BufRead *.Cfg,*.CFG			call dist#ft#FTcfg()
endif

" Debian autopkgtest
au BufNewFile,BufRead */debian/tests/control	setf autopkgtest

" Debian Control
au BufNewFile,BufRead */{debian,DEBIAN}/control		setf debcontrol
au BufNewFile,BufRead control
	\  if getline(1) =~ '^Source:\|^Package:'
	\|   setf debcontrol
	\| elseif getline(1) =~ '^Tests:\|^Test-Command:'
	\|   setf autopkgtest
	\| endif

" Debian Copyright
au BufNewFile,BufRead */debian/copyright	setf debcopyright
au BufNewFile,BufRead copyright
	\  if getline(1) =~ '^Format:'
	\|   setf debcopyright
	\| endif

" Debian Sources.list
au BufNewFile,BufRead */etc/apt/sources.list		setf debsources
au BufNewFile,BufRead */etc/apt/sources.list.d/*.list	setf debsources
au BufNewFile,BufRead */etc/apt/sources.list.d/*.sources	setf deb822sources

" dnsmasq(8) configuration files
au BufNewFile,BufRead */etc/dnsmasq.conf	setf dnsmasq

" the D language or dtrace
au BufNewFile,BufRead */dtrace/*.d		setf dtrace
au BufNewFile,BufRead *.d			call dist#ft#DtraceCheck()

" Dictd config
au BufNewFile,BufRead dictd*.conf		setf dictdconf

" DEP3 formatted patch files
au BufNewFile,BufRead */debian/patches/*	call dist#ft#Dep3patch()

" Diff files
au BufNewFile,BufRead *.patch
	\ if getline(1) =~# '^From [0-9a-f]\{40,\} Mon Sep 17 00:00:00 2001$' |
	\   setf gitsendemail |
	\ else |
	\   setf diff |
	\ endif

" Dircolors
au BufNewFile,BufRead .dir_colors,.dircolors,*/etc/DIR_COLORS	setf dircolors

" Diva (with Skill) or InstallShield
au BufNewFile,BufRead *.rul
	\ if getline(1).getline(2).getline(3).getline(4).getline(5).getline(6) =~? 'InstallShield' |
	\   setf ishd |
	\ else |
	\   setf diva |
	\ endif

" DCL (Digital Command Language - vms) or DNS zone file
au BufNewFile,BufRead *.com			call dist#ft#BindzoneCheck('dcl')

" Dune
au BufNewFile,BufRead jbuild,dune,dune-project,dune-workspace,dune-file setf dune

" Microsoft Module Definition or Modula-2
au BufNewFile,BufRead *.def			call dist#ft#FTdef()

if has("fname_case")
  au BufNewFile,BufRead *.DEF			setf modula2
endif

" dsl: DSSSL or Structurizr
au BufNewFile,BufRead *.dsl
	\ if getline(1) =~ '^\s*<\!' |
	\   setf dsl |
	\ else |
	\   setf structurizr |
	\ endif

" EDIF (*.edf,*.edif,*.edn,*.edo) or edn
au BufNewFile,BufRead *.ed\(f\|if\|o\)		setf edif
au BufNewFile,BufRead *.edn
	\ if getline(1) =~ '^\s*(\s*edif\>' |
	\   setf edif |
	\ else |
	\   setf clojure |
	\ endif

" Eiffel or Specman or Euphoria
au BufNewFile,BufRead *.e,*.E			call dist#ft#FTe()

" Elm Filter Rules file
au BufNewFile,BufRead filter-rules		setf elmfilt

" ESMTP rc file
au BufNewFile,BufRead *esmtprc			setf esmtprc

" Fennel
au BufNewFile,BufRead *.fnl,{,.}fennelrc	setf fennel

" Flatpak config
au BufNewFile,BufRead */flatpak/repo/config	setf dosini

" Fortran
if has("fname_case")
  au BufNewFile,BufRead *.F,*.FOR,*.FPP,*.FTN,*.F77,*.F90,*.F95,*.F03,*.F08	setf fortran
endif
au BufNewFile,BufRead *.for,*.fortran,*.fpp,*.ftn,*.f77,*.f90,*.f95,*.f03,*.f08	setf fortran

" Fortran or Forth
au BufNewFile,BufRead *.f			call dist#ft#FTf()

" F# or Forth
au BufNewFile,BufRead *.fs			call dist#ft#FTfs()

" GDB command files
au BufNewFile,BufRead .gdbinit,gdbinit,.cuda-gdbinit,cuda-gdbinit,.gdbearlyinit,gdbearlyinit,*.gdb		setf gdb

" Gedcom
au BufNewFile,BufRead *.ged,lltxxxxx.txt	setf gedcom

" Git
au BufNewFile,BufRead *.git/config,.gitconfig,*/etc/gitconfig	setf gitconfig
au BufNewFile,BufRead */.config/git/config			setf gitconfig
au BufNewFile,BufRead *.git/config.worktree			setf gitconfig
au BufNewFile,BufRead *.git/worktrees/*/config.worktree		setf gitconfig
au BufNewFile,BufRead .gitmodules,*.git/modules/*/config	setf gitconfig
if exists('$XDG_CONFIG_HOME')
  au BufNewFile,BufRead $XDG_CONFIG_HOME/git/config		setf gitconfig
  au BufNewFile,BufRead $XDG_CONFIG_HOME/git/attributes		setf gitattributes
  au BufNewFile,BufRead $XDG_CONFIG_HOME/git/ignore		setf gitignore
endif
au BufNewFile,BufRead .gitattributes,*.git/info/attributes	setf gitattributes
au BufNewFile,BufRead */.config/git/attributes			setf gitattributes
au BufNewFile,BufRead */etc/gitattributes			setf gitattributes
au BufNewFile,BufRead .gitignore,*.git/info/exclude		setf gitignore
au BufNewFile,BufRead */.config/git/ignore,*.prettierignore	setf gitignore
au BufNewFile,BufRead */.config/fd/ignore,.fdignore,.ignore	setf gitignore
au BufNewFile,BufRead .rgignore,.dockerignore,.containerignore	setf gitignore
au BufNewFile,BufRead .npmignore,.vscodeignore			setf gitignore
au BufNewFile,BufRead git-rebase-todo				setf gitrebase
au BufNewFile,BufRead .gitsendemail.msg.??????			setf gitsendemail
au BufNewFile,BufRead *.git/*
      \ if getline(1) =~# '^\x\{40,\}\>\|^ref: ' |
      \   setf git |
      \ endif

" Gkrellmrc
au BufNewFile,BufRead gkrellmrc,gkrellmrc_?	setf gkrellmrc

" GP scripts (2.0 and onward)
au BufNewFile,BufRead *.gp,.gprc		setf gp

" GPG
au BufNewFile,BufRead */.gnupg/options		setf gpg
au BufNewFile,BufRead */.gnupg/gpg.conf		setf gpg
au BufNewFile,BufRead */usr/*/gnupg/options.skel setf gpg
if !empty($GNUPGHOME)
  au BufNewFile,BufRead $GNUPGHOME/options	setf gpg
  au BufNewFile,BufRead $GNUPGHOME/gpg.conf	setf gpg
endif

" Gitolite
au BufNewFile,BufRead {,.}gitolite.rc,example.gitolite.rc	setf perl

" Gnuplot scripts
au BufNewFile,BufRead *.gpi,*.gnuplot,.gnuplot_history	setf gnuplot

" GNU Radio Companion files
au BufNewFile,BufRead *.grc
	\ if getline(1) =~# '<?xml' |
	\   setf xml |
	\ else |
	\   setf yaml |
	\ endif

" Groovy
au BufNewFile,BufRead *.gradle,*.groovy,Jenkinsfile		setf groovy

" Group file
au BufNewFile,BufRead */etc/group,*/etc/group-,*/etc/group.edit,*/etc/gshadow,*/etc/gshadow-,*/etc/gshadow.edit,*/var/backups/group.bak,*/var/backups/gshadow.bak  setf group

" Hare
au BufNewFile,BufRead README			call dist#ft#FTharedoc()

" Haskell
au BufNewFile,BufRead */{.,}cabal/config	setf cabalconfig
au BufNewFile,BufRead cabal.config		setf cabalconfig
au BufNewFile,BufRead *.persistentmodels	setf haskellpersistent

" Tilde (must be before HTML)
au BufNewFile,BufRead *.t.html			setf tilde

" Translate shell
au BufNewFile,BufRead init.trans,*/etc/translate-shell,.trans	setf clojure

" HTML (.stm for server side, .shtml is server-side or superhtml)
au BufNewFile,BufRead *.html,*.htm,*.shtml,*.stm  call dist#ft#FThtml()
au BufNewFile,BufRead *.cshtml			setf html

" Host config
au BufNewFile,BufRead */etc/host.conf		setf hostconf

" Hosts access
au BufNewFile,BufRead */etc/hosts.allow,*/etc/hosts.deny  setf hostsaccess

" Hy
au BufNewFile,BufRead *.hy,.hy-history		setf hy

" Hyprland Configuration language
au BufNewFile,BufRead */hypr/*.conf,hypr\(land\|paper\|idle\|lock\).conf setf hyprlang

" i3
au BufNewFile,BufRead */i3/config		setf i3config
au BufNewFile,BufRead */.i3/config		setf i3config

" sway
au BufNewFile,BufRead */sway/config		setf swayconfig
au BufNewFile,BufRead */sway/config.d/*		setf swayconfig
au BufNewFile,BufRead */.sway/config		setf swayconfig

" IDL (Interface Description Language)
au BufNewFile,BufRead *.idl			call dist#ft#FTidl()

" Icewm menu
au BufNewFile,BufRead */.icewm/menu		setf icemenu

" Indent profile (must come before IDL *.pro!)
au BufNewFile,BufRead indent.pro		call dist#ft#ProtoCheck('indent')

" IDL (Interactive Data Language), Prolog, Cproto or zsh module C
au BufNewFile,BufRead *.pro			call dist#ft#ProtoCheck('idlang')

" Initng
au BufNewFile,BufRead */etc/initng/*/*.i,*.ii	setf initng

" Innovation Data Processing
au BufNewFile,BufRead upstream.dat\c,upstream.*.dat\c,*.upstream.dat\c	setf upstreamdat
au BufNewFile,BufRead fdrupstream.log,upstream.log\c,upstream.*.log\c,*.upstream.log\c,UPSTREAM-*.log\c	setf upstreamlog
au BufNewFile,BufRead upstreaminstall.log\c,upstreaminstall.*.log\c,*.upstreaminstall.log\c setf upstreaminstalllog
au BufNewFile,BufRead usserver.log\c,usserver.*.log\c,*.usserver.log\c	setf usserverlog
au BufNewFile,BufRead usw2kagt.log\c,usw2kagt.*.log\c,*.usw2kagt.log\c	setf usw2kagtlog

" Java Properties resource file (note: doesn't catch font.properties.pl)
au BufNewFile,BufRead *.properties,*.properties_??,*.properties_??_??	setf jproperties
" Eclipse preference files use Java Properties syntax
au BufNewFile,BufRead org.eclipse.*.prefs	setf jproperties

" JSONC (JSON with comments)
au BufNewFile,BufRead *.jsonc,.babelrc,.eslintrc,.jsfmtrc,bun.lock	setf jsonc
au BufNewFile,BufRead .jshintrc,.jscsrc,.vsconfig,.hintrc,.swrc,[jt]sconfig*.json	setf jsonc
" Visual Studio Code settings
au BufNewFile,BufRead ~/*/{Code,VSCodium}/User/*.json setf jsonc

" Just
au BufNewFile,BufRead \c{,*.}justfile,\c*.just setf just

" Kitty
au BufNewFile,BufRead kitty.conf,*/kitty/*.conf setf kitty

" Kuka Robot Language
au BufNewFile,BufRead *.src			call dist#ft#FTsrc()
au BufNewFile,BufRead *.dat			call dist#ft#FTdat()
au BufNewFile,BufRead *.sub			setf krl
if has("fname_case")
   au BufNewFile,BufRead *.Src,*.SRC		call dist#ft#FTsrc()
   au BufNewFile,BufRead *.Dat,*.DAT		call dist#ft#FTdat()
   au BufNewFile,BufRead *.Sub,*.SUB		setf krl
endif

" Lalrpop
au BufNewFile,Bufread *.lalrpop			setf lalrpop

" Larch Shared Language
au BufNewFile,BufRead .lsl			call dist#ft#FTlsl()

" Limits
au BufNewFile,BufRead */etc/limits,*/etc/*limits.conf,*/etc/*limits.d/*.conf	setf limits

" LambdaProlog or SML (see dist#ft#FTmod for *.mod)
au BufNewFile,BufRead *.sig			call dist#ft#FTsig()

" Ld loader
au BufNewFile,BufRead *.ld,*/ldscripts/*	setf ld

" Libao
au BufNewFile,BufRead */etc/libao.conf,*/.libao	setf libao

" Libsensors
au BufNewFile,BufRead */etc/sensors.conf,*/etc/sensors3.conf	setf sensors

" LFTP
au BufNewFile,BufRead lftp.conf,.lftprc,*lftp/rc	setf lftp

" Lifelines, LLVM, or Lex for C++
au BufNewFile,BufRead *.ll			call dist#ft#FTll()

" Lisp (*.el = ELisp)
" *.jl was removed, it's also used for Julia, better skip than guess wrong.
if has("fname_case")
  au BufNewFile,BufRead *.lsp,*.lisp,*.asd,*.el,*.L,.emacs,.sawfishrc setf lisp
else
  au BufNewFile,BufRead *.lsp,*.lisp,*.asd,*.el,.emacs,.sawfishrc setf lisp
endif

" *.cl = Common Lisp or OpenCL
au BufNewFile,BufRead *.cl call dist#ft#FTcl()

" LiteStep RC files
au BufNewFile,BufRead */LiteStep/*/*.rc		setf litestep

" Login access
au BufNewFile,BufRead */etc/login.access	setf loginaccess

" Login defs
au BufNewFile,BufRead */etc/login.defs		setf logindefs

" LOTOS or LaTeX \listoftables files
au BufNewFile,BufRead *.lot
	\ if getline(1) =~# '\\contentsline' |setf tex|else|setf lotos|endif

" Lua, Texlua
au BufNewFile,BufRead *.lua,*.tlu,.lua_history	setf lua

" Luarocks
au BufNewFile,BufRead *.rockspec,rock_manifest	setf lua

" Linden Scripting Language (Second Life)
au BufNewFile,BufRead *.lsl			call dist#ft#FTlsl()

" M4
au BufNewFile,BufRead *.m4	call dist#ft#FTm4()

au BufNewFile,BufRead .m4_history		setf m4

" Mail (for Elm, trn, mutt, muttng, rn, slrn, neomutt)
au BufNewFile,BufRead snd.\d\+,.letter,.letter.\d\+,.followup,.article,.article.\d\+,pico.\d\+,mutt{ng,}-*-\w\+,mutt[[:alnum:]_-]\\\{6\},neomutt-*-\w\+,neomutt[[:alnum:]_-]\\\{6\},ae\d\+.txt,/tmp/SLRN[0-9A-Z.]\+,*.eml setf mail

" Mail aliases
au BufNewFile,BufRead */etc/mail/aliases,*/etc/aliases	setf mailaliases

" Makefile
au BufNewFile,BufRead *[mM]akefile,*.mk,*.mak	call dist#ft#FTmake()
au BufNewFile,BufRead Kbuild			setf make

" Man config
au BufNewFile,BufRead */etc/man.conf,man.config	setf manconf

" Map (UMN mapserver config file)
au BufNewFile,BufRead *.map
	\ if getline(1) =~ '^\*\+$' |
	\   setf lnkmap |
	\ else |
	\   setf map |
	\ endif

" Markdown
au BufNewFile,BufRead *.markdown,*.mdown,*.mkd,*.mkdn,*.mdwn,*.md
	\ if exists("g:filetype_md") |
	\   exe "setf " . g:filetype_md |
	\ else |
	\   setf markdown |
	\ endif

" Mathematica, Matlab, Murphi, Objective C or Octave
au BufNewFile,BufRead *.m			call dist#ft#FTm()

" mbsync
au BufNewFile,BufRead *.mbsyncrc,isyncrc	setf mbsync

" Mercurial (hg) commit file
au BufNewFile,BufRead hg-editor-*.txt		setf hgcommit

" Mercurial config (looks like generic config file)
au BufNewFile,BufRead *.hgrc,*hgrc		setf cfg

" MetaPost
au BufNewFile,BufRead *.mpxl,*.mpiv,*.mpvi	let b:mp_metafun = 1 | setf mp

" MMIX or VMS makefile
au BufNewFile,BufRead *.mms			call dist#ft#FTmms()

" ABB Rapid, Modula-2, Modsim III or LambdaProlog
au BufNewFile,BufRead *.mod			call dist#ft#FTmod()
if has("fname_case")
   au BufNewFile,BufRead *.Mod,*.MOD		call dist#ft#FTmod()
endif
au BufNewFile,BufRead *.modx			setf rapid
if has("fname_case")
   au BufNewFile,BufRead *.modX,*.Modx,*.ModX,*.MODX,*.MODx	setf rapid
endif

" Modula-3 (.m3, .i3, .mg, .ig)
au BufNewFile,BufRead *.[mi][3g]		setf modula3

" Modconf
au BufNewFile,BufRead */etc/modules.conf,*/etc/modules,*/etc/conf.modules setf modconf

" Mplayer config
au BufNewFile,BufRead mplayer.conf,*/.mplayer/config	setf mplayerconf

" Mysql
au BufNewFile,BufRead *.mysql,.mysql_history	setf mysql

" Tcl Shell RC file
" Vivado journal file records REPL input in tcl syntax
" Vivado log file records REPL input in tcl syntax and output
au BufNewFile,BufRead tclsh.rc,vivado*.{jou,log}	setf tcl

" M$ Resource files
" /etc/Muttrc.d/file.rc is muttrc
au BufNewFile,BufRead *.rc,*.rch
	\ if expand("<afile>") !~ "/etc/Muttrc.d/" |
	\   setf rc |
	\ endif

" Mojo
" Mojo files use either .mojo or .🔥 as extension
au BufNewFile,BufRead *.mojo,*.🔥		setf mojo

" Mutt setup file (also for Muttng)
au BufNewFile,BufRead Mutt{ng,}rc		setf muttrc

" Neomutt log
au BufNewFile,BufRead *.neomuttdebug*		setf neomuttlog

" Nano
au BufNewFile,BufRead */etc/nanorc,*.nanorc	setf nanorc

" Nastran input/DMAP
"au BufNewFile,BufRead *.dat			setf nastran

" Natural
au BufNewFile,BufRead *.NS[ACGLMNPS]		setf natural

" Neofetch
au BufNewFile,BufRead */neofetch/config.conf	setf sh

" Nginx
au BufNewFile,BufRead *.nginx,nginx*.conf,*nginx.conf,*/nginx/*.conf	setf nginx

" Nroff/Groff/Troff (*.ms and *.t are checked below)
au BufNewFile,BufRead *.me
	\ if expand("<afile>") != "read.me" && expand("<afile>") != "click.me" |
	\   setf nroff |
	\ endif
au BufNewFile,BufRead *.tr,*.nr,*.roff,*.tmac setf nroff
au BufNewFile,BufRead *.groff,*.mom setf groff
au BufNewFile,BufRead *.[0-9],*.[013]p,*.[1-8]x,*.3{am,perl,pm,posix,type},*.n	call dist#ft#FTnroff()

" Nroff or Objective C++
au BufNewFile,BufRead *.mm			call dist#ft#FTmm()

" notmuch
au BufNewFile,BufRead .notmuch-config{,.*}		setf dosini
au BufNewFile,BufRead ~/.config/notmuch/*/config	setf dosini
if exists('$XDG_CONFIG_HOME')
  au BufNewFile,BufRead $XDG_CONFIG_HOME/notmuch/*/config setf dosini
endif

" OCaml
au BufNewFile,BufRead *.ml,*.mli,*.mll,*.mly,.ocamlinit,*.mlt,*.mlp,*.mlip,*.mli.cppo,*.ml.cppo setf ocaml

" Octave
au BufNewFile,BufRead octave.conf,.octaverc,octaverc,*/octave/history	setf octave

" OPAM
au BufNewFile,BufRead opam,*.opam,*.opam.template,opam.locked,*.opam.locked setf opam

" OpenFOAM
au BufNewFile,BufRead fvSchemes,fvSolution,fvConstrains,fvModels,*/constant/g	call dist#ft#FTfoam()

" OPL
au BufNewFile,BufRead *.[Oo][Pp][Ll]			setf opl

" ini style config files, using # comments
au BufNewFile,BufRead */.aws/config,*/.aws/credentials,*/.aws/cli/alias	setf confini
au BufNewFile,BufRead *.nmconnection			setf confini
au BufNewFile,BufRead paru.conf				setf confini
au BufNewFile,BufRead */{,.}gnuradio/*.conf		setf confini
au BufNewFile,BufRead */gnuradio/conf.d/*.conf		setf confini

" Pacman hooks
au BufNewFile,BufRead *.hook
	\ if getline(1) == '[Trigger]' |
	\   setf confini |
	\ endif

" Pacman makepkg
au BufNewFile,BufRead {.,}makepkg.conf			setf sh

" Pacman log
au BufRead pacman.log*					call s:StarSetf('pacmanlog')

" Pam conf
au BufNewFile,BufRead */etc/pam.conf			setf pamconf

" Password file
au BufNewFile,BufRead */etc/passwd,*/etc/passwd-,*/etc/passwd.edit,*/etc/shadow,*/etc/shadow-,*/etc/shadow.edit,*/var/backups/passwd.bak,*/var/backups/shadow.bak setf passwd

" Pascal or Puppet manifest
au BufNewFile,BufRead *.pp				call dist#ft#FTpp()

" Xilinx labtools project file or Lazarus program file
au BufNewFile,BufRead *.lpr
	\ if getline(1) =~# "<?xml" |
	\   setf xml |
	\ else |
	\   setf pascal |
	\ endif

" Perl or Prolog
if has("fname_case")
  au BufNewFile,BufRead *.pl,*.PL			call dist#ft#FTpl()
else
  au BufNewFile,BufRead *.pl				call dist#ft#FTpl()
endif
au BufNewFile,BufRead *.plx,*.al,*.psgi			setf perl

" Perl, XPM or XPM2
au BufNewFile,BufRead *.pm
	\ if getline(1) =~ "XPM2" |
	\   setf xpm2 |
	\ elseif getline(1) =~ "XPM" |
	\   setf xpm |
	\ else |
	\   setf perl |
	\ endif

" Php, php3, php4, etc.
" Also Phtml (was used for PHP 2 in the past).
" Also .ctp for Cake template file.
" Also .phpt for php tests.
" Also .theme for Drupal theme files.
au BufNewFile,BufRead *.php,*.php\d,*.phtml,*.ctp,*.phpt,*.theme	setf php

" Pinfo config
au BufNewFile,BufRead */etc/pinforc,*/.pinforc	setf pinfo

" Pip requirements
au BufNewFile,BufRead *-requirements.txt	setf requirements
au BufNewFile,BufRead requirements-*.txt	setf requirements
au BufNewFile,BufRead constraints.txt		setf requirements
au BufNewFile,BufRead requirements.in		setf requirements
au BufNewFile,BufRead requirements/*.txt	setf requirements
au BufNewFile,BufRead requires/*.txt		setf requirements

" Pkl
au BufNewFile,BufRead *.pkl,*.pcf,pkl-lsp://*	setf pkl

" Povray, Pascal, PHP or assembly
au BufNewFile,BufRead *.inc			call dist#ft#FTinc()

" PowerShell
au BufNewFile,BufRead	*.ps1,*.psd1,*.psm1,*.pssc	setf ps1
au BufNewFile,BufRead	*.ps1xml			setf ps1xml
au BufNewFile,BufRead	*.cdxml,*.psc1			setf xml

" Printcap and Termcap
au BufNewFile,BufRead *printcap
	\ let b:ptcap_type = "print" | setf ptcap
au BufNewFile,BufRead *termcap
	\ let b:ptcap_type = "term" | setf ptcap

" Progress or CWEB
au BufNewFile,BufRead *.w			call dist#ft#FTprogress_cweb()

" Progress or assembly or Swig
au BufNewFile,BufRead *.i			call dist#ft#FTi()

" Progress or Pascal
au BufNewFile,BufRead *.p			call dist#ft#FTprogress_pascal()

" Software Distributor Product Specification File (POSIX 1387.2-1995)
au BufNewFile,BufRead INDEX,INFO
	\ if getline(1) =~ '^\s*\(distribution\|installed_software\|root\|bundle\|product\)\s*$' |
	\   setf psf |
	\ endif

" Protocols
au BufNewFile,BufRead */etc/protocols		setf protocols

" Python, Python Shell Startup and Python Stub Files
" Quixote (Python-based web framework) and IPython
au BufNewFile,BufRead *.py,*.pyw,.pythonstartup,.pythonrc,.python_history,.jline-jython.history	setf python
au BufNewFile,BufRead *.ipy,*.ptl,*.pyi,SConstruct		   setf python

" RCS file
au BufNewFile,BufRead *\,v			setf rcs

" Registry for MS-Windows
au BufNewFile,BufRead *.reg
	\ if getline(1) =~? '^REGEDIT[0-9]*\s*$\|^Windows Registry Editor Version \d*\.\d*\s*$' | setf registry | endif

" Ripgrep rc
au BufNewFile,BufRead {.,}ripgreprc			setf conf

" R Help file
if has("fname_case")
  au BufNewFile,BufRead *.rd,*.Rd		setf rhelp
else
  au BufNewFile,BufRead *.rd			setf rhelp
endif

" R noweb file
if has("fname_case")
  au BufNewFile,BufRead *.Rnw,*.rnw,*.Snw,*.snw		setf rnoweb
else
  au BufNewFile,BufRead *.rnw,*.snw			setf rnoweb
endif

" R Markdown file
if has("fname_case")
  au BufNewFile,BufRead *.Rmd,*.rmd,*.Smd,*.smd		setf rmd
else
  au BufNewFile,BufRead *.rmd,*.smd			setf rmd
endif

" R reStructuredText file
if has("fname_case")
  au BufNewFile,BufRead *.Rrst,*.rrst,*.Srst,*.srst	setf rrst
else
  au BufNewFile,BufRead *.rrst,*.srst			setf rrst
endif

" Rexx, Rebol or R
au BufNewFile,BufRead *.r,*.R				call dist#ft#FTr()

" Remind
au BufNewFile,BufRead .reminders,*.remind,*.rem		setf remind

" Rantfile and Rakefile is like Ruby
au BufNewFile,BufRead [rR]antfile,*.rant,[rR]akefile,*.rake	setf ruby

" Rust
au BufNewFile,BufRead Cargo.lock,*/.cargo/config,*/.cargo/credentials	setf toml

" Sather, TI linear assembly
au BufNewFile,BufRead *.sa			call dist#ft#FTsa()

" SuperCollider
au BufNewFile,BufRead *.sc			call dist#ft#FTsc()

au BufNewFile,BufRead *.quark			setf supercollider

" scdoc
au BufNewFile,BufRead *.scd			call dist#ft#FTscd()


" Sendmail .mc files are actually m4.  Could also be MS Message text file or
" Maxima.
au BufNewFile,BufRead *.mc			call dist#ft#McSetf()

" Services
au BufNewFile,BufRead */etc/services		setf services

" Service Location config
au BufNewFile,BufRead */etc/slp.conf		setf slpconf

" Service Location registration
au BufNewFile,BufRead */etc/slp.reg		setf slpreg

" Service Location SPI
au BufNewFile,BufRead */etc/slp.spi		setf slpspi

" Setserial config
au BufNewFile,BufRead */etc/serial.conf		setf setserial

" SGML
au BufNewFile,BufRead *.sgm,*.sgml
	\ if getline(1).getline(2).getline(3).getline(4).getline(5) =~? 'linuxdoc' |
	\   setf sgmllnx |
	\ elseif getline(1) =~ '<!DOCTYPE.*DocBook' || getline(2) =~ '<!DOCTYPE.*DocBook' |
	\   let b:docbk_type = "sgml" |
	\   let b:docbk_ver = 4 |
	\   setf docbk |
	\ else |
	\   setf sgml |
	\ endif

" SGMLDECL
au BufNewFile,BufRead *.decl,*.dcl,*.dec
	\ if getline(1).getline(2).getline(3) =~? '^<!SGML' |
	\    setf sgmldecl |
	\ endif

" Shell scripts (sh, ksh, bash, bash2, csh); Allow .profile_foo etc.
" Gentoo ebuilds and Arch Linux PKGBUILDs are actually bash scripts.
" NOTE: Patterns ending in a star are further down, these have lower priority.
au BufNewFile,BufRead .bashrc,bashrc,bash.bashrc,.bash[_-]profile,.bash[_-]logout,.bash[_-]aliases,.bash[_-]history,bash-fc[-.],*.ebuild,*.bash,*.eclass,PKGBUILD,*.bats,*.cygport call dist#ft#SetFileTypeSH("bash")
au BufNewFile,BufRead .kshrc,*.ksh call dist#ft#SetFileTypeSH("ksh")
au BufNewFile,BufRead */etc/profile,.profile,*.sh,*.env{rc,} call dist#ft#SetFileTypeSH(getline(1))
" Shell script (Arch Linux) or PHP file (Drupal)
au BufNewFile,BufRead *.install
	\ if getline(1) =~ '<?php' |
	\   setf php |
	\ else |
	\   call dist#ft#SetFileTypeSH("bash") |
	\ endif

" tcsh scripts (patterns ending in a star further below)
au BufNewFile,BufRead .tcshrc,*.tcsh,tcsh.tcshrc,tcsh.login	call dist#ft#SetFileTypeShell("tcsh")

" csh scripts, but might also be tcsh scripts (on some systems csh is tcsh)
" (patterns ending in a start further below)
au BufNewFile,BufRead .login,.cshrc,csh.cshrc,csh.login,csh.logout,*.csh,.alias  call dist#ft#CSH()

" Z-Shell script (patterns ending in a star further below)
au BufNewFile,BufRead .zprofile,*/etc/zprofile,.zfbfmarks  setf zsh
au BufNewFile,BufRead .zshrc,.zshenv,.zlogin,.zlogout,.zcompdump,.zsh_history setf zsh
au BufNewFile,BufRead *.zsh,*.zsh-theme,*.zunit		setf zsh

" Scheme, Supertux configuration, Lips.js history, Guile init file ("racket" patterns are now separate, see above)
au BufNewFile,BufRead *.scm,*.ss,*.sld,*.stsg,*/supertux2/config,.lips_repl_history,.guile	setf scheme

" SiSU
au BufNewFile,BufRead *.sst.meta,*.-sst.meta,*._sst.meta setf sisu

" Smalltalk (and Rexx, TeX, and Visual Basic)
au BufNewFile,BufRead *.cls			call dist#ft#FTcls()

" SMIL or XML
au BufNewFile,BufRead *.smil
	\ if getline(1) =~ '<?\s*xml.*?>' |
	\   setf xml |
	\ else |
	\   setf smil |
	\ endif

" SMIL or SNMP MIB file
au BufNewFile,BufRead *.smi
	\ if getline(1) =~ '\<smil\>' |
	\   setf smil |
	\ else |
	\   setf mib |
	\ endif

" Snakemake
au BufNewFile,BufRead Snakefile,*.smk		setf snakemake

" Snort Configuration
au BufNewFile,BufRead *.hog,snort.conf,vision.conf	setf hog
au BufNewFile,BufRead *.rules			call dist#ft#FTRules()

" *.typ can be either SQL or Typst files
au BufNewFile,BufRead *.typ			call dist#ft#FTtyp()

" SQL
au BufNewFile,BufRead *.sql			call dist#ft#SQL()
au BufNewFile,BufRead .sqlite_history		setf sql

" OpenSSH configuration
au BufNewFile,BufRead ssh_config,*/.ssh/config,*/.ssh/*.conf	setf sshconfig
au BufNewFile,BufRead */etc/ssh/ssh_config.d/*.conf		setf sshconfig

" OpenSSH server configuration
au BufNewFile,BufRead */etc/ssh/sshd_config.d/*.conf	setf sshdconfig

" OpenVPN configuration
au BufNewFile,BufRead */openvpn/*/*.conf	setf openvpn

" Stata
" Also *.class, but not when it's a Java bytecode file
au BufNewFile,BufRead *.class
	\ if getline(1) !~ "^\xca\xfe\xba\xbe" | setf stata | endif

" SPA JSON
au BufNewFile,BufRead */pipewire/*.conf		setf spajson
au BufNewFile,BufRead */wireplumber/*.conf	setf spajson

" Swift
au BufNewFile,BufRead *.swift.gyb		setf swiftgyb

" Swift Intermediate Language or SILE
au BufNewFile,BufRead *.sil			call dist#ft#FTsil()

" Sysctl
au BufNewFile,BufRead */etc/sysctl.conf,*/etc/sysctl.d/*.conf	setf sysctl

" Systemd unit files
au BufNewFile,BufRead */systemd/*.{automount,dnssd,link,mount,netdev,network,nspawn,path,service,slice,socket,swap,target,timer}	setf systemd
" Systemd overrides
au BufNewFile,BufRead */etc/systemd/*.conf.d/*.conf	setf systemd
au BufNewFile,BufRead */etc/systemd/system/*.d/*.conf	setf systemd
au BufNewFile,BufRead */.config/systemd/user/*.d/*.conf	setf systemd
" Systemd temp files
au BufNewFile,BufRead */etc/systemd/system/*.d/.#*	setf systemd
au BufNewFile,BufRead */etc/systemd/system/.#*		setf systemd
au BufNewFile,BufRead */.config/systemd/user/*.d/.#*	setf systemd
au BufNewFile,BufRead */.config/systemd/user/.#*	setf systemd

" Sudoers
au BufNewFile,BufRead */etc/sudoers,sudoers.tmp	setf sudoers

" Tads (or Nroff or Perl test file)
au BufNewFile,BufRead *.t
	\ if !dist#ft#FTnroff() && !dist#ft#FTperl() | setf tads | endif

" Task
au BufNewFile,BufRead {pending,completed,undo}.data  setf taskdata
au BufNewFile,BufRead *.task			setf taskedit

" Tcl (JACL too)
au BufNewFile,BufRead *.tcl,*.tm,*.tk,*.itcl,*.itk,*.jacl,.tclshrc,.wishrc,.tclsh-history	setf tcl

" Tera Term Language or Turtle
au BufNewFile,BufRead *.ttl
	\ if getline(1) =~ '^@\?\(prefix\|base\)' |
	\   setf turtle |
	\ else |
	\   setf teraterm |
	\ endif

" TeX
au BufNewFile,BufRead *.tex			call dist#ft#FTtex()
au BufNewFile,BufRead texdoc.cnf		setf conf

" LaTeX packages will generate some medium LaTeX files during compiling
" They should be ignored by .gitignore https://github.com/github/gitignore/blob/main/TeX.gitignore
" Sometime we need to view its content for debugging
au BufNewFile,BufRead *.{pgf,nlo,nls,thm,eps_tex,pygtex,pygstyle,clo,aux,brf,ind,lof,loe,nav,vrb,ins,tikz,bbx,cbx,beamer}	setf tex

" TF (TinyFugue) mud client or terraform
au BufNewFile,BufRead *.tf			call dist#ft#FTtf()

" tmux configuration
au BufNewFile,BufRead {.,}tmux*.conf		setf tmux

" TOML
au BufNewFile,BufRead *.toml,uv.lock		setf toml

" TypeScript or Qt translation file (which is XML)
au BufNewFile,BufRead *.ts
	\ if getline(1) =~ '<?xml' |
	\   setf xml |
	\ else |
	\   setf typescript |
	\ endif
au BufNewFile,BufRead .ts_node_repl_history	setf typescript

" Udev conf
au BufNewFile,BufRead */etc/udev/udev.conf	setf udevconf

" Udev permissions
au BufNewFile,BufRead */etc/udev/permissions.d/*.permissions setf udevperm
"
" Udev symlinks config
au BufNewFile,BufRead */etc/udev/cdsymlinks.conf	setf sh

" Updatedb
au BufNewFile,BufRead */etc/updatedb.conf	setf updatedb

" Upstart (init(8)) config files
au BufNewFile,BufRead */usr/share/upstart/*.conf	       setf upstart
au BufNewFile,BufRead */usr/share/upstart/*.override	       setf upstart
au BufNewFile,BufRead */etc/init/*.conf,*/etc/init/*.override  setf upstart
au BufNewFile,BufRead */.init/*.conf,*/.init/*.override	       setf upstart
au BufNewFile,BufRead */.config/upstart/*.conf		       setf upstart
au BufNewFile,BufRead */.config/upstart/*.override	       setf upstart

" Verilog HDL, V or Coq
au BufNewFile,BufRead *.v			call dist#ft#FTv()

" Vim script
au BufNewFile,BufRead *.vim,.exrc,_exrc,.netrwhist	setf vim

" Virata Config Script File or Drupal module
au BufNewFile,BufRead *.hw,*.module,*.pkg
	\ if getline(1) =~ '<?php' |
	\   setf php |
	\ else |
	\   setf virata |
	\ endif

" Visual Basic (see also *.bas *.cls)

" Visual Basic or FORM
au BufNewFile,BufRead *.frm			call dist#ft#FTfrm()

" Visual Basic or Vimball Archiver
au BufNewFile,BufRead *.vba			call dist#ft#FTvba()

" Waybar config
au BufNewFile,BufRead */waybar/config		setf jsonc

" CVS commit file
au BufNewFile,BufRead cvs\d\+			setf cvs

" WEB (*.web is also used for Winbatch: Guess, based on expecting "%" comment
" lines in a WEB file).
au BufNewFile,BufRead *.web
	\ if getline(1)[0].getline(2)[0].getline(3)[0].getline(4)[0].getline(5)[0] =~ "%" |
	\   setf web |
	\ else |
	\   setf winbatch |
	\ endif

" Windows Scripting Host and Windows Script Component
au BufNewFile,BufRead *.ws[fc]			setf wsh

" Xdg-user-dirs
au BufNewFile,BufRead user-dirs.dirs,user-dirs.defaults		setf sh

" X Pixmap (dynamically sets colors, this used to trigger on BufEnter to make
" it work better, but that breaks setting 'filetype' manually)
au BufNewFile,BufRead *.xpm
	\ if getline(1) =~ "XPM2" |
	\   setf xpm2 |
	\ else |
	\   setf xpm |
	\ endif
au BufNewFile,BufRead *.xpm2			setf xpm2

" XFree86 config
au BufNewFile,BufRead XF86Config
	\ if getline(1) =~ '\<XConfigurator\>' |
	\   let b:xf86conf_xfree86_version = 3 |
	\ endif |
	\ setf xf86conf
au BufNewFile,BufRead */xorg.conf.d/*.conf
	\ let b:xf86conf_xfree86_version = 4 |
	\ setf xf86conf

" Xorg config
au BufNewFile,BufRead xorg.conf,xorg.conf-4	let b:xf86conf_xfree86_version = 4 | setf xf86conf

" Xinetd conf
au BufNewFile,BufRead */etc/xinetd.conf		setf xinetd

" X resources file
au BufNewFile,BufRead .Xdefaults,.Xpdefaults,.Xresources,xdm-config,*.ad setf xdefaults

" Xmath
au BufNewFile,BufRead *.ms
	\ if !dist#ft#FTnroff() | setf xmath | endif

" XML  specific variants: docbk and xbl
au BufNewFile,BufRead *.xml			call dist#ft#FTxml()

" CSPROJ files are Visual Studio.NET's XML-based C# project config files
au BufNewFile,BufRead *.csproj,*.csproj.user	setf xml

" FSPROJ files are Visual Studio.NET's XML-based F# project config files
au BufNewFile,BufRead *.fsproj,*.fsproj.user	setf xml

" VBPROJ files are Visual Studio.NET's XML-based Visual Basic project config files
au BufNewFile,BufRead *.vbproj,*.vbproj.user	setf xml

" Xdg menus
au BufNewFile,BufRead */etc/xdg/menus/*.menu	setf xml

" X11 xmodmap (also see below)
au BufNewFile,BufRead *Xmodmap			setf xmodmap

" Yacc or racc
au BufNewFile,BufRead *.y			call dist#ft#FTy()

" Yaml
au BufNewFile,BufRead */.kube/config,*/.kube/kuberc	setf yaml

" yum conf (close enough to dosini)
au BufNewFile,BufRead */etc/yum.conf		setf dosini

" Zimbu
" Zope
"   dtml (zope dynamic template markup language), pt (zope page template),
"   cpt (zope form controller page template)
au BufNewFile,BufRead *.dtml,*.pt,*.cpt		call dist#ft#FThtml()
"   zsql (zope sql method)
au BufNewFile,BufRead *.zsql			call dist#ft#SQL()

" Detect by extention
au BufNewFile,BufRead *				call dist#ft#DetectFromExt()
augroup END


" Source the user-specified filetype file, for backwards compatibility with
" Vim 5.x.
if exists("myfiletypefile") && filereadable(expand(myfiletypefile))
  execute "source " . myfiletypefile
endif


" Check for "*" after loading myfiletypefile, so that scripts.vim is only used
" when there are no matching file name extensions.
" Don't do this for compressed files.
augroup filetypedetect
au BufNewFile,BufRead *
	\ if !did_filetype() && expand("<amatch>") !~ g:ft_ignore_pat
	\ | runtime! scripts.vim | endif
au StdinReadPost * if !did_filetype() | runtime! scripts.vim | endif


" Plain text files, needs to be far down to not override others.  This avoids
" the "conf" type being used if there is a line starting with '#'.
" But before patterns matching everything in a directory.
au BufNewFile,BufRead *.text,README,LICENSE,COPYING,AUTHORS	setf text

" What should *.out files be? Text?
" Disabled until it is clear, to what this should be set
"au BufNewFile,BufRead *.out	setf text


" Extra checks for when no filetype has been detected now.  Mostly used for
" patterns that end in "*".  E.g., "zsh*" matches "zsh.vim", but that's a Vim
" script file.
" Most of these should call s:StarSetf() to avoid names ending in .gz and the
" like are used.

" More Apache style config files
au BufNewFile,BufRead */etc/proftpd/*.conf*,*/etc/proftpd/conf.*/*	call s:StarSetf('apachestyle')
au BufNewFile,BufRead proftpd.conf*					call s:StarSetf('apachestyle')

" More Apache config files
au BufNewFile,BufRead access.conf*,apache.conf*,apache2.conf*,httpd.conf*,httpd-*.conf*,srm.conf*,proxy-html.conf*	call s:StarSetf('apache')
au BufNewFile,BufRead */etc/apache2/*.conf*,*/etc/apache2/conf.*/*,*/etc/apache2/mods-*/*,*/etc/apache2/sites-*/*,*/etc/httpd/conf.*/*,*/etc/httpd/mods-*/*,*/etc/httpd/sites-*/*,*/etc/httpd/conf.d/*.conf*		call s:StarSetf('apache')

" APT config file
au BufNewFile,BufRead */etc/apt/apt.conf.d/{[-_[:alnum:]]\+,[-_.[:alnum:]]\+.conf} call s:StarSetf('aptconf')

" Asterisk config file
au BufNewFile,BufRead *asterisk/*.conf*		call s:StarSetf('asterisk')
au BufNewFile,BufRead *asterisk*/*voicemail.conf* call s:StarSetf('asteriskvm')

" Bazaar version control
au BufNewFile,BufRead bzr_log.*			setf bzr

" Bazel and Buck2 build file
if !has("fname_case")
  au BufNewFile,BufRead *.BUILD,BUILD,BUCK	setf bzl
endif

" BIND zone
au BufNewFile,BufRead */named/db.*,*/bind/db.*	call s:StarSetf('bindzone')

au BufNewFile,BufRead cabal.project.*		call s:StarSetf('cabalproject')

" Calendar
au BufNewFile,BufRead */.calendar/*,
	\*/share/calendar/*/calendar.*,*/share/calendar/calendar.*
	\					call s:StarSetf('calendar')

" Changelog
au BufNewFile,BufRead [cC]hange[lL]og*
	\ if getline(1) =~ '; urgency='
	\|  call s:StarSetf('debchangelog')
	\|else
	\|  call s:StarSetf('changelog')
	\|endif

" Crontab
au BufNewFile,BufRead crontab,crontab.*,*/etc/cron.d/*		call s:StarSetf('crontab')

" dnsmasq(8) configuration
au BufNewFile,BufRead */etc/dnsmasq.d/*		call s:StarSetf('dnsmasq')

" Dockerfile
au BufNewFile,BufRead Dockerfile.*,Containerfile.*	call s:StarSetf('dockerfile')

" Dracula
au BufNewFile,BufRead drac.*			call s:StarSetf('dracula')

" Execline (s6) scripts
au BufNewFile,BufRead s6-*			call s:StarSetf('execline')

" Fvwm
au BufNewFile,BufRead */.fvwm/*			call s:StarSetf('fvwm')
au BufNewFile,BufRead *fvwmrc*,*fvwm95*.hook
	\ let b:fvwm_version = 1 | call s:StarSetf('fvwm')
au BufNewFile,BufRead *fvwm2rc*
	\ let b:fvwm_version = 2 | call s:StarSetf('fvwm')

" Gedcom
au BufNewFile,BufRead */tmp/lltmp*		call s:StarSetf('gedcom')

" Git
au BufNewFile,BufRead */.gitconfig.d/*,*/etc/gitconfig.d/*	call s:StarSetf('gitconfig')

" Gitolite
au BufNewFile,BufRead */gitolite-admin/conf/*	call s:StarSetf('gitolite')

" GTK RC
au BufNewFile,BufRead .gtkrc*,gtkrc*		call s:StarSetf('gtkrc')

" Jam
au BufNewFile,BufRead Prl*.*,JAM*.*		call s:StarSetf('jam')

" Jargon
au! BufNewFile,BufRead *jarg*
	\ if getline(1).getline(2).getline(3).getline(4).getline(5) =~? 'THIS IS THE JARGON FILE'
	\|  call s:StarSetf('jargon')
	\|endif

" Java Properties resource file (note: doesn't catch font.properties.pl)
au BufNewFile,BufRead *.properties_??_??_*	call s:StarSetf('jproperties')

" Kconfig
au BufNewFile,BufRead Kconfig.*,Config.in.*	call s:StarSetf('kconfig')

" Lilo: Linux loader
au BufNewFile,BufRead lilo.conf*		call s:StarSetf('lilo')

" Libsensors
au BufNewFile,BufRead */etc/sensors.d/[^.]*	call s:StarSetf('sensors')

" Logcheck
au BufNewFile,BufRead */etc/logcheck/*.d*/*	call s:StarSetf('logcheck')

" Makefile
au BufNewFile,BufRead [mM]akefile*		if expand('<afile>:t') !~ g:ft_ignore_pat | call dist#ft#FTmake() | endif

" Ruby Makefile
au BufNewFile,BufRead [rR]akefile*		call s:StarSetf('ruby')

" Mail (also matches muttrc.vim, so this is below the other checks)
au BufNewFile,BufRead {neo,}mutt[[:alnum:]._-]\\\{6\}	setf mail

au BufNewFile,BufRead reportbug-*		call s:StarSetf('mail')

" Messages (logs mostly)
au BufNewFile,BufRead */log/{auth,cron,daemon,debug,kern,lpr,mail,messages,news/news,syslog,user}{,.log,.err,.info,.warn,.crit,.notice}{,.[0-9]*,-[0-9]*}
      \ 					call s:StarSetf('messages')

" Modconf
au BufNewFile,BufRead */etc/modutils/*
	\ if executable(expand("<afile>")) != 1
	\|  call s:StarSetf('modconf')
	\|endif
au BufNewFile,BufRead */etc/modprobe.*		call s:StarSetf('modconf')

" Mutt setup files (must be before catch *.rc)
au BufNewFile,BufRead */etc/Muttrc.d/*		call s:StarSetf('muttrc')

" Mutt setup file
au BufNewFile,BufRead .mutt{ng,}rc*,*/.mutt{ng,}/mutt{ng,}rc*	call s:StarSetf('muttrc')
au BufNewFile,BufRead mutt{ng,}rc*,Mutt{ng,}rc*		call s:StarSetf('muttrc')

" Neomutt setup file
au BufNewFile,BufRead .neomuttrc*,*/.neomutt/neomuttrc*	call s:StarSetf('neomuttrc')
au BufNewFile,BufRead neomuttrc*,Neomuttrc*		call s:StarSetf('neomuttrc')

" Nginx
au BufNewFile,BufRead */etc/nginx/*,*/usr/local/nginx/conf/*	call s:StarSetf('nginx')

" Nroff macros
au BufNewFile,BufRead tmac.*			call s:StarSetf('nroff')

" OpenBSD hostname.if
au BufNewFile,BufRead */etc/hostname.*		call s:StarSetf('config')

" OpenFOAM
au BufNewFile,BufRead [a-zA-Z0-9]*Dict{,.*},[a-zA-Z]*Properties{,.*},*Transport.*,*/0{,.orig}/*
      \ if expand("<amatch>") !~ g:ft_ignore_pat
      \|  call dist#ft#FTfoam()
      \|endif

" Pam conf
au BufNewFile,BufRead */etc/pam.d/*		call s:StarSetf('pamconf')

" Pandoc
au BufNewFile,BufRead,BufFilePost *.pandoc,*.pdk,*.pd,*.pdc	setf pandoc

" PHP config
au BufNewFile,BufRead php.ini-*,php-fpm.conf*,www.conf*		call s:StarSetf('dosini')

" Printcap and Termcap
au BufNewFile,BufRead *printcap*
	\ if !did_filetype()
	\|  let b:ptcap_type = "print" | call s:StarSetf('ptcap')
	\|endif
au BufNewFile,BufRead *termcap*
	\ if !did_filetype()
	\|  let b:ptcap_type = "term" | call s:StarSetf('ptcap')
	\|endif

" ReDIF
" Only used when the .rdf file was not detected to be XML.
au BufNewFile,BufRead *.rdf			call dist#ft#Redif()

" Remind
au BufNewFile,BufRead .reminders*		call s:StarSetf('remind')

" SGML catalog file
au BufNewFile,BufRead sgml.catalog*		call s:StarSetf('catalog')

" avoid doc files being recognized a shell files
au BufNewFile,BufRead */doc/{,.}bash[_-]completion{,.d,.sh}{,/*} setf text

" Shell scripts ending in a star
au BufNewFile,BufRead .bashrc*,.bash[_-]profile*,.bash[_-]logout*,.bash[_-]aliases*,bash-fc[-.]*,PKGBUILD*,APKBUILD*,*/{,.}bash[_-]completion{,.d,.sh}{,/*} call dist#ft#SetFileTypeSH("bash")
au BufNewFile,BufRead .kshrc* call dist#ft#SetFileTypeSH("ksh")
au BufNewFile,BufRead .profile* call dist#ft#SetFileTypeSH(getline(1))

" Sudoers
au BufNewFile,BufRead */etc/sudoers.d/*		call s:StarSetf('sudoers')

" tcsh scripts ending in a star
au BufNewFile,BufRead .tcshrc*	call dist#ft#SetFileTypeShell("tcsh")

" csh scripts ending in a star
au BufNewFile,BufRead .login*,.cshrc*  call dist#ft#CSH()

" tmux configuration with arbitrary extension
au BufNewFile,BufRead {.,}tmux*.conf*		call s:StarSetf('tmux')

" UCI
" UCI files are normally in /etc/config, but that might be mounted over sshfs or similar, so we match more loosely.
" There was some concern[1] that this pattern would match too much, so now we check the file content as well.
" [1]: https://github.com/vim/vim/pull/14385#discussion_r1558878741
au BufNewFile,BufRead */etc/config/*		if dist#ft#Detect_UCI_statements() | call s:StarSetf('uci') | endif

" VHDL
au BufNewFile,BufRead *.vhdl_[0-9]*		call s:StarSetf('vhdl')

" Vim script
au BufNewFile,BufRead *vimrc*			call s:StarSetf('vim')

" Subversion commit file
au BufNewFile,BufRead svn-commit*.tmp		setf svn

" X resources file
au BufNewFile,BufRead Xresources*,*/app-defaults/*,*/Xresources/* call s:StarSetf('xdefaults')

" XFree86 config
au BufNewFile,BufRead XF86Config-4*
	\ let b:xf86conf_xfree86_version = 4 | call s:StarSetf('xf86conf')
au BufNewFile,BufRead XF86Config*
	\ if getline(1) =~ '\<XConfigurator\>'
	\|  let b:xf86conf_xfree86_version = 3
	\|endif
	\|call s:StarSetf('xf86conf')

" XKB
au BufNewFile,BufRead */{,.}xkb/{compat,geometry,keycodes,symbols,types}/*	call s:StarSetf('xkb')

" X11 xmodmap
au BufNewFile,BufRead *xmodmap*			call s:StarSetf('xmodmap')

" Xinetd conf
au BufNewFile,BufRead */etc/xinetd.d/*		call s:StarSetf('xinetd')

" yum conf (close enough to dosini)
au BufNewFile,BufRead */etc/yum.repos.d/*	call s:StarSetf('dosini')

" Z-Shell script ending in a star
au BufNewFile,BufRead .zsh*,.zlog*,.zcompdump*  call s:StarSetf('zsh')
au BufNewFile,BufRead zsh*,zlog*		call s:StarSetf('zsh')

" Help files match *.txt but should have a last line that is a modeline.
au BufNewFile,BufRead *.txt
	\  if getline('$') !~ 'vim:.*ft=help'
	\|   setf text
	\| endif



" Generic log file
" Disabled cause it is too distracting
" au BufNewFile,BufRead *.log,*_log,*.LOG,*_LOG	setf log

" Use the filetype detect plugins.  They may overrule any of the previously
" detected filetypes.
runtime! ftdetect/*.vim

" NOTE: The above command could have ended the filetypedetect autocmd group
" and started another one. Let's make sure it has ended to get to a consistent
" state.
augroup END

" Generic configuration file. Use FALLBACK, it's just guessing!
au filetypedetect BufNewFile,BufRead,StdinReadPost *
	\ if !did_filetype() && expand("<amatch>") !~ g:ft_ignore_pat
	\    && (expand("<amatch>") =~# '\.conf$'
	\	|| getline(1) =~ '^#' || getline(2) =~ '^#'
	\	|| getline(3) =~ '^#' || getline(4) =~ '^#'
	\	|| getline(5) =~ '^#') |
	\   setf FALLBACK conf |
	\ endif

" If the GUI is already running, may still need to install the Syntax menu.
" Don't do it when the 'M' flag is included in 'guioptions'.
if has("menu") && has("gui_running")
      \ && !exists("did_install_syntax_menu") && &guioptions !~# "M"
  source <sfile>:p:h/menu.vim
endif

" Function called for testing all functions defined here.  These are
" script-local, thus need to be executed here.
" Returns a string with error messages (hopefully empty).
func TestFiletypeFuncs(testlist)
  let output = ''
  for f in a:testlist
    try
      exe f
    catch
      let output = output . "\n" . f . ": " . v:exception
    endtry
  endfor
  return output
endfunc

" Restore 'cpoptions'
let &cpo = s:cpo_save
unlet s:cpo_save

" vim: ts=8
