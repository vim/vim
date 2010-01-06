"SiSU Vim syntax file
"SiSU Maintainer: Ralph Amissah <ralph@amissah.com>
"SiSU Markup:     SiSU (sisu-0.69.0, 2008-09-16)
"(originally looked at Ruby Vim by Mirko Nasato)

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
else
endif

" Errors:
syn match sisu_error contains=sisu_link,sisu_error_wspace "<![^ei]\S\+!>"

" Markers Identifiers:
if !exists("sisu_no_identifiers")
  syn match   sisu_mark_endnote                                           "\~^"
  syn match   sisu_contain             contains=@NoSpell                  "</\?sub>"
  syn match   sisu_break               contains=@NoSpell                  "<br>\|<br />"
  syn match   sisu_control             contains=@NoSpell                  "<p>\|</p>\|<p />\|<:p[bn]>"
  syn match   sisu_html                                                   "<center>\|</center>"
  syn match   sisu_marktail                                               "[~-]#"
  syn match   sisu_html                contains=@NoSpell                  "<td>\|<td \|<tr>\|</td>\|</tr>\|<table>\|<table \|</table>"
  syn match   sisu_control                                                "\""
  syn match   sisu_underline                                              "\(^\| \)_[a-zA-Z0-9]\+_\([ .,]\|$\)"
  syn match   sisu_number              contains=@NoSpell                  "[0-9a-f]\{32\}\|[0-9a-f]\{64\}"
  syn match   sisu_link                contains=@NoSpell                  "\(_\?https\?://\|\.\.\/\)\S\+"
  "metaverse specific
  syn match   sisu_ocn                 contains=@NoSpell                  "<\~\d\+;\w\d\+;\w\d\+>"
  syn match   sisu_marktail                                               "<\~#>"
  syn match   sisu_markpara            contains=@NoSpell                  "<:i[1-9]>"
  syn match   sisu_link                                                   " \*\~\S\+"
  syn match   sisu_action                                                 "^<:insert\d\+>"
  syn match   sisu_require             contains=@NoSpell                  "^<<\s*[a-zA-Z0-9^._-]\+\.ss[it]$"
  syn match   sisu_require             contains=@NoSpell                  "^<<{[a-zA-Z0-9^._-]\+\.ss[it]}$"
  syn match   sisu_contain                                                "<:e>"
  syn match   sisu_sem_marker                                             ";{\|};[a-z._]*[a-z]"
  syn match   sisu_sem_marker_block                                       "\([a-z][a-z._]*\|\):{\|}:[a-z._]*[a-z]"
  syn match   sisu_sem_ex_marker                                          ";\[\|\];[a-z._]*[a-z]"
  syn match   sisu_sem_ex_marker_block                                    "\([a-z][a-z._]*\|\):\[\|\]:[a-z._]*[a-z]"
  syn match   sisu_sem_block contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break,sisu_sem_block,sisu_sem_content,sisu_sem_marker_block,sisu_sem_marker,sisu_sem_ex_marker_block,sisu_sem_ex_marker,sisu_mark_endnote,sisu_content_endnote "\([a-z]*\):{[^}].\{-}}:\1"
  syn match   sisu_sem_content contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break,sisu_sem_marker_block,sisu_sem_marker,sisu_sem_ex_marker_block,sisu_sem_ex_marker ";{[^}].\{-}};[a-z]\+"
  syn match   sisu_sem_ex_block contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break,sisu_sem_block,sisu_sem_content,sisu_sem_marker_block,sisu_sem_marker,sisu_sem_ex_marker_block,sisu_sem_ex_marker,sisu_mark_endnote,sisu_content_endnote "\([a-z]*\):\[[^}].\{-}\]:\1"
  syn match   sisu_sem_ex_content contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break,sisu_sem_marker_block,sisu_sem_marker,sisu_sem_ex_marker_block,sisu_sem_ex_marker ";\[[^}].\{-}\];[a-z]\+"
