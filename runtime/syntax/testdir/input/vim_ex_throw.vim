" Vim :throw command

" :help :throw

try | throw "oops" | catch /^oo/ | echo "caught" | endtry
