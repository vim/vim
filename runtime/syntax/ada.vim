" Vim syntax file
" Language:	Ada (95)
" Maintainer:	David A. Wheeler <dwheeler@dwheeler.com>
" URL: http://www.dwheeler.com/vim
" Last Change:	2001-11-02

" Former Maintainer:	Simon Bradley <simon.bradley@pitechnology.com>
"			(was <sib93@aber.ac.uk>)
" Other contributors: Preben Randhol.
" The formal spec of Ada95 (ARM) is the "Ada95 Reference Manual".
" For more Ada95 info, see http://www.gnuada.org and http://www.adapower.com.

" This vim syntax file works on vim 5.6, 5.7, 5.8 and 6.x.
" It implements Bram Moolenaar's April 25, 2001 recommendations to make
" the syntax file maximally portable across different versions of vim.
" If vim 6.0+ is available,
" this syntax file takes advantage of the vim 6.0 advanced pattern-matching
" functions to avoid highlighting uninteresting leading spaces in
" some expressions containing "with" and "use".

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
 syntax clear
elseif exists("b:current_syntax")
 finish
endif

" Ada is entirely case-insensitive.
syn case ignore

" We don't need to look backwards to highlight correctly;
" this speeds things up greatly.
syn sync minlines=1 maxlines=1

" Highlighting commands.  There are 69 reserved words in total in Ada95.
" Some keywords are used in more than one way. For example:
" 1. "end" is a general keyword, but "end if" ends a Conditional.
" 2. "then" is a conditional, but "and then" is an operator.


" Standard Exceptions (including I/O).
" We'll highlight the standard exceptions, similar to vim's Python mode.
" It's possible to redefine the standard exceptions as something else,
" but doing so is very bad practice, so simply highlighting them makes sense.
syn keyword adaException Constraint_Error Program_Error Storage_Error
syn keyword adaException Tasking_Error
syn keyword adaException Status_Error Mode_Error Name_Error Use_Error
syn keyword adaException Device_Error End_Error Data_Error Layout_Error
syn keyword adaException Length_Error Pattern_Error Index_Error
syn keyword adaException Translation_Error
syn keyword adaException Time_Error Argument_Error
syn keyword adaException Tag_Error
syn keyword adaException Picture_Error
" Interfaces
syn keyword adaException Terminator_Error Conversion_Error
syn keyword adaException Pointer_Error Dereference_Error Update_Error
" This isn't in the Ada spec, but a GNAT extension.
syn keyword adaException Assert_Failure
" We don't list ALL exceptions defined in particular compilers (e.g., GNAT),
" because it's quite reasonable to define those phrases as non-exceptions.


" We don't normally highlight types in package Standard
" (Integer, Character, Float, etc.).  I don't think it looks good
" with the other type keywords, and many Ada programs define
" so many of their own types that it looks inconsistent.
" However, if you want this highlighting, turn on "ada_standard_types".
" For package Standard's definition, see ARM section A.1.

if exists("ada_standard_types")
  syn keyword adaBuiltinType	Boolean Integer Natural Positive Float
  syn keyword adaBuiltinType	Character Wide_Character
  syn keyword adaBuiltinType	String Wide_String
  syn keyword adaBuiltinType	Duration
  " These aren't listed in ARM section A.1's code, but they're noted as
  " options in ARM sections 3.5.4 and 3.5.7:
  syn keyword adaBuiltinType	Short_Integer Short_Short_Integer
  syn keyword adaBuiltinType	Long_Integer Long_Long_Integer
  syn keyword adaBuiltinType	Short_Float Short_Short_Float
  syn keyword adaBuiltinType	Long_Float Long_Long_Float
endif

" There are MANY other predefined types; they've not been added, because
" determining when they're a type requires context in general.
" One potential addition would be Unbounded_String.


syn keyword adaLabel		others

