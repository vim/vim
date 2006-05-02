"%% SiSU Vim syntax file
" SiSU Maintainer: Ralph Amissah <ralph@amissah.com>
" SiSU Markup:     SiSU (sisu-0.38)
" (originally looked at Ruby Vim by Mirko Nasato)
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
else
endif
"% 11 Errors?
syn match sisu_error contains=sisu_link,sisu_error_wspace "<![^ei]\S\+!>"
"% 10 Markers: Endnote Identifiers, Pagebreaks etc.: 
if !exists("sisu_no_identifiers")
  syn match   sisu_mark_endnote                      "\~^"
  syn match   sisu_contain       contains=@NoSpell   "</\?sub>"
  syn match   sisu_break         contains=@NoSpell   "<br>\|<br />"
  syn match   sisu_control       contains=@NoSpell   "<p>\|</p>\|<p />\|<:p[bn]>"
  syn match   sisu_html                              "<center>\|</center>"
  syn match   sisu_marktail                          "[~-]#"
  syn match   sisu_html          contains=@NoSpell   "<td>\|<td \|<tr>\|</td>\|</tr>\|<table>\|<table \|</table>"
  syn match   sisu_control                           "\""
  syn match   sisu_underline                         "\(^\| \)_[a-zA-Z0-9]\+_\([ .,]\|$\)"
  syn match   sisu_number        contains=@NoSpell   "[0-9a-f]\{32\}\|[0-9a-f]\{64\}"
  syn match   sisu_link          contains=@NoSpell   "\(https\?://\|\.\.\/\)\S\+"
  "metaverse specific
  syn match   sisu_ocn           contains=@NoSpell   "<\~\d\+;\w\d\+;\w\d\+>"
  syn match   sisu_marktail                          "<\~#>"
  syn match   sisu_markpara      contains=@NoSpell   "<:i[12]>"
  syn match   sisu_link                              " \*\~\S\+"
  syn match   sisu_action                            "^<:insert\d\+>"
  syn match   sisu_contain                           "<:e>"
endif
"% 9 URLs Numbers: and ASCII Codes
syn match   sisu_number                              "\<\(0x\x\+\|0b[01]\+\|0\o\+\|0\.\d\+\|0\|[1-9][\.0-9_]*\)\>"
syn match   sisu_number                              "?\(\\M-\\C-\|\\c\|\\C-\|\\M-\)\=\(\\\o\{3}\|\\x\x\{2}\|\\\=\w\)"
"% 8 Tuned Error - is error if not already matched
syn match sisu_error             contains=sisu_error "[\~/\*!_]{\|}[\~/\*!_]"
syn match sisu_error             contains=sisu_error "<a href\|</a>]"
"% 7 Simple Enclosed Markup:
" Simple Markup:
"%   url/link
syn region sisu_link contains=sisu_error,sisu_error_wspace matchgroup=sisu_action start="^<<\s*|[a-zA-Z0-9^._-]\+|@|[a-zA-Z0-9^._-]\+|"rs=s+2 end="$"
"%   header
syn region sisu_header_content contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break matchgroup=sisu_header start="^0\~\(\S\+\|[^-]\)" end="$"
syn region sisu_header_content contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break matchgroup=sisu_header start="^0\~\(tags\?\|date\)\s\+"rs=e-1 end="\n$"
syn region sisu_header_content contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break matchgroup=sisu_header start="^@\S\+:[+-]\?\s"rs=e-1 end="$"
syn region sisu_header_content contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break matchgroup=sisu_header start="^@\(tags\?\|date\):\s\+"rs=e-1 end="\n$"
"%   headings
syn region sisu_heading contains=sisu_mark_endnote,sisu_content_endnote,sisu_marktail,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_ocn,sisu_error,sisu_error_wspace matchgroup=sisu_structure start="^\([1-8]\|:\?[A-C]\)\~\(\S\+\|[^-]\)" end="$"
"%   grouped text
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="table{.\+" end="}table"
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="{t\~h}" end="$$"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="^\(alt\|group\|poem\){" end="^}\(alt\|group\|poem\)"
syn region sisu_content_alt contains=sisu_error matchgroup=sisu_contain start="^code{" end="^}code"
"%   endnotes
syn region sisu_content_endnote contains=sisu_link,sisu_strikeout,sisu_underline,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_wspace,sisu_mark,sisu_break matchgroup=sisu_mark_endnote start="\~{" end="}\~" skip="\n"
syn region sisu_content_endnote contains=sisu_strikeout,sisu_number,sisu_control,sisu_link,sisu_identifier,sisu_error,sisu_error_wspace,sisu_mark,sisu_break matchgroup=sisu_mark_endnote start="\^\~" end="\n\n"
"%   images
syn region sisu_linked contains=sisu_fontface,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_link start="{" end="}\(\(https\?://\|\.\./\)\S\+\|image\)" oneline
"%   some line operations
syn region sisu_control contains=sisu_strikeout,sisu_identifier,sisu_content_endnote,sisu_mark_endnote,sisu_error,sisu_error_wspace matchgroup=sisu_control start="\(\(^\| \)!_ \|<:b>\)" end="$"
syn region sisu_normal contains=sisu_strikeout,sisu_identifier,sisu_content_endnote,sisu_mark_endnote,sisu_link,sisu_linked,sisu_error,sisu_error_wspace matchgroup=sisu_markpara start="^_\([12*]\|[12]\*\) " end="$"
syn region sisu_normal contains=sisu_strikeout,sisu_identifier,sisu_content_endnote,sisu_mark_endnote,sisu_link,sisu_linked,sisu_error,sisu_error_wspace matchgroup=sisu_markpara start="^\(#[ 1]\|_# \)" end="$"
syn region sisu_comment matchgroup=sisu_comment start="^%\{1,2\} " end="$"
"%   font face curly brackets
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="\*{" end="}\*"
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="!{" end="}!"
syn region sisu_underline contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="_{" end="}_"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="/{" end="}/"
syn region sisu_underline contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="+{" end="}+"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start="\^{" end="}\^"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_fontface start=",{" end="},"
syn region sisu_strikeout contains=sisu_error matchgroup=sisu_fontface start="-{" end="}-" 
syn region sisu_html contains=sisu_error contains=sisu_strikeout matchgroup=sisu_contain start="<a href=\".\{-}\">" end="</a>" oneline
"%   single words bold italicise etc. "workon
syn region sisu_control contains=sisu_error matchgroup=sisu_control start="\([ (]\|^\)\*[^\|{\n\~\\]"hs=e-1 end="\*"he=e-0 skip="[a-zA-Z0-9']" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_content_alt start="\([ ]\|^\)/[^{ \|\n\\]"hs=e-1 end="/\[ \.\]" skip="[a-zA-Z0-9']" oneline
"%   misc
syn region sisu_identifier contains=sisu_error matchgroup=sisu_fontface start="\^[^ {\|\n\\]"rs=s+1 end="\^[ ,.;:'})\\\n]" skip="[a-zA-Z0-9']" oneline
"%   metaverse html (flagged as errors for filetype sisu)
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<b>" end="</b>" skip="\n" oneline
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<em>" end="</em>" skip="\n" oneline
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<i>" end="</i>" skip="\n" oneline
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<u>" end="</u>" skip="\n" oneline
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_mark matchgroup=sisu_html start="<ins>" end="</ins>" skip="\\\\\|\\'" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_html start="<del>" end="</del>" oneline
"%   metaverse <:>
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="<:Table.\{-}>" end="<:Table[-_]end>"
syn region sisu_content_alt contains=sisu_error matchgroup=sisu_contain start="<:code>" end="<:code[-_]end>"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="<:alt>" end="<:alt[-_]end>"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error matchgroup=sisu_contain start="<:poem>" end="<:poem[-_]end>"
"% 6 Expensive Mode
" Expensive Mode:
if !exists("sisu_no_expensive")
else " not Expensive
  syn region  sisu_content_alt  matchgroup=sisu_control start="^\s*def\s" matchgroup=NONE end="[?!]\|\>" skip="\.\|\(::\)" oneline
