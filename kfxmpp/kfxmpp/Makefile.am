INCLUDES=\
	 -I. \
	 -I.. \
	 $(PACKAGE_CFLAGS) \
	 -Wall
	 

lib_LTLIBRARIES = libkfxmpp-1.la

libkfxmpp_1_la_SOURCES = \
	core.c core.h \
	error.c error.h \
	event.c	event.h \
	kfxmpp.h \
	message.c message.h \
	sasl.c 	sasl.h \
	session.c session.h \
	stanza.c stanza.h \
	streamparser.c streamparser.h
	
libkfxmpp_1_la_LIBADD = \
	$(PACKAGE_LIBS)

libkfxmppincludedir = $(includedir)/kfxmpp-1.0/kfxmpp
