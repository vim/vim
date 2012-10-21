" Vim syntax file
" Language:     Cascading Style Sheets
" Previous Contributor List:
"               Claudio Fleiner <claudio@fleiner.com> (Maintainer)
"               Yeti            (Add full CSS2, HTML4 support)
"               Nikolai Weibull (Add CSS2 support)
" Maintainer:   Jules Wang      <w.jq0722@gmail.com>
" URL:          https://github.com/JulesWang/css.vim
" Last Change:  2012 Dec 15

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if !exists("main_syntax")
  if version < 600
    syntax clear
  elseif exists("b:current_syntax")
  finish
endif
  let main_syntax = 'css'
endif

let s:cpo_save = &cpo
set cpo&vim

syn case ignore

" All HTML4 tags
syn keyword cssTagName abbr acronym address applet area a b base
syn keyword cssTagName basefont bdo big blockquote body br button
syn keyword cssTagName caption center cite code col colgroup dd del
syn keyword cssTagName dfn dir div dl dt em fieldset font form frame
syn keyword cssTagName frameset h1 h2 h3 h4 h5 h6 head hr html img i
syn keyword cssTagName iframe img input ins isindex kbd label legend li
syn keyword cssTagName link map menu meta noframes noscript ol optgroup
syn keyword cssTagName option p param pre q s samp script select small
syn keyword cssTagName span strike strong style sub sup table tbody td
syn keyword cssTagName textarea tfoot th thead title tr tt ul u var
syn keyword cssTagName object

" HTML5 new tags 5*6=30
syn keyword cssTagName article aside audio bdi canvas command
syn keyword cssTagName datalist details embed figcaption figure footer
syn keyword cssTagName header hgroup keygen mark meter nav
syn keyword cssTagName output progress rt rp ruby section
syn keyword cssTagName source summary time track video wbr

" Tags not supported in HTML5
syn keyword cssDeprecated acronym applet basefont big center dir
syn keyword cssDeprecated font frame frameset noframes strike tt

"syn match cssTagName "\<table\>"
syn match cssTagName "\*"

" selectors
syn match cssSelectorOp "[,>+]"
syn match cssSelectorOp2 "[~|^$*]\?=" contained
" FIXME: add HTML5 attribute
syn region cssAttributeSelector matchgroup=cssSelectorOp start="\[" end="]" transparent contains=cssUnicodeEscape,cssSelectorOp2,cssStringQ,cssStringQQ

" .class and #id
syn match cssClassName "\.[A-Za-z][A-Za-z0-9_-]\+"

try
syn match cssIdentifier "#[A-Za-zÀ-ÿ_@][A-Za-zÀ-ÿ0-9_@-]*"
catch /^.*/
syn match cssIdentifier "#[A-Za-z_@][A-Za-z0-9_@-]*"
endtry

syn match cssTagName "@page\>" nextgroup=cssDefinition
" FIXME: use cssVendor here
syn match cssTagName "@\(-\(webkit\|moz\|o\|ms\)-\)\=keyframes\>" nextgroup=cssDefinition

syn match cssMedia "@media\>" nextgroup=cssMediaType skipwhite skipnl
syn keyword cssMediaType contained screen print aural braile embosed handheld projection ty tv all nextgroup=cssMediaComma,cssMediaBlock skipwhite skipnl
"syn match cssMediaComma "," nextgroup=cssMediaType skipwhite skipnl
syn region cssMediaBlock transparent matchgroup=cssBraces start='{' end='}' contains=cssTagName,cssError,cssComment,cssDefinition,cssURL,cssUnicodeEscape,cssIdentifier

syn match cssValueInteger contained "[-+]\=\d\+"
syn match cssValueNumber contained "[-+]\=\d\+\(\.\d*\)\="
syn match cssValueLength contained "[-+]\=\d\+\(\.\d*\)\=\(%\|mm\|cm\|in\|pt\|pc\|em\|ex\|px\|rem\)"
syn match cssValueAngle contained "[-+]\=\d\+\(\.\d*\)\=\(deg\|grad\|rad\)"
syn match cssValueTime contained "+\=\d\+\(\.\d*\)\=\(ms\|s\)"
syn match cssValueFrequency contained "+\=\d\+\(\.\d*\)\=\(Hz\|kHz\)"

