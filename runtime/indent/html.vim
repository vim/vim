" Vim indent script for HTML
" Header: "{{{
" Maintainer:	Michael Lee <michael.lee@zerustech.com>
" Original Author: Andy Wokula <anwoku@yahoo.de>
" Last Change:	2017 July 05
" Version:	2.0
" Description:	Fix bugs
"               Refactor code base
"               Reuse indent/css.vim
"               Support custom block tags and indent methods
"               Add detailed comments
"
" Credits:
"       indent/html.vim (2016 Mar 30) from Bram Moolenaar
"	indent/css.vim  (2012 May 30) from Nikolai Weibull
"
" History:
" 2017 Jul 05   (v2.0) overhaul (Michael)
" 2016 Mar 30   (v1.0) overhaul (Bram)
" 2014 June	(v1.0) overhaul (Bram)
" 2012 Oct 21	(v0.9) added support for shiftwidth()
" 2011 Sep 09	(v0.8) added HTML5 tags (thx to J. Zuckerman)
" 2008 Apr 28	(v0.6) revised customization
" 2008 Mar 09	(v0.5) fixed 'indk' issue (thx to C.J. Robinson)
"}}}

" Init Folklore, check user settings (2nd time ++)
if exists("b:did_indent") "{{{
  finish
endif
let b:did_indent = 1

"""""" Code below this is loaded only once. """""
"{{{
if exists("*HtmlIndent") && !exists('g:force_reload_html')
  call HtmlIndent_CheckUserSettings()
  finish
endif

" Disables text-wrap for normal text.
setlocal formatoptions-=t

" Enables text-wrap for comments.
setlocal formatoptions+=croql

" Due to issue https://github.com/vim/vim/issues/1696, the middle part of three-piece comments must NOT be blank.
setlocal comments=s1:<!--[,m:\ \ \ \ \,ex:]-->,s4:<!--,m://,ex:-->

" Allow for line continuation below.
let s:cpo_save = &cpo
set cpo-=C
"}}}

" Initialize script variables
"{{{
func! s:GetSID()
  return matchstr(expand('<sfile>'), '<SNR>\d\+_')
endfunc

" SID of current script
let s:SID = s:GetSID()
delfunc s:GetSID

" The number of spaces for 1 indent.
let s:indent_unit = shiftwidth()

" Regex pattern for matching an end tag (normal, custom, or block tag), at the
" start of line.
" Example:
" --------
" ^\s*\%\(<\zs!\[endif\]--\ze>\|<\zs/script\>\|<\zs/style\>\|<\zs/pre\>\|\zs--\ze>\|<\zs/\w\+\(-\w\+\)\+\>\|<\zs/\w\+\>\)
let s:starts_with_end_tag_pattern = ''

" Regex pattern for matching a block start tag.
" Example:
" --------
" <\zsscript\>\|<\zsstyle\>\|<\zs!--\[\ze\|<\zspre\>\|<\zs!--\ze
let s:block_start_tag_pattern = ''

" Regex pattern for matching a block end tag.
" Example:
" --------
" <\zs!\[endif\]--\ze>\|<\zs/script\>\|<\zs/style\>\|<\zs/pre\>\|\zs--\ze>
let s:block_end_tag_pattern = ''

" Regex pattern for matching a normal tag (start or end).
let s:normal_tag_pattern = '<\zs/\=\w\+\>'

" Regex pattern for matching a normal end tag.
let s:normal_end_tag_pattern = '<\zs/\w\+\>'

" Regex pattern for matching a full normal end tag.
let s:normal_end_tag_full_pattern = '</\w\+\s*>'

" Regex pattern for matching a custom tag (start or end).
let s:custom_tag_pattern = '<\zs/\=\w\+\(-\w\+\)\+\>'

" Regex pattern for matching a custom end tag.
let s:custom_end_tag_pattern = '<\zs/\w\+\(-\w\+\)\+\>'

" Regex pattern for matching a full custom end tag.
let s:custom_end_tag_full_pattern = '</\w\+\(-\w\+\)\+\s*>'

" A list that contains tag ids identified by tag names:
" {
"     'div' => 1,
"     '/div' = > -1,
"     ...
"     'pre' => 2,
"     '/pre' => -2,
"     'script' => 3,
"     '/script' => -3,
"     'style' => 4,
"     '/style' => -4,
"     '!--' => 5,
"     '--' => -5,
"     '!--[' => 6,
"     '![endif]--' => -6,
"     ...
" }
let s:indent_tags = {}

" Dictionary stores meta data of block tags:
"
" {
"     id => {
"         'start' => start tag,
"         'end' => end tag,
"         'type' => block type,
"         'brackets' => a:b,c:d,
"             a - left bracket character of the start tag
"             b - right bracket character of the start tag
"             c - left bracket character of the end tag
"             d - right bracket character of the end tag
"         'bracket_patterns' => a:b,c:d
"             a - bracket pattern for matching the left part of the start tag
"             b - bracket pattern for matching the right part of the start tag
"             c - bracket pattern for matching the left part of the end tag
"             d - bracket pattern for matching the right part of the end tag
"         'indent_alien' => name of the alien method
"     },
"     ...
" }
let s:block_tags = {}

" The dictionary stores block start tags indexed by the lengths of tag
" names.
let s:block_start_tags = {}

" The dictionary stores block end tags indexed by the lengths of tag
" names.
let s:block_end_tags = {}
"}}}

" Initialize buffer variables
" {{{
" The context for indenting current line.
" {
"     'lnum' => Last indented line: prevnonblank(a:lnum - 1),
"
"     'indent' => Indent of current line. Using it or not is up to the indent
"                 methods. Currently, it is used by s:IndentOfNormalLine()
"                 method,
"
"     'changed_tick' => The number of changes till current line is indented,
"
"     'block' => The id of the block, inside which, current line is located,
"
"     'block_start_tag_lnum' => line number of block start tag (if block!=0),
"
"     'script_type' => type attribute of a script tag (if block==3),
"
"     'indent_inside_block' => The fallback indent for all lines inside a
"                              block.  Using it or not is up to the indent
"                              methods. Currently, it is used by s:Alien3(),
"                              s:Alien4(), and s:Alien6() methods, and is
"                              ignored by other indent methods,
"
"     'root_block' => The id of the block that contains everything before the
"                     cursor in current line,
"
"     'state' => A string that indicates the state of current context. The
"                possible values of this property include 'normal_line',
"                'line_inside_attribute', 'line_inside_tag', and
"                'line_inside_block',
"
"     'ready' => A flag indicates if b:context has been calculated
"                successfully,
" }
let b:context = {'lnum': 0, 'indent': -1, 'changed_tick': 0,
                \'block': 0, 'block_start_tag_lnum': 0, 'script_type': '',
                \'indent_inside_block': -1, 'root_block': 0, 'state': 'normal_line',
                \'ready': 0}

" A flag indicates if indent has been calculated successfully.
let b:indent_ready = 0

" The context object for counting tags and indents in a string.
" {
"     'root_block' => The initial (root) block when starting the process of
"                     counting tags in a string,
"
"     'block' => Current container block, inside which, offsets are being
"                calculated. So it can be the root block (0 or 6), or any 2nd
"                level block inside the root block, but it CAN NOT be any
"                block in the 3rd level (assume nested conditional comments
"                are not supported),
"
"     'block_stack' => This list stores block start tags when counting tags in
"                      a string. Initially, the first element of this stack is
"                      a string constant 'guard' as the guard, and the 2nd
"                      element is the root block. When a block end tag is
"                      matched, the last block start tag is popped up from the
"                      end of this list. And if the original root block is
"                      reset, the guard is removed as well,
"
"     'block_index' => The index of current container block in b:block_stack,
"
"     'current_line_offset' => It indicates the number of indent units, caused
"                              by the end tags in a line, that the line is
"                              supposed to move backward. This value is no
"                              greater than 0,
"
"     'next_line_offset' => It indicates the number of indent units, caused by
"                           the start tags in a line, that the next line is
"                           supposed to move forward. This value is no less
"                           than 0,
"
"     'current_line_offset_stack' => This list stores changes to current line
"                                    offset within each blocks,
"
"     'next_line_offset_stack' => This list stores changes to next line offset
"                                 within each blocks,
" }
"
let b:tag_counter = {'root_block': 0, 'block': 0, 'block_stack': [],
                    \'block_index': 0, 'current_line_offset': 0, 'next_line_offset': 0,
                    \'current_line_offset_stack': [], 'next_line_offset_stack': []}

" This list stores history of cursor positions so that it will be possible to
" restore to a previous position.
let b:cursor_stack = []
" }}}

" Initializes local configuration when current script is loaded for the first
" time, or restores local configuration after executing external scripts.
func! s:RestoreLocalConfiguration()
  "{{{
  setlocal indentexpr=HtmlIndent()
  setlocal indentkeys=o,O,<Return>,<>>,{,},!^F
  " Needed for % to work when finding start/end of a tag.
  setlocal matchpairs+=<:>
  let b:current_syntax = 'html'
  let b:undo_indent = "setlocal inde< indk<"
endfunc "}}}

" Checks and processes settings from b:html_indent and g:html_indent...
" variables.  Prefer using buffer-local settings over global settings, so that
" there can be defaults for all HTML files and exceptions for specific types
" of HTML files.
"
" This method checks the following variables and changes the indent behavior
" accordingly:
"
" - b|g:html_indent_inctags—a string introducing additional, comma separated,
"   tags
"
" - b|g:html_indent_autotags—a string containing tags (comma separated) to be
"   marked as removed
"
" - b|g:html_indent_string_names—a list of syntax names indicating being
"   inside an attribute value.
"
" - b|g:html_indent_tag_names—a list of syntax names indicating being inside a
"   tag
"
" - b|g:html_indent_script1—indent method (zero, auto, or inc) for the first
"   line inside a <script> tag.
"
" - b|g:html_indent_style1—indent method (zero, auto, or inc) for the first
"   line inside a <style> tag.
"
" - b|g:html_indent_line_limit—the maximum number of lines to look backward
"   for synchronization.
"
" - b|g:html_indent_custom_block_tags—a list that contains arguments for
"   adding custom block tags. Each record of this list is a list of arguments
"   for adding one custom block tags.
"
func! HtmlIndent_CheckUserSettings()
  "{{{
  let inctags = ''
  if exists("b:html_indent_inctags")
    let inctags = b:html_indent_inctags
  elseif exists("g:html_indent_inctags")
    let inctags = g:html_indent_inctags
  endif
  let b:hi_tags = {}
  if len(inctags) > 0 | call s:AddITags(b:hi_tags, split(inctags, ",")) | endif

  let autotags = ''
  if exists("b:html_indent_autotags")
    let autotags = b:html_indent_autotags
  elseif exists("g:html_indent_autotags")
    let autotags = g:html_indent_autotags
  endif
  let b:hi_removed_tags = {}
  if len(autotags) > 0 | call s:RemoveITags(b:hi_removed_tags, split(autotags, ",")) | endif

  " Syntax names indicating being inside a string of an attribute value.
  let string_names = []
  if exists("b:html_indent_string_names")
    let string_names = b:html_indent_string_names
  elseif exists("g:html_indent_string_names")
    let string_names = g:html_indent_string_names
  endif
  let b:hi_insideStringNames = ['htmlString']
  if len(string_names) > 0
    for s in string_names | call add(b:hi_insideStringNames, s) | endfor
  endif

  " Syntax names indicating being inside a tag.
  let tag_names = []
  if exists("b:html_indent_tag_names")
    let tag_names = b:html_indent_tag_names
  elseif exists("g:html_indent_tag_names")
    let tag_names = g:html_indent_tag_names
  endif
  let b:hi_insideTagNames = ['htmlTag', 'htmlScriptTag']
  if len(tag_names) > 0
    for s in tag_names | call add(b:hi_insideTagNames, s) | endfor
  endif

  let indone = {"zero": 0
              \,"auto": "indent(prevnonblank(v:lnum-1))"
              \,"inc": "b:context.indent_inside_block + shiftwidth()"}

  let script1 = 'inc'
  if exists("b:html_indent_script1")
    let script1 = b:html_indent_script1
  elseif exists("g:html_indent_script1")
    let script1 = g:html_indent_script1
  endif
  let b:hi_js1indent = len(script1) > 0 ? get(indone, script1, indone.zero) : 0

  let style1 = 'inc'
  if exists("b:html_indent_style1")
    let style1 = b:html_indent_style1
  elseif exists("g:html_indent_style1")
    let style1 = g:html_indent_style1
  endif
  let b:hi_css1indent = len(style1) > 0 ? get(indone, style1, indone.zero) : 0

  if !exists('b:html_indent_line_limit')
    let b:html_indent_line_limit = exists('g:html_indent_line_limit') ? g:html_indent_line_limit : 200
  endif

  " Adding custom block tags
  let custom_block_tags = []
  if exists("b:html_indent_custom_block_tags")
    let custom_block_tags = b:html_indent_custom_block_tags
  elseif exists("g:html_indent_custom_block_tags")
    let custom_block_tags = g:html_indent_custom_block_tags
  endif
  for arguments in custom_block_tags | call call('s:AddBlockTag', arguments) | endfor

endfunc "}}}

" Updates value of the given variable. This is useful for unit testing.
" @param name The variable name.
" @param value The variable value.
func! HtmlIndentSet(name, value)
  let {a:name} = a:value
endfunc

" Returns value of the given variable. This is useful for unit testing.
" @param name The variable name.
" @return The variable value.
func! HtmlIndentGet(name)
  return {a:name}
endfunc

" Invokes the given function in current script, and returns the result. This
" is useful for unit testing.
" @param name The function name.
" @param arguments The list of arguments.
" @return The result.
func! HtmlIndentCall(name, arguments)
  let name = substitute(a:name, '^s:', s:SID, '')
  return call(name, a:arguments)
endfunc

" Saves current cursor position.
func! s:PushCursor()
  "{{{
  call add(b:cursor_stack, [line('.'), col('.')])
endfunc "}}}

" Restores previously saved cursor position.
func! s:PopCursor()
  " {{{
  if len(b:cursor_stack) > 0
    let pos = remove(b:cursor_stack, -1)
    call cursor(pos)
  endif
endfunc "}}}

" Looks backward to find the block that contains the whole line before current
" cursor position and returns the id of the block:
" <!--[...]>
" ^ <- The block that contains the whole line
"     <pre>
"        ...
"     <div></div></pre><div></div>...
"                                 ^ <- cursor
"     ^--------------------------^ <- the whole line before the cursor.
" <![endif]-->
" @return The block id, or 0 if no container block is found.
func! s:CurrentBlockId()
  " {{{
  let current_line_number = line('.')
  call s:PushCursor()
  let block = 0

  " Look backward for a block tag: start or end tag.
  let [current_block_tag, current_block_tag_lnum, current_block_tag_start_col, current_block_tag_end_col] = s:SearchStringPosition(s:BlockTagPattern(), "bW", 0)

  if current_block_tag_lnum > 0
    if s:IsStartTag(current_block_tag) && current_block_tag_lnum < current_line_number
      " It is a block start tag, and the block start tag is before current
      " line, so a match has been found.
      let block = get(s:indent_tags, current_block_tag)
    else
      " It is a block end tag, or a block start tag in current line, try to
      " locate an other block start tag before current line.
      let index = s:IndexOfBlockTagStartBracket(getline(current_block_tag_lnum), current_block_tag, current_block_tag_start_col - 1)
      call cursor(current_block_tag_lnum, index + 1)
      let start_lnum = s:IsStartTag(current_block_tag) ? current_block_tag_lnum : s:FindBlockStartTag(current_block_tag)
      " If current tag is a block start tag in current line, try to find
      " another block start tag from its start bracket, otherwise, try to find
      " another block start tag from its start tag.
      if start_lnum > 0 | let block = s:CurrentBlockId() | endif
    endif
  endif

  " Restore the cursor location.
  call s:PopCursor()

  return block

endfunc "}}}

" Finds the unclosed block start tag from the current cursor position.
" The cursor must be on or before a block end tag. After the start tag has
" been matched, the cursor stops at the start bracket of the block start tag:
" <div><block-start>
"      ^ <- cursor stops here
"     ...
"     </block-end>
" @return Line number of the start tag, or 0 on failure.
func! s:FindBlockStartTag(block_end_tag)
  "{{{
  let end_pattern = s:EndTagPattern(a:block_end_tag, 0)
  let start_pattern = s:StartTagPattern(s:StartTag(a:block_end_tag), 0)
  let start_lnum = searchpair(start_pattern, '', end_pattern, 'bW')

  return start_lnum > 0 ? start_lnum : 0

endfunc "}}}

" Finds and returns the index of the start bracket of the given block tag in a
" string from the specified position.
" NOTE: Index starts from 0.
" Example:
" -------
" ...<div>...
"    ^ <- index of the start bracket
" @param block_tag_line The string that contains the block tag.
" @param tag_name The tag name.
" @param tag_start_index The index of the start of the block tag.
" @return The index of the block start bracket, or start of the block tag if
" it does not have a start bracket.
func! s:IndexOfBlockTagStartBracket(block_tag_line, tag_name, tag_start_index)
  "{{{
  let id = s:indent_tags[a:tag_name]
  let meta = s:block_tags[abs(id)]
  let bracket_type = id > 0 ? 0 : 1
  let brackets = split(split(meta.brackets, ',', 1)[bracket_type], ':', 1)
  let bracket = brackets[0]

  return bracket == '' ? a:tag_start_index : a:tag_start_index - len(bracket)

endfunc "}}}

" Finds and returns the index of the right most bracket character of the given
" block tag in a string from the specified position.
" NOTE: index starts from 0.
" Example:
" -------
" ...<div>...
"        ^ <- index of the end bracket
" @param block_tag_line The string that contains the block tag.
" @param tag_name The tag name.
" @param tag_start_index The index of the start of the block tag.
" @return The index of the block end bracket, or end of the block tag if it
" does not have an end bracket.
func! s:IndexOfBlockTagEndBracket(block_tag_line, tag_name, tag_start_index)
  "{{{
  let id = s:indent_tags[a:tag_name]
  let meta = s:block_tags[abs(id)]
  let bracket_type = id > 0 ? 0 : 1
  let brackets = split(split(meta.brackets, ',', 1)[bracket_type], ':', 1)
  let bracket = brackets[1]

  return bracket == '' ? a:tag_start_index + len(a:tag_name) - 1 : match(a:block_tag_line, escape(bracket, '[]'), a:tag_start_index) + len(bracket) - 1

endfunc "}}}

" Searches for regex pattern from current cursor position, and returns the
" matched string, the line number, the start column, and the end column.
" Depends on the flags, the cursor may or may not move to the matched
" location.
" @param pattern The regex pattern.
" @param flag The search flag (:help search() for details).
" @param stop_line The search stops after the stop line has been scanned.
" @return [result, line number, start column, end column]
func! s:SearchStringPosition(pattern, flag, stop_line)
  "{{{
  let result = ''
  let lnum = 0
  let start_col = 0
  let end_col = 0

  " Try to match <div\> first to locate the position of the left most bracket.
  " This is necessary because technically, a tag may have a multi-character
  " bracket.
  let pattern_without_zs = substitute(a:pattern, '\\zs', '', 'g')
  call s:PushCursor()
  let [lnum, start_col_1] = searchpos(pattern_without_zs, a:flag, a:stop_line)

  " Move cursor to its original position, and try to match <\zsdiv\>, if there
  " is a \zs, to locate the position of the matched tag name.
  call s:PopCursor()
  let [lnum, start_col_2] = searchpos(a:pattern, a:flag, a:stop_line)

  if lnum > 0
    let text = tolower(getline(lnum))
    let start_col = start_col_2
    let result = matchstr(text[start_col_1 - 1:], a:pattern)
    " <div>
    "    ^ <- end col
    "  ^ <- start col
    let end_col = start_col + len(result) - 1
  endif

  return [result, lnum, start_col, end_col]

endfunc "}}}

" Adds a list of tag names for a pair of <tag> </tag> to 'tags'.
" @param tags The dictionary into which tags are to be added
" @param tag_list The list contains tags to be added
func! s:AddITags(tags, tag_list)
  "{{{
  for itag in a:tag_list
    let a:tags[itag] = 1
    let a:tags['/' . itag] = -1
  endfor

endfunc "}}}

" Takes a list of tag name pairs that are not to be used as tag pairs.
" @param tags The dictionary from which tags are to be removed
" @param tag_list The list contains tags to be removed
func! s:RemoveITags(tags, tag_list)
  "{{{
  for itag in a:tag_list
    let a:tags[itag] = 1
    let a:tags['/' . itag] = 1
  endfor

endfunc "}}}

" Adds a block tag, that is a tag with a different kind of indenting.
" @param tag The start tag
" @param id The id of start tag
" @param a:1 The end tag
" @param a:2 The block type: block, comment, and etc.
" @param a:3 The bracket characters for start and end tags.
" @param a:4 The bracket patterns for matching boundaries of start and end
" @param a:5 The method for calculating indent of lines inside this block.
" tags.
func! s:AddBlockTag(tag, id, ...)
  "{{{
  let start_tag = a:tag
  let end_tag = a:0 > 0 ? a:1 : '/' . a:tag

  let s:indent_tags[start_tag] = a:id
  let s:indent_tags[end_tag] = -a:id

  let s:block_tags[a:id] = {}
  let s:block_tags[a:id]['start'] = start_tag
  let s:block_tags[a:id]['end'] = end_tag
  let s:block_tags[a:id]['type'] = a:0 > 1 ? a:2 : 'block'
  let s:block_tags[a:id]['brackets'] = a:0 > 2 ? a:3 : '<:>,<:>'
  let s:block_tags[a:id]['bracket_patterns'] = a:0 > 3 ? a:4 : '<\zs:\>,<\zs:\>'
  let s:block_tags[a:id]['indent_alien'] = a:0 > 4 ? a:5 : 's:Alien' . a:id

  let key = len(start_tag)
  if !has_key(s:block_start_tags, key) | let s:block_start_tags[key] = [] | endif
  if match(s:block_start_tags[key], start_tag) == -1 | call add(s:block_start_tags[key], start_tag) | endif

  let key = len(end_tag)
  if !has_key(s:block_end_tags, key) | let s:block_end_tags[key] = [] | endif
  if match(s:block_end_tags[key], end_tag) == -1 | call add(s:block_end_tags[key], end_tag) | endif

endfunc "}}}

" Add known tag pairs.
" Self-closing tags and tags that are sometimes {{{
" self-closing (e.g., <p>) are not here (when encountering </p> we can find
" the matching <p>, but not the other way around).
" Old HTML tags:
call s:AddITags(s:indent_tags, [
    \ 'a', 'abbr', 'acronym', 'address', 'b', 'bdo', 'big',
    \ 'blockquote', 'body', 'button', 'caption', 'center', 'cite', 'code',
    \ 'colgroup', 'del', 'dfn', 'dir', 'div', 'dl', 'em', 'fieldset', 'font',
    \ 'form', 'frameset', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'head', 'html',
    \ 'i', 'iframe', 'ins', 'kbd', 'label', 'legend', 'li',
    \ 'map', 'menu', 'noframes', 'noscript', 'object', 'ol',
    \ 'optgroup', 'q', 's', 'samp', 'select', 'small', 'span', 'strong', 'sub',
    \ 'sup', 'table', 'textarea', 'title', 'tt', 'u', 'ul', 'var', 'th', 'td',
    \ 'tr', 'tbody', 'tfoot', 'thead'])

" New HTML5 elements:
call s:AddITags(s:indent_tags, [
    \ 'area', 'article', 'aside', 'audio', 'bdi', 'canvas',
    \ 'command', 'data', 'datalist', 'details', 'embed', 'figcaption',
    \ 'figure', 'footer', 'header', 'keygen', 'mark', 'meter', 'nav', 'output',
    \ 'progress', 'rp', 'rt', 'ruby', 'section', 'source', 'summary', 'svg',
    \ 'time', 'track', 'video', 'wbr'])

" Tags added for web components:
call s:AddITags(s:indent_tags, [
    \ 'content', 'shadow', 'template'])
"}}}

" Add Block Tags: these contain alien content
"{{{
call s:AddBlockTag('pre', 2)
call s:AddBlockTag('script', 3)
call s:AddBlockTag('style', 4)
call s:AddBlockTag('!--', 5, '--', 'comment', '<:,:>', '<\zs:\ze,\zs:\ze>')
call s:AddBlockTag('!--[', 6, '![endif]--', 'comment', '<:>,<:>', '<\zs:\ze,<\zs:\ze>')
"}}}

" Checks if the given tag is a start tag.
" @return 1 if the tag is a start tag, 0 otherwise.
func! s:IsStartTag(tag_name)
  "{{{
  let id = get(s:indent_tags, a:tag_name)

  return id > 0 || id == 0 && a:tag_name[0] != '/'

endfunc "}}}

" Checks if the given tag is a start tag.
" @return 1 if the tag is a start tag, 0 otherwise.
func! s:IsEndTag(tag_name)
  "{{{
  let id = get(s:indent_tags, a:tag_name)

  return id < 0 || id == 0 && a:tag_name[0] == '/'

endfunc "}}}

" Generates the regex pattern for matching the given tag as a start tag.
" Examples:
" ---------
" <\zsdiv\>, <\zsstyle\>, <\zs!--\ze, <\zs!--\[
"
" @param start_tag The start tag.
" @param a:1 A flag that indicates if \zs should be included in the pattern, 1
" by default.
" @return The regex pattern.
func! s:StartTagPattern(start_tag, ...)
  "{{{
  let left = '<\zs'
  let right = '\>'
  let id = get(s:indent_tags, a:start_tag)
  let include_zs = a:0 == 0 ? 1 : a:1

  if id > 1
    let meta = s:block_tags[id]
    let bracket_patterns = split(split(meta.bracket_patterns, ',', 1)[0], ':', 1)
    let left = bracket_patterns[0]
    let right = bracket_patterns[1]
  endif

  let pattern = left . escape(a:start_tag,'[]') . right
  if !include_zs | let pattern = substitute(pattern, '\\zs', '', '') | endif

  return pattern

endfunc "}}}

" Generates the regex pattern for matching the given tag as an end tag.
" Examples:
" ---------
" <\zs/div\>, <\zs/style\>, <\zs:--\ze>, <\zs!\[endif\]--\ze>
"
" @param end_tag The end tag.
" @param a:1 A flag that indicates if \zs should be included in the pattern, 1
" by default.
" @return The regex pattern.
func! s:EndTagPattern(end_tag, ...)
  "{{{
  let left = '<\zs'
  let right = '\>'
  let id = abs(get(s:indent_tags, a:end_tag))
  let include_zs = a:0 == 0 ? 1 : a:1

  if id > 1
    let meta = s:block_tags[id]
    let bracket_patterns = split(split(meta.bracket_patterns, ',', 1)[1], ':', 1)
    let left = bracket_patterns[0]
    let right = bracket_patterns[1]
  endif

  let pattern = left . escape(a:end_tag, '[]') . right
  if !include_zs | let pattern = substitute(pattern, '\\zs', '', '') | endif

  return pattern

endfunc "}}}

" Finds the corresponding start tag of the given end tag.
" @param end_tag The end tag.
" @return The start tag.
func! s:StartTag(end_tag)
  "{{{
  let start_tag = a:end_tag[1:]
  let id = -get(s:indent_tags, a:end_tag)
  if id >= 2 | let start_tag = s:block_tags[id].start | endif

  return start_tag

endfunc "}}}

" Generates regex pattern for matching an end tag at the start of line.
" @return The regex pattern.
func! s:StartsWithEndTagPattern()
  "{{{

  if s:starts_with_end_tag_pattern == ''
    for key in reverse(sort(keys(s:block_end_tags), 'N'))
      for tag_name in s:block_end_tags[key]
        if s:starts_with_end_tag_pattern != '' | let s:starts_with_end_tag_pattern .= '\|' | endif
        let s:starts_with_end_tag_pattern .= s:EndTagPattern(tag_name)
      endfor
    endfor

    let s:starts_with_end_tag_pattern .= '\|' . s:custom_end_tag_pattern . '\|' . s:normal_end_tag_pattern
  endif

  return '^\s*\%\(' . s:starts_with_end_tag_pattern .'\)'

endfunc "}}}

" Generates regex pattern for matching a block start tag.
" @return The regex pattern.
func! s:BlockStartTagPattern()
  "{{{

  if s:block_start_tag_pattern != '' | return s:block_start_tag_pattern | endif

  for key in reverse(sort(keys(s:block_start_tags), 'N'))
    for tag_name in s:block_start_tags[key]
      if s:block_start_tag_pattern != '' | let s:block_start_tag_pattern .= '\|' | endif
      let s:block_start_tag_pattern .= s:StartTagPattern(tag_name)
    endfor
  endfor

  return s:block_start_tag_pattern

endfunc "}}}

" Generates regex pattern for matching a block end tag.
" @return The regex pattern.
func! s:BlockEndTagPattern()
  "{{{
  if s:block_end_tag_pattern != '' | return s:block_end_tag_pattern | endif

  for key in reverse(sort(keys(s:block_end_tags), 'N'))
    for tag_name in s:block_end_tags[key]
      if s:block_end_tag_pattern != '' | let s:block_end_tag_pattern .= '\|' | endif
      let s:block_end_tag_pattern .= s:EndTagPattern(tag_name)
    endfor
  endfor

  return s:block_end_tag_pattern

endfunc "}}}

" Generates regex pattern for matching a block end or start tag.
" @return The regex pattern.
func! s:BlockTagPattern()
  "{{{
  return s:BlockEndTagPattern() . '\|' . s:BlockStartTagPattern()
endfunc "}}}

" Gets the id for a given tag name, taking care of buffer-local tags.
" @param tag_name The tag name
" @return Id of the tag, or 0 if no id is found.
func! s:GetTag(tag_name)
  "{{{
  let i = get(s:indent_tags, a:tag_name)

  if (i == 1 || i == -1) && get(b:hi_removed_tags, a:tag_name) != 0 | return 0 | endif

  if i == 0 | let i = get(b:hi_tags, a:tag_name) | endif

  return i

endfunc "}}}

" Counts the numbers of start and end tags in a string, and updates the values
" of b:tag_counter.current_line_offset and b:tag_counter.next_line_offset
" accordingly.
" @param text The line to be checked.
" @param root_block The id of the block, inside which, the string is being
" checked, 0 by default (not inside any block).
func! s:CountITags(text, root_block)
  "{{{
  let b:tag_counter.root_block = a:root_block
  let b:tag_counter.block = a:root_block
  let b:tag_counter.block_stack = ['guard', a:root_block]
  let b:tag_counter.block_index = 1
  let b:tag_counter.current_line_offset = 0
  let b:tag_counter.next_line_offset = 0
  let b:tag_counter.current_line_offset_stack = [0]
  let b:tag_counter.next_line_offset_stack = [0]

  call substitute(a:text, s:BlockTagPattern() . '\|' . s:custom_tag_pattern . '\|' . s:normal_tag_pattern, '\=s:CheckTag(submatch(0))', 'g')

  " Adjust b:tag_counter.current_line_offset
  call s:UpdateCurrentLineOffset(a:text)

endfunc "}}}

" Checks a single tag and upates the values of
" b:tag_counter.current_line_offset and b:tag_counter.next_line_offset
" accordingly. Used by s:CountITags().
" @param tag The tag to be checked.
" @return 1 if the tag has been checked, or 0 otherwise.
func! s:CheckTag(tag)
  "{{{
  if (s:CheckCustomTag(a:tag)) | return 1 | endif

  let id = s:GetTag(a:tag)

  if id == -1
    " end tag
    " Ignore tag within a block, but tags within a conditional comment
    " should be counted as if the conditional comment is not a block.
    if b:tag_counter.block != 0 && b:tag_counter.block != 6 | return 1 | endif
    if b:tag_counter.next_line_offset == 0
      let b:tag_counter.current_line_offset_stack[-1] -= 1
      let b:tag_counter.current_line_offset -= 1
    else
      let b:tag_counter.next_line_offset_stack[-1] -= 1
      let b:tag_counter.next_line_offset -= 1
    endif
    return 1
  endif

  if id == 1
    " start tag
    " Ignore tag within a block
    if b:tag_counter.block != 0 && b:tag_counter.block != 6 | return 1 | endif
    let b:tag_counter.next_line_offset_stack[-1] += 1
    let b:tag_counter.next_line_offset += 1
    return 1
  endif

  if id != 0
    " block-tag (start or end)
    return s:CheckBlockTag(id)
  endif

  return 0

endfunc "}}}

" Checks a block tag and updates the values of
" b:tag_counter.current_line_offset and b:tag_counter.next_line_offset
" accordingly. Used by s:CheckTag().
" @param id The block id.
" @return 1 if tag has been checked, 0 otherwise.
func! s:CheckBlockTag(id)
  "{{{
  if a:id > 0
    " A block starts here
    call add(b:tag_counter.block_stack, a:id)
    call add(b:tag_counter.current_line_offset_stack, 0)
    call add(b:tag_counter.next_line_offset_stack, 0)

    " If current block supports sub-block, move block index to current block
    " tag and make it the new current block.
    if b:tag_counter.block == 0 || b:tag_counter.block == 6
      let b:tag_counter.block = a:id
      let b:tag_counter.block_index += 1
    endif

    return 1

  endif

  if a:id < 0
    " A block ends here.

    " Pop a block tag from the block stack.
    call remove(b:tag_counter.block_stack, -1)

    " Make sure the block stack is never empty. Extra block end tags may be
    " included in the string incorrectly, which may cause an empty block
    " stack.
    if [] == b:tag_counter.block_stack || ['guard'] == b:tag_counter.block_stack | let b:tag_counter.block_stack = [0] | endif

    " If current block has been reset, move block index to the previous block
    " tag, and make it the current block.
    if [0] == b:tag_counter.block_stack || len(b:tag_counter.block_stack) == b:tag_counter.block_index
      let b:tag_counter.block_index = len(b:tag_counter.block_stack) - 1
      let b:tag_counter.block = b:tag_counter.block_stack[b:tag_counter.block_index]
    endif

    " Revert changes to current line offset in current block.
    let b:tag_counter.current_line_offset -= remove(b:tag_counter.current_line_offset_stack, -1)
    if [] == b:tag_counter.current_line_offset_stack | let b:tag_counter.current_line_offset_stack = [0] | endif

    " Revert changes to next line offset in current block.
    let b:tag_counter.next_line_offset -= remove(b:tag_counter.next_line_offset_stack, -1)
    if [] == b:tag_counter.next_line_offset_stack | let b:tag_counter.next_line_offset_stack = [0] | endif

    return 1

  endif

  return 0

endfunc "}}}

" Checks a custom tag and updates the values of b:tag_counter.current_line_offset and
" b:tag_counter.next_line_offset accordingly. Used by s:CheckTag().
" @param custom_tag The custom tag to be checked.
" @return 1 if the given tag is a valid custom tag name, 0 otherwise.
func! s:CheckCustomTag(custom_tag)
  "{{{
  let pattern = '\%\(\w\+-\)\+\w\+'

  if match(a:custom_tag, pattern) == -1 | return 0 | endif

  if matchstr(a:custom_tag, '\/\ze.\+') == "/"
    " end tag
    " ignore custom_tag within a block
    if b:tag_counter.block != 0 && b:tag_counter.block != 6 | return 1 | endif
    if b:tag_counter.next_line_offset == 0
      let b:tag_counter.current_line_offset_stack[-1] -= 1
      let b:tag_counter.current_line_offset -= 1
    else
      let b:tag_counter.next_line_offset_stack[-1] -= 1
      let b:tag_counter.next_line_offset -= 1
    endif
  else
    " start tag
    if b:tag_counter.block != 0 && b:tag_counter.block != 6 | return 1 | endif
    let b:tag_counter.next_line_offset_stack[-1] += 1
    let b:tag_counter.next_line_offset += 1
  endif

  return 1

endfunc "}}}

" Resolves script type from the given attribute value.
" @param str The attribute value.
" @return Empty if no script type is resolved, or 'javascript'.
func! s:GetScriptType(str)
  "{{{
  return a:str == "" || a:str =~ "java" ? 'javascript' : ''
endfunc "}}}

" Adjusts the value of b:tag_counter.current_line_offset if the given string starts with a
" normal end tag and its indent value is used for calculating the indent of
" the next line, because the given string is aligned to a position that is 1
" indent unit before the position that would have been if the string did not
" start with an end tag.
"
" Assume current line has been indented, the basic flow for calculating the
" indent of next line is as following:
"
" 1. Count tags of current line and upate values of b:tag_counter.current_line_offset and
" b:tag_counter.next_line_offset
"
" 2. If necessary, adjust b:tag_counter.current_line_offset
"
" 3. next_line.indent = indent(current line) + (b:tag_counter.current_line_offset +
" b:tag_counter.next_line_offset) * unit
"
" NOTE: keep it in mind that if current line starts with end tag, its indent
" would have already reflected the -1 unit caused by the end tag.
"
" Example A:
" ----------
" Current line is '</div><div>', and next_line.indent = indent(current) +
" (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) * unit, so
" b:tag_counter.current_line_offset needs to be adjusted by +1.
"
" Example B:
" ----------
" Current line is </div><div><script>, and next_line.indent_inside_block =
" indent(current) + (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) * unit, so
" b:tag_counter.current_line_offset needs to be adjusted by +1.
"
" Example C:
" ----------
" Current line is </div><div><script></script><div>, which will be simplified
" as </div><div><div>, and next_line.indent = indent(current) +
" (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) * unit, so
" b:tag_counter.current_line_offset needs to be adjusted.
"
" Example D:
" ----------
" Current line is </div><div><![endif]--><div>, and next_line.indent =
" indent(line of <!--[...]>) + (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) *
" unit, so indent(current) is not used, therefore there is no need to adjust
" b:tag_counter.current_line_offset.
" NOTE: In this case, the root block (<!--[...]>) has been reset by the end
" tag.
"
" Example E:
" ----------
" <!--[...]> <- root block
" <div><div>
"     </div><div><div> <- current line
"         </div>       <- next line aligns here:
"                         indent(current) + offset-of(</div><div>),
"                         so b:tag_counter.current_line_offset needs to be adjusted.
"     </div>           <- the 2nd line aligns here
"
" @param text The string to be checked.
func! s:UpdateCurrentLineOffset(text)
  "{{{
  let end_tag = matchstr(a:text, s:StartsWithEndTagPattern())

  " The string starts with a normal end tag; and
  " the root block is either 0 (no root block) or 6 (conditional comment); and
  " the root block has not been reset by any block end tags.
  if end_tag != '' && (b:tag_counter.root_block == 0 || b:tag_counter.root_block == 6) && 'guard' == '' . b:tag_counter.block_stack[0]
    " It is not really necessary to update current_line_offset_stack here,
    " because it will not be used any more. However, it is no harmful to
    " update it to keep consistent logic and semantics.
    let b:tag_counter.current_line_offset_stack[0] += 1
    let b:tag_counter.current_line_offset += 1
  endif

endfunc "}}}

" Checks if current context is still valid and can be reused for calculating
" indents of current line.
" @return 1 if current context is valid, 0 otherwise.
func! s:ValidateContext(state)
  return 1 == b:context.ready && b:context.state == a:state && prevnonblank(v:lnum - 1) == b:context.lnum && b:context.changed_tick == b:changedtick - 1
endfunc

" Initializes context if current line is inside an HTML attribute, and its line
" number is greater than 2.
"
" Example A:
" ----------
" <div class="container"
"      style="color: red;
"      ^ <- aligns to previous line
"      border: 1px solid #ff0000;" <- current line
"      >
"     </div>
"
" Example B:
" ----------
" <div>
"     <div class="container" style="color: red;
"     ^ <- aligns to previous line
"     border: 1px solid #ff0000; <- current line
"     ">
"     </div>
" </div>
"
" @param context Current context.
" @return The initialized context.
func! s:InitContextOfLineInsideAttribute(context)
  "{{{
  " Check if current context can be reused.
  if s:ValidateContext('line_inside_attribute') | return b:context | endif

  let context = a:context
  let text = tolower(getline(v:lnum))

  if context.lnum <= 1 || text =~ '^\s*<' | return context | endif

  normal! ^

  " Assume there are no tabs
  let stack = synstack(v:lnum, col('.'))
  if len(stack) > 0 | let stack = reverse(stack) | endif

  for synid in stack
    let name = synIDattr(synid, "name")
    if index(b:hi_insideStringNames, name) >= 0
      let context.ready = 1
      let context.state = 'line_inside_attribute'
      let context.indent = indent(context.lnum)
      break
    endif
  endfor

  return context

endfunc "}}}

" Initializes context if current line is inside an HTML tag. In other words, it
" is an HTML attribute.
"
" Example A:
" ----------
" <table class="container" width="100%"
"                          ^ <- aligns to the last attribute.
"                          style="color: red;" > <- current line
" </table>
"
" Example B:
" ----------
" <table class="container" style="..." <- len(this line) > 300
"        ^ <- aligns to the first attribute
"        width="100%"> <- current line
" </table>

" @param context Current context.
" @return The initialized context.
func! s:InitContextOfLineInsideTag(context)
  "{{{
  " Check if current context can be reused.
  if s:ValidateContext('line_inside_tag') | return b:context | endif

  let context = a:context
  let text = tolower(getline(v:lnum))

  if text =~ '^\s*<' | return context | endif

  normal! ^

  " Assume there are no tabs
  let stack = synstack(v:lnum, col('.'))

  if len(stack) > 0 | let stack = reverse(stack) | endif

  for synid in stack
    let name = synIDattr(synid, "name")
    if index(b:hi_insideTagNames, name) >= 0
      let context.ready = 1
      let context.state = 'line_inside_tag'
      " When calcuating indent of an attribute, the b:context.context is not
      " used, so there is no need to update context.indent here.
      " But it is no harmful to set this value to keep consistent logic and
      " semantics for it.
      let context.indent = b:context.indent
      break
    endif
  endfor

  return context

endfunc "}}}

" Initializes context if current line is inside a block.
"
" @param context Current context.
" @param current_block_start_tag_line The line of the start tag of the block
" that contains current line.
" @param current_block_start_tag The start tag of the block that contains
" current line.
" @param current_block_start_tag_lnum The line number of the block start tag.
" @param current_block_start_tag_start_col The start column number of the
" block start tag.
" @return The initialized context
func! s:InitContextOfLineInsideBlock(context, current_block_start_tag_line, current_block_start_tag, current_block_start_tag_lnum, current_block_start_tag_start_col)
  "{{{
  " Check if current context can be reused.
  if s:ValidateContext('line_inside_block') | return b:context | endif

  let context = a:context

  if s:IsStartTag(a:current_block_start_tag) && a:current_block_start_tag_lnum > 0 && a:current_block_start_tag_lnum < v:lnum
    " Current line is inside a block (between the block start and the block
    " end tag).

    let context.state = 'line_inside_block'

    " Update the block type in context
    let context.block = s:indent_tags[a:current_block_start_tag]

    if context.block == 3 | let context.script_type = s:GetScriptType(matchstr(a:current_block_start_tag_line, '\>[^>]*', a:current_block_start_tag_start_col)) | endif

    " Update the block start tag line number in context
    let context.block_start_tag_lnum = a:current_block_start_tag_lnum

    " Check the preceding tags of the block start tag and update b:tag_counter.current_line_offset and
    " b:tag_counter.next_line_offset.
    " String index (starts from 0):
    " ----------------------------
    " ...<block>
    "    ^ <- left_index
    "   ^  <- left_index - 1
    let left_index = s:IndexOfBlockTagStartBracket(a:current_block_start_tag_line, a:current_block_start_tag, a:current_block_start_tag_start_col - 1)
    let left_text = left_index == 0 ? '' : a:current_block_start_tag_line[: left_index - 1]

    let context.indent_inside_block = indent(a:current_block_start_tag_lnum)

    " It wouldn't be necessary to check tags, if left_index <=2, in which
    " case, the length of left text is less than the length of <a>, which is
    " the shortest tag.
    if left_index > 2

      call cursor(a:current_block_start_tag_lnum, left_index + 1)

      let context.root_block = s:CurrentBlockId()

      call s:CountITags(left_text, context.root_block)

      let context.indent_inside_block += (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) * s:indent_unit

    endif

    if context.block == 6 && context.lnum > context.block_start_tag_lnum
      " Current line is inside a conditional comment; and
      " is not the first line:
      "
      " <!--[...]>
      " ... <- The first line inside a conditional comment, it is aligned by
      " Alien6() as a line inside block.
      " ... <- This is current line, it should be aligned as a normal line, so
      " context.ready SHOULD NOT be set to 1
      " <![endif]-->
    else
      let context.ready = 1

      " Set the fallback indent of current line. Actually, it is not necessary,
      " because lines in block are to be indented by Alien{*} methods. However,
      " it is no harmful to set context.indent here to keep consistent logic
      " and semantics for it.
      let context.indent = context.indent_inside_block
    endif
  endif

  return context

endfunc "}}}

" Moves cursor to the start tag of the end tag on current cursor position, and
" initializes context of the prefix before the start tag.
" This method is used by s:InitContextOfLineStartsWithEndTag() and
" s:InitContextOfLineAfterEndTag().
" NOTE: When calling this method, cursor must has been positioned on the start
" bracket of the end tag:
" <p><div>
"     ...
"     <p></p></div>
"            ^ <- cursor
" @param context Current context
" @return The initialized context.
func! s:InitContextOfPrefixBeforeStartTag(context)

  let context = a:context

  let context.state = 'normal_line'

  " The pattern does not contain \zs, so cursor is positioned at the left
  " bracket:
  " .<div>
  "  ^   <- cursor positioned here, col('.')
  " ^    <- col('.') - 1
  let start_lnum = s:FindStartTag()

  if start_lnum <= 0 | return context | endif

  " Check for the line starting with something inside a tag:
  " <sometag               <- align here
  "    attr=val><open>     not here
  let text = tolower(getline(start_lnum))
  let bracket = matchstr(text, '[<>]')
  if bracket == '>'
    call cursor(start_lnum, 1)
    normal! f>%
    let start_lnum = line('.')
    let text = tolower(getline(start_lnum))
  endif

  " Now, start_lnum is the line number of an effective start tag:
  " current start tag or previous start tag, and text is the line that
  " contains the effective tag.

  " First, align current line to the start of the line of start tag.  This is
  " just the temporary indent, which has to be adjusted by
  " b:tag_counter.current_line_offset and b:tag_counter.next_line_offset if
  " the start tag is preceded by any tags.
  let context.indent = indent(start_lnum)

  " It wouldn't be necessary to check tags, if col('.') <= 3, in which case,
  " the length of text is less than the length of <a>, which is the shortest
  " tag.
  if col('.') > 3
    let context.root_block = s:CurrentBlockId()
    call s:CountITags(tolower(getline(start_lnum)[:col('.') - 2]), context.root_block)
    let context.indent += (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) * s:indent_unit
  endif

  let context.ready = 1

  return context

endfunc

" Initializes context if current line starts with an end tag.
" NOTE: Context can not be reused for a line starts with an end tag.
" @param context Current context.
" @return The initialized context.
func! s:InitContextOfLineStartsWithEndTag(context)
  "{{{
  " Does the line start with an end tag?
  let swendtag = match(tolower(getline(v:lnum)), s:StartsWithEndTagPattern()) >= 0

  if !swendtag | return a:context | endif

  " The current line starts with an end tag.
  " ...
  " </a>... <- current line
  " ...
  "
  " Basic flow for indenting current line is as follows:
  "
  " 1. Assume current line is </a>...
  "
  " 2. Find the previous line that contains the start tag of the end tag:
  " [preceding tags]<a>...
  "
  " 3. Align current line to the start of that line: indent =
  " indent(start_lnum)
  "
  " 4. Calculate b:tag_counter.current_line_offset and
  " b:tag_counter.next_line_offset of the preceding tags.
  "
  " 5. Further indent current line: indent +=
  " (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) *
  " shift-unit
  "
  " Refer to function checkTag() for the meanings of
  " b:tag_counter.current_line_offset and b:tag_counter.next_line_offset.
  "
  " NOTE: When a leading end tag is present, we always assume that all
  " tags between it and its start tag have been paired correctly, thus all
  " the intermediary tags can be ignored without affecting the correct
  " calculation of the indent:
  "
  " <div><a>...
  "     </a>... <- current line (indent of 1 shift-unit)
  " </div>
  "
  " can be simplified as
  "
  " <div><a>
  "     </a>... <- current line (indent of 1 shift-unit)
  " </div>
  "
  " When calculating the value of b:tag_counter.next_line_offset, the start
  " tag is excluded (b:tag_counter.next_line_offset is calculated for
  " preceding tags only), thus the indent of current line is 1 shift-unit less
  " than what would have been if the end tag were replaced or preceded by any
  " start tag(s):
  "
  " <div><a>    <- <a> is not included when calculating b:tag_counter.next_line_offset
  "     </a>... <- current line (indent of 1 shift-unit)
  " </div>
  "
  " <div><a>    <- <a> is included when calculating b:tag_counter.next_line_offset
  "         <span></span></a> <- current line (indent of 2 shift-units)
  " </div>
  "
  " In other words, for any line that starts with an end tag, the 1
  " shift-unit indent contributed by its start tag would have already been
  " deducted from its indent.
  "
  " As a result, when calculating b:tag_counter.current_line_offset for a line
  " that starts with an end tag, the -1 shift-unit contributed by the end tag
  " should be offset (by +1), because it has already been reflected in its
  " indent:
  "
  " <div>
  "     <a>
  "     ^ <- Align here (indent of 1 shift-unit).
  "          The indent contributed by <a> is deducted
  "     </a><b> <- The b:tag_counter.current_line_offset of </a> is 0: -1 + 1
  "     </b>    <- Current line (indent of 1 shift-unit: 1 + 0)
  " </div>
  "
  " <div>
  "     <a>
  "         ^ <- Align here (indent of 2 shift-units).
  "              The indent contributed by <a> is NOT deducted
  "         <span></span></a><b> <- The b:tag_counter.current_line_offset of
  "                                 '<span></span></a>' is -1
  "     </b> <- Current line (indent of 1 shift-unit: 2 - 1).
  " </div>
  "
  " This principle is very important for understanding the logic of indent
  " calculation and it is used widely in this script.
  call cursor(v:lnum, 1)

  return s:InitContextOfPrefixBeforeStartTag(a:context)

endfunc "}}}

" Initializes context if the previous line ends with an end tag. Adjusts
" context.indent based on the line of start tag.
" NOTE: Context can not be reused for a line after an end tag.
" @param context Current context.
" @return The initialized context.
func! s:InitContextOfLineAfterEndTag(context)
  "{{{
  let text = tolower(getline(a:context.lnum))
  let pattern = s:custom_end_tag_full_pattern . '\|' . s:normal_end_tag_full_pattern
  let pattern = '\(' . pattern . '\)' . '\s*$'

  if text !~ pattern | return a:context | endif

  call cursor(a:context.lnum, 1) | normal! $

  normal! F<

  return s:InitContextOfPrefixBeforeStartTag(a:context)

endfunc "}}}

" Initializes context if the preceding line of current line contains a block
" end tag. In such case, the block as well as the tags between the block start
" and end tags are to be ignored, and current line should be aligned to what
" remains:
"
" Example:
" --------
" <div><script>
" ...
" </script><div></div>
" ... <- current line
"
" The <script> block and its contents are to be ignored, and current line
" should be indented as following:
"
" <div><div></div>
"     ^   <- Aligns here
"     ... <- current line
"
" NOTE: Context can not be reused for a line after a block.
" @param context Current context.
" @param current_block_end_tag_line The line of the block end tag.
" @param current_block_end_tag The line of the block end tag.
" @param current_block_end_tag_lnum The line number of the block end tag.
" @param current_block_end_tag_start_col The start column number of the block end tag.
" parent block that contains current block, or 0 if there is no parent block.
" @return The initialized context.
func! s:InitContextOfLineAfterBlock(context, current_block_end_tag_line, current_block_end_tag, current_block_end_tag_lnum, current_block_end_tag_start_col)
  "{{{
  let context = a:context

  if s:IsEndTag(a:current_block_end_tag) && a:current_block_end_tag_lnum > 0 && a:current_block_end_tag_lnum == context.lnum
    " Current line is preceded by a line that contains a block end tag.

    let context.state = 'normal_line'

    " Find the start tag of current block.
    let start_tag_pattern = s:StartTagPattern(s:StartTag(a:current_block_end_tag))
    let [ current_block_start_tag,
          \ current_block_start_tag_lnum,
          \ current_block_start_tag_start_col,
          \ current_block_start_tag_end_col] = s:SearchStringPosition(start_tag_pattern, "bnW", 0)

    let current_block_start_tag_line = getline(current_block_start_tag_lnum)

    " String index (starts from 0):
    " -----------------------------
    " ...<block>
    "    ^  <- left_index
    "   ^   <- left_index - 1
    let left_index = s:IndexOfBlockTagStartBracket(current_block_start_tag_line, current_block_start_tag, current_block_start_tag_start_col - 1)
    let left = left_index == 0 ? '' : current_block_start_tag_line[: left_index - 1]

    " String index (stats from 0):
    " ----------------------------
    " </block>.
    "         ^ <- right_index + 1
    "        ^  <- right_index
    let right_index = s:IndexOfBlockTagEndBracket(a:current_block_end_tag_line, a:current_block_end_tag, a:current_block_end_tag_start_col - 1)
    let right = a:current_block_end_tag_line[right_index + 1:]

    call cursor(a:current_block_end_tag_lnum, 1) | normal! $
    let context.root_block = s:CurrentBlockId()

    call s:CountITags(tolower(left . right), context.root_block)

    let context.indent = indent(current_block_start_tag_lnum) + (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) * s:indent_unit
    let context.ready = 1
  endif

  return context

endfunc "}}}


" Initializes context if current line is a normal line: not inside a block,
" not after a block end line.
" NOTE: 
" * The line of a block start tag or an end tag is also a normal line.
" * A normal line may actually inside a root block:
" <!--[...]>
"     <pre>
"     </pre>
"     <div></div>
"     <div></div> <- current line
" <![endif]-->
" @param context Current context.
" @return The initialized context.
func! s:InitContextOfNormalLine(context)
  "{{{
  " Check if current context can be reused.
  if s:ValidateContext('normal_line') | return b:context | endif

  let context = a:context
  let context.state = 'normal_line'

  let [context.lnum, found] = s:FindTagStart(context.lnum)
  let text = getline(context.lnum)

  call cursor(context.lnum, 1) | normal! $

  let context.root_block = s:CurrentBlockId()

  call s:CountITags(tolower(text), context.root_block)

  let context.indent = indent(context.lnum) + (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) * s:indent_unit
  let context.ready = 1

  return context

endfunc "}}}

" Initializes context for a specific line.
"
" A context stores context information that is useful for calculating indent
" of current line. Two steps are required before the indent of current line
" can be determined:
"
" 1. Initialize context for current line. The value of indent in this context
" can not be used as the final value yet.
"
" 2. Calling the indent method to further adjust indent value based on the
" context.
"
" This method presents the first step.
"
" @param lnum The line number to be checked.
" @return The initialized context.
func! s:InitContext(lnum)
  "{{{
  let context = {}
  let context.lnum = prevnonblank(a:lnum - 1)
  let context.indent = -1
  let context.changed_tick = 0
  let context.block = 0
  let context.block_start_tag_lnum = 0
  let context.script_type = ""
  let context.indent_inside_block = -1
  let context.root_block = 0
  let context.state = 'normal_line'
  let context.ready = 0

  if context.lnum == 0 | return context | endif

  " Look backward from current line to find a block end or start tag.
  "
  " 1. If it is a block end tag:
  "
  "       1.1 If it is the previous line, align current line to the line of
  "       the corresponding block start tag.
  "
  "       1.2 If it is not the previous line, current line is aligned to the
  "       previous line as a normal line.
  "
  "       NOTE: If current block is also inside a root block, it won't be
  "       necessary to indent current line as a line-inside-block, because the
  "       line of block start tag would have already been indented as a
  "       line-inside-block (line inside the parent block).
  "
  " 2. If it is a block open tag, then current line is inside the block.
  "
  " ...<block>
  "         ^ <- current_block_tag_end_col
  "     ^     <- current_block_tag_start_col
  let [current_block_tag, current_block_tag_lnum, current_block_tag_start_col, current_block_tag_end_col] = s:SearchStringPosition(s:BlockTagPattern(), "bnW", 0)

  let current_block_tag_line = tolower(getline(current_block_tag_lnum))

  " Try to initialize context for current line as if it is inside an attribute.
  let context = s:InitContextOfLineInsideAttribute(context)
  if context.ready | return context | endif

  " Current line is not inside any attribute, try to initialize context as if it
  " is inside a tag.
  let context = s:InitContextOfLineInsideTag(context)
  if context.ready | return context | endif

  " Current line is not inside any tag, try to initialize context as if it is
  " inside a block.
  let context = s:InitContextOfLineInsideBlock(context, current_block_tag_line, current_block_tag, current_block_tag_lnum, current_block_tag_start_col)
  if context.ready | return context | endif

  " Current line is not inside any block, try to initialize context as if it
  " starts with an end tag.
  let context = s:InitContextOfLineStartsWithEndTag(context)
  if context.ready | return context | endif

  " Current line does not start with any end tag, try to initialize context as
  " if its previous line ends with an end tag.
  let context = s:InitContextOfLineAfterEndTag(context)
  if context.ready | return context | endif

  " The previous line does not end with any end tag, try to initialize context
  " as if it is after a block.
  " We should check if current line starts with an end tag, or if its previous
  " line ends with an end tag before this method, because a line after a block
  " may starts with or be after an end tag as well.
  let context = s:InitContextOfLineAfterBlock(context, current_block_tag_line, current_block_tag, current_block_tag_lnum, current_block_tag_start_col)
  if context.ready | return context | endif

  " Current line is not inside any block, and it does not start with any end
  " tag, and its previous line does not end with any end tag, and it is not
  " after any block, so it is just a normal line.
  let context = s:InitContextOfNormalLine(context)
  if context.ready | return context | endif

endfunc "}}}

" Indent method for lines inside a <pre> block: keep indent as-is.
" @return -1
func! s:Alien2()
  "{{{
  let b:indent_ready = 1 | return -1
endfunc "}}}

" Indent method for lines inside a <script> block.
" @return The calculated indent.
func! s:Alien3()
  "{{{
  let b:indent_ready = 1
  let lnum = prevnonblank(v:lnum - 1)

  " indent for the first line after <script>
  if lnum == b:context.block_start_tag_lnum | return eval(b:hi_js1indent) | endif

  let indent = -1

  if b:context.script_type == "javascript"
    setlocal comments+=s1:/*,mb:*,ex:*/

    if exists("b:did_indent") | unlet b:did_indent | endif
    runtime! indent/javascript.vim

    if exists("b:current_syntax") | unlet b:current_syntax | endif
    runtime! syntax/javascript.vim

    let indent = max([GetJavascriptIndent(), eval(b:hi_js1indent)])
    call s:RestoreLocalConfiguration()
  endif

  return indent

endfunc "}}}

" Indent method of lines inside a <style> block.
" @return The calculated indent.
func! s:Alien4()
  "{{{
  let b:indent_ready = 1
  let prev_lnum = prevnonblank(v:lnum - 1)

  setlocal comments+=s1:/*,mb:*,ex:*/

  " indent for first content line
  if prev_lnum == b:context.block_start_tag_lnum | return eval(b:hi_css1indent) | endif

  " Unlet b:did_indent, so that indent/css.vim can be loaded successfully.
  if exists("b:did_indent") | unlet b:did_indent | endif
  runtime! indent/css.vim

  if exists("b:current_syntax") | unlet b:current_syntax | endif
  runtime! syntax/css.vim

  " Calculate indent with the function in indent/css.vim
  let indent = max([GetCSSIndent(), eval(b:hi_css1indent)])

  " Restore local configuration after calling external scripts.
  call s:RestoreLocalConfiguration()

  return indent

endfunc "}}}

" Indent method of lines inside a <!-- and --> comment block.
" @return The calculated indent.
func! s:Alien5()
  "{{{
  let b:indent_ready = 1
  let prev_lnum = prevnonblank(v:lnum - 1)

  let indent = prev_lnum == b:context.block_start_tag_lnum ? indent(prev_lnum) + 1 : indent(prev_lnum)

  return indent

endfunc "}}}

" Indent method of lines inside a <!--[...]> and <![endif]--> conditional comment block.
" @return The calculated indent.
func! s:Alien6()
  "{{{
  " Mark b:indent_ready to 1 if current line is the first line inside a
  " conditional comment, 0 otherwise.
  " So basically, this method is only effective for the first line inside a
  " conditional comment.
  let b:indent_ready = b:context.lnum == b:context.block_start_tag_lnum ? 1 : 0

  return b:context.indent_inside_block

endfunc "}}}

" When the 'lnum' line ends in '>', finds the line containing the matching '<'
" (for start tag) or '</' (for end tag).
"
" This method is only called by method s:InitContextOfNormalLine(), which
" means the line of lnum is neither the block start line, nor the block end
" line, so it may only contain normal tags. Therefore, it will be good enough
" to pair tags with '<' and '>'.
"
" Example A:
" ----------
" <div>
"     ^ <- tag end
" ^ <- tag start
"
" Example B:
" ---------
" <div
" ^ <- tag start
"     >
"     ^ <- tag end
"
" Example C:
" ----------
" </div>
"      ^ <- tag end
" ^ <- tag start
"
" Example D:
" ----------
" </div
" ^ <- tag start
"     >
"     ^ <- tag end
"
" @param lnum The line number of the line to be checked.
func! s:FindTagStart(lnum)
  "{{{
  let lnum = 0
  let idx = match(tolower(getline(a:lnum)), '\S>[^<>]*$') + 1

  if idx > 0
    call cursor(a:lnum, idx)

    " Match < or </
    let lnum = searchpair('<\%\(/\|\w\)', '' , '\S>', 'bW', '', max([a:lnum - b:html_indent_line_limit, 0]))
  endif

  return lnum > 0 ? [lnum, 1]: [a:lnum, 0]

endfunc "}}}

" Finds the unclosed start tag from the current cursor position. The cursor
" must be on or before an end tag.
" @return Line number of the start tag, or 0 on failure.
func! s:FindStartTag()
  "{{{
  let pattern = s:custom_end_tag_pattern . '\|' . s:BlockEndTagPattern() . '\|' . s:normal_end_tag_pattern
  let end_tag = matchstr(tolower(getline('.')[col('.') - 1:]), pattern)
  let end_pattern = s:EndTagPattern(end_tag, 0)
  let start_pattern = s:StartTagPattern(s:StartTag(end_tag), 0)
  let start_lnum = searchpair(start_pattern, '', end_pattern, 'bW')

  return start_lnum > 0 ? start_lnum : 0

endfunc "}}}

" Calculates indent of a line that is inside an HTML attribute.
" @return The calculated indent, or -1 on failure.
func! s:IndentOfLineInsideAttribute()
  "{{{
  let indent = -1

  if b:context.state == 'line_inside_attribute' && b:context.lnum > 1
    let indent = exists('b:html_indent_tag_string_func') ? b:html_indent_tag_string_func(b:context.lnum) : b:context.indent
    let b:indent_ready = 1
    " No need to update indent for lines inside an attribute.
    call s:UpdateContext(0)
  endif

  return indent

endfunc "}}}

" Calculates indent of a line that is an HTML attribute.
" @return The calculated indent, or -1 on failure.
func! s:IndentOfLineInsideTag()
  "{{{
  let indent = -1
  let lnum = v:lnum

  while b:context.state == 'line_inside_tag' && lnum > 1 && indent <= 0
    let lnum -= 1
    let text = tolower(getline(lnum))

    " Find a match with one of these, align with "attr":
    "       attr=
    "  <tag attr=
    "  text<tag attr=
    "  <tag>text</tag>text<tag attr=
    " For long lines search for the first match, finding the last match
    " gets very slow.
    let indent = len(text) < 300 ? match(text, '.*\s\zs[_a-zA-Z0-9-]\+="') : match(text, '\s\zs[_a-zA-Z0-9-]\+="')
  endwhile

  if b:context.state == 'line_inside_tag' && indent >= 0
    let b:indent_ready = 1
    let b:context.indent = indent
    " No need to udpate indent for lines inside a tag.
    call s:UpdateContext(0)
  endif

  return indent

endfunc "}}}

" Calculates indent of current line if it's inside a block.
" @return The calculated indent, or -1 on failure.
func! s:IndentOfLineInsideBlock()
  "{{{
  let indent = -1

  if b:context.state == 'line_inside_block'
    " within block
    let alien = s:block_tags[b:context.block].indent_alien
    let indent = {alien}()
    if b:indent_ready
      let b:context.indent = indent
      " No need to update indent for lines inside block.
      call s:UpdateContext(0)
    endif
  endif

  return indent

endfunc "}}}

" Calculates indent of current line if it's a normal line.
" @return The calculated indent.
func! s:IndentOfNormalLine()
  "{{{
  let indent = b:context.indent
  let b:indent_ready = 1
  call s:UpdateContext(1)

  return indent

endfunc "}}}

" After current line has been indented, performs incremental update on
" b:context, so that it will be suitable for indenting the next line.
" @param update_indent A flag indicates if indent of current line needs to be
" recalculated, 1 by default.
func! s:UpdateContext(update_indent)
  "{{{
  let b:context.lnum = v:lnum

  let b:context.changed_tick = b:changedtick

  if a:update_indent

    call cursor(v:lnum, 1) | normal! $

    call s:CountITags(tolower(getline(v:lnum)), b:context.root_block)

    let b:context.indent = b:context.indent + (b:tag_counter.current_line_offset + b:tag_counter.next_line_offset) * s:indent_unit

  endif

endfunc "}}}

" THE MAIN INDENT FUNCTION.
" @return The amount of indent for v:lnum.
func! HtmlIndent()
  "{{{
  let b:indent_ready = 0

  " Remove JavaScript and CSS style comments.
  setlocal comments-=s1:/*,mb:*,ex:*/

  " First non-blank line has no indent.
  if prevnonblank(v:lnum - 1) < 1 | return 0 | endif

  " Initialize a new context or reuse the previous context if it can be
  " cached.
  let b:context = s:InitContext(v:lnum)

  " Check if current line is within an attribute value, and calculate indent
  " accordingly.
  let indent = s:IndentOfLineInsideAttribute()
  if b:indent_ready | return indent | endif

  " Current line is not within an attribute, now check if it is an attribute
  " and calculate indent accordingly.
  let indent = s:IndentOfLineInsideTag()
  if b:indent_ready | return indent | endif

  " Current line is not an attribute, now Check if it is inside a block tag
  " (block start and block end are not included), and calculate indent
  " accordingly.
  let indent = s:IndentOfLineInsideBlock()
  if b:indent_ready | return indent | endif

  " Current line is a normal line: not an attribute, not inside any tags, and
  " not inside any blocks. But it may starts with an end tag, after an end
  " tag, after a block, or just a simple line.
  let indent = s:IndentOfNormalLine()
  if b:indent_ready | return indent | endif

endfunc "}}}

" Initialize local configuration
call s:RestoreLocalConfiguration()

" Check user settings when loading this script the first time.
call HtmlIndent_CheckUserSettings()

let &cpo = s:cpo_save
unlet s:cpo_save

" vim: fdm=marker ts=8 sw=2 tw=78
