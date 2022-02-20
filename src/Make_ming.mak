#
# Makefile for VIM on Win32, using MinGW
#
# Also read INSTALLpc.txt!
#
# The old Make_ming.mak (maintained by Ron Aaron et al.) was merged into
# Make_cyg_ming.mak.
# This file contains MinGW specific settings. Common settings are contained
# in Make_cyg_ming.mak.
#
# Last updated by Ken Takata.
# Last Change: 2014 Oct 21


# uncomment 'PERL' if you want a perl-enabled version
#PERL=c:/perl

# uncomment 'LUA' if you want a Lua-enabled version
#LUA=c:/lua

# uncomment 'MZSCHEME' if you want a MzScheme-enabled version
#MZSCHEME=d:/plt

# uncomment 'PYTHON3' if you want a python3-enabled version
#PYTHON3=c:/python31

# uncomment 'TCL' if you want a Tcl-enabled version
#TCL=c:/tcl

# uncomment 'RUBY' if you want a Ruby-enabled version
#RUBY=c:/ruby


# Do not change this.
UNDER_CYGWIN = no
include Make_cyg_ming.mak

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=0:
