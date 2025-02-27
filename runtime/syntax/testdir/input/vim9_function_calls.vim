vim9script

# Vim9 function calls

clearmatches()
:call clearmatches()
echo "Foo" | clearmatches()


# Issue 16721 (vimscript highlight of builtin function after |)

&directory = $'{$MYVIMDIR}/.data/swap/'
&backupdir = $'{$MYVIMDIR}/.data/backup//'
&undodir = $'{$MYVIMDIR}/.data/undo//'
if !isdirectory(&undodir)   | mkdir(&undodir, "p")   | endif
if !isdirectory(&backupdir) | mkdir(&backupdir, "p") | endif
if !isdirectory(&directory) | mkdir(&directory, "p") | endif

