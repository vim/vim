# Project: gvimext
# Generates gvimext.dll with gcc.
# Can be used for Cygwin and MingW (MingW ignores -mno-cygwin)
#
# Originally, the DLL base address was fixed: -Wl,--image-base=0x1C000000
# Now it is allocated dymanically by the linker by evaluating all DLLs
# already loaded in memory. The binary image contains as well information
# for automatic pseudo-rebasing, if needed by the system. ALV 2004-02-29

# If cross-compiling set this to yes, else set it to no
CROSS = no
#CROSS = yes
# For the old MinGW 2.95 (the one you get e.g. with debian woody)
# set the following variable to yes and check if the executables are
# really named that way.
# If you have a newer MinGW or you are using cygwin set it to no and
# check also the executables
MINGWOLD = no

ifeq ($(CROSS),yes)
DEL = rm
ifeq ($(MINGWOLD),yes)
CXXFLAGS := -O2 -mno-cygwin -fvtable-thunks
else
CXXFLAGS := -O2 -mno-cygwin
endif
else
CXXFLAGS := -O2 -mno-cygwin
ifneq (sh.exe, $(SHELL))
DEL = rm
else
DEL = del
endif
endif
CXX := $(CROSS_COMPILE)g++
WINDRES := $(CROSS_COMPILE)windres
LIBS :=  -luuid
RES  := gvimext.res
DEFFILE = gvimext_ming.def
OBJ  := gvimext.o

DLL  := gvimext.dll

.PHONY: all all-before all-after clean clean-custom

all: all-before $(DLL) all-after

$(DLL): $(OBJ) $(RES) $(DEFFILE)
	$(CXX) -shared $(CXXFLAGS) -s -o $@ \
		-Wl,--enable-auto-image-base \
		-Wl,--enable-auto-import \
		-Wl,--whole-archive \
			$^ \
		-Wl,--no-whole-archive \
			$(LIBS)

gvimext.o: gvimext.cpp
	$(CXX) $(CXXFLAGS) -DFEAT_GETTEXT -c $? -o $@

$(RES): gvimext_ming.rc
	$(WINDRES) --input-format=rc --output-format=coff -DMING $? -o $@

clean: clean-custom
	-$(DEL)  $(OBJ) $(RES) $(DLL)

