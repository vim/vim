" Vim syntax file
" Language:     Mathematica
" Maintainer:   steve layland <layland@wolfram.com>
" Last Change:  Tue May 10 18:31:00 CDT 2005
" Source:       http://vim.sourceforge.net/scripts/script.php?script_id=1273
"               http://members.wri.com/layland/vim/syntax/mma.vim
"
" NOTE:
" Empty .m files will automatically be presumed as Matlab files
" unless you have the following in your .vimrc:
"
"       let filetype_m="mma"
"
" I also recommend setting the default 'Comment' hilighting to something
" other than the color used for 'Function', since both are plentiful in
" most mathematica files, and they are often the same color (when using 
" background=dark).
"
" Credits:
" o  Original Mathematica syntax version written by
"    Wolfgang Waltenberger <wwalten@ben.tuwien.ac.at>
" o  Some ideas like the CommentStar,CommentTitle were adapted
"    from the Java vim syntax file by Claudio Fleiner.  Thanks!
" o  Everything else written by steve <layland@wolfram.com>
"
" TODO:
"   folding
"   fix nesting
"   finish populating popular symbols

if version < 600
	syntax clear
elseif exists("b:current_syntax")
    finish
endif

" Group Definitions:
syntax cluster mmaNotes contains=mmaTodo,mmaFixme
syntax cluster mmaComments contains=mmaComment,mmaFunctionComment,mmaItem,mmaFunctionTitle,mmaCommentStar
syntax cluster mmaCommentStrings contains=mmaLooseQuote,mmaCommentString,mmaUnicode
syntax cluster mmaStrings contains=@mmaCommentStrings,mmaString
syntax cluster mmaTop contains=mmaOperator,mmaGenericFunction,mmaPureFunction,mmaVariable

" Predefined Constants:
"   to list all predefined Symbols would be too insane...
"   it's probably smarter to define a select few, and get the rest from
"   context if absolutely necessary.
"   TODO - populate this with other often used Symbols

" standard fixed symbols:
syntax keyword mmaVariable True False None Automatic All Null C General

" mathematical constants:
syntax keyword mmaVariable Pi I E Infinity ComplexInfinity Indeterminate GoldenRatio EulerGamma Degree Catalan Khinchin Glaisher 

" stream data / atomic heads:
syntax keyword mmaVariable Byte Character Expression Number Real String Word EndOfFile Integer Symbol

" sets:
syntax keyword mmaVariable Integers Complexes Reals Booleans Rationals

" character classes:
syntax keyword mmaPattern DigitCharacter LetterCharacter WhitespaceCharacter WordCharacter EndOfString StartOfString EndOfLine StartOfLine WordBoundary

" SelectionMove directions/units:
syntax keyword mmaVariable Next Previous After Before Character Word Expression TextLine CellContents Cell CellGroup EvaluationCell ButtonCell GeneratedCell Notebook
syntax keyword mmaVariable CellTags CellStyle CellLabel

" TableForm positions:
syntax keyword mmaVariable Above Below Left Right

" colors:
syntax keyword mmaVariable Black Blue Brown Cyan Gray Green Magenta Orange Pink Purple Red White Yellow

" function attributes
syntax keyword mmaVariable Protected Listable OneIdentity Orderless Flat Constant NumericFunction Locked ReadProtected HoldFirst HoldRest HoldAll HoldAllComplete SequenceHold NHoldFirst NHoldRest NHoldAll Temporary Stub 

" Comment Sections:
"   this:
"   :that:
syntax match mmaItem "\%(^[( |*\t]*\)\@<=\%(:\+\|\a\)[a-zA-Z0-9 ]\+:" contained contains=@mmaNotes

" Comment Keywords:
syntax keyword mmaTodo TODO NOTE HEY contained
syntax match mmaTodo "X\{3,}" contained
syntax keyword mmaFixme FIX[ME] FIXTHIS BROKEN contained
" yay pirates...
syntax match mmaFixme "\%(Y\=A\+R\+G\+\|GRR\+\|CR\+A\+P\+\)\%(!\+\)\=" contained

