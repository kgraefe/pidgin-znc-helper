EXTRA_DIST = \
	internal.h \
	messageparser.h \
	timeparser.h \
	zncconversation.h \
	pidgin_internals.h \
	prefs.h \
	Makefile.mingw

plugindir = $(libdir)/pidgin
DEFS = -DLOCALEDIR=\"$(localedir)\"

plugin_LTLIBRARIES = znchelper.la
znchelper_la_SOURCES = \
	znchelper.c \
	messageparser.c \
	zncconversation.c \
	pidgin_internals.c \
	timeparser.c \
	prefs.c
	
znchelper_la_LDFLAGS = -module -avoid-version

