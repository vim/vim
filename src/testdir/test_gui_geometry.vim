" Tests for GUI window geometry: initial size from -geometry option and
" size stability after :tabnew / :tabclose.
"
" Background: on GTK3 with client-side decorations (Wayland), the window
" compositor subtracts the CSD frame from the requested size, causing the
" window to open a few pixels too small (wrong &columns/&lines) and to shrink
" further with each :tabnew/:tabclose cycle.

CheckCanRunGui

source util/setup_gui.vim

func Setup()
  call GUISetUpCommon()
endfunc

func TearDown()
  call GUITearDownCommon()
endfunc

" Test that a GUI window opened with -geometry=WxH has exactly W columns
" and H lines.
"
" Without the CSD fix, on GTK3/Wayland the compositor subtracts the frame
" margin from the requested pixel size, so the window is a character cell too
" narrow and too short.
func Test_geometry_exact_size()
  CheckCanRunGui
  CheckFeature gui_gtk

  let after =<< trim [CODE]
    call writefile([string(&columns), string(&lines)], 'Xtest_geomsize')
    qall
  [CODE]

  " Hide the menu bar so it does not widen the minimum window size.
  if RunVim(['set guioptions-=m'], after, '-f -g -geometry 40x15')
    let result = readfile('Xtest_geomsize')
    call assert_equal('40', result[0], 'columns should match -geometry width')
    call assert_equal('15', result[1], 'lines should match -geometry height')
  endif

  call delete('Xtest_geomsize')
endfunc

" Test that the window size is unchanged after opening and closing a tab.
"
" Each :tabnew/:tabclose cycle triggers a tabline show/hide, which causes
" asynchronous GTK layout events.  Without the fix, stale configure events
" from these layout passes are mis-interpreted as user resizes, reducing
" &columns and &lines with every cycle.  Three cycles are performed to
" amplify any drift.
func Test_tabnew_tabclose_size_stable()
  CheckCanRunGui
  CheckFeature gui_gtk

  let after =<< trim [CODE]
    let cols0 = &columns
    let rows0 = &lines
    tabnew
    sleep 300m
    tabclose
    sleep 300m
    tabnew
    sleep 300m
    tabclose
    sleep 300m
    tabnew
    sleep 300m
    tabclose
    sleep 300m
    call writefile([string(cols0), string(rows0), string(&columns), string(&lines)], 'Xtest_tabsize')
    qall
  [CODE]

  if RunVim(['set guioptions-=m'], after, '-f -g -geometry 40x15')
    let result = readfile('Xtest_tabsize')
    call assert_equal('40', result[0], 'initial columns should match -geometry width')
    call assert_equal('15', result[1], 'initial lines should match -geometry height')
    call assert_equal(result[0], result[2],
          \ 'columns changed after 3x tabnew/tabclose: '
          \ .. result[0] .. ' -> ' .. result[2])
    call assert_equal(result[1], result[3],
          \ 'lines changed after 3x tabnew/tabclose: '
          \ .. result[1] .. ' -> ' .. result[3])
  endif

  call delete('Xtest_tabsize')
endfunc
