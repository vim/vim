" Vim syntax file
" Language:	Perl POD format
" Maintainer:	Scott Bigham <dsb@cs.duke.edu>
" Last Change:	2001 May 09

" To add embedded POD documentation highlighting to your syntax file, add
" the commands:
"
"   syn include @Pod <sfile>:p:h/pod.vim
"   syn region myPOD start="^=pod" start="^=head" end="^=cut" keepend contained contains=@Pod
"
" and add myPod to the contains= list of some existing region, probably a
" comment.  The "keepend" flag is needed because "=cut" is matched as a
" pattern in its own right.


" Remove any old syntax stuff hanging around (this is suppressed
" automatically by ":syn include" if necessary).
" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" POD commands
syn match podCommand	"^=head[12]"	nextgroup=podCmdText
syn match podCommand	"^=item"	nextgroup=podCmdText
syn match podCommand	"^=over"	nextgroup=podOverIndent skipwhite
syn match podCommand	"^=back"
syn match podCommand	"^=cut"
syn match podCommand	"^=pod"
syn match podCommand	"^=for"		nextgroup=podForKeywd skipwhite
syn match podCommand	"^=begin"	nextgroup=podForKeywd skipwhite
syn match podCommand	"^=end"		nextgroup=podForKeywd skipwhite

" Text of a =head1, =head2 or =item command
syn match podCmdText	".*$" contained contains=podFormat

" Indent amount of =over command
syn match podOverIndent	"\d\+" contained

" Formatter identifier keyword for =for, =begin and =end commands
syn match podForKeywd	"\S\+" contained

" An indented line, to be displayed verbatim
syn match podVerbatimLine	"^\s.*$"

" Inline textual items handled specially by POD
syn match podSpecial	"\(\<\|&\)\I\i*\(::\I\i*\)*([^)]*)"
syn match podSpecial	"[$@%]\I\i*\(::\I\i*\)*\>"

" Special formatting sequences
syn region podFormat	start="[IBSCLFXEZ]<" end=">" oneline contains=podFormat

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_pod_syntax_inits")
  if version < 508
    let did_pod_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink podCommand		Statement
  HiLink podCmdText		String
  HiLink podOverIndent		Number
  HiLink podForKeywd		Identifier
  HiLink podFormat		Identifier
  HiLink podVerbatimLine	PreProc
  HiLink podSpecial		Identifier

  delcommand HiLink
endif

let b:current_syntax = "pod"

" vim: ts=8