syn match cssFontDescriptor "@font-face\>" nextgroup=cssFontDescriptorBlock skipwhite skipnl
syn region cssFontDescriptorBlock contained transparent matchgroup=cssBraces start="{" end="}" contains=cssComment,cssError,cssUnicodeEscape,cssFontProp,cssFontAttr,cssCommonAttr,cssStringQ,cssStringQQ,cssFontDescriptorProp,cssValue.*,cssFontDescriptorFunction,cssUnicodeRange,cssFontDescriptorAttr
syn match cssFontDescriptorProp contained "\<\(unicode-range\|unit-per-em\|panose-1\|cap-height\|x-height\|definition-src\)\>"
syn keyword cssFontDescriptorProp contained src stemv stemh slope ascent descent widths bbox baseline centerline mathline topline
syn keyword cssFontDescriptorAttr contained all
syn region cssFontDescriptorFunction contained matchgroup=cssFunctionName start="\<\(uri\|url\|local\|format\)\s*(" end=")" contains=cssStringQ,cssStringQQ oneline keepend
syn match cssUnicodeRange contained "U+[0-9A-Fa-f?]\+"
syn match cssUnicodeRange contained "U+\x\+-\x\+"

" The 16 basic color names
syn keyword cssColor contained aqua black blue fuchsia gray green lime maroon navy olive purple red silver teal yellow

" 130 more color names
syn keyword cssColor contained aliceblue antiquewhite aquamarine azure
syn keyword cssColor contained beige bisque blanchedalmond blueviolet brown burlywood
syn keyword cssColor contained cadetblue chartreuse chocolate coral cornflowerblue cornsilk crimson cyan
syn match cssColor contained /dark\(blue\|cyan\|goldenrod\|gray\|green\|grey\|khaki\)/
syn match cssColor contained /dark\(magenta\|olivegreen\|orange\|orchid\|red\|salmon\|seagreen\)/
syn match cssColor contained /darkslate\(blue\|gray\|grey\)/
syn match cssColor contained /dark\(turquoise\|violet\)/
syn keyword cssColor contained deeppink deepskyblue dimgray dimgrey dodgerblue firebrick
syn keyword cssColor contained floralwhite forestgreen gainsboro ghostwhite gold
syn keyword cssColor contained goldenrod greenyellow grey honeydew hotpink
syn keyword cssColor contained indianred indigo ivory khaki lavender lavenderblush lawngreen
syn keyword cssColor contained lemonchiffon limegreen linen magenta
syn match cssColor contained /light\(blue\|coral\|cyan\|goldenrodyellow\|gray\|green\)/
syn match cssColor contained /light\(grey\|pink\|salmon\|seagreen\|skyblue\|yellow\)/
syn match cssColor contained /light\(slategray\|slategrey\|steelblue\)/
syn match cssColor contained /medium\(aquamarine\|blue\|orchid\|purple\|seagreen\)/
syn match cssColor contained /medium\(slateblue\|springgreen\|turquoise\|violetred\)/
syn keyword cssColor contained midnightblue mintcream mistyrose moccasin navajowhite
syn keyword cssColor contained oldlace olivedrab orange orangered orchid
syn match cssColor contained /pale\(goldenrod\|green\|turquoise\|violetred\)/
syn keyword cssColor contained papayawhip peachpuff peru pink plum powderblue
syn keyword cssColor contained rosybrown royalblue saddlebrown salmon sandybrown
syn keyword cssColor contained seagreen seashell sienna skyblue slateblue
syn keyword cssColor contained slategray slategrey snow springgreen steelblue tan
syn keyword cssColor contained thistle tomato turquoise violet wheat
syn keyword cssColor contained whitesmoke yellowgreen

