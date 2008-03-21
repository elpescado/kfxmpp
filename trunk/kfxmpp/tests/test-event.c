/*
 * kfxmpp event test
 * -----------------
 *
 * output:
Calling event
Callback #2
Callback #3 (*)
Calling event
Callback #2
Callback #1
 */

#include <glib.h>
#include <kfxmpp/event.h>

gboolean handler (KfxmppEventHandler *h, gpointer source, gpointer event, gpointer data);
gboolean handler2 (KfxmppEventHandler *h, gpointer source, gpointer event, gpointer data);

gint main (gint argc, gchar *argv[])
{
	KfxmppEvent *e;
	KfxmppEventHandler *h1, *h2, *h3;

	e = kfxmpp_event_new (NULL);
	h1 = kfxmpp_event_handler_new (handler, "Callback #1", NULL);
	h2 = kfxmpp_event_handler_new (handler, "Callback #2", NULL);
	h3 = kfxmpp_event_handler_new (handler2, "Callback #3", NULL);

	kfxmpp_event_add_handler (e, h1, 10);
	kfxmpp_event_add_handler (e, h2, 50);
	kfxmpp_event_add_handler (e, h3, 30);
	
	g_print ("Calling event\n");
	kfxmpp_event_trigger (e, NULL);

	kfxmpp_event_remove_handler (e, h3);

	g_print ("Calling event\n");
	kfxmpp_event_trigger (e, NULL);

	return 0;
}

gboolean handler (KfxmppEventHandler *h, gpointer source, gpointer event, gpointer data)
{
	const gchar *tct = data;
	g_print ("%s\n", tct);
	return FALSE;
}


gboolean handler2 (KfxmppEventHandler *h, gpointer source, gpointer event, gpointer data)
{
	const gchar *tct = data;
	g_print ("%s (*)\n", tct);
	return TRUE;
}
