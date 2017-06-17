" Vim syntax file
" Language:	Haskell Cabal Build file
" Maintainer:	Vincent Berthoux <twinside@gmail.com>
" File Types:	.cabal
" Last Change:  2017 June 17
" v1.4: Add benchmark support, thanks to Simon Meier
" v1.3: Updated to the last version of cabal
"       Added more highlighting for cabal function, true/false
"       and version number. Also added missing comment highlighting.
"       Cabal known compiler are highlighted too.
"
" V1.2: Added cpp-options which was missing. Feature implemented
"       by GHC, found with a GHC warning, but undocumented. 
"       Whatever...
"
" v1.1: Fixed operator problems and added ftdetect file
"       (thanks to Sebastian Schwarz)
"
" v1.0: Cabal syntax in vimball format
"       (thanks to Magnus Therning)

" quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn match	cabalCategory	"\c\<executable\>"
syn match	cabalCategory	"\c\<library\>"
syn match	cabalCategory	"\c\<benchmark\>"
syn match	cabalCategory	"\c\<test-suite\>"
syn match	cabalCategory	"\c\<source-repository\>"
syn match	cabalCategory	"\c\<flag\>"
syn match	cabalCategory	"\c\<custom-setup\>"

syn keyword     cabalConditional    if else
syn match       cabalOperator       "&&\|||\|!\|==\|>=\|<="
syn keyword     cabalFunction       os arche impl flag
syn match       cabalComment    /--.*$/
syn match       cabalVersion    "\d\+\(\.\(\d\)\+\)\+\(\.\*\)\?"

syn match       cabalTruth      "\c\<true\>"
syn match       cabalTruth      "\c\<false\>"

syn match       cabalCompiler   "\c\<ghc\>"
syn match       cabalCompiler   "\c\<nhc\>"
syn match       cabalCompiler   "\c\<yhc\>"
syn match       cabalCompiler   "\c\<hugs\>"
syn match       cabalCompiler   "\c\<hbc\>"
syn match       cabalCompiler   "\c\<helium\>"
syn match       cabalCompiler   "\c\<jhc\>"
syn match       cabalCompiler   "\c\<lhc\>"

syn match	cabalStatement	/^\c\s*\<default-language\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<default-extensions\s*:/me=e-1

syn match	cabalStatement	/^\c\s*\<author\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<branch\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<bug-reports\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<build-depends\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<build-tools\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<build-type\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<buildable\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<c-sources\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<cabal-version\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<category\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<cc-options\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<copyright\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<cpp-options\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<data-dir\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<data-files\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<default\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<description\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<executable\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<exposed-modules\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<exposed\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<extensions\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<extra-doc-files\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<extra-lib-dirs\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<extra-libraries\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<extra-source-files\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<extra-tmp-files\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<for example\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<frameworks\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<ghc-options\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<ghc-prof-options\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<ghc-shared-options\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<homepage\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<hs-source-dirs\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<hugs-options\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<include-dirs\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<includes\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<install-includes\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<ld-options\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<license-file\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<license\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<location\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<main-is\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<maintainer\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<module\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<name\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<nhc98-options\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<other-extensions\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<other-modules\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<package-url\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<pkgconfig-depends\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<setup-depends\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<stability\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<subdir\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<synopsis\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<tag\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<tested-with\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<type\s*:/me=e-1
syn match	cabalStatement	/^\c\s*\<version\s*:/me=e-1

" Define the default highlighting.
" Only when an item doesn't have highlighting yet

hi def link cabalVersion       Number
hi def link cabalTruth         Boolean
hi def link cabalComment       Comment
hi def link cabalStatement     Statement
hi def link cabalCategory      Type
hi def link cabalFunction      Function
hi def link cabalConditional   Conditional
hi def link cabalOperator      Operator
hi def link cabalCompiler      Constant

let b:current_syntax = "cabal"

" vim: ts=8
