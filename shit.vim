function! ShowHexLineNumbers()
    let l:bufnr = bufnr('%')
    sign unplace *
    for l:lnum in range(1, line('$'))
        let l:hex = printf('%X', l:lnum)
        execute 'sign place '.l:lnum.' line='.l:lnum.' name=HexLineNum buffer='.l:bufnr
        " You'd need to define the HexLineNum sign first
    endfor
endfunction

