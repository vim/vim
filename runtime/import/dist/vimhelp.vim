vim9script

# Extra functionality for displaying Vim help .

# Called when editing the doc/syntax.txt file
export def HighlightGroups()
  var buf: number = bufnr('%')
  var lnum: number = search('\*highlight-groups\*', 'cn')
  while getline(lnum) !~ '===' && lnum < line('$')
    var word: string = getline(lnum)->matchstr('^\w\+\ze\t')
    if word->hlexists()
      prop_type_add('help-hl-' .. word, {
	bufnr: buf,
	highlight: word,
	combine: false,
	})
      prop_add(lnum, 1, {length: word->strlen(), type: 'help-hl-' .. word})
    endif
    ++lnum
  endwhile
enddef
