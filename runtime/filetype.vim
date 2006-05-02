" Vim support file to detect file types
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2006 May 02

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
au BufNewFile,BufRead ?\+.orig,?\+.bak,?\+.old,?\+.new,?\+.rpmsave,?\+.rpmnew
	\ exe "doau filetypedetect BufRead " . expand("<afile>:r")
au BufNewFile,BufRead *~
	\ let s:name = expand("<afile>") |
	\ let s:short = substitute(s:name, '\~$', '', '') |
	\ if s:name != s:short && s:short != "" |
	\   exe "doau filetypedetect BufRead " . s:short |
	\ endif |
	\ unlet s:name |
	\ unlet s:short
au BufNewFile,BufRead ?\+.in
	\ if expand("<afile>:t") != "configure.in" |
	\   exe "doau filetypedetect BufRead " . expand("<afile>:r") |
	\ endif

" Pattern used to match file names which should not be inspected.
" Currently finds compressed files.
if !exists("g:ft_ignore_pat")
  let g:ft_ignore_pat = '\.\(Z\|gz\|bz2\|zip\|tgz\)$'
endif

" Function used for patterns that end in a star: don't set the filetype if the
" file name matches ft_ignore_pat.
fun! s:StarSetf(ft)
  if expand("<amatch>") !~ g:ft_ignore_pat
    exe 'setf ' . a:ft
  endif
endfun

" Abaqus or Trasys
au BufNewFile,BufRead *.inp			call s:Check_inp()

fun! s:Check_inp()
  if getline(1) =~ '^\*'
    setf abaqus
  else
    let n = 1
    if line("$") > 500
      let nmax = 500
    else
      let nmax = line("$")
    endif
    while n <= nmax
      if getline(n) =~? "^header surface data"
	setf trasys
	break
      endif
      let n = n + 1
    endwhile
  endif
endfun

" A-A-P recipe
au BufNewFile,BufRead *.aap			setf aap

" A2ps printing utility
au BufNewFile,BufRead etc/a2ps.cfg,etc/a2ps/*.cfg,a2psrc,.a2psrc setf a2ps

" ABAB/4
au BufNewFile,BufRead *.abap			setf abap

" ABC music notation
au BufNewFile,BufRead *.abc			setf abc

" ABEL
au BufNewFile,BufRead *.abl			setf abel

" AceDB
au BufNewFile,BufRead *.wrm			setf acedb

" Ada (83, 9X, 95)
au BufNewFile,BufRead *.adb,*.ads,*.ada		setf ada

" AHDL
au BufNewFile,BufRead *.tdf			setf ahdl

" AMPL
au BufNewFile,BufRead *.run			setf ampl

" Ant
au BufNewFile,BufRead build.xml			setf ant

" Apache style config file
au BufNewFile,BufRead proftpd.conf*		call s:StarSetf('apachestyle')

" Apache config file
au BufNewFile,BufRead .htaccess			 setf apache
au BufNewFile,BufRead httpd.conf*,srm.conf*,access.conf*,apache.conf*,apache2.conf*,/etc/apache2/*.conf* call s:StarSetf('apache')

" XA65 MOS6510 cross assembler
au BufNewFile,BufRead *.a65			setf a65

" Applix ELF
au BufNewFile,BufRead *.am
	\ if expand("<afile>") !~? 'Makefile.am\>' | setf elf | endif

" ALSA configuration
au BufNewFile,BufRead ~/.asoundrc,/usr/share/alsa/alsa.conf,/etc/asound.conf	setf alsaconf

" Arc Macro Language
au BufNewFile,BufRead *.aml			setf aml

" Arch Inventory file
au BufNewFile,BufRead .arch-inventory,=tagging-method	setf arch

" ART*Enterprise (formerly ART-IM)
au BufNewFile,BufRead *.art			setf art

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

" Grub (must be before catch *.lst)
au BufNewFile,BufRead /boot/grub/menu.lst,/boot/grub/grub.conf,/etc/grub.conf	setf grub

" Assembly (all kinds)
" *.lst is not pure assembly, it has two extra columns (address, byte codes)
au BufNewFile,BufRead *.asm,*.[sS],*.[aA],*.mac,*.lst	call s:FTasm()

" This function checks for the kind of assembly that is wanted by the user, or
" can be detected from the first five lines of the file.
fun! s:FTasm()
  " make sure b:asmsyntax exists
  if !exists("b:asmsyntax")
    let b:asmsyntax = ""
  endif

  if b:asmsyntax == ""
    call s:FTasmsyntax()
  endif

  " if b:asmsyntax still isn't set, default to asmsyntax or GNU
  if b:asmsyntax == ""
    if exists("g:asmsyntax")
      let b:asmsyntax = g:asmsyntax
    else
      let b:asmsyntax = "asm"
    endif
  endif

  exe "setf " . b:asmsyntax
endfun

fun! s:FTasmsyntax()
  " see if file contains any asmsyntax=foo overrides. If so, change
  " b:asmsyntax appropriately
  let head = " ".getline(1)." ".getline(2)." ".getline(3)." ".getline(4).
	\" ".getline(5)." "
  if head =~ '\sasmsyntax=\S\+\s'
    let b:asmsyntax = substitute(head, '.*\sasmsyntax=\(\S\+\)\s.*','\1', "")
  elseif ((head =~? '\.title') || (head =~? '\.ident') || (head =~? '\.macro') || (head =~? '\.subtitle') || (head =~? '\.library'))
    let b:asmsyntax = "vmasm"
  endif
endfun

" Macro (VAX)
au BufNewFile,BufRead *.mar			setf vmasm

" Atlas
au BufNewFile,BufRead *.atl,*.as		setf atlas

" Automake
au BufNewFile,BufRead [mM]akefile.am,GNUmakefile.am	setf automake

" Autotest .at files are actually m4
au BufNewFile,BufRead *.at			setf m4

" Avenue
au BufNewFile,BufRead *.ave			setf ave

" Awk
au BufNewFile,BufRead *.awk			setf awk

" B
au BufNewFile,BufRead *.mch,*.ref,*.imp		setf b

" BASIC or Visual Basic
au BufNewFile,BufRead *.bas			call s:FTVB("basic")

" Check if one of the first five lines contains "VB_Name".  In that case it is
" probably a Visual Basic file.  Otherwise it's assumed to be "alt" filetype.
fun! s:FTVB(alt)
  if getline(1).getline(2).getline(3).getline(4).getline(5) =~? 'VB_Name\|Begin VB\.\(Form\|MDIForm\|UserControl\)'
    setf vb
  else
    exe "setf " . a:alt
  endif
endfun

" Visual Basic Script (close to Visual Basic)
au BufNewFile,BufRead *.vbs,*.dsm,*.ctl		setf vb

" Batch file for MSDOS.
au BufNewFile,BufRead *.bat,*.sys		setf dosbatch
" *.cmd is close to a Batch file, but on OS/2 Rexx files also use *.cmd.
au BufNewFile,BufRead *.cmd
	\ if getline(1) =~ '^/\*' | setf rexx | else | setf dosbatch | endif

" Batch file for 4DOS
au BufNewFile,BufRead *.btm			call s:FTbtm()
fun! s:FTbtm()
  if exists("g:dosbatch_syntax_for_btm") && g:dosbatch_syntax_for_btm
    setf dosbatch
  else
    setf btm
  endif
endfun

" BC calculator
au BufNewFile,BufRead *.bc			setf bc

" BDF font
au BufNewFile,BufRead *.bdf			setf bdf

" BibTeX bibliography database file
au BufNewFile,BufRead *.bib			setf bib

" BibTeX Bibliography Style
au BufNewFile,BufRead *.bst			setf bst

" BIND configuration
au BufNewFile,BufRead named.conf,rndc.conf	setf named

" BIND zone
au BufNewFile,BufRead named.root		setf bindzone
au BufNewFile,BufRead *.db			call s:BindzoneCheck('')

func! s:BindzoneCheck(default)
  if getline(1).getline(2).getline(3).getline(4) =~ '^; <<>> DiG [0-9.]\+ <<>>\|BIND.*named\|$ORIGIN\|$TTL\|IN\s\+SOA'
    setf bindzone
  elseif a:default != ''
    exe 'setf ' . a:default
  endif
endfunc

" Blank
au BufNewFile,BufRead *.bl			setf blank

" C or lpc
au BufNewFile,BufRead *.c			call s:FTlpc()

fun! s:FTlpc()
  if exists("g:lpc_syntax_for_c")
    let lnum = 1
    while lnum <= 12
      if getline(lnum) =~# '^\(//\|inherit\|private\|protected\|nosave\|string\|object\|mapping\|mixed\)'
	setf lpc
	return
      endif
      let lnum = lnum + 1
    endwhile
  endif
  setf c
endfun

" Calendar
au BufNewFile,BufRead calendar			setf calendar
au BufNewFile,BufRead */.calendar/*,
	\*/share/calendar/*/calendar.*,*/share/calendar/calendar.*
	\					call s:StarSetf('calendar')

