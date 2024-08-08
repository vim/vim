" Vim :menu commands

" :help disable menus
menu disable &File.&Open\.\.\.
amenu enable *
amenu disable &Tools.*


" :help menu-examples
nmenu Words.Add\ Var         wb"zye:menu! Words.<C-R>z <C-R>z<CR>
nmenu Words.Remove\ Var      wb"zye:unmenu! Words.<C-R>z<CR>
vmenu Words.Add\ Var         "zy:menu! Words.<C-R>z <C-R>z <CR>
vmenu Words.Remove\ Var      "zy:unmenu! Words.<C-R>z<CR>
imenu Words.Add\ Var         <Esc>wb"zye:menu! Words.<C-R>z <C-R>z<CR>a
imenu Words.Remove\ Var      <Esc>wb"zye:unmenu! Words.<C-R>z<CR>a


" special keys
menu <silent> &Foo\ bar  :echo "Foobar"<CR>
menu <special> &Foo\ bar :echo "Foobar"<CR>
menu <script> &Foo\ bar  :echo "Foobar"<CR>
menu <silent> <special> &Foo\ bar :echo "Foobar"<CR>
menu <silent> <special> <script> &Foo\ bar :echo "Foobar"<CR>


function Foo()
  menu <silent> &Foo\ bar :echo "Foobar"<CR>
endfunction


" Example: runtime/menu.vim (modified)
an <silent> 10.330 &File.&Close<Tab>:close :confirm close<CR>

an <silent> 10.330 &File.&Close<Tab>:close
        \ :if winheight(2) < 0 && tabpagewinnr(2) == 0 <Bar>
	\   confirm enew <Bar>
	\ else <Bar>
	\   confirm close <Bar>
	\ endif<CR>

an <silent> 10.330 &File.&Close<Tab>:close
	"\ comment
        \ :if winheight(2) < 0 && tabpagewinnr(2) == 0 <Bar>
	"\ comment
	\   confirm enew <Bar>
	"\ comment
	\ else <Bar>
	"\ comment
	\   confirm close <Bar>
	"\ comment
	\ endif<CR>

an <silent> 10.330 &File.&Close<Tab>:close :if winheight(2) < 0 && tabpagewinnr(2) == 0 <Bar>
	\   confirm enew <Bar>
	\ else <Bar>
	\   confirm close <Bar>
	\ endif<CR>

an <silent> 10.330 &File.&Close<Tab>:close :if winheight(2) < 0 && tabpagewinnr(2) == 0 <Bar>
	"\ comment
	\   confirm enew <Bar>
	"\ comment
	\ else <Bar>
	"\ comment
	\   confirm close <Bar>
	"\ comment
	\ endif<CR>


" popup menus
popup &Foo  | echo "Foo"
popup! &Foo | echo "Foo"


" Issue #14230

" a menu item name cannot start with '.'

def HistoryJumpMenu()
    popup.FilterMenu("Jump history", dir_hist,
        (res, _) => {
            HistoryJump(res.text)
        })
enddef

popup\.FilterMenu<Tab>Filter()<CR>

