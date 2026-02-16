vim9script

# Vim syntax file.
# Language:    Hare
# Maintainer:  Amelia Clarke <selene@perilune.dev>
# Last Change: 2026 Feb 15
# Upstream:    https://git.sr.ht/~sircmpwn/hare.vim

if exists('b:current_syntax')
  finish
endif

# Syntax {{{1
syn case match
syn iskeyword @,48-57,@-@,_

# Identifiers {{{2
syn cluster hareIdentifier contains=hareName,hareScopeDelimiter,@hareReserved
syn match hareIdentifier '\v<\h\w*%(::\h\w*)*>' contains=@hareIdentifier nextgroup=hareScopeDelimiter,@harePostfix skipempty skipwhite
syn match hareName '\<\h\w*\>' contained transparent

# Reserved keywords.
syn cluster hareReserved contains=hareBoolean,hareBuiltin,hareConditional,hareConstant,hareDefine,hareInclude,hareKeyword,hareLabel,hareOperator,hareRepeat,hareStatement,hareStorageClass,hareStructure,hareType,hareTypedef

# Punctuators {{{2

# Balanced tokens.
syn region hareBraces matchgroup=hareBrace start='{' end='}' contains=TOP fold transparent
syn region hareBrackets matchgroup=hareBracket start='\[' end=']' contains=TOP transparent
syn region hareParens matchgroup=hareParen start='(' end=')' contains=TOP nextgroup=@harePostfix skipempty skipwhite transparent

# Symbolic operators.
syn match hareSymbolOperator '\.\{2,3}'
syn match hareSymbolOperator '[!<=>]=\?'
syn match hareSymbolOperator '=>'

# Additive and multiplicative arithmetic.
syn match hareSymbolOperator '[-+*/%]=\?'

# Bitwise arithmetic.
syn match hareSymbolOperator '\%(<<\|>>\)=\?'
syn match hareSymbolOperator '[&^|]=\?'
syn match hareSymbolOperator '\~'

# Logical arithmetic.
syn match hareSymbolOperator '\%(&&\|^^\|||\)=\?'

# Types {{{2
syn cluster hareType contains=hareArray,hareError,harePointer,hareStorageClass,hareStructure,hareTaggedUnion,hareType
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

# Pointer types.
syn match harePointer '*' contained containedin=hareTaggedUnion,hareTypeParens nextgroup=@hareType skipempty skipwhite
syn keyword hareStorageClass nullable nextgroup=harePointer skipempty skipwhite

# Slice and array types.
syn region hareArray matchgroup=hareBracket start='\[' end=']' contained containedin=hareTaggedUnion,hareTypeParens contains=TOP nextgroup=@hareType skipempty skipwhite transparent
syn match hareArray '\[[*_]]' contains=hareArrayBounds nextgroup=@hareType skipempty skipwhite transparent
syn match hareArrayBounds '*' contained display

# Tagged union and tuple types.
syn region hareTaggedUnion matchgroup=hareParen start='(' end=')' contained containedin=hareTaggedUnion,hareTypeParens contains=TOP transparent
syn match hareTaggedUnionBar '|' contained containedin=hareTaggedUnion

# Other types.
syn match hareError '!' contained containedin=hareTaggedUnion,hareTypeParens nextgroup=@hareType skipempty skipwhite
syn keyword hareStructure enum struct union

# Declarations {{{2
syn keyword hareDefine def
syn keyword hareInclude use
syn keyword hareKeyword export static
syn keyword hareKeyword fn nextgroup=@hareFunction,@hareReserved skipempty skipwhite
syn keyword hareStatement const let
syn keyword hareTypedef type nextgroup=hareTypedefBinding,@hareReserved skipempty skipwhite

# Highlight `const` as a storage-class in places types are expected.
syn keyword hareStorageClass const contained containedin=hareTaggedUnion,hareTypeParens nextgroup=@hareType skipempty skipwhite

# Function declarations.
syn cluster hareFunction contains=hareFunction,hareFunctionParams
syn match hareFunction '\v<\h\w*%(::\h\w*)*>' contained contains=@hareIdentifier nextgroup=hareFunctionParams skipempty skipwhite
syn region hareFunctionParams matchgroup=hareParen start='(' end=')' contained contains=TOP nextgroup=@hareType skipempty skipwhite transparent