" FIXME: These are actually case-insentivie too, but (a) specs recommend using
" mixed-case (b) it's hard to highlight the word `Background' correctly in
" all situations
syn case match
syn keyword cssColor contained ActiveBorder ActiveCaption AppWorkspace ButtonFace ButtonHighlight ButtonShadow ButtonText CaptionText GrayText Highlight HighlightText InactiveBorder InactiveCaption InactiveCaptionText InfoBackground InfoText Menu MenuText Scrollbar ThreeDDarkShadow ThreeDFace ThreeDHighlight ThreeDLightShadow ThreeDShadow Window WindowFrame WindowText Background
syn case ignore

syn match cssImportant contained "!\s*important\>"

syn match cssColor contained "\<transparent\>"
syn match cssColor contained "\<white\>"
syn match cssColor contained "#[0-9A-Fa-f]\{3\}\>"
syn match cssColor contained "#[0-9A-Fa-f]\{6\}\>"

syn region cssURL contained matchgroup=cssFunctionName start="\<url\s*(" end=")" oneline keepend
syn region cssFunction contained matchgroup=cssFunctionName start="\<\(rgb\|clip\|attr\|counter\|rect\|cubic-bezier\)\s*(" end=")" oneline keepend
syn region cssFunction contained matchgroup=cssFunctionName start="\<\(rgba\|hsl\|hsla\)\s*(" end=")" oneline keepend
syn region cssFunction contained matchgroup=cssFunctionName start="\<\(linear\|radial\)-gradient\s*(" end=")" oneline keepend
syn region cssFunction contained matchgroup=cssFunctionName start="\<\(matrix\(3d\)\=\|scale\(3d\|X\|Y|\Z\)\=\|translate\(3d\|X\|Y|\Z\)\=\|skew\(X\|Y\)\=\|rotate\(3d\|X\|Y|\Z\)\=\|perspective\)\s*(" end=")" oneline keepend

" Prop and Attr
" Reference: http://www.w3schools.com/cssref/default.asp
syn keyword cssCommonAttr contained auto none inherit all
syn keyword cssCommonAttr contained top bottom
syn keyword cssCommonAttr contained medium normal


syn match cssAnimationProp contained "\<animation\(-\(name\|duration\|timing-function\|delay\|iteration-cout\|play-state\)\)\=\>"


syn keyword cssAnimationAttr contained infinite alternate paused running
" bugfix: escape linear-gradient
syn match cssAnimationAttr contained "\<linear\(-gradient\)\@!\>"
syn match cssAnimationAttr contained "\<ease\(-\(in-out\|out\|in\)\)\=\>"

syn match cssBackgroundProp contained "\<background\(-\(color\|image\|attachment\|position\|clip\|origin\|size\)\)\=\>"
syn keyword cssBackgroundAttr contained center fixed over contain
syn match cssBackgroundAttr contained "\<no-repeat\>"
syn match cssBackgroundAttr contained "\<repeat\(-[xy]\)\=\>"
syn match cssBackgroundAttr contained "\<\(border\|content\|padding\)-box\>"


syn match cssBorderOutlineProp contained "\<border\(-\(top\|right\|bottom\|left\)\)\=\(-\(width\|color\|style\)\)\=\>"
syn match cssBorderOutlineProp contained "\<outline\(-\(width\|style\|color\)\)\=\>"
syn match cssBorderOutlineProp contained "\<border-\(top\|bottom\)-\(left\|right\)\(-radius\)\=\>"
syn match cssBorderOutlineProp contained "\<border-image\(-\(outset\|repeat\|slice\|source\|width\)\)\=\>"
syn match cssBorderOutlineProp contained "\<border-radius\>"
syn keyword cssBorderOutlineAttr contained thin thick medium
syn keyword cssBorderOutlineAttr contained dotted dashed solid double groove ridge inset outset
syn keyword cssBorderOutlineAttr contained hidden visible scroll collapse
syn keyword cssBorderOutlineAttr contained stretch round


