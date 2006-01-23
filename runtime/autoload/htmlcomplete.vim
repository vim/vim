" Vim completion script
" Language:	XHTML 1.0 Strict
" Maintainer:	Mikolaj Machowski ( mikmach AT wp DOT pl )
" Last Change:	2006 Jan 22

function! htmlcomplete#CompleteTags(findstart, base)
  if a:findstart
    " locate the start of the word
    let line = getline('.')
    let start = col('.') - 1
	let compl_begin = col('.') - 2
    while start >= 0 && line[start - 1] =~ '\(\k\|[:.-]\)'
		let start -= 1
    endwhile
	if start >= 0 && line[start - 1] =~ '&'
		let b:entitiescompl = 1
		let b:compl_context = ''
		return start
	endif
	let stylestart = searchpair('<style\>', '', '<\/style\>', "bnW")
	let styleend   = searchpair('<style\>', '', '<\/style\>', "nW")
	if stylestart != 0 && styleend != 0 
		let curpos = line('.')
		if stylestart <= curpos && styleend >= curpos
			let start = col('.') - 1
			let b:csscompl = 1
			while start >= 0 && line[start - 1] =~ '\(\k\|-\)'
				let start -= 1
			endwhile
		endif
	endif
	if !exists("b:csscompl")
		let b:compl_context = getline('.')[0:(compl_begin)]
		let b:compl_context = matchstr(b:compl_context, '.*\zs<.*')
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
	if exists("b:csscompl")
		unlet! b:csscompl
		let context = b:compl_context
		return csscomplete#CompleteCSS(0, context)
	else
		if len(b:compl_context) == 0 && !exists("b:entitiescompl")
			return []
		endif
		let context = matchstr(b:compl_context, '.\zs.*')
	endif
	unlet! b:compl_context
	" Make entities completion
	if exists("b:entitiescompl")
		unlet! b:entitiescompl

		if !exists("g:xmldata_xhtml10s")
			runtime! autoload/xml/xhtml10s.vim
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
	if context =~ '>'
		" Generally if context contains > it means we are outside of tag and
		" should abandon action - with one exception: <style> span { bo
		if context =~ 'style[^>]\{-}>[^<]\{-}$'
			return csscomplete#CompleteCSS(0, context)
		else
			return []
		endif
	endif
	"if context !~ '<$'

	" Set attribute groups
    let coreattrs = ["id", "class", "style", "title"] 
    let i18n = ["lang", "xml:lang", "dir=\"ltr\" ", "dir=\"rtl\" "]
    let events = ["onclick", "ondblclick", "onmousedown", "onmouseup", "onmousemove",
    			\ "onmouseover", "onmouseout", "onkeypress", "onkeydown", "onkeyup"]
    let focus = ["accesskey", "tabindex", "onfocus", "onblur"]
    let coregroup = coreattrs + i18n + events
    " find tags matching with "context"
	" If context contains > it means we are already outside of tag and we
	" should abandon action
	" If context contains white space it is attribute. 
	" It could be also value of attribute...
	" We have to get first word to offer
	" proper completions
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
		if context =~ "\\(on[a-z]*\\|id\\|style\\|class\\)\\s*=\\s*[\"']"
			if context =~ "\\(id\\|class\\)\\s*=\\s*[\"'][a-zA-Z0-9_ -]*$"
				if context =~ "class\\s*=\\s*[\"'][a-zA-Z0-9_ -]*$"
					let search_for = "class"
				elseif context =~ "id\\s*=\\s*[\"'][a-zA-Z0-9_ -]*$"
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

			elseif context =~ "style\\s*=\\s*[\"'][^\"']*$"
				return csscomplete#CompleteCSS(0, context)

			endif
			let stripbase = matchstr(context, ".*\\(on[a-z]*\\|style\\|class\\)\\s*=\\s*[\"']\\zs.*")
			" Now we have context stripped from all chars up to style/class.
			" It may fail with some strange style value combinations.
			if stripbase !~ "[\"']"
				return []
			endif
		endif
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
		" Shorten context to not include last word
		let sbase = matchstr(context, '.*\ze\s.*')
		if tag =~ '^\(abbr\|acronym\|address\|b\|bdo\|big\|caption\|cite\|code\|dd\|dfn\|div\|dl\|dt\|em\|fieldset\|h\d\|hr\|i\|kbd\|li\|noscript\|ol\|p\|samp\|small\|span\|strong\|sub\|sup\|tt\|ul\|var\)$'
			let attrs = coregroup
		elseif tag == 'a'
			let attrs = coregroup + focus + ["charset", "type", "name", "href", "hreflang", "rel", "rev", "shape", "coords"]
		elseif tag == 'area'
			let attrs = coregroup + focus + ["shape", "coords", "href", "nohref", "alt"]
		elseif tag == 'base'
			let attrs = ["href", "id"]
		elseif tag == 'blockquote'
			let attrs = coregroup + ["cite"]
		elseif tag == 'body'
			let attrs = coregroup + ["onload", "onunload"]
		elseif tag == 'br'
			let attrs = coreattrs
		elseif tag == 'button'
			let attrs = coregroup + focus + ["name", "value", "type"]
		elseif tag == '^\(col\|colgroup\)$'
			let attrs = coregroup + ["span", "width", "align", "char", "charoff", "valign"]
		elseif tag =~ '^\(del\|ins\)$'
			let attrs = coregroup + ["cite", "datetime"]
		elseif tag == 'form'
			let attrs = coregroup + ["action", "method=\"get\" ", "method=\"post\" ", "enctype", "onsubmit", "onreset", "accept", "accept-charset"]
		elseif tag == 'head'
			let attrs = i18n + ["id", "profile"]
		elseif tag == 'html'
			let attrs = i18n + ["id", "xmlns"]
		elseif tag == 'img'
			let attrs = coregroup + ["src", "alt", "longdesc", "height", "width", "usemap", "ismap"]
		elseif tag == 'input'
			let attrs = coregroup + ["type", "name", "value", "checked", "disabled", "readonly", "size", "maxlength", "src", "alt", "usemap", "onselect", "onchange", "accept"]
		elseif tag == 'label'
			let attrs = coregroup + ["for", "accesskey", "onfocus", "onblur"]
		elseif tag == 'legend'
			let attrs = coregroup + ["accesskey"]
		elseif tag == 'link'
			let attrs = coregroup + ["charset", "href", "hreflang", "type", "rel", "rev", "media"]
		elseif tag == 'map'
			let attrs = i18n + events + ["id", "class", "style", "title", "name"]
		elseif tag == 'meta'
			let attrs = i18n + ["id", "http-equiv", "content", "scheme", "name"]
		elseif tag == 'title'
			let attrs = i18n + ["id"]
		elseif tag == 'object'
			let attrs = coregroup + ["declare", "classid", "codebase", "data", "type", "codetype", "archive", "standby", "height", "width", "usemap", "name", "tabindex"]
		elseif tag == 'optgroup'
			let attrs = coregroup + ["disbled", "label"]
		elseif tag == 'option'
			let attrs = coregroup + ["disbled", "selected", "value", "label"]
		elseif tag == 'param'
			let attrs = ["id", "name", "value", "valuetype", "type"]
		elseif tag == 'pre'
			let attrs = coregroup + ["xml:space"]
		elseif tag == 'q'
			let attrs = coregroup + ["cite"]
		elseif tag == 'script'
			let attrs = ["id", "charset", "type=\"text/javascript\"", "type", "src", "defer", "xml:space"]
		elseif tag == 'select'
			let attrs = coregroup + ["name", "size", "multiple", "disabled", "tabindex", "onfocus", "onblur", "onchange"]
		elseif tag == 'style'
			let attrs = coreattrs + ["id", "type=\"text/css\"", "type", "media", "title", "xml:space"]
		elseif tag == 'table'
			let attrs = coregroup + ["summary", "width", "border", "frame", "rules", "cellspacing", "cellpadding"]
		elseif tag =~ '^\(thead\|tfoot\|tbody\|tr\)$'
			let attrs = coregroup + ["align", "char", "charoff", "valign"]
		elseif tag == 'textarea'
			let attrs = coregroup + ["name", "rows", "cols", "disabled", "readonly", "onselect", "onchange"]
		elseif tag =~ '^\(th\|td\)$'
			let attrs = coregroup + ["abbr", "headers", "scope", "rowspan", "colspan", "align", "char", "charoff", "valign"]
		else
			return []
		endif

		for m in sort(attrs)
			if m =~ '^'.attr
				if m =~ '^\(ismap\|defer\|declare\|nohref\|checked\|disabled\|selected\|readonly\)$' || m =~ '='
					call add(res, m)
				else
					call add(res, m.'="')
				endif
			elseif m =~ attr
				if m =~ '^\(ismap\|defer\|declare\|nohref\|checked\|disabled\|selected\|readonly\)$' || m =~ '='
					call add(res2, m)
				else
					call add(res2, m.'="')
				endif
			endif
		endfor

		return res + res2

	endif
	" Close tag
	let b:unaryTagsStack = "base meta link hr br param img area input col"
	if context =~ '^\/'
		let opentag = xmlcomplete#GetLastOpenTag("b:unaryTagsStack")
		return [opentag.">"]
	endif
	" Deal with tag completion.
	let opentag = xmlcomplete#GetLastOpenTag("b:unaryTagsStack")
	if opentag == ''
		" Hack for sometimes failing GetLastOpenTag.
		" As far as I tested fail isn't GLOT fault but problem
		" of invalid document - not properly closed tags and other mish-mash.
		" If returns empty string assume <body>. Safe bet.
		let opentag = 'body'
	endif

	if !exists("g:xmldata_xhtml10s")
		runtime! autoload/xml/xhtml10s.vim
	endif

	let tags = g:xmldata_xhtml10s[opentag][0]

	for m in sort(tags)
		if m =~ '^'.context
			call add(res, m)
		elseif m =~ context
			call add(res2, m)
		endif
	endfor

	return res + res2

  endif
endfunction
