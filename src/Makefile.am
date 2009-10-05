EXTRA_DIST = \
	internal.h \
	messageparser.h \
	timeparser.h \
	Makefile.mingw

plugindir = $(libdir)/pidgin
DEFS = -DLOCALEDIR=\"$(localedir)\"

plugin_LTLIBRARIES = znchelper.la
znchelper_la_SOURCES = \
	znchelper.c \
	messageparser.c \
	timeparser.c
	
znchelper_la_LDFLAGS = -module -avoid-version

