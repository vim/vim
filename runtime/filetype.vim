" Vim support file to detect file types
"
" Maintainer:		The Vim Project <https://github.com/vim/vim>
" Last Change:		2025 Aug 10
" Former Maintainer:	Bram Moolenaar <Bram@vim.org>

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

" Abaqus or Trasys
au BufNewFile,BufRead *.inp			call dist#ft#Check_inp()

" 8th (Firth-derivative)
au BufNewFile,BufRead *.8th			setf 8th

" A-A-P recipe
au BufNewFile,BufRead *.aap			setf aap

" A2ps printing utility
au BufNewFile,BufRead */etc/a2ps.cfg,*/etc/a2ps/*.cfg,a2psrc,.a2psrc setf a2ps

" ABAB/4
au BufNewFile,BufRead *.abap			setf abap

" ABC music notation
au BufNewFile,BufRead *.abc			setf abc

" ABEL
au BufNewFile,BufRead *.abl			setf abel

" ABNF
au BufNewFile,BufRead *.abnf			setf abnf

" AceDB
au BufNewFile,BufRead *.wrm			setf acedb

" Ada (83, 9X, 95)
au BufNewFile,BufRead *.adb,*.ads,*.ada		setf ada
if has("vms")
  au BufNewFile,BufRead *.gpr,*.ada_m,*.adc	setf ada
else
  au BufNewFile,BufRead *.gpr			setf ada
endif

" AHDL
au BufNewFile,BufRead *.tdf			setf ahdl

" AIDL
au BufNewFile,BufRead *.aidl			setf aidl

" AMPL
au BufNewFile,BufRead *.run			setf ampl

" Ant
au BufNewFile,BufRead build.xml			setf ant

" ANTLR / PCCTS
"au BufNewFile,BufRead *.g			setf antlr
au BufNewFile,BufRead *.g			setf pccts

" ANTLR 4
au BufNewFile,BufRead *.g4			setf antlr4

" Arduino
au BufNewFile,BufRead *.ino,*.pde		setf arduino

" Ash of busybox
au BufNewFile,BufRead .ash_history		setf sh

" Asymptote
au BufNewFile,BufRead *.asy		setf asy

" Apache config file
au BufNewFile,BufRead .htaccess,*/etc/httpd/*.conf		setf apache
au BufNewFile,BufRead */etc/apache2/sites-*/*.com		setf apache

" XA65 MOS6510 cross assembler
au BufNewFile,BufRead *.a65			setf a65

" Applescript
au BufNewFile,BufRead *.scpt			setf applescript

" Automake (must be before the *.am pattern)
au BufNewFile,BufRead [mM]akefile.am,GNUmakefile.am	setf automake

" Applix ELF
au BufNewFile,BufRead *.am			setf elf

" ALSA configuration
au BufNewFile,BufRead .asoundrc,*/usr/share/alsa/alsa.conf,*/etc/asound.conf setf alsaconf

" Arc Macro Language
au BufNewFile,BufRead *.aml			setf aml

" APT config file
au BufNewFile,BufRead apt.conf		       setf aptconf
au BufNewFile,BufRead */.aptitude/config       setf aptconf
" more generic pattern far down

" Arch Inventory file
au BufNewFile,BufRead .arch-inventory,=tagging-method	setf arch

" ART*Enterprise (formerly ART-IM)
au BufNewFile,BufRead *.art			setf art

" AsciiDoc
au BufNewFile,BufRead *.asciidoc,*.adoc		setf asciidoc

" ASN.1
au BufNewFile,BufRead *.asn,*.asn1		setf asn

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

" Assembly - Netwide
au BufNewFile,BufRead *.nasm			setf nasm

" Assembly - Microsoft
au BufNewFile,BufRead *.masm			setf masm

" Assembly - Macro (VAX)
au BufNewFile,BufRead *.mar			setf vmasm

" Astro
au BufNewFile,BufRead *.astro			setf astro

" Atlas
au BufNewFile,BufRead *.atl,*.as		setf atlas

" Atom is based on XML
au BufNewFile,BufRead *.atom			setf xml

" Authzed
au BufNewFile,BufRead *.zed			setf authzed

" Autoit v3
au BufNewFile,BufRead *.au3			setf autoit

" Autohotkey
au BufNewFile,BufRead *.ahk			setf autohotkey

" Autotest .at files are actually m4
au BufNewFile,BufRead *.at			setf m4

" Avenue
au BufNewFile,BufRead *.ave			setf ave

" Awk
au BufNewFile,BufRead *.awk,*.gawk		setf awk

" B
au BufNewFile,BufRead *.mch,*.ref,*.imp		setf b

" BASIC or Visual Basic
au BufNewFile,BufRead *.bas			call dist#ft#FTbas()
au BufNewFile,BufRead *.bi,*.bm			call dist#ft#FTbas()

" Bass
au BufNewFile,BufRead *.bass			setf bass

" IBasic file (similar to QBasic)
au BufNewFile,BufRead *.iba,*.ibi		setf ibasic

" FreeBasic file (similar to QBasic)
au BufNewFile,BufRead *.fb			setf freebasic

" Batch file for MSDOS. See dist#ft#FTsys for *.sys
au BufNewFile,BufRead *.bat			setf dosbatch
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

" BC calculator
au BufNewFile,BufRead *.bc			setf bc

" BDF font
au BufNewFile,BufRead *.bdf			setf bdf

" Beancount
au BufNewFile,BufRead *.beancount		setf beancount

" BibTeX bibliography database file
au BufNewFile,BufRead *.bib			setf bib

" BibTeX Bibliography Style
au BufNewFile,BufRead *.bst			setf bst

" Bicep
au BufNewFile,BufRead *.bicep,*.bicepparam			setf bicep

" BIND configuration
" sudoedit uses namedXXXX.conf
au BufNewFile,BufRead named*.conf,rndc*.conf,rndc*.key	setf named

" BIND zone
au BufNewFile,BufRead named.root		setf bindzone
au BufNewFile,BufRead *.zone			setf bindzone
au BufNewFile,BufRead *.db			call dist#ft#BindzoneCheck('')

" Blade
au BufNewFile,BufRead *.blade.php		setf blade

" Blank
au BufNewFile,BufRead *.bl			setf blank

" Bitbake
au BufNewFile,BufRead *.bb,*.bbappend,*.bbclass,*/build/conf/*.conf,*/meta{-*,}/conf/*.conf,*/project-spec/configs/*.conf	setf bitbake

" Blkid cache file
au BufNewFile,BufRead */etc/blkid.tab,*/etc/blkid.tab.old   setf xml

" Brighterscript
au BufNewFile,BufRead *.bs			setf brighterscript

" Brightscript
au BufNewFile,BufRead *.brs			setf brightscript

" BSDL
au BufNewFile,BufRead *.bsd,*.bsdl			setf bsdl

" Bazel (https://bazel.build) and Buck2 (https://buck2.build/)
autocmd BufRead,BufNewFile *.bzl,*.bazel,WORKSPACE,WORKSPACE.bzlmod	setf bzl
if has("fname_case")
  " There is another check for BUILD and BUCK further below.
  autocmd BufRead,BufNewFile *.BUILD,BUILD,BUCK		setf bzl
endif

" Busted (Lua unit testing framework - configuration files)
au BufNewFile,BufRead .busted			setf lua

" Bun history
au BufNewFile,BufRead .bun_repl_history		setf javascript

" Bundle config
au BufNewFile,BufRead */.bundle/config			setf yaml

" C or lpc
au BufNewFile,BufRead *.c			call dist#ft#FTlpc()
au BufNewFile,BufRead *.lpc,*.ulpc		setf lpc

" C3
au BufNewFile,BufRead *.c3,*.c3i,*.c3t		setf c3

" Cairo
au BufNewFile,BufRead *.cairo			setf cairo

" Calendar
au BufNewFile,BufRead calendar			setf calendar

" Cap'n Proto
au BufNewFile,BufRead *.capnp			setf capnp

" Cgdb config file
au BufNewFile,BufRead cgdbrc			setf cgdbrc

" m17n database files. */m17n/* matches installed files, */.m17n.d/* matches
" per-user config files, */m17n-db/* matches the git repo. (must be before
" *.cs)
au BufNewFile,BufRead */{m17n,.m17n.d,m17n-db}/*.{ali,cs,dir,flt,fst,lnm,mic,mim,tbl} setf m17ndb

" C#
au BufNewFile,BufRead *.cs,*.csx,*.cake		setf cs

" CSDL
au BufNewFile,BufRead *.csdl			setf csdl

" Ctags
au BufNewFile,BufRead *.ctags			setf conf

" Cabal
au BufNewFile,BufRead *.cabal			setf cabal

" Cdrdao TOC or LaTeX \tableofcontents files
au BufNewFile,BufRead *.toc
	\ if getline(1) =~# '\\contentsline' |setf tex|else|setf cdrtoc|endif

" Cdrdao config
au BufNewFile,BufRead */etc/cdrdao.conf,*/etc/defaults/cdrdao,*/etc/default/cdrdao,.cdrdao	setf cdrdaoconf

" Cedar
au BufNewFile,BufRead *.cedar			setf cedar

" Cfengine
au BufNewFile,BufRead cfengine.conf		setf cfengine

" ChaiScript
au BufRead,BufNewFile *.chai			setf chaiscript

" Chatito
au BufNewFile,BufRead *.chatito			setf chatito

" Chktex
au BufRead,BufNewFile .chktexrc			setf conf

" Chuck
au BufNewFile,BufRead *.ck			setf chuck

" Comshare Dimension Definition Language
au BufNewFile,BufRead *.cdl			setf cdl

" Conary Recipe
au BufNewFile,BufRead *.recipe			setf conaryrecipe

