vim9script
# Vim syntax file
# Language:    OmniMark
# Maintainer:  Peter Kenny <kennypete.t2o3y@aleeas.com>
# Previous Maintainer: Paul Terray <mailto:terray@4dconcept.fr>
# Last Change: 2025-03-23
# License:     Vim (see :help license)
# History:     2000-10-11 Vintage minimal syntax file (Paul Terray)
#
# - Syntax is grouped, generally, by type (action to rule), using the
#   version 12 headings.  Refer:
#   https://developers.stilo.com/docs/html/keyword/type.html
# - Deprecated/legacy syntax back to version 4 is included.
# - OmniMark is largely case insensitive, with handled exceptions (e.g., %g)
# ----------------------------------------------------------------------------
syntax case ignore
# Current syntax exists: finish {{{
if exists("b:current_syntax")
  finish
endif
# }}}
# Keyword characters {{{
# 	!#%&*+-/0123456789<=>@
# 	ABCDEFGHIJKLMNOPQRSTUVWXYZ
# 	\_abcdefghijklmnopqrstuvwxyz|~
setlocal iskeyword=33,35,37-38,42-43,45,47-57,60-62,64-90,92,95,97-122,124,126
# }}}
# _ action {{{
syntax keyword omnimarkAction activate
syntax keyword omnimarkAction assert
syntax keyword omnimarkAction clear
syntax keyword omnimarkAction close
syntax keyword omnimarkAction collect-garbage
syntax match omnimarkAction "\v\c<%(copy)%(-clear)?"
syntax keyword omnimarkAction deactivate
syntax keyword omnimarkAction decrement
syntax keyword omnimarkAction discard
syntax keyword omnimarkAction flush
syntax match omnimarkAction "\v\c<%(halt)%(-everything)?"
syntax keyword omnimarkAction increment
syntax keyword omnimarkAction log
syntax keyword omnimarkAction log-message
syntax keyword omnimarkAction match
syntax keyword omnimarkAction new
  # new takes before and after (as in new x{"wilma"} after [2])
  syntax keyword omnimarkAction after
  syntax keyword omnimarkAction before
