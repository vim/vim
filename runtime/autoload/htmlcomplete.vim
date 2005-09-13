" Vim completion script
" Language:	XHTML 1.0 Strict
" Maintainer:	Mikolaj Machowski ( mikmach AT wp DOT pl )
" Last Change:	2005 Sep 13

function! htmlcomplete#CompleteTags(findstart, base)
  if a:findstart
    " locate the start of the word
    let line = getline('.')
    let start = col('.') - 1
    while start >= 0 && line[start - 1] !~ '<'
      let start -= 1
    endwhile
	let g:st = start
    return start
  else
	" Set attribute groups
    let g:coreattrs = ["id", "class", "style", "title"] 
    let g:i18n = ["lang", "xml:lang", "dir"]
    let g:events = ["onclick", "ondblclick", "onmousedown", "onmouseup", "onmousemove",
    			\ "onmouseout", "onkeypress", "onkeydown", "onkeyup"]
    let g:focus = ["accesskey", "tabindex", "onfocus", "onblur"]
    let g:coregroup = g:coreattrs
    let g:coregroup = extend(g:coregroup, g:i18n)
    let g:coregroup = extend(g:coregroup, g:events)
    " find tags matching with "a:base"
    let res = []
	" If a:base contains > it means we are already outside of tag and we
	" should abandon action
	if a:base =~ '>'
		return []
	endif
	" If a:base contains white space it is attribute. 
	" It could be also value of attribute...
	" Possible situations where any prediction would be difficult:
	" 1. Events attributes
	if a:base =~ '\s'
		" Sort out style, class, and on* cases
		" Perfect solution for style would be switching for CSS completion. Is
		" it possible?
		" Also retrieving class names from current file and linked
		" stylesheets.
		if a:base =~ "\\(on[a-z]*\\|style\\|class\\)\\s*=\\s*[\"']"
			let stripbase = matchstr(a:base, ".*\\(on[a-z]*\\|style\\|class\\)\\s*=\\s*[\"']\\zs.*")
			" Now we have a:base stripped from all chars up to style/class.
			" It may fail with some strange style value combinations.
			if stripbase !~ "[\"']"
				return []
			endif
		endif
		" We have to get first word to offer
		" proper attributes.
		let tag = split(a:base)[0]
		" Get last word, it should be attr name
		let attr = matchstr(a:base, '.*\s\zs.*')
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
				if a:base =~ '^a\>'
					let values = ["rect"]
				else
					let values = ["rect", "circle", "poly", "default"]
				endif
			elseif attrname == 'valuetype'
				let values = ["data", "ref", "object"]
			elseif attrname == 'method'
				let values = ["get", "post"]
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
				if a:base =~ '^input'
					let values = ["input-text", "password", "checkbox", "radio", "submit", "reset", "input-file", "hidden", "input-image", "input-button"]
				elseif a:base =~ '^button'
					let values = ["button", "submit", "reset"]
				endif
			else
				return []
			endif

			if len(values) == 0
				return []
			endif

			" We need special version of sbase
			let attrbase = matchstr(a:base, ".*[\"']")

			for m in values
				if m =~ '^' . entered_value
					call add(res, attrbase . m . '" ')
				endif
			endfor
		endif
		" Shorten a:base to not include last word
		let sbase = matchstr(a:base, '.*\ze\s.*')
		if tag =~ '^\(abbr\|acronym\|b\|bdo\|big\|caption\|cite\|code\|dd\|dfn\|div\|dl\|dt\|em\|fieldset\|h\d\|kbd\|li\|noscript\|ol\|p\|samp\|small\|span\|strong\|sub\|sup\|tt\|ul\|var\)$'
			let attrs = g:coregroup
		elseif tag == 'a'
			let tagspec = ["charset", "type", "name", "href", "hreflang", "rel", "rev", "shape", "coords"]
			let attrs = extend(tagspec, g:coregroup)
			let attrs = extend(attrs, g:focus)
		elseif tag == 'area'
			let attrs = g:coregroup
		elseif tag == 'base'
			let attrs = ["href", "id"]
		elseif tag == 'blockquote'
			let attrs = g:coregroup
			let attrs = extend(attrs, ["cite"])
		elseif tag == 'body'
			let attrs = g:coregroup
			let attrs = extend(attrs, ["onload", "onunload"])
		elseif tag == 'br'
			let attrs = g:coreattrs
		elseif tag == 'button'
			let attrs = g:coreattrs
			let attrs = extend(attrs, g:focus)
			let attrs = extend(attrs, ["name", "value", "type"])
		elseif tag == '^\(col\|colgroup\)$'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["span", "width", "align", "char", "charoff", "valign"])
		elseif tag =~ '^\(del\|ins\)$'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["cite", "datetime"])
		elseif tag == 'form'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["action", "method", "enctype", "onsubmit", "onreset", "accept", "accept-charset"])
		elseif tag == 'head'
			let attrs = g:i18n
			let attrs = extend(attrs, ["id", "profile"])
		elseif tag == 'html'
			let attrs = g:i18n
			let attrs = extend(attrs, ["id", "xmlns"])
		elseif tag == 'img'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["src", "alt", "longdesc", "height", "width", "usemap", "ismap"])
		elseif tag == 'input'
			let attrs = g:coreattrs
			let attrs = extend(attrs, g:focus)
			let attrs = extend(attrs, ["type", "name", "value", "checked", "disabled", "readonly", "size", "maxlength", "src", "alt", "usemap", "onselect", "onchange", "accept"])
		elseif tag == 'label'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["for", "accesskey", "onfocus", "onblur"])
		elseif tag == 'legend'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["accesskey"])
		elseif tag == 'link'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["charset", "href", "hreflang", "type", "rel", "rev", "media"])
		elseif tag == 'map'
			let attrs = g:i18n
			let attrs = extend(attrs, g:events)
			let attrs = extend(attrs, ["id", "class", "style", "title", "name"])
		elseif tag == 'meta'
			let attrs = g:i18n
			let attrs = extend(attrs, ["id", "http-equiv", "content", "scheme", "name"])
		elseif tag == 'title'
			let attrs = g:i18n
			let attrs = extend(attrs, ["id"])
		elseif tag == 'object'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["declare", "classid", "codebase", "data", "type", "codetype", "archive", "standby", "height", "width", "usemap", "name", "tabindex"])
		elseif tag == 'optgroup'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["disbled", "label"])
		elseif tag == 'option'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["disbled", "selected", "value", "label"])
		elseif tag == 'param'
			let attrs = ["id", "name", "value", "valuetype", "type"]
		elseif tag == 'pre'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["xml:space"])
		elseif tag == 'q'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["cite"])
		elseif tag == 'script'
			let attrs = ["id", "charset", "type", "src", "defer", "xml:space"]
		elseif tag == 'select'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["name", "size", "multiple", "disabled", "tabindex", "onfocus", "onblur", "onchange"])
		elseif tag == 'style'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["id", "type", "media", "title", "xml:space"])
		elseif tag == 'table'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["summary", "width", "border", "frame", "rules" "cellspacing", "cellpadding"])
		elseif tag =~ '^\(thead\|tfoot\|tbody\|tr\)$'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["align", "char", "charoff", "valign"])
		elseif tag == 'textarea'
			let attrs = g:coreattrs
			let attrs = extend(attrs, g:focus)
			let attrs = extend(attrs, ["name", "rows", "cols", "disabled", "readonly", "onselect", "onchange"])
		elseif tag =~ '^\(th\|td\)$'
			let attrs = g:coreattrs
			let attrs = extend(attrs, ["abbr", "headers", "scope", "rowspan", "colspan", "align", "char", "charoff", "valign"])
		endif

		for m in sort(attrs)
			if m =~ '^' . attr
				if m =~ '^\(ismap\|defer\|declare\|nohref\|checked\|disabled\|selected\|readonly\)$'
					call add(res, sbase.' '.m)
				else
					call add(res, sbase.' '.m.'="')
				endif
			endif
		endfor
		return res
	endif
    for m in split("a abbr acronym address area b base bdo big blockquote body br button caption cite code col colgroup dd del dfn div dl dt em fieldset form head h1 h2 h3 h4 h5 h6 hr html i img input ins kbd label legend li link map meta noscript object ol optgroup option p param pre q samp script select small span strong style sub sup table tbody td textarea tfoot th thead title tr tt ul var")
		if m =~ '^' . a:base
			call add(res, m)
		endif
    endfor
    return res
  endif
endfunction
