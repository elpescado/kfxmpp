#include "kfxmpp.h"
#include <stdio.h>

#define BUFFER_SIZE 10

void onXml (KfxmppStreamParser *p, xmlNodePtr node, gpointer data);

gint main (gint argc, gchar *argv[])
{
	KfxmppStreamParser *p;
	FILE *f;
	gssize bytes_read;
	gchar buffer[BUFFER_SIZE];

	p = kfxmpp_stream_parser_new (onXml, NULL);

	const gchar *file = argc > 1 ? argv[1] : "stream.xml";
	f = fopen (file, "r");
	if (f == NULL) {
		g_printerr ("Unable to open file %s\n", file);
		return 1;
	}

	while ((bytes_read = fread (buffer, 1, BUFFER_SIZE, f)) > 0) {
		kfxmpp_stream_parser_feed (p, buffer, bytes_read);
	}
	fclose (f);

	return 0;
}


void onXml (KfxmppStreamParser *p, xmlNodePtr node, gpointer data)
{
	g_print ("Got <%s>\n", node->name);
}
