" VHDL indent ('93 syntax)
" Language:    VHDL
" Maintainer:  Gerald Lai <laigera+vim?gmail.com>
" Version:     1.36
" Last Change: 2006 Apr 12
" URL:         http://www.vim.org/scripts/script.php?script_id=1450

" only load this indent file when no other was loaded
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

" setup indent options for local VHDL buffer
setlocal indentexpr=GetVHDLindent()
setlocal indentkeys=!^F,o,O,e,0(,0)
setlocal indentkeys+==~if,=~then,=~elsif,=~else
setlocal indentkeys+==~begin,=~is,=~select

" constants
" not a comment
let s:NC = '\%(--.*\)\@<!'
" end of string
let s:ES = '\s*\%(--.*\)\=$'
" no "end" keyword in front
let s:NE = '\%(\<end\s\+\)\@<!'

" only define indent function once
if exists("*GetVHDLindent")
  finish
endif

function GetVHDLindent()
  " store current line & string
  let curn = v:lnum
  let curs = getline(curn)

  " find previous line that is not a comment
  let prevn = prevnonblank(curn - 1)
  let prevs = getline(prevn)
  while prevn > 0 && prevs =~ '^\s*--'
    let prevn = prevnonblank(prevn - 1)
    let prevs = getline(prevn)
  endwhile

  " default indent starts as previous non-comment line's indent
  let ind = prevn > 0 ? indent(prevn) : 0
  " backup default
  let ind2 = ind

  " indent:   special; kill string so it would not affect other filters
  " keywords: "report" + string
  " where:    anywhere in current or previous line
  let s0 = s:NC.'\<report\>\s*".*"'
  if curs =~? s0
    let curs = ""
  endif
  if prevs =~? s0
    let prevs = ""
  endif

  " indent:   previous line's comment position, otherwise follow next non-comment line if possible
  " keyword:  "--"
  " where:    start of current line
  if curs =~ '^\s*--'
    let pn = curn - 1
    let ps = getline(pn)
    if ps =~ '--'
      return stridx(ps, '--')
    else
      " find nextnonblank line that is not a comment
      let nn = nextnonblank(curn + 1)
      let ns = getline(nn)
      while nn > 0 && ns =~ '^\s*--'
        let nn = nextnonblank(nn + 1)
        let ns = getline(nn)
      endwhile
      let n = indent(nn)
      return n != -1 ? n : ind
    endif
  endif

  " ****************************************************************************************
  " indent:   align generic variables & port names
  " keywords: "generic", "map", "port" + "(", provided current line is part of mapping
  " where:    anywhere in previous 2 lines
  " find following previous non-comment line
  let pn = prevnonblank(prevn - 1)
  let ps = getline(pn)
  while pn > 0 && ps =~ '^\s*--'
    let pn = prevnonblank(pn - 1)
    let ps = getline(pn)
  endwhile
  if (curs =~ '^\s*)' || curs =~? '^\s*\%(\<\%(generic\|map\|port\)\>.*\)\@<!\S\+\s*\%(=>\s*\S\+\|:[^=]\@=\s*\%(\%(in\|out\|inout\|buffer\|linkage\)\>\|\w\+\s\+:=\)\)') && (prevs =~? s:NC.'\<\%(generic\|map\|port\)\s*(\%(\s*\w\)\=' || (ps =~? s:NC.'\<\%(generic\|map\|port\)'.s:ES && prevs =~ '^\s*('))
    " align closing ")" with opening "("
    if curs =~ '^\s*)'
      return stridx(prevs, '(')
    endif
    let m = matchend(prevs, '(\s*\ze\w')
    if m != -1
      return m
    else
      return stridx(prevs, '(') + &sw
    endif
  endif

  " indent:   align conditional/select statement
  " keywords: variable + "<=" without ";" ending
  " where:    start of previous line
  if prevs =~? '^\s*\S\+\s*<=[^;]*'.s:ES
    return matchend(prevs, '<=\s*\ze.')
  endif

  " indent:   backtrace previous non-comment lines for next smaller or equal size indent
  " keywords: "end" + "record", "units"
  " where:    start of previous line
  " keyword:  ")"
  " where:    start of previous line
  " keyword:  without "<=" + ";" ending
  " where:    anywhere in previous line
  " keyword:  "=>" + ")" ending, provided current line does not begin with ")"
  " where:    anywhere in previous line
  " _note_:   indent allowed to leave this filter
  let m = 0
  if prevs =~? '^\s*end\s\+\%(record\|units\)\>'
    let m = 3
  elseif prevs =~ '^\s*)'
    let m = 1
  elseif prevs =~ s:NC.'\%(<=.*\)\@<!;'.s:ES || (curs !~ '^\s*)' && prevs =~ s:NC.'=>.*'.s:NC.')'.s:ES)
    let m = 2
  endif

  if m > 0
    let pn = prevnonblank(prevn - 1)
    let ps = getline(pn)
    while pn > 0
      let t = indent(pn)
      if ps !~ '^\s*--' && t < ind
        " make sure one of these is true
        " keywords: variable + "<=" without ";" ending
        " where:    start of previous non-comment line
        " keywords: "generic", "map", "port"
        " where:    anywhere in previous non-comment line
        " keyword:  "("
        " where:    start of previous non-comment line
        if m < 3 && ps !~? '^\s*\S\+\s*<=[^;]*'.s:ES
          if ps =~? s:NC.'\<\%(generic\|map\|port\)\>' || ps =~ '^\s*('
            let ind = t
          endif
          break
        endif
        let ind = t
        if m > 1
          " find following previous non-comment line
          let ppn = prevnonblank(pn - 1)
          let pps = getline(ppn)
          while ppn > 0 && pps =~ '^\s*--'
            let ppn = prevnonblank(ppn - 1)
            let pps = getline(ppn)
          endwhile
          " indent:   follow
          " keyword:  "select"
          " where:    end of following previous non-comment line
          " keyword:  "type"
          " where:    start of following previous non-comment line
          if m == 2
            let s1 = s:NC.'\<select'.s:ES
            if ps !~? s1 && pps =~? s1
              let ind = indent(ppn)
            endif
          elseif m == 3
            let s1 = '^\s*type\>'
            if ps !~? s1 && pps =~? s1
              let ind = indent(ppn)
            endif
          endif
        endif
        break
      endif
      let pn = prevnonblank(pn - 1)
      let ps = getline(pn)
    endwhile
  endif

  " indent:   follow indent of previous opening statement, otherwise -sw
  " keyword:  "begin"
  " where:    anywhere in current line
  if curs =~? s:NC.'\<begin\>'
    let ind = ind - &sw
    " find previous opening statement of
    " keywords: "architecture", "block", "entity", "function", "generate", "procedure", "process"
    let s2 = s:NC.s:NE.'\<\%(architecture\|block\|entity\|function\|generate\|procedure\|process\)\>'
    if curs !~? s2.'.*'.s:NC.'\<begin\>.*'.s:ES && prevs =~? s2
      let ind = ind + &sw
    endif
    return ind
  endif

  " indent:   +sw if previous line is previous opening statement
  " keywords: "record", "units"
  " where:    anywhere in current line
  if curs =~? s:NC.s:NE.'\<\%(record\|units\)\>'
    " find previous opening statement of
    " keyword: "type"
    let s3 = s:NC.s:NE.'\<type\>'
    if curs !~? s3.'.*'.s:NC.'\<\%(record\|units\)\>.*'.s:ES && prevs =~? s3
      let ind = ind + &sw
    endif
    return ind
  endif

  " ****************************************************************************************
  " indent:   0
  " keywords: "architecture", "configuration", "entity", "library", "package"
  " where:    start of current line
  if curs =~? '^\s*\%(architecture\|configuration\|entity\|library\|package\)\>'
    return 0
  endif

  " indent:   maintain indent of previous opening statement
  " keyword:  "is"
  " where:    start of current line
  " find previous opening statement of
  " keywords: "architecture", "block", "configuration", "entity", "function", "package", "procedure", "process", "type"
  if curs =~? '^\s*\<is\>' && prevs =~? s:NC.s:NE.'\<\%(architecture\|block\|configuration\|entity\|function\|package\|procedure\|process\|type\)\>'
    return ind2
  endif

  " indent:   maintain indent of previous opening statement
  " keyword:  "then"
  " where:    start of current line
  " find previous opening statement of
  " keywords: "elsif", "if"
  if curs =~? '^\s*\<then\>' && prevs =~? s:NC.'\%(\<elsif\>\|'.s:NE.'\<if\>\)'
    return ind2
  endif

  " indent:   maintain indent of previous opening statement
  " keyword:  "generate"
  " where:    start of current line
  " find previous opening statement of
  " keywords: "for", "if"
  if curs =~? '^\s*\<generate\>' && prevs =~? s:NC.s:NE.'\%(\%(\<wait\s\+\)\@<!\<for\>\|\<if\>\)'
    return ind2
  endif

  " indent:   +sw
  " keywords: "begin", "block", "loop", "process", "record", "units"
  " removed:  "case", "elsif", "if", "while"
  " where:    anywhere in previous line
  if prevs =~? s:NC.'\%(\<begin\>\|'.s:NE.'\<\%(block\|loop\|process\|record\|units\)\>\)'
    return ind + &sw
  endif

  " indent:   +sw
  " keywords: "architecture", "component", "configuration", "entity", "for", "package"
  " removed:  "when", "with"
  " where:    start of previous line
  if prevs =~? '^\s*\%(architecture\|component\|configuration\|entity\|for\|package\)\>'
    return ind + &sw
  endif

  " indent:   +sw
  " keyword:  "generate", "is", "select", "=>"
  " where:    end of previous line
  if prevs =~? s:NC.'\%(\%('.s:NE.'\<generate\|\<is\|\<select\)\|=>\)'.s:ES
    return ind + &sw
  endif

  " indent:   +sw
  " keyword:  "else"
  " where:    start of previous line
  " keyword:  "then"
  " where:    end of previous line
  " _note_:   indent allowed to leave this filter
  if prevs =~? '^\s*else\>' || prevs =~? s:NC.'\<then'.s:ES
    let ind = ind + &sw
  endif

  " ****************************************************************************************
  " indent:   -sw
  " keywords: "when", provided previous line does not begin with "when"
  " where:    start of current line
  let s4 = '^\s*when\>'
  if curs =~? s4
    if prevs !~? s4
      return ind - &sw
    else
      return ind2
    endif
  endif

  " indent:   -sw
  " keywords: "else", "elsif", provided previous line does not contain "then"
  " where:    start of current line
  if curs =~? '^\s*\%(else\|elsif\)\>'
    if prevs !~? s:NC.'\<then\>'
      return ind - &sw
    else
      return ind2
    endif
  endif

  " indent:   -sw
  " keywords: "end" + "if", provided previous line does not begin with "else", not contain "then"
  " where:    start of current line
  if curs =~? '^\s*end\s\+if\>'
    if prevs !~? '^\s*else\>' && prevs !~? s:NC.'\<then\>'
      return ind - &sw
    else
      return ind2
    endif
  endif

  " indent:   -sw
  " keywords: "end" + "function", "procedure", provided previous line does not contain "begin"
  " where:    start of current line
  if curs =~? '^\s*end\s\+\%(function\|procedure\)\>'
    if prevs !~? s:NC.'\<begin\>'
      return ind - &sw
    else
      return ind2
    endif
  endif

  " indent:   -sw
  " keywords: "end" + "block", "for", "generate", "loop", "process", "record", "units"
  " where:    start of current line
  if curs =~? '^\s*end\s\+\%(block\|for\|generate\|loop\|process\|record\|units\)\>'
    return ind - &sw
  endif

  " indent:   backtrace previous non-comment lines
  " keyword:  "end" + "case", "component"
  " where:    start of current line
  let m = 0
  if curs =~? '^\s*end\s\+case\>'
    let m = 1
  elseif curs =~? '^\s*end\s\+component\>'
    let m = 2
  endif

  if m > 0
    " find following previous non-comment line
    let pn = prevn
    let ps = getline(pn)
    while pn > 0
      if ps !~ '^\s*--'
        "indent:   -2sw
        "keywords: "end" + "case"
        "where:    start of previous non-comment line
        "indent:   -sw
        "keywords: "when"
        "where:    start of previous non-comment line
        "indent:   follow
        "keywords: "case"
        "where:    start of previous non-comment line
        if m == 1
          if ps =~? '^\s*end\s\+case\>'
            return indent(pn) - 2 * &sw
          elseif ps =~? '^\s*when\>'
            return indent(pn) - &sw
          elseif ps =~? '^\s*case\>'
            return indent(pn)
          endif
        "indent:   follow
        "keyword:  "component"
        "where:    anywhere in previous non-comment line
        elseif m == 2
          if ps =~? s:NC.s:NE.'\<component\>'
            return indent(pn)
          endif
        endif
      endif
      let pn = prevnonblank(pn - 1)
      let ps = getline(pn)
    endwhile
    return ind - &sw
  endif

  " indent:   -sw
  " keyword:  ")"
  " where:    start of current line
  if curs =~ '^\s*)'
    return ind - &sw
  endif

  " indent:   0
  " keywords: "end" + "architecture", "configuration", "entity", "package"
  " where:    start of current line
  if curs =~? '^\s*end\s\+\%(architecture\|configuration\|entity\|package\)\>'
    return 0
  endif

  " indent:   -sw
  " keywords: "end" + identifier
  " where:    start of current line
  if curs =~? '^\s*end\s\+\w\+\>'
    return ind - &sw
  endif

  " ****************************************************************************************
  " indent:   maintain indent of previous opening statement
  " keywords: without "generic", "map", "port" + ":" but not ":=" + "in", "out", "inout", "buffer", "linkage", variable & ":="
  " where:    start of current line
  if curs =~? '^\s*\%(\<\%(generic\|map\|port\)\>.*\)\@<!\S\+\s*:[^=]\@=\s*\%(\%(in\|out\|inout\|buffer\|linkage\)\>\|\w\+\s\+:=\)'
    return ind2
  endif

  " return leftover filtered indent
  return ind
endfunction
