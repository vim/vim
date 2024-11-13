
source check.vim
import './vim9.vim' as v9
CheckFeature python3

def Test_python3_py3eval_locals()
  var lines =<< trim EOF
    var s = 'string'
    var d = {'s': s}
    assert_equal('string', py3eval('s', {'s': s}))
    py3eval('d.update({"s": "new"})', {'d': d})
    assert_equal('new', d['s'])
  EOF
  v9.CheckDefAndScriptSuccess(lines)
enddef

" vim: shiftwidth=2 sts=2 expandtab
