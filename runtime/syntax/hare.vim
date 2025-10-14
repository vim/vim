vim9script

# Vim syntax file.
# Language:    Hare
# Maintainer:  Amelia Clarke <selene@perilune.dev>
# Last Change: 2025 Sep 06
# Upstream:    https://git.sr.ht/~sircmpwn/hare.vim

if exists('b:current_syntax')
  finish
endif

# Syntax {{{1
syn case match
syn iskeyword @,48-57,@-@,_

# Reserved keywords.
syn cluster hareReserved contains=hareBoolean,hareBuiltin,hareConditional,hareConstant,hareDefine,hareInclude,hareKeyword,hareLabel,hareOperator,hareRepeat,hareStorageClass,hareStructure,hareType,hareTypedef

# Types {{{2
syn cluster hareType contains=hareErrorFlag,harePointer,hareSlice,hareStorageClass,hareStructure,hareTaggedUnion,hareType
syn keyword hareType bool
syn keyword hareType done
syn keyword hareType f32 f64
syn keyword hareType i8 i16 i32 i64 int
syn keyword hareType never
syn keyword hareType nomem
syn keyword hareType opaque
syn keyword hareType rune str
syn keyword hareType u8 u16 u32 u64 uint uintptr
syn keyword hareType void

# C ABI.
syn keyword hareType valist

# Slice and array types.
syn region hareSlice matchgroup=hareSlice start='\[' end=']' contained containedin=hareBuiltinTypeCall,hareTaggedUnion contains=TOP nextgroup=@hareType skipempty skipwhite
syn match hareSlice '\[[*_]]' contains=hareSliceBounds nextgroup=@hareType skipempty skipwhite
syn match hareSliceBounds '[*_]' contained display

# Other types.
syn keyword hareStorageClass nullable nextgroup=harePointer skipempty skipwhite
syn keyword hareStructure enum struct union

# Declarations {{{2
syn keyword hareDefine def
syn keyword hareInclude use
syn keyword hareKeyword const nextgroup=@hareType skipempty skipwhite
syn keyword hareKeyword export static
syn keyword hareKeyword fn nextgroup=@hareFunction skipempty skipwhite
syn keyword hareKeyword let
syn keyword hareTypedef type nextgroup=hareTypeIdentifier skipempty skipwhite

# Function declarations.
syn cluster hareFunction contains=hareFunction,hareFuncParams
syn match hareFunction '\v<\h\w*%(::\h\w*)*>' contained contains=@hareIdentifier nextgroup=hareFuncParams skipempty skipwhite
syn region hareFuncParams matchgroup=hareFuncParams start='(' end=')' contained contains=TOP nextgroup=@hareType skipempty skipwhite

# Type declarations.
# FIXME: Does not yet account for type declarations with multiple bindings.
syn match hareTypeIdentifier '\v<\h\w*%(::\h\w*)*>' contained contains=hareIdentifier nextgroup=hareTypeEquals skipempty skipwhite transparent
syn match hareTypeEquals '=' contained nextgroup=@hareType skipempty skipwhite transparent

# Identifiers.
syn match hareIdentifier '\v<\h\w*%(::\h\w*)*>' contains=@hareIdentifier nextgroup=@harePostfix skipempty skipwhite
syn cluster hareIdentifier contains=hareDelimiter,hareName
syn match hareName '\<\h\w*\>' contained contains=@hareReserved transparent

# Attributes {{{3
syn keyword hareAttribute @init @fini @test
syn keyword hareAttribute @offset nextgroup=hareAttrParens skipempty skipwhite
syn keyword hareAttribute @packed
syn keyword hareAttribute @symbol nextgroup=hareAttrParens skipempty skipwhite
syn keyword hareAttribute @threadlocal

# Match the parens after attributes.
syn region hareAttrParens matchgroup=hareAttrParens start='(' end=')' contained contains=TOP

# Expressions {{{2
syn keyword hareConditional else
syn keyword hareConditional if nextgroup=hareCondParens skipempty skipwhite
syn keyword hareConditional match switch nextgroup=@hareCondition skipempty skipwhite
syn keyword hareKeyword break continue return yield
syn keyword hareKeyword defer
syn keyword hareLabel case nextgroup=@hareType skipempty skipwhite
syn keyword hareOperator as is nextgroup=@hareType skipempty skipwhite
syn keyword hareRepeat for nextgroup=@hareCondition skipempty skipwhite

