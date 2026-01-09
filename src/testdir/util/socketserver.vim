" Check if only the socketserver backend is available for clientserver (only on
" Unix), and set g:socketserver_only to v:true along with starting the
" socketserver.
command TrySocketServer call TrySocketServer()
func TrySocketServer()
  if has("socketserver") && !has("x11")
    let g:socketserver_only = v:true

    if v:servername == ""
      call remote_startserver('VIMSOCKETSERVERTEST')
    endif
  else
      let g:socketserver_only = v:false
    endif
endfunc

" vim: shiftwidth=2 sts=2 expandtab
