EXTRA_DIST = \
	internal.h \
	Makefile.mingw

plugindir = $(libdir)/purple-2
DEFS = -DLOCALEDIR=\"$(localedir)\"

plugin_LTLIBRARIES = znchelper.la
znchelper_la_SOURCES = znchelper.c
	
znchelper_la_LDFLAGS = -module -avoid-version

