vim9script

# Vim function for detecting a filetype from the file contents.
# Invoked from "scripts.vim" in 'runtimepath'
#
# Maintainer:	Bram Moolenaar <Bram@vim.org>
# Last Change:	2022 Nov 24

export def DetectFiletype()
  var line1 = getline(1)
  if line1[0] == '#' && line1[1] == '!'
    # File that starts with "#!".
    DetectFromHashBang(line1)
  else
    # File does not start with "#!".
    DetectFromText(line1)
  endif
enddef

# Called for a script that has "#!" in the first line.
def DetectFromHashBang(firstline: string)
  var line1 = firstline

  # Check for a line like "#!/usr/bin/env {options} bash".  Turn it into
  # "#!/usr/bin/bash" to make matching easier.
  # Recognize only a few {options} that are commonly used.
  if line1 =~ '^#!\s*\S*\<env\s'
    line1 = substitute(line1, '\S\+=\S\+', '', 'g')
    line1 = substitute(line1, '\(-[iS]\|--ignore-environment\|--split-string\)', '', '')
    line1 = substitute(line1, '\<env\s\+', '', '')
  endif

  # Get the program name.
  # Only accept spaces in PC style paths: "#!c:/program files/perl [args]".
  # If the word env is used, use the first word after the space:
  # "#!/usr/bin/env perl [path/args]"
  # If there is no path use the first word: "#!perl [path/args]".
  # Otherwise get the last word after a slash: "#!/usr/bin/perl [path/args]".
  var name: string
  if line1 =~ '^#!\s*\a:[/\\]'
    name = substitute(line1, '^#!.*[/\\]\(\i\+\).*', '\1', '')
  elseif line1 =~ '^#!.*\<env\>'
    name = substitute(line1, '^#!.*\<env\>\s\+\(\i\+\).*', '\1', '')
  elseif line1 =~ '^#!\s*[^/\\ ]*\>\([^/\\]\|$\)'
    name = substitute(line1, '^#!\s*\([^/\\ ]*\>\).*', '\1', '')
  else
    name = substitute(line1, '^#!\s*\S*[/\\]\(\i\+\).*', '\1', '')
  endif

  # tcl scripts may have #!/bin/sh in the first line and "exec wish" in the
  # third line.  Suggested by Steven Atkinson.
  if getline(3) =~ '^exec wish'
    name = 'wish'
  endif

    # Bourne-like shell scripts: bash bash2 dash ksh ksh93 sh
  if name =~ '^\(bash\d*\|dash\|ksh\d*\|sh\)\>'
    call dist#ft#SetFileTypeSH(line1)

    # csh scripts
  elseif name =~ '^csh\>'
    if exists("g:filetype_csh")
      call dist#ft#SetFileTypeShell(g:filetype_csh)
    else
      call dist#ft#SetFileTypeShell("csh")
    endif

    # tcsh scripts
  elseif name =~ '^tcsh\>'
    call dist#ft#SetFileTypeShell("tcsh")

    # Z shell scripts
  elseif name =~ '^zsh\>'
    set ft=zsh

    # TCL scripts
  elseif name =~ '^\(tclsh\|wish\|expectk\|itclsh\|itkwish\)\>'
    set ft=tcl

    # Expect scripts
  elseif name =~ '^expect\>'
    set ft=expect

    # Gnuplot scripts
  elseif name =~ '^gnuplot\>'
    set ft=gnuplot

    # Makefiles
  elseif name =~ 'make\>'
    set ft=make

    # Pike
  elseif name =~ '^pike\%(\>\|[0-9]\)'
    set ft=pike

    # Lua
  elseif name =~ 'lua'
    set ft=lua

    # Perl
  elseif name =~ 'perl'
    set ft=perl

    # PHP
  elseif name =~ 'php'
    set ft=php

    # Python
  elseif name =~ 'python'
    set ft=python

    # Groovy
  elseif name =~ '^groovy\>'
    set ft=groovy

    # Raku
  elseif name =~ 'raku'
    set ft=raku

    # Ruby
  elseif name =~ 'ruby'
    set ft=ruby

    # JavaScript
  elseif name =~ 'node\(js\)\=\>\|js\>' || name =~ 'rhino\>'
    set ft=javascript

    # BC calculator
  elseif name =~ '^bc\>'
    set ft=bc

    # sed
  elseif name =~ 'sed\>'
    set ft=sed

    # OCaml-scripts
  elseif name =~ 'ocaml'
    set ft=ocaml

    # Awk scripts; also finds "gawk"
  elseif name =~ 'awk\>'
    set ft=awk

    # Website MetaLanguage
  elseif name =~ 'wml'
    set ft=wml

    # Scheme scripts
  elseif name =~ 'scheme'
    set ft=scheme

    # CFEngine scripts
  elseif name =~ 'cfengine'
    set ft=cfengine

    # Erlang scripts
  elseif name =~ 'escript'
    set ft=erlang

    # Haskell
  elseif name =~ 'haskell'
    set ft=haskell

    # Scala
  elseif name =~ 'scala\>'
    set ft=scala

    # Clojure
  elseif name =~ 'clojure'
    set ft=clojure

    # Free Pascal
  elseif name =~ 'instantfpc\>'
    set ft=pascal

    # Fennel
  elseif name =~ 'fennel\>'
    set ft=fennel

    # MikroTik RouterOS script
  elseif name =~ 'rsc\>'
    set ft=routeros

    # Fish shell
  elseif name =~ 'fish\>'
    set ft=fish

    # Gforth
  elseif name =~ 'gforth\>'
    set ft=forth

    # Icon
  elseif name =~ 'icon\>'
    set ft=icon

    # Guile
  elseif name =~ 'guile'
    set ft=scheme

  endif
