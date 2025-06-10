" Vim function variable highlighting

function Foo()
  " :let

  let foo = expr

  let foo[0] = expr

  let foo[1:2] = expr
  let foo[:2] = expr
  let foo[1:] = expr
  let foo[:] = expr

  let foo["key"] = expr
  let foo['key'] = expr

  let foo += expr
  let foo -= expr
  let foo *= expr
  let foo /= expr
  let foo %= expr
  let foo .= expr
  let foo ..= expr

  let b:foo = expr

  let b:foo[0] = expr

  let b:foo[1:2] = expr
  let b:foo[:2] = expr
  let b:foo[1:] = expr
  let b:foo[:] = expr

  let b:foo["key"] = expr
  let b:foo['key'] = expr

  let b:foo += expr
  let b:foo -= expr
  let b:foo *= expr
  let b:foo /= expr
  let b:foo %= expr
  let b:foo .= expr
  let b:foo ..= expr

  let g:foo = expr

  let g:foo[0] = expr

  let g:foo[1:2] = expr
  let g:foo[:2] = expr
  let g:foo[1:] = expr
  let g:foo[:] = expr

  let g:foo["key"] = expr
  let g:foo['key'] = expr

  let g:foo += expr
  let g:foo -= expr
  let g:foo *= expr
  let g:foo /= expr
  let g:foo %= expr
  let g:foo .= expr
  let g:foo ..= expr

  let s:foo = expr

  let s:foo[0] = expr

  let s:foo[1:2] = expr
  let s:foo[:2] = expr
  let s:foo[1:] = expr
  let s:foo[:] = expr

  let s:foo["key"] = expr
  let s:foo['key'] = expr

  let s:foo += expr
  let s:foo -= expr
  let s:foo *= expr
  let s:foo /= expr
  let s:foo %= expr
  let s:foo .= expr
  let s:foo ..= expr

  let t:foo = expr

  let t:foo[0] = expr

  let t:foo[1:2] = expr
  let t:foo[:2] = expr
  let t:foo[1:] = expr
  let t:foo[:] = expr

  let t:foo["key"] = expr
  let t:foo['key'] = expr

  let t:foo += expr
  let t:foo -= expr
  let t:foo *= expr
  let t:foo /= expr
  let t:foo %= expr
  let t:foo .= expr
  let t:foo ..= expr

  let v:true = expr

  let v:true[0] = expr

  let v:true[1:2] = expr
  let v:true[:2] = expr
  let v:true[1:] = expr
  let v:true[:] = expr

  let v:true["key"] = expr
  let v:true['key'] = expr

  let v:true += expr
  let v:true -= expr
  let v:true *= expr
  let v:true /= expr
  let v:true %= expr
  let v:true .= expr
  let v:true ..= expr

  let w:foo = expr

  let w:foo[0] = expr

  let w:foo[1:2] = expr
  let w:foo[:2] = expr
  let w:foo[1:] = expr
  let w:foo[:] = expr

  let w:foo["key"] = expr
  let w:foo['key'] = expr

  let w:foo += expr
  let w:foo -= expr
  let w:foo *= expr
  let w:foo /= expr
  let w:foo %= expr
  let w:foo .= expr
  let w:foo ..= expr

  let $FOO = expr
  let $FOO .= expr
  let $FOO ..= expr

  let @f = expr
  let @f .= expr
  let @f ..= expr

  let &ari = expr

  let &t_k1 = "\<Esc>[234;"

  let &ari .= expr
  let &ari ..= expr
  let &ari += expr
  let &ari -= expr

  let &l:aleph = expr

  let &l:aleph .= expr
  let &l:aleph ..= expr
  let &l:aleph += expr
  let &l:aleph -= expr

  let &g:aleph = expr

  let &g:aleph .= expr
  let &g:aleph ..= expr
  let &g:aleph += expr
  let &g:aleph -= expr

  let [foo, bar] = expr
  let [foo,
	\ bar] = expr
  let [v:true, v:false] = expr
  let [v:true,
	\ v:false] = expr
  let [&ari, &bkc] = expr
  let [&ari,
	\  &bkc] = expr
  let [$foo, $bar] = expr
  let [$foo,
	\  $bar] = expr
  let [@a, @b] = expr
  let [@a,
	\  @b] = expr

  let [foo, bar] .= expr
  let [foo, bar] ..= expr
  let [foo, bar] += expr
  let [foo, bar] -= expr

  let [foo, bar; baz] = expr
  let [foo,
	\ bar;
	\ baz] = expr
  let [v:true, v:false; v:none] = expr
  let [v:true,
	\ v:false;
	\ v:none] = expr
  let [$foo, $bar; $baz] = expr
  let [$foo,
	\ $bar;
	\ $baz] = expr
  let [&ari, &bkc; &cmp] = expr
  let [&ari,
	\ &bkc;
	\ &cmp] = expr
  let [@a, @b; @c] = expr
  let [@a,
	\ @b;
	\ @c] = expr

  let foo =<< END
