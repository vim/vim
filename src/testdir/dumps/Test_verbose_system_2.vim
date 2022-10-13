" Filter that removes the ever changing temp directory name from the screendump
" that shows the system() command executed.
1s+|t|m|p|/|.|.|.*| |+|t|m|p|/|x|x|x|x|x|x|x|/|1| |+
