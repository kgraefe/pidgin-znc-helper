EXTRA_DIST = \
	internal.h \
	prefs.h \
	Makefile.mingw

plugindir = $(libdir)/pidgin
DEFS = -DLOCALEDIR=\"$(localedir)\"

plugin_LTLIBRARIES = znchelper.la
znchelper_la_SOURCES = \
	znchelper.c \
	prefs.c
	
znchelper_la_LDFLAGS = -module -avoid-version

