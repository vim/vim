"%% SiSU Vim syntax file
" Markup:       SiSU
" SiSU Maintainer: Ralph Amissah <ralph@amissah.com>
" (originally looked at Ruby Vim by Mirko Nasato)
if version < 600
  syntax clear
elseif exists("b:current_syntax")
"  :colorscheme green
  finish
else
endif
"% 12 Errors?
syn match sisu_error contains=sisu_contain,sisu_control,sisu_markpara,sisu_mark,sisu_content_alt,sisu_error_wspace "<![^ei]\S\+!>"
"% 11 Expression Substitution: and Backslash Notation
"% 10 Markers: Endnote Identifiers, Pagebreaks etc.: 
if !exists("sisu_no_identifiers")
  syn match   sisu_mark_endnote   "\~^"
  syn match   sisu_contain        "</\?sub>"
  syn match   sisu_break          "<br>\|<br />"
  syn match   sisu_control        "<p>\|</p>\|<p />\|<:p[bn]>"
  syn match   sisu_html           "<center>\|</center>"
  syn match   sisu_markpara       "^_\([12]\*\?\|\*\)\s\+"
  syn match   sisu_markpara       "#[ 1]\|_# "
  syn match   sisu_marktail       "[~-]#"
  syn match   sisu_html           "<td>\|<td \|<tr>\|</td>\|</tr>\|<table>\|<table \|</table>"
  syn match   sisu_control        "\""
  syn match   sisu_underline      "\(^\| \)_[a-zA-Z0-9]\+_\([ .,]\|$\)"he=e-1
  "metaverse specific
  syn match   sisu_ocn            "<\~\d\+;\w\d\+;\w\d\+>"
  syn match   sisu_digest         "<[0-9a-f]\{32\}:[0-9a-f]\{32\}>\|<[0-9a-f]\{32\}>"
  syn match   sisu_digest         "<[0-9a-f]\{64\}:[0-9a-f]\{64\}>\|<[0-9a-f]\{64\}>"
  syn match   sisu_marktail       "<\~#>"
  syn match   sisu_markpara       "<:i[12]>"
  syn match   sisu_link           " \*\~\S\+"
  syn match   sisu_action         "^<<.\+"
  syn match   sisu_action         "^<:insert\d\+>"
  syn match   sisu_contain        "<:e>"
endif
"% 9 URLs Numbers: and ASCII Codes
syn match   sisu_number           "\<\(0x\x\+\|0b[01]\+\|0\o\+\|0\.\d\+\|0\|[1-9][\.0-9_]*\)\>"
syn match   sisu_link             "\(http://\|\.\./\)\S\+"
syn match   sisu_number           "?\(\\M-\\C-\|\\c\|\\C-\|\\M-\)\=\(\\\o\{3}\|\\x\x\{2}\|\\\=\w\)"
"% 8 Tuned Error - is error if not already matched
syn match sisu_error              "[\~/\*!_]{\|}[\~/\*!_]" contains=sisu_error
syn match sisu_error              "<a href\|</a>]" contains=sisu_error
"% 7 Simple Enclosed Markup:
" Simple Markup:
"%   header
syn region sisu_header contains=sisu_error,sisu_error_wspace,sisu_content_alt,sisu_link,sisu_linked,sisu_break matchgroup=sisu_header start="0\~" end="$"
"%   headings
syn region sisu_heading contains=sisu_mark_endnote,sisu_content_endnote,sisu_marktail,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_ocn,sisu_digest,sisu_error,sisu_error_wspace,sisu_error_spell matchgroup=sisu_heading start="[1-8]\~[^-]" end="$"
"%   grouped text
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_contain start="table{.\+" end="}table"
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_contain start="{t\~h}" end="$$"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_contain start="^\(alt\|group\|poem\){" end="^}\(alt\|group\|poem\)"
syn region sisu_content_alt contains=sisu_error matchgroup=sisu_contain start="^code{" end="^}code"
"%   endnotes
syn region sisu_content_endnote contains=sisu_digest,sisu_link,sisu_strikeout,sisu_underline,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_wspace,sisu_error_spell,sisu_mark,sisu_break matchgroup=sisu_mark_endnote start="\~{" end="}\~" skip="\n"
syn region sisu_content_endnote contains=sisu_strikeout,sisu_number,sisu_control,sisu_link,sisu_identifier,sisu_error,sisu_error_wspace,sisu_error_spell,sisu_mark,sisu_break matchgroup=sisu_mark_endnote start="\^\~" end="\n\n"
"%   images
syn region sisu_linked contains=sisu_fontface,sisu_strikeout,sisu_number,sisu_control,sisu_error,sisu_error_spell matchgroup=sisu_link start="{" end="}\(\(http://\|\.\./\)\S\+\|image\)" oneline
"syn region sisu_linked contains=sisu_fontface,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_link start="{" end="}\(\(http://\|\.\./\)\S\+\|image\)" oneline
"sisu_identifier fix
""syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_link start="{" end="}\(http\S\+\|image\)" oneline
"%   font face curly brackets
syn region sisu_control contains=sisu_strikeout,sisu_identifier,sisu_content_endnote,sisu_mark_endnote,sisu_error,sisu_error_spell matchgroup=sisu_control start="\(\(^\| \)!_ \|<:b>\)" end="$"
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_fontface start="\*{" end="}\*"
syn region sisu_control contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_fontface start="!{" end="}!"
syn region sisu_underline contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_fontface start="_{" end="}_"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_fontface start="/{" end="}/"
syn region sisu_underline contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_fontface start="+{" end="}+"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_fontface start="\^{" end="}\^"
syn region sisu_identifier contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_fontface start=",{" end="}," 
syn region sisu_strikeout contains=sisu_error matchgroup=sisu_fontface start="-{" end="}-" 
syn region sisu_control contains=sisu_error matchgroup=sisu_content_alt start="<b>" end="</b>" oneline
syn region sisu_control contains=sisu_error matchgroup=sisu_content_alt start="<em>" end="</em>" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_content_alt start="<u>" end="</u>" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_content_alt start="<i>" end="</i>" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_content_alt start="<ins>" end="</ins>" skip="\\\\\|\\'" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_content_alt start="<del>" end="</del>" oneline
syn region sisu_html contains=sisu_error contains=sisu_strikeout matchgroup=sisu_contain start="<a href=\".\{-}\">" end="</a>" oneline
"%   single words bold italicise etc. "workon
syn region sisu_control contains=sisu_error matchgroup=sisu_control start="\([ (]\|^\)\*[^\|{\n\~\\]"hs=e-1 end="\*"he=e-0 skip="[a-zA-Z0-9']" oneline
syn region sisu_identifier contains=sisu_error matchgroup=sisu_content_alt start="\([ ]\|^\)/[^{ \|\n\\]"hs=e-1 end="/\[ \.\]" skip="[a-zA-Z0-9']" oneline
"syn region sisu_underline matchgroup=sisu_underline start="\([ (]\|^\)_\([^ !*{\\][\w]\|[^12][^*]\)"hs=e-2 end="\(_\([ )\.]\|$\)\| \)"he=e-1 skip="[a-zA-Z0-9']" oneline
"%   html
syn region sisu_number contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell,sisu_mark matchgroup=sisu_contain start="<b>" end="</b>" skip="\n"
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell,sisu_mark matchgroup=sisu_contain start="<i>" end="</i>" skip="\n"
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell,sisu_mark matchgroup=sisu_contain start="<u>" end="</u>" skip="\n"
"%   misc
syn region sisu_identifier contains=sisu_error matchgroup=sisu_content_alt start="\^[^{\|\n\\]" end="\^[ ,.;:'})\\\n]" skip="[a-zA-Z0-9']" oneline
"% metaverse
syn region sisu_content_alt contains=sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_contain start="<:Table.\{-}>" end="<:Table[-_]end>"
syn region sisu_content_alt contains=sisu_error matchgroup=sisu_contain start="<:code>" end="<:code[-_]end>"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_contain start="<:alt>" end="<:alt[-_]end>"
syn region sisu_content_alt contains=sisu_mark_endnote,sisu_content_endnote,sisu_link,sisu_mark,sisu_strikeout,sisu_number,sisu_control,sisu_identifier,sisu_error,sisu_error_spell matchgroup=sisu_contain start="<:poem>" end="<:poem[-_]end>"
"% 6 Expensive Mode
" Expensive Mode:
if !exists("sisu_no_expensive")
else " not Expensive
  syn region  sisu_content_alt  matchgroup=sisu_control start="^\s*def\s" matchgroup=NONE end="[?!]\|\>" skip="\.\|\(::\)" oneline
endif " Expensive?
"% 5 Headers: and Headings (Document Instructions)
"syn match   sisu_header contains=sisu_error,sisu_error_wspace,sisu_mark "0\~.*"
syn match   sisu_comment contains=sisu_error "^% .*\|^%% .*"
syn match   sisu_control contains=sisu_error,sisu_error_wspace "4\~! \S\+"
syn region  sisu_markpara contains=sisu_error,sisu_error_wspace start="^=begin" end="^=end.*$"
"% 4 Errors?
syn match sisu_error_wspace contains=sisu_error_wspace "^\s\+"
syn match sisu_error_wspace contains=sisu_error_wspace "\s\s\+"
syn match sisu_error_wspace contains=sisu_error_wspace  " \s*$"
syn match sisu_error contains=sisu_error,sisu_error_wspace "[^ (}]http:\S\+"
syn match sisu_error contains=sisu_error "http:\S\+[}><]"
syn match sisu_error contains=sisu_error "\([!*/_\+,^]\){\([^(\}\1)]\)\{-}\n\n"
syn match sisu_error contains=sisu_error "^[\-\~]{[^{]\{-}\n\n"
syn match sisu_error contains=sisu_error "\s\+.{{"
syn match sisu_error contains=sisu_error "^\~\s*$"
syn match sisu_error contains=sisu_error "^[0-9]\~\s*$"
syn match sisu_error contains=sisu_error "^[0-9]\~\S\+\s*$"
syn match sisu_error contains=sisu_error "[^{]\~\^[^ \)]"
syn match sisu_error contains=sisu_error "\~\^\s\+\.\s*"
syn match sisu_error contains=sisu_error "[_/\*!^]{[ .,:;?><]*}[_/\*!^]"
syn match sisu_error contains=sisu_error "[^ (\"'(\[][_/\*!]{\|}[_/\*!][a-zA-Z0-9)\]\"']"
syn match sisu_error contains=sisu_error "<dir>"
syn match sisu_error contains=sisu_error,sisu_match,sisu_strikeout,sisu_contain,sisu_content_alt,sisu_mark,sisu_break,sisu_number "<[a-zA-Z\/]\+>"
syn match sisu_error  "/\?<\([biu]\)>[^(</\1>)]\{-}\n\n"
"% 3 Error Exceptions?
syn match sisu_control "\n\n" "contains=ALL
syn match sisu_control " //"
syn match sisu_error  "%{"
syn match sisu_error "<br>http:\S\+\|http:\S\+<br>"
syn match sisu_error "[><]http:\S\+\|http:\S\+[><]"
"% 2 Definitions - Define the default highlighting.
if version >= 508 || !exists("did_sisu_syntax_inits")
  if version < 508
    let did_sisu_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
"% 1 Defined
  HiLink sisu_header          PreProc
  HiLink sisu_heading         Title
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
  HiLink sisu_digest          Identifier
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
  HiLink sisu_error_spell     SpellErrors "line does nothing
  HiLink sisu_error_wspace    Error
  HiLink sisu_error           Error
  "HiLink sisu_                Statement
  "HiLink sisu_                Operator
  delcommand HiLink
endif
let b:current_syntax = "sisu"
