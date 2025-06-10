vim9script

# Vim9 function variable highlighting

def Foo()
  # Declarations

  var foo = expr

  b:foo = expr
  g:foo = expr
  t:foo = expr
  w:foo = expr

  v:true = expr

  $FOO = expr

  var [foo, bar] = expr
  var [foo,
	\ bar] = expr
  var [$foo, $bar] = expr
  var [$foo,
	\ $bar] = expr

  var [foo, bar; baz] = expr
  var [foo,
	\ bar;
	\ baz] = expr
  var [$foo, $bar; $baz] = expr
  var [$foo,
	\ $bar;
	\ $baz] = expr

  var foo =<< END
...
END
  var foo =<< trim END
  ...
  END
  var foo =<< eval END
...
END
  var foo =<< trim eval END
  ...
  END
  var foo =<< eval trim END
    ...
  END

  # Assignments

  foo = expr

  foo[0] = expr

  foo[1:2] = expr
  foo[:2] = expr
  foo[1:] = expr
  foo[:] = expr

  foo["key"] = expr
  foo['key'] = expr

  foo += expr
  foo -= expr
  foo *= expr
  foo /= expr
  foo %= expr
  foo ..= expr

  b:foo = expr
  g:foo = expr
  t:foo = expr
  w:foo = expr

  b:foo += expr
  g:foo += expr
  t:foo += expr
  w:foo += expr

  b:foo -= expr
  g:foo -= expr
  t:foo -= expr
  w:foo -= expr

  b:foo *= expr
  g:foo *= expr
  t:foo *= expr
  w:foo *= expr

  b:foo /= expr
  g:foo /= expr
  t:foo /= expr
  w:foo /= expr

  b:foo %= expr
  g:foo %= expr
  t:foo %= expr
  w:foo %= expr

  b:foo ..= expr
  g:foo ..= expr
  t:foo ..= expr
  w:foo ..= expr

  $FOO = expr
  $FOO ..= expr

  @f = expr
  @f ..= expr

  &ari = expr

  &t_k1 = "\<Esc>[234;"

  &ari ..= expr

  &ari += expr
  &ari -= expr

  &l:aleph = expr

  &l:aleph ..= expr
  &l:aleph += expr
  &l:aleph -= expr

  &g:aleph = expr

  &g:aleph ..= expr
  &g:aleph += expr
  &g:aleph -= expr

  [foo, bar] = expr
  [foo,
	\ bar] = expr
  [v:true, v:false] = expr
  [v:true,
	\ v:false] = expr
  [&ari, &bkc] = expr
  [&ari,
	\ &bkc] = expr
  [$foo, $bar] = expr
  [$foo,
	\  $bar] = expr
  [@a, @b] = expr
  [@a,
	\  @a] = expr

  [foo, bar] ..= expr
  [foo, bar] += expr
  [foo, bar] -= expr
  [foo, bar] *= expr
  [foo, bar] /= expr
  [foo, bar] %= expr

  [foo, bar; baz] = expr
  [foo,
	\ bar;
	\ baz] = expr
  [v:true, v:false; v:none] = expr
  [v:true,
	\ v:false;
	\ v:none] = expr
  [$foo, $bar; $baz] = expr
  [$foo,
	\ $bar;
	\ $baz] = expr
  [&ari, &bkc; &cmp] = expr
  [&ari,
	\ &bkc;
	\ &cmp] = expr
  [@a, @b; @c] = expr
  [@a,
	\ @b;
	\ @c] = expr

  foo =<< END
...
END
  foo =<< trim END
  ...
  END
  foo =<< eval END
...
END
  foo =<< trim eval END
  ...
  END
  foo =<< eval trim END
    ...
  END

  # :for

  for foo in expr
  endfor

  for [foo, bar] in expr
  endfor

# Scope dictionaries

echo get(b:, 'foo', 42)
echo get(w:, 'foo', 42)
echo get(t:, 'foo', 42)
echo get(g:, 'foo', 42)
echo get(v:, 'foo', 42)

for k in keys(b:) | echo b:[k] | endfor
for k in keys(w:) | echo w:[k] | endfor
for k in keys(t:) | echo t:[k] | endfor
for k in keys(g:) | echo g:[k] | endfor
for k in keys(v:) | echo v:[k] | endfor

# Neovim-specific variables (not highlighted by default)

echo v:lua v:msgpack_types v:relnum v:stderr v:termrequest v:virtnum

echo &channel &inccommand &mousescroll &pumblend &redrawdebug &scrollback
echo &shada &shadafile &statuscolumn &termpastefilter &termsync &winbar
echo &winblend &winhighlight

enddef

