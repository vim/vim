" Vim syntax file
" Language:	Vim syntax file for SNMPv1 and SNMPv2 MIB and SMI files
" Author:	David Pascoe <pascoedj@spamcop.net>
" Written:	Wed Jan 28 14:37:23 GMT--8:00 1998
" Last Changed:	Thu Feb 27 10:18:16 WST 2003

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if version >= 600
  setlocal iskeyword=@,48-57,_,128-167,224-235,-,:,=
else
  set iskeyword=@,48-57,_,128-167,224-235,-,:,=
endif

syn keyword mibImplicit ACCESS ANY AUGMENTS BEGIN BIT BITS BOOLEAN CHOICE
syn keyword mibImplicit COMPONENTS CONTACT-INFO DEFINITIONS DEFVAL
syn keyword mibImplicit DESCRIPTION DISPLAY-HINT END ENTERPRISE EXTERNAL FALSE
syn keyword mibImplicit FROM GROUP IMPLICIT IMPLIED IMPORTS INDEX
syn keyword mibImplicit LAST-UPDATED MANDATORY-GROUPS MAX-ACCESS
syn keyword mibImplicit MIN-ACCESS MODULE MODULE-COMPLIANCE MODULE-IDENTITY
syn keyword mibImplicit NOTIFICATION-GROUP NOTIFICATION-TYPE NOTIFICATIONS
syn keyword mibImplicit NULL OBJECT-GROUP OBJECT-IDENTITY OBJECT-TYPE
syn keyword mibImplicit OBJECTS OF OPTIONAL ORGANIZATION REFERENCE
syn keyword mibImplicit REVISION SEQUENCE SET SIZE STATUS SYNTAX
syn keyword mibImplicit TEXTUAL-CONVENTION TRAP-TYPE TRUE UNITS VARIABLES
syn keyword mibImplicit WRITE-SYNTAX ::=
syn keyword mibValue accessible-for-notify current DisplayString
syn keyword mibValue deprecated mandatory not-accessible obsolete optional
syn keyword mibValue read-create read-only read-write write-only INTEGER
syn keyword mibValue Counter Gauge IpAddress OCTET STRING experimental mib-2
syn keyword mibValue TimeTicks RowStatus TruthValue UInteger32 snmpModules
syn keyword mibValue Integer32 Counter32 TestAndIncr TimeStamp InstancePointer
syn keyword mibValue OBJECT IDENTIFIER Gauge32 AutonomousType Counter64
syn keyword mibValue PhysAddress TimeInterval MacAddress StorageType RowPointer
syn keyword mibValue TDomain TAddress ifIndex

" Epilogue SMI extensions
syn keyword mibEpilogue FORCE-INCLUDE EXCLUDE cookie get-function set-function
syn keyword mibEpilogue test-function get-function-async set-function-async
syn keyword mibEpilogue test-function-async next-function next-function-async
syn keyword mibEpilogue leaf-name
syn keyword mibEpilogue DEFAULT contained

syn match  mibComment		"\ *--.*$"
syn match  mibNumber		"\<['0-9a-fA-FhH]*\>"
syn region mibDescription start="\"" end="\"" contains=DEFAULT

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_mib_syn_inits")
  if version < 508
    let did_mib_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink mibImplicit	     Statement
  HiLink mibComment	     Comment
  HiLink mibConstants	     String
  HiLink mibNumber	     Number
  HiLink mibDescription      Identifier
  HiLink mibEpilogue	     SpecialChar
  HiLink mibValue	     Structure
  delcommand HiLink
endif

let b:current_syntax = "mib"

" vim: ts=8