syn match cssBoxProp contained "\<overflow\(-\(x\|y\|style\)\)\=\>"
syn match cssBoxProp contained "\<rotation\(-point\)=\>"
syn keyword cssBoxAttr contained visible hidden scroll auto
syn match cssBoxAttr contained "\<no-\(display\|content\)\>"

syn keyword cssColorProp contained opacity
syn match cssColorProp contained "\<color-profile\>"
syn match cssColorProp contained "\<rendering-intent\>"


syn match cssDimensionProp contained "\<\(min\|max\)-\(width\|height\)\>"
syn keyword cssDimensionProp contained height
syn keyword cssDimensionProp contained width

" shadow and sizing are in other property groups
syn match cssFlexibleBoxProp contained "\<box-\(align\|direction\|flex\|ordinal-group\|orient\|pack\|shadow\|sizing\)\>"
syn keyword cssFlexibleBoxAttr contained start end center baseline stretch
syn keyword cssFlexibleBoxAttr contained normal reverse
syn keyword cssFlexibleBoxAttr contained single mulitple
syn keyword cssFlexibleBoxAttr contained horizontal
" bugfix: escape vertial-align
syn match cssFlexibleBoxAttr contained "\<vertical\(-align\)\@!\>"
syn match cssFlexibleBoxAttr contained "\<\(inline\|block\)-axis\>"


syn match cssFontProp contained "\<font\(-\(family\|style\|variant\|weight\|size\(-adjust\)\=\|stretch\)\)\=\>"
syn match cssFontAttr contained "\<\(sans-\)\=\<serif\>"
syn match cssFontAttr contained "\<small\(-\(caps\|caption\)\)\=\>"
syn match cssFontAttr contained "\<x\{1,2\}-\(large\|small\)\>"
syn match cssFontAttr contained "\<message-box\>"
syn match cssFontAttr contained "\<status-bar\>"
syn match cssFontAttr contained "\<\(\(ultra\|extra\|semi\|status-bar\)-\)\=\(condensed\|expanded\)\>"
syn keyword cssFontAttr contained cursive fantasy monospace italic oblique
syn keyword cssFontAttr contained bold bolder light lighter larger smaller
syn keyword cssFontAttr contained icon menu caption
syn keyword cssFontAttr contained large smaller larger narrower wider
syn keyword cssFontAttr contained Courier Arial Georgia Times


syn keyword cssGeneratedContentProp contained content quotes crop
syn match cssGeneratedContentProp contained "\<counter-\(reset\|increment\)\>"
syn match cssGeneratedContentProp contained "\<move-to\>"
syn match cssGeneratedContentProp contained "\<page-policy\>"
syn match cssGeneratedContentAttr contained "\<\(no-\)\=\(open\|close\)-quote\>"


syn match cssGridProp contained "\<grid-\(columns\|rows\)\>"

syn match cssHyerlinkProp contained "\<target\(-\(name\|new\|position\)\)\=\>"

syn match cssListProp contained "\<list-style\(-\(type\|position\|image\)\)\=\>"
syn match cssListAttr contained "\<\(lower\|upper\)-\(roman\|alpha\|greek\|latin\)\>"
syn match cssListAttr contained "\<\(hiragana\|katakana\)\(-iroha\)\=\>"
syn match cssListAttr contained "\<\(decimal\(-leading-zero\)\=\|cjk-ideographic\)\>"
syn keyword cssListAttr contained disc circle square hebrew armenian georgian
syn keyword cssListAttr contained inside outside


syn match cssMarginProp contained "\<margin\(-\(top\|right\|bottom\|left\)\)\=\>"

syn match cssMultiColumnProp contained "\<column\(-\(\break-\(after\|before\)\|count\|gap\|rule\(-\(color\|style\|width\)\)\=\)\|span\|width\)\=\>"


syn match cssPaddingProp contained "\<padding\(-\(top\|right\|bottom\|left\)\)\=\>"