# Match the parens in conditionals and for-loops.
syn cluster hareCondition contains=hareCondLabel,hareCondParens
syn match hareCondLabel ':\h\w*\>' contained contains=hareUserLabel nextgroup=hareCondParens skipempty skipwhite transparent
syn region hareCondParens matchgroup=hareCondParens start='(' end=')' contained contains=TOP

# Builtins {{{3
syn keyword hareBuiltin abort assert nextgroup=hareBuiltinCall skipempty skipwhite
syn keyword hareBuiltin align nextgroup=hareBuiltinTypeCall skipempty skipwhite
syn keyword hareBuiltin alloc free nextgroup=hareBuiltinCall skipempty skipwhite
syn keyword hareBuiltin append insert delete nextgroup=hareBuiltinCall skipempty skipwhite
syn keyword hareBuiltin len offset nextgroup=hareBuiltinCall skipempty skipwhite

# C ABI.
syn keyword hareBuiltin vastart vaarg vaend nextgroup=hareBuiltinCall skipempty skipwhite

# Highlight `size` as a builtin only if it is followed by an open paren.
syn match hareType '\<size\>'
syn match hareBuiltin '\<size\ze(' nextgroup=hareBuiltinTypeCall

# Match the parens in builtin expressions.
syn region hareBuiltinCall matchgroup=hareBuiltinCall start='(' end=')' contained contains=TOP nextgroup=@harePostfix skipempty skipwhite
syn region hareBuiltinTypeCall matchgroup=hareBuiltinTypeCall start='(' end=')' contained contains=TOP nextgroup=@harePostfix skipempty skipwhite

# Operators {{{3
syn match hareSymbolOperator '\.\{2,3}'
syn match hareSymbolOperator '[!<=>]=\?'
syn match hareSymbolOperator '=>'

# Additive and multiplicative arithmetic.
syn match hareSymbolOperator '[-+*/%]=\?'

# Bit-shifting arithmetic.
syn match hareSymbolOperator '\%(<<\|>>\)=\?'

# Bitwise arithmetic.
syn match hareSymbolOperator '[&^|]=\?'
syn match hareSymbolOperator '\~'

# Logical arithmetic.
syn match hareSymbolOperator '\%(&&\|^^\|||\)=\?'

# Highlight `!`, `*`, and `|` correctly in types.
syn match hareErrorFlag '!' contained containedin=hareBuiltinTypeCall,hareTaggedUnion nextgroup=@hareType skipempty skipwhite
syn match harePointer '*' contained containedin=hareBuiltinTypeCall,hareTaggedUnion nextgroup=@hareType skipempty skipwhite
syn match hareTaggedUnionBar '|' contained containedin=hareTaggedUnion

# Postfix expressions {{{3
# TODO: Match postfix expressions after literals.
syn cluster harePostfix contains=hareCast,hareErrorCheck,hareFieldAccess,hareFuncCall,hareIndex

# Casts and type hints.
syn match hareCast ':' nextgroup=@hareType skipempty skipwhite

# Error handling.
syn match hareErrorCheck '!=\@!' contained nextgroup=@harePostfix skipempty skipwhite
syn match hareErrorCheck '?' nextgroup=@harePostfix skipempty skipwhite

# Field access.
syn match hareFieldAccess '\.\w\+\>' contained contains=hareName,hareNumber nextgroup=@harePostfix skipempty skipwhite

# Function calls.
syn region hareFuncCall matchgroup=hareFuncCall start='(' end=')' contained contains=TOP nextgroup=@harePostfix skipempty skipwhite

# Indexing and slicing.
syn region hareIndex matchgroup=hareIndex start='\[' end=']' contained contains=TOP nextgroup=@harePostfix skipempty skipwhite

# Nested expressions.
syn region hareParens matchgroup=hareParens start='(' end=')' contains=TOP nextgroup=@harePostfix skipempty skipwhite

# Tagged union and tuple types.
syn region hareTaggedUnion matchgroup=hareTaggedUnion start='(' end=')' contained containedin=hareBuiltinTypeCall,hareTaggedUnion contains=TOP

# Literals {{{3
syn keyword hareBoolean true false
syn keyword hareConstant null

# Integers.
syn match hareNumber '\v<%(0|[1-9]%(_?\d)*)%([Ee]\+?\d+)?%([iu]%(8|16|32|64)?|z)?>'
syn match hareNumber '\v<0b[01]%(_?[01])*%([iu]%(8|16|32|64)?|z)?>'
syn match hareNumber '\v<0o\o%(_?\o)*%([iu]%(8|16|32|64)?|z)?>'
syn match hareNumber '\v<0x\x%(_?\x)*%([iu]%(8|16|32|64)?|z)?>'

