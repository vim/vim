let encoding = &enc
if encoding == 'latin1'
    if has("unix")
	let encoding = 'iso-8859-2'
    else
	let encoding = 'cp1250'
    endif
endif

if encoding == 'utf-8'
	source <sfile>:p:h/polish-slash_utf-8.vim
elseif encoding == 'cp1250'
	source <sfile>:p:h/polish-slash_cp1250.vim
elseif encoding == 'iso-8859-2'
	source <sfile>:p:h/polish-slash_iso-8859-2.vim
else
	source <sfile>:p:h/polish-slash_cp852.vim
endif
