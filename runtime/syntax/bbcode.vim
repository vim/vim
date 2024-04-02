" Vim syntax file
" Language:		BBCode
" Maintainer:		Quinn Johnson <winseven4lyf@gmail.com>
" Repository:		https://github.com/Winseven4lyf/vim-bbcode
" Last Change:		2019 April 2
" Credits:		Jorge Maldonado Ventura <jorgesumle@freakspot.net> and Claudio Fleiner <claudio@fleiner.com>, maintainers of the HTML syntax file included with Vim.
" Acknowledgements:	Tom Link <micathom@gmail.com>, creator of the bbcode script on vim.org, which isn't available via Git, making it difficult to use with Dein.

if !exists("main_syntax")
	if exists("b:current_syntax")
		finish
	endif
	let main_syntax = "bbcode"
endif

syn spell toplevel
syn case ignore

" Tags
syn region	bbcodeString	contained start=+"+ end=+"+
syn region	bbcodeString	contained start=+'+ end=+'+
hi def link	bbcodeString	String
syn match	bbcodeValue	contained +=[^'" \]]*+hs=s+1
hi def link	bbcodeValue	String
syn region	bbcodeEndTag	start=+\[/+ end=+\]+		contains=bbcodeTagN,bbcodeTagError,@NoSpell
hi def link	bbcodeEndTag	Identifier
syn region	bbcodeTag	start=+\[[^/]+ end=+\]+		fold contains=bbcodeTagN,bbcodeTagError,bbcodeValue,@NoSpell,bbcodeString,bbcodeArg
syn match	bbcodeTag	+^\s*\*+
hi def link	bbcodeTag	Function
syn match	bbcodeTagN	contained +\[\h\++hs=s+1	contains=bbcodeTagName
syn match	bbcodeTagN	contained +\[/\h\++hs=s+2	contains=bbcodeTagName
hi def link	bbcodeTagN	Statement
syn match	bbcodeTagError	contained +[^\]]\[+ms=s+1
hi def link	bbcodeTagError	Error
syn keyword	bbcodeTagName	contained b u i color size list url email img attachment
hi def link	bbcodeTagName	Statement
syn match	bbcodeArg	contained +\h\+=+me=e-1
hi def link	bbcodeArg	Type

" Tokens, used when creating custom BBCodes.
syn match	bbcodeToken		+{\h\+}+ contains=bbcodeTokenName,bbcodeTokenLocalized
hi def link	bbcodeToken		PreProc
syn keyword	bbcodeTokenName		contained TEXT SIMPLETEXT INTTEXT IDENTIFIER NUMBER EMAIL URL LOCAL_URL RELATIVE_URL COLOR
hi def link	bbcodeTokenName		Identifier
syn match	bbcodeTokenLocalized	contained +L_\w\++
hi def link	bbcodeTokenLocalized	Identifier

" Rendering
syn cluster	bbcodeTop	contains=@Spell,bbcodeTag,bbcodeEndTag,bbcodeLink,bbcodeToken

syn region	bbcodeBold			start=+\[b\>+ end=+\[/b\]+me=e-4		contains=@bbcodeTop,bbcodeBoldUnderline,bbcodeBoldItalic
syn region	bbcodeBold			start=+\[th\>+ end=+\[/th\]+me=e-5		contains=@bbcodeTop,bbcodeBoldUnderline,bbcodeBoldItalic
hi def		bbcodeBold			term=bold cterm=bold gui=bold
syn region	bbcodeBoldUnderline		contained start=+\[u\>+ end=+\[/u\]+me=e-4	contains=@bbcodeTop,bbcodeBoldUnderlineItalic
hi def		bbcodeBoldUnderline		term=bold,underline cterm=bold,underline gui=bold,underline
syn region	bbcodeBoldUnderlineItalic	contained start=+\[i\>+ end=+\[/i\]+me=e-4	contains=@bbcodeTop
hi def		bbcodeBoldUnderlineItalic	term=bold,underline,italic cterm=bold,underline,italic gui=bold,underline,italic
syn region	bbcodeBoldItalic		contained start=+\[i\>+ end=+\[/i\]+me=e-4	contains=@bbcodeTop,bbcodeBoldItalicUnderline
hi def		bbcodeBoldItalic		term=bold,italic cterm=bold,italic gui=bold,italic
syn region	bbcodeBoldItalicUnderline	contained start=+\[u\>+ end=+\[/u\]+me=e-4	contains=@bbcodeTop
hi def link	bbcodeBoldItalicUnderline	bbcodeBoldUnderlineItalic

syn region	bbcodeUnderline			start=+\[u\>+ end=+\[/u\]+me=e-4		contains=@bbcodeTop,bbcodeUnderlineBold,bbcodeUnderlineItalic
hi def		bbcodeUnderline			term=underline cterm=underline gui=underline
syn region	bbcodeUnderlineBold		contained start=+\[b\>+ end=+\[/b\]+me=e-4	contains=@bbcodeTop,bbcodeUnderlineBoldItalic
hi def link	bbcodeUnderlineBold		bbcodeBoldUnderline
syn region	bbcodeUnderlineBoldItalic	contained start=+\[i\>+ end=+\[/i\]+me=e-4	contains=@bbcodeTop
hi def link	bbcodeUnderlineBoldItalic	bbcodeBoldUnderlineItalic
syn region	bbcodeUnderlineItalic		contained start=+\[i\>+ end=+\[/i\]+me=e-4	contains=@bbcodeTop,bbcodeUnderlineItalicBold
hi def		bbcodeUnderlineItalic		term=underline,italic cterm=underline,italic gui=underline,italic
syn region	bbcodeUnderlineItalicBold	contained start=+\[b\>+ end=+\[/b\]+me=e-4	contains=@bbcodeTop
hi def link	bbcodeUnderlineItalicBold	bbcodeBoldUnderlineItalic

syn region	bbcodeItalic			start=+\[i\>+ end=+\[/i\]+me=e-4		contains=@bbcodeTop,bbcodeItalicBold,bbcodeItalicUnderline
hi def		bbcodeItalic			term=italic cterm=italic gui=italic
syn region	bbcodeItalicBold		contained start=+\[b\>+ end=+\[/b\]+me=e-4	contains=@bbcodeTop,bbcodeItalicBoldUnderline
hi def link	bbcodeItalicBold		bbcodeBoldItalic
syn region	bbcodeItalicBoldUnderline	contained start=+\[u\>+ end=+\[/u\]+me=e-4	contains=@bbcodeTop
hi def link	bbcodeItalicBoldUnderline	bbcodeBoldUnderlineItalic
syn region	bbcodeItalicUnderline		contained start=+\[u\>+ end=+\[/u\]+me=e-4	contains=@bbcodeTop,bbcodeItalicUnderlineBold
hi def link	bbcodeItalicUnderline		bbcodeUnderlineItalic
syn region	bbcodeItalicUnderlineBold	contained start=+\[b\>+ end=+\[/u\]+me=e-4	contains=@bbcodeTop
hi def link	bbcodeItalicUnderlineBold	bbcodeBoldUnderlineItalic

syn region	bbcodeCode			start=+\[code\>+ end=+\[/code\]+me=e-7		contains=@NoSpell
syn region	bbcodePre			start=+\[pre\>+ end=+\[/pre\]+me=e-6		contains=@NoSpell

syn match	bbcodeLeadingSpace		+^\s\++ contained
hi def link	bbcodeLeadingSpace		None
syn region	bbcodeLink			start=+\[url\]+ end=+\[/url\]+me=e-6		contains=@NoSpell,bbcodeTag,bbcodeEndTag,bbcodeLeadingSpace
syn region	bbcodeLink			start=+\[url=.\+\]+ end=+\[/url\]+me=e-6	contains=@Spell,bbcodeTag,bbcodeEndTag,bbcodeLeadingSpace
syn region	bbcodeLink			start=+\[img\>+ end=+\[/img\]+me=e-6		contains=@NoSpell,bbcodeTag,bbcodeEndTag,bbcodeLeadingSpace
syn region	bbcodeLink			start=+\[youtube\>+ end=+\[/youtube\]+me=e-10	contains=@NoSpell,bbcodeTag,bbcodeEndTag,bbcodeLeadingSpace
syn region	bbcodeLink			start=+\[flash\>+ end=+\[/flash\]+me=e-8	contains=@NoSpell,bbcodeTag,bbcodeEndTag,bbcodeLeadingSpace
hi def link	bbcodeLink			Underlined

if main_syntax == "bbcode"
	syn sync match	bbcodeHighlight		groupthere NONE +\[[/\h]+
	syn sync match	bbcodeHighlightSkip	"^.*['\"].*$"
	syn sync minlines=10
endif

let b:current_syntax = "bbcode"

if main_syntax == "bbcode"
	unlet main_syntax
endif
