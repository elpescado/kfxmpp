#include <kfxmpp/session.h>

void connected (KfxmppSession *session, KfxmppError error, gpointer data);

gint main (gint argc, gchar *argv[])
{
	KfxmppSession *sess;
	GMainLoop *loop;
	
	loop = g_main_loop_new (NULL, FALSE);

	kfxmpp_init ();

	sess = kfxmpp_session_new (NULL);
	kfxmpp_session_set_server (sess, argv[2]);
	kfxmpp_session_set_username (sess, argv[1]);
	kfxmpp_session_set_password (sess, argv[3]);
	kfxmpp_session_set_resource (sess, "kfxmpp");
	
//	kfxmpp_session_set_protocol (sess, KFXMPP_PROTOCOL_JABBER);
	
	kfxmpp_session_connect (sess, connected, loop, NULL);
	
	g_print ("Running main loop\n");
	g_main_loop_run (loop);

	kfxmpp_deinit ();
	return 0;
}


void connected (KfxmppSession *session, KfxmppError error, gpointer data)
{
	GMainLoop *loop = data;
	
	if (error == KFXMPP_ERROR_NONE) {
		g_print ("Connected\n");

//		g_print ("And now... disconnecting:-)\n");
//		kfxmpp_session_disconnect (session, NULL);		
	} else {
		g_print ("Error:-/\n");
		g_main_loop_quit (loop);
	}
}
