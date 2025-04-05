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

