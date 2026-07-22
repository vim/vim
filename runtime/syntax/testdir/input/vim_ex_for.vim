" Vim :for command


" :for {var} in {object}

let expr = [42]

for foo in expr
  echo foo
endfor

for foo in expr " comment
  echo foo
endfor

for foo in
      "\ comment
      \ expr
  echo foo
endfor

for foo in expr | echo foo | endfor

for foo in [42]
  echo foo
endfor

for foo in [42] | echo foo | endfor

echo "foo" | for foo in expr
  echo foo
endfor


" :for [{var1}, {var2}, ...] in {listlist}

let expr = [[42, 83]]

for [foo, bar] in expr
  echo foo bar
endfor

for [foo, bar] in expr " comment
  echo foo bar
endfor

for [foo, bar] in
      "\ comment
      \ expr
  echo foo bar
endfor

for [foo, bar] in expr | echo foo bar | endfor

for [foo, bar] in [[42, 83]]
  echo foo bar
endfor

for [foo, bar] in [[42, 83]] | echo foo bar | endfor

