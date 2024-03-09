" Vim :menutranslate command

menutranslate clear
menutranslate clear | echo "Foo"
menutranslate clear " comment

menutranslate &Foo\ bar &FuBar | echo "Foo"

menutranslate &Foo\ bar &FuBar " comment
menutranslate \"&Foo"\ bar \"&FuBar
menutranslate &Foo\ "bar" &FuBar

menutranslate &Foo\ bar
      \ &Fubar | echo "Foo"

menutranslate
      \ &Foo\ bar
      \ &Fubar | echo "Foo"

menutranslate
      \ &Foo\ bar
      \ &Fubar| echo "Foo"

menutranslate
      \ &Foo\ bar
      \ &Fubar
      \ | echo "Foo"

menutranslate &Foo\ bar
      "\ comment
      \ &Fubar | echo "Foo"

menutranslate
      "\ comment
      \ &Foo\ bar
      "\ comment
      \ &Fubar | echo "Foo"

menutranslate
      \ &Foo\ bar
      "\ comment
      \ &Fubar| echo "Foo"
      "\ comment

menutranslate
      "\ comment
      \ &Foo\ bar
      "\ comment
      \ &Fubar
      \ | echo "Foo"

