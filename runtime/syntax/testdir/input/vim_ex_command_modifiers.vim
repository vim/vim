" Vim Ex command modifiers


 aboveleft        echo "Foo"

aboveleft         echo "Foo"
belowright        echo "Foo"
botright          echo "Foo"
browse            echo "Foo"
confirm           echo "Foo"
filter  /pattern/ echo "Foo"
filter! /pattern/ echo "Foo"
hide              echo "Foo"
horizontal        echo "Foo"
keepalt           echo "Foo"
keepjumps         echo "Foo"
keepmarks         echo "Foo"
keeppatterns      echo "Foo"
leftabove         echo "Foo"
legacy            echo "Foo"
lockmarks         echo "Foo"
noautocmd         echo "Foo"
noswapfile        echo "Foo"
rightbelow        echo "Foo"
sandbox           echo "Foo"
silent            echo "Foo"
silent!           echo "Foo"
tab               echo "Foo"
topleft           echo "Foo"
unsilent          echo "Foo"
verbose           echo "Foo"
vertical          echo "Foo"
vim9cmd           echo "Foo"


:  aboveleft       echo "Foo"
 " FIXME: not a ternary operator ':'
 : aboveleft       echo "Foo"
  :aboveleft       echo "Foo"

:aboveleft         echo "Foo"
:belowright        echo "Foo"
:botright          echo "Foo"
:browse            echo "Foo"
:confirm           echo "Foo"
:filter  /pattern/ echo "Foo"
:filter! /pattern/ echo "Foo"
:hide              echo "Foo"
:horizontal        echo "Foo"
:keepalt           echo "Foo"
:keepjumps         echo "Foo"
:keepmarks         echo "Foo"
:keeppatterns      echo "Foo"
:leftabove         echo "Foo"
:legacy            echo "Foo"
:lockmarks         echo "Foo"
:noautocmd         echo "Foo"
:noswapfile        echo "Foo"
:rightbelow        echo "Foo"
:sandbox           echo "Foo"
:silent            echo "Foo"
:silent!           echo "Foo"
:tab               echo "Foo"
:topleft           echo "Foo"
:unsilent          echo "Foo"
:verbose           echo "Foo"
:vertical          echo "Foo"
:vim9cmd           echo "Foo"


echo|aboveleft           echo "Foo"
echo| aboveleft          echo "Foo"
echo |aboveleft          echo "Foo"

echo | aboveleft         echo "Foo"
echo | belowright        echo "Foo"
echo | botright          echo "Foo"
echo | browse            echo "Foo"
echo | confirm           echo "Foo"
echo | filter  /pattern/ echo "Foo"
echo | filter! /pattern/ echo "Foo"
echo | hide              echo "Foo"
echo | horizontal        echo "Foo"
echo | keepalt           echo "Foo"
echo | keepjumps         echo "Foo"
echo | keepmarks         echo "Foo"
echo | keeppatterns      echo "Foo"
echo | leftabove         echo "Foo"
echo | legacy            echo "Foo"
echo | lockmarks         echo "Foo"
echo | noautocmd         echo "Foo"
echo | noswapfile        echo "Foo"
echo | rightbelow        echo "Foo"
echo | sandbox           echo "Foo"
echo | silent            echo "Foo"
echo | silent!           echo "Foo"
echo | tab               echo "Foo"
echo | topleft           echo "Foo"
echo | unsilent          echo "Foo"
echo | verbose           echo "Foo"
echo | vertical          echo "Foo"
echo | vim9cmd           echo "Foo"


aboveleft belowright botright browse confirm filter /pattern/ filter! /pattern/ hide horizontal keepalt keepjumps keepmarks keeppatterns leftabove legacy lockmarks noautocmd noswapfile rightbelow sandbox silent silent! tab topleft unsilent verbose vertical vim9cmd echo "Foo"
aboveleft
      \ belowright
      \ botright
      \ browse
      \ confirm
      \ filter  /pattern/
      \ filter! /pattern/
      \ hide
      \ horizontal
      \ keepalt
      \ keepjumps
      \ keepmarks
      \ keeppatterns
      \ leftabove
      \ legacy
      \ lockmarks
      \ noautocmd
      \ noswapfile
      \ rightbelow
      \ sandbox
      \ silent
      \ silent!
      \ tab
      \ topleft
      \ unsilent
      \ verbose
      \ vertical
      \ vim9cmd
      \ echo "Foo"


aboveleft         42print
belowright        42print
botright          42print
browse            42print
confirm           42print
filter  /pattern/ 42print
filter! /pattern/ 42print
hide              42print
horizontal        42print
keepalt           42print
keepjumps         42print
keepmarks         42print
keeppatterns      42print
leftabove         42print
legacy            42print
lockmarks         42print
noautocmd         42print
noswapfile        42print
rightbelow        42print
sandbox           42print
silent            42print
silent!           42print
tab               42print
topleft           42print
unsilent          42print
verbose           42print
vertical          42print
vim9cmd           42print

aboveleft         :42print
belowright        :42print
botright          :42print
browse            :42print
confirm           :42print
filter  /pattern/ :42print
filter! /pattern/ :42print
hide              :42print
horizontal        :42print
keepalt           :42print
keepjumps         :42print
keepmarks         :42print
keeppatterns      :42print
leftabove         :42print
legacy            :42print
lockmarks         :42print
noautocmd         :42print
noswapfile        :42print
rightbelow        :42print
sandbox           :42print
silent            :42print
silent!           :42print
tab               :42print
topleft           :42print
unsilent          :42print
verbose           :42print
vertical          :42print
vim9cmd           :42print


silent :delete _
silent $delete _
silent :$delete _

lockmarks '[,']d _

silent keeppatterns %s/\v^%(%(([=`:.'"~^_*+#-])\1+\n)?.{1,2}\n([=`:.'"~^_*+#-])\2+)|%(%(([=`:.''"~^_*+#-])\3{2,}\n)?.{3,}\n([=`:.''"~^_*+#-])\4{2,})$/\=closure.Process(submatch(0))/gn
