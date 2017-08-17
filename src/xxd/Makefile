ifeq ($(shell uname),Darwin)
  LIBTOOL ?= glibtool
else
  LIBTOOL ?= libtool
endif

ifneq ($(VERBOSE),1)
  LIBTOOL +=--quiet
endif

override CFLAGS +=-I.

ifeq ($(shell uname),SunOS)
  override CFLAGS +=-D__EXTENSIONS__ -D_XPG6 -D__XOPEN_OR_POSIX
endif

ifeq ($(DEBUG),1)
  override CFLAGS +=-ggdb -DDEBUG
endif

ifeq ($(PROFILE),1)
  override CFLAGS +=-pg
  override LDFLAGS+=-pg
endif

CFILES=libxxd.c
HFILES=xxd.h
OBJECTS=$(CFILES:.c=.lo)
LIBRARY=libxxd.la

BINFILES_SRC=xxd.c
BINFILES=$(BINFILES_SRC:.c=)

VERSION_MAJOR=0
VERSION_MINOR=0

VERSION_CURRENT=0
VERSION_REVISION=0
VERSION_AGE=0

VERSION=0

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include
MANDIR=$(PREFIX)/share/man
MAN3DIR=$(MANDIR)/man3

all: $(LIBRARY) $(BINFILES)

$(LIBRARY): $(OBJECTS)
	@echo LINK $@
	@$(LIBTOOL) --mode=link --tag=CC $(CC) -rpath $(LIBDIR) -version-info $(VERSION_CURRENT):$(VERSION_REVISION):$(VERSION_AGE) -o $@ $^ $(LDFLAGS)

%.lo: %.c
	@echo CC $<
	@$(LIBTOOL) --mode=compile --tag=CC $(CC) $(CFLAGS) -o $@ -c $<

%: %.c $(LIBRARY)
	@echo CC $<
	@$(LIBTOOL) --mode=link --tag=CC $(CC) $(CFLAGS) -o $@ $< -lxxd $(LDFLAGS)

.PHONY: clean
clean:
	$(LIBTOOL) --mode=clean rm -f $(OBJECTS)
	$(LIBTOOL) --mode=clean rm -f $(LIBRARY) $(BINFILES)

.PHONY: install
install: install-inc install-lib install-bin

install-inc:
	install -d $(DESTDIR)$(INCDIR)
	install -m644 $(HFILES) $(DESTDIR)$(INCDIR)
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	sed -e "s,@PREFIX@,$(PREFIX)," -e "s,@LIBDIR@,$(LIBDIR)," -e "s,@VERSION@,$(VERSION)," <xxd.pc.in >$(DESTDIR)$(LIBDIR)/pkgconfig/xxd.pc

install-lib: $(LIBRARY)
	install -d $(DESTDIR)$(LIBDIR)
	$(LIBTOOL) --mode=install install $(LIBRARY) $(DESTDIR)$(LIBDIR)/$(LIBRARY)
	$(LIBTOOL) --mode=finish $(DESTDIR)$(LIBDIR)

install-bin: $(BINFILES)
	install -d $(DESTDIR)$(BINDIR)
	$(LIBTOOL) --mode=install install $(BINFILES) $(DESTDIR)$(BINDIR)/

# DIST CUT

VERSION=$(VERSION_MAJOR).$(VERSION_MINOR)

DISTDIR=libxxd-$(VERSION)

distdir:
	mkdir __distdir
	cp LICENSE __distdir
	mkdir __distdir/src
	cp src/*.c src/*.h __distdir/src
	mkdir __distdir/src/encoding
	cp src/encoding/*.inc __distdir/src/encoding
	mkdir __distdir/include
	cp include/*.h __distdir/include
	mkdir __distdir/bin
	cp bin/*.c __distdir/bin
	mkdir __distdir/t
	cp t/*.test t/harness.c t/run-test.pl __distdir/t
	sed "s,@VERSION@,$(VERSION)," <xxd.pc.in >__distdir/xxd.pc.in
	sed "/^# DIST CUT/Q" <Makefile >__distdir/Makefile
	mv __distdir $(DISTDIR)

TARBALL=$(DISTDIR).tar.gz

dist: distdir
	tar -czf $(TARBALL) $(DISTDIR)
	rm -rf $(DISTDIR)

dist+bzr:
	$(MAKE) dist VERSION=$(VERSION)+bzr`bzr revno`

distdir+bzr:
	$(MAKE) distdir VERSION=$(VERSION)+bzr`bzr revno`
