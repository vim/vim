" Test Vim9 classes

source check.vim
import './vim9.vim' as v9

def Test_class_basic()
  var lines =<< trim END
      class NotWorking
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1316:')

  lines =<< trim END
      vim9script
      class notWorking
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1314:')

  lines =<< trim END
      vim9script
      class Not@working
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1315:')

  lines =<< trim END
      vim9script
      abstract noclass Something
      endclass
  END
  v9.CheckScriptFailure(lines, 'E475:')

  lines =<< trim END
      vim9script
      abstract classy Something
      endclass
  END
  v9.CheckScriptFailure(lines, 'E475:')

  lines =<< trim END
      vim9script
      class Something
      endcl
  END
  v9.CheckScriptFailure(lines, 'E1065:')

  lines =<< trim END
      vim9script
      class Something
      endclass school's out 
  END
  v9.CheckScriptFailure(lines, 'E488:')

  lines =<< trim END
      vim9script
      class Something
      endclass | echo 'done'
  END
  v9.CheckScriptFailure(lines, 'E488:')

  lines =<< trim END
      vim9script
      class Something
        this
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1317:')

  lines =<< trim END
      vim9script
      class Something
        this.
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1317:')

  lines =<< trim END
      vim9script
      class Something
        this .count
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1317:')

  lines =<< trim END
      vim9script
      class Something
        this. count
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1317:')

  lines =<< trim END
      vim9script
      class Something
        this.count: number
        that.count
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1318: Not a valid command in a class: that.count')

  lines =<< trim END
      vim9script
      class Something
        this.count
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1022:')

  lines =<< trim END
      vim9script
      class Something
        this.count : number
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1059:')

  lines =<< trim END
      vim9script
      class Something
        this.count:number
      endclass
  END
  v9.CheckScriptFailure(lines, 'E1069:')

  lines =<< trim END
      vim9script

      class TextPosition
        this.lnum: number
	this.col: number
      endclass

      # use the automatically generated new() method
      var pos = TextPosition.new(2, 12)
      assert_equal(2, pos.lnum)
      assert_equal(12, pos.col)
  END
  v9.CheckScriptSuccess(lines)
enddef


" vim: ts=8 sw=2 sts=2 expandtab tw=80 fdm=marker