endif

"URLs Numbers And ASCII Codes:
syn match   sisu_number                              "\<\(0x\x\+\|0b[01]\+\|0\o\+\|0\.\d\+\|0\|[1-9][\.0-9_]*\)\>"
syn match   sisu_number                              "?\(\\M-\\C-\|\\c\|\\C-\|\\M-\)\=\(\\\o\{3}\|\\x\x\{2}\|\\\=\w\)"

"Tuned Error: (is error if not already matched)
syn match sisu_error             contains=sisu_error "[\~/\*!_]{\|}[\~/\*!_]"
syn match sisu_error             contains=sisu_error "<a href\|</a>]"

"Simple Paired Enclosed Markup:
"url/link
syn region sisu_link contains=sisu_error,sisu_error_wspace matchgroup=sisu_action start="^<<\s*|[a-zA-Z0-9^._-]\+|@|[a-zA-Z0-9^._-]\+|"rs=s+2 end="$"
"header
syn region sisu_header_content contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break matchgroup=sisu_header start="^0\~\(\S\+\|[^-]\)" end="\n$"
syn region sisu_header_content contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break matchgroup=sisu_header start="^[@%]\S\+:[+-]\?\s"rs=e-1 end="\n$"
"headings
syn region sisu_heading contains=sisu_mark_endnote,sisu_content_endnote,sisu_marktail,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_ocn,sisu_error,sisu_error_wspace matchgroup=sisu_structure start="^\([1-8]\|:\?[A-C]\)\~\(\S\+\|[^-]\)" end="$"
"grouped text
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="^table{.\+" end="}table"
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="^{\(t\|table\)\(\~h\)\?\(\sc[0-9]\+;\)\?[0-9; ]*}" end="\n\n"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="^\(alt\|group\|poem\){" end="^}\(alt\|group\|poem\)"
syn region sisu_content_alt contains=sisu_error matchgroup=sisu_contain start="^code{" end="^}code"
"endnotes
syn region sisu_content_endnote contains=sisu_link,sisu_strikeout,sisu_underline,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_wspace,sisu_mark,sisu_break,sisu_sem_block,sisu_sem_content,sisu_sem_marker_block,sisu_sem_marker,sisu_sem_ex_marker_block,sisu_sem_ex_marker matchgroup=sisu_mark_endnote start="\~{[*+]*" end="}\~" skip="\n"
syn region sisu_content_endnote contains=sisu_link,sisu_strikeout,sisu_underline,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_wspace,sisu_mark,sisu_break,sisu_sem_block,sisu_sem_content,sisu_sem_marker matchgroup=sisu_mark_endnote start="\~\[[*+]*" end="\]\~" skip="\n"
syn region sisu_content_endnote contains=sisu_strikeout,sisu_number,sisu_control,sisu_link,sisu_identifier,sisu_error,sisu_error_wspace,sisu_mark,sisu_break matchgroup=sisu_mark_endnote start="\^\~" end="\n\n"
"links and images
syn region sisu_linked contains=sisu_fontface,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_sem_block,sisu_sem_content,sisu_sem_marker_block,sisu_sem_marker,sisu_sem_ex_marker_block,sisu_sem_ex_marker,sisu_sem_block,sisu_error matchgroup=sisu_link start="{\(\~^\s\)\?" end="}\(https\?:/\/\|\.\./\)\S\+" oneline
syn region sisu_linked contains=sisu_fontface,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_sem_block,sisu_sem_content,sisu_sem_marker_block,sisu_sem_marker,sisu_sem_ex_marker_block,sisu_sem_ex_marker,sisu_sem_block,sisu_error matchgroup=sisu_link start="{\(\~^\s\)\?" end="\[[1-5][sS]*\]}\S\+\.ss[tm]" oneline
syn region sisu_linked contains=sisu_fontface,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_link start="{" end="}image" oneline
"some line operations
syn region sisu_control contains=sisu_strikeout,sisu_identifier,sisu_content_endnote,sisu_mark_endnote,sisu_error,sisu_error_wspace matchgroup=sisu_control start="\(\(^\| \)!_ \|<:b>\)" end="$"
syn region sisu_normal contains=sisu_strikeout,sisu_identifier,sisu_content_endnote,sisu_mark_endnote,sisu_link,sisu_sem_block,sisu_sem_content,sisu_sem_marker_block,sisu_sem_marker,sisu_sem_ex_marker_block,sisu_sem_ex_marker,sisu_linked,sisu_error,sisu_error_wspace matchgroup=sisu_markpara start="^_\([1-9*]\|[1-9]\*\) " end="$"
syn region sisu_normal contains=sisu_strikeout,sisu_identifier,sisu_content_endnote,sisu_mark_endnote,sisu_link,sisu_linked,sisu_error,sisu_error_wspace matchgroup=sisu_markpara start="^\(#[ 1]\|_# \)" end="$"
syn region sisu_comment matchgroup=sisu_comment start="^%\{1,2\} " end="$"
"font face curly brackets
"syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_sem start="\S\+:{" end="}:[^<>,.!?:; ]\+" oneline
syn region sisu_index matchgroup=sisu_index_block start="^={" end="}"
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="\*{" end="}\*"
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="!{" end="}!"
syn region sisu_underline contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="_{" end="}_"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="/{" end="}/"
syn region sisu_underline contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="+{" end="}+"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="\^{" end="}\^"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start=",{" end="},"
syn region sisu_strikeout contains=sisu_error matchgroup=sisu_fontface start="-{" end="}-"
syn region sisu_html contains=sisu_error contains=sisu_strikeout matchgroup=sisu_contain start="<a href=\".\{-}\">" end="</a>" oneline
"single words bold italicise etc. "workon
syn region sisu_control contains=sisu_error matchgroup=sisu_control start="\([ (]\|^\)\*[^\|{\n\~\\]"hs=e-1 end="\*"he=e-0 skip="[a-zA-Z0-9']" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_content_alt start="\([ ]\|^\)/[^{ \|\n\\]"hs=e-1 end="/\[ \.\]" skip="[a-zA-Z0-9']" oneline
"misc
syn region sisu_identifier contains=sisu_error matchgroup=sisu_fontface start="\^[^ {\|\n\\]"rs=s+1 end="\^[ ,.;:'})\\\n]" skip="[a-zA-Z0-9']" oneline
"metaverse html (flagged as errors for filetype sisu)
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<b>" end="</b>" skip="\n" oneline
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<em>" end="</em>" skip="\n" oneline
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<i>" end="</i>" skip="\n" oneline
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<u>" end="</u>" skip="\n" oneline
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<ins>" end="</ins>" skip="\\\\\|\\'" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_html start="<del>" end="</del>" oneline
"metaverse
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="<:Table.\{-}>" end="<:Table[-_]end>"
syn region sisu_content_alt contains=sisu_error matchgroup=sisu_contain start="<:code>" end="<:code[-_]end>"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="<:alt>" end="<:alt[-_]end>"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="<:poem>" end="<:poem[-_]end>"

