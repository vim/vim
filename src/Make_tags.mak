#
# tags
#

include Make_all.mak
-include auto/config.mk

# Override with `make TAGPRG='...' tags`
TAGPRG ?= ctags -I INIT+,INIT2+,INIT3+,INIT4+,INIT5+ --fields=+S -f tags

notags:
	-rm -f tags

# You can ignore error messages for missing files.
tags TAGS: notags
	$(TAGPRG) $(TAGS_FILES)
