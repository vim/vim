vim9script
# Vim9 :enum command
# VIM_TEST_SETUP hi link vim9EnumValue Todo
# See: https://github.com/vim/vim/pull/16365#issuecomment-2571420551


enum Letter
    #
    #########################################
    A("(\" # not a comment NOR_ANOTHER_VALUE,
        \ "), B(")\""), C($"'')
        \('"), D($'""(),"'), E,
    F(
    ")" .. # FA,
    "(" # FB,
    ), G
    #enum NotSupported
    #endenum

    def new(this.value = v:none)
        return
    enddef
    const value: string
endenum

for letter in Letter.values
    echo letter
endfor

echo Letter.D

