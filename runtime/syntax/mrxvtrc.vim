" Created	: Wed 26 Apr 2006 01:20:53 AM CDT
" Modified	: Thu 27 Apr 2006 02:29:25 PM CDT
" Author	: Gautam Iyer <gi1242@users.sourceforge.net>
" Description	: Syntax file for mrxvtrc

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Define options.
let s:boolOpts = '(highlightTabOnBell|syncTabTitle|hideTabbar|autohideTabbar|bottomTabbar|hideButtons|syncTabIcon|veryBoldFont|maximized|fullscreen|reverseVideo|loginShell|jumpScroll|scrollBar|scrollbarRight|scrollbarFloating|scrollTtyOutputInhibit|scrollTtyKeypress|scrollWithBuffer|transparent|transparentForce|transparentScrollbar|transparentMenubar|transparentTabbar|tabUsePixmap|utmpInhibit|visualBell|mapAlert|meta8|mouseWheelScrollPage|multibyte_cursor|tripleclickwords|showMenu|xft|xftNomFont|xftSlowOutput|xftAntialias|xftHinting|xftAutoHint|xftGlobalAdvance|tabShell|cmdAllTabs|cmdInitTabs|protectSecondary|thai|borderLess|overrideRedirect|holdExit|broadcast|smartResize|smoothResize|pointerBlank|cursorBlink|noSysConfig|disableMacros|linuxHomeEndKey|sessionMgt)'

let s:colorOpts = '(vt\d+.(foreground|background)|background|foreground|ufBackground|textShadow|i?tab(Foreground|Background)|color([0-9]|1[0-5]|BD|UL|RV)|(scroll|trough|highlight|cursor|pointer|border|tint)Color|cursorColor2)'

let s:numOpts = '(vt\d+.saveLines|maxTabWidth|minVisibleTabs|saveLines|scrollbarThickness|xftmSize|xftSize|desktop|externalBorder|internalBorder|lineSpace|pointerBlankDelay|cursorBlinkInterval|initTermNumber|shading|backgroundFade|bgRefreshInterval|fading|opacity|opacityDegree|xftPSize)'

let s:strOpts = '(vt\d+\.(tabTitle|command)|tabTitle|termName|title|clientName|iconName|bellCommand|holdExitText|backspaceKey|deleteKey|printPipe|cutChars|answerbackString|smClientID|geometry|confFileSave|path|boldFont|m?font[1-5]?|xftFont|xftmFont|xftPFont|inputMethod|greektoggle_key|menu|menubarPixmap|vt\d+\.Pixmap|Pixmap|scrollbarPixmap|tabbarPixmap|appIcon|multichar_encoding)'

syn case match

syn match	mrxvtrcComment	contains=@Spell '^\s*!.*$'
syn match	mrxvtrcComment	'\v^\s*!\s*\w+[.*]\w+.*:.*'

"
" Generic options (string / color / number / boolean)
"
syn match	mrxvtrcOptErr	'\v^\s*\w+[.*]?.{-}(:|$)'
exec 'syn match	mrxvtrcBoolOpts	nextgroup=mrxvtrcBoolVal,mrxvtrcValErr'
	    \ '"\v^\w+[.*]'.s:boolOpts.':\s*"'
exec 'syn match	mrxvtrcNumOpts	nextgroup=mrxvtrcNumVal,mrxvtrcValErr'
	    \ '"\v^\w+[.*]'.s:numOpts.':\s*"'
exec 'syn match	mrxvtrcColorOpts	nextgroup=mrxvtrcColorVal'
	    \ '"\v^\w+[.*]'.s:colorOpts.':\s*"'
exec 'syn match	mrxvtrcStrOpts	nextgroup=mrxvtrcStrVal,mrxvtrcValErr'
	    \ '"\v^\w+[.*]'.s:strOpts.':\s*"'

syn case ignore

syn match	mrxvtrcValErr	contained '\v.+$'
syn keyword	mrxvtrcBoolVal	contained 0 1 yes no on off true false
syn match	mrxvtrcStrVal	contained '\v.+$'
syn match	mrxvtrcColorVal	contained '\v#[0-9a-f]{6}\s*$'
syn match	mrxvtrcNumVal	contained '\v[+-]?(0[0-7]+|\d+|0x[0-9a-f]+)$'

syn case match

"
" Options with special values
"
syn match	mrxvtrcOptions	nextgroup=mrxvtrcSBstyle,mrxvtrcValErr
				\ '\v^\w+[.*]scrollbarStyle:\s*'
syn keyword	mrxvtrcSBstyle	contained plain xterm rxvt next sgi

syn match	mrxvtrcOptions	nextgroup=mrxvtrcSBalign,mrxvtrcValErr
				\ '\v^\w+[.*]scrollbarAlign:\s*'
syn keyword	mrxvtrcSBalign	contained top bottom

