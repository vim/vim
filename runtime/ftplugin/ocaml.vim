" Vim settings file
" Language:     OCaml
" Maintainers:  Mike Leary          <leary@nwlink.com>
"               Markus Mottl        <markus@oefai.at>
"               Stefano Zacchiroli  <zack@bononia.it>
" URL:          http://www.oefai.at/~markus/vim/ftplugin/ocaml.vim
" Last Change:  2004 Apr 12 - better .ml/.mli-switching without Python (SZ)
"               2003 Nov 21 - match_words-patterns and .ml/.mli-switching (MM)
"               2003 Oct 16 - re-entered variable 'did_ocaml_dtypes' (MM)
"               2003 Oct 15 - added Stefano Zacchirolis (SZ) Python-code for
"                             displaying type annotations (MM)

" Only do these settings when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't do other file type settings for this buffer
let b:did_ftplugin = 1

set cpo-=C

" Error formats
setlocal efm=
  \%EFile\ \"%f\"\\,\ line\ %l\\,\ characters\ %c-%*\\d:,
  \%EFile\ \"%f\"\\,\ line\ %l\\,\ character\ %c:%m,
  \%+EReference\ to\ unbound\ regexp\ name\ %m,
  \%Eocamlyacc:\ e\ -\ line\ %l\ of\ \"%f\"\\,\ %m,
  \%Wocamlyacc:\ w\ -\ %m,
  \%-Zmake%.%#,
  \%C%m

" Add mappings, unless the user didn't want this.
if !exists("no_plugin_maps") && !exists("no_ocaml_maps")
  " Uncommenting
  if !hasmapto('<Plug>Comment')
    nmap <buffer> <LocalLeader>c <Plug>LUncomOn
    vmap <buffer> <LocalLeader>c <Plug>BUncomOn
    nmap <buffer> <LocalLeader>C <Plug>LUncomOff
    vmap <buffer> <LocalLeader>C <Plug>BUncomOff
  endif

  nnoremap <buffer> <Plug>LUncomOn mz0i(* <ESC>$A *)<ESC>`z
  nnoremap <buffer> <Plug>LUncomOff <ESC>:s/^(\* \(.*\) \*)/\1/<CR>
  vnoremap <buffer> <Plug>BUncomOn <ESC>:'<,'><CR>`<O<ESC>0i(*<ESC>`>o<ESC>0i*)<ESC>`<
  vnoremap <buffer> <Plug>BUncomOff <ESC>:'<,'><CR>`<dd`>dd`<

  if !hasmapto('<Plug>Abbrev')
    iabbrev <buffer> ASS (assert false)
  endif
endif

" Let % jump between structure elements (due to Issac Trotts)
let b:mw='\<let\>:\<and\>:\(\<in\>\|;;\),'
let b:mw=b:mw . '\<if\>:\<then\>:\<else\>,\<do\>:\<done\>,'
let b:mw=b:mw . '\<\(object\|sig\|struct\|begin\)\>:\<end\>'
let b:match_words=b:mw

" switching between interfaces (.mli) and implementations (.ml)
if !exists("g:did_ocaml_switch")
  let g:did_ocaml_switch = 1
  map ,s :call OCaml_switch(0)<CR>
  map ,S :call OCaml_switch(1)<CR>
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

" Vim support for OCaml 3.07 .annot files (requires Vim with python support)
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
" Hitting the <F3> key will cause OCamlPrintType function to be invoked with
" the right argument depending on the current mode (visual or not).
"
" Copyright (C) <2003> Stefano Zacchiroli <zack@bononia.it>
"
" Created:        Wed, 01 Oct 2003 18:16:22 +0200 zack
" LastModified:   Mon, 06 Oct 2003 11:05:39 +0200 zack
"
" This program is free software; you can redistribute it and/or modify
" it under the terms of the GNU General Public License as published by
" the Free Software Foundation; either version 2 of the License, or
" (at your option) any later version.
"
" This program is distributed in the hope that it will be useful,
" but WITHOUT ANY WARRANTY; without even the implied warranty of
" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
" GNU General Public License for more details.
"
" You should have received a copy of the GNU General Public License
" along with this program; if not, write to the Free Software
" Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
"

if !has("python")
  echo "Python support not found: OCaml .annot support disabled"
  finish
endif

if !exists("g:did_ocaml_dtypes")
  let g:did_ocaml_dtypes = 1
else
  finish
endif

python << EOF

import re
import os
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
      - end of the range described by the block.
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
          '^"[^"]+"\s+(\d+)\s+(\d+)\s+(\d+)\s+"[^"]+"\s+(\d+)\s+(\d+)\s+(\d+)$')

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
                self.__annot[(line1, col1), (line2, col2)] = type
                line = f.readline() # position line
            f.close()
            self.__filename = fname
            self.__ml_filename = re.sub("\.annot$", ".ml", fname)
            self.__timestamp = int(time.time())
        except IOError:
            raise no_annotations

    def parse(self):
        annot_file = re.sub("\.ml$", ".annot", vim.current.buffer.name)
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

fun OCamlPrintType(current_mode)
  if (a:current_mode == "visual")
    python printOCamlType("visual")
  else
    python printOCamlType("normal")
  endif
endfun

fun OCamlParseAnnot()
  python parseOCamlAnnot()
endfun

map <F3> :call OCamlPrintType("normal")<RETURN>
vmap <F3> :call OCamlPrintType("visual")<RETURN>
