" Vim syntax file
" Language:		HTML
" Maintainer:		Doug Kearns <dougkearns@gmail.com>
" Previous Maintainers: Jorge Maldonado Ventura <jorgesumle@freakspot.net>
"			Claudio Fleiner <claudio@fleiner.com>
" Last Change:		2026 Sep 18

" See :help html.vim for some comments and a description of the options

" quit when a syntax file was already loaded
if !exists("main_syntax")
  if exists("b:current_syntax")
    finish
  endif
  let main_syntax = 'html'
endif

let s:cpo_save = &cpo
set cpo&vim

syntax spell toplevel

syn include @htmlXml syntax/xml.vim
unlet b:current_syntax

syn case ignore

" mark illegal characters
syn match htmlError "[<>&]"

" tags
syn region  htmlString	 contained start=+"+ end=+"+ contains=htmlSpecialChar,javaScriptExpression,@htmlPreproc
syn region  htmlString	 contained start=+'+ end=+'+ contains=htmlSpecialChar,javaScriptExpression,@htmlPreproc
syn match   htmlValue	 contained "=[\t ]*[^'" \t>][^ \t>]*"hs=s+1   contains=javaScriptExpression,@htmlPreproc
syn region  htmlEndTag		   start=+</+	   end=+>+ contains=htmlTagN,htmlTagError
syn region  htmlTag		   start=+<[^/]+   end=+>+ fold contains=htmlTagN,htmlString,htmlArg,htmlValue,htmlTagError,htmlEvent,htmlCssDefinition,@htmlPreproc,@htmlArgCluster
syn match   htmlTagN	 contained +<\s*[-a-zA-Z0-9_:]\++hs=s+1 contains=htmlTagName,htmlSpecialTagName,@htmlTagNameCluster
syn match   htmlTagN	 contained +</\s*[-a-zA-Z0-9_:]\++hs=s+2 contains=htmlTagName,htmlSpecialTagName,@htmlTagNameCluster
syn match   htmlTagError contained "[^>]<"ms=s+1

" generic tag name and attribute matching
syn match   htmlTagName	contained "\h[-0-9A-Za-z_:]*"
syn match   htmlArg	contained "[@:]\?\h[-0-9A-Za-z_:.]*\%(\_s*=\)\@="
" boolean attributes (no value, e.g. checked, disabled, x-transition)
syn match   htmlArg	contained "[@:]\?\h[-0-9A-Za-z_:.]*\%(\_s*[>\s/]\)\@="

" backward compat: special/math/svg tag name groups
syn match   htmlSpecialTagName	contained "\%(script\|style\)\>"
syn match   htmlMathTagName	contained "math\>" containedin=htmlTagN
syn match   htmlSvgTagName	contained "svg\>" containedin=htmlTagN

" svg and math regions (allow HTML inside)
syn region  htmlMath start=+<math\>\_[^>]*>+ end=+</math>+ contains=@htmlXml,@htmlTop transparent keepend
syn region  htmlSvg  start=+<svg\>\_[^>]*>+  end=+</svg>+  contains=@htmlXml,@htmlTop transparent keepend

syn cluster xmlTagHook	add=htmlMathTagName,htmlSvgTagName

" special characters
syn match htmlSpecialChar "&#\=[0-9A-Za-z]\{1,32};"

" Comments (the real ones or the old netscape ones)
if exists("html_wrong_comments")
  syn region htmlComment	start=+<!--+	end=+--\s*>+	contains=@Spell
else
  " The HTML 5.2 syntax 8.2.4.41: bogus comment is parser error; browser skips until next &gt
  syn region htmlComment	start=+<!+	end=+>+		contains=htmlCommentError keepend
  " Idem 8.2.4.42,51: Comment starts with <!-- and ends with -->
  " Idem 8.2.4.43,44: Except <!--> and <!---> are parser errors
  " Idem 8.2.4.52: dash-dash-bang (--!>) is error ignored by parser, also closes comment
  syn region htmlComment matchgroup=htmlComment start=+<!--\%(-\?>\)\@!+	end=+--!\?>+	contains=htmlCommentNested,@htmlPreproc,@Spell keepend
  " Idem 8.2.4.49: nested comment is parser error, except <!--> is all right
  syn match htmlCommentNested contained "<!-->\@!"
  syn match htmlCommentError  contained "[^><!]"
