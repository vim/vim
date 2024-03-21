if has('conceal')
	let g:markdown_syntax_conceal = 1
	setlocal conceallevel=3 concealcursor=n
endif

let g:markdown_fenced_languages = ['html']
highlight link markdownBold Todo
highlight link htmlBold Todo
