vim9script

# Extra functionality for displaying Vim help .

# Called when editing the doc/syntax.txt file
export def HighlightGroups()
  var buf: number = bufnr('%')
  var lnum: number = search('\*highlight-groups\*', 'cn')
  while getline(lnum) !~ '===' && lnum < line('$')
    var word: string = getline(lnum)->matchstr('^\w\+\ze\t')
    if word->hlexists()
      var name = 'help-hl-' .. word
      if prop_type_list({bufnr: buf})->match(name) == -1
	prop_type_add('help-hl-' .. word, {
	  bufnr: buf,
	  highlight: word,
	  combine: false,
	  })
      else
	# was called before, delete existing properties
	prop_remove({type: name, bufnr: buf})
      endif
      prop_add(lnum, 1, {length: word->strlen(), type: 'help-hl-' .. word})
    endif
    ++lnum
  endwhile
enddef
