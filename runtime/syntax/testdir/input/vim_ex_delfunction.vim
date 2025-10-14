" Vim :delfunction command


delfunction Foo
delfunction <SID>Foo
delfunction foo.bar
delfunction g:foo.bar
delfunction s:foo.bar
delfunction foo#bar
delfunction g:foo#bar
delfunction foo#bar.baz
delfunction g:foo#bar.baz


delfunction! Foo
delfunction! <SID>Foo
delfunction! foo.bar
delfunction! g:foo.bar
delfunction! s:foo.bar
delfunction! foo#bar
delfunction! g:foo#bar
delfunction! foo#bar.baz
delfunction! g:foo#bar.baz


delfunction Foo | echo "Foo"
delfunction Foo " comment


" Issue https://github.com/vim/vim/pull/17420#issuecomment-2927798687
" (arg named /fu%\[nction]/)

silent! delfunc! func

