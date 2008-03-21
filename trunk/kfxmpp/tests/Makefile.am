INCLUDES=-I$(top_srcdir) $(PACKAGE_CFLAGS)

noinst_PROGRAMS=test-event test-session test-stanza test-parser

test_event_SOURCES = \
		      test-event.c

test_session_SOURCES = \
		       test-session.c

test_stanza_SOURCES = \
		      test-stanza.c

test_parser_SOURCES =\
		     test-parser.c

LDADD = $(PACKAGE_LIBS) \
	$(top_builddir)/kfxmpp/libkfxmpp-1.la
