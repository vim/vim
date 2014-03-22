" Vim syntax file
" Language:	J
" Maintainer:	David Bürgin <676c7473@gmail.com>
" URL:		https://github.com/glts/vim-j
" Last Change:	2014-03-17

if exists('b:current_syntax')
  finish
endif

syntax case match
syntax sync minlines=50

syntax match jControl /\<\%(assert\|break\|case\|catch[dt]\=\|continue\|do\|else\%(if\)\=\|end\|fcase\|for\|if\|return\|select\|throw\|try\|whil\%(e\|st\)\)\./
syntax match jControl /\<\%(for\|goto\|label\)_\a\k*\./

" Standard library names defined in the z locale. A few verbs need to be
" defined with ":syntax match" because they would otherwise take precedence
" over the corresponding jControl items.
syntax keyword jNoun ARGV BINPATH CR CRLF DEL Debug EAV EMPTY FF FHS IF64 IFIOS IFJCDROID IFJHS IFQT IFRASPI IFUNIX IFWIN IFWINCE IFWINE IFWOW64 JB01 JBOXED JCHAR JCMPX JFL JINT JPTR JSIZES JSTR JTYPES JVERSION LF LF2 TAB UNAME UNXLIB adverb andurl conjunction dbhelp dyad libjqt monad noun verb
syntax keyword jAdverb define each every fapplylines inv inverse items leaf rows table
syntax keyword jConjunction bind cuts def on
syntax keyword jVerb AND Endian IFDEF Note OR XOR alpha17 alpha27 anddf android_exec_host andunzip apply boxopen boxxopen bx calendar cd cdcb cder cderx cdf charsub chopstring clear coclass cocreate cocurrent codestroy coerase cofind cofindv cofullname coinfo coinsert coname conames conew conl conouns conounsx copath copathnl copathnlx coreset costate cut cutLF cutopen cutpara datatype dbctx dberm dberr dbg dbjmp dblocals dblxq dblxs dbnxt dbq dbr dbret dbrr dbrrx dbrun dbs dbsig dbsq dbss dbst dbstack dbstk dbstop dbstopme dbstopnext dbstops dbtrace dbview deb debc delstring detab dfh dir dircompare dircompares dirfind dirpath dirss dirssrplc dirtree dirused dlb dltb dltbs dquote drop dropafter dropto dtb dtbs echo empty endian erase evtloop exit expand f2utf8 fappend fappends fboxname fc fcopynew fdir ferase fetch fexist fexists fgets file2url fixdotdot fliprgb fmakex foldpara foldtext fpathcreate fpathname fputs fread freadblock freadr freads frename freplace fsize fss fssrplc fstamp fstringreplace ftype fview fwrite fwritenew fwrites getargs getdate getenv getqtbin hfd hostpathsep ic install iospath isatty isotimestamp isutf8 jcwdpath joinstring jpathsep jsystemdefs list ljust load loadd mema memf memr memw nameclass namelist names nc nl pick quote require rjust rplc script scriptd setbreak show sign sminfo smoutput sort split splitnostring splitstring ss startupandroid startupconsole startupide stderr stdin stdout stringreplace symdat symget symset take takeafter taketo timespacex timestamp timex tmoutput toCRLF toHOST toJ todate todayno tolower topara toupper tsdiff tsrep tstamp type ucp ucpcount unxlib usleep utf8 uucp valdate wcsize weekday weeknumber weeksinyear winpathsep
syntax match   jVerb /\<\%(assert\|break\|do\)\>\.\@!/

syntax region jString oneline start=/'/ skip=/''/ end=/'/

" Number patterns. Matching J numbers is difficult. The regular expression
" used for the general case roughly embodies this grammar sketch:
"
"     BASE     := /_?\d+(\.\d*)?([eE]_?\d+)?/
"     RATIONAL := BASE  |  BASE r BASE
"     COMPLEX  := BASE  |  BASE (j|a[dr]) BASE
"     JNUMBER  := RATIONAL  |  RATIONAL [px] RATIONAL  |  COMPLEX  |  COMPLEX [px] COMPLEX
"
" The grammar is implemented as shown in this pseudo-regexp:
"
"        base         rational                       complex                       remainder
"     /\< B  (  [r]B ([px]B([r]B)?)?  |  (j|a[dr])B ([px]B((j|a[dr])B)?)?  |  [px]B ((j|a[dr]|r)B)?  )?/
"
" All in all, a compromise between correctness and practicality had to be
" made. See http://www.jsoftware.com/help/dictionary/dcons.htm for reference.
syntax match jNumber /\<_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\%(\%(r_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\%([px]_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\%(r_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\)\=\)\=\)\|\%(\%(j\|a[dr]\)_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\%([px]_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\%(\%(j\|a[dr]\)_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\)\=\)\=\)\|\%([px]_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\%(\%(j\|a[dr]\|r\)_\=\d\+\%(\.\d*\)\=\%([eE]_\=\d\+\)\=\)\=\)\)\=/
syntax match jNumber /\<_\=\d\+\%([eE]\d\+\)\=b_\=[0-9a-z]\+/
syntax match jNumber /\<__\=\>/
syntax match jNumber /\<_\=\d\+x\>/

syntax keyword jTodo TODO FIXME XXX contained
syntax match jComment /NB\..*$/ contains=jTodo,@Spell
syntax match jSharpBang /\%^#!.*$/

highlight default link jControl Statement
highlight default link jNoun Identifier
highlight default link jAdverb Identifier
highlight default link jConjunction Identifier
highlight default link jVerb Function
highlight default link jString String
highlight default link jNumber Number
highlight default link jTodo Todo
highlight default link jComment Comment
highlight default link jSharpBang PreProc

let b:current_syntax = 'j'