" EmPHAsis:
" this unnecessary, but whatever :)
syntax match mmaemPHAsis "\%(^\|\s\)\([_/]\)[a-zA-Z0-9]\+\%(\s\+[a-zA-Z0-9]\+\)*\1\%(\s\|$\)" contained contains=mmaemPHAsis
syntax match mmaemPHAsis "\%(^\|\s\)(\@<!\*[a-zA-Z0-9]\+\%(\s\+[a-zA-Z0-9]\+\)*)\@!\*\%(\s\|$\)" contained contains=mmaemPHAsis

" Regular Comments:
"   (* *)
"   allow nesting (* (* *) *) even though the frontend
"   won't always like it.
syntax region mmaComment start=+(\*+ end=+\*)+ skipempty contains=@mmaNotes,mmaItem,@mmaCommentStrings,mmaemPHAsis,mmaComment

" Function Comments:
"   just like a normal comment except the first sentance is Special ala Java
"   (** *)
"   TODO - fix this for nesting, or not...
syntax region mmaFunctionComment start="(\*\*\+" end="\*\+)" contains=@mmaNotes,mmaItem,mmaFunctionTitle,@mmaCommentStrings,mmaemPHAsis,mmaComment
syntax region mmaFunctionTitle contained matchgroup=mmaFunctionComment start="\%((\*\*[ *]*\)" matchgroup=mmaFunctionTitle keepend end=".[.!-]\=\s*$" end="[.!-][ \t\r<&]"me=e-1 end="\%(\*\+)\)\@=" contained contains=@mmaNotes,mmaItem,mmaCommentStar

" catch remaining (**********)'s
syntax match mmaComment "(\*\*\+)"
" catch preceding *
syntax match mmaCommentStar "^\s*\*\+" contained

" Variables:
"   Dollar sign variables
syntax match mmaVariable "$\a\+\d*"
"   Preceding contexts
syntax match mmaVariable "`\=\a\+\d*`"

" Strings:
"   "string"
"   'string' is not accepted (until literal strings are supported!)
syntax region mmaString start=+\\\@<!"+ skip=+\\\@<!\\\%(\\\\\)*"+ end=+"+
syntax region mmaCommentString oneline start=+\\\@<!"+ skip=+\\\@<!\\\%(\\\\\)*"+ end=+"+ contained


" Patterns:
"   Each pattern marker below can be Blank[] (_), BlankSequence[] (__)
"   or BlankNullSequence[] (___).  Most examples below can also be 
"   combined, for example Pattern tests with Default values.
"   
"   _Head                   Anonymous patterns
"   name_Head 
"   name:(_Head|_Head2)     Named patterns
"    
"   _Head : val
"   name:_Head:val          Default values
"
"   _Head?testQ, 
"   _Head?(test[#]&)        Pattern tests
"
"   name_Head/;test[name]   Conditionals
"   
"   _Head:.                 Predefined Default
"
"   .. ...                  Pattern Repeat
   
syntax match mmaPatternError "\%(_\{4,}\|)\s*&\s*)\@!\)" contained

"pattern name:
syntax match mmaPattern "[A-Za-z0-9`]\+\s*:\+[=>]\@!" contains=mmaOperator
"pattern default:
syntax match mmaPattern ": *[^ ,]\+[\], ]\@=" contains=@mmaCommentStrings,@mmaTop,mmaOperator
"pattern head/test:
syntax match mmaPattern "[A-Za-z0-9`]*_\+\%(\a\+\)\=\%(?([^)]\+)\|?[^\]},]\+\)\=" contains=@mmaTop,@mmaCommentStrings,mmaPatternError

" Operators:
"   /: ^= ^:=   UpValue
"   /;          Conditional
"   := =        DownValue
"   == === ||
"   != =!= &&   Logic
"   >= <= < >
"   += -= *=
"   /= ++ --    Math
"   ^* 
"   -> :>       Rules
"   @@ @@@      Apply
"   /@ //@      Map
"   /. //.      Replace
"   // @        Function application
"   <> ~~       String/Pattern join
"   ~           infix operator
"   . :         Pattern operators
syntax match mmaOperator "\%(@\{1,3}\|//[.@]\=\)"
syntax match mmaOperator "\%(/[;:@.]\=\|\^\=:\==\)"
syntax match mmaOperator "\%([-:=]\=>\|<=\=\)"
"syntax match mmaOperator "\%(++\=\|--\=\|[/+-*]=\|[^*]\)"
syntax match mmaOperator "[*+=^.:?-]"
syntax match mmaOperator "\%(\~\~\=\)"
syntax match mmaOperator "\%(=\{2,3}\|=\=!=\|||\=\|&&\|!\)" contains=ALLBUT,mmaPureFunction

