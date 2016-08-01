appdatadir = $(datadir)/appdata

appdata_in_files = \
	pidgin-znc-helper.metainfo.xml.in
nodist_appdata_DATA = $(appdata_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

EXTRA_DIST = $(appdata_in_files)

CLEANFILES = \
	$(nodist_appdata_DATA) \
	$(NULL)