endif
syn region htmlDoctype	start=+<!DOCTYPE+	end=+>+ keepend

" server-parsed commands
syn region htmlPreProc start=+<!--#+ end=+-->+ contains=htmlPreStmt,htmlPreError,htmlPreAttr
syn match htmlPreStmt contained "<!--#\%(config\|echo\|exec\|fsize\|flastmod\|include\|printenv\|set\|if\|elif\|else\|endif\|geoguide\)\>"
syn match htmlPreError contained "<!--#\S*"ms=s+4
syn match htmlPreAttr contained "\w\+=[^"]\S\+" contains=htmlPreProcAttrError,htmlPreProcAttrName
syn region htmlPreAttr contained start=+\w\+="+ skip=+\\\\\|\\"+ end=+"+ contains=htmlPreProcAttrName keepend
syn match htmlPreProcAttrError contained "\w\+="he=e-1
syn match htmlPreProcAttrName contained "\%(expr\|errmsg\|sizefmt\|timefmt\|var\|cgi\|cmd\|file\|virtual\|value\)="he=e-1

if !exists("html_no_rendering")
  " rendering
  syn cluster htmlTop contains=@Spell,htmlTag,htmlEndTag,htmlSpecialChar,htmlPreProc,htmlComment,htmlLink,javaScript,htmlVbScript,@htmlPreproc

  syn region htmlStrike start="<del\>" end="</del\_s*>"me=s-1 contains=@htmlTop
  syn region htmlStrike start="<s\>" end="</s\_s*>"me=s-1 contains=@htmlTop
  syn region htmlStrike start="<strike\>" end="</strike\_s*>"me=s-1 contains=@htmlTop

  syn region htmlBold start="<b\>" end="</b\_s*>"me=s-1 contains=@htmlTop,htmlBoldUnderline,htmlBoldItalic
  syn region htmlBold start="<strong\>" end="</strong\_s*>"me=s-1 contains=@htmlTop,htmlBoldUnderline,htmlBoldItalic
  syn region htmlBoldUnderline contained start="<u\>" end="</u\_s*>"me=s-1 contains=@htmlTop,htmlBoldUnderlineItalic
  syn region htmlBoldItalic contained start="<i\>" end="</i\_s*>"me=s-1 contains=@htmlTop,htmlBoldItalicUnderline
  syn region htmlBoldItalic contained start="<em\>" end="</em\_s*>"me=s-1 contains=@htmlTop,htmlBoldItalicUnderline
  syn region htmlBoldUnderlineItalic contained start="<i\>" end="</i\_s*>"me=s-1 contains=@htmlTop
  syn region htmlBoldUnderlineItalic contained start="<em\>" end="</em\_s*>"me=s-1 contains=@htmlTop
  syn region htmlBoldItalicUnderline contained start="<u\>" end="</u\_s*>"me=s-1 contains=@htmlTop,htmlBoldUnderlineItalic

  syn region htmlUnderline start="<u\>" end="</u\_s*>"me=s-1 contains=@htmlTop,htmlUnderlineBold,htmlUnderlineItalic
  syn region htmlUnderlineBold contained start="<b\>" end="</b\_s*>"me=s-1 contains=@htmlTop,htmlUnderlineBoldItalic
  syn region htmlUnderlineBold contained start="<strong\>" end="</strong\_s*>"me=s-1 contains=@htmlTop,htmlUnderlineBoldItalic
  syn region htmlUnderlineItalic contained start="<i\>" end="</i\_s*>"me=s-1 contains=@htmlTop,htmlUnderlineItalicBold
  syn region htmlUnderlineItalic contained start="<em\>" end="</em\_s*>"me=s-1 contains=@htmlTop,htmlUnderlineItalicBold
  syn region htmlUnderlineItalicBold contained start="<b\>" end="</b\_s*>"me=s-1 contains=@htmlTop
  syn region htmlUnderlineItalicBold contained start="<strong\>" end="</strong\_s*>"me=s-1 contains=@htmlTop
  syn region htmlUnderlineBoldItalic contained start="<i\>" end="</i\_s*>"me=s-1 contains=@htmlTop
  syn region htmlUnderlineBoldItalic contained start="<em\>" end="</em\_s*>"me=s-1 contains=@htmlTop

  syn region htmlItalic start="<i\>" end="</i\_s*>"me=s-1 contains=@htmlTop,htmlItalicBold,htmlItalicUnderline
  syn region htmlItalic start="<em\>" end="</em\_s*>"me=s-1 contains=@htmlTop
  syn region htmlItalicBold contained start="<b\>" end="</b\_s*>"me=s-1 contains=@htmlTop,htmlItalicBoldUnderline
  syn region htmlItalicBold contained start="<strong\>" end="</strong\_s*>"me=s-1 contains=@htmlTop,htmlItalicBoldUnderline
  syn region htmlItalicBoldUnderline contained start="<u\>" end="</u\_s*>"me=s-1 contains=@htmlTop
  syn region htmlItalicUnderline contained start="<u\>" end="</u\_s*>"me=s-1 contains=@htmlTop,htmlItalicUnderlineBold
  syn region htmlItalicUnderlineBold contained start="<b\>" end="</b\_s*>"me=s-1 contains=@htmlTop
  syn region htmlItalicUnderlineBold contained start="<strong\>" end="</strong\_s*>"me=s-1 contains=@htmlTop

  syn match htmlLeadingSpace "^\s\+" contained
  syn region htmlLink start="<a\>\_[^>]*\<href\>" end="</a\_s*>"me=s-1 contains=@Spell,htmlTag,htmlEndTag,htmlSpecialChar,htmlPreProc,htmlComment,htmlLeadingSpace,javaScript,htmlVbScript,@htmlPreproc
  syn region htmlH1 start="<h1\>" end="</h1\_s*>"me=s-1 contains=@htmlTop
  syn region htmlH2 start="<h2\>" end="</h2\_s*>"me=s-1 contains=@htmlTop
  syn region htmlH3 start="<h3\>" end="</h3\_s*>"me=s-1 contains=@htmlTop
  syn region htmlH4 start="<h4\>" end="</h4\_s*>"me=s-1 contains=@htmlTop
  syn region htmlH5 start="<h5\>" end="</h5\_s*>"me=s-1 contains=@htmlTop
  syn region htmlH6 start="<h6\>" end="</h6\_s*>"me=s-1 contains=@htmlTop
  syn region htmlHead start="<head\>" end="</head\_s*>"me=s-1 end="<body\>"me=s-1 end="<h[1-6]\>"me=s-1 contains=htmlTag,htmlEndTag,htmlSpecialChar,htmlPreProc,htmlComment,htmlLink,htmlTitle,javaScript,htmlVbScript,cssStyle,@htmlPreproc
  syn region htmlTitle start="<title\>" end="</title\_s*>"me=s-1 contains=htmlTag,htmlEndTag,htmlSpecialChar,htmlPreProc,htmlComment,javaScript,htmlVbScript,@htmlPreproc