# Type declarations.
# XXX: Does not yet account for type declarations with multiple bindings.
syn match hareTypedefBinding '\v<\h\w*%(::\h\w*)*>' contained nextgroup=hareTypedefEquals skipempty skipwhite transparent
syn match hareTypedefEquals '=' contained nextgroup=@hareType skipempty skipwhite transparent

# Attributes {{{3
syn keyword hareAttribute @init @fini @test
syn keyword hareAttribute @packed
syn keyword hareAttribute @symbol nextgroup=hareAttributeParens skipempty skipwhite
syn keyword hareAttribute @threadlocal
syn keyword hareAttribute @undefined

# Match the parens following attributes.
syn region hareAttributeParens matchgroup=hareParen start='(' end=')' contained contains=TOP transparent

# Expressions {{{2
syn keyword hareConditional else
syn keyword hareConditional if nextgroup=hareConditionParens skipempty skipwhite
syn keyword hareConditional match switch nextgroup=@hareCondition skipempty skipwhite
syn keyword hareLabel case nextgroup=@hareType skipempty skipwhite
syn keyword hareOperator as is nextgroup=@hareType skipempty skipwhite
syn keyword hareRepeat for nextgroup=@hareCondition skipempty skipwhite
syn keyword hareStatement break continue return yield
syn keyword hareStatement defer

# Match the parens in conditionals and loops.
syn cluster hareCondition contains=hareConditionLabel,hareConditionParens
syn match hareConditionLabel ':\h\w*\>' contained nextgroup=hareConditionParens skipempty skipwhite transparent
syn region hareConditionParens matchgroup=hareParen start='(' end=')' contained contains=TOP transparent

# Builtins {{{3
syn keyword hareBuiltin abort assert
syn keyword hareBuiltin align nextgroup=hareTypeParens skipempty skipwhite
syn keyword hareBuiltin alloc free
syn keyword hareBuiltin append insert delete
syn keyword hareBuiltin len offset

# C ABI.
syn keyword hareBuiltin vastart vaarg vaend

# Highlight `size` as a type unless it is followed by an open paren.
syn match hareType '\<size\>'
syn match hareBuiltin '\<size\ze(' nextgroup=hareTypeParens

# Match the parens in builtin expressions expecting a type.
syn region hareTypeParens matchgroup=hareParen start='(' end=')' contained contains=TOP nextgroup=@harePostfix skipempty skipwhite transparent

# Postfix expressions {{{3
# TODO: Match postfix expressions after literals.
syn cluster harePostfix contains=hareCast,hareField,hareSlice,hareSpecial

# Casts and type hints.
syn match hareCast ':' nextgroup=@hareType skipempty skipwhite

# Error checking.
syn match hareSpecial '!=\@!' contained nextgroup=@harePostfix skipempty skipwhite
syn match hareSpecial '?' nextgroup=@harePostfix skipempty skipwhite

# Field access.
syn match hareField '\.\w\+\>' contained contains=hareName,hareNumber,@hareReserved nextgroup=@harePostfix skipempty skipwhite transparent

# Indexing and slicing.
syn region hareSlice matchgroup=hareBracket start='\[' end=']' contained contains=TOP nextgroup=@harePostfix skipempty skipwhite transparent

# Literals {{{2
syn keyword hareBoolean true false
syn keyword hareConstant null

# Integers {{{3
syn match hareNumber '\v<%(0|[1-9]%(_?\d)*)%([Ee]\+?\d+)?%([iu]%(8|16|32|64)?|z)?>'
syn match hareNumber '\v<0b[01]%(_?[01])*%([iu]%(8|16|32|64)?|z)?>'
syn match hareNumber '\v<0o\o%(_?\o)*%([iu]%(8|16|32|64)?|z)?>'
syn match hareNumber '\v<0x\x%(_?\x)*%([iu]%(8|16|32|64)?|z)?>'