syn keyword adaOperator		abs mod not rem xor
syn match adaOperator		"\<and\>"
syn match adaOperator		"\<and\s\+then\>"
syn match adaOperator		"\<or\>"
syn match adaOperator		"\<or\s\+else\>"
syn match adaOperator		"[-+*/<>&]"
syn keyword adaOperator		**
syn match adaOperator		"[/<>]="
syn keyword adaOperator		=>
syn match adaOperator		"\.\."
syn match adaOperator		"="

" We won't map "adaAssignment" by default, but we need to map ":=" to
" something or the "=" inside it will be mislabelled as an operator.
" Note that in Ada, assignment (:=) is not considered an operator.
syn match adaAssignment		":="

" Handle the box, <>, specially:
syn keyword adaSpecial	<>

" Numbers, including floating point, exponents, and alternate bases.
syn match   adaNumber		"\<\d[0-9_]*\(\.\d[0-9_]*\)\=\([Ee][+-]\=\d[0-9_]*\)\=\>"
syn match   adaNumber		"\<\d\d\=#\x[0-9A-Fa-f_]*\(\.\x[0-9A-Fa-f_]*\)\=#\([Ee][+-]\=\d[0-9_]*\)\="

" Identify leading numeric signs. In "A-5" the "-" is an operator,
" but in "A:=-5" the "-" is a sign. This handles "A3+-5" (etc.) correctly.
" This assumes that if you put a don't put a space after +/- when it's used
" as an operator, you won't put a space before it either -- which is true
" in code I've seen.
syn match adaSign "[[:space:]<>=(,|:;&*/+-][+-]\d"lc=1,hs=s+1,he=e-1,me=e-1

" Labels for the goto statement.
syn region  adaLabel		start="<<"  end=">>"

" Boolean Constants.
syn keyword adaBoolean	true false

" Warn people who try to use C/C++ notation erroneously:
syn match adaError "//"
syn match adaError "/\*"
syn match adaError "=="


if exists("ada_space_errors")
  if !exists("ada_no_trail_space_error")
    syn match   adaSpaceError     excludenl "\s\+$"
  endif
  if !exists("ada_no_tab_space_error")
    syn match   adaSpaceError     " \+\t"me=e-1
  endif
endif

" Unless special ("end loop", "end if", etc.), "end" marks the end of a
" begin, package, task etc. Assiging it to adaEnd.
syn match adaEnd		"\<end\>"

syn keyword adaPreproc		pragma

syn keyword adaRepeat		exit for loop reverse while
syn match adaRepeat		"\<end\s\+loop\>"

syn keyword adaStatement	accept delay goto raise requeue return
syn keyword adaStatement	terminate
syn match adaStatement	"\<abort\>"

" Handle Ada's record keywords.
" 'record' usually starts a structure, but "with null record;" does not,
" and 'end record;' ends a structure.  The ordering here is critical -
" 'record;' matches a "with null record", so make it a keyword (this can
" match when the 'with' or 'null' is on a previous line).
" We see the "end" in "end record" before the word record, so we match that
" pattern as adaStructure (and it won't match the "record;" pattern).
syn match adaStructure	"\<record\>"
syn match adaStructure	"\<end\s\+record\>"
syn match adaKeyword	"\<record;"me=e-1

syn keyword adaStorageClass	abstract access aliased array at constant delta
syn keyword adaStorageClass	digits limited of private range tagged
syn keyword adaTypedef		subtype type

" Conditionals. "abort" after "then" is a conditional of its own.
syn match adaConditional	"\<then\>"
syn match adaConditional	"\<then\s\+abort\>"
syn match adaConditional	"\<else\>"
syn match adaConditional	"\<end\s\+if\>"
syn match adaConditional	"\<end\s\+case\>"
syn match adaConditional	"\<end\s\+select\>"
syn keyword adaConditional	if case select
syn keyword adaConditional	elsif when

syn keyword adaKeyword		all do exception in is new null out
syn keyword adaKeyword		separate until

