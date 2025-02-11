vim9script

# Vim9 :def command (nested)
# VIM_TEST_SETUP let g:vimsyn_folding = 'f'
# VIM_TEST_SETUP setl fdc=2 fdm=syntax
# VIM_TEST_SETUP hi link vim9This Todo

class Test
    const name: string

    def new()
	def Name(): string
	    function GiveName()
		return "any"
	    endfunction

	    return GiveName()
	enddef

	this.name = Name()
    enddef
endclass

echo Test.new()
