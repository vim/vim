" Vim syntax file
" Language:	Icewm Menu
" Maintainer:	James Mahler <jmahler@purdue.edu>
" Last Change:	Tue Dec  9 21:08:22 EST 2003
" Extensions:	~/.icewm/menu
" Comment:	Icewm is a lightweight window manager.  This adds syntax
"		highlighting when editing your user's menu file (~/.icewm/menu).

" clear existing syntax
if version < 600
	syntax clear
elseif exists("bLcurrent_syntax")
	finish
endif

" not case sensitive
syntax case ignore

" icons .xpm .png and .gif
syntax match _icon /"\=\/.*\.xpm"\=/
syntax match _icon /"\=\/.*\.png"\=/
syntax match _icon /"\=\/.*\.gif"\=/
syntax match _icon /"\-"/

" separator
syntax keyword _rules separator

" prog and menu
syntax keyword _ids menu prog

" highlights
highlight link _rules Underlined
highlight link _ids Type
highlight link _icon Special

let b:current_syntax = "IceMenu"