...
END
  let foo =<< trim END
  ...
  END
  let foo =<< eval END
...
END
  let foo =<< trim eval END
  ...
  END
  let foo =<< eval trim END
    ...
  END

  " :let list values

  let foo
  let foo bar
  let foo
	"\ comment
	\ bar

  let foo " comment
  let foo "\ comment
  let foo | echo "Foo"
  let foo bar " comment
  let foo bar | echo "Foo"
  let foo bar "\ comment

  " :unlet

  unlet foo
  unlet foo bar
  unlet foo
	"\ comment
	\ bar

  unlet! foo
  unlet! foo bar
  unlet! foo
	"\ comment
	\ bar

  unlet $FOO
  unlet! $FOO

  unlet list[3]
  unlet list[3:]
  unlet dict['two']
  unlet dict.two

  unlet foo " comment
  unlet foo "\ comment
  unlet foo | echo "Foo"
  unlet foo bar " comment
  unlet foo bar "\ comment
  unlet foo bar | echo "Foo"

  " :const

  const foo = expr

  const [foo, bar] = expr

  const [foo, bar; baz] = expr

  const foo =<< END
...
END
  const foo =<< trim END
  ...
  END
  const foo =<< eval END
...
END
  const foo =<< trim eval END
  ...
  END
  const foo =<< eval trim END
    ...
  END

  const foo
  const foo bar

  " :for

  for foo in expr
  endfor

  for [foo, bar] in expr
  endfor

  " :lockvar

  lockvar foo
  lockvar foo bar
  lockvar foo
	"\ comment
	\ bar

  lockvar foo " comment
  lockvar foo | echo "Foo"
  lockvar foo bar " comment
  lockvar foo bar | echo "Foo"

  lockvar! foo
  lockvar! foo bar
  lockvar! foo
	"\ comment
	\ bar

  lockvar! foo " comment
  lockvar! foo | echo "Foo"
  lockvar! foo bar " comment
  lockvar! foo bar | echo "Foo"

  lockvar 2 foo
  lockvar 2 foo bar
  lockvar 2 foo
	"\ comment
	\ bar

  lockvar 2 foo " comment
  lockvar 2 foo | echo "Foo"
  lockvar 2 foo bar " comment
  lockvar 2 foo bar | echo "Foo"

  " :unlockvar

  unlockvar foo
  unlockvar foo bar
  unlockvar foo
	"\ comment
	\ bar

  unlockvar foo " comment
  unlockvar foo | echo "Foo"
  unlockvar foo bar " comment
  unlockvar foo bar | echo "Foo"

  unlockvar! foo
  unlockvar! foo bar
  unlockvar! foo
	"\ comment
	\ bar

  unlockvar! foo " comment
  unlockvar! foo | echo "Foo"
  unlockvar! foo bar " comment
  unlockvar! foo bar | echo "Foo"

  unlockvar 2 foo
  unlockvar 2 foo bar
  unlockvar 2 foo
	"\ comment
	\ bar

  unlockvar 2 foo " comment
  unlockvar 2 foo | echo "Foo"
  unlockvar 2 foo bar " comment
  unlockvar 2 foo bar | echo "Foo"

" Scope dictionaries

echo get(b:, 'foo', 42)
echo get(w:, 'foo', 42)
echo get(t:, 'foo', 42)
echo get(g:, 'foo', 42)
echo get(l:, 'foo', 42)
echo get(s:, 'foo', 42)
echo get(a:, 'foo', 42)
echo get(v:, 'foo', 42)

for k in keys(b:) | echo b:[k] | endfor
for k in keys(w:) | echo w:[k] | endfor
for k in keys(t:) | echo t:[k] | endfor
for k in keys(g:) | echo g:[k] | endfor
for k in keys(l:) | echo l:[k] | endfor
for k in keys(s:) | echo s:[k] | endfor
for k in keys(a:) | echo a:[k] | endfor
for k in keys(v:) | echo v:[k] | endfor

" Neovim-specific variables (not highlighted by default)

echo v:lua v:msgpack_types v:relnum v:stderr v:termrequest v:virtnum

echo &channel &inccommand &mousescroll &pumblend &redrawdebug &scrollback
echo &shada &shadafile &statuscolumn &termpastefilter &termsync &winbar
echo &winblend &winhighlight

endfunction

