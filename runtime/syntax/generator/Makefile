#
# Makefile for generate runtime/syntax/vim.vim
#
VIM_SRCDIR := ../../../src
RUN_VIMPROG := $(VIM_SRCDIR)/vim -N -u NONE -i NONE -n
TARGET := ../vim.vim
CHECK_HELP_DOC := 0

SRC :=	$(VIM_SRCDIR)/eval.c $(VIM_SRCDIR)/ex_cmds.h $(VIM_SRCDIR)/ex_docmd.c \
		$(VIM_SRCDIR)/fileio.c $(VIM_SRCDIR)/option.c $(VIM_SRCDIR)/syntax.c

export VIM_SRCDIR
export CHECK_HELP_DOC

.PHONY: generate check_doc clean
all: generate

generate: $(TARGET)

check_doc: CHECK_HELP_DOC := 1
check_doc: clean $(TARGET)

clean:
	rm -f vim.vim.rc $(TARGET)
	rm -f sanity_check.err generator.err

$(TARGET): vim.vim.rc update_date.vim
	@echo "Generating $(TARGET) ..."
	@cp -f vim.vim.rc $(TARGET)
	@$(RUN_VIMPROG) -S update_date.vim $(TARGET)
	@echo "done."

vim.vim.rc: gen_syntax_vim.vim vim.vim.base $(SRC)
	@echo "Generating vim.vim.rc ..."
	@rm -f sanity_check.err generator.err
	@$(RUN_VIMPROG) -S gen_syntax_vim.vim $(TARGET)
	@if test -f sanity_check.err ; then \
		echo ; \
		echo "Sanity errors:" ; \
		cat sanity_check.err ; \
	fi
	@if test -f generator.err ; then \
		echo ; \
		echo "Generator errors:" ; \
		cat generator.err ; \
	fi
	@if test -f sanity_check.err || test -f generator.err ; then \
		exit 1 ; \
	fi
	@echo "done."