endif " Expensive?
"% 5 Headers: and Headings (Document Instructions)
syn match sisu_control contains=sisu_error,sisu_error_wspace "4\~! \S\+"
syn region  sisu_markpara contains=sisu_error,sisu_error_wspace start="^=begin" end="^=end.*$"
"% 4 Errors?
syn match sisu_error_wspace contains=sisu_error_wspace "^\s\+"
syn match sisu_error_wspace contains=sisu_error_wspace "\s\s\+"
syn match sisu_error_wspace contains=sisu_error_wspace  " \s*$"
syn match sisu_error contains=sisu_error,sisu_error_wspace "[^ (}]https\?:\S\+"
syn match sisu_error contains=sisu_error_wspace "\t\+"
syn match sisu_error contains=sisu_error "https\?:\S\+[}><]"
syn match sisu_error contains=sisu_error "\([!*/_\+,^]\){\([^(\}\1)]\)\{-}\n\n"
syn match sisu_error contains=sisu_error "^[\-\~]{[^{]\{-}\n\n"
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
"% 3 Error Exceptions?
syn match sisu_control "\n\n" "contains=ALL
syn match sisu_control " //"
syn match sisu_error  "%{"
syn match sisu_error "<br>https\?:\S\+\|https\?:\S\+<br>"
syn match sisu_error "[><]https\?:\S\+\|https\?:\S\+[><]"
"% 2 Definitions - Define the default highlighting.
if version >= 508 || !exists("did_sisu_syntax_inits")
  if version < 508
    let did_sisu_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
"% 1 Defined
  HiLink sisu_normal          Normal
  HiLink sisu_header          PreProc
  HiLink sisu_header_content  Statement
  HiLink sisu_heading         Title
  HiLink sisu_structure       Operator
  HiLink sisu_contain         Include
  HiLink sisu_mark_endnote    Include
  HiLink sisu_link            NonText
  HiLink sisu_linked          String
  HiLink sisu_fontface        Include
  HiLink sisu_strikeout       DiffDelete
  HiLink sisu_content_alt     Special
  HiLink sisu_content_endnote Special
  HiLink sisu_control         Define
  HiLink sisu_ocn             Include
  HiLink sisu_number          Number
  HiLink sisu_identifier      Function
  HiLink sisu_underline       Underlined
  HiLink sisu_markpara        Include
  HiLink sisu_marktail        Include
  HiLink sisu_mark            Identifier
  HiLink sisu_break           Structure
  HiLink sisu_html            Type
  HiLink sisu_action          Identifier
  HiLink sisu_comment         Comment
  HiLink sisu_error_wspace    Error
  HiLink sisu_error           Error
  delcommand HiLink
endif
let b:current_syntax = "sisu"
