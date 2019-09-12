func Test_cjk_linebreak_after()
  for punct in [
    '!', '%', ')', ',', ':', ';', '>', '?', ']', '}', '’', '”', '†', '‡',
    '…', '‰', '‱', '‼', '⁇', '⁈', '⁉', '℃', '℉', '、', '。', '〉', '》',
    '」', '』', '】', '〕', '〗', '〙', '〛', '！', '）', '，', '．', '：',
    '；', '？', '］', '｝']
    call setline('.', '这是一个测试'.punct.'试试 CJK 行禁则补丁。')
    set textwidth=12 formatoptions=croqn2mB1j
    normal gqq
    call assert_equal('这是一个测试'.punct, getline(1))
  endfor
endfunc

func Test_cjk_linebreak_before()
  for punct in [
    '(', '<', '[', '`', '{', '‘', '“', '〈', '《', '「', '『', '【', '〔',
    '〖', '〘', '〚', '（', '［', '｛']
    call setline('.', '这是个测试'.punct.'试试 CJK 行禁则补丁。')
    set textwidth=12 formatoptions=croqn2mB1j
    normal gqq
    call assert_equal('这是个测试'.punct, getline(1))
  endfor
endfunc

func Test_cjk_linebreak_nobetween()
  for punct in ['……', '——']
    call setline('.', '这是个测试'.punct.'试试 CJK 行禁则补丁。')
    set textwidth=12 formatoptions=croqn2mB1j ambiwidth=double
    normal gqq
    call assert_equal('这是个测试'.punct, getline(1))
  endfor

  %d_
  call setline('.', '这是一个测试……试试 CJK 行禁则补丁。')
  set textwidth=12 formatoptions=croqn2mB1j ambiwidth=double
  normal gqq
  call assert_equal('这是一个测', getline(1))

  %d_
  call setline('.', '这是个测试——试试 CJK 行禁则补丁。')
  set textwidth=12 formatoptions=croqn2mB1j ambiwidth=double
  normal gqq
  call assert_equal('这是个测试', getline(1))
endfunc
