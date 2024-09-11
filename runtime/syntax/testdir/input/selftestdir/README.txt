The test files with made-up syntax in this directory serve for additional
linewise checks to be manually performed whenever the algorithm managing
screen dump file generation is modified (../../runtest.vim#RunTest()).

This is mainly used for debugging and testing the syntax test suite.

Please test any changes as follows:
	cd runtime/syntax/
	make clean self-testing test