syn keyword cssPositioningProp contained bottom clear clip display float left
syn keyword cssPositioningProp contained position right top visibility
syn match cssPositioningProp contained "\<z-index\>"
syn keyword cssPositioningAttr contained block inline compact
syn match cssPositioningAttr contained "\<table\(-\(row-gorup\|\(header\|footer\)-group\|row\|column\(-group\)\=\|cell\|caption\)\)\=\>"
syn keyword cssPositioningAttr contained left right both
syn match cssPositioningAttr contained "\<list-item\>"
syn match cssPositioningAttr contained "\<inline-\(block\|table\)\>"
syn keyword cssPositioningAttr contained static relative absolute fixed

syn match cssPrintProp contained "\<page\(-break-\(before\|after\|inside\)\)\=\>"
syn keyword cssPrintProp contained orphans widows
syn keyword cssPrintAttr contained landscape portrait crop cross always avoid

syn match cssTableProp contained "\<\(caption-side\|table-layout\|border-collapse\|border-spacing\|empty-cells\)\>"
syn keyword cssTableAttr contained fixed collapse separate show hide once always


syn keyword cssTextProp contained color direction
syn match cssTextProp "\<\(\(word\|letter\)-spacing\|text\(-\(decoration\|transform\|align\|index\|shadow\)\)\=\|vertical-align\|unicode-bidi\|line-height\)\>"
syn match cssTextProp contained "\<text-\(justify\|\outline\|overflow\|warp\|align-last\)\>"
syn match cssTextProp contained "\<word-\(break\|\wrap\)\>"
syn match cssTextProp contained "\<white-space\>"
syn match cssTextProp contained "\<hanging-punctuation\>"
syn match cssTextProp contained "\<punctuation-trim\>"
syn match cssTextAttr contained "\<line-through\>"
syn match cssTextAttr contained "\<text-indent\>"
syn match cssTextAttr contained "\<\(text-\)\=\(top\|bottom\)\>"
syn keyword cssTextAttr contained ltr rtl embed nowrap
syn keyword cssTextAttr contained underline overline blink sub super middle
syn keyword cssTextAttr contained capitalize uppercase lowercase
syn keyword cssTextAttr contained center justify baseline sub super
syn match cssTextAttr contained "\<pre\(-\(line\|wrap\)\)\=\>"
syn match cssTextAttr contained "\<\(allow\|force\)-end\>"
syn keyword cssTextAttr contained start end adjacent
syn match cssTextAttr contained "\<inter-\(word\|ideographic\|cluster\)\>"
syn keyword cssTextAttr contained distribute kashida first last
syn keyword cssTextAttr contained clip ellipsis unrestricted suppress
syn match cssTextAttr contained "\<break-all\>"
syn match cssTextAttr contained "\<break-word\>"
syn keyword cssTextAttr contained hyphenate


syn match cssTransformProp contained "\<transform\(-\(origin\|style\)\)\=\>"
syn match cssTransformProp contained "\<perspective\(-origin\)\=\>"
syn match cssTransformProp contained "\<backface-visibility\>"

syn match cssTransitionProp contained "\<transition\(-\(delay\|duration\|property\|timing-function\)\)\=\>"

syn match cssUIProp contained "\<nav-\(down\|index\|left\|right\|up\)\=\>"
syn match cssUIProp contained "\<outline-offset\>"
syn match cssUIProp contained "\<box-sizing\>"
syn keyword cssUIProp contained appearance icon resize
syn keyword cssUIAttr contained window button menu field