syn match	mrxvtrcOptions	nextgroup=mrxvtrcTSmode,mrxvtrcValErr
				\ '\v^\w+[.*]textShadowMode:\s*'
syn keyword	mrxvtrcTSmode	contained
				\ none top bottom left right topleft topright
				\ botleft botright

syn match	mrxvtrcOptions	nextgroup=mrxvtrcGrkKbd,mrxvtrcValErr
				\ '\v^\w+[.*]greek_keyboard:\s*'
syn keyword	mrxvtrcGrkKbd	contained iso ibm

syn match	mrxvtrcOptions	nextgroup=mrxvtrcXftWt,mrxvtrcValErr
				\ '\v^\w+[.*]xftWeight:\s*'
syn keyword	mrxvtrcXftWt	contained light medium bold

syn match	mrxvtrcOptions	nextgroup=mrxvtrcXftSl,mrxvtrcValErr
				\ '\v^\w+[.*]xftSlant:\s*'
syn keyword	mrxvtrcXftSl	contained roman italic oblique

syn match	mrxvtrcOptions	nextgroup=mrxvtrcXftWd,mrxvtrcValErr
				\ '\v^\w+[.*]xftWidth:\s*'
syn keyword	mrxvtrcXftWd	contained
				\ ultracondensed ultraexpanded
				\ condensed expanded normal

syn match	mrxvtrcOptions	nextgroup=mrxvtrcXftHt,mrxvtrcValErr
				\ '\v^\w+[.*]xftRGBA:\s*'
syn keyword	mrxvtrcXftHt	contained rgb bgr vrgb vbgr none

syn match	mrxvtrcOptions	nextgroup=mrxvtrcPedit,mrxvtrcValErr
				\ '\v^\w+[.*]preeditType:\s*'
syn keyword	mrxvtrcPedit	contained OverTheSpot OffTheSpot Root

syn match	mrxvtrcOptions	nextgroup=mrxvtrcMod,mrxvtrcValErr
				\ '\v^\w+[.*]modifier:\s*'
syn keyword	mrxvtrcMod	contained
				\ alt meta hyper super mod1 mod2 mod3 mod4 mod5

syn match	mrxvtrcOptions	nextgroup=mrxvtrcSelSty,mrxvtrcValErr
				\ '\v^\w+[.*]selectStyle:\s*'
syn keyword	mrxvtrcSelSty	contained old oldword


"
" Macros
"
syn match	mrxvtrcOptions	nextgroup=mrxvtrcMacro,mrxvtrcValErr
	    \ '\v\c^\w+[.*]macro.(primary\+)?((ctrl|alt|meta|shift)\+)*\w+:\s*'
syn keyword	mrxvtrcMacro	contained nextgroup=mrxvtrcMacroArg
				\ Dummy Esc Str NewTab Close GotoTab MoveTab
				\ Scroll Copy Paste ToggleSubwin ResizeFont
				\ ToggleVeryBold ToggleTransparency
				\ ToggleBroadcast ToggleHold SetTitle
				\ PrintScreen SaveConfig ToggleMacros
syn match	mrxvtrcMacroArg	contained '.\+$'


unlet s:strOpts s:boolOpts s:colorOpts s:numOpts

"
" Highlighting groups
"
hi def link mrxvtrcComment	Comment

hi def link mrxvtrcBoolOpts	Statement
hi def link mrxvtrcColorOpts	mrxvtrcBoolOpts
hi def link mrxvtrcNumOpts	mrxvtrcBoolOpts
hi def link mrxvtrcStrOpts	mrxvtrcBoolOpts
hi def link mrxvtrcOptions	mrxvtrcBoolOpts

hi def link mrxvtrcBoolVal	Boolean
hi def link mrxvtrcStrVal	String
hi def link mrxvtrcColorVal	Constant
hi def link mrxvtrcNumVal	Number

hi def link mrxvtrcSBstyle	mrxvtrcStrVal
hi def link mrxvtrcSBalign	mrxvtrcStrVal
hi def link mrxvtrcTSmode	mrxvtrcStrVal
hi def link mrxvtrcGrkKbd	mrxvtrcStrVal
hi def link mrxvtrcXftWt	mrxvtrcStrVal
hi def link mrxvtrcXftSl	mrxvtrcStrVal
hi def link mrxvtrcXftWd	mrxvtrcStrVal
hi def link mrxvtrcXftHt	mrxvtrcStrVal
hi def link mrxvtrcPedit	mrxvtrcStrVal
hi def link mrxvtrcMod		mrxvtrcStrVal
hi def link mrxvtrcSelSty	mrxvtrcStrVal

hi def link mrxvtrcMacro	Identifier
hi def link mrxvtrcMacroArg	String

hi def link mrxvtrcOptErr	Error
hi def link mrxvtrcValErr	Error

let b:current_syntax = "mrxvtrc"
