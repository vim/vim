" Vim syntax file
" Language:	    Eterm configuration file
" Maintainer:	    Nikolai Weibull <source@pcppopper.org>
" URL:		    http://www.pcppopper.org/vim/syntax/pcp/eterm/
" Latest Revision:  2004-05-06
" arch-tag:	    f4c58caf-2b91-4fc4-96af-e3cad7c70e6b

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" magic number
syn match   etermMagic		display "^<Eterm-[0-9.]\+>$"

" comments
syn region  etermComment	matchgroup=etermComment start="^#" end="$" contains=etermTodo

" todo
syn keyword etermTodo		contained TODO FIXME XXX NOTE

" numbers
syn match   etermNumber		contained display "\<\(\d\+\|0x\x\{1,2}\)\>"

" strings
syn region  etermString		contained display oneline start=+"+ skip=+\\"+ end=+"+

" booleans
syn keyword etermBoolean	contained on off true false yes no

" colors (not pretty, but can't figure out better way...)
syn match   etermColor		contained display "\s\+#\x\{6}\>"
syn keyword etermColor		contained white black

" preproc
syn match   etermPreProc	contained "%\(appname\|exec\|get\|put\|random\|version\|include\|preproc\)("he=e-1

" functions
syn match   etermFunctions	contained "\<\(copy\|exit\|kill\|nop\|paste\|save\|scroll\|search\|spawn\)("

" and make it easy to refer to the above...
syn cluster etermGeneral	contains=etermComment,etermNumber,etermString,etermBoolean,etermColor,etermFunction,etermPreProc

" key modifiers
syn keyword etermKeyMod		contained ctrl shift lock mod1 mod2 mod3 mod4 mod5 alt meta anymod
syn keyword etermKeyMod		contained button1 button2 button3 button4 button5

" color context
syn region  etermColorOptions	contained oneline matchgroup=etermOption start="^\s*video\>" matchgroup=etermType end="\<\(normal\|reverse\)\>"
syn region  etermColorOptions	contained oneline matchgroup=etermOption start="^\s*color\>" matchgroup=etermType end="\<\(bd\|ul\|[0-9]\|1[0-5]\)\>"
syn keyword etermColorOptions	contained foreground background cursor cursor_text pointer

syn region  etermColorContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+color\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermColorOptions

" attributes context
syn region  etermAttrOptions	contained oneline matchgroup=etermOption start="^\s*geometry\>" matchgroup=etermType end="\<\d\+x\d\++\d\++\d\+\>"
syn region  etermAttrOptions	contained oneline matchgroup=etermOption start="^\s*scrollbar_type\>" matchgroup=etermType end="\<\(motif\|xterm\|next\)\>"
syn region  etermAttrOptions	contained oneline matchgroup=etermOption start="^\s*font\>" matchgroup=etermType end="\<\(bold\|default\|proportional\|fx\|[0-5]\)\>"
syn keyword etermAttrOptions	contained title name iconname desktop scrollbar_width

syn region  etermAttrContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+attributes\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermAttrOptions

" image context
" image types
syn keyword etermImageTypes	contained background trough anchor up_arrow
syn keyword etermImageTypes	contained left_arrow right_arrow menu menuitem
syn keyword etermImageTypes	contained submenu button buttonbar down_arrow
syn region  etermImageOptions	contained transparent oneline matchgroup=etermOption start="^\s*type\>" end="$" contains=etermImageTypes
" image modes
syn keyword etermImageModes	contained image trans viewport auto solid
syn keyword etermImageModesAllow contained allow
syn region  etermImageOptions	contained transparent oneline matchgroup=etermOption start="^\s*mode\>" end="$" contains=etermImageModes,etermImageModesAllow
" image states
syn region  etermImageOptions	contained transparent oneline matchgroup=etermOption start="^\s*state\>" matchgroup=etermType end="\<\(normal\|selected\|clicked\|disabled\)\>"
" image geometry
syn region  etermImageOptions	contained transparent oneline matchgroup=etermOption start="^\s*geom\>" matchgroup=etermType end="\s\+\(\d\+x\d\++\d\++\d\+\)\=:\(\(tile\|scale\|hscale\|vscale\|propscale\)d\=\)\="
" image color modification
syn region  etermImageOptions	contained transparent oneline matchgroup=etermOption start="^\s*\(cmod\|colormod\)\>" matchgroup=etermType end="\<\(image\|red\|green\|blue\)\>"
" other keywords
syn keyword etermImageOptions	contained file padding border bevel color

syn region  etermImageContext	contained transparent fold matchgroup=etermContext start="^\s*begin\s\+image\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermImageOptions

" imageclasses context
syn keyword etermIClassOptions	contained icon cache path anim

syn region  etermIClassContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+imageclasses\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermImageContext,etermIClassOptions

