" Language:    OCaml
" Maintainer:  David Baelde        <firstname.name@ens-lyon.org>
"              Mike Leary          <leary@nwlink.com>
"              Markus Mottl        <markus.mottl@gmail.com>
"              Stefano Zacchiroli  <zack@bononia.it>
" URL:         http://www.ocaml.info/vim/ftplugin/ocaml.vim
" Last Change: 2006 May 01 - Added .annot support for file.whateverext (SZ)
"	       2006 Apr 11 - Fixed an initialization bug; fixed ASS abbrev (MM)
"              2005 Oct 13 - removed GPL; better matchit support (MM, SZ)
"
if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin=1

" Error handling -- helps moving where the compiler wants you to go
let s:cposet=&cpoptions
set cpo-=C
setlocal efm=
      \%EFile\ \"%f\"\\,\ line\ %l\\,\ characters\ %c-%*\\d:,
      \%EFile\ \"%f\"\\,\ line\ %l\\,\ character\ %c:%m,
      \%+EReference\ to\ unbound\ regexp\ name\ %m,
      \%Eocamlyacc:\ e\ -\ line\ %l\ of\ \"%f\"\\,\ %m,
      \%Wocamlyacc:\ w\ -\ %m,
      \%-Zmake%.%#,
      \%C%m,
      \%D%*\\a[%*\\d]:\ Entering\ directory\ `%f',
      \%X%*\\a[%*\\d]:\ Leaving\ directory\ `%f',
      \%D%*\\a:\ Entering\ directory\ `%f',
      \%X%*\\a:\ Leaving\ directory\ `%f',
      \%DMaking\ %*\\a\ in\ %f