endif

if main_syntax != 'java' || exists("java_javascript")
  " JAVA SCRIPT
  syn include @htmlJavaScript syntax/javascript.vim
  unlet b:current_syntax
  syn region  javaScript start=+<script\>\_[^>]*>+ keepend end=+</script\_[^>]*>+me=s-1 contains=@htmlJavaScript,htmlCssStyleComment,htmlScriptTag,@htmlPreproc
  syn region  htmlScriptTag	contained start=+<script+ end=+>+ fold contains=htmlTagN,htmlString,htmlArg,htmlValue,htmlTagError,htmlEvent
  hi def link htmlScriptTag htmlTag

  " html events (i.e. arguments that include javascript commands)
  if exists("html_extended_events")
    syn region htmlEvent	contained start=+\<on\a\+\s*=[\t ]*'+ end=+'+ contains=htmlEventSQ
    syn region htmlEvent	contained start=+\<on\a\+\s*=[\t ]*"+ end=+"+ contains=htmlEventDQ
  else
    syn region htmlEvent	contained start=+\<on\a\+\s*=[\t ]*'+ end=+'+ keepend contains=htmlEventSQ
    syn region htmlEvent	contained start=+\<on\a\+\s*=[\t ]*"+ end=+"+ keepend contains=htmlEventDQ
  endif
  syn region htmlEventSQ	contained start=+'+ms=s+1 end=+'+me=s-1 contains=@htmlJavaScript
  syn region htmlEventDQ	contained start=+"+ms=s+1 end=+"+me=s-1 contains=@htmlJavaScript
  hi def link htmlEventSQ htmlEvent
  hi def link htmlEventDQ htmlEvent

  " a javascript expression is used as an arg value
  syn region  javaScriptExpression contained start=+&{+ keepend end=+};+ contains=@htmlJavaScript,@htmlPreproc