"Expensive Mode:
if !exists("sisu_no_expensive")
else " not Expensive
  syn region  sisu_content_alt  matchgroup=sisu_control start="^\s*def\s" matchgroup=NONE end="[?!]\|\>" skip="\.\|\(::\)" oneline
endif " Expensive?

"Headers And Headings: (Document Instructions)
syn match sisu_control contains=sisu_error,sisu_error_wspace "4\~! \S\+"
syn region  sisu_markpara contains=sisu_error,sisu_error_wspace start="^=begin" end="^=end.*$"

"Errors:
syn match sisu_error_wspace contains=sisu_error_wspace "^\s\+"
syn match sisu_error_wspace contains=sisu_error_wspace "\s\s\+"
syn match sisu_error_wspace contains=sisu_error_wspace  " \s*$"
syn match sisu_error contains=sisu_error_wspace "\t\+"
syn match sisu_error contains=sisu_error,sisu_error_wspace "\([^ (][_\\]\||[^ (}]\)https\?:\S\+"
syn match sisu_error contains=sisu_error "_\?https\?:\S\+[}><]"
syn match sisu_error contains=sisu_error "\([!*/_\+,^]\){\([^(\}\1)]\)\{-}\n\n"
syn match sisu_error contains=sisu_error "^[\~]{[^{]\{-}\n\n"
syn match sisu_error contains=sisu_error "\s\+.{{"
syn match sisu_error contains=sisu_error "^\~\s*$"
syn match sisu_error contains=sisu_error "^[0-9]\~\s*$"
syn match sisu_error contains=sisu_error "^[0-9]\~\S\+\s*$"
syn match sisu_error contains=sisu_error "[^{]\~\^[^ \)]"
syn match sisu_error contains=sisu_error "\~\^\s\+\.\s*"
syn match sisu_error contains=sisu_error "{\~^\S\+"
syn match sisu_error contains=sisu_error "[_/\*!^]{[ .,:;?><]*}[_/\*!^]"
syn match sisu_error contains=sisu_error "[^ (\"'(\[][_/\*!]{\|}[_/\*!][a-zA-Z0-9)\]\"']"
syn match sisu_error contains=sisu_error "<dir>"
"errors for filetype sisu, though not error in 'metaverse':
syn match sisu_error contains=sisu_error,sisu_match,sisu_strikeout,sisu_contain,sisu_content_alt,sisu_mark,sisu_break,sisu_number "<[a-zA-Z\/]\+>"
syn match sisu_error  "/\?<\([biu]\)>[^(</\1>)]\{-}\n\n"

"Error Exceptions:
syn match sisu_control "\n\n" "contains=ALL
syn match sisu_control " //"
syn match sisu_error  "%{"
syn match sisu_error "<br>_\?https\?:\S\+\|_\?https\?:\S\+<br>"
syn match sisu_error "[><]_\?https\?:\S\+\|_\?https\?:\S\+[><]"

"Definitions Default Highlighting:
hi def link sisu_normal           Normal
hi def link sisu_header           PreProc
hi def link sisu_header_content   Statement
hi def link sisu_heading          Title
hi def link sisu_structure        Operator
hi def link sisu_contain          Include
hi def link sisu_mark_endnote     Include
hi def link sisu_require          NonText
hi def link sisu_link             NonText
hi def link sisu_linked           String
hi def link sisu_fontface         Include
hi def link sisu_strikeout        DiffDelete
hi def link sisu_content_alt      Special
hi def link sisu_sem_content      SpecialKey
hi def link sisu_sem_block        Special
hi def link sisu_sem_marker       Visual
"hi def link sisu_sem_marker       Structure
hi def link sisu_sem_marker_block MatchParen
hi def link sisu_sem_ex_marker    FoldColumn
hi def link sisu_sem_ex_marker_block Folded
hi def link sisu_sem_ex_content   Comment
"hi def link sisu_sem_ex_content   SpecialKey
hi def link sisu_sem_ex_block     Comment
hi def link sisu_index            SpecialKey
hi def link sisu_index_block      Visual
hi def link sisu_content_endnote  Special
hi def link sisu_control          Define
hi def link sisu_ocn              Include
hi def link sisu_number           Number
hi def link sisu_identifier       Function
hi def link sisu_underline        Underlined
hi def link sisu_markpara         Include
hi def link sisu_marktail         Include
hi def link sisu_mark             Identifier
hi def link sisu_break            Structure
hi def link sisu_html             Type
hi def link sisu_action           Identifier
hi def link sisu_comment          Comment
hi def link sisu_error_sem_marker Error
hi def link sisu_error_wspace     Error
hi def link sisu_error            Error
let b:current_syntax = "sisu"