" C#
au BufNewFile,BufRead *.cs			setf cs

" Cfengine
au BufNewFile,BufRead cfengine.conf		setf cfengine

" Comshare Dimension Definition Language
au BufNewFile,BufRead *.cdl			setf cdl

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

" C++
if has("fname_case")
  au BufNewFile,BufRead *.cxx,*.c++,*.C,*.H,*.hh,*.hxx,*.hpp,*.moc,*.tcc,*.inl setf cpp
else
  au BufNewFile,BufRead *.cxx,*.c++,*.hh,*.hxx,*.hpp,*.moc,*.tcc,*.inl setf cpp
endif

" .h files can be C, Ch or C++, set c_syntax_for_h if you want C,
" ch_syntax_for_h if you want Ch.
au BufNewFile,BufRead *.h
	\ if exists("c_syntax_for_h") | setf c |
	\ elseif exists("ch_syntax_for_h") | setf ch |
	\ else | setf cpp | endif

" Ch (CHscript)
au BufNewFile,BufRead *.chf			setf ch

" TLH files are C++ headers generated by Visual C++'s #import from typelibs
au BufNewFile,BufRead *.tlh			setf cpp

" Cascading Style Sheets
au BufNewFile,BufRead *.css			setf css

" Century Term Command Scripts (*.cmd too)
au BufNewFile,BufRead *.con			setf cterm

" Changelog
au BufNewFile,BufRead changelog.Debian,changelog.dch,NEWS.Debian,NEWS.dch
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
au BufNewFile,BufRead *.ch			call s:FTchange()

" This function checks if one of the first ten lines start with a '@'.  In
" that case it is probably a change file.
" If the first line starts with # or ! it's probably a ch file.
" If a line has "main", "include", "//" ir "/*" it's probably ch.
" Otherwise CHILL is assumed.
fun! s:FTchange()
  let lnum = 1
  while lnum <= 10
    if getline(lnum)[0] == '@'
      setf change
      return
    endif
    if lnum == 1 && (getline(1)[0] == '#' || getline(1)[0] == '!')
      setf ch
      return
    endif
    if getline(lnum) =~ "MODULE"
      setf chill
      return
    endif
    if getline(lnum) =~ 'main\s*(\|#\s*include\|//'
      setf ch
      return
    endif
    let lnum = lnum + 1
  endwhile
  setf chill
endfun

" ChordPro
au BufNewFile,BufRead *.chopro,*.crd,*.cho,*.crdpro,*.chordpro	setf chordpro

" Clean
au BufNewFile,BufRead *.dcl,*.icl		setf clean

" Clever
au BufNewFile,BufRead *.eni			setf cl

" Clever or dtd
au BufNewFile,BufRead *.ent			call s:FTent()

fun! s:FTent()
  " This function checks for valid cl syntax in the first five lines.
  " Look for either an opening comment, '#', or a block start, '{".
  " If not found, assume SGML.
  let lnum = 1
  while lnum < 6
    let line = getline(lnum)
    if line =~ '^\s*[#{]'
      setf cl
      return
    elseif line !~ '^\s*$'
      " Not a blank line, not a comment, and not a block start,
      " so doesn't look like valid cl code.
      break
    endif
    let lnum = lnum + 1
  endw
  setf dtd
endfun

" Clipper (or FoxPro; could also be eviews)
au BufNewFile,BufRead *.prg
	\ if exists("g:filetype_prg") |
	\   exe "setf " . g:filetype_prg |
	\ else |
	\   setf clipper |
	\ endif

" Cmake
au BufNewFile,BufRead CMakeLists.txt,*.cmake,*.cmake.in		setf cmake

" Cobol
au BufNewFile,BufRead *.cbl,*.cob,*.lib	setf cobol
"   cobol or zope form controller python script? (heuristic)
au BufNewFile,BufRead *.cpy
	\ if getline(1) =~ '^##' |
	\   setf python |
	\ else |
	\   setf cobol |
	\ endif

" Cold Fusion
au BufNewFile,BufRead *.cfm,*.cfi,*.cfc		setf cf

" Configure scripts
au BufNewFile,BufRead configure.in,configure.ac setf config

" WildPackets EtherPeek Decoder
au BufNewFile,BufRead *.dcd			setf dcd

" Enlightenment configuration files
au BufNewFile,BufRead *enlightenment/*.cfg	setf c

" Eterm
au BufNewFile,BufRead *Eterm/*.cfg		setf eterm

" Lynx config files
au BufNewFile,BufRead lynx.cfg			setf lynx

" Quake
au BufNewFile,BufRead *baseq[2-3]/*.cfg,*id1/*.cfg	setf quake
au BufNewFile,BufRead *quake[1-3]/*.cfg			setf quake

" Quake C
au BufNewFile,BufRead *.qc			setf c

" Configure files
au BufNewFile,BufRead *.cfg			setf cfg

" Communicating Sequential Processes
au BufNewFile,BufRead *.csp,*.fdr		setf csp

" CUPL logic description and simulation
au BufNewFile,BufRead *.pld			setf cupl
au BufNewFile,BufRead *.si			setf cuplsim

" Debian Control
au BufNewFile,BufRead */debian/control		setf debcontrol

" Debian Sources.list
au BufNewFile,BufRead /etc/apt/sources.list	setf debsources

" ROCKLinux package description
au BufNewFile,BufRead *.desc			setf desc

" the D language
au BufNewFile,BufRead *.d			setf d

" Desktop files
au BufNewFile,BufRead *.desktop,.directory	setf desktop

" Dict config
au BufNewFile,BufRead dict.conf,.dictrc		setf dictconf

" Dictd config
au BufNewFile,BufRead dictd.conf		setf dictdconf

" Diff files
au BufNewFile,BufRead *.diff,*.rej,*.patch	setf diff

" Dircolors
au BufNewFile,BufRead .dir_colors,/etc/DIR_COLORS	setf dircolors

" Diva (with Skill) or InstallShield
au BufNewFile,BufRead *.rul
	\ if getline(1).getline(2).getline(3).getline(4).getline(5).getline(6) =~? 'InstallShield' |
	\   setf ishd |
	\ else |
	\   setf diva |
	\ endif

" DCL (Digital Command Language - vms) or DNS zone file
au BufNewFile,BufRead *.com			call s:BindzoneCheck('dcl')

" DOT
au BufNewFile,BufRead *.dot			setf dot

" Dylan - lid files
au BufNewFile,BufRead *.lid			setf dylanlid

" Dylan - intr files (melange)
au BufNewFile,BufRead *.intr			setf dylanintr

" Dylan
au BufNewFile,BufRead *.dylan			setf dylan

" Microsoft Module Definition
au BufNewFile,BufRead *.def			setf def

" Dracula
au BufNewFile,BufRead *.drac,*.drc,*lvs,*lpe	setf dracula

" dsl
au BufNewFile,BufRead *.dsl			setf dsl

" DTD (Document Type Definition for XML)
au BufNewFile,BufRead *.dtd			setf dtd

" EDIF (*.edf,*.edif,*.edn,*.edo)
au BufNewFile,BufRead *.ed\(f\|if\|n\|o\)	setf edif

" Embedix Component Description
au BufNewFile,BufRead *.ecd			setf ecd