" Function Usage Messages:
"   "SymbolName::item"
syntax match mmaMessage "$\=\a\+\d*::\a\+\d*"

" Pure Functions:
syntax match mmaPureFunction "#\%(#\|\d\+\)\="
syntax match mmaPureFunction "&"

" Named Functions:
" Since everything is pretty much a function, get this straight 
" from context
syntax match mmaGenericFunction "[A-Za-z0-9`]\+\s*\%([@[]\|/:\|/\=/@\)\@=" contains=mmaOperator
syntax match mmaGenericFunction "\~\s*[^~]\+\s*\~"hs=s+1,he=e-1 contains=mmaOperator,mmaBoring
syntax match mmaGenericFunction "//\s*[A-Za-z0-9`]\+"hs=s+2 contains=mmaOperator

" Numbers:
syntax match mmaNumber "\<\%(\d\+\.\=\d*\|\d*\.\=\d\+\)\>"
syntax match mmaNumber "`\d\+\%(\d\@!\.\|\>\)"

" Special Characters:
"   \[Name]     named character
"   \ooo        octal
"   \.xx        2 digit hex
"   \:xxxx      4 digit hex (multibyte unicode)
syntax match mmaUnicode "\\\[\w\+\d*\]"
syntax match mmaUnicode "\\\%(\x\{3}\|\.\x\{2}\|:\x\{4}\)"

" Syntax Errors:
syntax match mmaError "\*)" containedin=ALLBUT,@mmaComments,@mmaStrings
syntax match mmaError "\%([&:|+*/?~-]\{3,}\|[.=]\{4,}\|_\@<=\.\{2,}\|`\{2,}\)" containedin=ALLBUT,@mmaComments,@mmaStrings

" Punctuation:
" things that shouldn't really be highlighted, or highlighted 
" in they're own group if you _really_ want. :)
"  ( ) { }
" TODO - use Delimiter group?
syntax match mmaBoring "[(){}]" contained

" Function Arguments:
"   anything between brackets []
"   TODO - make good folds for this.
"syntax region mmaArgument start="\[" end="]" containedin=ALLBUT,@mmaComments,@mmaCommentStrings transparent fold
"syntax sync fromstart
"set foldmethod=syntax
"set foldminlines=10

if version >= 508 || !exists("did_mma_syn_inits")
	if version < 508
		let did_mma_syn_inits = 1
		command -nargs=+ HiLink hi link <args>
	else
		command -nargs=+ HiLink hi def link <args>
	endif

    " NOTE - the following links are not guaranteed to 
    " look good under all colorschemes.  You might need to 
    " :so $VIMRUNTIME/syntax/hitest.vim and tweak these to
    " look good in yours

    HiLink mmaComment           Comment
    HiLink mmaCommentStar       Comment
    HiLink mmaFunctionComment   Comment
    HiLink mmaLooseQuote        Comment
	HiLink mmaGenericFunction   Function
	HiLink mmaVariable          Identifier
	HiLink mmaOperator          Operator
    HiLink mmaPatternOp         Operator
	HiLink mmaPureFunction      Operator
	HiLink mmaString            String
    HiLink mmaCommentString     String
	HiLink mmaUnicode           String
	HiLink mmaMessage           Type
    HiLink mmaNumber            Type
	HiLink mmaPattern           Type
	HiLink mmaError             Error
	HiLink mmaFixme             Error
    HiLink mmaPatternError      Error
    HiLink mmaTodo              Todo
    HiLink mmaemPHAsis          Special
    HiLink mmaFunctionTitle     Special
    HiLink mmaItem              Preproc

	delcommand HiLink
endif

let b:current_syntax = "mma"
