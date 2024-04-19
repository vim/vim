" Vim :def and :function commands (nested)
" VIM_TEST_SETUP let g:vimsyn_folding = 'f'
" VIM_TEST_SETUP setl fdc=2 fdm=syntax

def FunA(): string
    def DoFunA(): string
	return "."
    enddef

    return DoFunA()
enddef

def FunB(): string
    function DoFunB()
	return ".."
    endfunction

    return DoFunB()
enddef

function FunC()
    def DoFunC(): string
	return "..."
    enddef

    return DoFunC()
endfunction

function FunD()
    function DoFunD()
	return "...."
    endfunction

    return DoFunD()
endfunction

echo FunA()
echo FunB()
echo FunC()
echo FunD()