" Eiffel or Specman
au BufNewFile,BufRead *.e,*.E			call s:FTe()

" Elinks configuration
au BufNewFile,BufRead */etc/elinks.conf,*/.elinks/elinks.conf	setf elinks

fun! s:FTe()
  let n = 1
  while n < 100 && n < line("$")
    if getline(n) =~ "^\\s*\\(<'\\|'>\\)\\s*$"
      setf specman
      return
    endif
    let n = n + 1
  endwhile
  setf eiffel
endfun

" ERicsson LANGuage
au BufNewFile,BufRead *.erl			setf erlang

" Elm Filter Rules file
au BufNewFile,BufRead filter-rules		setf elmfilt

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

" Factor
au BufNewFile,BufRead *.factor			setf factor

" Fetchmail RC file
au BufNewFile,BufRead .fetchmailrc		setf fetchmail

" FlexWiki
au BufNewFile,BufRead *.wiki			setf flexwiki

" Focus Executable
au BufNewFile,BufRead *.fex,*.focexec		setf focexec

" Focus Master file (but not for auto.master)
au BufNewFile,BufRead auto.master		setf conf
au BufNewFile,BufRead *.mas,*.master		setf master

" Forth
au BufNewFile,BufRead *.fs,*.ft			setf forth

" Fortran
au BufNewFile,BufRead *.f,*.F,*.for,*.fpp,*.FPP,*.ftn,*.f77,*.F77,*.f90,*.F90,*.f95,*.F95	setf fortran

" FStab
au BufNewFile,BufRead fstab,mtab		setf fstab

" GDB command files
au BufNewFile,BufRead .gdbinit			setf gdb

" GDMO
au BufNewFile,BufRead *.mo,*.gdmo		setf gdmo

" Gedcom
au BufNewFile,BufRead *.ged			setf gedcom

" Gkrellmrc
au BufNewFile,BufRead gkrellmrc,gkrellmrc_?	setf gkrellmrc

" GP scripts (2.0 and onward)
au BufNewFile,BufRead *.gp,.gprc		setf gp

" GPG
au BufNewFile,BufRead */.gnupg/options		setf gpg
au BufNewFile,BufRead */.gnupg/gpg.conf		setf gpg
au BufNewFile,BufRead /usr/**/gnupg/options.skel setf gpg

" Gnuplot scripts
au BufNewFile,BufRead *.gpi			setf gnuplot

" GrADS scripts
au BufNewFile,BufRead *.gs			setf grads

" Gretl
au BufNewFile,BufRead *.gretl			setf gretl

" Groovy
au BufNewFile,BufRead *.groovy			setf groovy

" GNU Server Pages
au BufNewFile,BufRead *.gsp			setf gsp

" Group file
au BufNewFile,BufRead /etc/group		setf group

" GTK RC
au BufNewFile,BufRead .gtkrc,gtkrc		setf gtkrc

" Haskell
au BufNewFile,BufRead *.hs			setf haskell
au BufNewFile,BufRead *.lhs			setf lhaskell
au BufNewFile,BufRead *.chs			setf chaskell

" Hercules
au BufNewFile,BufRead *.vc,*.ev,*.rs,*.sum,*.errsum	setf hercules

" HEX (Intel)
au BufNewFile,BufRead *.hex,*.h32		setf hex

" Tilde (must be before HTML)
au BufNewFile,BufRead *.t.html			setf tilde

" HTML (.shtml and .stm for server side)
au BufNewFile,BufRead *.html,*.htm,*.shtml,*.stm  call s:FThtml()

" Distinguish between HTML, XHTML and Django
fun! s:FThtml()
  let n = 1
  while n < 10 && n < line("$")
    if getline(n) =~ '\<DTD\s\+XHTML\s'
      setf xhtml
      return
    endif
    if getline(n) =~ '{%\s*\(extends\|block\)\>'
      setf htmldjango
      return
    endif
    let n = n + 1
  endwhile
  setf html
endfun

" HTML with Ruby - eRuby
au BufNewFile,BufRead *.rhtml			setf eruby

" HTML with M4
au BufNewFile,BufRead *.html.m4			setf htmlm4

" HTML Cheetah template
au BufNewFile,BufRead *.tmpl			setf htmlcheetah

" Hyper Builder
au BufNewFile,BufRead *.hb			setf hb

" Icon
au BufNewFile,BufRead *.icn			setf icon

" IDL (Interface Description Language)
au BufNewFile,BufRead *.idl			call s:FTidl()

" Distinguish between standard IDL and MS-IDL
fun! s:FTidl()
  let n = 1
  while n < 50 && n < line("$")
    if getline(n) =~ '^\s*import\s\+"\(unknwn\|objidl\)\.idl"'
      setf msidl
      return
    endif
    let n = n + 1
  endwhile
  setf idl
endfun

" Microsoft IDL (Interface Description Language)  Also *.idl
" MOF = WMI (Windows Management Instrumentation) Managed Object Format
au BufNewFile,BufRead *.odl,*.mof		setf msidl

" Icewm menu
au BufNewFile,BufRead */.icewm/menu		setf icemenu

" IDL (Interactive Data Language)
au BufNewFile,BufRead *.pro			setf idlang

" Indent RC
au BufNewFile,BufRead indentrc			setf indentrc

" Inform
au BufNewFile,BufRead .indent.pro		setf indent

" Inform
au BufNewFile,BufRead *.inf,*.INF		setf inform

" Ipfilter
au BufNewFile,BufRead ipf.conf,ipf.rules	setf ipfilter

" Informix 4GL (source - canonical, include file, I4GL+M4 preproc.)
au BufNewFile,BufRead *.4gl,*.4gh,*.m4gl	setf fgl

" .INI file for MSDOS
au BufNewFile,BufRead *.ini			setf dosini

" SysV Inittab
au BufNewFile,BufRead inittab			setf inittab

" Inno Setup
au BufNewFile,BufRead *.iss			setf iss

" JAL
au BufNewFile,BufRead *.jal,*.JAL		setf jal

" Jam
au BufNewFile,BufRead *.jpl,*.jpr		setf jam

" Java
au BufNewFile,BufRead *.java,*.jav		setf java

" JavaCC
au BufNewFile,BufRead *.jj,*.jjt		setf javacc

" JavaScript
au BufNewFile,BufRead *.js,*.javascript		setf javascript

" Java Server Pages
au BufNewFile,BufRead *.jsp			setf jsp

" Java Properties resource file (note: doesn't catch font.properties.pl)
au BufNewFile,BufRead *.properties,*.properties_??,*.properties_??_??	setf jproperties
au BufNewFile,BufRead *.properties_??_??_*	call s:StarSetf('jproperties')

" Jess
au BufNewFile,BufRead *.clp			setf jess

" Jgraph
au BufNewFile,BufRead *.jgr			setf jgraph

" Kixtart
au BufNewFile,BufRead *.kix			setf kix

" Kimwitu[++]
au BufNewFile,BufRead *.k			setf kwt

" KDE script
au BufNewFile,BufRead *.ks			setf kscript

" Kconfig
au BufNewFile,BufRead Kconfig,Kconfig.debug	setf kconfig

" Lace (ISE)
au BufNewFile,BufRead *.ace,*.ACE		setf lace

" Latte
au BufNewFile,BufRead *.latte,*.lte		setf latte

" Limits
au BufNewFile,BufRead /etc/limits		setf limits

" LambdaProlog (*.mod too, see Modsim)
au BufNewFile,BufRead *.sig			setf lprolog

" LDAP LDIF
au BufNewFile,BufRead *.ldif			setf ldif

" Ld loader
au BufNewFile,BufRead *.ld			setf ld

" Lex
au BufNewFile,BufRead *.lex,*.l			setf lex

" Libao
au BufNewFile,BufRead /etc/libao.conf,*/.libao	setf libao

" Libsensors
au BufNewFile,BufRead /etc/sensors.conf		setf sensors

" LFTP
au BufNewFile,BufRead lftp.conf,.lftprc,*lftp/rc	setf lftp

" Lifelines (or Lex for C++!)
au BufNewFile,BufRead *.ll			setf lifelines

" Lilo: Linux loader
au BufNewFile,BufRead lilo.conf*		call s:StarSetf('lilo')

" Lisp (*.el = ELisp, *.cl = Common Lisp, *.jl = librep Lisp)
if has("fname_case")
  au BufNewFile,BufRead *.lsp,*.lisp,*.el,*.cl,*.jl,*.L,.emacs,.sawfishrc setf lisp
else
  au BufNewFile,BufRead *.lsp,*.lisp,*.el,*.cl,*.jl,.emacs,.sawfishrc setf lisp
endif

" SBCL implementation of Common Lisp
au BufNewFile,BufRead sbclrc,.sbclrc		setf lisp

" Lite
au BufNewFile,BufRead *.lite,*.lt		setf lite

" Login access
au BufNewFile,BufRead /etc/login.access		setf loginaccess

" Login defs
au BufNewFile,BufRead /etc/login.defs		setf logindefs

" Logtalk
au BufNewFile,BufRead *.lgt			setf logtalk

" LOTOS
au BufNewFile,BufRead *.lot,*.lotos		setf lotos

" Lout (also: *.lt)
au BufNewFile,BufRead *.lou,*.lout		setf lout

" Lua
au BufNewFile,BufRead *.lua			setf lua

" Lynx style file (or LotusScript!)
au BufNewFile,BufRead *.lss			setf lss

" M4
au BufNewFile,BufRead *.m4
	\ if expand("<afile>") !~? 'html.m4$\|fvwm2rc' | setf m4 | endif

" MaGic Point
au BufNewFile,BufRead *.mgp			setf mgp

" Mail (for Elm, trn, mutt, rn, slrn)
au BufNewFile,BufRead snd.\d\+,.letter,.letter.\d\+,.followup,.article,.article.\d\+,pico.\d\+,mutt{ng,}-*-\w\+,mutt\w\{6\},ae\d\+.txt,/tmp/SLRN[0-9A-Z.]\+,*.eml setf mail

" Mail aliases
au BufNewFile,BufRead /etc/mail/aliases,/etc/aliases	setf mailaliases

" Mailcap configuration file
au BufNewFile,BufRead .mailcap,mailcap		setf mailcap

" Makefile
au BufNewFile,BufRead *[mM]akefile,*.mk,*.mak,*.dsp setf make

" MakeIndex
au BufNewFile,BufRead *.ist,*.mst		setf ist

" Manpage
au BufNewFile,BufRead *.man			setf man

" Man config
au BufNewFile,BufRead /etc/man.conf,man.config	setf manconf

" Maple V
au BufNewFile,BufRead *.mv,*.mpl,*.mws		setf maple

" Mason
au BufNewFile,BufRead *.mason,*.mhtml		setf mason

" Matlab or Objective C
au BufNewFile,BufRead *.m			call s:FTm()

fun! s:FTm()
  let n = 1
  while n < 10
    let line = getline(n)
    if line =~ '^\s*\(#\s*\(include\|import\)\>\|/\*\)'
      setf objc
      return
    endif
    if line =~ '^\s*%'
      setf matlab
      return
    endif
    if line =~ '^\s*(\*'
      setf mma
      return
    endif
    let n = n + 1
  endwhile
  if exists("g:filetype_m")
    exe "setf " . g:filetype_m
  else
    setf matlab
  endif
endfun

" Maya Extension Language
au BufNewFile,BufRead *.mel			setf mel

" Metafont
au BufNewFile,BufRead *.mf			setf mf

" MetaPost
au BufNewFile,BufRead *.mp			setf mp

" MGL
au BufNewFile,BufRead *.mgl			setf mgl

" MMIX or VMS makefile
au BufNewFile,BufRead *.mms			call s:FTmms()

fun! s:FTmms()
  let n = 1
  while n < 10
    let line = getline(n)
    if line =~ '^\s*\(%\|//\)' || line =~ '^\*'
      setf mmix
      return
    endif
    if line =~ '^\s*#'
      setf make
      return
    endif
    let n = n + 1
  endwhile
  setf mmix
endfun


" Modsim III (or LambdaProlog)
au BufNewFile,BufRead *.mod
	\ if getline(1) =~ '\<module\>' |
	\   setf lprolog |
	\ else |
	\   setf modsim3 |
	\ endif

" Modula 2
au BufNewFile,BufRead *.m2,*.DEF,*.MOD,*.md,*.mi setf modula2

" Modula 3 (.m3, .i3, .mg, .ig)
au BufNewFile,BufRead *.[mi][3g]		setf modula3

" Monk
au BufNewFile,BufRead *.isc,*.monk,*.ssc,*.tsc	setf monk

" MOO
au BufNewFile,BufRead *.moo			setf moo

" Modconf
au BufNewFile,BufRead /etc/modules.conf,/etc/conf.modules	setf modconf
au BufNewFile,BufRead /etc/modutils/*
	\ if executable(expand("<afile>")) != 1
	\|  call s:StarSetf('modconf')
	\|endif

" Mplayer config
au BufNewFile,BufRead mplayer.conf,*/.mplayer/config	setf mplayerconf

" Moterola S record
au BufNewFile,BufRead *.s19,*.s28,*.s37		setf srec

" Mrxvtrc
au BufNewFile,BufRead mrxvtrc,.mrxvtrc		setf mrxvtrc

" Msql
au BufNewFile,BufRead *.msql			setf msql

" Mysql
au BufNewFile,BufRead *.mysql			setf mysql

" M$ Resource files
au BufNewFile,BufRead *.rc			setf rc

" MuPAD source
au BufRead,BufNewFile *.mu			setf mupad

" Mush
au BufNewFile,BufRead *.mush			setf mush

" Mutt setup file
au BufNewFile,BufRead Mutt{ng,}rc			setf muttrc
au BufNewFile,BufRead .mutt{ng,}rc*,*/.mutt{ng,}/mutt{ng,}rc*	call s:StarSetf('muttrc')

" Nano
au BufNewFile,BufRead /etc/nanorc,.nanorc	setf nanorc

" Nastran input/DMAP
"au BufNewFile,BufRead *.dat			setf nastran

" Natural
au BufNewFile,BufRead *.NS[ACGLMNPS]		setf natural

" Netrc
au BufNewFile,BufRead .netrc			setf netrc

" Novell netware batch files
au BufNewFile,BufRead *.ncf			setf ncf

" Nroff/Troff (*.ms and *.t are checked below)
au BufNewFile,BufRead *.me
	\ if expand("<afile>") != "read.me" && expand("<afile>") != "click.me" |
	\   setf nroff |
	\ endif
au BufNewFile,BufRead *.tr,*.nr,*.roff,*.tmac,*.mom	setf nroff
au BufNewFile,BufRead *.[1-9]			call s:FTnroff()

" This function checks if one of the first five lines start with a dot.  In
" that case it is probably an nroff file: 'filetype' is set and 1 is returned.
fun! s:FTnroff()
  if getline(1)[0] . getline(2)[0] . getline(3)[0] . getline(4)[0] . getline(5)[0] =~ '\.'
    setf nroff
    return 1
  endif
  return 0
endfun

" Nroff or Objective C++
au BufNewFile,BufRead *.mm			call s:FTmm()

fun! s:FTmm()
  let n = 1
  while n < 10
    let line = getline(n)
    if line =~ '^\s*\(#\s*\(include\|import\)\>\|/\*\)'
      setf objcpp
      return
    endif
    let n = n + 1
  endwhile
  setf nroff
endfun

" Not Quite C
au BufNewFile,BufRead *.nqc			setf nqc

" NSIS
au BufNewFile,BufRead *.nsi			setf nsis

" OCAML
au BufNewFile,BufRead *.ml,*.mli,*.mll,*.mly	setf ocaml

" Occam
au BufNewFile,BufRead *.occ			setf occam

" Omnimark
au BufNewFile,BufRead *.xom,*.xin		setf omnimark

" OpenROAD
au BufNewFile,BufRead *.or			setf openroad

" OPL
au BufNewFile,BufRead *.[Oo][Pp][Ll]		setf opl

" Oracle config file
au BufNewFile,BufRead *.ora			setf ora

" Packet filter conf
au BufNewFile,BufRead pf.conf			setf pf

" Pam conf
au BufNewFile,BufRead /etc/pam.conf		setf pamconf

" PApp
au BufNewFile,BufRead *.papp,*.pxml,*.pxsl	setf papp

" Password file
au BufNewFile,BufRead /etc/passwd,/etc/shadow,/etc/shadow- setf passwd

" Pascal (also *.p)
au BufNewFile,BufRead *.pas			setf pascal

" Delphi project file
au BufNewFile,BufRead *.dpr			setf pascal

" Perl
if has("fname_case")
  au BufNewFile,BufRead *.pl,*.PL		call s:FTpl()
else
  au BufNewFile,BufRead *.pl			call s:FTpl()
endif
au BufNewFile,BufRead *.plx			setf perl

fun! s:FTpl()
  if exists("g:filetype_pl")
    exe "setf " . g:filetype_pl
  else
    " recognize Prolog by specific text in the first non-empty line
    " require a blank after the '%' because Perl uses "%list" and "%translate"
    let l = getline(nextnonblank(1))
    if l =~ '\<prolog\>' || l =~ '^\s*\(%\+\(\s\|$\)\|/\*\)' || l =~ ':-'
      setf prolog
    else
      setf perl
    endif
  endif
endfun

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
au BufNewFile,BufRead *.php,*.php\d		setf php

" Phtml
au BufNewFile,BufRead *.phtml			setf phtml

" Pike
au BufNewFile,BufRead *.pike,*.lpc,*.ulpc,*.pmod setf pike

" Pinfo config
au BufNewFile,BufRead */etc/pinforc,*/.pinforc	setf pinfo

" Palm Resource compiler
au BufNewFile,BufRead *.rcp			setf pilrc

" Pine config
au BufNewFile,BufRead .pinerc,pinerc,.pinercex,pinercex		setf pine

" PL/M (also: *.inp)
au BufNewFile,BufRead *.plm,*.p36,*.pac		setf plm

" PL/SQL
au BufNewFile,BufRead *.pls,*.plsql		setf plsql

" PLP
au BufNewFile,BufRead *.plp			setf plp

" PO and PO template (GNU gettext)
au BufNewFile,BufRead *.po,*.pot		setf po

" Postfix main config
au BufNewFile,BufRead main.cf			setf pfmain

" PostScript (+ font files, encapsulated PostScript, Adobe Illustrator)
au BufNewFile,BufRead *.ps,*.pfa,*.afm,*.eps,*.epsf,*.epsi,*.ai	  setf postscr

" PostScript Printer Description
au BufNewFile,BufRead *.ppd			setf ppd

" Povray
au BufNewFile,BufRead *.pov			setf pov

" Povray configuration
au BufNewFile,BufRead .povrayrc			setf povini

" Povray, PHP or assembly
au BufNewFile,BufRead *.inc			call s:FTinc()

fun! s:FTinc()
  if exists("g:filetype_inc")
    exe "setf " . g:filetype_inc
  else
    let lines = getline(1).getline(2).getline(3)
    if lines =~? "perlscript"
      setf aspperl
    elseif lines =~ "<%"
      setf aspvbs
    elseif lines =~ "<?"
      setf php
    else
      call s:FTasmsyntax()
      if exists("b:asmsyntax")
	exe "setf " . b:asmsyntax
      else
	setf pov
      endif
    endif
  endif
endfun

" Printcap and Termcap
au BufNewFile,BufRead *printcap
	\ let b:ptcap_type = "print" | setf ptcap
au BufNewFile,BufRead *termcap
	\ let b:ptcap_type = "term" | setf ptcap

" PCCTS / ANTRL
"au BufNewFile,BufRead *.g			setf antrl
au BufNewFile,BufRead *.g			setf pccts

" PPWizard
au BufNewFile,BufRead *.it,*.ih			setf ppwiz

" Oracle Pro*C/C++
au BufNewFile,BufRead .pc			setf proc

" Procmail
au BufNewFile,BufRead .procmail,.procmailrc	setf procmail

" Progress or CWEB
au BufNewFile,BufRead *.w			call s:FTprogress_cweb()

function! s:FTprogress_cweb()
  if exists("g:filetype_w")
    exe "setf " . g:filetype_w
    return
  endif
  if getline(1) =~ '&ANALYZE' || getline(3) =~ '&GLOBAL-DEFINE'
    setf progress
  else
    setf cweb
  endif
endfun

" Progress or assembly
au BufNewFile,BufRead *.i			call s:FTprogress_asm()

function! s:FTprogress_asm()
  if exists("g:filetype_i")
    exe "setf " . g:filetype_i
    return
  endif
  " This function checks for an assembly comment the first ten lines.
  " If not found, assume Progress.
  let lnum = 1
  while lnum <= 10 && lnum < line('$')
    let line = getline(lnum)
    if line =~ '^\s*;' || line =~ '^\*'
      call s:FTasm()
      return
    elseif line !~ '^\s*$' || line =~ '^/\*'
      " Not an empty line: Doesn't look like valid assembly code.
      " Or it looks like a Progress /* comment
      break
    endif
    let lnum = lnum + 1
  endw
  setf progress
endfun

" Progress or Pascal
au BufNewFile,BufRead *.p			call s:FTprogress_pascal()

function! s:FTprogress_pascal()
  if exists("g:filetype_p")
    exe "setf " . g:filetype_p
    return
  endif
  " This function checks for valid Pascal syntax in the first ten lines.
  " Look for either an opening comment or a program start.
  " If not found, assume Progress.
  let lnum = 1
  while lnum <= 10 && lnum < line('$')
    let line = getline(lnum)
    if line =~ '^\s*\(program\|unit\|procedure\|function\|const\|type\|var\)\>'
	\ || line =~ '^\s*{' || line =~ '^\s*(\*'
      setf pascal
      return
    elseif line !~ '^\s*$' || line =~ '^/\*'
      " Not an empty line: Doesn't look like valid Pascal code.
      " Or it looks like a Progress /* comment
      break
    endif
    let lnum = lnum + 1
  endw
  setf progress
endfun


" Software Distributor Product Specification File (POSIX 1387.2-1995)
au BufNewFile,BufRead *.psf			setf psf
au BufNewFile,BufRead INDEX,INFO
	\ if getline(1) =~ '^\s*\(distribution\|installed_software\|root\|bundle\|product\)\s*$' |
	\   setf psf |
	\ endif

" Prolog
au BufNewFile,BufRead *.pdb			setf prolog

" Protocols
au BufNewFile,BufRead /etc/protocols		setf protocols

" Pyrex
au BufNewFile,BufRead *.pyx,*.pxd		setf pyrex

" Python
au BufNewFile,BufRead *.py,*.pyw		setf python

" Radiance
au BufNewFile,BufRead *.rad,*.mat		setf radiance

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

" Rexx
au BufNewFile,BufRead *.rexx,*.rex		setf rexx

" R (Splus)
if has("fname_case")
  au BufNewFile,BufRead *.s,*.S			setf r
else
  au BufNewFile,BufRead *.s			setf r
endif

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

" Rexx, Rebol or R
au BufNewFile,BufRead *.r,*.R			call s:FTr()

fun! s:FTr()
  if getline(1) =~ '^REBOL'
    setf rebol
  else
    let n = 1
    let max = line("$")
    if max > 50
      let max = 50
    endif
    while n < max
      " R has # comments
      if getline(n) =~ '^\s*#'
	setf r
	break
      endif
      " Rexx has /* comments */
      if getline(n) =~ '^\s*/\*'
	setf rexx
	break
      endif
      let n = n + 1
    endwhile
    if n >= max
      setf rexx
    endif
  endif
endfun

" Remind
au BufNewFile,BufRead .reminders*		call s:StarSetf('remind')

" Resolv.conf
au BufNewFile,BufRead resolv.conf		setf resolv

" Relax NG Compact
au BufNewFile,BufRead *.rnc			setf rnc

" RPL/2
au BufNewFile,BufRead *.rpl			setf rpl

" Robots.txt
au BufNewFile,BufRead robots.txt		setf robots