syn match cssAuralProp contained "\<\(pause\|cue\)\(-\(before\|after\)\)\=\>"
syn match cssAuralProp contained "\<\(play-during\|speech-rate\|voice-family\|pitch\(-range\)\=\|speak\(-\(punctuation\|numerals\)\)\=\)\>"
syn keyword cssAuralProp contained volume during azimuth elevation stress richness
syn match cssAuralAttr contained "\<\(x-\)\=\(soft\|loud\)\>"
syn keyword cssAuralAttr contained silent
syn match cssAuralAttr contained "\<spell-out\>"
syn keyword cssAuralAttr contained non mix
syn match cssAuralAttr contained "\<\(left\|right\)-side\>"
syn match cssAuralAttr contained "\<\(far\|center\)-\(left\|center\|right\)\>"
syn keyword cssAuralAttr contained leftwards rightwards behind
syn keyword cssAuralAttr contained below level above higher
syn match cssAuralAttr contained "\<\(x-\)\=\(slow\|fast\)\>"
syn keyword cssAuralAttr contained faster slower
syn keyword cssAuralAttr contained male female child code digits continuous
syn match cssAuralAttr contained "\<lower\>"

" cursor
syn keyword cssUIProp contained cursor
syn match cssUIAttr contained "\<[ns]\=[ew]\=-resize\>"
syn keyword cssUIAttr contained crosshair default help move pointer
syn keyword cssUIAttr contained progress wait

" FIXME: I could not find them in reference
syn keyword cssUIAttr contained invert maker size zoom
syn match cssRenderAttr contained "\<run-in\>"
syn match cssRenderAttr contained "\<text-rendering\>"
syn match cssRenderAttr contained "\<font-smoothing\>"
syn match cssRenderProp contained "\<marker-offset\>"
syn match cssRenderAttr contained "\<bidi-override\>"


" FIXME: This allows cssMediaBlock before the semicolon, which is wrong.
syn region cssInclude start="@import" end=";" contains=cssComment,cssURL,cssUnicodeEscape,cssMediaType
syn match cssBraces contained "[{}]"
syn match cssError contained "{@<>"
syn region cssDefinition transparent matchgroup=cssBraces start='{' end='}' contains=css.*Attr,css.*Prop,cssComment,cssValue.*,cssColor,cssURL,cssImportant,cssError,cssStringQ,cssStringQQ,cssFunction,cssUnicodeEscape,cssVendor,cssDefinition
syn match cssBraceError "}"

" Pseudo class
syn match cssPseudoClass ":[A-Za-z0-9_-]*" contains=cssPseudoClassId,cssUnicodeEscape
syn keyword cssPseudoClassId link visited active hover focus before after left right lang
syn match cssPseudoClassId contained "\<first\(-\(line\|letter\|child\)\)\=\>"
" FIXME: handle functions.
"syn region cssPseudoClassLang matchgroup=cssPseudoClassId start="lang(" end=")"
syn match cssPseudoClassId contained "\<\(last\|only\|nth\|nth-last\)-child\>"
syn match cssPseudoClassId contained "\<\(first\|last\|only\|nth\|nth-last\)-of-type\>"
syn keyword cssPseudoClassId root empty target enable disabled checked not invalid
syn match cssPseudoClassId contained  "::\(-moz-\)\=selection"

" Comment
syn region cssComment start="/\*" end="\*/" contains=@Spell
syn region cssComment start="//" skip="\\$" end="$" keepend contains=@Spell

syn match cssUnicodeEscape "\\\x\{1,6}\s\?"
syn match cssSpecialCharQQ +\\"+ contained
syn match cssSpecialCharQ +\\'+ contained
syn region cssStringQQ start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=cssUnicodeEscape,cssSpecialCharQQ
syn region cssStringQ start=+'+ skip=+\\\\\|\\'+ end=+'+ contains=cssUnicodeEscape,cssSpecialCharQ

" Vendor Prefix
syn match cssVendor contained "\(-\(webkit\|moz\|o\|ms\)-\)"


if main_syntax == "css"
  syn sync minlines=10
