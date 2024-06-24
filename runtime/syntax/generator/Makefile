VIM_SRCDIR = ../../../src
RUN_VIMPROG = $(VIM_SRCDIR)/vim -N -u NONE -i NONE -n
REVISION ?= $(shell date +%Y-%m-%dT%H:%M:%S%:z)

SRC =	$(VIM_SRCDIR)/eval.c $(VIM_SRCDIR)/ex_cmds.h $(VIM_SRCDIR)/ex_docmd.c \
		$(VIM_SRCDIR)/fileio.c $(VIM_SRCDIR)/option.c $(VIM_SRCDIR)/syntax.c

export VIM_SRCDIR

.PHONY: generate clean
all: generate

generate: vim.vim

vim.vim: vim.vim.rc update_date.vim
	@echo "Generating vim.vim ..."
	@cp -f vim.vim.rc ../vim.vim
	@$(RUN_VIMPROG) -S update_date.vim
	@echo "done."

vim.vim.rc: gen_syntax_vim.vim vim.vim.base $(SRC)
	@echo "Generating vim.vim.rc ..."
	@rm -f sanity_check.err generator.err
	@$(RUN_VIMPROG) -S gen_syntax_vim.vim
	@if test -f sanity_check.err ; then \
		echo ; \
		echo "Sanity errors:" ; \
		cat sanity_check.err ; \
		exit 1 ; \
	fi
	@if test -f generator.err ; then \
		echo ; \
		echo "Generator errors:" ; \
		cat generator.err ; \
		echo ; \
		exit 1 ; \
	fi
	@echo "done."

clean:
	rm -f vim.vim.rc
	rm -f vim.vim
	rm -f sanity_check.err generator.err