" Add mappings, unless the user didn't want this.
if !exists("no_plugin_maps") && !exists("no_ocaml_maps")
  " (un)commenting
  if !hasmapto('<Plug>Comment')
    nmap <buffer> <LocalLeader>c <Plug>LUncomOn
    vmap <buffer> <LocalLeader>c <Plug>BUncomOn
    nmap <buffer> <LocalLeader>C <Plug>LUncomOff
    vmap <buffer> <LocalLeader>C <Plug>BUncomOff
  endif

  nnoremap <buffer> <Plug>LUncomOn mz0i(* <ESC>$A *)<ESC>`z
  nnoremap <buffer> <Plug>LUncomOff :s/^(\* \(.*\) \*)/\1/<CR>:noh<CR>
  vnoremap <buffer> <Plug>BUncomOn <ESC>:'<,'><CR>`<O<ESC>0i(*<ESC>`>o<ESC>0i*)<ESC>`<
  vnoremap <buffer> <Plug>BUncomOff <ESC>:'<,'><CR>`<dd`>dd`<

  if !hasmapto('<Plug>Abbrev')
    iabbrev <buffer> ASS (assert (0=1) (* XXX *))
  endif
endif

" Let % jump between structure elements (due to Issac Trotts)
let b:mw = ''
let b:mw = b:mw . ',\<let\>:\<and\>:\(\<in\>\|;;\)'
let b:mw = b:mw . ',\<if\>:\<then\>:\<else\>'
let b:mw = b:mw . ',\<\(for\|while\)\>:\<do\>:\<done\>,'
let b:mw = b:mw . ',\<\(object\|sig\|struct\|begin\)\>:\<end\>'
let b:mw = b:mw . ',\<\(match\|try\)\>:\<with\>'
let b:match_words = b:mw

let b:match_ignorecase=0

" switching between interfaces (.mli) and implementations (.ml)
if !exists("g:did_ocaml_switch")
  let g:did_ocaml_switch = 1
  map <LocalLeader>s :call OCaml_switch(0)<CR>
  map <LocalLeader>S :call OCaml_switch(1)<CR>
  fun OCaml_switch(newwin)
    if (match(bufname(""), "\\.mli$") >= 0)
      let fname = substitute(bufname(""), "\\.mli$", ".ml", "")
      if (a:newwin == 1)
        exec "new " . fname
      else
        exec "arge " . fname
      endif
    elseif (match(bufname(""), "\\.ml$") >= 0)
      let fname = bufname("") . "i"
      if (a:newwin == 1)
        exec "new " . fname
      else
        exec "arge " . fname
      endif
    endif
  endfun
endif

" Folding support

" Get the modeline because folding depends on indentation
let s:s = line2byte(line('.'))+col('.')-1
if search('^\s*(\*:o\?caml:')
  let s:modeline = getline(".")
else
  let s:modeline = ""
endif
if s:s > 0
  exe 'goto' s:s
endif

" Get the indentation params
let s:m = matchstr(s:modeline,'default\s*=\s*\d\+')
if s:m != ""
  let s:idef = matchstr(s:m,'\d\+')
elseif exists("g:omlet_indent")
  let s:idef = g:omlet_indent
else
  let s:idef = 2
endif
let s:m = matchstr(s:modeline,'struct\s*=\s*\d\+')
if s:m != ""
  let s:i = matchstr(s:m,'\d\+')
elseif exists("g:omlet_indent_struct")
  let s:i = g:omlet_indent_struct
else
  let s:i = s:idef
endif

" Set the folding method
if exists("g:ocaml_folding")
  setlocal foldmethod=expr
  setlocal foldexpr=OMLetFoldLevel(v:lnum)
endif

" - Only definitions below, executed once -------------------------------------

if exists("*OMLetFoldLevel")
  finish
endif

function s:topindent(lnum)
  let l = a:lnum
  while l > 0
    if getline(l) =~ '\s*\%(\<struct\>\|\<sig\>\|\<object\>\)'
      return indent(l)
    endif
    let l = l-1
  endwhile
  return -s:i
endfunction

function OMLetFoldLevel(l)

  " This is for not merging blank lines around folds to them
  if getline(a:l) !~ '\S'
    return -1
  endif

  " We start folds for modules, classes, and every toplevel definition
  if getline(a:l) =~ '^\s*\%(\<val\>\|\<module\>\|\<class\>\|\<type\>\|\<method\>\|\<initializer\>\|\<inherit\>\|\<exception\>\|\<external\>\)'
    exe 'return ">' (indent(a:l)/s:i)+1 '"'
  endif

  " Toplevel let are detected thanks to the indentation
  if getline(a:l) =~ '^\s*let\>' && indent(a:l) == s:i+s:topindent(a:l)
    exe 'return ">' (indent(a:l)/s:i)+1 '"'
  endif

  " We close fold on end which are associated to struct, sig or object.
  " We use syntax information to do that.
  if getline(a:l) =~ '^\s*end\>' && synIDattr(synID(a:l, indent(a:l)+1, 0), "name") != "ocamlKeyword"
    return (indent(a:l)/s:i)+1
  endif

  " Folds end on ;;
  if getline(a:l) =~ '^\s*;;'
    exe 'return "<' (indent(a:l)/s:i)+1 '"'
  endif

  " Comments around folds aren't merged to them.
  if synIDattr(synID(a:l, indent(a:l)+1, 0), "name") == "ocamlComment"
    return -1
  endif

  return '='
endfunction

" Vim support for OCaml .annot files (requires Vim with python support)
"
" Executing OCamlPrintType(<mode>) function will display in the Vim bottom
" line(s) the type of an ocaml value getting it from the corresponding .annot
" file (if any).  If Vim is in visual mode, <mode> should be "visual" and the
" selected ocaml value correspond to the highlighted text, otherwise (<mode>
" can be anything else) it corresponds to the literal found at the current
" cursor position.
"
" .annot files are parsed lazily the first time OCamlPrintType is invoked; is
" also possible to force the parsing using the OCamlParseAnnot() function.
"
" Typing ',3' will cause OCamlPrintType function to be invoked with
" the right argument depending on the current mode (visual or not).
"
" Copyright (C) <2003-2004> Stefano Zacchiroli <zack@bononia.it>
"
" Created:        Wed, 01 Oct 2003 18:16:22 +0200 zack
" LastModified:   Wed, 25 Aug 2004 18:28:39 +0200 zack

if !has("python")
  finish
endif

python << EOF

import re
import os
import os.path
import string
import time
import vim

debug = False

class AnnExc(Exception):
    def __init__(self, reason):
        self.reason = reason

no_annotations = AnnExc("No type annotations (.annot) file found")
annotation_not_found = AnnExc("No type annotation found for the given text")
def malformed_annotations(lineno):
    return AnnExc("Malformed .annot file (line = %d)" % lineno)

class Annotations:
    """
      .annot ocaml file representation

      File format (copied verbatim from caml-types.el)

      file ::= block *
      block ::= position <SP> position <LF> annotation *
      position ::= filename <SP> num <SP> num <SP> num
      annotation ::= keyword open-paren <LF> <SP> <SP> data <LF> close-paren

      <SP> is a space character (ASCII 0x20)
      <LF> is a line-feed character (ASCII 0x0A)
      num is a sequence of decimal digits
      filename is a string with the lexical conventions of O'Caml
      open-paren is an open parenthesis (ASCII 0x28)
      close-paren is a closed parenthesis (ASCII 0x29)
      data is any sequence of characters where <LF> is always followed by
           at least two space characters.

      - in each block, the two positions are respectively the start and the
        end of the range described by the block.
      - in a position, the filename is the name of the file, the first num
        is the line number, the second num is the offset of the beginning
        of the line, the third num is the offset of the position itself.
      - the char number within the line is the difference between the third
        and second nums.

      For the moment, the only possible keyword is \"type\"."
    """

    def __init__(self):
        self.__filename = None  # last .annot parsed file
        self.__ml_filename = None # as above but s/.annot/.ml/
        self.__timestamp = None # last parse action timestamp
        self.__annot = {}
        self.__re = re.compile(
          '^"[^"]*"\s+(\d+)\s+(\d+)\s+(\d+)\s+"[^"]*"\s+(\d+)\s+(\d+)\s+(\d+)$')

    def __parse(self, fname):
        try:
            f = open(fname)
            line = f.readline() # position line
            lineno = 1
            while (line != ""):
                m = self.__re.search(line)
                if (not m):
                    raise malformed_annotations(lineno)
                line1 = int(m.group(1))
                col1 = int(m.group(3)) - int(m.group(2))
                line2 = int(m.group(4))
                col2 = int(m.group(6)) - int(m.group(5))
                line = f.readline() # "type(" string
                lineno += 1
                if (line == ""): raise malformed_annotations(lineno)
                type = []
                line = f.readline() # type description
                lineno += 1
                if (line == ""): raise malformed_annotations(lineno)
                while line != ")\n":
                    type.append(string.strip(line))
                    line = f.readline()
                    lineno += 1
                    if (line == ""): raise malformed_annotations(lineno)
                type = string.join(type, "\n")
                key = ((line1, col1), (line2, col2))
                if not self.__annot.has_key(key):
                    self.__annot[key] = type
                line = f.readline() # position line
            f.close()
            self.__filename = fname
            self.__ml_filename = vim.current.buffer.name
            self.__timestamp = int(time.time())
        except IOError:
            raise no_annotations

    def parse(self):
        annot_file = os.path.splitext(vim.current.buffer.name)[0] + ".annot"
        self.__parse(annot_file)

    def get_type(self, (line1, col1), (line2, col2)):
        if debug:
            print line1, col1, line2, col2
        if vim.current.buffer.name == None:
            raise no_annotations
        if vim.current.buffer.name != self.__ml_filename or  \
          os.stat(self.__filename).st_mtime > self.__timestamp:
            self.parse()
        try:
            return self.__annot[(line1, col1), (line2, col2)]
        except KeyError:
            raise annotation_not_found

word_char_RE = re.compile("^[\w.]$")

  # TODO this function should recognize ocaml literals, actually it's just an
  # hack that recognize continuous sequences of word_char_RE above
def findBoundaries(line, col):
    """ given a cursor position (as returned by vim.current.window.cursor)
    return two integers identify the beggining and end column of the word at
    cursor position, if any. If no word is at the cursor position return the
    column cursor position twice """
    left, right = col, col
    line = line - 1 # mismatch vim/python line indexes
    (begin_col, end_col) = (0, len(vim.current.buffer[line]) - 1)
    try:
        while word_char_RE.search(vim.current.buffer[line][left - 1]):
            left = left - 1
    except IndexError:
        pass
    try:
        while word_char_RE.search(vim.current.buffer[line][right + 1]):
            right = right + 1
    except IndexError:
        pass
    return (left, right)

annot = Annotations() # global annotation object

def printOCamlType(mode):
    try:
        if mode == "visual":  # visual mode: lookup highlighted text
            (line1, col1) = vim.current.buffer.mark("<")
            (line2, col2) = vim.current.buffer.mark(">")
        else: # any other mode: lookup word at cursor position
            (line, col) = vim.current.window.cursor
            (col1, col2) = findBoundaries(line, col)
            (line1, line2) = (line, line)
        begin_mark = (line1, col1)
        end_mark = (line2, col2 + 1)
        print annot.get_type(begin_mark, end_mark)
    except AnnExc, exc:
        print exc.reason

def parseOCamlAnnot():
    try:
        annot.parse()
    except AnnExc, exc:
        print exc.reason

EOF

fun! OCamlPrintType(current_mode)
  if (a:current_mode == "visual")
    python printOCamlType("visual")
  else
    python printOCamlType("normal")
  endif
endfun

fun! OCamlParseAnnot()
  python parseOCamlAnnot()
endfun

map <LocalLeader>t :call OCamlPrintType("normal")<RETURN>
vmap <LocalLeader>t :call OCamlPrintType("visual")<RETURN>

let &cpoptions=s:cposet
unlet s:cposet

" vim:sw=2
