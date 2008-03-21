/*
 * simple kfxmpp application
 */

#include <kfxmpp/session.h>
#include <kfxmpp/message.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#define BUFFER_SIZE 1024


typedef struct {
	gchar *username;		// Jabber Username
	gchar *server;			// Jabber server
	gchar *pass;			// Jabber password
	gchar *connect_to;		// Jabber server to connect to

	const gchar *to;		// Message recipient
	const gchar *subject;		// Message subject
	gchar *body;			// Message body
	gboolean chat;			// Send as chat
	
	const gchar *configfile;	// Configuration file
	GMainLoop *loop;		// Main loop pointer

	gint	errorcode;		// Error code to return by app
} ConfigData;


void print_usage (void)
{
	g_print ("usage: kfxmpp-send [options] jid\n");
}


void print_help (void)
{
	print_usage ();
	g_print("Options are:\n"
		" -c, --config=config	Use file config as configuration file\n"
		" -C, --chat		Send message as chat message\n"
		" -h, --help		Print this message\n"
		" -s, --subject=text	Use text as subject for this message\n");
}


static struct option options[] = {
	{"config",	required_argument,	NULL, 'c'},
	{"chat",	no_argument,		NULL, 'C'},
	{"help",	no_argument,		NULL, 'h'},
	{"subject",	required_argument,	NULL, 's'},
	{NULL,		0, 			NULL, '\0'}
};

/* Parse command line options */
void parse_cmdline (gint argc, gchar *argv[], ConfigData *config)
{
	gint c;
	extern char *optarg;
	while ((c = getopt_long (argc, argv, "c:Chs:", options, NULL)) != -1) {
		switch (c) {
			case 'c':	config->configfile = optarg;
					break;
			case 'C':	config->chat = TRUE;
					break;
			case 'h':	print_help ();
					exit (EXIT_SUCCESS);
					break;
			case 's':	config->subject = optarg;
					break;
		}
	}
}


/*
 *  Read configuration file
 * filename - config file name
 * config - structure that will be filled with data read from config file
 */
gboolean read_config (const gchar *filename, ConfigData *config)
{
	FILE *f;
	gchar buffer[BUFFER_SIZE];

	f = fopen (filename, "r");
	if (f == NULL)
		return FALSE;

	while (fgets (buffer, BUFFER_SIZE, f)) {
		if (*buffer == '#' || *buffer == '\n')
			continue;	// skip comments and empty lines

		gchar *key = buffer;
		gchar *value = strchr (key, '=');
		if (value) {
			*value++ = '\0';

			gchar *endl = strchr (value, '\n');
			if (endl) *endl = '\0';

			if (strcmp (key, "username") == 0)
				config->username = g_strdup (value);
			else if (strcmp (key, "server") == 0)
				config->server = g_strdup (value);
			else if (strcmp (key, "password") == 0)
				config->pass = g_strdup (value);
			else if (strcmp (key, "connect_to") == 0)
				config->connect_to = g_strdup (value);
		}
	}

	if (config->username == NULL || config->server == NULL)
		return FALSE;
	
	fclose (f);
	return TRUE;
}


void connected (KfxmppSession *session, KfxmppError error, gpointer data);
/*
 * Estabilish connection to server and run main loop
 */
gint do_connect (ConfigData *config)
{
	KfxmppSession *sess;
	GMainLoop *loop;
	
	config->loop = g_main_loop_new (NULL, FALSE);

	kfxmpp_init ();

	sess = kfxmpp_session_new (NULL);
	kfxmpp_session_set_server (sess, config->server);
	kfxmpp_session_set_username (sess, config->username);
	kfxmpp_session_set_password (sess, config->pass);
	if (config->connect_to)
		kfxmpp_session_set_host_address (sess, config->connect_to);
	kfxmpp_session_set_resource (sess, "kfxmpp");
	
//	kfxmpp_session_set_protocol (sess, KFXMPP_PROTOCOL_JABBER);
	
	kfxmpp_session_connect (sess, connected, config, NULL);
	
	g_main_loop_run (config->loop);

	kfxmpp_deinit ();
	return 0;
}




/* Callback called when connection is estabilished */
void connected (KfxmppSession *session, KfxmppError error, gpointer data)
{
	ConfigData *config = data;
	GMainLoop *loop = config->loop;
	
	if (error == KFXMPP_ERROR_NONE) {
		KfxmppMessage *msg;

		msg = kfxmpp_message_new (config->to);
		if (config->subject)
			kfxmpp_message_set_subject (msg, config->subject);
		kfxmpp_message_set_body (msg, config->body);

		if (config->chat)
			kfxmpp_message_set_type (msg, KFXMPP_MESSAGE_TYPE_CHAT);

		kfxmpp_message_send (msg, session);
		kfxmpp_message_unref (msg);
		
		kfxmpp_session_disconnect (session, NULL);		
	} else {
		g_print ("kfxmpp-send: Cannot connect to Jabber server\n");
		config->errorcode = EXIT_FAILURE;
	}
	g_main_loop_quit (loop);
}


/* Read message from standard output */
gchar *read_message (void)
{
	GString *str;
	gchar buffer[BUFFER_SIZE];

	str = g_string_new ("");
	while (fgets (buffer, BUFFER_SIZE, stdin)) {
		g_string_append (str, buffer);
	}
	return g_string_free (str, FALSE);
}


int main (int argc, char *argv[])
{
	extern int optind;
	ConfigData config = {NULL, NULL, NULL};
	
	parse_cmdline (argc, argv, &config);
	
	const gchar *conf = config.configfile?config.configfile:"config";
	if (! read_config (conf, &config)) {
		g_printerr ("kfxmpp-send: Reading config file '%s' failed\n", conf);
		return EXIT_FAILURE;
	}

	config.errorcode = EXIT_SUCCESS;


	/* Get recipient address */
	config.to = argv[optind];
	if (config.to == NULL) {
		print_usage ();
		exit (EXIT_SUCCESS);
	}
	
	/* Read message */
	g_print ("Enter message body. Press Ctrl+D when finished.\n");
	config.body = read_message ();


	if (config.pass == NULL) {
		/* Password not specified. Read it from user */
		config.pass = g_strdup (getpass ("Enter Jabber password: "));
	}
	
	do_connect (&config);

	g_free (config.username); g_free (config.server); g_free (config.pass);
	g_free (config.connect_to);
	g_free (config.body);

	return config.errorcode;
}