" Containers config files
au BufNewFile,BufRead */containers/containers.conf{,.d/*.conf}		setf toml
au BufNewFile,BufRead */containers/containers.conf.modules/*.conf	setf toml
au BufNewFile,BufRead */containers/registries.conf{,.d/*.conf}		setf toml
au BufNewFile,BufRead */containers/storage.conf				setf toml

" Corn config file
au BufNewFile,BufRead *.corn			setf corn

" ChainPack Object Notation (CPON)
au BufNewFile,BufRead *.cpon			setf cpon

" Controllable Regex Mutilator
au BufNewFile,BufRead *.crm			setf crm

" Cyn++
au BufNewFile,BufRead *.cyn			setf cynpp

" Cynlib
" .cc and .cpp files can be C++ or Cynlib.
au BufNewFile,BufRead *.cc
	\ if exists("cynlib_syntax_for_cc")|setf cynlib|else|setf cpp|endif
au BufNewFile,BufRead *.cpp
	\ if exists("cynlib_syntax_for_cpp")|setf cynlib|else|setf cpp|endif

" Cypher query language
au BufNewFile,BufRead *.cypher			setf cypher

" C++
au BufNewFile,BufRead *.cxx,*.c++,*.hh,*.hxx,*.hpp,*.ipp,*.moc,*.tcc,*.inl setf cpp
if has("fname_case")
	au BufNewFile,BufRead *.C,*.H if !&fileignorecase | setf cpp | endif
endif

" MS files (ixx: C++ module interface file, Microsoft Project file)
au BufNewFile,BufRead *.ixx,*.mpp setf cpp

" C++ 20 modules (clang)
" https://clang.llvm.org/docs/StandardCPlusPlusModules.html#file-name-requirement
au BufNewFile,BufRead *.cppm,*.ccm,*.cxxm,*.c++m setf cpp

" .h files can be C, C++, Ch, Objective-C, or Objective-C++.
" Set g_filetype_h to set a different filetype
au BufNewFile,BufRead *.h			call dist#ft#FTheader()

" Ch (CHscript)
au BufNewFile,BufRead *.chf			setf ch

" TLH files are C++ headers generated by Visual C++'s #import from typelibs
au BufNewFile,BufRead *.tlh			setf cpp

" Cascading Style Sheets
au BufNewFile,BufRead *.css			setf css

" Century Term Command Scripts (*.cmd too)
au BufNewFile,BufRead *.con			setf cterm

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

" ChordPro
au BufNewFile,BufRead *.chopro,*.crd,*.cho,*.crdpro,*.chordpro	setf chordpro

" Clangd
au BufNewFile,BufRead .clangd			setf yaml

" Clang-format
au BufNewFile,BufRead .clang-format		setf yaml

" Clang-tidy
au BufNewFile,BufRead .clang-tidy		setf yaml

" Conda configuration file
au BufNewFile,BufRead .condarc,condarc		setf yaml

" Matplotlib
au BufNewFile,BufRead *.mplstyle,matplotlibrc	setf yaml

" Clean
au BufNewFile,BufRead *.dcl,*.icl		setf clean

" Clever
au BufNewFile,BufRead *.eni			setf cl

" Clever or dtd
au BufNewFile,BufRead *.ent			call dist#ft#FTent()

" Cling
au BufNewFile,BufRead .cling_history		setf cpp

" Clipper, FoxPro, ABB RAPID or eviews
au BufNewFile,BufRead *.prg			call dist#ft#FTprg()
if has("fname_case")
  au BufNewFile,BufRead *.Prg,*.PRG			call dist#ft#FTprg()
endif

" Clojure
au BufNewFile,BufRead *.clj,*.cljs,*.cljx,*.cljc		setf clojure

" Cmake
au BufNewFile,BufRead CMakeLists.txt,*.cmake,*.cmake.in		setf cmake

" CmakeCache
autocmd BufRead,BufNewFile CMakeCache.txt			setf cmakecache

" Cmusrc
au BufNewFile,BufRead */.cmus/{autosave,rc,command-history,*.theme} setf cmusrc
au BufNewFile,BufRead */cmus/{rc,*.theme}			setf cmusrc

" Cobol
au BufNewFile,BufRead *.cbl,*.cob	setf cobol
"   cobol or zope form controller python script? (heuristic)
au BufNewFile,BufRead *.cpy
	\ if getline(1) =~ '^##' |
	\   setf python |
	\ else |
	\   setf cobol |
	\ endif

" Coco/R
au BufNewFile,BufRead *.atg			setf coco

" Cold Fusion
au BufNewFile,BufRead *.cfm,*.cfi,*.cfc		setf cf

" Configure scripts
au BufNewFile,BufRead configure.in,configure.ac setf config

" Cooklang
au BufNewFile,BufRead *.cook			setf cook

" Clinical Quality Language (CQL)
" .cql is also mentioned as the 'XDCC Catcher queue list' file extension.
" If support for XDCC Catcher is needed in the future, the contents of the file
" needs to be inspected.
au BufNewFile,BufRead *.cql			setf cqlang

" Crystal
au BufNewFile,BufRead *.cr			setf crystal

" CSV Files
au BufNewFile,BufRead *.csv			setf csv

" CUDA Compute Unified Device Architecture
au BufNewFile,BufRead *.cu,*.cuh		setf cuda

" Cue
au BufNewFile,BufRead *.cue			setf cue

" DAX
au BufNewFile,BufRead *.dax			setf dax

" Debian devscripts
au BufNewFile,BufRead devscripts.conf,.devscripts	setf sh

" Dockerfile; Podman uses the same syntax with name Containerfile
" Also see Dockerfile.* below.
au BufNewFile,BufRead Containerfile,Dockerfile,dockerfile,*.[dD]ockerfile	setf dockerfile

" WildPackets EtherPeek Decoder
au BufNewFile,BufRead *.dcd			setf dcd

" Enlightenment configuration files
au BufNewFile,BufRead *enlightenment/*.cfg	setf c

" Eterm
au BufNewFile,BufRead *Eterm/*.cfg		setf eterm

" Elixir or Euphoria
au BufNewFile,BufRead *.ex call dist#ft#ExCheck()

" Elixir
au BufRead,BufNewFile mix.lock,*.exs setf elixir
au BufRead,BufNewFile *.eex,*.leex setf eelixir

" Elvish
au BufRead,BufNewFile *.elv setf elvish

" Euphoria 3 or 4
au BufNewFile,BufRead *.eu,*.ew,*.exu,*.exw  call dist#ft#EuphoriaCheck()
if has("fname_case")
   au BufNewFile,BufRead *.EU,*.EW,*.EX,*.EXU,*.EXW  call dist#ft#EuphoriaCheck()
endif

" Execline (s6) scripts
au BufNewFile,BufRead *s6*/\(up\|down\|run\|finish\)    setf execline

" Fontconfig config files
au BufNewFile,BufRead fonts.conf			setf xml

" Faust
au BufNewFile,BufRead *.lib				setf faust
au BufNewFile,BufRead *.dsp				call dist#ft#FTdsp()

" Libreoffice config files
au BufNewFile,BufRead *.xcu,*.xlb,*.xlc,*.xba		setf xml
au BufNewFile,BufRead psprint.conf,sofficerc		setf dosini

" Libtool files
au BufNewFile,BufRead *.lo,*.la,*.lai		setf sh

" Lynx config files
au BufNewFile,BufRead lynx.cfg			setf lynx

" LyRiCs
au BufNewFile,BufRead *.lrc			setf lyrics

" MLIR
au BufNewFile,BufRead *.mlir			setf mlir

" Modula-3 configuration language (must be before *.cfg and *makefile)
au BufNewFile,BufRead *.quake,cm3.cfg		setf m3quake
au BufNewFile,BufRead m3makefile,m3overrides	setf m3build

" XDG mimeapps.list
au BufNewFile,BufRead mimeapps.list	setf dosini

" Many tools written in Python use dosini as their config
" like setuptools, pudb, coverage, pypi, gitlint, oelint-adv, pylint, bpython, mypy
" (must be before *.cfg)
au BufNewFile,BufRead pip.conf,setup.cfg,pudb.cfg,.coveragerc,.pypirc,.gitlint,.oelint.cfg	setf dosini
au BufNewFile,BufRead {.,}pylintrc,*/bpython/config,*/mypy/config			setf dosini

" Many tools written in Python use toml as their config, like black
au BufNewFile,BufRead .black	setf toml
au BufNewFile,BufRead black
	\  if getline(1) =~ 'tool.back'
	\|   setf toml
	\| endif

" LXQt's programs use dosini as their config
au BufNewFile,BufRead */{lxqt,screengrab}/*.conf	setf dosini

" Quake
au BufNewFile,BufRead *baseq[2-3]/*.cfg,*id1/*.cfg	setf quake
au BufNewFile,BufRead *quake[1-3]/*.cfg			setf quake

" Quake C
au BufNewFile,BufRead *.qc			setf c

" LaTeX packages use LaTeX as their configuration, such as:
" ~/.texlive/texmf-config/tex/latex/hyperref/hyperref.cfg
" ~/.texlive/texmf-config/tex/latex/docstrip/docstrip.cfg
au BufNewFile,BufRead */tex/latex/**.cfg		setf tex

" Wakatime config
au BufNewFile,BufRead .wakatime.cfg		setf dosini

" Configure files
au BufNewFile,BufRead *.cfg			call dist#ft#FTcfg()
if has("fname_case")
  au BufNewFile,BufRead *.Cfg,*.CFG			call dist#ft#FTcfg()
endif

" Cucumber
au BufNewFile,BufRead *.feature			setf cucumber

" Communicating Sequential Processes
au BufNewFile,BufRead *.csp,*.fdr		setf csp

" CUPL logic description and simulation
au BufNewFile,BufRead *.pld			setf cupl
au BufNewFile,BufRead *.si			setf cuplsim

" Dafny
au BufNewFile,BufRead *.dfy			setf dafny

" Dart
au BufRead,BufNewfile *.dart,*.drt		setf dart

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

" Deno history
au BufNewFile,BufRead deno_history.txt		setf javascript

" Deny hosts
au BufNewFile,BufRead denyhosts.conf		setf denyhosts

" Dhall
au BufNewFile,BufRead *.dhall			setf dhall

" dnsmasq(8) configuration files
au BufNewFile,BufRead */etc/dnsmasq.conf	setf dnsmasq

" ROCKLinux package description
au BufNewFile,BufRead *.desc			setf desc

" the D language or dtrace
au BufNewFile,BufRead */dtrace/*.d		setf dtrace
au BufNewFile,BufRead *.d			call dist#ft#DtraceCheck()

" Desktop files
au BufNewFile,BufRead *.desktop,*.directory	setf desktop

" Dict config
au BufNewFile,BufRead dict.conf,.dictrc		setf dictconf

" Dictd config
au BufNewFile,BufRead dictd*.conf		setf dictdconf

" DEP3 formatted patch files
au BufNewFile,BufRead */debian/patches/*	call dist#ft#Dep3patch()

" Diff files
au BufNewFile,BufRead *.diff,*.rej		setf diff
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

" DOT
au BufNewFile,BufRead *.dot,*.gv		setf dot

" Dune
au BufNewFile,BufRead jbuild,dune,dune-project,dune-workspace,dune-file setf dune

" Dylan - lid files
au BufNewFile,BufRead *.lid			setf dylanlid

" Dylan - intr files (melange)
au BufNewFile,BufRead *.intr			setf dylanintr

" Dylan
au BufNewFile,BufRead *.dylan			setf dylan

" Microsoft Module Definition or Modula-2
au BufNewFile,BufRead *.def			call dist#ft#FTdef()

if has("fname_case")
  au BufNewFile,BufRead *.DEF			setf modula2
endif

" Dracula
au BufNewFile,BufRead *.drac,*.drc,*.lvs,*.lpe	setf dracula

" Datascript
au BufNewFile,BufRead *.ds			setf datascript

" dsl: DSSSL or Structurizr
au BufNewFile,BufRead *.dsl
	\ if getline(1) =~ '^\s*<\!' |
	\   setf dsl |
	\ else |
	\   setf structurizr |
	\ endif

" DTD (Document Type Definition for XML)
au BufNewFile,BufRead *.dtd			setf dtd

" Devicetree (.its for U-Boot Flattened Image Trees, .keymap for ZMK keymap, and
" .overlay for Zephyr overlay)
au BufNewFile,BufRead *.dts,*.dtsi,*.dtso	setf dts
au BufNewFile,BufRead *.its			setf dts
au BufNewFile,BufRead *.keymap			setf dts
au BufNewFile,BufRead *.overlay			setf dts

" Earthfile
au BufNewFile,BufRead Earthfile			setf earthfile

" EDIF (*.edf,*.edif,*.edn,*.edo) or edn
au BufNewFile,BufRead *.ed\(f\|if\|o\)		setf edif
au BufNewFile,BufRead *.edn
	\ if getline(1) =~ '^\s*(\s*edif\>' |
	\   setf edif |
	\ else |
	\   setf clojure |
	\ endif

" EditorConfig
au BufNewFile,BufRead .editorconfig		setf editorconfig

" Embedix Component Description
au BufNewFile,BufRead *.ecd			setf ecd

" Eiffel or Specman or Euphoria
au BufNewFile,BufRead *.e,*.E			call dist#ft#FTe()

" Elinks configuration
au BufNewFile,BufRead elinks.conf		setf elinks

" ERicsson LANGuage; Yaws is erlang too
au BufNewFile,BufRead *.erl,*.hrl,*.yaws	setf erlang

" Elm
au BufNewFile,BufRead *.elm			setf elm

" Elm Filter Rules file
au BufNewFile,BufRead filter-rules		setf elmfilt

" Elsa - https://github.com/ucsd-progsys/elsa
au BufNewFile,BufRead *.lc			setf elsa

" EdgeDB Schema Definition Language
au BufNewFile,BufRead *.esdl			setf esdl

" ESMTP rc file
au BufNewFile,BufRead *esmtprc			setf esmtprc

" ESQL-C
au BufNewFile,BufRead *.ec,*.EC			setf esqlc

" Esterel
au BufNewFile,BufRead *.strl			setf esterel

" Essbase script
au BufNewFile,BufRead *.csc			setf csc

" Exim
au BufNewFile,BufRead exim.conf			setf exim

" Expect
au BufNewFile,BufRead *.exp			setf expect

" Exports
au BufNewFile,BufRead exports			setf exports

" Falcon
au BufNewFile,BufRead *.fal			setf falcon

" Fantom
au BufNewFile,BufRead *.fan,*.fwt		setf fan

" Factor
au BufNewFile,BufRead *.factor			setf factor

" Fennel
autocmd BufRead,BufNewFile *.fnl,{,.}fennelrc	setf fennel

" Fetchmail RC file
au BufNewFile,BufRead .fetchmailrc		setf fetchmail

" FGA
au BufNewFile,BufRead *.fga			setf fga

" FIRRTL - Flexible Internal Representation for RTL
au BufNewFile,BufRead *.fir			setf firrtl

" Fish shell
au BufNewFile,BufRead *.fish			setf fish

" Flatpak config
au BufNewFile,BufRead */flatpak/repo/config	setf dosini

" Flix
au BufNewFile,BufRead *.flix			setf flix

" Focus Executable
au BufNewFile,BufRead *.fex,*.focexec		setf focexec

" Focus Master file (but not for auto.master)
au BufNewFile,BufRead auto.master		setf conf
au BufNewFile,BufRead *.mas,*.master		setf master

" Forth
au BufNewFile,BufRead *.ft,*.fth,*.4th		setf forth

" Reva Forth
au BufNewFile,BufRead *.frt			setf reva

" Fortran
if has("fname_case")
  au BufNewFile,BufRead *.F,*.FOR,*.FPP,*.FTN,*.F77,*.F90,*.F95,*.F03,*.F08	setf fortran
endif
au BufNewFile,BufRead *.for,*.fortran,*.fpp,*.ftn,*.f77,*.f90,*.f95,*.f03,*.f08	setf fortran

" Fortran or Forth
au BufNewFile,BufRead *.f			call dist#ft#FTf()

" Framescript
au BufNewFile,BufRead *.fsl			setf framescript

" FStab
au BufNewFile,BufRead fstab,mtab		setf fstab

" Func
au BufNewFile,BufRead *.fc			setf func

" Fusion
au BufRead,BufNewFile *.fusion			setf fusion

" F# or Forth
au BufNewFile,BufRead *.fs			call dist#ft#FTfs()

" FHIR Shorthand (FSH)
au BufNewFile,BufRead *.fsh			setf fsh

" F#
au BufNewFile,BufRead *.fsi,*.fsx		setf fsharp

" GDB command files
au BufNewFile,BufRead .gdbinit,gdbinit,.cuda-gdbinit,cuda-gdbinit,.gdbearlyinit,gdbearlyinit,*.gdb		setf gdb

" GDMO
au BufNewFile,BufRead *.mo,*.gdmo		setf gdmo

" GDscript
au BufNewFile,BufRead *.gd			setf gdscript

" Godot resource
au BufRead,BufNewFile *.tscn,*.tres		setf gdresource

" Godot shader
au BufRead,BufNewFile *.gdshader,*.shader	setf gdshader

" Gedcom
au BufNewFile,BufRead *.ged,lltxxxxx.txt	setf gedcom

" Gemtext
au BufNewFile,BufRead *.gmi,*.gemini		setf gemtext

" Gift (Moodle)
autocmd BufRead,BufNewFile *.gift		setf gift

" Git
au BufNewFile,BufRead COMMIT_EDITMSG,MERGE_MSG,TAG_EDITMSG	setf gitcommit
au BufNewFile,BufRead NOTES_EDITMSG,EDIT_DESCRIPTION		setf gitcommit
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
au BufRead,BufNewFile .gitsendemail.msg.??????			setf gitsendemail
au BufNewFile,BufRead *.git/*
      \ if getline(1) =~# '^\x\{40,\}\>\|^ref: ' |
      \   setf git |
      \ endif

" Gkrellmrc
au BufNewFile,BufRead gkrellmrc,gkrellmrc_?	setf gkrellmrc

" Gleam
au BufNewFile,BufRead *.gleam			setf gleam

" GLSL
" Extensions supported by Khronos reference compiler (with one exception, ".glsl")
" https://github.com/KhronosGroup/glslang
au BufNewFile,BufRead *.vert,*.tesc,*.tese,*.glsl,*.geom,*.frag,*.comp,*.rgen,*.rmiss,*.rchit,*.rahit,*.rint,*.rcall	setf glsl

" GN (generate ninja) files
au BufNewFile,BufRead *.gn,*.gni		setf gn

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

" gnash(1) configuration files
au BufNewFile,BufRead gnashrc,.gnashrc,gnashpluginrc,.gnashpluginrc setf gnash

" Gitolite
au BufNewFile,BufRead gitolite.conf		setf gitolite
au BufNewFile,BufRead {,.}gitolite.rc,example.gitolite.rc	setf perl

" Glimmer-flavored TypeScript and JavaScript
au BufNewFile,BufRead *.gts			setf typescript.glimmer
au BufNewFile,BufRead *.gjs			setf javascript.glimmer

" Gnuplot scripts
au BufNewFile,BufRead *.gpi,*.gnuplot,.gnuplot_history	setf gnuplot

" GNU Radio Companion files
au BufNewFile,BufRead *.grc
	\ if getline(1) =~# '<?xml' |
	\   setf xml |
	\ else |
	\   setf yaml |
	\ endif

" Go (Google)
au BufNewFile,BufRead *.go			setf go
au BufNewFile,BufRead Gopkg.lock		setf toml
au BufRead,BufNewFile go.work			setf gowork

" GoAccess configuration
au BufNewFile,BufRead goaccess.conf		setf goaccess

" GrADS scripts
au BufNewFile,BufRead *.gs			setf grads

" GraphQL
au BufNewFile,BufRead *.graphql,*.graphqls,*.gql			setf graphql

" Gretl
au BufNewFile,BufRead *.gretl			setf gretl

" Groovy
au BufNewFile,BufRead *.gradle,*.groovy,Jenkinsfile		setf groovy

" GNU Server Pages
au BufNewFile,BufRead *.gsp			setf gsp

" Group file
au BufNewFile,BufRead */etc/group,*/etc/group-,*/etc/group.edit,*/etc/gshadow,*/etc/gshadow-,*/etc/gshadow.edit,*/var/backups/group.bak,*/var/backups/gshadow.bak  setf group

" GTK RC
au BufNewFile,BufRead .gtkrc,gtkrc		setf gtkrc

" GYP
au BufNewFile,BufRead *.gyp,*.gypi		setf gyp

" Hack
au BufRead,BufNewFile *.hack,*.hackpartial			setf hack

" Haml
au BufNewFile,BufRead *.haml			setf haml

" Hamster Classic | Playground files
au BufNewFile,BufRead *.hsm			setf hamster

" Handlebars
au BufNewFile,BufRead *.hbs			setf handlebars

" Hare
au BufNewFile,BufRead *.ha			setf hare
au BufNewFile,BufRead README			call dist#ft#FTharedoc()

" Haskell
au BufNewFile,BufRead *.hs,*.hsc,*.hs-boot,*.hsig setf haskell
au BufNewFile,BufRead *.lhs			setf lhaskell
au BufNewFile,BufRead *.chs			setf chaskell
au BufNewFile,BufRead cabal.project		setf cabalproject
au BufNewFile,BufRead */{.,}cabal/config	setf cabalconfig
au BufNewFile,BufRead cabal.config		setf cabalconfig
au BufNewFile,BufRead *.persistentmodels	setf haskellpersistent

" Haste
au BufNewFile,BufRead *.ht			setf haste
au BufNewFile,BufRead *.htpp			setf hastepreproc

" Haxe
au BufNewFile,BufRead *.hx			setf haxe

" HCL
au BufRead,BufNewFile *.hcl			setf hcl

" Go checksum file (must be before *.sum Hercules)
au BufNewFile,BufRead go.sum,go.work.sum	setf gosum

" Hercules
au BufNewFile,BufRead *.vc,*.ev,*.sum,*.errsum	setf hercules

" HEEx
au BufRead,BufNewFile *.heex			setf heex

" HEX (Intel)
au BufNewFile,BufRead *.hex,*.ihex,*.int,*.ihe,*.ihx,*.mcs,*.h32,*.h80,*.h86,*.a43,*.a90	setf hex

" Hjson
au BufNewFile,BufRead *.hjson			setf hjson

" HLS Playlist (or another form of playlist)
au BufNewFile,BufRead *.m3u,*.m3u8		setf hlsplaylist

" Hollywood
au BufRead,BufNewFile *.hws			setf hollywood

" Hoon
au BufRead,BufNewFile *.hoon			setf hoon

" TI Code Composer Studio General Extension Language
au BufNewFile,BufRead *.gel			setf gel

" Tilde (must be before HTML)
au BufNewFile,BufRead *.t.html			setf tilde

" Translate shell
au BufNewFile,BufRead init.trans,*/etc/translate-shell,.trans	setf clojure

" HTML (.stm for server side, .shtml is server-side or superhtml)
au BufNewFile,BufRead *.html,*.htm,*.shtml,*.stm  call dist#ft#FThtml()
au BufNewFile,BufRead *.cshtml			setf html

" HTTP request files
au BufNewFile,BufRead *.http			setf http

" HTML with Ruby - eRuby
au BufNewFile,BufRead *.erb,*.rhtml		setf eruby

" HTML with M4
au BufNewFile,BufRead *.html.m4			setf htmlm4

" Some template.  Used to be HTML Cheetah.
au BufNewFile,BufRead *.tmpl			setf template

" Host config
au BufNewFile,BufRead */etc/host.conf		setf hostconf

" Hosts access
au BufNewFile,BufRead */etc/hosts.allow,*/etc/hosts.deny  setf hostsaccess

" Hurl
au BufRead,BufNewFile *.hurl			setf hurl

" Hy
au BufRead,BufNewFile *.hy,.hy-history		setf hy

" Hyper Builder
au BufNewFile,BufRead *.hb			setf hb

" Hyprland Configuration language
au BufNewFile,BufRead */hypr/*.conf,hypr\(land\|paper\|idle\|lock\).conf setf hyprlang

" Httest
au BufNewFile,BufRead *.htt,*.htb		setf httest

" i3
au BufNewFile,BufRead */i3/config		setf i3config
au BufNewFile,BufRead */.i3/config		setf i3config

" sway
au BufNewFile,BufRead */sway/config		setf swayconfig
au BufNewFile,BufRead */.sway/config		setf swayconfig

" Icon
au BufNewFile,BufRead *.icn			setf icon

" IDL (Interface Description Language)
au BufNewFile,BufRead *.idl			call dist#ft#FTidl()

" Microsoft IDL (Interface Description Language)  Also *.idl
" MOF = WMI (Windows Management Instrumentation) Managed Object Format
au BufNewFile,BufRead *.odl,*.mof		setf msidl

" Icewm menu
au BufNewFile,BufRead */.icewm/menu		setf icemenu

" Indent profile (must come before IDL *.pro!)
au BufNewFile,BufRead .indent.pro		setf indent
au BufNewFile,BufRead indent.pro		call dist#ft#ProtoCheck('indent')

" IDL (Interactive Data Language), Prolog, Cproto or zsh module C
au BufNewFile,BufRead *.pro			call dist#ft#ProtoCheck('idlang')

" Idris2
au BufNewFile,BufRead *.idr			setf idris2
au BufNewFile,BufRead *.lidr			setf lidris2

" Indent RC
au BufNewFile,BufRead indentrc			setf indent

" Inform
au BufNewFile,BufRead *.inf,*.INF		setf inform

" Initng
au BufNewFile,BufRead */etc/initng/*/*.i,*.ii	setf initng

" Innovation Data Processing
au BufRead,BufNewFile upstream.dat\c,upstream.*.dat\c,*.upstream.dat\c	setf upstreamdat
au BufRead,BufNewFile fdrupstream.log,upstream.log\c,upstream.*.log\c,*.upstream.log\c,UPSTREAM-*.log\c	setf upstreamlog
au BufRead,BufNewFile upstreaminstall.log\c,upstreaminstall.*.log\c,*.upstreaminstall.log\c setf upstreaminstalllog
au BufRead,BufNewFile usserver.log\c,usserver.*.log\c,*.usserver.log\c	setf usserverlog
au BufRead,BufNewFile usw2kagt.log\c,usw2kagt.*.log\c,*.usw2kagt.log\c	setf usw2kagtlog

" Ipfilter
au BufNewFile,BufRead ipf.conf,ipf6.conf,ipf.rules	setf ipfilter

" Ipkg for Idris 2 language
au BufNewFile,BufRead *.ipkg			setf ipkg

" Informix 4GL (source - canonical, include file, I4GL+M4 preproc.)
au BufNewFile,BufRead *.4gl,*.4gh,*.m4gl	setf fgl

" .INI file for MSDOS
au BufNewFile,BufRead *.ini,*.INI		setf dosini

" SysV Inittab
au BufNewFile,BufRead inittab			setf inittab

" Inko
au BufNewFile,BufRead *.inko			setf inko

" Inno Setup
au BufNewFile,BufRead *.iss			setf iss

" J
au BufNewFile,BufRead *.ijs			setf j

" JAL
au BufNewFile,BufRead *.jal,*.JAL		setf jal

" Jam
au BufNewFile,BufRead *.jpl,*.jpr		setf jam

" Janet
au BufNewFile,BufRead *.janet			setf janet

" Java
au BufNewFile,BufRead *.java,*.jav,*.jsh	setf java

" JavaCC
au BufNewFile,BufRead *.jj,*.jjt		setf javacc

" JavaScript, ECMAScript, ES module script, CommonJS script
au BufNewFile,BufRead *.js,*.jsm,*.javascript,*.es,*.mjs,*.cjs   setf javascript
au BufNewFile,BufRead .node_repl_history	setf javascript

" JavaScript with React
au BufNewFile,BufRead *.jsx			setf javascriptreact

" Java Server Pages
au BufNewFile,BufRead *.jsp			setf jsp

" Java Properties resource file (note: doesn't catch font.properties.pl)
au BufNewFile,BufRead *.properties,*.properties_??,*.properties_??_??	setf jproperties
" Eclipse preference files use Java Properties syntax
au BufNewFile,BufRead org.eclipse.*.prefs	setf jproperties

" Jess
au BufNewFile,BufRead *.clp			setf jess

" Jgraph
au BufNewFile,BufRead *.jgr			setf jgraph

" Jinja
au BufNewFile,BufRead *.jinja			setf jinja

" Jujutsu
au BufNewFile,BufRead *.jjdescription		setf jjdescription

" Jovial
au BufNewFile,BufRead *.jov,*.j73,*.jovial	setf jovial

" Jq
au BufNewFile,BufRead *.jq			setf jq

" JSON5
au BufNewFile,BufRead *.json5			setf json5

" JSON Patch (RFC 6902)
au BufNewFile,BufRead *.json-patch		setf json

" Geojson is also json
au BufNewFile,BufRead *.geojson			setf json

" Jupyter Notebook and jupyterlab config is also json
au BufNewFile,BufRead *.ipynb,*.jupyterlab-settings	setf json

" Sublime config
au BufNewFile,BufRead *.sublime-project,*.sublime-settings,*.sublime-workspace	setf json

" Other files that look like json
au BufNewFile,BufRead .prettierrc,.firebaserc,.stylelintrc,.lintstagedrc,flake.lock,deno.lock,.swcrc,composer.lock,symfony.lock	setf json

" JSONC (JSON with comments)
au BufNewFile,BufRead *.jsonc,.babelrc,.eslintrc,.jsfmtrc,bun.lock	setf jsonc
au BufNewFile,BufRead .jshintrc,.jscsrc,.vsconfig,.hintrc,.swrc,[jt]sconfig*.json	setf jsonc
" Visual Studio Code settings
au BufRead,BufNewFile ~/*/{Code,VSCodium}/User/*.json setf jsonc

" JSON
au BufNewFile,BufRead *.json,*.jsonp,*.webmanifest	setf json

" JSON Lines
au BufNewFile,BufRead *.jsonl			setf jsonl

" Jsonnet
au BufNewFile,BufRead *.jsonnet,*.libsonnet	setf jsonnet

" Julia
au BufNewFile,BufRead *.jl			setf julia

" Just
au BufNewFile,BufRead \c{,*.}justfile,\c*.just setf just

" KAREL
au BufNewFile,BufRead *.kl setf karel
if has("fname_case")
   au BufNewFile,BufRead *.KL setf karel
endif

" KDL
au BufNewFile,BufRead *.kdl			setf kdl

" Kixtart
au BufNewFile,BufRead *.kix			setf kix

" Kuka Robot Language
au BufNewFile,BufRead *.src			call dist#ft#FTsrc()
au BufNewFile,BufRead *.dat			call dist#ft#FTdat()
au BufNewFile,BufRead *.sub			setf krl
if has("fname_case")
   au BufNewFile,BufRead *.Src,*.SRC		call dist#ft#FTsrc()
   au BufNewFile,BufRead *.Dat,*.DAT		call dist#ft#FTdat()
   au BufNewFile,BufRead *.Sub,*.SUB		setf krl
endif

" Kimwitu[++]
au BufNewFile,BufRead *.k			setf kwt

" Kivy
au BufNewFile,BufRead *.kv			setf kivy

" Kotlin
au BufNewFile,BufRead *.kt,*.ktm,*.kts		setf kotlin

" KDE script
au BufNewFile,BufRead *.ks			setf kscript

" Kconfig
au BufNewFile,BufRead Kconfig,Kconfig.debug,Config.in	setf kconfig

" Lace (ISE)
au BufNewFile,BufRead *.ace,*.ACE		setf lace

" Lalrpop
au BufNewFile,Bufread *.lalrpop			setf lalrpop

" Larch Shared Language
au BufNewFile,BufRead .lsl			call dist#ft#FTlsl()

" Latexmkrc
au BufNewFile,BufRead .latexmkrc,latexmkrc	setf perl

" Latte
au BufNewFile,BufRead *.latte,*.lte		setf latte

" Limits
au BufNewFile,BufRead */etc/limits,*/etc/*limits.conf,*/etc/*limits.d/*.conf	setf limits

" LambdaProlog or SML (see dist#ft#FTmod for *.mod)
au BufNewFile,BufRead *.sig			call dist#ft#FTsig()

" LDAP configuration
au BufNewFile,BufRead ldaprc,.ldaprc,ldap.conf	setf ldapconf

" LDAP LDIF
au BufNewFile,BufRead *.ldif			setf ldif

" Luadoc, Ldoc (must be before *.ld)
au BufNewFile,BufRead config.ld			setf lua

" Ld loader
au BufNewFile,BufRead *.ld,*/ldscripts/*	setf ld

" Lean
au BufNewFile,BufRead *.lean			setf lean

" Ledger
au BufRead,BufNewFile *.ldg,*.ledger,*.journal			setf ledger

" lf configuration (lfrc)
au BufNewFile,BufRead lfrc			setf lf

" Leo
au BufNewFile,BufRead *.leo			setf leo

" Less
au BufNewFile,BufRead *.less			setf less

" Lex
au BufNewFile,BufRead *.lex,*.l,*.lxx,*.l++	setf lex

" Libao
au BufNewFile,BufRead */etc/libao.conf,*/.libao	setf libao

" Libsensors
au BufNewFile,BufRead */etc/sensors.conf,*/etc/sensors3.conf	setf sensors

" LFTP
au BufNewFile,BufRead lftp.conf,.lftprc,*lftp/rc	setf lftp

" Lifelines, LLVM, or Lex for C++
au BufNewFile,BufRead *.ll			call dist#ft#FTll()

" Lilo: Linux loader
au BufNewFile,BufRead lilo.conf			setf lilo

" Lilypond
au BufNewFile,BufRead *.ly,*.ily		setf lilypond

" Lisp (*.el = ELisp)
" *.jl was removed, it's also used for Julia, better skip than guess wrong.
if has("fname_case")
  au BufNewFile,BufRead *.lsp,*.lisp,*.asd,*.el,*.L,.emacs,.sawfishrc setf lisp
else
  au BufNewFile,BufRead *.lsp,*.lisp,*.asd,*.el,.emacs,.sawfishrc setf lisp
endif

" *.cl = Common Lisp or OpenCL
au BufNewFile,BufRead *.cl call dist#ft#FTcl()

" SBCL implementation of Common Lisp
au BufNewFile,BufRead sbclrc,.sbclrc		setf lisp

" Liquidsoap
au BufNewFile,BufRead *.liq			setf liquidsoap

" Liquid
au BufNewFile,BufRead *.liquid			setf liquid

" Lite
au BufNewFile,BufRead *.lite,*.lt		setf lite

" LiteStep RC files
au BufNewFile,BufRead */LiteStep/*/*.rc		setf litestep

" Livebook
au BufNewFile,BufRead *.livemd			setf livebook

" Login access
au BufNewFile,BufRead */etc/login.access	setf loginaccess

" Login defs
au BufNewFile,BufRead */etc/login.defs		setf logindefs

" Logtalk
au BufNewFile,BufRead *.lgt			setf logtalk

" LOTOS
au BufNewFile,BufRead *.lotos		setf lotos

" LOTOS or LaTeX \listoftables files
au BufNewFile,BufRead *.lot
	\ if getline(1) =~# '\\contentsline' |setf tex|else|setf lotos|endif

" Lout (also: *.lt)
au BufNewFile,BufRead *.lou,*.lout		setf lout

" Lua, Texlua
au BufNewFile,BufRead *.lua,*.tlu,.lua_history	setf lua

" Luau
au BufNewFile,BufRead *.luau		setf luau

" Luau config
au BufNewFile,BufRead .luaurc		setf jsonc

" Luacheck
au BufNewFile,BufRead .luacheckrc		setf lua

" Luarocks
au BufNewFile,BufRead *.rockspec,rock_manifest	setf lua

" Linden Scripting Language (Second Life)
au BufNewFile,BufRead *.lsl			call dist#ft#FTlsl()

" Lynx style file (or LotusScript!)
au BufNewFile,BufRead *.lss			setf lss

" M4
au BufNewFile,BufRead *.m4
	\ if expand("<afile>") !~? 'html.m4$\|fvwm2rc' | setf m4 | endif
au BufNewFile,BufRead .m4_history		setf m4

" MaGic Point
au BufNewFile,BufRead *.mgp			setf mgp

" Mail (for Elm, trn, mutt, muttng, rn, slrn, neomutt)
au BufNewFile,BufRead snd.\d\+,.letter,.letter.\d\+,.followup,.article,.article.\d\+,pico.\d\+,mutt{ng,}-*-\w\+,mutt[[:alnum:]_-]\\\{6\},neomutt-*-\w\+,neomutt[[:alnum:]_-]\\\{6\},ae\d\+.txt,/tmp/SLRN[0-9A-Z.]\+,*.eml setf mail

" Mail aliases
au BufNewFile,BufRead */etc/mail/aliases,*/etc/aliases	setf mailaliases

" Mailcap configuration file
au BufNewFile,BufRead .mailcap,mailcap		setf mailcap

" Makefile
au BufNewFile,BufRead *[mM]akefile,*.mk,*.mak	call dist#ft#FTmake()
au BufNewFile,BufRead Kbuild			setf make

" MakeIndex
au BufNewFile,BufRead *.ist,*.mst		setf ist

" Mallard
au BufNewFile,BufRead *.page			setf mallard

" Manpage
au BufNewFile,BufRead *.man			setf man

" Man config
au BufNewFile,BufRead */etc/man.conf,man.config	setf manconf

" Maple V
au BufNewFile,BufRead *.mv,*.mpl,*.mws		setf maple

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

" Mason (it used to include *.comp, are those Mason files?)
au BufNewFile,BufRead *.mason,*.mhtml	setf mason

" Mathematica, Matlab, Murphi, Objective C or Octave
au BufNewFile,BufRead *.m			call dist#ft#FTm()

" Mathematica notebook and package files
au BufNewFile,BufRead *.nb,*.wl			setf mma

" Maya Extension Language
au BufNewFile,BufRead *.mel			setf mel

" mbsync
au BufNewFile,BufRead *.mbsyncrc,isyncrc	setf mbsync

" mcmeta
au BufNewFile,BufRead *.mcmeta			setf json

" MediaWiki
au BufNewFile,BufRead *.mw,*.wiki		setf mediawiki

" Mercurial (hg) commit file
au BufNewFile,BufRead hg-editor-*.txt		setf hgcommit

" Mercurial config (looks like generic config file)
au BufNewFile,BufRead *.hgrc,*hgrc		setf cfg

" Mermaid
au BufNewFile,BufRead *.mmd,*.mmdc,*.mermaid	setf mermaid

" Meson Build system config
au BufNewFile,BufRead meson.build,meson.options,meson_options.txt setf meson
au BufNewFile,BufRead *.wrap			setf dosini

" Metafont
au BufNewFile,BufRead *.mf			setf mf

" MetaPost
au BufNewFile,BufRead *.mp			setf mp
au BufNewFile,BufRead *.mpxl,*.mpiv,*.mpvi	let b:mp_metafun = 1 | setf mp

" MGL
au BufNewFile,BufRead *.mgl			setf mgl

" MIX - Knuth assembly
au BufNewFile,BufRead *.mix,*.mixal		setf mix

" MMIX or VMS makefile
au BufNewFile,BufRead *.mms			call dist#ft#FTmms()

" msmtp
au BufNewFile,BufRead .msmtprc			setf msmtp

" Symbian meta-makefile definition (MMP)
au BufNewFile,BufRead *.mmp			setf mmp

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

" Larch/Modula-3
au BufNewFile,BufRead *.lm3			setf modula3

" Modconf
au BufNewFile,BufRead */etc/modules.conf,*/etc/modules,*/etc/conf.modules setf modconf

" Monk
au BufNewFile,BufRead *.isc,*.monk,*.ssc,*.tsc	setf monk

" MOO
au BufNewFile,BufRead *.moo			setf moo

" Moonscript
au BufNewFile,BufRead *.moon			setf moonscript

" Move language
au BufNewFile,BufRead *.move			setf move

" MPD is based on XML
au BufNewFile,BufRead *.mpd			setf xml

" Mplayer config
au BufNewFile,BufRead mplayer.conf,*/.mplayer/config	setf mplayerconf

" Motorola S record
au BufNewFile,BufRead *.s19,*.s28,*.s37,*.mot,*.srec	setf srec

" Mrxvtrc
au BufNewFile,BufRead mrxvtrc,.mrxvtrc		setf mrxvtrc

" Msql
au BufNewFile,BufRead *.msql			setf msql

" Mysql
au BufNewFile,BufRead *.mysql,.mysql_history	setf mysql

" Tcl Shell RC file
au BufNewFile,BufRead tclsh.rc			setf tcl

" M$ Resource files
" /etc/Muttrc.d/file.rc is muttrc
au BufNewFile,BufRead *.rc,*.rch
	\ if expand("<afile>") !~ "/etc/Muttrc.d/" |
	\   setf rc |
	\ endif

" Mojo
" Mojo files use either .mojo or .🔥 as extension
au BufNewFile,BufRead *.mojo,*.🔥		setf mojo

" MuPAD source
au BufRead,BufNewFile *.mu			setf mupad

" Mush
au BufNewFile,BufRead *.mush			setf mush

" Mustache
au BufNewFile,BufRead *.mustache		setf mustache

" Mutt setup file (also for Muttng)
au BufNewFile,BufRead Mutt{ng,}rc		setf muttrc

" N1QL
au BufRead,BufNewfile *.n1ql,*.nql		setf n1ql

" Neomutt log
au BufNewFile,BufRead *.neomuttdebug*		setf neomuttlog

" Nano
au BufNewFile,BufRead */etc/nanorc,*.nanorc	setf nanorc

" Nastran input/DMAP
"au BufNewFile,BufRead *.dat			setf nastran

" Natural
au BufNewFile,BufRead *.NS[ACGLMNPS]		setf natural

" Noemutt setup file
au BufNewFile,BufRead Neomuttrc			setf neomuttrc

" Netrc
au BufNewFile,BufRead .netrc			setf netrc

" Neofetch
au BufNewFile,BufRead */neofetch/config.conf	setf sh

" Nginx
au BufNewFile,BufRead *.nginx,nginx*.conf,*nginx.conf,*/nginx/*.conf	setf nginx

" Nim file
au BufNewFile,BufRead *.nim,*.nims,*.nimble	setf nim

" Ninja file
au BufNewFile,BufRead *.ninja			setf ninja

" Nix
au BufRead,BufNewFile *.nix			setf nix

" Norg
au BufNewFile,BufRead *.norg		setf norg

" NPM RC file
au BufNewFile,BufRead npmrc,.npmrc		setf dosini

" Novell netware batch files
au BufNewFile,BufRead *.ncf			setf ncf

" Nroff/Troff (*.ms and *.t are checked below)
au BufNewFile,BufRead *.me
	\ if expand("<afile>") != "read.me" && expand("<afile>") != "click.me" |
	\   setf nroff |
	\ endif
au BufNewFile,BufRead *.tr,*.nr,*.roff,*.tmac,*.mom	setf nroff
au BufNewFile,BufRead *.[0-9],*.[013]p,*.[1-8]x,*.3{am,perl,pm,posix,type},*.n	call dist#ft#FTnroff()

" Nroff or Objective C++
au BufNewFile,BufRead *.mm			call dist#ft#FTmm()

" Not Quite C
au BufNewFile,BufRead *.nqc			setf nqc

" notmuch
au BufNewFile,BufRead .notmuch-config{,.*}		setf dosini
au BufNewFile,BufRead ~/.config/notmuch/*/config	setf dosini
if exists('$XDG_CONFIG_HOME')
  au BufNewFile,BufRead $XDG_CONFIG_HOME/notmuch/*/config setf dosini
endif

" NSE - Nmap Script Engine - uses Lua syntax
au BufNewFile,BufRead *.nse			setf lua

" NSIS
au BufNewFile,BufRead *.nsi,*.nsh		setf nsis

" N-Triples
au BufNewFile,BufRead *.nt			setf ntriples

" Nu
au BufNewFile,BufRead *.nu		setf nu

" Numbat
au BufNewFile,BufRead *.nbt		setf numbat

" Oblivion Language and Oblivion Script Extender
au BufNewFile,BufRead *.obl,*.obse,*.oblivion,*.obscript  setf obse

" Objdump
au BufNewFile,BufRead *.objdump,*.cppobjdump  setf objdump

" OCaml
au BufNewFile,BufRead *.ml,*.mli,*.mll,*.mly,.ocamlinit,*.mlt,*.mlp,*.mlip,*.mli.cppo,*.ml.cppo setf ocaml

" Occam
au BufNewFile,BufRead *.occ			setf occam

" Octave
au BufNewFile,BufRead octave.conf,.octaverc,octaverc,*/octave/history	setf octave

" Odin
au BufNewFile,BufRead *.odin			setf odin

" Omnimark
au BufNewFile,BufRead *.xom,*.xin		setf omnimark

" ondir
au BufNewFile,BufRead .ondirrc			setf ondir

" OPAM
au BufNewFile,BufRead opam,*.opam,*.opam.template,opam.locked,*.opam.locked setf opam

" OpenAL Soft config files
au BufNewFile,BufRead .alsoftrc,alsoft.conf,alsoft.ini,alsoftrc.sample setf dosini

" OpenFOAM
au BufNewFile,BufRead fvSchemes,fvSolution,fvConstrains,fvModels,*/constant/g	call dist#ft#FTfoam()

" OpenROAD
au BufNewFile,BufRead *.or				setf openroad

" OPL
au BufNewFile,BufRead *.[Oo][Pp][Ll]			setf opl

" OpenSCAD
au BufNewFile,BufRead *.scad				setf openscad

" Oracle config file
au BufNewFile,BufRead *.ora				setf ora

" Org (Emacs' org-mode)
au BufNewFile,BufRead *.org,*.org_archive		setf org

" Packet filter conf
au BufNewFile,BufRead pf.conf				setf pf

" ini style config files, using # comments
au BufNewFile,BufRead pacman.conf,mpv.conf		setf confini
au BufNewFile,BufRead */.aws/config,*/.aws/credentials	setf confini
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

" Pam environment
au BufNewFile,BufRead pam_env.conf,.pam_environment	setf pamenv

" PApp
au BufNewFile,BufRead *.papp,*.pxml,*.pxsl		setf papp

" Password file
au BufNewFile,BufRead */etc/passwd,*/etc/passwd-,*/etc/passwd.edit,*/etc/shadow,*/etc/shadow-,*/etc/shadow.edit,*/var/backups/passwd.bak,*/var/backups/shadow.bak setf passwd

" Pascal (also *.p, *.pp, *.inc)
au BufNewFile,BufRead *.pas				setf pascal

" Pascal or Puppet manifest
au BufNewFile,BufRead *.pp				call dist#ft#FTpp()

" Delphi
au BufNewFile,BufRead *.dpr				setf pascal

" Xilinx labtools project file or Lazarus program file
au BufNewFile,BufRead *.lpr
	\ if getline(1) =~# "<?xml" |
	\   setf xml |
	\ else |
	\   setf pascal |
	\ endif

" Free Pascal makefile definition file
au BufNewFile,BufRead *.fpc				setf fpcmake

" Path of Exile item filter
au BufNewFile,BufRead *.filter				setf poefilter

" PDF
au BufNewFile,BufRead *.pdf				setf pdf

" PCMK - HAE - crm configure edit
au BufNewFile,BufRead *.pcmk				setf pcmk

" PEM (Privacy-Enhanced Mail)
au BufNewFile,BufRead *.pem,*.cer,*.crt,*.csr		setf pem

" Perl or Prolog
if has("fname_case")
  au BufNewFile,BufRead *.pl,*.PL			call dist#ft#FTpl()
else
  au BufNewFile,BufRead *.pl				call dist#ft#FTpl()
endif
au BufNewFile,BufRead *.plx,*.al,*.psgi			setf perl

" Perl Reply
au BufNewFile,BufRead .replyrc				setf dosini

" Perl, XPM or XPM2
au BufNewFile,BufRead *.pm
	\ if getline(1) =~ "XPM2" |
	\   setf xpm2 |
	\ elseif getline(1) =~ "XPM" |
	\   setf xpm |
	\ else |
	\   setf perl |
	\ endif

" Perl POD
au BufNewFile,BufRead *.pod			setf pod

" Php, php3, php4, etc.
" Also Phtml (was used for PHP 2 in the past).
" Also .ctp for Cake template file.
" Also .phpt for php tests.
" Also .theme for Drupal theme files.
au BufNewFile,BufRead *.php,*.php\d,*.phtml,*.ctp,*.phpt,*.theme	setf php

" Pike and Cmod
au BufNewFile,BufRead *.pike,*.pmod		setf pike
au BufNewFile,BufRead *.cmod			setf cmod

" Pinfo config
au BufNewFile,BufRead */etc/pinforc,*/.pinforc	setf pinfo

" Palm Resource compiler
au BufNewFile,BufRead *.rcp			setf pilrc

" Pine config
au BufNewFile,BufRead .pinerc,pinerc,.pinercex,pinercex		setf pine

" Pip requirements
au BufNewFile,BufRead *.pip			setf requirements
au BufNewFile,BufRead requirements.txt		setf requirements
au BufNewFile,BufRead *-requirements.txt	setf requirements
au BufNewFile,BufRead requirements-*.txt	setf requirements
au BufNewFile,BufRead constraints.txt		setf requirements
au BufNewFile,BufRead requirements.in		setf requirements
au BufNewFile,BufRead requirements/*.txt	setf requirements
au BufNewFile,BufRead requires/*.txt		setf requirements

" Pipenv Pipfiles
au BufNewFile,BufRead Pipfile			setf toml
au BufNewFile,BufRead Pipfile.lock		setf json

" Pixi lock
au BufNewFile,BufRead pixi.lock			setf yaml

" Pkl
au BufNewFile,BufRead *.pkl			setf pkl

" PL/1, PL/I
au BufNewFile,BufRead *.pli,*.pl1		setf pli

" PL/M (also: *.inp)
au BufNewFile,BufRead *.plm,*.p36,*.pac		setf plm

" PL/SQL
au BufNewFile,BufRead *.pls,*.plsql		setf plsql

" PLP
au BufNewFile,BufRead *.plp			setf plp

" PO and PO template (GNU gettext)
au BufNewFile,BufRead *.po,*.pot		setf po

" Pony
au BufNewFile,BufRead *.pony			setf pony

" Postfix main config
au BufNewFile,BufRead main.cf,main.cf.proto	setf pfmain

" PostScript (+ font files, encapsulated PostScript, Adobe Illustrator)
au BufNewFile,BufRead *.ps,*.pfa,*.afm,*.eps,*.epsf,*.epsi,*.ai	  setf postscr

" PostScript Printer Description
au BufNewFile,BufRead *.ppd			setf ppd

" Povray
au BufNewFile,BufRead *.pov			setf pov

" Povray configuration
au BufNewFile,BufRead .povrayrc			setf povini

" Povray, Pascal, PHP or assembly
au BufNewFile,BufRead *.inc			call dist#ft#FTinc()

" PowerShell
au BufNewFile,BufRead	*.ps1,*.psd1,*.psm1,*.pssc	setf ps1
au BufNewFile,BufRead	*.ps1xml			setf ps1xml
au BufNewFile,BufRead	*.cdxml,*.psc1			setf xml

" Power Query M
au BufNewFile,BufRead *.pq			setf pq

" Printcap and Termcap
au BufNewFile,BufRead *printcap
	\ let b:ptcap_type = "print" | setf ptcap
au BufNewFile,BufRead *termcap
	\ let b:ptcap_type = "term" | setf ptcap

" Prisma
au BufRead,BufNewFile *.prisma			setf prisma

" PPWizard
au BufNewFile,BufRead *.it,*.ih			setf ppwiz

" Pug
au BufRead,BufNewFile *.pug			setf pug

" Puppet
au BufNewFile,BufRead Puppetfile		setf ruby

" Embedded Puppet
au BufNewFile,BufRead *.epp			setf epuppet

" Obj 3D file format
" TODO: is there a way to avoid MS-Windows Object files?
au BufNewFile,BufRead *.obj			setf obj

" Oracle Pro*C/C++
au BufNewFile,BufRead *.pc			setf proc

" Privoxy actions file
au BufNewFile,BufRead *.action			setf privoxy

" Procmail
au BufNewFile,BufRead .procmail,.procmailrc	setf procmail

" Progress or CWEB
au BufNewFile,BufRead *.w			call dist#ft#FTprogress_cweb()

" Progress or assembly or Swig
au BufNewFile,BufRead *.i			call dist#ft#FTi()

" Progress or Pascal
au BufNewFile,BufRead *.p			call dist#ft#FTprogress_pascal()

" Software Distributor Product Specification File (POSIX 1387.2-1995)
au BufNewFile,BufRead *.psf			setf psf
au BufNewFile,BufRead INDEX,INFO
	\ if getline(1) =~ '^\s*\(distribution\|installed_software\|root\|bundle\|product\)\s*$' |
	\   setf psf |
	\ endif

" Prolog
au BufNewFile,BufRead *.pdb			setf prolog

" Promela
au BufNewFile,BufRead *.pml			setf promela

" Property Specification Language (PSL)
au BufNewFile,BufRead *.psl			setf psl

" Google protocol buffers
au BufNewFile,BufRead *.proto			setf proto
au BufNewFile,BufRead *.txtpb,*.textproto,*.textpb,*.pbtxt setf pbtxt

" Poke
au BufNewFile,BufRead *.pk			setf poke

" Protocols
au BufNewFile,BufRead */etc/protocols		setf protocols

" Nvidia PTX (Parallel Thread Execution)
" See https://docs.nvidia.com/cuda/parallel-thread-execution/
au BufNewFile,BufRead *.ptx			setf ptx

" Purescript
au BufNewFile,BufRead *.purs			setf purescript

" PyPA manifest files
au BufNewFile,BufRead MANIFEST.in		setf pymanifest

" Pyret
au BufNewFile,BufRead *.arr			setf pyret

" Pyrex/Cython
au BufNewFile,BufRead *.pyx,*.pyx+,*.pxd,*.pxi	setf pyrex

" Python, Python Shell Startup and Python Stub Files
" Quixote (Python-based web framework) and IPython
au BufNewFile,BufRead *.py,*.pyw,.pythonstartup,.pythonrc,.python_history,.jline-jython.history	setf python
au BufNewFile,BufRead *.ipy,*.ptl,*.pyi,SConstruct		   setf python

" QL
au BufRead,BufNewFile *.ql,*.qll		setf ql

" QML
au BufRead,BufNewFile *.qml,*.qbs			setf qml

" QMLdir
au BufRead,BufNewFile qmldir			setf qmldir

" Quarto
au BufRead,BufNewFile *.qmd			setf quarto

" QuickBms
au BufRead,BufNewFile *.bms			setf quickbms

" Racket (formerly detected as "scheme")
au BufNewFile,BufRead *.rkt,*.rktd,*.rktl	setf racket

" Radiance
au BufNewFile,BufRead *.rad,*.mat		setf radiance

" Raku (formerly Perl6)
au BufNewFile,BufRead *.pm6,*.p6,*.t6,*.pod6,*.raku,*.rakumod,*.rakudoc,*.rakutest  setf raku

" Ratpoison config/command files
au BufNewFile,BufRead .ratpoisonrc,ratpoisonrc	setf ratpoison

" RCS file
au BufNewFile,BufRead *\,v			setf rcs

" Readline
au BufNewFile,BufRead .inputrc,inputrc		setf readline

" Registry for MS-Windows
au BufNewFile,BufRead *.reg
	\ if getline(1) =~? '^REGEDIT[0-9]*\s*$\|^Windows Registry Editor Version \d*\.\d*\s*$' | setf registry | endif

" Renderman Interface Bytestream
au BufNewFile,BufRead *.rib			setf rib

" Rego Policy Language
au BufNewFile,BufRead *.rego			setf rego

" Rexx
au BufNewFile,BufRead *.rex,*.orx,*.rxo,*.rxj,*.jrexx,*.rexxj,*.rexx,*.testGroup,*.testUnit	setf rexx

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

" R profile file
au BufNewFile,BufRead .Rhistory,.Rprofile,Rprofile,Rprofile.site	setf r

" RSS looks like XML
au BufNewFile,BufRead *.rss				setf xml

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

" ReScript
au BufNewFile,BufRead *.res,*.resi			setf rescript

" Resolv.conf
au BufNewFile,BufRead resolv.conf		setf resolv

" Relax NG Compact
au BufNewFile,BufRead *.rnc			setf rnc

" Relax NG XML
au BufNewFile,BufRead *.rng			setf rng

" ILE RPG
au BufNewFile,BufRead *.rpgle,*.rpgleinc	setf rpgle

" RPL/2
au BufNewFile,BufRead *.rpl			setf rpl

" Robot Framework
au BufNewFile,BufRead *.robot,*.resource	setf robot

" Robots.txt
au BufNewFile,BufRead robots.txt		setf robots

" Roc
au BufNewFile,BufRead *.roc			setf roc

" RON (Rusty Object Notation)
au BufNewFile,BufRead *.ron			setf ron

" MikroTik RouterOS script
au BufRead,BufNewFile *.rsc			setf routeros

" Rpcgen
au BufNewFile,BufRead *.x			setf rpcgen

" reStructuredText Documentation Format
au BufNewFile,BufRead *.rst			setf rst

" RTF
au BufNewFile,BufRead *.rtf			setf rtf

" Interactive Ruby shell
au BufNewFile,BufRead .irbrc,irbrc,.irb_history,irb_history	setf ruby

" Ruby
au BufNewFile,BufRead *.rb,*.rbw		setf ruby

" RubyGems
au BufNewFile,BufRead *.gemspec			setf ruby

" RBS (Ruby Signature)
au BufNewFile,BufRead *.rbs			setf rbs

" Rackup
au BufNewFile,BufRead *.ru			setf ruby

" Bundler
au BufNewFile,BufRead Gemfile			setf ruby

" Ruby on Rails
au BufNewFile,BufRead *.builder,*.rxml,*.rjs	setf ruby

" Rantfile and Rakefile is like Ruby
au BufNewFile,BufRead [rR]antfile,*.rant,[rR]akefile,*.rake	setf ruby

" Rust
au BufNewFile,BufRead *.rs			setf rust
au BufNewFile,BufRead Cargo.lock,*/.cargo/config,*/.cargo/credentials	setf toml

" S-lang
au BufNewFile,BufRead *.sl			setf slang

" Sage
au BufNewFile,BufRead *.sage			setf sage

" Samba config
au BufNewFile,BufRead smb.conf			setf samba

" SAS script
au BufNewFile,BufRead *.sas			setf sas

" Sass
au BufNewFile,BufRead *.sass			setf sass

" Sather, TI linear assembly
au BufNewFile,BufRead *.sa			call dist#ft#FTsa()

" Scala
au BufNewFile,BufRead *.scala,*.mill		setf scala

" SBT - Scala Build Tool
au BufNewFile,BufRead *.sbt			setf sbt

" Slang Shading Language
au BufNewFile,BufRead *.slang			setf shaderslang

" Slint
au BufNewFile,BufRead *.slint			setf slint

" SuperCollider
au BufNewFile,BufRead *.sc			call dist#ft#FTsc()

au BufNewFile,BufRead *.quark			setf supercollider

" scdoc
au BufNewFile,BufRead *.scd			call dist#ft#FTscd()

" Scilab
au BufNewFile,BufRead *.sci,*.sce		setf scilab


" SCSS
au BufNewFile,BufRead *.scss			setf scss

" SD: Streaming Descriptors
au BufNewFile,BufRead *.sd			setf sd

" SDL
au BufNewFile,BufRead *.sdl,*.pr		setf sdl

" sed
au BufNewFile,BufRead *.sed			setf sed

" SubRip
au BufNewFile,BufRead *.srt			setf srt

" SubStation Alpha
au BufNewFile,BufRead *.ass,*.ssa		setf ssa

" svelte
au BufNewFile,BufRead *.svelte			setf svelte

" Sieve (RFC 3028, 5228)
au BufNewFile,BufRead *.siv,*.sieve		setf sieve

" Sendmail
au BufNewFile,BufRead sendmail.cf		setf sm

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

" SGML catalog file
au BufNewFile,BufRead catalog			setf catalog

" Shell scripts (sh, ksh, bash, bash2, csh); Allow .profile_foo etc.
" Gentoo ebuilds and Arch Linux PKGBUILDs are actually bash scripts.
" NOTE: Patterns ending in a star are further down, these have lower priority.
au BufNewFile,BufRead .bashrc,bashrc,bash.bashrc,.bash[_-]profile,.bash[_-]logout,.bash[_-]aliases,.bash[_-]history,bash-fc[-.],*.ebuild,*.bash,*.eclass,PKGBUILD,*.bats,*.cygport call dist#ft#SetFileTypeSH("bash")
au BufNewFile,BufRead .kshrc,*.ksh call dist#ft#SetFileTypeSH("ksh")
au BufNewFile,BufRead */etc/profile,.profile,*.sh,*.env{rc,} call dist#ft#SetFileTypeSH(getline(1))
" Alpine Linux APKBUILDs are actually POSIX sh scripts with special treatment.
au BufNewFile,BufRead APKBUILD	setf apkbuild

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

" TriG
au BufNewFile,BufRead *.trig			setf trig

" Zig and Zig Object Notation (ZON)
au BufNewFile,BufRead *.zig,*.zon		setf zig

" Ziggy and Ziggy Schema
au BufNewFile,BufRead *.ziggy                   setf ziggy
au BufNewFile,BufRead *.ziggy-schema            setf ziggy_schema

" Zserio
au BufNewFile,BufRead *.zs			setf zserio

" Z-Shell script (patterns ending in a star further below)
au BufNewFile,BufRead .zprofile,*/etc/zprofile,.zfbfmarks  setf zsh
au BufNewFile,BufRead .zshrc,.zshenv,.zlogin,.zlogout,.zcompdump,.zsh_history setf zsh
au BufNewFile,BufRead *.zsh,*.zsh-theme,*.zunit		setf zsh

" Salt state files
au BufNewFile,BufRead *.sls			setf salt

" Scheme, Supertux configuration, Lips.js history, Guile init file ("racket" patterns are now separate, see above)
au BufNewFile,BufRead *.scm,*.ss,*.sld,*.stsg,*/supertux2/config,.lips_repl_history,.guile	setf scheme

" Screen RC
au BufNewFile,BufRead .screenrc,screenrc	setf screen

" Sexplib
au BufNewFile,BufRead *.sexp setf sexplib

" Simula
au BufNewFile,BufRead *.sim			setf simula

" SINDA
au BufNewFile,BufRead *.sin,*.s85		setf sinda

" SiSU
au BufNewFile,BufRead *.sst,*.ssm,*.ssi,*.-sst,*._sst setf sisu
au BufNewFile,BufRead *.sst.meta,*.-sst.meta,*._sst.meta setf sisu

" SKILL
au BufNewFile,BufRead *.il,*.ils,*.cdf		setf skill

" Cadence
au BufNewFile,BufRead *.cdc			setf cdc

" Cangjie
au BufNewFile,BufRead *.cj			setf cangjie

" SLRN
au BufNewFile,BufRead .slrnrc			setf slrnrc
au BufNewFile,BufRead *.score			setf slrnsc

" Smali
au BufNewFile,BufRead *.smali			setf smali

" Smalltalk
au BufNewFile,BufRead *.st			setf st

" Smalltalk (and Rexx, TeX, and Visual Basic)
au BufNewFile,BufRead *.cls			call dist#ft#FTcls()

" Smarty templates
au BufNewFile,BufRead *.tpl			setf smarty

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

" SMITH
au BufNewFile,BufRead *.smt,*.smith		setf smith

" Smithy
au BufNewFile,BufRead *.smithy			setf smithy

" Snakemake
au BufNewFile,BufRead Snakefile,*.smk		setf snakemake

" Snobol4 and spitbol
au BufNewFile,BufRead *.sno,*.spt		setf snobol4

" SNMP MIB files
au BufNewFile,BufRead *.mib,*.my		setf mib

" Snort Configuration
au BufNewFile,BufRead *.hog,snort.conf,vision.conf	setf hog
au BufNewFile,BufRead *.rules			call dist#ft#FTRules()

" Solidity
au BufRead,BufNewFile *.sol			setf solidity

" SPARQL queries
au BufNewFile,BufRead *.rq,*.sparql		setf sparql

" Spec (Linux RPM)
au BufNewFile,BufRead *.spec			setf spec

" Speedup (AspenTech plant simulator)
au BufNewFile,BufRead *.speedup,*.spdata,*.spd	setf spup

" Slice
au BufNewFile,BufRead *.ice			setf slice

" Microsoft Visual Studio Solution
au BufNewFile,BufRead *.sln			setf solution
au BufNewFile,BufRead *.slnf			setf json
au BufNewFile,BufRead *.slnx			setf xml

" Spice
au BufNewFile,BufRead *.sp,*.spice		setf spice

" Spyce
au BufNewFile,BufRead *.spy,*.spi		setf spyce

" Squid
au BufNewFile,BufRead squid.conf		setf squid

" SQL for Oracle Designer
au BufNewFile,BufRead *.tyb,*.tyc,*.pkb,*.pks	setf sql

" *.typ can be either SQL or Typst files
au BufNewFile,BufRead *.typ			call dist#ft#FTtyp()

" SQL
au BufNewFile,BufRead *.sql			call dist#ft#SQL()
au BufNewFile,BufRead .sqlite_history		setf sql

" SQLJ
au BufNewFile,BufRead *.sqlj			setf sqlj

" PRQL
au BufNewFile,BufRead *.prql			setf prql

" SQR
au BufNewFile,BufRead *.sqr,*.sqi		setf sqr

" Squirrel
au BufNewFile,BufRead *.nut			setf squirrel

" OpenSSH configuration
au BufNewFile,BufRead ssh_config,*/.ssh/config,*/.ssh/*.conf	setf sshconfig
au BufNewFile,BufRead */etc/ssh/ssh_config.d/*.conf		setf sshconfig

" OpenSSH server configuration
au BufNewFile,BufRead sshd_config			setf sshdconfig
au BufNewFile,BufRead */etc/ssh/sshd_config.d/*.conf	setf sshdconfig

" Starlark
au BufNewFile,BufRead *.ipd,*.star,*.starlark	setf starlark

" OpenVPN configuration
au BufNewFile,BufRead *.ovpn			setf openvpn
au BufNewFile,BufRead */openvpn/*/*.conf	setf openvpn

" Stata
au BufNewFile,BufRead *.ado,*.do,*.imata,*.mata	setf stata
" Also *.class, but not when it's a Java bytecode file
au BufNewFile,BufRead *.class
	\ if getline(1) !~ "^\xca\xfe\xba\xbe" | setf stata | endif

" SMCL
au BufNewFile,BufRead *.hlp,*.ihlp,*.smcl	setf smcl

" SPA JSON
au BufNewFile,BufRead */pipewire/*.conf		setf spajson
au BufNewFile,BufRead */wireplumber/*.conf	setf spajson

" Stored Procedures
au BufNewFile,BufRead *.stp			setf stp

" Standard ML
au BufNewFile,BufRead *.sml			setf sml

" Sratus VOS command macro
au BufNewFile,BufRead *.cm			setf voscm

" Sway (programming language)
au BufNewFile,BufRead *.sw			setf sway

" Swift
au BufNewFile,BufRead *.swift,*.swiftinterface	setf swift
au BufNewFile,BufRead *.swift.gyb		setf swiftgyb

" Swift Intermediate Language or SILE
au BufNewFile,BufRead *.sil			call dist#ft#FTsil()

" Swig
au BufNewFile,BufRead *.swg,*.swig setf swig

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

" Synopsys Design Constraints
au BufNewFile,BufRead *.sdc			setf sdc

" Sudoers
au BufNewFile,BufRead */etc/sudoers,sudoers.tmp	setf sudoers

" SVG (Scalable Vector Graphics)
au BufNewFile,BufRead *.svg			setf svg

" Surface
au BufRead,BufNewFile *.sface			setf surface

" LLVM TableGen
au BufNewFile,BufRead *.td			setf tablegen

" Tads (or Nroff or Perl test file)
au BufNewFile,BufRead *.t
	\ if !dist#ft#FTnroff() && !dist#ft#FTperl() | setf tads | endif

" Tags
au BufNewFile,BufRead tags			setf tags

" TAK
au BufNewFile,BufRead *.tak			setf tak

" Unx Tal
au BufNewFile,BufRead *.tal			setf tal

" Task
au BufRead,BufNewFile {pending,completed,undo}.data  setf taskdata
au BufRead,BufNewFile *.task			setf taskedit

" Tcl (JACL too)
au BufNewFile,BufRead *.tcl,*.tm,*.tk,*.itcl,*.itk,*.jacl,.tclshrc,.wishrc,.tclsh-history	setf tcl

" Xilinx's xsct and xsdb use tcl
au BufNewFile,BufRead .xsctcmdhistory,.xsdbcmdhistory	setf tcl

" templ
au BufNewFile,BufRead *.templ			setf templ

" Teal
au BufRead,BufNewFile *.tl			setf teal

" TealInfo
au BufNewFile,BufRead *.tli			setf tli

" Telix Salt
au BufNewFile,BufRead *.slt			setf tsalt

" Tera Term Language or Turtle
au BufRead,BufNewFile *.ttl
	\ if getline(1) =~ '^@\?\(prefix\|base\)' |
	\   setf turtle |
	\ else |
	\   setf teraterm |
	\ endif

" Terminfo
au BufNewFile,BufRead *.ti			setf terminfo

" Tera
au BufRead,BufNewFile *.tera			setf tera

" Terraform variables
au BufRead,BufNewFile *.tfvars			setf terraform-vars

" TeX
au BufNewFile,BufRead *.latex,*.sty,*.dtx,*.ltx,*.bbl	setf tex
au BufNewFile,BufRead *.tex			call dist#ft#FTtex()
au BufNewFile,BufRead texdoc.cnf		setf conf

" LaTeX packages will generate some medium LaTeX files during compiling
" They should be ignored by .gitignore https://github.com/github/gitignore/blob/main/TeX.gitignore
" Sometime we need to view its content for debugging
au BufNewFile,BufRead *.{pgf,nlo,nls,thm,eps_tex,pygtex,pygstyle,clo,aux,brf,ind,lof,loe,nav,vrb,ins,tikz,bbx,cbx,beamer}	setf tex

" LaTeX files generated by Inkscape
au BufNewFile,BufRead *.pdf_tex			setf tex

" ConTeXt
au BufNewFile,BufRead *.mkii,*.mkiv,*.mkvi,*.mkxl,*.mklx   setf context

" Texinfo
au BufNewFile,BufRead *.texinfo,*.texi,*.txi	setf texinfo

" TeX configuration
au BufNewFile,BufRead texmf.cnf			setf texmf

" Thrift (Apache)
au BufNewFile,BufRead *.thrift			setf thrift

" Tidy config
au BufNewFile,BufRead .tidyrc,tidyrc,tidy.conf	setf tidy

" TF (TinyFugue) mud client
au BufNewFile,BufRead .tfrc,tfrc		setf tf

" TF (TinyFugue) mud client or terraform
au BufNewFile,BufRead *.tf			call dist#ft#FTtf()

" TLA+
au BufNewFile,BufRead *.tla			setf tla

" tmux configuration
au BufNewFile,BufRead {.,}tmux*.conf		setf tmux

" TOML
au BufNewFile,BufRead *.toml,uv.lock		setf toml

" TPP - Text Presentation Program
au BufNewFile,BufRead *.tpp			setf tpp

" TRACE32 Script Language
au BufNewFile,BufRead *.cmm,*.cmmt,*.t32	setf trace32

" Treetop
au BufRead,BufNewFile *.treetop			setf treetop

" Trustees
au BufNewFile,BufRead trustees.conf		setf trustees

" TSS - Geometry
au BufNewFile,BufReadPost *.tssgm		setf tssgm

" TSS - Optics
au BufNewFile,BufReadPost *.tssop		setf tssop

" TSS - Command Line (temporary)
au BufNewFile,BufReadPost *.tsscl		setf tsscl

" TSV Files
au BufNewFile,BufRead *.tsv			setf tsv

" Tutor mode
au BufNewFile,BufReadPost *.tutor		setf tutor

" TWIG files
au BufNewFile,BufReadPost *.twig		setf twig

" TypeScript or Qt translation file (which is XML)
au BufNewFile,BufReadPost *.ts
	\ if getline(1) =~ '<?xml' |
	\   setf xml |
	\ else |
	\   setf typescript |
	\ endif
au BufNewFile,BufRead .ts_node_repl_history	setf typescript

" TypeScript module and common
au BufNewFile,BufRead *.mts,*.cts		setf typescript

" TypeScript with React
au BufNewFile,BufRead *.tsx			setf typescriptreact

" TypeSpec files
au BufNewFile,BufRead *.tsp			setf typespec

" Motif UIT/UIL files
au BufNewFile,BufRead *.uit,*.uil		setf uil

" Udev conf
au BufNewFile,BufRead */etc/udev/udev.conf	setf udevconf

" Udev permissions
au BufNewFile,BufRead */etc/udev/permissions.d/*.permissions setf udevperm
"
" Udev symlinks config
au BufNewFile,BufRead */etc/udev/cdsymlinks.conf	setf sh

" Ungrammar, AKA Un-grammar
au BufNewFile,BufRead *.ungram			setf ungrammar

" UnrealScript
au BufNewFile,BufRead *.uc			setf uc

" Updatedb
au BufNewFile,BufRead */etc/updatedb.conf	setf updatedb

" Upstart (init(8)) config files
au BufNewFile,BufRead */usr/share/upstart/*.conf	       setf upstart
au BufNewFile,BufRead */usr/share/upstart/*.override	       setf upstart
au BufNewFile,BufRead */etc/init/*.conf,*/etc/init/*.override  setf upstart
au BufNewFile,BufRead */.init/*.conf,*/.init/*.override	       setf upstart
au BufNewFile,BufRead */.config/upstart/*.conf		       setf upstart
au BufNewFile,BufRead */.config/upstart/*.override	       setf upstart

" URL shortcut
au BufNewFile,BufRead *.url			setf urlshortcut

" V
au BufNewFile,BufRead *.vsh,*.vv			setf v

" Vala
au BufNewFile,BufRead *.vala			setf vala

" VDF
au BufNewFile,BufRead *.vdf			setf vdf

" VDM
au BufRead,BufNewFile *.vdmpp,*.vpp		setf vdmpp
au BufRead,BufNewFile *.vdmrt			setf vdmrt
au BufRead,BufNewFile *.vdmsl,*.vdm		setf vdmsl

" Vento
au BufNewFile,BufRead *.vto			setf vento

" Vera
au BufNewFile,BufRead *.vr,*.vri,*.vrh		setf vera

" Vagrant (uses Ruby syntax)
au BufNewFile,BufRead Vagrantfile		setf ruby

" Verilog HDL, V or Coq
au BufNewFile,BufRead *.v			call dist#ft#FTv()

" Verilog-AMS HDL
au BufNewFile,BufRead *.va,*.vams		setf verilogams

" SystemVerilog
au BufNewFile,BufRead *.sv,*.svh		setf systemverilog

" VHS tape
" .tape is also used by TapeCalc, which we do not support ATM.  If TapeCalc
" support is needed the contents of the file needs to be inspected.
au BufNewFile,BufRead *.tape			setf vhs

" VHDL
au BufNewFile,BufRead *.hdl,*.vhd,*.vhdl,*.vbe,*.vst,*.vho  setf vhdl

" Vim script
au BufNewFile,BufRead *.vim,.exrc,_exrc,.netrwhist	setf vim

" Viminfo file
au BufNewFile,BufRead .viminfo,_viminfo		setf viminfo

" Virata Config Script File or Drupal module
au BufRead,BufNewFile *.hw,*.module,*.pkg
	\ if getline(1) =~ '<?php' |
	\   setf php |
	\ else |
	\   setf virata |
	\ endif

" Visual Basic (see also *.bas *.cls)

" Visual Basic or FORM
au BufNewFile,BufRead *.frm			call dist#ft#FTfrm()

" Visual Basic
" user control, ActiveX document form, active designer, property page
au BufNewFile,BufRead *.ctl,*.dob,*.dsr,*.pag	setf vb

" Visual Basic or Vimball Archiver
au BufNewFile,BufRead *.vba			call dist#ft#FTvba()

" Visual Basic Project
au BufNewFile,BufRead *.vbp			setf dosini

" VBScript (close to Visual Basic)
au BufNewFile,BufRead *.vbs			setf vb

" Visual Basic .NET (close to Visual Basic)
au BufNewFile,BufRead *.vb			setf vb

" Visual Studio Macro
au BufNewFile,BufRead *.dsm			setf vb

" SaxBasic (close to Visual Basic)
au BufNewFile,BufRead *.sba			setf vb

" Vgrindefs file
au BufNewFile,BufRead vgrindefs			setf vgrindefs

" VRML V1.0c
au BufNewFile,BufRead *.wrl			setf vrml

" Vroom (vim testing and executable documentation)
au BufNewFile,BufRead *.vroom			setf vroom

" Vue.js Single File Component
au BufNewFile,BufRead *.vue			setf vue

" Waybar config
au BufNewFile,BufRead */waybar/config		setf jsonc

" WebAssembly
au BufNewFile,BufRead *.wat,*.wast		setf wat

" WebAssembly Interface Type (WIT)
au BufNewFile,BufRead *.wit			setf wit

" Webmacro
au BufNewFile,BufRead *.wm			setf webmacro

" Wget config
au BufNewFile,BufRead .wgetrc,wgetrc		setf wget

" Wget2 config
au BufNewFile,BufRead .wget2rc,wget2rc		setf wget2

" WebGPU Shading Language (WGSL)
au BufNewFile,BufRead *.wgsl			setf wgsl

" Website MetaLanguage
au BufNewFile,BufRead *.wml			setf wml

" Winbatch
au BufNewFile,BufRead *.wbt			setf winbatch

" WSML
au BufNewFile,BufRead *.wsml			setf wsml

" WPL
au BufNewFile,BufRead *.wpl			setf xml

" WvDial
au BufNewFile,BufRead wvdial.conf,.wvdialrc	setf wvdial

" CVS RC file
au BufNewFile,BufRead .cvsrc			setf cvsrc

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

" XHTML
au BufNewFile,BufRead *.xhtml,*.xht		setf xhtml

" X11vnc
au BufNewFile,BufRead .x11vncrc			setf conf

" Xprofile
au BufNewFile,BufRead .xprofile			setf sh

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

" Xilinx Vivado/Vitis project files and block design files
au BufNewFile,BufRead *.xpr,*.xpfm,*.spfm,*.bxml,*.mmi		setf xml
au BufNewFile,BufRead *.bd,*.bda,*.xci				setf json
au BufNewFile,BufRead *.mss					setf mss

" XS Perl extension interface language
au BufNewFile,BufRead *.xs			setf xs

" X compose file
au BufNewFile,BufRead .XCompose,Compose	setf xcompose

" X resources file
au BufNewFile,BufRead .Xdefaults,.Xpdefaults,.Xresources,xdm-config,*.ad setf xdefaults

" Xmath
au BufNewFile,BufRead *.msc,*.msf		setf xmath
au BufNewFile,BufRead *.ms
	\ if !dist#ft#FTnroff() | setf xmath | endif

" XML  specific variants: docbk and xbl
au BufNewFile,BufRead *.xml			call dist#ft#FTxml()

" XMI (holding UML models) is also XML
au BufNewFile,BufRead *.xmi			setf xml

" CSPROJ files are Visual Studio.NET's XML-based C# project config files
au BufNewFile,BufRead *.csproj,*.csproj.user	setf xml

" FSPROJ files are Visual Studio.NET's XML-based F# project config files
au BufNewFile,BufRead *.fsproj,*.fsproj.user	setf xml

" VBPROJ files are Visual Studio.NET's XML-based Visual Basic project config files
au BufNewFile,BufRead *.vbproj,*.vbproj.user	setf xml

" MSBUILD configuration files are also XML
au BufNewFile,BufRead Directory.Packages.props,Directory.Build.targets,Directory.Build.props	setf xml

" Unison Language
au BufNewFile,BufRead *.u,*.uu				setf unison

" Qt Linguist translation source and Qt User Interface Files are XML
" However, for .ts TypeScript is more common.
au BufNewFile,BufRead *.ui			setf xml

" TPM's are RDF-based descriptions of TeX packages (Nikolai Weibull)
au BufNewFile,BufRead *.tpm			setf xml

" Xdg menus
au BufNewFile,BufRead */etc/xdg/menus/*.menu	setf xml

" ATI graphics driver configuration
au BufNewFile,BufRead fglrxrc			setf xml

" Web Services Description Language (WSDL)
au BufNewFile,BufRead *.wsdl			setf xml

" Workflow Description Language (WDL)
au BufNewFile,BufRead *.wdl			setf wdl

" XLIFF (XML Localisation Interchange File Format) is also XML
au BufNewFile,BufRead *.xlf			setf xml
au BufNewFile,BufRead *.xliff			setf xml

" XML User Interface Language
au BufNewFile,BufRead *.xul			setf xml

" X11 xmodmap (also see below)
au BufNewFile,BufRead *Xmodmap			setf xmodmap

" Xquery
au BufNewFile,BufRead *.xq,*.xql,*.xqm,*.xquery,*.xqy	setf xquery

" XSD
au BufNewFile,BufRead *.xsd			setf xsd

" Xslt
au BufNewFile,BufRead *.xsl,*.xslt		setf xslt

" Yacc
au BufNewFile,BufRead *.yy,*.yxx,*.y++		setf yacc

" Yacc or racc
au BufNewFile,BufRead *.y			call dist#ft#FTy()

" Yaml
au BufNewFile,BufRead *.yaml,*.yml,*.eyaml		setf yaml
au BufNewFile,BufRead */.kube/config	setf yaml

" Raml
au BufNewFile,BufRead *.raml			setf raml

" yum conf (close enough to dosini)
au BufNewFile,BufRead */etc/yum.conf		setf dosini

" YANG
au BufRead,BufNewFile *.yang			setf yang

" Yuck
au BufNewFile,BufRead *.yuck			setf yuck

" Zimbu
au BufNewFile,BufRead *.zu			setf zimbu
" Zimbu Templates
au BufNewFile,BufRead *.zut			setf zimbutempl

" Zope
"   dtml (zope dynamic template markup language), pt (zope page template),
"   cpt (zope form controller page template)
au BufNewFile,BufRead *.dtml,*.pt,*.cpt		call dist#ft#FThtml()
"   zsql (zope sql method)
au BufNewFile,BufRead *.zsql			call dist#ft#SQL()

" Z80 assembler asz80
au BufNewFile,BufRead *.z8a			setf z8a

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
	\ if expand("<afile>:e") == "m4"
	\|  call s:StarSetf('fvwm2m4')
	\|else
	\|  let b:fvwm_version = 2 | call s:StarSetf('fvwm')
	\|endif

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

" Nfs
au BufNewFile,BufRead nfs.conf,nfsmount.conf		setf dosini

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
au BufRead,BufNewFile *.rdf			call dist#ft#Redif()

" Remind
au BufNewFile,BufRead .reminders*		call s:StarSetf('remind')

" SGML catalog file
au BufNewFile,BufRead sgml.catalog*		call s:StarSetf('catalog')

" Stylus
au BufNewFile,BufReadPost *.styl,*.stylus	setf stylus

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

" Universal Scene Description
au BufNewFile,BufRead *.usda,*.usd		setf usd

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

" Yarn lock
au BufNewFile,BufRead yarn.lock			setf yaml

" Zathurarc
au BufNewFile,BufRead zathurarc			setf zathurarc

" Rofi stylesheet
au BufNewFile,BufRead *.rasi			setf rasi

" Z-Shell script ending in a star
au BufNewFile,BufRead .zsh*,.zlog*,.zcompdump*  call s:StarSetf('zsh')
au BufNewFile,BufRead zsh*,zlog*		call s:StarSetf('zsh')

" Zsh module
" mdd: https://github.com/zsh-users/zsh/blob/57248b88830ce56adc243a40c7773fb3825cab34/Etc/zsh-development-guide#L285-L288
" mdh, pro: https://github.com/zsh-users/zsh/blob/57248b88830ce56adc243a40c7773fb3825cab34/Etc/zsh-development-guide#L268-L271
" *.mdd will generate *.mdh, *.pro and *.epro.
" module's *.c will #include *.mdh containing module dependency information and
" *.pro containing all static declarations of *.c
" *.epro contains all external declarations of *.c
au BufNewFile,BufRead *.mdh,*.epro		setf c
au BufNewFile,BufRead *.mdd			setf sh

" Help files match *.txt but should have a last line that is a modeline.
au BufNewFile,BufRead *.txt
	\  if getline('$') !~ 'vim:.*ft=help'
	\|   setf text
	\| endif

" Blueprint markup files
au BufNewFile,BufRead *.blp			setf blueprint

" Blueprint build system file
au BufNewFile,BufRead *.bp			setf bp

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
