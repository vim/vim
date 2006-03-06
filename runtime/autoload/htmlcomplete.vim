" Vim completion script
" Language:	XHTML 1.0 Strict
" Maintainer:	Mikolaj Machowski ( mikmach AT wp DOT pl )
" Last Change:	2006 Mar 5

function! htmlcomplete#CompleteTags(findstart, base)
  if a:findstart
    " locate the start of the word
    let line = getline('.')
    let start = col('.') - 1
	let curline = line('.')
	let compl_begin = col('.') - 2
    while start >= 0 && line[start - 1] =~ '\(\k\|[:.-]\)'
		let start -= 1
    endwhile
	" Handling of entities {{{
	if start >= 0 && line[start - 1] =~ '&'
		let b:entitiescompl = 1
		let b:compl_context = ''
		return start
	endif
	" }}}
	" Handling of <style> tag {{{
	let stylestart = searchpair('<style\>', '', '<\/style\>', "bnW")
	let styleend   = searchpair('<style\>', '', '<\/style\>', "nW")
	if stylestart != 0 && styleend != 0 
		if stylestart <= curline && styleend >= curline
			let start = col('.') - 1
			let b:csscompl = 1
			while start >= 0 && line[start - 1] =~ '\(\k\|-\)'
				let start -= 1
			endwhile
		endif
	endif
	" }}}
	" Handling of <script> tag {{{
	let scriptstart = searchpair('<script\>', '', '<\/script\>', "bnW")
	let scriptend   = searchpair('<script\>', '', '<\/script\>', "nW")
	if scriptstart != 0 && scriptend != 0 
		if scriptstart <= curline && scriptend >= curline
			let start = col('.') - 1
			let b:jscompl = 1
			let b:jsrange = [scriptstart, scriptend]
			while start >= 0 && line[start - 1] =~ '\k'
				let start -= 1
			endwhile
			" We are inside of <script> tag. But we should also get contents
			" of all linked external files and (secondary, less probably) other <script> tags
			" This logic could possible be done in separate function - may be
			" reused in events scripting (also with option could be reused for
			" CSS
			let b:js_extfiles = []
			let l = line('.')
			let c = col('.')
			call cursor(1,1)
			while search('<\@<=script\>', 'W') && line('.') <= l
				if synIDattr(synID(line('.'),col('.')-1,0),"name") !~? 'comment'
					let sname = matchstr(getline('.'), '<script[^>]*src\s*=\s*\([''"]\)\zs.\{-}\ze\1')
					if filereadable(sname)
						let b:js_extfiles += readfile(sname)
					endif
				endif
			endwhile
			call cursor(1,1)
			let js_scripttags = []
			while search('<script\>', 'W') && line('.') < l
				if matchstr(getline('.'), '<script[^>]*src') == ''
					let js_scripttag = getline(line('.'), search('</script>', 'W'))
					let js_scripttags += js_scripttag
				endif
			endwhile
			let b:js_extfiles += js_scripttags
			call cursor(l,c)
			unlet! l c
		endif
	endif
	" }}}
	if !exists("b:csscompl") && !exists("b:jscompl")
		let b:compl_context = getline('.')[0:(compl_begin)]
		if b:compl_context !~ '<[^>]*$'
			" Look like we may have broken tag. Check previous lines.
			let i = 1
			while 1
				let context_line = getline(curline-i)
				if context_line =~ '<[^>]*$'
					" Yep, this is this line
					let context_lines = getline(curline-i, curline)
					let b:compl_context = join(context_lines, ' ')
					break
				elseif context_line =~ '>[^<]*$'
					" Normal tag line, no need for completion at all
					let b:compl_context = ''
					break
				endif
				let i += 1
			endwhile
			" Make sure we don't have counter
			unlet! i
		endif
		let b:compl_context = matchstr(b:compl_context, '.*\zs<.*')
		" Return proper start for on-events. Without that beginning of
		" completion will be badly reported
		if b:compl_context =~? 'on[a-z]*\s*=\s*\(''[^'']*\|"[^"]*\)$'
			let start = col('.') - 1
			while start >= 0 && line[start - 1] =~ '\k'
				let start -= 1
			endwhile
		endif
	else
		let b:compl_context = getline('.')[0:compl_begin]
	endif
    return start
  else
	" Initialize base return lists
    let res = []
    let res2 = []
	" a:base is very short - we need context
	let context = b:compl_context
	" Check if we should do CSS completion inside of <style> tag
	" or JS completion inside of <script> tag
	if exists("b:csscompl")
		unlet! b:csscompl
		let context = b:compl_context
		unlet! b:compl_context
		return csscomplete#CompleteCSS(0, context)
	elseif exists("b:jscompl")
		unlet! b:jscompl
		return javascriptcomplete#CompleteJS(0, a:base)
	else
		if len(b:compl_context) == 0 && !exists("b:entitiescompl")
			return []
		endif
		let context = matchstr(b:compl_context, '.\zs.*')
	endif
	unlet! b:compl_context
	" Entities completion {{{
	if exists("b:entitiescompl")
		unlet! b:entitiescompl

		if !exists("g:xmldata_xhtml10s")
			"runtime! autoload/xml/xhtml10s.vim
			call htmlcomplete#LoadData()
		endif

	    let entities =  g:xmldata_xhtml10s['vimxmlentities']

		if len(a:base) == 1
			for m in entities
				if m =~ '^'.a:base
					call add(res, m.';')
				endif
			endfor
			return res
		else
			for m in entities
				if m =~? '^'.a:base
					call add(res, m.';')
				elseif m =~? a:base
					call add(res2, m.';')
				endif
			endfor

			return res + res2
		endif


	endif
	" }}}
	if context =~ '>'
		" Generally if context contains > it means we are outside of tag and
		" should abandon action - with one exception: <style> span { bo
		if context =~ 'style[^>]\{-}>[^<]\{-}$'
			return csscomplete#CompleteCSS(0, context)
		elseif context =~ 'script[^>]\{-}>[^<]\{-}$'
			let b:jsrange = [line('.'), search('<\/script\>', 'nW')]
			return javascriptcomplete#CompleteJS(0, context)
		else
			return []
		endif
	endif

	" If context contains > it means we are already outside of tag and we
	" should abandon action
	" If context contains white space it is attribute. 
	" It can be also value of attribute.
	" We have to get first word to offer proper completions
	if context == ''
		let tag = ''
	else
		let tag = split(context)[0]
	endif
	" Get last word, it should be attr name
	let attr = matchstr(context, '.*\s\zs.*')
	" Possible situations where any prediction would be difficult:
	" 1. Events attributes
	if context =~ '\s'
		" Sort out style, class, and on* cases
		if context =~? "\\(on[a-z]*\\|id\\|style\\|class\\)\\s*=\\s*[\"']"
			" Id, class completion {{{
			if context =~? "\\(id\\|class\\)\\s*=\\s*[\"'][a-zA-Z0-9_ -]*$"
				if context =~? "class\\s*=\\s*[\"'][a-zA-Z0-9_ -]*$"
					let search_for = "class"
				elseif context =~? "id\\s*=\\s*[\"'][a-zA-Z0-9_ -]*$"
					let search_for = "id"
				endif
				" Handle class name completion
				" 1. Find lines of <link stylesheet>
				" 1a. Check file for @import
				" 2. Extract filename(s?) of stylesheet,
				call cursor(1,1)
				let head = getline(search('<head\>'), search('<\/head>'))
				let headjoined = join(copy(head), ' ')
				if headjoined =~ '<style'
					" Remove possibly confusing CSS operators
					let stylehead = substitute(headjoined, '+>\*[,', ' ', 'g')
					if search_for == 'class'
						let styleheadlines = split(stylehead)
						let headclasslines = filter(copy(styleheadlines), "v:val =~ '\\([a-zA-Z0-9:]\\+\\)\\?\\.[a-zA-Z0-9_-]\\+'")
					else
						let stylesheet = split(headjoined, '[{}]')
						" Get all lines which fit id syntax
						let classlines = filter(copy(stylesheet), "v:val =~ '#[a-zA-Z0-9_-]\\+'")
						" Filter out possible color definitions
						call filter(classlines, "v:val !~ ':\\s*#[a-zA-Z0-9_-]\\+'")
						" Filter out complex border definitions
						call filter(classlines, "v:val !~ '\\(none\\|hidden\\|dotted\\|dashed\\|solid\\|double\\|groove\\|ridge\\|inset\\|outset\\)\\s*#[a-zA-Z0-9_-]\\+'")
						let templines = join(classlines, ' ')
						let headclasslines = split(templines)
						call filter(headclasslines, "v:val =~ '#[a-zA-Z0-9_-]\\+'")
					endif
					let internal = 1
				else
					let internal = 0
				endif
				let styletable = []
				let secimportfiles = []
				let filestable = filter(copy(head), "v:val =~ '\\(@import\\|link.*stylesheet\\)'")
				for line in filestable
					if line =~ "@import"
						let styletable += [matchstr(line, "import\\s\\+\\(url(\\)\\?[\"']\\?\\zs\\f\\+\\ze")]
					elseif line =~ "<link"
						let styletable += [matchstr(line, "href\\s*=\\s*[\"']\\zs\\f\\+\\ze")]
					endif
				endfor
				for file in styletable
					if filereadable(file)
						let stylesheet = readfile(file)
						let secimport = filter(copy(stylesheet), "v:val =~ '@import'")
						if len(secimport) > 0
							for line in secimport
								let secfile = matchstr(line, "import\\s\\+\\(url(\\)\\?[\"']\\?\\zs\\f\\+\\ze")
								let secfile = fnamemodify(file, ":p:h").'/'.secfile
								let secimportfiles += [secfile]
							endfor
						endif
					endif
				endfor
				let cssfiles = styletable + secimportfiles
				let classes = []
				for file in cssfiles
					if filereadable(file)
						let stylesheet = readfile(file)
						let stylefile = join(stylesheet, ' ')
						let stylefile = substitute(stylefile, '+>\*[,', ' ', 'g')
						if search_for == 'class'
							let stylesheet = split(stylefile)
							let classlines = filter(copy(stylesheet), "v:val =~ '\\([a-zA-Z0-9:]\\+\\)\\?\\.[a-zA-Z0-9_-]\\+'")
						else
							let stylesheet = split(stylefile, '[{}]')
							" Get all lines which fit id syntax
							let classlines = filter(copy(stylesheet), "v:val =~ '#[a-zA-Z0-9_-]\\+'")
							" Filter out possible color definitions
							call filter(classlines, "v:val !~ ':\\s*#[a-zA-Z0-9_-]\\+'")
							" Filter out complex border definitions
							call filter(classlines, "v:val !~ '\\(none\\|hidden\\|dotted\\|dashed\\|solid\\|double\\|groove\\|ridge\\|inset\\|outset\\)\\s*#[a-zA-Z0-9_-]\\+'")
							let templines = join(classlines, ' ')
							let stylelines = split(templines)
							let classlines = filter(stylelines, "v:val =~ '#[a-zA-Z0-9_-]\\+'")

						endif
					endif
					" We gathered classes definitions from all external files
					let classes += classlines
				endfor
				if internal == 1
					let classes += headclasslines
				endif

				if search_for == 'class'
					let elements = {}
					for element in classes
						if element =~ '^\.'
							let class = matchstr(element, '^\.\zs[a-zA-Z][a-zA-Z0-9_-]*\ze')
							let class = substitute(class, ':.*', '', '')
							if has_key(elements, 'common')
								let elements['common'] .= ' '.class
							else
								let elements['common'] = class
							endif
						else
							let class = matchstr(element, '[a-zA-Z1-6]*\.\zs[a-zA-Z][a-zA-Z0-9_-]*\ze')
							let tagname = tolower(matchstr(element, '[a-zA-Z1-6]*\ze.'))
							if tagname != ''
								if has_key(elements, tagname)
									let elements[tagname] .= ' '.class
								else
									let elements[tagname] = class
								endif
							endif
						endif
					endfor

					if has_key(elements, tag) && has_key(elements, 'common')
						let values = split(elements[tag]." ".elements['common'])
					elseif has_key(elements, tag) && !has_key(elements, 'common')
						let values = split(elements[tag])
					elseif !has_key(elements, tag) && has_key(elements, 'common')
						let values = split(elements['common'])
					else
						return []
					endif

				elseif search_for == 'id'
					" Find used IDs
					" 1. Catch whole file
					let filelines = getline(1, line('$'))
					" 2. Find lines with possible id
					let used_id_lines = filter(filelines, 'v:val =~ "id\\s*=\\s*[\"''][a-zA-Z0-9_-]\\+"')
					" 3a. Join all filtered lines 
					let id_string = join(used_id_lines, ' ')
					" 3b. And split them to be sure each id is in separate item
					let id_list = split(id_string, 'id\s*=\s*')
					" 4. Extract id values
					let used_id = map(id_list, 'matchstr(v:val, "[\"'']\\zs[a-zA-Z0-9_-]\\+\\ze")')
					let joined_used_id = ','.join(used_id, ',').','

					let allvalues = map(classes, 'matchstr(v:val, ".*#\\zs[a-zA-Z0-9_-]\\+")')

					let values = []

					for element in classes
						if joined_used_id !~ ','.element.','
							let values += [element]
						endif

					endfor

				endif

				" We need special version of sbase
				let classbase = matchstr(context, ".*[\"']")
				let classquote = matchstr(classbase, '.$')

				let entered_class = matchstr(attr, ".*=\\s*[\"']\\zs.*")

				for m in sort(values)
					if m =~? '^'.entered_class
						call add(res, m . classquote)
					elseif m =~? entered_class
						call add(res2, m . classquote)
					endif
				endfor

				return res + res2

			elseif context =~? "style\\s*=\\s*[\"'][^\"']*$"
				return csscomplete#CompleteCSS(0, context)

			endif
			" }}}
			" Complete on-events {{{
			if context =~? 'on[a-z]*\s*=\s*\(''[^'']*\|"[^"]*\)$'
				" We have to:
				" 1. Find external files
				let b:js_extfiles = []
				let l = line('.')
				let c = col('.')
				call cursor(1,1)
				while search('<\@<=script\>', 'W') && line('.') <= l
					if synIDattr(synID(line('.'),col('.')-1,0),"name") !~? 'comment'
						let sname = matchstr(getline('.'), '<script[^>]*src\s*=\s*\([''"]\)\zs.\{-}\ze\1')
						if filereadable(sname)
							let b:js_extfiles += readfile(sname)
						endif
					endif
				endwhile
				" 2. Find at least one <script> tag
				call cursor(1,1)
				let js_scripttags = []
				while search('<script\>', 'W') && line('.') < l
					if matchstr(getline('.'), '<script[^>]*src') == ''
						let js_scripttag = getline(line('.'), search('</script>', 'W'))
						let js_scripttags += js_scripttag
					endif
				endwhile
				let b:js_extfiles += js_scripttags

				" 3. Proper call for javascriptcomplete#CompleteJS
				call cursor(l,c)
				let js_context = matchstr(a:base, '\k\+$')
				let js_shortcontext = substitute(a:base, js_context.'$', '', '')
				let b:compl_context = context
				let b:jsrange = [l, l]
				unlet! l c
				return javascriptcomplete#CompleteJS(0, js_context)

			endif
				
			" }}}
			let stripbase = matchstr(context, ".*\\(on[a-zA-Z]*\\|style\\|class\\)\\s*=\\s*[\"']\\zs.*")
			" Now we have context stripped from all chars up to style/class.
			" It may fail with some strange style value combinations.
			if stripbase !~ "[\"']"
				return []
			endif
		endif
		" Value of attribute completion {{{
		" If attr contains =\s*[\"'] we catched value of attribute
		if attr =~ "=\s*[\"']"
			" Let do attribute specific completion
			let attrname = matchstr(attr, '.*\ze\s*=')
			let entered_value = matchstr(attr, ".*=\\s*[\"']\\zs.*")
			let values = []
			if attrname == 'media'
				let values = ["screen", "tty", "tv", "projection", "handheld", "print", "braille", "aural", "all"]
			elseif attrname == 'xml:space'
				let values = ["preserve"]
			elseif attrname == 'shape'
				let values = ["rect", "circle", "poly", "default"]
			elseif attrname == 'valuetype'
				let values = ["data", "ref", "object"]
			elseif attrname == 'method'
				let values = ["get", "post"]
			elseif attrname == 'dir'
				let values = ["ltr", "rtl"]
			elseif attrname == 'frame'
				let values = ["void", "above", "below", "hsides", "lhs", "rhs", "vsides", "box", "border"]
			elseif attrname == 'rules'
				let values = ["none", "groups", "rows", "all"]
			elseif attrname == 'align'
				let values = ["left", "center", "right", "justify", "char"]
			elseif attrname == 'valign'
				let values = ["top", "middle", "bottom", "baseline"]
			elseif attrname == 'scope'
				let values = ["row", "col", "rowgroup", "colgroup"]
			elseif attrname == 'href'
				" Now we are looking for local anchors defined by name or id
				if entered_value =~ '^#'
					let file = join(getline(1, line('$')), ' ')
					" Split it be sure there will be one id/name element in
					" item, it will be also first word [a-zA-Z0-9_-] in element
					let oneelement = split(file, "\\(meta \\)\\@<!\\(name\\|id\\)\\s*=\\s*[\"']")
					for i in oneelement
						let values += ['#'.matchstr(i, "^[a-zA-Z][a-zA-Z0-9%_-]*")]
					endfor
				endif
			elseif attrname == 'type'
				if context =~ '^input'
					let values = ["text", "password", "checkbox", "radio", "submit", "reset", "file", "hidden", "image", "button"]
				elseif context =~ '^button'
					let values = ["button", "submit", "reset"]
				elseif context =~ '^style'
					let values = ["text/css"]
				elseif context =~ '^script'
					let values = ["text/javascript"]
				endif
			else
				return []
			endif

			if len(values) == 0
				return []
			endif

			" We need special version of sbase
			let attrbase = matchstr(context, ".*[\"']")
			let attrquote = matchstr(attrbase, '.$')

			for m in values
				" This if is needed to not offer all completions as-is
				" alphabetically but sort them. Those beginning with entered
				" part will be as first choices
				if m =~ '^'.entered_value
					call add(res, m . attrquote.' ')
				elseif m =~ entered_value
					call add(res2, m . attrquote.' ')
				endif
			endfor

			return res + res2

		endif
		" }}}
		" Attribute completion {{{
		" Shorten context to not include last word
		let sbase = matchstr(context, '.*\ze\s.*')

		" Load data {{{
		if !exists("g:xmldata_xhtml10s")
			"runtime! autoload/xml/xhtml10s.vim
			call htmlcomplete#LoadData()
		endif
		" }}}
		"
		let attrs = keys(g:xmldata_xhtml10s[tag][1])

		for m in sort(attrs)
			if m =~ '^'.attr
				call add(res, m)
			elseif m =~ attr
				call add(res2, m)
			endif
		endfor
		let menu = res + res2
		if has_key(g:xmldata_xhtml10s, 'vimxmlattrinfo')
			let final_menu = []
			for i in range(len(menu))
				let item = menu[i]
				if has_key(g:xmldata_xhtml10s['vimxmlattrinfo'], item)
					let m_menu = g:xmldata_xhtml10s['vimxmlattrinfo'][item][0]
					let m_info = g:xmldata_xhtml10s['vimxmlattrinfo'][item][1]
					if m_menu !~ 'Bool'
						let item .= '="'
					endif
				else
					let m_menu = ''
					let m_info = ''
					let item .= '="'
				endif
				let final_menu += [{'word':item, 'menu':m_menu, 'info':m_info}]
			endfor
		else
			let final_menu = map(menu, 'v:val."=\""')
		endif
		return final_menu

	endif
	" }}}
	" Close tag {{{
	let b:unaryTagsStack = "base meta link hr br param img area input col"
	if context =~ '^\/'
		if context =~ '^\/.'
			return []
		else
			let opentag = xmlcomplete#GetLastOpenTag("b:unaryTagsStack")
			return [opentag.">"]
		endif
	endif
	" Load data {{{
	if !exists("g:xmldata_xhtml10s")
		"runtime! autoload/xml/xhtml10s.vim
		call htmlcomplete#LoadData()
	endif
	" }}}
	" Tag completion {{{
	" Deal with tag completion.
	let opentag = xmlcomplete#GetLastOpenTag("b:unaryTagsStack")
	" MM: TODO: GLOT works always the same but with some weird situation it
	" behaves as intended in HTML but screws in PHP
	let g:ot = opentag
	if opentag == '' || &ft == 'php' && !has_key(g:xmldata_xhtml10s, opentag)
		" Hack for sometimes failing GetLastOpenTag.
		" As far as I tested fail isn't GLOT fault but problem
		" of invalid document - not properly closed tags and other mish-mash.
		" Also when document is empty. Return list of *all* tags.
	    let tags = keys(g:xmldata_xhtml10s)
		call filter(tags, 'v:val !~ "^vimxml"')
	else
		let tags = g:xmldata_xhtml10s[opentag][0]
	endif
	" }}}

	for m in sort(tags)
		if m =~ '^'.context
			call add(res, m)
		elseif m =~ context
			call add(res2, m)
		endif
	endfor
	let menu = res + res2
	if has_key(g:xmldata_xhtml10s, 'vimxmltaginfo')
		let final_menu = []
		for i in range(len(menu))
			let item = menu[i]
			if has_key(g:xmldata_xhtml10s['vimxmltaginfo'], item)
				let m_menu = g:xmldata_xhtml10s['vimxmltaginfo'][item][0]
				let m_info = g:xmldata_xhtml10s['vimxmltaginfo'][item][1]
			else
				let m_menu = ''
				let m_info = ''
			endif
			let final_menu += [{'word':item, 'menu':m_menu, 'info':m_info}]
		endfor
	else
		let final_menu = menu
	endif
	return final_menu

	" }}}
  endif