endif

if main_syntax != 'java' || exists("java_vb")
  " VB SCRIPT
  syn include @htmlVbScript syntax/vb.vim
  unlet b:current_syntax
  syn region  htmlVbScript start=+<script \_[^>]*language *=\_[^>]*vbscript\_[^>]*>+ keepend end=+</script\_[^>]*>+me=s-1 contains=@htmlVbScript,htmlCssStyleComment,htmlScriptTag,@htmlPreproc
endif

syn cluster htmlJavaScript	add=@htmlPreproc

if main_syntax != 'java' || exists("java_css")
  " embedded style sheets
  syn include @htmlCss syntax/css.vim
  unlet b:current_syntax
  syn region cssStyle start=+<style+ keepend end=+</style>+ contains=@htmlCss,htmlTag,htmlEndTag,htmlCssStyleComment,@htmlPreproc
  syn match htmlCssStyleComment contained "\%(<!--\|-->\)"
  syn region htmlCssDefinition matchgroup=htmlArg start='style="' keepend matchgroup=htmlString end='"' contains=css.*Attr,css.*Prop,cssComment,cssLength,cssColor,cssURL,cssImportant,cssError,cssString,@htmlPreproc
endif

if main_syntax == "html"
  syn sync clear
  syn sync match htmlHighlightSkip "^.*['\"].*$"
  syn sync match htmlHighlight grouphere  htmlComment  "<!--"
  syn sync match htmlHighlight groupthere NONE         "--!\=\s*>"
  syn sync match htmlHighlight groupthere NONE         "</script\>"
  syn sync match htmlHighlight groupthere javaScript   "<script\>"
  syn sync match htmlHighlight groupthere htmlVbScript "<script \_[^>]*language *=\_[^>]*vbscript"
  syn sync match htmlHighlight groupthere NONE         "</style\>"
  syn sync match htmlHighlight groupthere cssStyle     "<style\>"
  exe "syn sync minlines=" .. get(g:, 'html_minlines', 250)
  exe "syn sync maxlines=" .. get(g:, 'html_maxlines', 500)
endif