" Rpcgen
au BufNewFile,BufRead *.x			setf rpcgen

" reStructuredText Documentation Format
au BufNewFile,BufRead *.rst			setf rst

" RTF
au BufNewFile,BufRead *.rtf			setf rtf

" Ruby
au BufNewFile,BufRead *.rb,*.rbw,*.gem,*.gemspec	setf ruby

" Rantfile is like Ruby
au BufNewFile,BufRead [rR]antfile,*.rant	setf ruby

" S-lang (or shader language!)
au BufNewFile,BufRead *.sl			setf slang

" Samba config
au BufNewFile,BufRead smb.conf			setf samba

" SAS script
au BufNewFile,BufRead *.sas			setf sas

" Sather
au BufNewFile,BufRead *.sa			setf sather

" Scilab
au BufNewFile,BufRead *.sci,*.sce		setf scilab

" SDL
au BufNewFile,BufRead *.sdl,*.pr		setf sdl

" sed
au BufNewFile,BufRead *.sed			setf sed

" Sieve (RFC 3028)
au BufNewFile,BufRead *.siv			setf sieve

" Sendmail
au BufNewFile,BufRead sendmail.cf		setf sm

" Sendmail .mc files are actually m4
au BufNewFile,BufRead *.mc			setf m4

" Services
au BufNewFile,BufRead /etc/services		setf services

" Service Location config
au BufNewFile,BufRead /etc/slp.conf		setf slpconf

" Service Location registration
au BufNewFile,BufRead /etc/slp.reg		setf slpreg

" Service Location SPI
au BufNewFile,BufRead /etc/slp.spi		setf slpspi

" Setserial config
au BufNewFile,BufRead /etc/serial.conf		setf setserial

" SGML
au BufNewFile,BufRead *.sgm,*.sgml
	\ if getline(1).getline(2).getline(3).getline(4).getline(5) =~? 'linuxdoc' |
	\   setf sgmllnx |
	\ elseif getline(1) =~ '<!DOCTYPE.*DocBook' || getline(2) =~ '<!DOCTYPE.*DocBook' |
	\   let b:docbk_type="sgml" |
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
au BufNewFile,BufRead sgml.catalog*		call s:StarSetf('catalog')

" Shell scripts (sh, ksh, bash, bash2, csh); Allow .profile_foo etc.
" Gentoo ebuilds are actually bash scripts
au BufNewFile,BufRead .bashrc*,bashrc,bash.bashrc,.bash_profile*,.bash_logout*,*.bash,*.ebuild call SetFileTypeSH("bash")
au BufNewFile,BufRead .kshrc*,*.ksh call SetFileTypeSH("ksh")
au BufNewFile,BufRead /etc/profile,.profile*,*.sh,*.env call SetFileTypeSH(getline(1))

" Also called from scripts.vim.
fun! SetFileTypeSH(name)
  if expand("<amatch>") =~ g:ft_ignore_pat
    return
  endif
  if a:name =~ '\<ksh\>'
    let b:is_kornshell = 1
    if exists("b:is_bash")
      unlet b:is_bash
    endif
    if exists("b:is_sh")
      unlet b:is_sh
    endif
  elseif exists("g:bash_is_sh") || a:name =~ '\<bash\>' || a:name =~ '\<bash2\>'
    let b:is_bash = 1
    if exists("b:is_kornshell")
      unlet b:is_kornshell
    endif
    if exists("b:is_sh")
      unlet b:is_sh
    endif
  elseif a:name =~ '\<sh\>'
    let b:is_sh = 1
    if exists("b:is_kornshell")
      unlet b:is_kornshell
    endif
    if exists("b:is_bash")
      unlet b:is_bash
    endif
  endif
  call SetFileTypeShell("sh")
endfun

" For shell-like file types, check for an "exec" command hidden in a comment,
" as used for Tcl.
" Also called from scripts.vim, thus can't be local to this script.
fun! SetFileTypeShell(name)
  if expand("<amatch>") =~ g:ft_ignore_pat
    return
  endif
  let l = 2
  while l < 20 && l < line("$") && getline(l) =~ '^\s*\(#\|$\)'
    " Skip empty and comment lines.
    let l = l + 1
  endwhile
  if l < line("$") && getline(l) =~ '\s*exec\s' && getline(l - 1) =~ '^\s*#.*\\$'
    " Found an "exec" line after a comment with continuation
    let n = substitute(getline(l),'\s*exec\s\+\([^ ]*/\)\=', '', '')
    if n =~ '\<tclsh\|\<wish'
      setf tcl
      return
    endif
  endif
  exe "setf " . a:name
endfun

" tcsh scripts
au BufNewFile,BufRead .tcshrc*,*.tcsh,tcsh.tcshrc,tcsh.login	call SetFileTypeShell("tcsh")

" csh scripts, but might also be tcsh scripts (on some systems csh is tcsh)
au BufNewFile,BufRead .login*,.cshrc*,csh.cshrc,csh.login,csh.logout,*.csh,.alias  call s:CSH()

fun! s:CSH()
  if exists("g:filetype_csh")
    call SetFileTypeShell(g:filetype_csh)
  elseif &shell =~ "tcsh"
    call SetFileTypeShell("tcsh")
  else
    call SetFileTypeShell("csh")
  endif
endfun

" Z-Shell script
au BufNewFile,BufRead .zprofile,/etc/zprofile,.zfbfmarks  setf zsh
au BufNewFile,BufRead .zsh*,.zlog*,.zcompdump*  call s:StarSetf('zsh')

" Scheme
au BufNewFile,BufRead *.scm,*.ss		setf scheme

" Screen RC
au BufNewFile,BufRead .screenrc,screenrc	setf screen

" Simula
au BufNewFile,BufRead *.sim			setf simula

" SINDA
au BufNewFile,BufRead *.sin,*.s85		setf sinda

" SiSU
au BufNewFile,BufRead *.sst,*.ssm,*.ssi,*.-sst,*._sst setf sisu
au BufNewFile,BufRead *.sst.meta,*.-sst.meta,*._sst.meta setf sisu

" SKILL
au BufNewFile,BufRead *.il,*.ils,*.cdf		setf skill

" SLRN
au BufNewFile,BufRead .slrnrc			setf slrnrc
au BufNewFile,BufRead *.score			setf slrnsc

" Smalltalk (and TeX)
au BufNewFile,BufRead *.st			setf st
au BufNewFile,BufRead *.cls
	\ if getline(1) =~ '^%' |
	\  setf tex |
	\ else |
	\  setf st |
	\ endif

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

" Snobol4 and spitbol
au BufNewFile,BufRead *.sno,*.spt		setf snobol4

" SNMP MIB files
au BufNewFile,BufRead *.mib,*.my		setf mib

" Snort Configuration
au BufNewFile,BufRead *.hog,snort.conf,vision.conf,*.rules	setf hog

" Spec (Linux RPM)
au BufNewFile,BufRead *.spec			setf spec

" Speedup (AspenTech plant simulator)
au BufNewFile,BufRead *.speedup,*.spdata,*.spd	setf spup

" Slice
au BufNewFile,BufRead *.ice			setf slice

" Spice
au BufNewFile,BufRead *.sp,*.spice		setf spice

" Spyce
au BufNewFile,BufRead *.spy,*.spi		setf spyce

" Squid
au BufNewFile,BufRead squid.conf		setf squid

" SQL for Oracle Designer
au BufNewFile,BufRead *.tyb,*.typ,*.tyc,*.pkb,*.pks	setf sql

" SQL
au BufNewFile,BufRead *.sql			call s:SQL()

fun! s:SQL()
  if exists("g:filetype_sql")
    exe "setf " . g:filetype_sql
  else
    setf sql
  endif
endfun

" SQLJ
au BufNewFile,BufRead *.sqlj			setf sqlj

" SQR
au BufNewFile,BufRead *.sqr,*.sqi		setf sqr

" OpenSSH configuration
au BufNewFile,BufRead ssh_config,*/.ssh/config	setf sshconfig

" OpenSSH server configuration
au BufNewFile,BufRead sshd_config		setf sshdconfig

" Stata
au BufNewFile,BufRead *.ado,*.class,*.do,*.imata,*.mata   setf stata

" SMCL
au BufNewFile,BufRead *.hlp,*.ihlp,*.smcl	setf smcl

" Stored Procedures
au BufNewFile,BufRead *.stp			setf stp

" Standard ML
au BufNewFile,BufRead *.sml			setf sml

" Sysctl
au BufNewFile,BufRead /etc/sysctl.conf		setf sysctl

" Sudoers
au BufNewFile,BufRead /etc/sudoers,sudoers.tmp	setf sudoers

" If the file has an extension of 't' and is in a directory 't' then it is
" almost certainly a Perl test file.
" If the first line starts with '#' and contains 'perl' it's probably a Perl
" file.
" (Slow test) If a file contains a 'use' statement then it is almost certainly
" a Perl file.
fun! s:FTperl()
  if expand("%:e") == 't' && expand("%:p:h:t") == 't'
    setf perl
    return 1
  endif
  if getline(1)[0] == '#' && getline(1) =~ 'perl'
    setf perl
    return 1
  endif
  if search('^use\s\s*\k', 'nc', 30)
    setf perl
    return 1
  endif
  return 0
endfun

" Tads (or Nroff or Perl test file)
au BufNewFile,BufRead *.t
	\ if !s:FTnroff() && !s:FTperl() | setf tads | endif

" Tags
au BufNewFile,BufRead tags			setf tags

" TAK
au BufNewFile,BufRead *.tak			setf tak

" Tcl (JACL too)
au BufNewFile,BufRead *.tcl,*.tk,*.itcl,*.itk,*.jacl	setf tcl

" TealInfo
au BufNewFile,BufRead *.tli			setf tli

" Telix Salt
au BufNewFile,BufRead *.slt			setf tsalt

" Terminfo
au BufNewFile,BufRead *.ti			setf terminfo

" TeX
au BufNewFile,BufRead *.latex,*.sty,*.dtx,*.ltx,*.bbl	setf tex
au BufNewFile,BufRead *.tex			call s:FTtex()

" Choose context, plaintex, or tex (LaTeX) based on these rules:
" 1. Check the first line of the file for "%&<format>".
" 2. Check the first 1000 non-comment lines for LaTeX or ConTeXt keywords.
" 3. Default to "latex" or to g:tex_flavor, can be set in user's vimrc.
fun! s:FTtex()
  let firstline = getline(1)
  if firstline =~ '^%&\s*\a\+'
    let format = tolower(matchstr(firstline, '\a\+'))
    let format = substitute(format, 'pdf', '', '')
    if format == 'tex'
      let format = 'plain'
    endif
  else
    " Default value, may be changed later:
    let format = exists("g:tex_flavor") ? g:tex_flavor : 'plain'
    " Save position, go to the top of the file, find first non-comment line.
    let save_cursor = getpos('.')
    call cursor(1,1)
    let firstNC = search('^\s*[^[:space:]%]', 'c', 1000)
    if firstNC " Check the next thousand lines for a LaTeX or ConTeXt keyword.
      let lpat = 'documentclass\>\|usepackage\>\|begin{\|newcommand\>\|renewcommand\>'
      let cpat = 'start\a\+\|setup\a\+\|usemodule\|enablemode\|enableregime\|setvariables\|useencoding\|usesymbols\|stelle\a\+\|verwende\a\+\|stel\a\+\|gebruik\a\+\|usa\a\+\|imposta\a\+\|regle\a\+\|utilisemodule\>'
      let kwline = search('^\s*\\\%(' . lpat . '\)\|^\s*\\\(' . cpat . '\)',
			      \ 'cnp', firstNC + 1000)
      if kwline == 1	" lpat matched
	let format = 'latex'
      elseif kwline == 2	" cpat matched
	let format = 'context'
      endif		" If neither matched, keep default set above.
      " let lline = search('^\s*\\\%(' . lpat . '\)', 'cn', firstNC + 1000)
      " let cline = search('^\s*\\\%(' . cpat . '\)', 'cn', firstNC + 1000)
      " if cline > 0
      "   let format = 'context'
      " endif
      " if lline > 0 && (cline == 0 || cline > lline)
      "   let format = 'tex'
      " endif
    endif " firstNC
    call setpos('.', save_cursor)
  endif " firstline =~ '^%&\s*\a\+'

  " Translation from formats to file types.  TODO:  add AMSTeX, RevTex, others?
  if format == 'plain'
    setf plaintex
  elseif format == 'context'
    setf context
  else " probably LaTeX
    setf tex
  endif
  return
endfun

" Context
au BufNewFile,BufRead tex/context/*/*.tex	setf context

" Texinfo
au BufNewFile,BufRead *.texinfo,*.texi,*.txi	setf texinfo

" TeX configuration
au BufNewFile,BufRead texmf.cnf			setf texmf

" Tidy config
au BufNewFile,BufRead .tidyrc,tidyrc		setf tidy

" TF mud client
au BufNewFile,BufRead *.tf,.tfrc,tfrc		setf tf

" TPP - Text Presentation Program
au BufNewFile,BufReadPost *.tpp			setf tpp

" Trustees
au BufNewFile,BufRead trustees.conf		setf trustees

" TSS - Geometry
au BufNewFile,BufReadPost *.tssgm		setf tssgm

" TSS - Optics
au BufNewFile,BufReadPost *.tssop		setf tssop

" TSS - Command Line (temporary)
au BufNewFile,BufReadPost *.tsscl		setf tsscl

" Motif UIT/UIL files
au BufNewFile,BufRead *.uit,*.uil		setf uil

" Udev conf
au BufNewFile,BufRead /etc/udev/udev.conf	setf udevconf

" Udev rules
au BufNewFile,BufRead /etc/udev/rules.d/*.rules setf udevrules

" Udev permissions
au BufNewFile,BufRead /etc/udev/permissions.d/*.permissions setf udevperm
"
" Udev symlinks config
au BufNewFile,BufRead /etc/udev/cdsymlinks.conf	setf sh

" UnrealScript
au BufNewFile,BufRead *.uc			setf uc

" Updatedb
au BufNewFile,BufRead /etc/updatedb.conf	setf updatedb

" Vera
au BufNewFile,BufRead *.vr,*.vri,*.vrh		setf vera

" Verilog HDL
au BufNewFile,BufRead *.v			setf verilog

" Verilog-AMS HDL
au BufNewFile,BufRead *.va,*.vams		setf verilogams

" VHDL
au BufNewFile,BufRead *.hdl,*.vhd,*.vhdl,*.vbe,*.vst  setf vhdl
au BufNewFile,BufRead *.vhdl_[0-9]*		call s:StarSetf('vhdl')

" Vim script
au BufNewFile,BufRead *.vim,*.vba,.exrc,_exrc	setf vim

" Viminfo file
au BufNewFile,BufRead .viminfo,_viminfo		setf viminfo

" Virata Config Script File
au BufRead,BufNewFile *.hw,*.module,*.pkg	setf virata

" Visual Basic (also uses *.bas) or FORM
au BufNewFile,BufRead *.frm			call s:FTVB("form")

" SaxBasic is close to Visual Basic
au BufNewFile,BufRead *.sba			setf vb

" Vgrindefs file
au BufNewFile,BufRead vgrindefs			setf vgrindefs

" VRML V1.0c
au BufNewFile,BufRead *.wrl			setf vrml

" Webmacro
au BufNewFile,BufRead *.wm			setf webmacro

" Wget config
au BufNewFile,BufRead .wgetrc,wgetrc		setf wget

" Website MetaLanguage
au BufNewFile,BufRead *.wml			setf wml

" Winbatch
au BufNewFile,BufRead *.wbt			setf winbatch

" WSML
au BufNewFile,BufRead *.wsml			setf wsml

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

" X Pixmap (dynamically sets colors, use BufEnter to make it work better)
au BufEnter *.xpm
	\ if getline(1) =~ "XPM2" |
	\   setf xpm2 |
	\ else |
	\   setf xpm |
	\ endif
au BufEnter *.xpm2				setf xpm2

" XFree86 config
au BufNewFile,BufRead XF86Config
	\ if getline(1) =~ '\<XConfigurator\>' |
	\   let b:xf86c_xfree86_version = 3 |
	\ endif |
	\ setf xf86conf

" Xorg config
au BufNewFile,BufRead xorg.conf,xorg.conf-4	let b:xf86c_xfree86_version = 4 | setf xf86conf

" Xinetd conf
au BufNewFile,BufRead /etc/xinetd.conf		setf xinetd

" XS Perl extension interface language
au BufNewFile,BufRead *.xs			setf xs

" X resources file
au BufNewFile,BufRead .Xdefaults,.Xpdefaults,.Xresources,xdm-config,*.ad setf xdefaults

" Xmath
au BufNewFile,BufRead *.msc,*.msf		setf xmath
au BufNewFile,BufRead *.ms
	\ if !s:FTnroff() | setf xmath | endif

" XML
au BufNewFile,BufRead *.xml
	\ if getline(1) . getline(2) . getline(3) =~ '<!DOCTYPE.*DocBook' |
	\   let b:docbk_type="xml" |
	\   setf docbk |
	\ else |
	\   setf xml |
	\ endif

" XMI (holding UML models) is also XML
au BufNewFile,BufRead *.xmi			setf xml

" CSPROJ files are Visual Studio.NET's XML-based project config files
au BufNewFile,BufRead *.csproj,*.csproj.user	setf xml

" Qt Linguist translation source and Qt User Interface Files are XML
au BufNewFile,BufRead *.ts,*.ui			setf xml

" Xdg menus
au BufNewFile,BufRead /etc/xdg/menus/*.menu	setf xml

" Xquery
au BufNewFile,BufRead *.xq,*.xql,*.xqm,*.xquery,*.xqy	setf xquery

" XSD
au BufNewFile,BufRead *.xsd			setf xsd

" Xslt
au BufNewFile,BufRead *.xsl,*.xslt		setf xslt

" Yacc
au BufNewFile,BufRead *.yy			setf yacc

" Yacc or racc
au BufNewFile,BufRead *.y			call s:FTy()

fun! s:FTy()
  let n = 1
  while n < 100 && n < line("$")
    let line = getline(n)
    if line =~ '^\s*%'
      setf yacc
      return
    endif
    if getline(n) =~ '^\s*\(#\|class\>\)' && getline(n) !~ '^\s*#\s*include'
      setf racc
      return
    endif
    let n = n + 1
  endwhile
  setf yacc
endfun


" Yaml
au BufNewFile,BufRead *.yaml,*.yml		setf yaml

" Zope
"   dtml (zope dynamic template markup language), pt (zope page template),
"   cpt (zope form controller page template)
au BufNewFile,BufRead *.dtml,*.pt,*.cpt		call s:FThtml()
"   zsql (zope sql method)
au BufNewFile,BufRead *.zsql			call s:SQL()

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


" Extra checks for when no filetype has been detected now.  Mostly used for
" patterns that end in "*".  E.g., "zsh*" matches "zsh.vim", but that's a Vim
" script file.
" Most of these should call s:StarSetf() to avoid names ending in .gz and the
" like are used.

" Asterisk config file
au BufNewFile,BufRead *asterisk/*.conf*		call s:StarSetf('asterisk')
au BufNewFile,BufRead *asterisk*/*voicemail.conf* call s:StarSetf('asteriskvm')

" BIND zone
au BufNewFile,BufRead */named/db.*,*/bind/db.*	call s:StarSetf('bindzone')

" Changelog
au BufNewFile,BufRead [cC]hange[lL]og*
	\ if getline(1) =~ '; urgency='
	\|  call s:StarSetf('debchangelog')
	\|else
	\|  call s:StarSetf('changelog')
	\|endif

" Crontab
au BufNewFile,BufRead crontab,crontab.*		call s:StarSetf('crontab')

" Debian Sources.list
au BufNewFile,BufRead /etc/apt/sources.list.d/*	call s:StarSetf('debsources')

" Dracula
au BufNewFile,BufRead drac.*			call s:StarSetf('dracula')

" Fvwm
au BufNewFile,BufRead *fvwmrc*,*fvwm95*.hook
	\ let b:fvwm_version = 1 | call s:StarSetf('fvwm')
au BufNewFile,BufRead *fvwm2rc*
	\ if expand("<afile>:e") == "m4"
	\|  call s:StarSetf('fvwm2m4')
	\|else
	\|  let b:fvwm_version = 2 | call s:StarSetf('fvwm')
	\|endif

" GTK RC
au BufNewFile,BufRead .gtkrc*,gtkrc*		call s:StarSetf('gtkrc')

" Jam
au BufNewFile,BufRead Prl*.*,JAM*.*		call s:StarSetf('jam')

" Jargon
au! BufNewFile,BufRead *jarg*
	\ if getline(1).getline(2).getline(3).getline(4).getline(5) =~? 'THIS IS THE JARGON FILE'
	\|  call s:StarSetf('jargon')
	\|endif

" Kconfig
au BufNewFile,BufRead Kconfig.*			call s:StarSetf('kconfig')

" Makefile
au BufNewFile,BufRead [mM]akefile*		call s:StarSetf('make')

" Modconf
au BufNewFile,BufRead /etc/modprobe.*		call s:StarSetf('modconf')

" Ruby Makefile
au BufNewFile,BufRead [rR]akefile*		call s:StarSetf('ruby')

" Mutt setup file
au BufNewFile,BufRead mutt{ng,}rc*,Mutt{ng,}rc*		call s:StarSetf('muttrc')

" Nroff macros
au BufNewFile,BufRead tmac.*			call s:StarSetf('nroff')

" Pam conf
au BufNewFile,BufRead /etc/pam.d/*		call s:StarSetf('pamconf')

" Printcap and Termcap
au BufNewFile,BufRead *printcap*
	\ if !did_filetype()
	\|  let b:ptcap_type = "print" | call s:StarSetf('ptcap')
	\|endif
au BufNewFile,BufRead *termcap*
	\ if !did_filetype()
	\|  let b:ptcap_type = "term" | call s:StarSetf('ptcap')
	\|endif

" Vim script
au BufNewFile,BufRead *vimrc*			call s:StarSetf('vim')

" Subversion commit file
au BufNewFile,BufRead svn-commit*.tmp		setf svn

" X resources file
au BufNewFile,BufRead Xresources*,*/app-defaults/*,*/Xresources/* call s:StarSetf('xdefaults')

" XFree86 config
au BufNewFile,BufRead XF86Config-4*
	\ let b:xf86c_xfree86_version = 4 | call s:StarSetf('xf86conf')
au BufNewFile,BufRead XF86Config*
	\ if getline(1) =~ '\<XConfigurator\>'
	\|  let b:xf86c_xfree86_version = 3
	\|endif
	\|call s:StarSetf('xf86conf')

" X11 xmodmap
au BufNewFile,BufRead *xmodmap*			call s:StarSetf('xmodmap')

" Xinetd conf
au BufNewFile,BufRead /etc/xinetd.d/*		call s:StarSetf('xinetd')

" Z-Shell script
au BufNewFile,BufRead zsh*,zlog*		call s:StarSetf('zsh')


" Generic configuration file (check this last, it's just guessing!)
au BufNewFile,BufRead,StdinReadPost *
	\ if !did_filetype() && expand("<amatch>") !~ g:ft_ignore_pat
	\    && (getline(1) =~ '^#' || getline(2) =~ '^#' || getline(3) =~ '^#'
	\	|| getline(4) =~ '^#' || getline(5) =~ '^#') |
	\   setf conf |
	\ endif

" Use the plugin-filetype checks last, they may overrule any of the previously
" detected filetypes.
runtime! ftdetect/*.vim

augroup END


" If the GUI is already running, may still need to install the Syntax menu.
" Don't do it when the 'M' flag is included in 'guioptions'.
if has("menu") && has("gui_running")
      \ && !exists("did_install_syntax_menu") && &guioptions !~# "M"
  source <sfile>:p:h/menu.vim
endif

" Restore 'cpoptions'
let &cpo = s:cpo_save
unlet s:cpo_save
