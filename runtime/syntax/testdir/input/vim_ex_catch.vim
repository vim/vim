" Vim :catch command

" :help :catch

catch /^Vim:Interrupt$/		" catch interrupts (CTRL-C)
catch /^Vim\%((\a\+)\)\=:E/	" catch all Vim errors
catch /^Vim\%((\a\+)\)\=:/	" catch errors and interrupts
catch /^Vim(write):/		" catch all errors in :write
catch /^Vim\%((\a\+)\)\=:E123:/	" catch error E123
catch /my-exception/		" catch user exception
catch /.*/			" catch everything
catch				" same as /.*/

" :help :try

try | sleep 100 | catch /^Vim:Interrupt$/ | endtry
try | edit | catch /^Vim(edit):E\d\+/ | echo "error" | endtry