# Floats.
syn match hareFloat '\v<%(0|[1-9]%(_?\d)*)\.\d%(_?\d)*%([Ee][+-]?\d+)?%(f32|f64)?>'
syn match hareFloat '\v<%(0|[1-9]%(_?\d)*)%([Ee][+-]?\d+)?%(f32|f64)>'
syn match hareFloat '\v<%(0|[1-9]%(_?\d)*)[Ee]-\d+>'
syn match hareFloat '\v<0x\x%(_?\x)*%(\.\x%(_?\x)*)?[Pp][+-]?\d+%(f32|f64)?>'

# Rune and string literals.
syn region hareRune start="'" skip="\\'" end="'" contains=hareEscape
syn region hareString start='"' skip='\\"' end='"' contains=hareEscape,hareFormat
syn region hareString start='`' end='`' contains=hareFormat

# Escape sequences.
syn match hareEscape '\\[0abfnrtv\\'"]' contained
syn match hareEscape '\v\\%(x\x{2}|u\x{4}|U\x{8})' contained display

# Format sequences.
syn match hareFormat '\v\{\d*%(:%(\.?\d+|[- +=Xbefgox]|F[.2ESUs]|_%(\_.|\\%([0abfnrtv\'"]|x\x{2}|u\x{4}|U\x{8})))*)?}' contained contains=hareEscape
syn match hareFormat '{\d*%\d*}' contained display
syn match hareFormat '{{\|}}' contained

# Miscellaneous {{{2

# Annotations.
syn region hareAnnotation start='#\[' end=']' contains=hareAnnotationIdentifier
syn match hareAnnotationIdentifier '\v<\h\w*%(::\h\w*)*>' contained contains=@hareIdentifier nextgroup=hareAnnotationParens skipempty skipwhite transparent
syn region hareAnnotationParens matchgroup=hareAnnotationParens start='(' end=')' contained contains=TOP

# Blocks.
syn region hareBlock matchgroup=hareBlock start='{' end='}' contains=TOP fold nextgroup=@harePostfix skipempty skipwhite

# Comments.
syn region hareComment start='//' end='$' contains=@hareComment keepend
syn cluster hareComment contains=hareCommentCode,hareCommentRef,hareTodo,@Spell
syn region hareCommentCode start='\t\zs' end='$' contained contains=@NoSpell display
syn match hareCommentRef '\v\[\[\h\w*%(::\h\w*)*%(::)?]]' contained contains=@NoSpell display
syn keyword hareTodo FIXME TODO XXX contained

# Delimiters.
syn match hareDelimiter '::'

# Labels.
syn match hareUserLabel ':\h\w*\>' contains=hareName

# Default highlighting {{{1
hi def link hareAnnotation PreProc
hi def link hareAnnotationParens hareAnnotation
hi def link hareAttribute PreProc
hi def link hareBoolean Boolean
hi def link hareBuiltin Operator
hi def link hareComment Comment
hi def link hareCommentCode hareComment
hi def link hareCommentRef SpecialComment
hi def link hareConditional Conditional
hi def link hareConstant Constant
hi def link hareDefine Define
hi def link hareDelimiter Delimiter
hi def link hareErrorFlag hareStorageClass
hi def link hareErrorCheck Special
hi def link hareEscape SpecialChar
hi def link hareFloat Float
hi def link hareFormat SpecialChar
hi def link hareFunction Function
hi def link hareInclude Include
hi def link hareKeyword Keyword
hi def link hareLabel Label
hi def link hareNumber Number
hi def link hareOperator Operator
hi def link harePointer hareStorageClass
hi def link hareRepeat Repeat
hi def link hareRune Character
hi def link hareSliceBounds harePointer
hi def link hareStorageClass StorageClass
hi def link hareString String
hi def link hareStructure Structure
hi def link hareTodo Todo
hi def link hareType Type
hi def link hareTypedef Typedef
hi def link hareUserLabel Identifier

# Optionally highlight symbolic operators.
if get(g:, 'hare_symbol_operators')
  hi! def link hareSymbolOperator hareOperator
else
  hi! def link hareSymbolOperator NONE
endif

# Highlight incorrect whitespace by default.
syn match hareSpaceError '\s\+$' containedin=ALL display
syn match hareSpaceError ' \+\ze\t' display
if get(g:, 'hare_space_error', 1)
  hi! def link hareSpaceError Error
else
  hi! def link hareSpaceError NONE
endif

b:current_syntax = 'hare'

# vim: fdm=marker et sts=2 sw=2 ts=8 tw=80
