vim9script
# Vim9 :def command (nested)

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
