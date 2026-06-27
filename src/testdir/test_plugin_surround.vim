" Test for the surround package

packadd surround

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

  enew
  call setline(1, lines)

  normal ysiw'
  normal j0yst.#
  normal fCysis!
  normal j0ysiwb
  normal fiysiw(
  normal j.3Wys$*
  normal j.
  normal 6Gftys2Eb
  normal 7GWWys2E*
  normal 8G3WysE!W.
  exe "%normal yss`\<CR>"
  let result = getline(1, '$')
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

  enew
  call setline(1, lines)

  normal yssb.
  normal jyss{
  normal 2jyss"
  exe "normal jysstp\<CR>"
  normal 2jyss]

  let result = getline(1, '$')
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

  call setline(1, lines)

  normal vipS}
  normal vipS{
  normal vipSB
  normal vipS)
  normal vipS(
  normal vipSb
  normal 6GVjS"
  exe "normal 8GVjStp\<CR>"

  let result = getline(1, '$')
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

  enew
  call setline(1, lines)

  normal visS(
  normal j0vjf,S[
  normal veeS$
  normal jvjj$SB
  exe "normal jlvjStp\<CR>"
  normal 6GWvjWWlSb
  normal 8G3WlvWllSb

  let result = getline(1, '$')
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

  enew
  call setline(1, lines)

  exe "normal \<C-v>GSb.fr.fc."
  exe "normal fe\<C-v>G$Stp class=\"test\"\<CR>"

  let result = getline(1, '$')
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

  enew
  setlocal virtualedit=all et
  call setline(1, lines)

  exe "normal \<C-v>e4jSb"
  exe "normal f ;;ll\<C-v>jjSb"
  exe "normal 5G0f ;;;;\<C-v>ekkStp class=\"test\"\<CR>"
  exe "normal 1G$20l\<C-v>llljjjS*"

  let result = getline(1, '$')
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

  enew
  set ft=c
  setlocal ve&
  call setline(1, lines)
  normal 4GVjS{.

  let result = getline(1, '$')
  call assert_equal([
        \ '#include <stdio.h>',
        \ '',
        \ 'int main() {',
        \ "\t{",
        \ "\t\t{",
        \ "\t\t\tprintf(\"hello world\\n\");",
        \ "\t\t\treturn 0;",
        \ "\t\t}",
        \ "    }",
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

  enew
  call setline(1, lines)

  normal fLdss....
  normal fddsb
  normal jjfPds"
  normal jdst
  normal jj0f"ds"
  normal f(dss.
  normal f*ds*
  normal j$ds'
  normal F)dss.
  normal j0f".W.WW.
  normal j0ftdsb
  normal j0ftds"

  let result = getline(1, '$')
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

func Test_surround_change()
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
    another one (hello \(this\) world)h
  END

  enew
  call setline(1, lines)

  normal csb}.cs})css>css'
  exe "normal fdcsstspan class=\"text\"\<CR>"
  normal 3GfPcs"'
  normal 4Gfscst"
  normal 6Gftcssb
  normal f(f)cssBcsb*$cs*_
  normal 7Gf(css]$css"
  normal 8Gf"css]f'cssbf_css'
  normal 9Gftcss]
  normal 10Gftcs"'
  normal 11Gfics)'

  let result = getline(1, '$')
  call assert_equal([
        \ '{{(<''Lorem''>)}} ipsum <span class="text">dolor sit amet, consectetur adipiscing elit.  Maecenas feugiat',
        \ 'fermentum pretium</span>.  Cras eu dolor imperdiet justo mattis pulvinar.  Cras nec',
        \ 'lectus ligula.  ''Proin elementum luctus elit'', a tincidunt quam facilisis non.',
        \ 'Nunc q"uis mauris non turpis finibus luctus.  Maecenas ante sapien, sagittis',
        \ 'quis accums"an in, feugiat quis sem.',
        \ 'one (보two) 여보*{세요 дважды}* два _четыре_',
        \ 'three세 four 여{[보세]}요 это всем "известно"',
        \ 'five 요 six [여보세요] (여보세요) в ''целом'' мире',
        \ '[hello \(this\) world]',
        \ '''hello \"this\" world''',
        \ 'another one ''hello \(this\) world''h',
        \] , result)
endfunc

func Test_surround_change2()
  let lines =<< trim END
    Lorem ipsum (
    fermentum pretium).
  END

  enew
  call setline(1, lines)

  normal f(cs({cs{[

  let result = getline(1, '$')
  call assert_equal([
        \ 'Lorem ipsum [',
        \ 'fermentum pretium ].'
        \] , result)
endfunc

func Test_surround_custom_pairs()
  let lines =<< trim END
    one "보two" 여보((세요 дважды)) два *четыре*
    three세 four 여{(보세)}요 «это» всем 'известно'
    five 요 ‹six› "여보세요" '여보세요' в _целом_ мире
  END

  enew
  call setline(1, lines)

  let b:surround_pairs = {
        \ 'q': ("\n‘", "’"), 'Q': ("\n“", "”"),
        \ 'w': ("\n‹", "›"), 'W': ("\n«", "»")
        \}

  normal ysiwq
  normal ftcssWf(cssWcsswf*cssQ
  normal 2Gf(cs(q
  normal WdsW
  normal 3Gfscswq
  normal f"cssQf'cssq
  normal vipSW

  let result = getline(1, '$')
  call assert_equal([
        \ "«‘one’ «보two» 여보«‹세요 дважды›» два “четыре”",
        \ "three세 four 여{‘보세’}요 это всем 'известно'",
        \ "five 요 ‘six’ “여보세요” ‘여보세요’ в _целом_ мире»",
        \] , result)
endfunc