endfunction
function! htmlcomplete#LoadData()
let g:xmldata_xhtml10s = {
\ 'vimxmlentities' : ["AElig", "Aacute", "Acirc", "Agrave", "Alpha", "Aring", "Atilde", "Auml", "Beta", "Ccedil", "Chi", "Dagger", "Delta", "ETH", "Eacute", "Ecirc", "Egrave", "Epsilon", "Eta", "Euml", "Gamma", "Iacute", "Icirc", "Igrave", "Iota", "Iuml", "Kappa", "Lambda", "Mu", "Ntilde", "Nu", "OElig", "Oacute", "Ocirc", "Ograve", "Omega", "Omicron", "Oslash", "Otilde", "Ouml", "Phi", "Pi", "Prime", "Psi", "Rho", "Scaron", "Sigma", "THORN", "TITY", "Tau", "Theta", "Uacute", "Ucirc", "Ugrave", "Upsilon", "Uuml", "Xi", "Yacute", "Yuml", "Zeta", "amp", "aacute", "acirc", "acute", "aelig", "agrave", "alefsym", "alpha", "and", "ang", "apos", "aring", "asymp", "atilde", "auml", "bdquo", "beta", "brvbar", "bull", "cap", "ccedil", "cedil", "cent", "chi", "circ", "clubs", "copy", "cong", "crarr", "cup", "curren", "dArr", "dagger", "darr", "deg", "delta", "diams", "divide", "eacute", "ecirc", "egrave", "empty", "ensp", "emsp", "epsilon", "equiv", "eta", "eth", "euro", "euml", "exist", "fnof", "forall", "frac12", "frac14", "frac34", "frasl", "gt", "gamma", "ge", "hArr", "harr", "hearts", "hellip", "iacute", "icirc", "iexcl", "igrave", "image", "infin", "int", "iota", "iquest", "isin", "iuml", "kappa", "lt", "laquo", "lArr", "lambda", "lang", "larr", "lceil", "ldquo", "le", "lfloor", "lowast", "loz", "lrm", "lsaquo", "lsquo", "macr", "mdash", "micro", "middot", "minus", "mu", "nbsp", "nabla", "ndash", "ne", "ni", "not", "notin", "nsub", "ntilde", "nu", "oacute", "ocirc", "oelig", "ograve", "oline", "omega", "omicron", "oplus", "or", "ordf", "ordm", "oslash", "otilde", "otimes", "ouml", "para", "part", "permil", "perp", "phi", "pi", "piv", "plusmn", "pound", "prime", "prod", "prop", "psi", "quot", "rArr", "raquo", "radic", "rang", "rarr", "rceil", "rdquo", "real", "reg", "rfloor", "rho", "rlm", "rsaquo", "rsquo", "sbquo", "scaron", "sdot", "sect", "shy", "sigma", "sigmaf", "sim", "spades", "sub", "sube", "sum", "sup", "sup1", "sup2", "sup3", "supe", "szlig", "tau", "there4", "theta", "thetasym", "thinsp", "thorn", "tilde", "times", "trade", "uArr", "uacute", "uarr", "ucirc", "ugrave", "uml", "upsih", "upsilon", "uuml", "weierp", "xi", "yacute", "yen", "yuml", "zeta", "zwj", "zwnj"],
\ 'vimxmlattrinfo' : {
\ 'accept' : ['ContentType', ''],
\ 'accesskey' : ['Character', ''],
\ 'action' : ['*URI', ''],
\ 'align' : ['String', ''],
\ 'alt' : ['*Text', ''],
\ 'archive' : ['UriList', ''],
\ 'axis' : ['CDATA', ''],
\ 'border' : ['Pixels', ''],
\ 'cellpadding' : ['Length', ''],
\ 'cellspacing' : ['Length', ''],
\ 'char' : ['Character', ''],
\ 'charoff' : ['Length', ''],
\ 'charset' : ['LangCode', ''],
\ 'checked' : ['Bool', ''],
\ 'class' : ['CDATA', 'Name of class, used for connecting element with style'],
\ 'codetype' : ['ContentType', ''],
\ 'cols' : ['*Number', ''],
\ 'colspan' : ['Number', ''],
\ 'content' : ['*CDATA', ''],
\ 'coords' : ['Coords', ''],
\ 'data' : ['URI', ''],
\ 'datetime' : ['DateTime', ''],
\ 'declare' : ['Bool', ''],
\ 'defer' : ['Bool', ''],
\ 'dir' : ['String', ''],
\ 'disabled' : ['Bool', ''],
\ 'enctype' : ['ContentType', ''],
\ 'for' : ['ID', ''],
\ 'headers' : ['IDREFS', ''],
\ 'height' : ['Number', ''],
\ 'href' : ['*URI', ''],
\ 'hreflang' : ['LangCode', ''],
\ 'id' : ['ID', 'Unique string'],
\ 'ismap' : ['Bool', ''],
\ 'label' : ['*Text', ''],
\ 'lang' : ['LangCode', ''],
\ 'longdesc' : ['URI', ''],
\ 'maxlength' : ['Number', ''],
\ 'media' : ['MediaDesc', ''],
\ 'method' : ['String', ''],
\ 'multiple' : ['Bool', ''],
\ 'name' : ['CDATA', ''],
\ 'nohref' : ['Bool', ''],
\ 'onblur' : ['Script', ''],
\ 'onchange' : ['Script', ''],
\ 'onclick' : ['Script', ''],
\ 'ondblclick' : ['Script', ''],
\ 'onfocus' : ['Script', ''],
\ 'onkeydown' : ['Script', ''],
\ 'onkeypress' : ['Script', ''],
\ 'onkeyup' : ['Script', ''],
\ 'onload' : ['Script', ''],
\ 'onmousedown' : ['Script', ''],
\ 'onmousemove' : ['Script', ''],
\ 'onmouseout' : ['Script', ''],
\ 'onmouseover' : ['Script', ''],
\ 'onmouseup' : ['Script', ''],
\ 'onreset' : ['Script', ''],
\ 'onselect' : ['Script', ''],
\ 'onsubmit' : ['Script', ''],
\ 'onunload' : ['Script', ''],
\ 'profile' : ['URI', ''],
\ 'readonly' : ['Bool', ''],
\ 'rel' : ['LinkTypes', ''],
\ 'rev' : ['LinkTypes', ''],
\ 'rows' : ['*Number', ''],
\ 'rules' : ['String', ''],
\ 'scheme' : ['CDATA', ''],
\ 'selected' : ['Bool', ''],
\ 'shape' : ['Shape', ''],
\ 'size' : ['CDATA', ''],
\ 'span' : ['Number', ''],
\ 'src' : ['*URI', ''],
\ 'standby' : ['Text', ''],
\ 'style' : ['StyleSheet', ''],
\ 'summary' : ['*Text', ''],
\ 'tabindex' : ['Number', ''],
\ 'title' : ['Text', ''],
\ 'type' : ['*ContentType', ''],
\ 'usemap' : ['URI', ''],
\ 'valign' : ['String', ''],
\ 'valuetype' : ['String', ''],
\ 'width' : ['Number', ''],
\ 'xmlns' : ['URI', '']
\ },
\ 'vimxmltaginfo' : {
\ 'base' : ['/>', ''],
\ 'meta' : ['/>', ''],
\ 'link' : ['/>', ''],
\ 'img' : ['/>', ''],
\ 'hr' : ['/>', ''],
\ 'br' : ['/>', ''],
\ 'param' : ['/>', ''],
\ 'area' : ['/>', ''],
\ 'input' : ['/>', ''],
\ 'col' : ['/>', '']
\ },
\ 'tr' : [
\ [
\ 'th',
\ 'td'
\ ],
\ {
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'charoff' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'align' : [
\ 'left',
\ 'center',
\ 'right',
\ 'justify',
\ 'char'
\ ],
\ 'valign' : [
\ 'top',
\ 'middle',
\ 'bottom',
\ 'baseline'
\ ],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'char' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'input' : [[],
\ {
\ 'ondblclick' : [],
\ 'onchange' : [],
\ 'readonly' : [
\  'BOOL'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'src' : [],
\ 'value' : [],
\ 'name' : [],
\ 'checked' : [
\ 'BOOL'
\ ],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : [],
\ 'type' : [
\ 'text',
\ 'password',
\ 'checkbox',
\ 'radio',
\ 'submit',
\ 'reset',
\ 'file',
\ 'hidden',
\ 'image',
\ 'button'
\ ],
\ 'accesskey' : [],
\ 'disabled' : [
\  'BOOL'
\ ],
\ 'usemap' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'size' : [],
\ 'onblur' : [],
\ 'onfocus' : [],
\ 'maxlength' : [],
\ 'onselect' : [],
\ 'accept' : [],
\ 'alt' : [],
\ 'tabindex' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'xml:lang' : []
\ }
\ ],
\ 'table' : [
\ [
\ 'caption',
\ 'col',
\ 'colgroup',
\ 'thead',
\ 'tfoot',
\ 'tbody',
\ 'tr'
\ ],
\ {
\ 'width' : [],
\ 'frame' : [
\ 'void',
\ 'above',
\ 'below',
\ 'hsides',
\ 'lhs',
\ 'rhs',
\ 'vsides',
\ 'box',
\ 'border'
\ ],
\ 'ondblclick' : [],
\ 'rules' : [
\ 'none',
\ 'groups',
\ 'rows',
\ 'cols',
\ 'all'
\ ],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'summary' : [],
\ 'onkeyup' : [],
\ 'cellspacing' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'border' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'cellpadding' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'form' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onsubmit' : [],
\ 'enctype' : [
\ '',
\ 'application/x-www-form-urlencoded',
\ ],
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onreset' : [],
\ 'onmouseup' : [],
\ 'method' : [
\ 'get',
\ 'post'
\ ],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'accept' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'accept-charset' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'action' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'h5' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'meta' : [[],
\ {
\ 'http-equiv' : [],
\ 'lang' : [],
\ 'name' : [],
\ 'scheme' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ]
\ }
\ ],
\ 'map' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript',
\ 'area'
\ ],
\ {
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'name' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'style' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'title' : [],
\ 'onclick' : [],
\ 'class' : []
\ }
\ ],
\ 'tfoot' : [
\ [
\ 'tr'
\ ],
\ {
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'charoff' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'align' : [
\ 'left',
\ 'center',
\ 'right',
\ 'justify',
\ 'char'
\ ],
\ 'valign' : [
\ 'top',
\ 'middle',
\ 'bottom',
\ 'baseline'
\ ],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'char' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'caption' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'code' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'base' : [[],
\ {
\ 'href' : []
\ }
\ ],
\ 'br' : [[],
\ {
\ 'style' : [],
\ 'title' : [],
\ 'class' : [],
\ 'id' : []
\ }
\ ],
\ 'acronym' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'strong' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'h4' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'em' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'b' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'q' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : [],
\ 'cite' : []
\ }
\ ],
\ 'span' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'title' : [
\ {
\ 'lang' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ]
\ }
\ ],
\ 'small' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'area' : [[],
\ {
\ 'accesskey' : [],
\ 'coords' : [],
\ 'ondblclick' : [],
\ 'onblur' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onfocus' : [],
\ 'nohref' : [
\ 'BOOL'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'href' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'tabindex' : [],
\ 'alt' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : [],
\ 'shape' : [
\ 'rect',
\ 'circle',
\ 'poly',
\ 'default'
\ ]
\ }
\ ],
\ 'body' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'onunload' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onload' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'ol' : [
\ [
\ 'li'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'html' : [
\ [
\ 'head',
\ 'body'
\ ],
\ {
\ 'xmlns' : [
\ 'http://www.w3.org/1999/xhtml',
\ ],
\ 'lang' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ]
\ }
\ ],
\ 'var' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'ul' : [
\ [
\ 'li'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'del' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'ondblclick' : [],
\ 'datetime' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'cite' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'blockquote' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\  'ltr',
\  'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : [],
\ 'cite' : []
\ }
\ ],
\ 'style' : [[],
\ {
\ 'lang' : [],
\ 'media' : [],
\ 'title' : [],
\ 'type' : [],
\ 'xml:space' : [
\ 'preserve'
\ ],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ]
\ }
\ ],
\ 'dfn' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'h3' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'textarea' : [[], 
\ {
\ 'accesskey' : [],
\ 'disabled' : [
\ 'disabled'
\ ],
\ 'ondblclick' : [],
\ 'rows' : [],
\ 'onblur' : [],
\ 'cols' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onchange' : [],
\ 'onfocus' : [],
\ 'readonly' : [
\ 'BOOL'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onselect' : [],
\ 'onmouseover' : [],
\ 'tabindex' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'name' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'a' : [
\ [
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'accesskey' : [],
\ 'rel' : [],
\ 'coords' : [],
\ 'ondblclick' : [],
\ 'onblur' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onfocus' : [],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'href' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'tabindex' : [],
\ 'lang' : [],
\ 'name' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'charset' : [],
\ 'hreflang' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'rev' : [],
\ 'shape' : [
\ 'rect',
\ 'circle',
\ 'poly',
\ 'default'
\ ],
\ 'type' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'img' : [[],
\ {
\ 'width' : [],
\ 'ismap' : [
\ 'BOOL'
\ ],
\ 'usemap' : [],
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'alt' : [],
\ 'longdesc' : [],
\ 'src' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'height' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'tt' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'thead' : [
\ [
\ 'tr'
\ ],
\ {
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'charoff' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'align' : [
\ 'left',
\ 'center',
\ 'right',
\ 'justify',
\ 'char'
\ ],
\ 'valign' : [
\ 'top',
\ 'middle',
\ 'bottom',
\ 'baseline'
\ ],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'char' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'abbr' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'h6' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'sup' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'address' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'param' : [[],
\ {
\ 'value' : [],
\ 'name' : [],
\ 'type' : [],
\ 'valuetype' : [
\ 'data',
\ 'ref',
\ 'object'
\ ],
\ 'id' : []
\ }
\ ],
\ 'th' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'headers' : [],
\ 'ondblclick' : [],
\ 'axis' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'abbr' : [],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'h1' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'head' : [
\ [
\ 'script',
\ 'style',
\ 'meta',
\ 'link',
\ 'object',
\ 'title',
\ 'script',
\ 'style',
\ 'meta',
\ 'link',
\ 'object',
\ 'base',
\ 'script',
\ 'style',
\ 'meta',
\ 'link',
\ 'object',
\ 'base',
\ 'script',
\ 'style',
\ 'meta',
\ 'link',
\ 'object',
\ 'title',
\ 'script',
\ 'style',
\ 'meta',
\ 'link',
\ 'object'
\ ],
\ {
\ 'profile' : [],
\ 'lang' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ]
\ }
\ ],
\ 'tbody' : [
\ [
\ 'tr'
\ ],
\ {
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'charoff' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'align' : [
\ 'left',
\ 'center',
\ 'right',
\ 'justify',
\ 'char'
\ ],
\ 'valign' : [
\ 'top',
\ 'middle',
\ 'bottom',
\ 'baseline'
\ ],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'char' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'legend' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'accesskey' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'dd' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'hr' : [[],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'li' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'td' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'headers' : [],
\ 'ondblclick' : [],
\ 'axis' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'abbr' : [],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'label' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'for' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'dl' : [
\ [
\ 'dt',
\ 'dd'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'kbd' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'div' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'object' : [
\ [
\ 'param',
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'width' : [],
\ 'usemap' : [],
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'tabindex' : [],
\ 'standby' : [],
\ 'archive' : [],
\ 'lang' : [],
\ 'classid' : [],
\ 'name' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'data' : [],
\ 'height' : [],
\ 'xml:lang' : [],
\ 'codetype' : [],
\ 'declare' : [
\  'BOOL'
\ ],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'type' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : [],
\ 'codebase' : []
\ }
\ ],
\ 'dt' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'pre' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button'
\ ],
\ {
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'xml:space' : [
\ 'preserve'
\ ],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'samp' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'col' : [[],
\ {
\ 'disabled' : [
\ 'disabled'
\ ],
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'value' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'label' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : [],
\ 'selected' : [
\ 'BOOL'
\ ]
\ }
\ ],
\ 'cite' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'i' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'select' : [
\ [
\ 'optgroup',
\ 'option'
\ ],
\ {
\ 'disabled' : [
\ 'BOOL'
\ ],
\ 'ondblclick' : [],
\ 'onblur' : [],
\ 'size' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onchange' : [],
\ 'onfocus' : [],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'tabindex' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'name' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'multiple' : [
\ 'multiple'
\ ],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'link' : [[],
\ {
\ 'rel' : [],
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'media' : [],
\ 'href' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'charset' : [],
\ 'hreflang' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'rev' : [],
\ 'type' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'script' : [[],
\ {
\ 'defer' : [
\ 'BOOL'
\ ],
\ 'src' : [],
\ 'type' : [],
\ 'charset' : [],
\ 'xml:space' : [
\  'preserve'
\  ]
\ }
\ ],
\ 'bdo' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'colgroup' : [
\ [
\ 'col'
\ ],
\ {
\ 'width' : [],
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'charoff' : [],
\ 'onmouseover' : [],
\ 'align' : [
\  'left',
\  'center',
\  'right',
\  'justify',
\  'char'
\ ],
\ 'lang' : [],
\ 'valign' : [
\ 'top',
\ 'middle',
\ 'bottom',
\ 'baseline'
\ ],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'char' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : [],
\ 'span' : [
\ '',
\ '1',
\ ]
\ }
\ ],
\ 'h2' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'ins' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'ondblclick' : [],
\ 'datetime' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'cite' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'p' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'sub' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'big' : [
\ [
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'fieldset' : [
\ [
\ 'legend',
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'a',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'input',
\ 'select',
\ 'textarea',
\ 'label',
\ 'button',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'noscript' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'fieldset',
\ 'table',
\ 'form',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'onmouseout' : [],
\ 'onmousemove' : [],
\ 'style' : [],
\ 'ondblclick' : [],
\ 'xml:lang' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onkeypress' : [],
\ 'onmousedown' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'class' : [],
\ 'title' : [],
\ 'onclick' : []
\ }
\ ],
\ 'button' : [
\ [
\ 'p',
\ 'h1',
\ 'h2',
\ 'h3',
\ 'h4',
\ 'h5',
\ 'h6',
\ 'div',
\ 'ul',
\ 'ol',
\ 'dl',
\ 'pre',
\ 'hr',
\ 'blockquote',
\ 'address',
\ 'table',
\ 'br',
\ 'span',
\ 'bdo',
\ 'object',
\ 'img',
\ 'map',
\ 'tt',
\ 'i',
\ 'b',
\ 'big',
\ 'small',
\ 'em',
\ 'strong',
\ 'dfn',
\ 'code',
\ 'q',
\ 'sub',
\ 'sup',
\ 'samp',
\ 'kbd',
\ 'var',
\ 'cite',
\ 'abbr',
\ 'acronym',
\ 'ins',
\ 'del',
\ 'script',
\ 'noscript'
\ ],
\ {
\ 'accesskey' : [],
\ 'disabled' : [
\ 'disabled'
\ ],
\ 'ondblclick' : [],
\ 'onblur' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onfocus' : [],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'tabindex' : [],
\ 'lang' : [],
\ 'value' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'name' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'type' : [
\ 'button',
\ 'submit',
\ 'reset'
\ ],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ],
\ 'optgroup' : [
\ [
\ 'option'
\ ],
\ {
\ 'disabled' : [
\ 'disabled'
\ ],
\ 'ondblclick' : [],
\ 'dir' : [
\ 'ltr',
\ 'rtl'
\ ],
\ 'onkeydown' : [],
\ 'onkeyup' : [],
\ 'onmouseup' : [],
\ 'id' : [],
\ 'onmouseover' : [],
\ 'lang' : [],
\ 'style' : [],
\ 'onmousemove' : [],
\ 'onmouseout' : [],
\ 'xml:lang' : [],
\ 'onmousedown' : [],
\ 'onkeypress' : [],
\ 'label' : [],
\ 'onclick' : [],
\ 'title' : [],
\ 'class' : []
\ }
\ ]
\ }
endfunction
" vim:set foldmethod=marker:
