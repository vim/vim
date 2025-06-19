# List of files to generate
GEN_NAMES = ext-data-control-v1 \
	    primary-selection-unstable-v1 \
	    wlr-data-control-unstable-v1 \
	    xdg-shell
GEN_SRC = $(addsuffix .c,$(GEN_NAMES))
GEN_INCLUDE = $(addsuffix .h,$(GEN_NAMES))
GEN_XML = $(addprefix protocols/,$(addsuffix .xml,$(GEN_NAMES)))

SCANNER := $(shell command -v wayland-scanner 2> /dev/null)

# Default target
all: $(GEN_SRC) $(GEN_INCLUDE)
ifndef SCANNER
	$(error "wayland-scanner not available, cannot generate protocol files")
endif

%.c:
	wayland-scanner private-code $(addprefix protocols/,$(patsubst %.c,%.xml,$@)) $@

%.h:
	wayland-scanner client-header $(addprefix protocols/,$(patsubst %.h,%.xml,$@)) $@

$(GEN_SRC) $(GEN_INCLUDE): $(GEN_XML)

# Clean rule
clean:
	rm -f *.c *.h

.PHONY: all clean

# vim:ts=8:sw=8:tw=78
