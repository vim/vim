" Test :version Ex command

func Test_version()
  " version should always return the same string.
  let v1 = execute('version')
  let v2 = execute('version')
  call assert_equal(v1, v2)

  call assert_match("^\n\nVIM - Vi IMproved .*", v1)

  " Extract the list of features (e.g.  ['+acl', '-arabic', ...])
  let features_str = substitute(v1, ".*Features included (+) or not (-):\n", '', '')
  let features_str = substitute(features_str, "\n   system vimrc file:.*", '', '')
  let features = split(features_str)
  call assert_notequal([], features)

  " Check that if a feature in :version shows +xxx then has('xxx') is 1
  " and if it shows -xxx then has('xxx) is 0.
  "
  " Notes:
  " - the fork feature is displayed in :version as "+fork()" with parentheses
  " - the builtin_terms feature may be displayed as "++builtin_terms" (2 pluses)
  for feature_str in features
    let has_feature = (feature_str[0] == '+')
    let feature = substitute(substitute(
          \ feature_str, '^[+-]\+', '', ''), '()$', '', '')

    call assert_equal(has_feature, has(feature), feature)
  endfor
endfunc
