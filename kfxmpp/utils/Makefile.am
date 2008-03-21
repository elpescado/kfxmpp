INCLUDES=-I$(top_srcdir) $(PACKAGE_CFLAGS)

bin_PROGRAMS=kfxmpp-send kfxmpp-client

kfxmpp_send_SOURCES = \
		      kfxmpp-send.c

kfxmpp_client_SOURCES = \
		      kfxmpp-client.c

LDADD = $(PACKAGE_LIBS) \
	$(top_builddir)/kfxmpp/libkfxmpp-1.la
