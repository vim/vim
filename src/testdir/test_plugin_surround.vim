" Test for the surround package

CheckRunVimInTerminal


func Test_basic_surround_add()
  let lines =<< trim END
    Lorem ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat
    fermentum pretium.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec
    lectus ligula.  Proin elementum luctus elit, a tincidunt quam facilisis non.
    Nunc quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis
    quis accumsan in, feugiat quis sem.
    one 보two 여보세요 дважды два четыре
    three세 four 여보세요 это всем известно
    five 요 six 여보세요 여보세요 в целом мире
  END

  let input_file = "test_basic_surround_add_input"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
  call term_sendkeys(buf, "ysiw'")
  call term_sendkeys(buf, "j0yst.#")
  call term_sendkeys(buf, 'fCysis!')
  call term_sendkeys(buf, 'j0ysiwb')
  call term_sendkeys(buf, 'fiysiw(')
  call term_sendkeys(buf, 'j.3Wys$*')
  call term_sendkeys(buf, 'j.')
  call term_sendkeys(buf, '6Gftys2Eb')
  call term_sendkeys(buf, '7GWWys2E*')
  call term_sendkeys(buf, '89G3WysE!W.')
  call term_sendkeys(buf, ":%norm yss`\<CR>")
  let output_file = "surround_add_basic_test"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ "`'Lorem' ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat`",
        \ "`#fermentum pretium#.  !Cras eu dolor imperdiet justo mattis pulvinar.!  Cras nec`",
        \ "`(lectus) ( ligula ).  Proin elementum luctus elit, a tincidunt quam facilisis non.`",
        \ "`Nunc quis ( mauris ) non *turpis finibus luctus.  Maecenas ante sapien, sagittis*`",
        \ "`quis accumsan in, feugiat *quis sem.*`",
        \ "`one 보(two 여보세요) дважды два четыре`",
        \ "`three세 four *여보세요 это* всем известно`",
        \ "`five 요 six !여보세요! !여보세요! в целом мире`"
        \] , result)
endfunc

func Test_single_line_surround_add()
  let lines =<< trim END
    Lorem ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat
    fermentum pretium.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec
    lectus ligula.  Proin elementum luctus elit, a tincidunt quam facilisis non.
    Nunc quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis
    quis accumsan in, feugiat quis sem.
  END

  let input_file = "test_basic_surround_add_input"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
  call term_sendkeys(buf, "yssb.")
  call term_sendkeys(buf, "jyss{")
  call term_sendkeys(buf, '2jyss"')
  call term_sendkeys(buf, "jysstp\<CR>")
  call term_sendkeys(buf, '2jyss]')
  let output_file = "surround_add_single_line_test"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ '((Lorem ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat))',
        \ '{',
        \ 'fermentum pretium.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec',
        \ '}',
        \ '"lectus ligula.  Proin elementum luctus elit, a tincidunt quam facilisis non."',
        \ '<p>',
        \ 'Nunc quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis',
        \ '</p>',
        \ '[quis accumsan in, feugiat quis sem.]'
        \] , result)
endfunc

func Test_visual_line_surround_add()
  let lines =<< trim END
    Lorem ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat
    fermentum pretium.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec
    lectus ligula.  Proin elementum luctus elit, a tincidunt quam facilisis non.
    Nunc quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis
    quis accumsan in, feugiat quis sem.
  END

  let input_file = "test_visual_line_surround_add_input"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
  call term_sendkeys(buf, "vipS}")
  call term_sendkeys(buf, "vipS{")
  call term_sendkeys(buf, "vipSB")
  call term_sendkeys(buf, "vipS)")
  call term_sendkeys(buf, "vipS(")
  call term_sendkeys(buf, "vipSb")
  call term_sendkeys(buf, '6GVjS"')
  call term_sendkeys(buf, "8GVjStp\<CR>")

  let output_file = "surround_add_visual_line_test"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ '(',
        \ '(',
        \ '({',
        \ '{',
        \ '{Lorem ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat',
        \ '"fermentum pretium.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec',
        \ 'lectus ligula.  Proin elementum luctus elit, a tincidunt quam facilisis non."',
        \ '<p>',
        \ 'Nunc quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis',
        \ 'quis accumsan in, feugiat quis sem.}',
        \ '</p>',
        \ '}',
        \ '})',
        \ ')',
        \ ')',
        \] , result)
endfunc

func Test_visual_char_surround_add()
  let lines =<< trim END
    Lorem ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat
    fermentum pretium.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec
    lectus ligula.  Proin elementum luctus elit, a tincidunt quam facilisis non.
    Nunc quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis
    quis accumsan in, feugiat quis sem.
    one 보two 여보세요 дважды два четыре
    three세 four 여보세요 это всем известно
    five 요 six 여보세요 여보세요 в целом мире
  END

  let input_file = "test_visual_char_surround_add_input"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
  call term_sendkeys(buf, "visS(")
  call term_sendkeys(buf, "j0vjf,S[")
  call term_sendkeys(buf, "veeS$")
  call term_sendkeys(buf, "jvjj$SB")
  call term_sendkeys(buf, "jlvjStp\<CR>")
  call term_sendkeys(buf, "6GWvjWWlSb")
  call term_sendkeys(buf, "8G3WlvWllSb")

  let output_file = "surround_add_visual_char_test"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ '( Lorem ipsum dolor sit amet, consectetur adipiscing elit. )  Maecenas feugiat',
        \ '[ $fermentum pretium$.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec',
        \ 'lec{tus ligula.  Proin elementum luctus elit, ] a tincidunt quam facilisis non.',
        \ 'Nunc <p>quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis',
        \ 'quis a</p>ccumsan in, feugiat quis sem.}',
        \ 'one (보two 여보세요 дважды два четыре',
        \ 'three세 four 여보)세요 это всем известно',
        \ 'five 요 six 여(보세요 여보세)요 в целом мире'
        \] , result)
endfunc

func Test_visual_block_surround_add()
  let lines =<< trim END
    Lorem ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat
    fermentum pretium.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec
    lectus ligula.  Proin elementum luctus elit, a tincidunt quam facilisis non.
    Nunc quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis
    quis accumsan in, feugiat quis sem.
  END

  let input_file = "test_visual_block_surround_add_input"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
  call term_sendkeys(buf, "\<C-v>GSb.fr.fc.")
  call term_sendkeys(buf, "fe\<C-v>G$Stp class=\"test\"\<CR>")

  let output_file = "surround_add_visual_block_test"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ '((L))o(r)em ipsum dolor sit amet, (c)ons<p class="test">ectetur adipiscing elit.  Maecenas feugiat</p>',
        \ '((f))e(r)mentum pretium.  Cras eu (d)olo<p class="test">r imperdiet justo mattis pulvinar.  Cras nec</p>',
        \ '((l))e(c)tus ligula.  Proin elemen(t)um <p class="test">luctus elit, a tincidunt quam facilisis non.</p>',
        \ '((N))u(n)c quis mauris non turpis (f)ini<p class="test">bus luctus.  Maecenas ante sapien, sagittis</p>',
        \ '((q))u(i)s accumsan in, feugiat qu(i)s s<p class="test">em.</p>',
        \] , result)
endfunc

func Test_visual_block_virtualedit_surround_add()
  let lines =<< trim END
    one 보two 여보세요 дважды два четыре

    세 four 여보세요 это

    five 요 six 여보세요 여보세요 всем известно в целом мире
  END

  let input_file = "test_visual_block_virtualedit_surround_add_input"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" -c "set virtualedit=all et" ' .. input_file, {})
  call term_sendkeys(buf, "\<C-v>e4jSb")
  call term_sendkeys(buf, "f ;;ll\<C-v>jjSb")
  call term_sendkeys(buf, "5G0f ;;;;\<C-v>ekkStp class=\"test\"\<CR>")
  call term_sendkeys(buf, "1G$20l\<C-v>llljjjS*")

  let output_file = "surround_add_visual_block_virtualedit_test"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ '(one) 보two 여보세요 д(в)ажды два четыре                   *    *',
        \ '(   )                 ( )                                  *    *',
        \ '(세 )four 여보세요 это( )      <p class="test">     </p>   *    *',
        \ '(   )                          <p class="test">     </p>   *    *',
        \ '(fiv)e 요 six 여보세요 여보세요<p class="test"> всем</p> известно в целом мире',
        \] , result)
endfunc

func Test_indent_surround_add()
  let lines =<< trim END
    #include <stdio.h>

    int main() {
        printf("hello world\n");
        return 0;
    }
  END

  let input_file = "test_indent_surround_add_input.c"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
  call term_sendkeys(buf, "4GVjS{.")

  let output_file = "indent_surround_add_test.c"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ '#include <stdio.h>',
        \ '',
        \ 'int main() {',
        \ "\t{",
        \ "\t\t{",
        \ "\t\t\tprintf(\"hello world\\n\");",
        \ "\t\t\treturn 0;",
        \ "\t\t}",
        \ "\t}",
        \ '}',
        \] , result)
endfunc