" Folding
" (Originally written by Ingo Karkat and Marcus Zanona; see
" https://vi.stackexchange.com/questions/2306/html-syntax-folding-in-vim .)
if get(g:, "html_syntax_folding", 0)
  syn region htmlFold start="<\z(\<\%(area\|base\|br\|col\|command\|embed\|hr\|img\|input\|keygen\|link\|meta\|param\|source\|track\|wbr\>\)\@![a-z-]\+\>\)\%(\_s*\_[^/]\?>\|\_s\_[^>]*\_[^>/]>\)" end="</\z1\_s*>" fold transparent keepend extend containedin=htmlHead,htmlH\d
  " fold comments (the real ones and the old Netscape ones)
  if exists("html_wrong_comments")
    syn region htmlComment start=+<!--+ end=+--\s*>\%(\n\s*<!--\)\@!+ contains=@Spell fold
  endif
endif

" The default highlighting.
hi def link htmlTag			Function
hi def link htmlEndTag			Identifier
hi def link htmlArg			Type
hi def link htmlTagName			htmlStatement
hi def link htmlSpecialTagName		Exception
hi def link htmlMathTagName		htmlTagName
hi def link htmlSvgTagName		htmlTagName
hi def link htmlValue			String
hi def link htmlSpecialChar		Special

if !exists("html_no_rendering")
  hi def link htmlH1			  Title
  hi def link htmlH2			  htmlH1
  hi def link htmlH3			  htmlH2
  hi def link htmlH4			  htmlH3
  hi def link htmlH5			  htmlH4
  hi def link htmlH6			  htmlH5
  hi def link htmlHead			  PreProc
  hi def link htmlTitle			  Title
  hi def link htmlBoldItalicUnderline	  htmlBoldUnderlineItalic
  hi def link htmlUnderlineBold		  htmlBoldUnderline
  hi def link htmlUnderlineItalicBold	  htmlBoldUnderlineItalic
  hi def link htmlUnderlineBoldItalic	  htmlBoldUnderlineItalic
  hi def link htmlItalicUnderline	  htmlUnderlineItalic
  hi def link htmlItalicBold		  htmlBoldItalic
  hi def link htmlItalicBoldUnderline	  htmlBoldUnderlineItalic
  hi def link htmlItalicUnderlineBold	  htmlBoldUnderlineItalic
  hi def link htmlLink			  Underlined
  hi def link htmlLeadingSpace		  None
  if !exists("html_my_rendering")
    hi def htmlBold		   term=bold cterm=bold gui=bold
    hi def htmlBoldUnderline	   term=bold,underline cterm=bold,underline gui=bold,underline
    hi def htmlBoldItalic	   term=bold,italic cterm=bold,italic gui=bold,italic
    hi def htmlBoldUnderlineItalic term=bold,italic,underline cterm=bold,italic,underline gui=bold,italic,underline
    hi def htmlUnderline	   term=underline cterm=underline gui=underline
    hi def htmlUnderlineItalic	   term=italic,underline cterm=italic,underline gui=italic,underline
    hi def htmlItalic		   term=italic cterm=italic gui=italic
    if v:version > 800 || v:version == 800 && has("patch1038")
      hi def htmlStrike	term=strikethrough cterm=strikethrough gui=strikethrough
    else
      hi def htmlStrike	term=underline cterm=underline gui=underline
    endif
  endif
endif

hi def link htmlPreStmt		   PreProc
hi def link htmlPreError	   Error
hi def link htmlPreProc		   PreProc
hi def link htmlPreAttr		   String
hi def link htmlPreProcAttrName    PreProc
hi def link htmlPreProcAttrError   Error
hi def link htmlString		   String
hi def link htmlStatement	   Statement
hi def link htmlComment		   Comment
hi def link htmlDoctype		   htmlComment
hi def link htmlCommentNested	   htmlError
hi def link htmlCommentError	   htmlError
hi def link htmlTagError	   htmlError
hi def link htmlEvent		   javaScript
hi def link htmlError		   Error

hi def link javaScript		   Special
hi def link javaScriptExpression   javaScript
hi def link htmlCssStyleComment    Comment
hi def link htmlCssDefinition	   Special

let b:current_syntax = "html"

if main_syntax == 'html'
  unlet main_syntax
endif

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: nowrap sw=2 sts=2 ts=8 noet fdm=marker:
