# List of files to generate
GEN_XML = protocols/ext-data-control-v1.xml \
	    protocols/primary-selection-unstable-v1.xml \
	    protocols/wlr-data-control-unstable-v1.xml \
	    protocols/xdg-shell.xml
GEN_SRC = ext-data-control-v1.c \
	    primary-selection-unstable-v1.c \
	    wlr-data-control-unstable-v1.c \
	    xdg-shell.c
GEN_INCLUDE = ext-data-control-v1.h \
	    primary-selection-unstable-v1.h \
	    wlr-data-control-unstable-v1.h \
	    xdg-shell.h

# Default target
all: $(GEN_SRC) $(GEN_INCLUDE)
	@if ! command -v wayland-scanner 2>&1 > /dev/null ; then \
		echo "wayland-scanner not available, cannot generate protocol files"; \
		false; \
	fi

ext-data-control-v1.c:
	wayland-scanner private-code protocols/ext-data-control-v1.xml $@
ext-data-control-v1.h:
	wayland-scanner client-header protocols/ext-data-control-v1.xml $@

wlr-data-control-unstable-v1.c:
	wayland-scanner private-code protocols/wlr-data-control-unstable-v1.xml $@
wlr-data-control-unstable-v1.h:
	wayland-scanner client-header protocols/wlr-data-control-unstable-v1.xml $@

primary-selection-unstable-v1.c:
	wayland-scanner private-code protocols/primary-selection-unstable-v1.xml $@
primary-selection-unstable-v1.h:
	wayland-scanner client-header protocols/primary-selection-unstable-v1.xml $@

xdg-shell.c:
	wayland-scanner private-code protocols/xdg-shell.xml $@
xdg-shell.h:
	wayland-scanner client-header protocols/xdg-shell.xml $@

$(GEN_SRC) $(GEN_INCLUDE): $(GEN_XML)

$(GEN_XML):

# Clean rule
clean:
	rm -f *.c *.h

.PHONY: all clean

# vim:ts=8:sw=8:tw=78