" menuitem context
syn region  etermMenuItemOptions contained transparent oneline matchgroup=etermOption start="^\s*action\>" matchgroup=etermType end="\<string\|echo\|submenu\|script\|separator\>"
syn keyword etermMenuItemOptions contained text rtext

syn region  etermMenuItemContext fold transparent matchgroup=etermContext start="^\s*begin\s\+menuitem\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermMenuItemOptions

" menu context (should contain - as well, but no...)
syn keyword etermMenuOptions    contained title font_name sep

syn region  etermMenuContext    fold transparent  matchgroup=etermContext start="^\s*begin\s\+menu\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermMenuOptions,etermMenuItemContext

" action context
syn match   etermActionDef	contained "\<\(to\|string\|echo\|menu\|script\)\>"
syn region  etermActionsOptions	contained transparent oneline matchgroup=etermOption start="^\s*bind\>" end="$" contains=etermActionDef,etermKeyMod

syn region  etermActionsContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+actions\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermActionsOptions

" button bar context
syn match   etermButtonDef	contained "\<\(action\|string\|echo\|menu\|scrupt\)\>"
syn region  etermButtonOptions	contained transparent oneline matchgroup=etermOption start="^\s*button\>" end="$" contains=etermButtonDef
syn keyword etermButtonOptions	contained font visible dock

syn region  etermButtonContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+button_bar\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermButtonOptions

" multichar context
syn keyword etermMultiOptions	contained encoding font

syn region  etermMultiContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+multichar\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermMultiOptions

" xim context
syn keyword etermXimOptions     contained input_method preedit_type

syn region  etermXimContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+xim\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermXimOptions

" toggles context
syn keyword etermTogOptions	contained map_alert visual_bell login_shell scrollbar utmp_logging meta8 iconic no_input
syn keyword etermTogOptions	contained home_on_output home_on_input scrollbar_floating scrollbar_right scrollbar_popup
syn keyword etermTogOptions	contained borderless double_buffer no_cursor pause xterm_select select_line
syn keyword etermTogOptions	contained select_trailing_spaces report_as_keysyms itrans immotile_trans buttonbar
syn keyword etermTogOptions	contained resize_gravity

syn region  etermTogContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+toggles\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermTogOptions

" keyboard context
syn keyword etermKeyboardOptions contained smallfont_key bigfont_key keysym meta_mod alt_mod
syn keyword etermKeyboardOptions contained greek numlock_mod app_keypad app_cursor

syn region  etermKeyboardContext fold transparent  matchgroup=etermContext start="^\s*begin\s\+keyboard\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermKeyboardOptions

" misc context
syn keyword etermMiscOptions	contained print_pipe save_lines cut_chars min_anchor_size
syn keyword etermMiscOptions	contained border_width line_space finished_title term_name
syn keyword etermMiscOptions	contained finished_text exec

syn region  etermMiscContext	fold transparent  matchgroup=etermContext start="^\s*begin\s\+misc\s*$" end="^\s*end\>\(\s\+.\{-0,}\)\=$" contains=@etermGeneral,etermMiscOptions

if exists("eterm_minlines")
  let b:eterm_minlines = eterm_minlines
else
  let b:eterm_minlines = 30
endif
exec "syn sync minlines=" . b:eterm_minlines

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_eterm_syn_inits")
  if version < 508
    let did_eterm_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink etermMagic		Special
  HiLink etermComment		Comment
  HiLink etermTodo		Todo
  HiLink etermNumber		Number
  HiLink etermString		String
  HiLink etermBoolean		Boolean
  HiLink etermColor		Number
  HiLink etermPreProc		PreProc
  HiLink etermFunctions    	Function
  HiLink etermKeyMod		Special
  HiLink etermContext		Keyword
  HiLink etermOption		Keyword
  HiLink etermType		Type
  HiLink etermColorOptions	Keyword
  HiLink etermAttrOptions	Keyword
  HiLink etermIClassOptions	Keyword
  HiLink etermImageTypes	Type
  HiLink etermImageModes	Type
  HiLink etermImageModesAllow	Keyword
  HiLink etermImageOptions	Keyword
  HiLink etermMenuOptions	Keyword
  HiLink etermMenuItemOptions	Keyword
  HiLink etermActionDef	Type
  HiLink etermActionsOptions	Keyword
  HiLink etermButtonDef	Type
  HiLink etermButtonOptions	Keyword
  HiLink etermMultiOptions	Keyword
  HiLink etermXimOptions	Keyword
  HiLink etermTogOptions	Keyword
  HiLink etermKeyboardOptions	Keyword
  HiLink etermMiscOptions	Keyword
  delcommand HiLink
endif

let b:current_syntax = "eterm"

" vim: set sts=2 sw=2:
