" Vim :redir command


redir  > filename
redir! > filename
redir  > file name
redir! > file name

redir  >> filename
redir! >> filename
redir  >> file name
redir! >> file name

redir > filename  " comment
redir > filename  | echo "Foo"
redir > file name " comment
redir > file name | echo "Foo"

redir >> filename  " comment
redir >> filename  | echo "Foo"
redir >> file name " comment
redir >> file name | echo "Foo"

redir @a
redir @A

redir @a>
redir @A>

redir @a>>
redir @A>>

redir @*>
redir @+>

redir @*>>
redir @+>>

redir @">
redir @">>

redir =>  var
redir =>> var

redir END


" Issue #18319 (redir command not highlighted in some cases)

def Vim9Context()

var foo: string
if true
    redir => foo
    smile
    redir END
endif
echo foo

command Foo {
    redir => foo
    echo "hello from cmd"
    redir END
}

redir => foo
echom "hello global"
redir END

enddef