enddef


# Called for a script that does not have "#!" in the first line.
def DetectFromText(line1: string)
  var line2 = getline(2)
  var line3 = getline(3)
  var line4 = getline(4)
  var line5 = getline(5)

  # Bourne-like shell scripts: sh ksh bash bash2
  if line1 =~ '^:$'
    call dist#ft#SetFileTypeSH(line1)

  # Z shell scripts
  elseif line1 =~ '^#compdef\>'
      || line1 =~ '^#autoload\>'
      || "\n" .. line1 .. "\n" .. line2 .. "\n" .. line3 ..
	 "\n" .. line4 .. "\n" .. line5
	 =~ '\n\s*emulate\s\+\%(-[LR]\s\+\)\=[ckz]\=sh\>'
    set ft=zsh

  # ELM Mail files
  elseif line1 =~ '^From \([a-zA-Z][a-zA-Z_0-9\.=-]*\(@[^ ]*\)\=\|-\) .* \(19\|20\)\d\d$'
    set ft=mail

  # Mason
  elseif line1 =~ '^<[%&].*>'
    set ft=mason

  # Vim scripts (must have '" vim' as the first line to trigger this)
  elseif line1 =~ '^" *[vV]im$'
    set ft=vim

  # libcxx and libstdc++ standard library headers like "iostream" do not have
  # an extension, recognize the Emacs file mode.
  elseif line1 =~? '-\*-.*C++.*-\*-'
    set ft=cpp

  # MOO
  elseif line1 =~ '^\*\* LambdaMOO Database, Format Version \%([1-3]\>\)\@!\d\+ \*\*$'
    set ft=moo

    # Diff file:
    # - "diff" in first line (context diff)
    # - "Only in " in first line
    # - "--- " in first line and "+++ " in second line (unified diff).
    # - "*** " in first line and "--- " in second line (context diff).
    # - "# It was generated by makepatch " in the second line (makepatch diff).
    # - "Index: <filename>" in the first line (CVS file)
    # - "=== ", line of "=", "---", "+++ " (SVK diff)
    # - "=== ", "--- ", "+++ " (bzr diff, common case)
    # - "=== (removed|added|renamed|modified)" (bzr diff, alternative)
    # - "# HG changeset patch" in first line (Mercurial export format)
  elseif line1 =~ '^\(diff\>\|Only in \|\d\+\(,\d\+\)\=[cda]\d\+\>\|# It was generated by makepatch \|Index:\s\+\f\+\r\=$\|===== \f\+ \d\+\.\d\+ vs edited\|==== //\f\+#\d\+\|# HG changeset patch\)'
	 || (line1 =~ '^--- ' && line2 =~ '^+++ ')
	 || (line1 =~ '^\* looking for ' && line2 =~ '^\* comparing to ')
	 || (line1 =~ '^\*\*\* ' && line2 =~ '^--- ')
	 || (line1 =~ '^=== ' && ((line2 =~ '^=\{66\}' && line3 =~ '^--- ' && line4 =~ '^+++') || (line2 =~ '^--- ' && line3 =~ '^+++ ')))
	 || (line1 =~ '^=== \(removed\|added\|renamed\|modified\)')
    set ft=diff

    # PostScript Files (must have %!PS as the first line, like a2ps output)
  elseif line1 =~ '^%![ \t]*PS'
    set ft=postscr

    # M4 scripts: Guess there is a line that starts with "dnl".
  elseif line1 =~ '^\s*dnl\>'
	 || line2 =~ '^\s*dnl\>'
	 || line3 =~ '^\s*dnl\>'
	 || line4 =~ '^\s*dnl\>'
	 || line5 =~ '^\s*dnl\>'
    set ft=m4

    # AmigaDos scripts
  elseif $TERM == "amiga" && (line1 =~ "^;" || line1 =~? '^\.bra')
    set ft=amiga

    # SiCAD scripts (must have procn or procd as the first line to trigger this)
  elseif line1 =~? '^ *proc[nd] *$'
    set ft=sicad

    # Purify log files start with "****  Purify"
  elseif line1 =~ '^\*\*\*\*  Purify'
    set ft=purifylog

    # XML
  elseif line1 =~ '<?\s*xml.*?>'
    set ft=xml

    # XHTML (e.g.: PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN")
  elseif line1 =~ '\<DTD\s\+XHTML\s'
    set ft=xhtml

    # HTML (e.g.: <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN")
    # Avoid "doctype html", used by slim.
  elseif line1 =~? '<!DOCTYPE\s\+html\>'
    set ft=html

    # PDF
  elseif line1 =~ '^%PDF-'
    set ft=pdf

    # XXD output
  elseif line1 =~ '^\x\{7}: \x\{2} \=\x\{2} \=\x\{2} \=\x\{2} '
    set ft=xxd

    # RCS/CVS log output
  elseif line1 =~ '^RCS file:' || line2 =~ '^RCS file:'
    set ft=rcslog

    # CVS commit
  elseif line2 =~ '^CVS:' || getline("$") =~ '^CVS: '
    set ft=cvs

    # Prescribe
  elseif line1 =~ '^!R!'
    set ft=prescribe

    # Send-pr
  elseif line1 =~ '^SEND-PR:'
    set ft=sendpr

    # SNNS files
  elseif line1 =~ '^SNNS network definition file'
    set ft=snnsnet
  elseif line1 =~ '^SNNS pattern definition file'
    set ft=snnspat
  elseif line1 =~ '^SNNS result file'
    set ft=snnsres

    # Virata
  elseif line1 =~ '^%.\{-}[Vv]irata'
	 || line2 =~ '^%.\{-}[Vv]irata'
	 || line3 =~ '^%.\{-}[Vv]irata'
	 || line4 =~ '^%.\{-}[Vv]irata'
	 || line5 =~ '^%.\{-}[Vv]irata'
    set ft=virata

    # Strace
  elseif line1 =~ '[0-9:.]* *execve(' || line1 =~ '^__libc_start_main'
    set ft=strace

    # VSE JCL
  elseif line1 =~ '^\* $$ JOB\>' || line1 =~ '^// *JOB\>'
    set ft=vsejcl

    # TAK and SINDA
  elseif line4 =~ 'K & K  Associates' || line2 =~ 'TAK 2000'
    set ft=takout
  elseif line3 =~ 'S Y S T E M S   I M P R O V E D '
    set ft=sindaout
  elseif getline(6) =~ 'Run Date: '
    set ft=takcmp
  elseif getline(9) =~ 'Node    File  1'
    set ft=sindacmp

    # DNS zone files
  elseif line1 .. line2 .. line3 .. line4 =~ '^; <<>> DiG [0-9.]\+.* <<>>\|$ORIGIN\|$TTL\|IN\s\+SOA'
    set ft=bindzone

    # BAAN
  elseif line1 =~ '|\*\{1,80}' && line2 =~ 'VRC '
	 || line2 =~ '|\*\{1,80}' && line3 =~ 'VRC '
    set ft=baan

    # Valgrind
  elseif line1 =~ '^==\d\+== valgrind' || line3 =~ '^==\d\+== Using valgrind'
    set ft=valgrind

    # Go docs
  elseif line1 =~ '^PACKAGE DOCUMENTATION$'
    set ft=godoc

    # Renderman Interface Bytestream
  elseif line1 =~ '^##RenderMan'
    set ft=rib

    # Scheme scripts
  elseif line1 =~ 'exec\s\+\S*scheme' || line2 =~ 'exec\s\+\S*scheme'
    set ft=scheme

    # Git output
  elseif line1 =~ '^\(commit\|tree\|object\) \x\{40,\}\>\|^tag \S\+$'
    set ft=git

    # Gprof (gnu profiler)
  elseif line1 == 'Flat profile:'
	&& line2 == ''
	&& line3 =~ '^Each sample counts as .* seconds.$'
    set ft=gprof

    # Erlang terms
    # (See also: http://www.gnu.org/software/emacs/manual/html_node/emacs/Choosing-Modes.html#Choosing-Modes)
  elseif line1 =~? '-\*-.*erlang.*-\*-'
    set ft=erlang

    # YAML
  elseif line1 =~ '^%YAML'
    set ft=yaml

    # MikroTik RouterOS script
  elseif line1 =~ '^#.*by RouterOS.*$'
    set ft=routeros

    # Sed scripts
    # #ncomment is allowed but most likely a false positive so require a space
    # before any trailing comment text
  elseif line1 =~ '^#n\%($\|\s\)'
    set ft=sed

  else
    var lnum = 1
    while getline(lnum) =~ "^? " && lnum < line("$")
      lnum += 1
    endwhile
    if getline(lnum) =~ '^Index:\s\+\f\+$'
      # CVS diff
      set ft=diff

      # locale input files: Formal Definitions of Cultural Conventions
      # filename must be like en_US, fr_FR@euro or en_US.UTF-8
    elseif expand("%") =~ '\a\a_\a\a\($\|[.@]\)\|i18n$\|POSIX$\|translit_'
      lnum = 1
      while lnum < 100 && lnum < line("$")
	if getline(lnum) =~ '^LC_\(IDENTIFICATION\|CTYPE\|COLLATE\|MONETARY\|NUMERIC\|TIME\|MESSAGES\|PAPER\|TELEPHONE\|MEASUREMENT\|NAME\|ADDRESS\)$'
	  setf fdcc
	  break
	endif
	lnum += 1
      endwhile
    endif
  endif
enddef
