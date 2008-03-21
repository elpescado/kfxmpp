#include <kfxmpp/kfxmpp.h>
#include <kfxmpp/stanza.h>
#include <libxml/tree.h>

gint main (gint argc, gchar *argv[])
{
	xmlNodePtr node;
	xmlNodePtr child;
	KfxmppStanza *s;

	node = xmlNewNode (NULL, "message");
	xmlSetProp (node, "to", "test@example.com");
	xmlNewTextChild (node, NULL, "subject", "Test");
	xmlNewTextChild (node, NULL, "body", "This is test XML message");

	s = kfxmpp_stanza_new_from_xml (node);
	g_print ("<%s>,%s", s->node->name, kfxmpp_stanza_to_string (s));
	return 0;
}