# Floats {{{3
# XXX: Technically, the third form is not a valid floating literal according to
#      the specification, but is currently accepted by the Hare compiler and
#      used occasionally within the standard library.
syn match hareFloat '\v<%(0|[1-9]%(_?\d)*)\.\d%(_?\d)*%([Ee][+-]?\d+)?%(f32|f64)?>'
syn match hareFloat '\v<%(0|[1-9]%(_?\d)*)%([Ee][+-]?\d+)?%(f32|f64)>'
syn match hareFloat '\v<%(0|[1-9]%(_?\d)*)[Ee]-\d+>'
syn match hareFloat '\v<0x\x%(_?\x)*%(\.\x%(_?\x)*)?[Pp][+-]?\d+%(f32|f64)?>'

# Rune and string literals {{{3
syn region hareRune matchgroup=hareRuneDelimiter start="'" skip="\\'" end="'" contains=hareEscape
syn region hareString matchgroup=hareStringDelimiter start='"' skip='\\"' end='"' contains=hareEscape,hareFormat
syn region hareString matchgroup=hareStringDelimiter start='`' end='`' contains=hareFormat

# Escape sequences.
syn match hareEscape '\\[0abfnrtv\\'"]' contained
syn match hareEscape '\v\\%(x\x{2}|u\x{4}|U\x{8})' contained display

# Format sequences.
syn match hareFormat '\v\{\d*%(:%(\.?\d+|[- +=befgoxX]|F[.2EsSU]|_%(\_[^\\]|\\%([0abfnrtv\\'"]|x\x{2}|u\x{4}|U\x{8})))*)?}' contained contains=hareEscape
syn match hareFormat '{\d*%\d*}' contained display
syn match hareFormat '{{\|}}' contained

# Miscellaneous {{{2

# Annotations.
syn region hareAnnotation start='#\[' end=']' contains=hareAnnotationIdentifier,hareComment,hareRune,hareString
syn match hareAnnotationIdentifier '\v#\[\s*\zs\h\w*%(::\h\w*)*>' contained contains=hareName,@hareReserved nextgroup=hareAnnotationParens skipempty skipwhite
syn region hareAnnotationParens matchgroup=hareAnnotationParen start='(' end=')' contained contains=TOP

# Comments.
syn region hareComment excludenl start='//' end='$' contains=hareSpecialComment,hareTodo,@Spell
syn match hareSpecialComment '\v\[\[\h\w*%(::\h\w*)*%(::)?]]' contained contains=@NoSpell display
syn keyword hareTodo FIXME TODO XXX contained

# Scope delimiters.
syn match hareScopeDelimiter '::'

# User labels.
syn match hareUserLabel ':\h\w*\>' contains=hareName,@hareReserved

# Default highlighting {{{1
hi def link hareAnnotation Special
hi def link hareAnnotationIdentifier hareAnnotation
hi def link hareAnnotationParen hareAnnotation
hi def link hareArrayBounds harePointer
hi def link hareAttribute PreProc
hi def link hareBoolean Boolean
hi def link hareBuiltin hareOperator
hi def link hareComment Comment
hi def link hareConditional Conditional
hi def link hareConstant Constant
hi def link hareDefine Define
hi def link hareError hareSpecial
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
hi def link hareRuneDelimiter hareRune
hi def link hareScopeDelimiter Delimiter
hi def link hareSpecial Special
hi def link hareSpecialComment SpecialComment
hi def link hareStatement Statement
hi def link hareStorageClass StorageClass
hi def link hareString String
hi def link hareStringDelimiter hareString
hi def link hareStructure Structure
hi def link hareTodo Todo
hi def link hareType Type
hi def link hareTypedef Typedef
hi def link hareUserLabel Identifier

# Highlight incorrect whitespace by default.
syn match hareSpaceError excludenl '\s\+$' containedin=ALL display
syn match hareSpaceError ' \+\ze\t' display
if get(g:, 'hare_space_error', 1)
  hi! def link hareSpaceError Error
else
  hi! def link hareSpaceError NONE
endif

b:current_syntax = 'hare'

# vim: fdm=marker et sts=2 sw=2 ts=8 tw=80
