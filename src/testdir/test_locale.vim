" Test for locale behaviors

source check.vim

" Test that on macOS, we correctly set the default locale to have UTF-8
" encoding and LC_NUMERIC is set to "C", if $LANG env variable is not set.
func Test_macos_default_locale()
  if !has('osxdarwin')
    return
  endif

  " Run Vim after unsetting all the locale environmental vars, and capture the
  " output of :lang.
  let lang_results = system("unset LANG; unset LC_MESSAGES; " ..
                            \ shellescape(v:progpath) ..
                            \ " --clean -esX -c 'redir @a' -c 'lang' -c 'put a' -c 'print' -c 'qa!' ")

  " Check that:
  " 1. The locale is the form of <locale>.UTF-8.
  " 2. Check that fourth item (LC_NUMERIC) is properly set to "C".
  " Example match: "en_US.UTF-8/en_US.UTF-8/en_US.UTF-8/C/en_US.UTF-8/en_US.UTF-8"
  call assert_match('"\([a-zA-Z_]\+\.UTF-8/\)\{3}C\(/[a-zA-Z_]\+\.UTF-8\)\{2}"',
                    \ lang_results,
                    \ "Default locale should have UTF-8 encoding set, and LC_NUMERIC set to 'C'")
endfunc

" vim: shiftwidth=2 sts=2 expandtab