endif

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_css_syn_inits")
  if version < 508
    let did_css_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink cssComment Comment
  HiLink cssVendor Comment
  HiLink cssTagName Statement
  HiLink cssDeprecated Error
  HiLink cssSelectorOp Special
  HiLink cssSelectorOp2 Special

  HiLink cssAnimationProp StorageClass
  HiLink cssBackgroundProp StorageClass
  HiLink cssBorderOutlineProp StorageClass
  HiLink cssBoxProp StorageClass
  HiLink cssColorProp StorageClass
  HiLink cssContentForPagedMediaProp StorageClass
  HiLink cssDimensionProp StorageClass
  HiLink cssFlexibleBoxProp StorageClass
  HiLink cssFontProp StorageClass
  HiLink cssGeneratedContentProp StorageClass
  HiLink cssGridProp StorageClass
  HiLink cssHyerlinkProp StorageClass
  HiLink cssLineboxProp StorageClass
  HiLink cssListProp StorageClass
  HiLink cssMarginProp StorageClass
  HiLink cssMarqueeProp StorageClass
  HiLink cssMultiColumnProp StorageClass
  HiLink cssPaddingProp StorageClass
  HiLink cssPagedMediaProp StorageClass
  HiLink cssPositioningProp StorageClass
  HiLink cssPrintProp StorageClass
  HiLink cssRubyProp StorageClass
  HiLink cssSpeechProp StorageClass
  HiLink cssTableProp StorageClass
  HiLink cssTextProp StorageClass
  HiLink cssTransformProp StorageClass
  HiLink cssTransitionProp StorageClass
  HiLink cssUIProp StorageClass
  HiLink cssAuralProp StorageClass
  HiLink cssRenderProp StorageClass

  HiLink cssAnimationAttr Type
  HiLink cssBackgroundAttr Type
  HiLink cssBorderOutlineAttr Type
  HiLink cssBoxAttr Type
  HiLink cssColorAttr Type
  HiLink cssContentForPagedMediaAttr Type
  HiLink cssDimensionAttr Type
  HiLink cssFlexibleBoxAttr Type
  HiLink cssFontAttr Type
  HiLink cssGeneratedContentAttr Type
  HiLink cssGridAttr Type
  HiLink cssHyerlinkAttr Type
  HiLink cssLineboxAttr Type
  HiLink cssListAttr Type
  HiLink cssMarginAttr Type
  HiLink cssMarqueeAttr Type
  HiLink cssMultiColumnAttr Type
  HiLink cssPaddingAttr Type
  HiLink cssPagedMediaAttr Type
  HiLink cssPositioningAttr Type
  HiLink cssPrintAttr Type
  HiLink cssRubyAttr Type
  HiLink cssSpeechAttr Type
  HiLink cssTableAttr Type
  HiLink cssTextAttr Type
  HiLink cssTransformAttr Type
  HiLink cssTransitionAttr Type
  HiLink cssUIAttr Type
  HiLink cssAuralAttr Type
  HiLink cssRenderAttr Type
  HiLink cssCommonAttr Type

  HiLink cssPseudoClassId PreProc
  HiLink cssPseudoClassLang Constant
  HiLink cssValueLength Number
  HiLink cssValueInteger Number
  HiLink cssValueNumber Number
  HiLink cssValueAngle Number
  HiLink cssValueTime Number
  HiLink cssValueFrequency Number
  HiLink cssFunction Constant
  HiLink cssURL String
  HiLink cssFunctionName Function
  HiLink cssColor Constant
  HiLink cssIdentifier Function
  HiLink cssInclude Include
  HiLink cssImportant Special
  HiLink cssBraces Function
  HiLink cssBraceError Error
  HiLink cssError Error
  HiLink cssInclude Include
  HiLink cssUnicodeEscape Special
  HiLink cssStringQQ String
  HiLink cssStringQ String
  HiLink cssMedia Special
  HiLink cssMediaType Special
  HiLink cssMediaComma Normal
  HiLink cssFontDescriptor Special
  HiLink cssFontDescriptorFunction Constant
  HiLink cssFontDescriptorProp StorageClass
  HiLink cssFontDescriptorAttr Type
  HiLink cssUnicodeRange Constant
  HiLink cssClassName Function
  delcommand HiLink
endif

let b:current_syntax = "css"

if main_syntax == 'css'
  unlet main_syntax
endif

let &cpo = s:cpo_save
unlet s:cpo_save
" vim: ts=8