# This is the only way 'next' is used (and it cannot be 'isnt'):
syntax match omnimarkAction "\v\c<%(next\s+group\s+is)"
syntax keyword omnimarkAction not-reached
syntax keyword omnimarkAction open
syntax match omnimarkAction "\v\c<%(output)%(-to)?"
syntax keyword omnimarkAction put
# When alone, 'referent' is nearly always 'put...referent,' which is an action
syntax keyword omnimarkAction referent
syntax match omnimarkAction "\v\c<%(remove)%(\s+key\s+of)?"
syntax keyword omnimarkAction reopen
syntax keyword omnimarkAction reset
syntax keyword omnimarkAction return
# 'scan' because it can start a line in a block of 'do *-parse ... done'
syntax keyword omnimarkAction scan
syntax match omnimarkAction "\v\c<%(set)%(\s+%(buffer|creator\s+of|external-function|file|function-library|key\s+of|new|referent|stream))?"
syntax keyword omnimarkAction sgml-in
syntax keyword omnimarkAction sgml-out
syntax keyword omnimarkAction signal
syntax keyword omnimarkAction submit
syntax keyword omnimarkAction suppress
syntax keyword omnimarkAction throw
syntax keyword omnimarkAction void
# }}}
# _ built-in data type {{{
# attribute-declaration: Every attribute-declaration instance has two
#                        properties: attribute-default-declaration and
#                                    attribute-value-declaration
# attribute-default-declaration: is the abstract type of declared defaults for
#                                unspecified attribute values. Its concrete
#                                instances can be obtained with the following
#                                four constants:
syntax keyword omnimarkConstant attribute-declared-conref
syntax keyword omnimarkConstant attribute-declared-current
syntax keyword omnimarkConstant attribute-declared-implied
syntax keyword omnimarkConstant attribute-declared-required
#                                and with two operators which take a string
#                                default value argument:
syntax match omnimarkOperator "\v\c<%(attribute-declared-defaulted\s+to)"
syntax match omnimarkOperator "\v\c<%(attribute-declared-fixed to)"
# attribute-value-declaration: is the abstract type of values that an
#                              attribute is declared to accept. Its concrete
#                              instances can be obtained with the following
#                              constants:
syntax match omnimarkConstant "\v\c<%(attribute-declared-fixed to)"
syntax keyword omnimarkConstant attribute-declared-id
syntax keyword omnimarkConstant attribute-declared-idref
syntax keyword omnimarkConstant attribute-declared-idrefs
syntax keyword omnimarkConstant attribute-declared-name
syntax keyword omnimarkConstant attribute-declared-names
syntax keyword omnimarkConstant attribute-declared-nmtoken
syntax keyword omnimarkConstant attribute-declared-nmtokens
syntax keyword omnimarkConstant attribute-declared-number
syntax keyword omnimarkConstant attribute-declared-numbers
syntax keyword omnimarkConstant attribute-declared-nutoken
syntax keyword omnimarkConstant attribute-declared-nutokens
#                              and with one operator that takes a string shelf
#                              argument listing all the values allowed for the
#                              attribute:
syntax keyword omnimarkOperator attribute-declared-group
# content-model - the following six constants are the only possible values:
syntax keyword omnimarkConstant any-content-model
syntax keyword omnimarkConstant cdata-content-model
syntax keyword omnimarkConstant element-content-model
syntax keyword omnimarkConstant empty-content-model
syntax keyword omnimarkConstant mixed-content-model
syntax keyword omnimarkConstant rcdata-content-model
# refer also:
# developers.stilo.com/docs/html/keyword/create-element-declaration.html
#
# declared-attribute is and abstract type with subtypes:
#                             implied-attribute
#                             specified-attribute
# dtd abstract data type has subtypes:
#                             sgml-dtd
#                             xml-dtd
# element-declaration
# entity-declaration
# markup-element-event
# markup-event has subtypes:
#                             markup-point-event
#                             markup-region-event
# }}}
# _ built-in entity {{{
syntax match omnimarkBuiltinEntity "\v\c<%(#capacity)"
syntax match omnimarkBuiltinEntity "\v\c<%(#charset)"
syntax match omnimarkBuiltinEntity "\v\c<%(#document)"
syntax match omnimarkBuiltinEntity "\v\c<%(#dtd)"
syntax match omnimarkBuiltinEntity "\v\c<%(#implied)"
syntax match omnimarkBuiltinEntity "\v\c<%(#schema)"
syntax match omnimarkBuiltinEntity "\v\c<%(#syntax)"
# }}}
# _ built-in shelf {{{
syntax match omnimarkBuiltinShelf "\v\c<%(#additional-info)"
syntax match omnimarkBuiltinShelf "\v\c<%(#appinfo)"
syntax match omnimarkBuiltinShelf "\v\c<%(#args)"
syntax match omnimarkBuiltinShelf "\v\c<%(#class)"
syntax match omnimarkBuiltinShelf "\v\c<%(#command-line-names)"
syntax match omnimarkBuiltinShelf "\v\c<%(#console)"
syntax match omnimarkBuiltinShelf "\v\c<%(#content)"
syntax match omnimarkBuiltinShelf "\v\c<%(#current-dtd)"
syntax match omnimarkBuiltinShelf "\v\c<%(#current-input)"
syntax match omnimarkBuiltinShelf "\v\c<%(#current-markup-event)"
syntax match omnimarkBuiltinShelf "\v\c<%(#current-output)"
syntax match omnimarkBuiltinShelf "\v\c<%(#doctype)"
syntax match omnimarkBuiltinShelf "\v\c<%(#error)"
syntax match omnimarkBuiltinShelf "\v\c<%(#error-code)"
syntax match omnimarkBuiltinShelf "\v\c<%(#file-name)"
syntax match omnimarkBuiltinShelf "\v\c<%(#language-version)"
syntax match omnimarkBuiltinShelf "\v\c<%(#libpath)"
syntax match omnimarkBuiltinShelf "\v\c<%(#library)"
syntax match omnimarkBuiltinShelf "\v\c<%(#libvalue)"
syntax match omnimarkBuiltinShelf "\v\c<%(#line-number)"
syntax match omnimarkBuiltinShelf "\v\c<%(#log)"
syntax match omnimarkBuiltinShelf "\v\c<%(#main-input)"
syntax match omnimarkBuiltinShelf "\v\c<%(#main-output)"
syntax match omnimarkBuiltinShelf "\v\c<%(#markup-error-count)"
syntax match omnimarkBuiltinShelf "\v\c<%(#markup-error-total)"
syntax match omnimarkBuiltinShelf "\v\c<%(#markup-parser)"
syntax match omnimarkBuiltinShelf "\v\c<%(#markup-warning-count)"
syntax match omnimarkBuiltinShelf "\v\c<%(#markup-warning-total)"
syntax match omnimarkBuiltinShelf "\v\c<%(#message)"
syntax match omnimarkBuiltinShelf "\v\c<%(#output)"
syntax match omnimarkBuiltinShelf "\v\c<%(#platform-info)"
syntax match omnimarkBuiltinShelf "\v\c<%(#process-input)"
syntax match omnimarkBuiltinShelf "\v\c<%(#process-output)"
syntax match omnimarkBuiltinShelf "\v\c<%(#recovery-info)"
syntax match omnimarkBuiltinShelf "\v\c<%(#sgml)"
syntax match omnimarkBuiltinShelf "\v\c<%(#sgml-error-count)"
syntax match omnimarkBuiltinShelf "\v\c<%(#sgml-error-total)"
syntax match omnimarkBuiltinShelf "\v\c<%(#sgml-warning-count)"
syntax match omnimarkBuiltinShelf "\v\c<%(#sgml-warning-total)"
syntax match omnimarkBuiltinShelf "\v\c<%(#suppress)"
syntax match omnimarkBuiltinShelf "\v\c<%(#xmlns-names)"
syntax keyword omnimarkBuiltinShelf attributes
syntax match omnimarkBuiltinShelf "\v\c<%(current)%(\s+%(element%(s)?|dtd|sgml-dtd))"
syntax keyword omnimarkBuiltinShelf data-attributes
syntax keyword omnimarkBuiltinShelf referents
syntax keyword omnimarkBuiltinShelf sgml-dtds
  # deprecated synonym for sgml-dtds:
  syntax keyword omnimarkBuiltinShelf dtds
syntax keyword omnimarkBuiltinShelf xml-dtds
syntax match omnimarkBuiltinShelf "\v\c<%(specified\s+attributes)"
# }}}
# _ catch name {{{
syntax match omnimarkCatchName "\v\c<%(#external-exception)"
  # external exception parameters
  syntax keyword omnimarkCatchName identity
  syntax keyword omnimarkCatchName message
syntax keyword omnimarkCatchName location
syntax match omnimarkCatchName "\v\c<%(#markup-end)"
syntax match omnimarkCatchName "\v\c<%(#markup-point)"
syntax match omnimarkCatchName "\v\c<%(#markup-start)"
syntax match omnimarkCatchName "\v\c<%(#program-error)"
# }}}
# _ constant {{{
syntax keyword omnimarkConstant false
syntax keyword omnimarkConstant true
# Example: local stream u initial {unattached}
syntax keyword omnimarkConstant unattached
# }}}
# _ control structure {{{
syntax match omnimarkControlStructure "\v\c<%(#first)"
syntax match omnimarkControlStructure "\v\c<%(#group)"
syntax match omnimarkControlStructure "\v\c<%(#item)"
syntax match omnimarkControlStructure "\v\c<%(#last)"
syntax match omnimarkControlStructure "\v%(\s)?%(-\>)%(\s)?"
syntax keyword omnimarkControlStructure again
syntax keyword omnimarkControlStructure always
syntax keyword omnimarkControlStructure as
syntax keyword omnimarkControlStructure case
syntax keyword omnimarkControlStructure catch
syntax match omnimarkControlStructure "\v\c<%(do)>%(%(\s|\n)+%((markup|sgml|xml)-parse|scan|select|select-type|skip%(\s+over)?|unless|when))?"
  # with id-checking and with utf-8, example:
  # do sgml-parse document with id-checking false scan file "my.sgml"
  syntax keyword omnimarkControlStructure id-checking
  syntax keyword omnimarkControlStructure utf-8
  syntax keyword omnimarkControlStructure with
syntax keyword omnimarkControlStructure done
syntax keyword omnimarkControlStructure else
syntax keyword omnimarkControlStructure exit
syntax match omnimarkControlStructure "\v\c<%(repeat)>%(\s+%(for|over%(\s+current elements)?|scan|to))?"
  # Example: repeat over reversed
  syntax keyword omnimarkControlStructure reversed
# Note: repeat over attribute(s) not needed - handled separately
# Note: repeat over data-attribute(s) not needed - handled separately
# Note: repeat over referents not needed - handled separately
syntax keyword omnimarkControlStructure rethrow
syntax keyword omnimarkControlStructure select
syntax keyword omnimarkControlStructure unless
syntax match omnimarkControlStructure "\v\c<%(using)%( %(%(data-)?attribute%(s)?|catch|group|input as|nested-referents|output\s+as|referents))?"
syntax keyword omnimarkControlStructure when
# }}}
# _ data type {{{
# (not a v12 heading)
syntax keyword omnimarkDataType bcd
syntax keyword omnimarkDataType counter
# db.database data type - refer omdb, below, for library functions
syntax match omnimarkDataType "\v\c<%(db)%([.])?%(database)"
syntax keyword omnimarkDataType document
syntax keyword omnimarkDataType float
syntax keyword omnimarkDataType instance
syntax match omnimarkDataType "\v\c<%(int32)"
syntax keyword omnimarkDataType integer
syntax match omnimarkDataType "\v\c<%(markup\s+sink)"
syntax match omnimarkDataType "\v\c<%(markup\s+source)"
syntax keyword omnimarkDataType pattern
syntax keyword omnimarkDataType string
syntax match omnimarkDataType "\v\c<%(string\s+sink)"
syntax match omnimarkDataType "\v\c<%(string\s+source)"
syntax keyword omnimarkDataType stream
syntax keyword omnimarkDataType string
syntax keyword omnimarkDataType subdocument
syntax keyword omnimarkDataType switch
# }}}
# _ declaration/definition {{{
syntax keyword omnimarkDeclaration constant
syntax keyword omnimarkDeclaration context-translate
syntax keyword omnimarkDeclaration created by
syntax keyword omnimarkDeclaration cross-translate
syntax keyword omnimarkDeclaration declare
syntax match omnimarkDeclaration "\v\c<%(declare)%(\s+%(data-letters|function-library|heralded-names|markup-identification))?"
# Note: declare #error, #main-input, #main-output, #process-input,
#         #process-output, catch, data-letters, function-library, letters,
#         name-letters, no-default-io, opaque, record
#         - Those are all handed as separate keywords/matches
syntax match omnimarkDeclaration "\v\c<%(define\s+conversion-function)"
syntax match omnimarkDeclaration "\v\c<%(define\s+external)\s+%(source\s+)?%(function)"
  # in function-library is part of an external function
  syntax match omnimarkDeclaration "\v\c<%(in\s+function-library)"
syntax match omnimarkDeclaration "\v\c<%(define\s+external\s+output)"
syntax match omnimarkDeclaration "\v\c<%(define\s+function)"
syntax match omnimarkDeclaration "\v\c<%(define\s+\w+function)"
  # Example: define integer function add (value integer x, value ...)
  syntax keyword omnimarkDeclaration value
syntax match omnimarkDeclaration "\v\c<%(define\s+infix-function)"
syntax match omnimarkDeclaration "\v\c<%(define\s+overloaded\s+function)"
syntax match omnimarkDeclaration "\v\c<%(define\s+string\s+sink\s+function)"
syntax match omnimarkDeclaration "\v\c<%(define\s+string\s+source\s+function)"
# Some combinations are missed, so the general, 'define', is needed too:
syntax match omnimarkDeclaration "\v\c<%(define\s+)"
syntax keyword omnimarkDeclaration delimiter
syntax keyword omnimarkDeclaration domain-bound
syntax keyword omnimarkDeclaration down-translate
syntax keyword omnimarkDeclaration dynamic
syntax keyword omnimarkDeclaration elsewhere
syntax keyword omnimarkDeclaration escape
syntax match omnimarkDeclaration "\v\c<%(export\s+as\s+opaque)"
syntax keyword omnimarkDeclaration export
syntax keyword omnimarkDeclaration field
syntax keyword omnimarkDeclaration function
syntax keyword omnimarkDeclaration global
syntax match omnimarkDeclaration "\v\c<%(group)%(s)?"
syntax keyword omnimarkDeclaration import
syntax match omnimarkDeclaration "\v\c<%(include)%(-guard)?"
syntax match omnimarkDeclaration "\v\c<%(initial)%(-size)?"
syntax keyword omnimarkDeclaration letters
syntax keyword omnimarkDeclaration library
syntax keyword omnimarkDeclaration local
syntax match omnimarkDeclaration "\v\c<%(macro)%(-end)?"
  # macros can take an arg and/or a token (or args or tokens)
  syntax keyword omnimarkDeclaration arg
  syntax keyword omnimarkDeclaration token
syntax keyword omnimarkDeclaration modifiable
syntax keyword omnimarkDeclaration module
syntax keyword omnimarkDeclaration name-letters
syntax match omnimarkDeclaration "\v\c<%(namecase\s+entity)"
syntax match omnimarkDeclaration "\v\c<%(namecase\s+general)"
syntax keyword omnimarkDeclaration newline
syntax keyword omnimarkDeclaration no-default-io
syntax keyword omnimarkDeclaration opaque
syntax keyword omnimarkDeclaration optional
syntax keyword omnimarkDeclaration overriding
syntax match omnimarkDeclaration "\v\c<%(prefixed\s+by)"
syntax keyword omnimarkDeclaration read-only
syntax keyword omnimarkDeclaration record
syntax keyword omnimarkDeclaration remainder
syntax keyword omnimarkDeclaration require
syntax keyword omnimarkDeclaration save
syntax keyword omnimarkDeclaration save-clear
syntax keyword omnimarkDeclaration silent-referent
syntax keyword omnimarkDeclaration size
syntax keyword omnimarkDeclaration supply
syntax keyword omnimarkDeclaration symbol
syntax keyword omnimarkDeclaration unprefixed
syntax keyword omnimarkDeclaration up-translate
syntax keyword omnimarkDeclaration use
syntax keyword omnimarkDeclaration variable
syntax keyword omnimarkDeclaration write-only
# }}}
# _ element qualifier {{{
syntax keyword omnimarkElementQualifier ancestor
syntax keyword omnimarkElementQualifier doctype
syntax keyword omnimarkElementQualifier document-element
syntax match omnimarkElementQualifier "\v\c<%(open\s+element)"
syntax keyword omnimarkElementQualifier parent
syntax keyword omnimarkElementQualifier preparent
syntax keyword omnimarkElementQualifier previous
# }}}
# _ modifier {{{
syntax match omnimarkModifier "\v\c<%(#base)"
syntax match omnimarkModifier "\v\c<%(#full)"
syntax match omnimarkModifier "\v\c<%(#xmlns)"
syntax keyword omnimarkModifier append
syntax keyword omnimarkModifier binary-input
syntax keyword omnimarkModifier binary-mode
syntax keyword omnimarkModifier binary-output
syntax keyword omnimarkModifier break-width
syntax keyword omnimarkModifier buffered
syntax match omnimarkModifier "\v\c<%(declare\s+#main-output\s+has\s+domain-free)"
syntax keyword omnimarkModifier defaulting
syntax keyword omnimarkModifier domain-free
syntax keyword omnimarkModifier notation
# of may be standalone, e.g., data-attribute colwidth of (attribute name)
syntax match omnimarkModifier "\v\c<%(of)%(\s%(ancestor|doctype|element|open element|%(pre)?parent))?"
syntax keyword omnimarkModifier referents-allowed
syntax keyword omnimarkModifier referents-displayed
syntax keyword omnimarkModifier referents-not-allowed
syntax keyword omnimarkModifier text-mode
syntax keyword omnimarkModifier unbuffered
# : (field selection operator) [not included]
# ` (keyword access character) [not included]
# }}}
# _ operator {{{
syntax keyword omnimarkOperator !
syntax keyword omnimarkOperator not
syntax keyword omnimarkOperator !=
# 'isnt equal' is handled by separate keywords
syntax keyword omnimarkOperator !==
syntax match omnimarkOperator "\%(#!\)"
syntax match omnimarkOperator "\v\c<%(#empty)"
# Example: usemap is #none:
syntax match omnimarkOperator "\v\c<%(#none)"
syntax keyword omnimarkOperator %
syntax keyword omnimarkOperator format
syntax keyword omnimarkOperator &
syntax keyword omnimarkOperator and
syntax keyword omnimarkOperator *
syntax keyword omnimarkOperator times
syntax keyword omnimarkOperator **
syntax keyword omnimarkOperator power
syntax keyword omnimarkOperator +
syntax keyword omnimarkOperator plus
syntax keyword omnimarkOperator -
syntax keyword omnimarkOperator minus
syntax keyword omnimarkOperator negate
syntax keyword omnimarkOperator /
syntax keyword omnimarkOperator divide
syntax keyword omnimarkOperator <
syntax keyword omnimarkOperator less-than
syntax keyword omnimarkOperator greater-equal
syntax keyword omnimarkOperator <=
syntax keyword omnimarkOperator less-equal
syntax keyword omnimarkOperator =
syntax keyword omnimarkOperator equal
# 'is equal' is handled by separate keywords
syntax keyword omnimarkOperator ==
syntax match omnimarkOperator "\v<[=][>]\s*"
syntax keyword omnimarkOperator >
syntax keyword omnimarkOperator greater-than
syntax keyword omnimarkOperator >=
syntax keyword omnimarkOperator greater-equal
syntax keyword omnimarkOperator abs
syntax keyword omnimarkOperator active
syntax keyword omnimarkOperator attribute
  # attribute is defaulted, implied, specified: split to single keywords
  # because it can be isnt too.  Similarly for ancestor (is/isnt)
  # with ancestor being an element qualifier.
  # Tests for element attributes, e.g.:
  # do when attribute myid is (id | idref | idrefs)
  #                                    cdata (already omnimarkPattern)
  #                                    name (already omnimarkOperator)
  syntax keyword omnimarkAttributeType names
  # number: match rather than keyword since "number of" is an operator
  syntax match omnimarkAttributeType "\v\c<%(number)"
  syntax keyword omnimarkAttributeType numbers
  syntax keyword omnimarkAttributeType nmtoken
  syntax keyword omnimarkAttributeType nmtokens
  syntax keyword omnimarkAttributeType nutoken
  syntax keyword omnimarkAttributeType nutokens
  syntax keyword omnimarkAttributeType id
  syntax keyword omnimarkAttributeType idref
  syntax keyword omnimarkAttributeType idrefs
  #                                    notation (already omnimarkModifier)
  #                                    entity (already omnimarkOperator)
  #                                    entities (already omnimarkOperator)
syntax keyword omnimarkOperator base
syntax keyword omnimarkOperator binary
syntax keyword omnimarkOperator cast
syntax keyword omnimarkOperator ceiling
syntax keyword omnimarkOperator children
syntax keyword omnimarkOperator compiled-date
syntax keyword omnimarkOperator complement
syntax match omnimarkOperator "\v\c<%(content\s+of)"
syntax keyword omnimarkOperator create-attribute-declaration
syntax keyword omnimarkOperator create-element-declaration
syntax keyword omnimarkOperator create-element-event
syntax keyword omnimarkOperator create-processing-instruction-event
syntax keyword omnimarkOperator create-specified-attribute
syntax keyword omnimarkOperator create-unspecified-attribute
syntax keyword omnimarkOperator creating
syntax match omnimarkOperator "\v\c<%(creator\s+of)"
syntax keyword omnimarkOperator data-attribute
syntax keyword omnimarkOperator date
syntax match omnimarkOperator "\v\c<%(declaration\s+of)"
syntax keyword omnimarkOperator declared-elements
syntax keyword omnimarkOperator declared-general-entities
syntax keyword omnimarkOperator declared-parameter-entities
syntax keyword omnimarkOperator defaulted
syntax keyword omnimarkOperator difference
syntax match omnimarkOperator "\v\c<%(doctype\s+is)"
syntax keyword omnimarkOperator drop
syntax match omnimarkOperator "\v\c<%(element\s+is)"
syntax match omnimarkOperator "\v\c<%(elements\s+of)"
syntax match omnimarkOperator "\v\c<%(entity\s+is)"
syntax keyword omnimarkOperator except
syntax keyword omnimarkOperator exists
syntax keyword omnimarkOperator exp
syntax keyword omnimarkOperator external-function
syntax keyword omnimarkOperator file
syntax keyword omnimarkOperator floor
syntax match omnimarkOperator "\v\c<%(function-library\s+of)"
syntax keyword omnimarkOperator has
syntax keyword omnimarkOperator hasnt
  # 'has'/'hasnt' (before 'key'/'^' or 'item'/'@' (other obscure things too?):
  syntax keyword omnimarkOperator key
  syntax match omnimarkOperator "\v[\^]"
  # 'item' is addressed elsewhere - @ needs to be a match, not keyword
  syntax match omnimarkOperator "\v<[@]"
syntax keyword omnimarkOperator implied
syntax keyword omnimarkOperator in-codes
syntax keyword omnimarkOperator is
syntax keyword omnimarkOperator isnt
  # 'is'/'isnt' (usually before, or sometime after, e.g., 'content isnt'):
  syntax keyword omnimarkOperator attached
  syntax keyword omnimarkOperator buffer
  syntax keyword omnimarkOperator catchable
  syntax keyword omnimarkOperator cdata-entity
  syntax keyword omnimarkOperator closed
  syntax keyword omnimarkOperator conref
  syntax keyword omnimarkOperator content
  # 'content is' ... empty, any, cdata, rcdata, mixed, conref
  syntax keyword omnimarkOperator default-entity
  syntax keyword omnimarkOperator directory
  # 'entity'/'entities', e.g., 'attribute x is (entity | entities)'
  syntax match omnimarkOperator "\v\c<%(entit)%(y|ies)"
  syntax keyword omnimarkOperator external
  # E.g., 'open s as external-output-function' (v. buffer, etc.) '-call'?
  syntax match omnimarkOperator "\v\c<%(external-output-function)%(-call)?"
  syntax keyword omnimarkOperator file
  syntax keyword omnimarkOperator general
  syntax keyword omnimarkOperator in-library
  syntax keyword omnimarkOperator internal
  syntax keyword omnimarkOperator keyed
  syntax keyword omnimarkOperator markup-parser
  syntax keyword omnimarkOperator ndata-entity
  # These are not to be confused with the omnimarkAction, 'open'
  syntax match omnimarkOperator "\v\c<%(is\s+open)"
  syntax match omnimarkOperator "\v\c<%(isnt\s+open)"
  syntax keyword omnimarkOperator parameter
  syntax keyword omnimarkOperator past
  syntax keyword omnimarkOperator public
  syntax keyword omnimarkOperator readable
  # These are not to be confused with the omnimarkAction, 'referent'
  syntax match omnimarkOperator "\v\c<%(is\s+referent)"
  syntax match omnimarkOperator "\v\c<%(isnt\s+referent)"
  syntax keyword omnimarkOperator sdata-entity
  # Deprecated form - markup-parser is recommended
  syntax keyword omnimarkOperator sgml-parser
  syntax keyword omnimarkOperator subdoc-entity
  syntax keyword omnimarkOperator system
  syntax keyword omnimarkOperator thrown
syntax match omnimarkOperator "\v\c<%(item)%(\sof)?%(\s%(data-)?attributes)?"
syntax match omnimarkOperator "\v\c<%(key\s+of)"
# 'key of attribute'/s not needed: 'key of' and 'attribute' are separate
# 'key of data-attribute'/s not needed: 'key of' &c. are separate
# 'key of referents' not needed: 'key of' and 'referents' are separate
syntax keyword omnimarkOperator last
syntax keyword omnimarkOperator lastmost
syntax match omnimarkOperator "\v\c<%(length\s*of)"
syntax keyword omnimarkOperator literal
syntax keyword omnimarkOperator ln
syntax keyword omnimarkOperator log10
syntax keyword omnimarkOperator lookahead
# 'lookahead not' not needed: 'lookahead' and 'not' are separate
syntax keyword omnimarkOperator mask
syntax keyword omnimarkOperator matches
syntax keyword omnimarkOperator modulo
syntax match omnimarkOperator "\v\c<%(name)>%(\s+of)?%(\s+current)?%(\s+element)?"
syntax keyword omnimarkOperator named
syntax match omnimarkOperator "\v\c<%(notation\s+equals)"
syntax match omnimarkOperator "\v\c<%(number\s+of)"
# 'number of attribute'/s not needed: 'number of' and 'attribute' are separate
# 'number of current elements' is not needed: 'number of' &c. are separate
syntax match omnimarkOperator "\v\c<%(number\s+of\s+current\s+subdocuments)"
# 'number of data-attribute'/s not needed: 'number of' &c. are separate
# 'number of referents' not needed: 'number of' and 'referents' are separate
syntax keyword omnimarkOperator occurrence
syntax match omnimarkOperator "\v\c<%(open\s+element\s+is)"
syntax match omnimarkOperator "\v\c<%(parent\s+is)"
syntax match omnimarkOperator "\v\c<%(preparent\s+is)"
syntax match omnimarkOperator "\v\c<%(previous\s+is)"
syntax match omnimarkOperator "\v\c<%(public-identifier\s+of)"
syntax match omnimarkOperator "\v\c<%(referents\s+has\s+key)"
syntax match omnimarkOperator "\v\c<%(referents\s+is\s+attached)"
syntax keyword omnimarkOperator round
syntax keyword omnimarkOperator shift
syntax keyword omnimarkOperator specified
syntax keyword omnimarkOperator sqrt
# E.g., last proper? subelement _element-qualifier_ is/isnt
syntax keyword omnimarkOperator subelement
syntax keyword omnimarkOperator system-call
syntax match omnimarkOperator "\v\c<%(system-identifier\s+of)"
syntax keyword omnimarkOperator status
  # E.g., 'status ... is (proper | inclusion)'
  syntax keyword omnimarkOperator inclusion
  syntax keyword omnimarkOperator proper
  # E.g., 'last content {element-qualifier} is #DATA'
  syntax match omnimarkOperator "\v\c<%(#data)"
syntax keyword omnimarkOperator take
# 'this' only appears before 'referent', so requires no standalone 'this'
syntax match omnimarkOperator "\v\c<%(this\s*referent)"
syntax keyword omnimarkOperator to
syntax keyword omnimarkOperator truncate
syntax keyword omnimarkOperator ul
syntax keyword omnimarkOperator union
syntax keyword omnimarkOperator usemap
syntax keyword omnimarkOperator valued
syntax keyword omnimarkOperator writable
syntax keyword omnimarkOperator xmlns-name
syntax match omnimarkOperator "\v%(\s)%([\\])%(\s)"
syntax match omnimarkOperator "\v%(\s)([_])%(\s)"
syntax match omnimarkOperator "\v%(\s)([\|])%(\s)"
syntax keyword omnimarkOperator or
syntax match omnimarkOperator "\v%(\s)([\|][\|])%(\s)"
syntax keyword omnimarkOperator join
syntax match omnimarkOperator "\v%(\s)([\|][\|][\*])%(\s)"
syntax keyword omnimarkOperator repeated
syntax match omnimarkOperator "\v%(\s)([~])"
# }}}
# _ pattern {{{
syntax match omnimarkPattern "\v%(\s)%(\=\|)"
syntax match omnimarkPattern "\v\c<%(any)%(\+%(\+)?|*%(*)?|\?)?"
syntax match omnimarkPattern "\v\c<%(any-text)%(\+|*|\?)?"
syntax match omnimarkPattern "\v\c<%(blank)%(\+|*|\?)?"
syntax keyword omnimarkPattern cdata
syntax keyword omnimarkPattern content-end
syntax keyword omnimarkPattern content-start
syntax match omnimarkPattern "\v\c<%(digit)%(\+|*|\?)?"
syntax keyword omnimarkPattern empty
syntax match omnimarkPattern "\v\c<%(lc)%(\+|*|\?)?"
syntax match omnimarkPattern "\v\c<%(letter)%(\+|*|\?)?"
syntax keyword omnimarkPattern line-end
syntax keyword omnimarkPattern line-start
syntax keyword omnimarkPattern mixed
syntax keyword omnimarkPattern non-cdata
syntax keyword omnimarkPattern non-sdata
syntax keyword omnimarkPattern null
syntax keyword omnimarkPattern pcdata
syntax keyword omnimarkPattern rcdata
syntax keyword omnimarkPattern sdata
syntax match omnimarkPattern "\v\c<%(space)%(\+|*|\?)?"
syntax match omnimarkPattern "\v\c<%(text)%([[:space:]\n])"
syntax match omnimarkPattern "\v\c<%(uc)%(\+|*|\?)?"
syntax keyword omnimarkPattern unanchored
syntax keyword omnimarkPattern value-end
syntax keyword omnimarkPattern value-start
syntax match omnimarkPattern "\v\c<%(white-space)%(\+|*|\?)?"
syntax keyword omnimarkPattern word-end
syntax keyword omnimarkPattern word-start
syntax match omnimarkPattern "\v%(\s)%(\|\=)"
# }}}
# _ rule {{{
syntax keyword omnimarkRule data-content
syntax keyword omnimarkRule document-end
syntax keyword omnimarkRule document-start
syntax keyword omnimarkRule document-type-declaration
syntax keyword omnimarkRule dtd-end
syntax keyword omnimarkRule dtd-start
syntax keyword omnimarkRule element
syntax keyword omnimarkRule epilog-start
syntax keyword omnimarkRule external-entity
syntax keyword omnimarkRule external-data-entity
syntax keyword omnimarkRule external-text-entity
syntax match omnimarkRule "\v\c<%(external-text-entity\s+#document)"
syntax keyword omnimarkRule find
syntax keyword omnimarkRule find-end
syntax keyword omnimarkRule find-start
syntax keyword omnimarkRule insertion-break
syntax keyword omnimarkRule invalid-data
syntax match omnimarkRule "\v\c<%(marked-section)%(\s+%(cdata|ignore|include-%(end|start)|rcdata))?"
syntax keyword omnimarkRule markup-comment
syntax keyword omnimarkRule markup-error
syntax keyword omnimarkRule process
syntax keyword omnimarkRule process-end
syntax keyword omnimarkRule processing-instruction
syntax keyword omnimarkRule process-start
syntax keyword omnimarkRule prolog-end
syntax keyword omnimarkRule prolog-in-error
syntax keyword omnimarkRule replacement-break
syntax keyword omnimarkRule sgml-comment
syntax keyword omnimarkRule sgml-declaration-end
syntax keyword omnimarkRule sgml-error
syntax keyword omnimarkRule translate
syntax keyword omnimarkRule xmlns-change
# }}}
# Libraries {{{
# ombase64
syntax match omnimarkLibrary "\v\c<%(base64[.])%([orw])%([[:print:]])+"
# ombcd
# (NB: abs, ceiling, exp, floor, ln, log10, round, sqrt, and truncate
#  are all operators)
syntax match omnimarkLibrary "\v\c<%(ombcd-version)"
# ombessel
syntax match omnimarkLibrary "\v\c<%(j0|j1|jn|y0|y1|yn)"
# ombig5
syntax match omnimarkLibrary "\v\c<%(big5[.])%([orw])%([[:print:]])+"
# omblowfish and omffblowfish
syntax match omnimarkLibrary "\v\c<%(blowfish[.])%([deorsw])%(\a|-)+"
# omcgi
syntax match omnimarkLibrary "\v\c<%(cgiGet)%([EQ])\a+"
# omff8859
syntax match omnimarkLibrary "\v\c<%(iso8859[.])%([iorw])%([[:print:]])+"
# omfloat
# (NB: uses same operator names as ombcd except for the following)
syntax match omnimarkLibrary "\v\c<%(is-nan)"
syntax match omnimarkLibrary "\v\c<%(omfloat-version)"
# omdate
syntax match omnimarkLibrary "\v\c<%(add-to-ymdhms)"
syntax match omnimarkLibrary "\v\c<%(arpadate-to-ymdhms)"
syntax match omnimarkLibrary "\v\c<%(format-ymdhms)"
syntax match omnimarkLibrary "\v\c<%(now-as-ymdhms)"
syntax match omnimarkLibrary "\v\c<%(round-down-ymdhms)"
syntax match omnimarkLibrary "\v\c<%(round-up-ymdhms)"
syntax match omnimarkLibrary "\v\c<%(ymdhms-)%([adjmst])%(\a|-)+"
syntax match omnimarkLibrary "\v\c<%(ymd-weekday)"
# omdb
syntax match omnimarkLibrary "\v\c<%(db[.])%([acdefimopqrstuw])%(\a|-|1)+"
# omffeuc
syntax match omnimarkLibrary "\v\c<%(euc[.])%([orw])%(\a|-)+"
# omffjis
syntax match omnimarkLibrary "\v\c<%(jis[.])%([orw])%(\a|-)+"
# omffutf16 and omffutf32
syntax match omnimarklibrary "\v\c<%(utf)%(16|32)%([.])%([orw])%(\a|-)+"
# omfloat
syntax match omnimarklibrary "\v\C<%(FP_)%([a-z])+\d?"
# omfsys
syntax match omnimarkLibrary "\v\C<%(FS_)%([CDGLMR])\a+"
# omftp
syntax match omnimarkLibrary "\v\c<%(FTP)%([CIL])\a+"
# omhttp
syntax match omnimarkLibrary "\v\c<%(Http)%([CORS])\a+"
# omiobuf
syntax match omnimarkLibrary "\v\c<%(iobuf[.])%([brw])\a+"
# omldap
syntax match omnimarkLibrary "\v\c<%(ldap[.])%([acdemnors])%(\a|-)+"
# omprocess
syntax match omnimarkLibrary "\v\c<%(command-line)"
syntax match omnimarkLibrary "\v\c<%(executable-name)"
syntax match omnimarkLibrary "\v\c<%(execute)"
syntax match omnimarkLibrary "\v\c<%(glob)"
syntax match omnimarkLibrary "\v\c<%(omprocess-version)"
# omrandom
syntax match omnimarkLibrary "\v\c<%(random[.])%([eosu])%(\a|-)+"
# omunicode
syntax match omnimarkLibrary "\v\c<%(unicode[.])%([bgo])%(\a|-)+"
# omutf8
syntax match omnimarkLibrary "\v\c<%(utf8[.])%([bceilmos])%(\a|8|-)+"
# omioe/omfio
syntax match omnimarkLibrary "\v\c<%(get-exception-status)"
syntax match omnimarkLibrary "\v\c<%(io-exception-text)"
syntax match omnimarkLibrary "\v\c<%(new-io-exception)"
syntax match omnimarkLibrary "\v\c<%(set-voluntary-end-exception)"
syntax match omnimarkLibrary "\v\c<%(%(Big5|euc|jis|sjis|utf16)%(-))?%(%(in|out)put-file)"
# omioprotocol
syntax match omnimarkLibrary "\v\c<%(IOProtocol)%([EILMS])\a+"
# ommail
syntax match omnimarkLibrary "\v\c<%(Mail)%([ILO]\a+)"
# omtrig
syntax match omnimarkLibrary "\v\c<%(a)?%(cos%(h)?|sin%(h)?|tan%(h|2)?|hypot)"
# omnetutl
syntax match omnimarkLibrary "\v\c<%(from-net-long)"
syntax match omnimarkLibrary "\v\c<%(net-long)"
syntax match omnimarkLibrary "\v\c<%(NET)%([GIL]\a+)"
syntax match omnimarkLibrary "\v\c<%(to-net-long)"
# omnetutil
syntax match omnimarkLibrary "\v\c<%(netutil[.])%([fhnot]%(\a|3|2|-)+)"
# omoci
syntax match omnimarkLibrary "\v\c<%(OCI_)%([GLoS]\a+)"
# omodbc
syntax match omnimarkLibrary "\v\c<%(SQL)%([_])?%([ABCDEFGLMNPRST][[:alpha:]]+)"
# omsocat
syntax match omnimarkLibrary "\v\c<%(socat-[clr-])%([[:alpha:]]+)"
# omsort
syntax match omnimarkLibrary "\v\c<%(sort[.][os])%(\a|-)+"
# omutil
syntax match omnimarkLibrary "\v\c<%(UTIL[_.][EGLmopRSU])%(\a|-)+"
# omvfs
syntax match omnimarkLibrary "\v\c<%(vfs[.][cdflmorstuw])%(\a|-)+"
# tcp
syntax match omnimarkLibrary "\v\c<%(TCP)%([.])?%([acdegilmoprstw][[:alpha:]-]+)"
# uri
syntax match omnimarkLibrary "\v\c<%(uri[.])%([ceopr])%(\a|-)+"
# wsb
syntax match omnimarkLibrary "\v\c<%(wsb[.])%([acdfhrsw])%(\a|-)+"
# }}}
# Comments {{{
# -------
syntax region omnimarkComment start=";" end="$"
# }}}
# Strings and format-modifiers {{{
syntax region omnimarkString matchgroup=Normal start=+'+  end=+'+ skip=+%'+ contains=omnimarkEscape
syntax region omnimarkString matchgroup=Normal start=+"+  end=+"+ skip=+%"+ contains=omnimarkEscape
  # This handles format items inside strings:
  # NB: escape _quoted-character_ allows a new character to be used to
  # indicate a special character or a format item, rather than the normal %.
  # The use of escape is deprecated in general, because it leads to
  # non-standard OmniMark code that can be difficult to understand. It has
  # not be handled here as it would be almost impossible to do so.
  # dynamic: %a %b %d %g %i %p %q %v %x %w %y
  #         a - integer data type formatting
  #         b - integer data type formatting
  #         c - parsed data formatting
  #         d - integer data type formatting, BCD data type formatting
  #         g - string data formatting
  #         i - integer data type formatting
  #         p - parsed data formatting
  #         q - parsed data formatting
  #         v - parsed data formatting
  #         x - deprecated format command used instead of g
  #         y - symbol declaration
  #         @ - macro arguments
  # static: %% %_ %n %t %# %) %" %' %/ %[ %] %@%%
  #         %% - insert an explicit percent sign
  #         %_ - insert an explicit space character
  #         %n - insert an explicit newline character
  #         %t - insert an explicit tab character
  #         %0# through to %255# - insert an explicit byte with given value
  #         %{...}—a sequence of characters, e.g., %16r{0d, 0a}
  #         %#—insert an explicit octothorpe character (#)
  #         %)—insert an explicit closing parenthesis
  #         %"—insert an explicit double quote character
  #         %'—insert an explicit single quote character
  #         %/—indicates a point where line breaking can occur
  #         %[ and %]—protect the delimited text from line breaking
  #         %@%—insert an explicit percent sign inside a macro expansion
  # % format-modifier a
  syntax match omnimarkEscape contained =\v\C%(\%%(%(%(\d)*%(f))?%(j|k|l|u|w)*)?a)=
  # % format-modifier b
  syntax match omnimarkEscape contained =\v\C%(\%%(%(%(\d)*%(f))?%(\d)?)?b)=
  # % format-modifier c
  syntax match omnimarkEscape contained =\v\C%(%(\%%(h|l|u|s|z)*c))=
  # % format-modifier d
  syntax match omnimarkEscape contained =\v\C%(\%%(%(%(\d)*%(f|r|s)?)*%(j|k|l|u|z)*)?d)=
  # % format-modifier g
  syntax match omnimarkEscape contained =\v\C%(\%%(%(%(\d)*%(f|r|s)?)*%(j|k|l|u|z)*)?g)=
  # % format-modifier
  syntax match omnimarkEscape contained =\v\C%(%(\%[abdgipqvxwy%_nt#)"'/\[\]])|%(\%\@\%)|%(\%\d+#)|\%%(\d+r[{]%([0-9A-z, ]+)}))+=
  # }}}
# Number {{{
syntax match omnimarkNumber "\v([[:alpha:]]+)@<![-]?\d+([.]\d+)?"
# }}}
# Define default highlighting {{{
highlight default link omnimarkAction		Statement
highlight default link omnimarkAttributeType	Structure
highlight default link omnimarkDataType		Type
highlight default link omnimarkBuiltinEntity	Identifier
highlight default link omnimarkBuiltinShelf	Identifier
highlight default link omnimarkCatchName	Exception
highlight default link omnimarkConstant		Constant
highlight default link omnimarkControlStructure	Conditional
highlight default link omnimarkDeclaration	Keyword
highlight default link omnimarkElementQualifier	Type
highlight default link omnimarkLibrary		Function
highlight default link omnimarkModifier		Keyword
highlight default link omnimarkOperator		Operator
highlight default link omnimarkPattern		Label
highlight default link omnimarkRule		Keyword
highlight default link omnimarkString		String
highlight default link omnimarkNumber		Number
highlight default link omnimarkComment		Comment
highlight default link omnimarkEscape		Special
highlight default link omnimarkNormal		Statement
# }}}
syntax sync fromstart
b:current_syntax = "omnimark"
# vim: cc=+1 et fdm=marker ft=vim sta sw=2 ts=8 tw=79
