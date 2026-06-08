" Vim :delete command


delete


" print flags


" :delete | :list

dl
del	" :delete only
dell
delel
deletl
deletel

" :delete | :print
	
dp
dep
delp
delep
deletp
deletep

" :delete | :number
	
d#
de#
del#
dele#
delet#
delete#

" after register and count args

delete l l
delete p p
delete a #

delete ll
delete pp
delete a#

delete 42 l
delete 42 p
delete 42 #

delete 42l
delete 42p
delete 42#

delete l 42 l
delete p 42 p
delete a 42 #

delete l42l
delete p42p
delete a42#

" multiple, any order, optional whitespace

delete l p # lp# ll pp ##


" registers and count

delete l
delete p
delete _
delete \"

delete_
delete\"

delete a
delete 42
delete a 42
delete a42

delete_
delete42
delete_42
delete _42
delete _ 42

delete\"
delete42
delete\"42
delete \"42
delete \" 42


" trailing bar and tail comment

delete      | echo "..."
delete|       echo "..."
delete a    | echo "..."
delete a|     echo "..."
delete 42   | echo "..."
delete 42|    echo "..."
deletel     | echo "..."
deletel|      echo "..."

delete      " comment
delete"       comment
delete a    " comment
delete a"     comment
delete 42   " comment
delete 42"    comment
deletel     " comment
deletel"      comment