" These keywords begin various constructs, and you _might_ want to
" highlight them differently.
syn keyword adaBegin		begin body declare entry function generic
syn keyword adaBegin		package procedure protected renames task


if exists("ada_withuse_ordinary")
" Don't be fancy. Display "with" and "use" as ordinary keywords in all cases.
 syn keyword adaKeyword		with use
else
 " Highlight "with" and "use" clauses like C's "#include" when they're used
 " to reference other compilation units; otherwise they're ordinary keywords.
 " If we have vim 6.0 or later, we'll use its advanced pattern-matching
 " capabilities so that we won't match leading spaces.
 syn match adaKeyword	"\<with\>"
 syn match adaKeyword	"\<use\>"
 if version < 600
  syn match adaBeginWith "^\s*\(\(with\(\s\+type\)\=\)\|\(use\)\)\>" contains=adaInc
  syn match adaSemiWith	";\s*\(\(with\(\s\+type\)\=\)\|\(use\)\)\>"lc=1 contains=adaInc
 else
  syn match adaBeginWith "^\s*\zs\(\(with\(\s\+type\)\=\)\|\(use\)\)\>" contains=adaInc
  syn match adaSemiWith	";\s*\zs\(\(with\(\s\+type\)\=\)\|\(use\)\)\>" contains=adaInc
 endif
 syn match adaInc	"\<with\>" contained contains=NONE
 syn match adaInc	"\<with\s\+type\>" contained contains=NONE
 syn match adaInc	"\<use\>" contained contains=NONE
 " Recognize "with null record" as a keyword (even the "record").
 syn match adaKeyword	"\<with\s\+null\s\+record\>"
 " Consider generic formal parameters of subprograms and packages as keywords.
 if version < 600
  syn match adaKeyword	";\s*with\s\+\(function\|procedure\|package\)\>"
  syn match adaKeyword	"^\s*with\s\+\(function\|procedure\|package\)\>"
 else
  syn match adaKeyword	";\s*\zswith\s\+\(function\|procedure\|package\)\>"
  syn match adaKeyword	"^\s*\zswith\s\+\(function\|procedure\|package\)\>"
 endif
endif


" String and character constants.
syn region  adaString		start=+"+  skip=+""+  end=+"+
syn match   adaCharacter	"'.'"

" Todo (only highlighted in comments)
syn keyword adaTodo contained	TODO FIXME XXX

" Comments.
syn region  adaComment	oneline contains=adaTodo start="--"  end="$"



" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_ada_syn_inits")
  if version < 508
    let did_ada_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default methods for highlighting. Can be overridden later.
  HiLink adaCharacter	Character
  HiLink adaComment	Comment
  HiLink adaConditional	Conditional
  HiLink adaKeyword	Keyword
  HiLink adaLabel	Label
  HiLink adaNumber	Number
  HiLink adaSign	Number
  HiLink adaOperator	Operator
  HiLink adaPreproc	PreProc
  HiLink adaRepeat	Repeat
  HiLink adaSpecial	Special
  HiLink adaStatement	Statement
  HiLink adaString	String
  HiLink adaStructure	Structure
  HiLink adaTodo	Todo
  HiLink adaType	Type
  HiLink adaTypedef	Typedef
  HiLink adaStorageClass	StorageClass
  HiLink adaBoolean	Boolean
  HiLink adaException	Exception
  HiLink adaInc	Include
  HiLink adaError	Error
  HiLink adaSpaceError	Error
  HiLink adaBuiltinType Type

  if exists("ada_begin_preproc")
   " This is the old default display:
   HiLink adaBegin	PreProc
   HiLink adaEnd	PreProc
  else
   " This is the new default display:
   HiLink adaBegin	Keyword
   HiLink adaEnd	Keyword
  endif

  delcommand HiLink
endif

let b:current_syntax = "ada"

" vim: ts=8