func Test_surround_remove()
  let lines =<< trim END
    (({["Lorem"]})) ipsum (dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat
    fermentum pretium).  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec
    lectus ligula.  "Proin elementum luctus elit", a tincidunt quam facilisis non.
    Nunc q<span class="test">uis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis
    quis accums</span>an in, feugiat quis sem.
    one "보two" 여보((세요 дважды)) два *четыре*
    three세 four 여{(보세)}요 это всем 'известно'
    five 요 six "여보세요" '여보세요' в _целом_ мире
    (hello \(this\) world)
    "hello \"this\" world"
  END

  let input_file = "test_surround_remove_input"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
  call term_sendkeys(buf, "fLdss....")
  call term_sendkeys(buf, "fddsb")
  call term_sendkeys(buf, 'jjfPds"')
  call term_sendkeys(buf, 'jdst')
  call term_sendkeys(buf, 'jj0f"ds"')
  call term_sendkeys(buf, 'f(dss.')
  call term_sendkeys(buf, 'f*ds*')
  call term_sendkeys(buf, "j$ds'")
  call term_sendkeys(buf, 'F)dss.')
  call term_sendkeys(buf, 'j0f".W.WW.')
  call term_sendkeys(buf, 'j0ftdsb')
  call term_sendkeys(buf, 'j0ftds"')

  let output_file = "basic_surround_remove_test"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ "Lorem ipsum dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat",
        \ "fermentum pretium.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec",
        \ "lectus ligula.  Proin elementum luctus elit, a tincidunt quam facilisis non.",
        \ "Nunc quis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis",
        \ "quis accumsan in, feugiat quis sem.",
        \ "one 보two 여보세요 дважды два четыре",
        \ "three세 four 여보세요 это всем известно",
        \ "five 요 six 여보세요 여보세요 в целом мире",
        \ 'hello \(this\) world',
        \ 'hello \"this\" world'
        \] , result)
endfunc

" func Test_surround_change()
"   let lines =<< trim END
"     (({["Lorem"]})) ipsum (dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat
"     fermentum pretium).  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec
"     lectus ligula.  "Proin elementum luctus elit", a tincidunt quam facilisis non.
"     Nunc q<span class="test">uis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis
"     quis accums</span>an in, feugiat quis sem.
"     one "보two" 여보((세요 дважды)) два *четыре*
"     three세 four 여{(보세)}요 это всем 'известно'
"     five 요 six "여보세요" '여보세요' в _целом_ мире
"     (hello \(this\) world)
"     "hello \"this\" world"
"   END

"   let input_file = "test_surround_change_input"
"   call writefile(lines, input_file, "D")

"   let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
"   " call term_sendkeys(buf, "csbB.csBb")
"   call term_sendkeys(buf, "csb}}.cs}))")
"   " call term_sendkeys(buf, "fLcs\"''")
"   " call term_sendkeys(buf, "cs[<<")
"   " call term_sendkeys(buf, "fdcsstp\<CR>")
"   " call term_sendkeys(buf, "3GfPcss`")
"   " call term_sendkeys(buf, "4Gfscst*`")
"   " call term_sendkeys(buf, "6Gftcss'")
"   " call term_sendkeys(buf, "f)cs(}.")
"   " call term_sendkeys(buf, "$cs*_")
"   " call term_sendkeys(buf, "7Gf(css_cs{*")
"   " call term_sendkeys(buf, "$cs'#")
"   " call term_sendkeys(buf, '8Gf"cssbW.WW.')
"   " call term_sendkeys(buf, '9GftcssB')
"   " call term_sendkeys(buf, "10Gftcss'")

"   let output_file = "basic_surround_change_test"
"   call term_sendkeys(buf, $":w {output_file}\<CR>")
"   defer delete(output_file)

"   call StopVimInTerminal(buf)
"   let result = readfile(output_file)
"   call assert_equal([
"         \ "{{([\"Lorem\"])}} ipsum <p>dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat",
"         \ 'fermentum pretium</p>.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec',
"         \ 'lectus ligula.  `Proin elementum luctus elit`, a tincidunt quam facilisis non.',
"         \ 'Nunc q*uis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis',
"         \ 'quis accums*an in, feugiat quis sem.',
"         \ "one '보two' 여보{{세요 дважды}} два _четыре_",
"         \ 'three세 four 여*_보세_*요 это всем #известно#',
"         \ 'five 요 six (여보세요) (여보세요) в (целом) мире',
"         \ '{hello \(this\) world}',
"         \ '''hello \"this\" world''',
"         \] , result)
" endfunc
"
"

func Test_surround_change()
  let lines =<< trim END
    (({['Lorem']})) ipsum
    (hello \(this\) world)
    "hello \"this\" world"
    hello (this) world
    hello <p class="hello">this</p> world
  END

  let input_file = "test_surround_change_input"
  call writefile(lines, input_file, "D")

  let buf = RunVimInTerminal('-c "packadd surround" ' .. input_file, {})
  call term_sendkeys(buf, "csb}}.cs}))")
  call term_sendkeys(buf, "2Gftcsb}}")
  call term_sendkeys(buf, "3Gftcs\"'")
  call term_sendkeys(buf, "4Gftcs(tp\<CR>")
  call term_sendkeys(buf, "5Gftcstb")

  let output_file = "basic_surround_change_test"
  call term_sendkeys(buf, $":w {output_file}\<CR>")
  defer delete(output_file)

  call StopVimInTerminal(buf)
  let result = readfile(output_file)
  call assert_equal([
        \ "{{(['Lorem'])}} ipsum",
        \ '{hello \(this\) world}',
        \ '''hello \"this\" world''',
        \ 'hello <p>this</p> world',
        \ 'hello (this) world'
        \] , result)
endfunc
