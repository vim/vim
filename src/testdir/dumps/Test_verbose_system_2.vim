" Filter that removes the ever changing temp directory name from the screendump
" that shows the system() command executed.
" This should be on the first line, but if it isn't there ignore the error,
" the screendump will then show the problem.
1,2s+|>|/|.*|2|>|&|1|".*+|>|.|.|.|2|>|\&|1|"+e
