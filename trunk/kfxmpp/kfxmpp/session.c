/*
 * kfxmpp
 * ------
 *
 * Copyright (C) 2003-2004 Przemysław Sitek <psitek@rams.pl> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/** \file session.h */

#include "kfxmpp.h"
#include "session.h"
#include "streamparser.h"
#include "stanza.h"
#include "event.h"
#include "sasl.h"
#include "message.h"

#include <string.h>
#include <gnet.h>

#ifdef HAVE_GNUTLS
#  include <gnutls/gnutls.h>
#endif

/* Starttls command */
#define KFXMPP_SESSION_TLS "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>"

/* Buffer size */
#define BUFFER_SIZE 1024

/* Id scheme */
#define RESPONSE_STRING "msg%d"

/* Default timeout length */
#define DEFAULT_TIMEOUT 60


/**
 * \brief Object representing connection to a XMPP server
 *
 *    This is central object in \b kfxmpp library
 **/
struct _KfxmppSession {
	/* General information */
	KfxmppSessionState state;	/**< Object state				*/
	gint	ref_count;		/**< Number of references to this object	*/
	KfxmppProtocol protocol;	/**< Protocol version to be used		*/
	
	/* Account information */
	gchar *username;	/**< Username part of JID	*/
	gchar *server;		/**< Server part of JID		*/
	gchar *password;	/**< Password			*/
	gchar *resource;	/**< Resource name		*/
	gchar *host_address;	/**< Host name to connect to, may be different than \b server */
	gint port;		/**< Port number		*/
	gint priority;		/**< Priority			*/
	KfxmppTlsUsagePolicy use_tls;	/**< Whether try to use SSL or not */

	/* Network stuff */
	GTcpSocket	*socket;		/**< Connection socket	*/
	GTcpSocketConnectAsyncID connect_id;	/**< ID of connection attempt */
	GIOChannel	*io;			/**< I/O stream from socket	*/

	/* Event watching stuff */
	GMainContext *context;	/**< Main loop context */
	GSource	*sources[4];	/**< Source condition */
	

	/* Connect callback */
	KfxmppSessionConnectCallback callback;	/**< Callback called when connection is estabilished */
	gpointer	callback_data;		/**< Data passed to above callback */

	/* Disconnect callback */
	KfxmppSessionDisconnectCallback disconnect_callback;
	gpointer disconnect_data;
	
	KfxmppStreamParser *parser;	/**< XML parser */

	/* TLS stuff */
	gboolean	secure;			/**< Whether link is secured	*/
#ifdef HAVE_GNUTLS
	gnutls_session_t gnutls;		/**< gnutls session object	*/
	gnutls_certificate_credentials_t cred;	/**< certificate info	*/
#endif

	/* Event handling stuff */
	KfxmppEvent *events[KFXMPP_N_EVENT_TYPES];	/**< Events emitted by session */
	GHashTable *response_ids;			/**< Event handlers indexed by their IDs */

	/* Timeouts */
	gint timeout;					/**< Timeout length, in seconds */
	gint connect_timeout_id;			/**< ID of connect callback */
	gint ping_pong_id;				/**< ID of ping-pong timeout */
};


/***********************************************************************
 *
 * Static function prototypes
 *
 */

static gboolean kfxmpp_session_io_event (GIOChannel *source, GIOCondition condition, gpointer data);
static void kfxmpp_session_connect_ok (KfxmppSession *self);
static void kfxmpp_session_connect_failed (KfxmppSession *self, KfxmppError error);
static void kfxmpp_session_connected (GTcpSocket *socket, GTcpSocketConnectAsyncStatus status, gpointer data);
static void kfxmpp_session_close (KfxmppSession *self);
static void kfxmpp_session_open_stream (KfxmppSession *self);
#ifdef HAVE_GNUTLS
static gssize kfxmpp_session_tls_send (gnutls_transport_ptr_t p, const void*data, gsize size);
static gssize kfxmpp_session_tls_recv (gnutls_transport_ptr_t p, void* data, gsize size);
#endif
static void kfxmpp_session_got_stream (KfxmppStreamParser *parser, gint version, const gchar *id, gpointer data);
static void kfxmpp_session_got_xml (KfxmppStreamParser *parser, xmlNodePtr node, gpointer data);
static gboolean kfxmpp_session_xml_event (KfxmppEventHandler *handler, KfxmppSession *self, KfxmppStanza *stazna, gpointer data);
static void kfxmpp_session_bind_resource (KfxmppSession *self);
static gboolean kfxmpp_session_bind_resource_response (KfxmppEventHandler *handler, gpointer source,
					gpointer event, gpointer data);

/* Non SASL authentication */
static void kfxmpp_session_iq_auth (KfxmppSession *self);
static gboolean kfxmpp_session_iq_auth_response (KfxmppEventHandler *handler, gpointer source,
					gpointer event, gpointer data);
static gboolean kfxmpp_session_iq_auth_response2 (KfxmppEventHandler *handler, gpointer source,
					gpointer event, gpointer data);


/**
 * \brief Create a new KfxmppSession
 **/
KfxmppSession *kfxmpp_session_new (const gchar *server)
{
	KfxmppSession *self;
	gint i;
	KfxmppEventHandler *handler;

	self = g_new0 (KfxmppSession, 1);

	self->server = server ? g_strdup (server) : NULL;
	
	self->state = KFXMPP_SESSION_STATE_CLOSED;
	self->ref_count = 1;
	self->protocol = KFXMPP_PROTOCOL_AUTO;
	
	self->port = KFXMPP_DEFAULT_PORT;
	self->use_tls = KFXMPP_TLS_POLICY_IF_AVAILABLE;

	self->context = g_main_context_default ();

	/* Setup parser */
	self->parser = kfxmpp_stream_parser_new (kfxmpp_session_got_xml, self);	
	kfxmpp_stream_parser_set_stream_callback (self->parser, kfxmpp_session_got_stream);

	/* Setup events */
	for (i = 0; i < KFXMPP_N_EVENT_TYPES; i++) {
		self->events[i] = kfxmpp_event_new (self);
	}
	handler = kfxmpp_event_handler_new ((KfxmppEventHandlerFunc) kfxmpp_session_xml_event, NULL, NULL);
	kfxmpp_event_add_handler (self->events[KFXMPP_EVENT_TYPE_XML], handler, KFXMPP_EVENT_HANDLER_PRIORITY_KFXMPP);

	self->response_ids = g_hash_table_new_full (g_str_hash, g_str_equal,
			g_free, (GDestroyNotify) kfxmpp_event_handler_unref);
	

	/* Timeouts */
	self->timeout = DEFAULT_TIMEOUT;
	self->connect_timeout_id = 0;
	self->ping_pong_id = 0;
	
	return self;
}


/**
 * \brief Free the session object and all associated resources
 * \param self A session
 **/
void kfxmpp_session_free (KfxmppSession *self)
{
	gint i;

	g_return_if_fail (self);
	
	g_free (self->username);
	g_free (self->server);
	g_free (self->password);
	g_free (self->resource);
	g_free (self->host_address);

	gnet_tcp_socket_unref (self->socket);
	kfxmpp_stream_parser_unref (self->parser);

#ifdef HAVE_GNUTLS	
	gnutls_deinit (self->gnutls);
	gnutls_certificate_free_credentials (self->cred);
#endif

	/* Free events */
	for (i = 0; i < KFXMPP_N_EVENT_TYPES; i++) {
		kfxmpp_event_unref (self->events[i]);
	}
	g_hash_table_destroy (self->response_ids);
	
	g_free (self);
}


/**
 * \brief Add a reference to KfxmppSession
 **/
KfxmppSession* kfxmpp_session_ref (KfxmppSession *self)
{
        g_return_val_if_fail (self, NULL);
        self->ref_count++;
        return self;
}


/**
 * \brief Remove a reference from KfxmppSession
 *
 * Object will be deleted when reference count reaches 0
 **/
void kfxmpp_session_unref (KfxmppSession *self)
{
        g_return_if_fail (self);
        self->ref_count--;
        if (self->ref_count == 0)
                kfxmpp_session_free (self);
}


/***********************************************************************
 *
 * Public access functions
 *
 */

/**
 * \brief Set username
 * \param self A session
 * \param username A username used in this session
 **/
void kfxmpp_session_set_username (KfxmppSession *self, const gchar *username)
{
	g_return_if_fail (self);

	if (self->username)
		g_free (self->username);
	self->username = username ? g_strdup (username) : NULL;
}


/**
 * \brief Get username used to authenticate this session
 * \param self A session
 * \return Username associated with this session
 **/
const gchar *kfxmpp_session_get_username (KfxmppSession *self)
{
	g_return_val_if_fail (self, NULL);

	return self->username;
}


/**
 * \brief Set XMPP/Jabber server address
 * \param self A session
 * \param server XMPP/Jabber server address
 **/
void kfxmpp_session_set_server (KfxmppSession *self, const gchar *server)
{
	g_return_if_fail (self);

	g_free (self->server);
	self->server = server ? g_strdup (server) : NULL;
}


/**
 * \brief Get server
 * \param self A session
 * \return Server address
 **/
const gchar *kfxmpp_session_get_server (KfxmppSession *self)
{
	g_return_val_if_fail (self, NULL);

	return self->server;
}


/**
 * \brief Set password for this connection
 * \param self A session
 * \param pass A password that will be used to authenticate this session
 **/
void kfxmpp_session_set_password (KfxmppSession *self, const gchar *pass)
{
	g_return_if_fail (self);

	g_free (self->password);
	self->password = pass ? g_strdup (pass) : NULL;
}


/**
 * \brief Get password
 * \param self A session
 * \return A password
 **/
const gchar *kfxmpp_session_get_password (KfxmppSession *self)
{
	g_return_val_if_fail (self, NULL);

	return self->password;
}


/**
 * \brief Set resource name used for this session
 * \param self A session
 * \param resource Resource name
 **/
void kfxmpp_session_set_resource (KfxmppSession *self, const gchar *resource)
{
	g_return_if_fail (self);

	g_free (self->resource);
	self->resource = resource ? g_strdup (resource) : NULL;
}


/**
 * \brief Get resource name
 * \param self A session
 * \return A resource name
 **/
const gchar *kfxmpp_session_get_resource (KfxmppSession *self)
{
	g_return_val_if_fail (self, NULL);

	return self->resource;
}


/**
 * \brief Set host address to connect to
 * \param self A session
 * \param host_address host address
 *
 * In some cases, it is needed to connect to different machine (e.g. 'jabber.exmple.com')
 * than the one that is in user JID (e.g. 'someone@exmple.com'). This function sets that 
 * server, whereas \a kfxmpp_session_set_server sets the server part of JID. However,
 * in most cases, both names are the same, so you need not call this function.
 **/
void kfxmpp_session_set_host_address (KfxmppSession *self, const gchar *host_address)
{
	g_return_if_fail (self);

	g_free (self->host_address);
	self->host_address = host_address ? g_strdup (host_address) : NULL;
}


/**
 * \brief Get host address
 * \param self A session
 * \return Host address, or NULL
 **/
const gchar *kfxmpp_session_get_host_address (KfxmppSession *self)
{
	g_return_val_if_fail (self, NULL);

	return self->host_address;
}


/**
 * \brief Set port used to this connection. Default is 5222.
 * \param self A session
 * \param port Port number
 **/
void kfxmpp_session_set_port (KfxmppSession *self, int port)
{
	g_return_if_fail (self);

	self->port = port;
}


/**
 * \brief Get port number.
 * \param self A session
 * \return Port number
 **/
int kfxmpp_session_get_port (KfxmppSession *self)
{
	g_return_val_if_fail (self, 0);

	return self->port;
}


/**
 * \brief Set priority for this resource
 * \param self A session
 * \param priority A priority
 **/
void kfxmpp_session_set_priority (KfxmppSession *self, int priority)
{
	g_return_if_fail (self);

	self->priority = priority;
}


/**
 * \brief Get priority of this resource
 * \param self A session
 * \return priority
 **/
int kfxmpp_session_get_priority (KfxmppSession *self)
{
	g_return_val_if_fail (self, 0);

	return self->priority;
}


void kfxmpp_session_set_use_tls (KfxmppSession *self, KfxmppTlsUsagePolicy use_tls)
{
	g_return_if_fail (self);

	self->use_tls = use_tls;
}


KfxmppTlsUsagePolicy kfxmpp_session_get_use_tls (KfxmppSession *self)
{
	g_return_val_if_fail (self, FALSE);

	return self->use_tls;
}


/**
 * \brief Set protocol version to use
 * \param self A session
 * \param proto A protocol version
 *
 * There are some minor differences between XMPP 1.0 and legacy Jabber protocol.
 * KFXMPP_PROTOCOL_AUTO, the default setting, should be suitable for most needs.
 **/
void kfxmpp_session_set_protocol (KfxmppSession *self, KfxmppProtocol proto)
{
	g_return_if_fail (self);

	self->protocol = proto;
}


/**
 * \brief Get protocol version used it this session
 * \param self A session
 * \return Protocol version
 **/
KfxmppProtocol kfxmpp_session_get_protocol (KfxmppSession *self)
{
	g_return_val_if_fail (self, -1);

	return self->protocol;
}


/**
 * \brief Set timeout length
 * \param self A session
 * \param timeout Timeout length (in seconds)
 **/
void kfxmpp_session_set_timeout (KfxmppSession *self, gint timeout)
{
	g_return_if_fail (self);

	self->timeout = timeout;
}


/**
 * \brief Get timeout length version used it this session
 * \param self A session
 * \return Timeout (in seconds)
 **/
KfxmppProtocol kfxmpp_session_get_timeout (KfxmppSession *self)
{
	g_return_val_if_fail (self, -1);

	return self->timeout;
}



/***********************************************************************
 * 
 * Network Input/Output
 *
 */

/**
 * \brief Read data from remote host
 * \param self A session
 * \param buffer Location of buffer to store read data
 * \param size Size of buffer
 * \param error Locatiopn to store error information (may be NULL)
 * \return Number of bytes read. Negative value means error.
 **/
gssize kfxmpp_session_read (KfxmppSession *self, gchar *buffer, gssize size, GError **error)
{
	gssize bytes_read;

	g_return_val_if_fail (self, -999);
	
#ifdef HAVE_GNUTLS
	if (self->secure) {
		/* Write through gnutls */
		bytes_read = gnutls_record_recv (self->gnutls, buffer, size);
	} else
#endif
	{
		GIOStatus status;
		status = g_io_channel_read_chars
				(self->io, buffer, size, &bytes_read, NULL);
		
		if (status != G_IO_STATUS_NORMAL) {
			/* Some error */
			/* TODO: check these errors */
			return -1;
		}
	}

#ifdef DEBUG
	extern gboolean debug_net;
	if (debug_net) {
		g_printerr ("\nrecv\n----\n");
		fwrite (buffer, 1, bytes_read, stderr);
		g_printerr ("\n");
	}
#endif

	return bytes_read;
}


/**
 * \brief Send an XML stanza to remote host
 * \param self A session
 * \param stanza A xml stanza
 * \param error Location to store error information (may be NULL)
 * \return Number of bytes written. Negative value means an error.
 **/
gssize kfxmpp_session_send (KfxmppSession *self, KfxmppStanza *stanza, GError **error)
{
	gchar *xml;
	gssize ret;
	
	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (stanza, -1);

	xml = kfxmpp_stanza_to_string (stanza);
	ret = kfxmpp_session_send_raw (self, xml, strlen (xml), NULL);
//	kfxmpp_log ("send\n----\n%s\n", xml);
	g_free (xml);
	return ret;
}


/**
 * \brief Send an XML stanza to remote host and call a handler when a response is received
 * \param self A session
 * \param stanza A xml stanza
 * \param handler An event handler to be called when response is received
 * \param error Locatiopn to store error information (may be NULL)
 * \return an ID. Response handler can be canceled with kfxmpp_session_cancel_response
 **/
gint kfxmpp_session_send_await_response (KfxmppSession *self, KfxmppStanza *stanza, KfxmppEventHandler *handler, GError **error)
{
	static gint id = 0;
	gchar idstr[10];	/* Should be enough */

	g_return_val_if_fail (self, -1);
	g_return_val_if_fail (stanza, -1);
	g_return_val_if_fail (handler, -1);

	snprintf (idstr, 10, RESPONSE_STRING, ++id);

	/* Set the id property */
	xmlSetProp (stanza->node, BAD_CAST "id", BAD_CAST idstr);

	/* Send stanza */
	kfxmpp_session_send (self, stanza, NULL);

	/* Register handler */
	kfxmpp_session_await_response (self, idstr, handler);

	return id;
}


/**
 * \brief Send raw character data to server
 * \param self A session
 * \param buffer Character data to be sent
 * \param size Size of data to be transmitted, or -1 if data is null-terminated
 * \param error Locatiopn to store error information (may be NULL)
 * \return Number of bytes sent. Negative value means an error
 **/
gssize kfxmpp_session_send_raw (KfxmppSession *self, const gchar *buffer, gssize size, GError **error)
{
	gssize bytes_written;
	
	g_return_val_if_fail (self, -9);	/* Magic numbers... FIXME */

	if (size == -1)
		size = strlen (buffer);
	
#ifdef DEBUG
	extern gboolean debug_net;
	if (debug_net) {
		g_printerr ("\nsend\n----\n");
		fwrite (buffer, 1, size, stderr);
		g_printerr ("\n");
	}
#endif
	
#ifdef HAVE_GNUTLS
	if (self->secure) {
		bytes_written = gnutls_record_send (self->gnutls, buffer, size);
	} else
#endif
	{
		GIOStatus status;
		status = g_io_channel_write_chars
			(self->io, buffer, size, &bytes_written, NULL);

		if (status != G_IO_STATUS_NORMAL) {
			/* Error */
			return -1;
		}
	}
	return bytes_written;
}


/**
 * \brief Function called when connection was broken
 * \param self KfxmppSession
 * \param condition Condition that caused connection to close
 **/
static void kfxmpp_session_disconnected (KfxmppSession *self, GIOCondition condition)
{
	kfxmpp_log ("Disconnected\n");

	/* Close underlying socket */
	gnet_tcp_socket_delete (self->socket);
	self->socket = NULL;

	/* Clean up session */
	kfxmpp_session_close (self);

	/* Notify user of broken connection */
	if (self->disconnect_callback) {
		self->disconnect_callback (self, KFXMPP_SESSION_DISCONNECT_STATUS_REMOTE_HOST, self->disconnect_data);
	}
}


/**
 * \brief Callback called when data from remote host is received
 * \param I/O channel on which event occured
 * \param condition A condition that was satisfied
 * \param data A Kfxmpp session
 * \return FALSE if this callback should be removed
 **/
static gboolean kfxmpp_session_io_event (GIOChannel *source, GIOCondition condition, gpointer data)
{
	KfxmppSession *self = data;
	
	if (condition & G_IO_IN) {
		gchar buffer[BUFFER_SIZE];
		gssize bytes_read;

		bytes_read = kfxmpp_session_read (self, buffer, BUFFER_SIZE, NULL);
		if (bytes_read > 0) {
			kfxmpp_stream_parser_feed (self->parser, buffer, bytes_read);
		}
	}
	if (condition & G_IO_HUP) {
		/* Connection hung up... handle it */
		/* TODO */

		kfxmpp_log ("G_IO_HUP\n");
		kfxmpp_session_disconnected (self, condition);
	}
	if (condition & G_IO_NVAL) {
		/* Performing action on a file description that is not open... */

		kfxmpp_log ("G_IO_NVAL\n");
		kfxmpp_session_disconnected (self, condition);
	}
	if (condition & G_IO_ERR) {
		/* Some error */

		kfxmpp_log ("G_IO_ERR\n");
		kfxmpp_session_disconnected (self, condition);
	}

	return TRUE;
}


/***********************************************************************
 *
 * Network stuff
 * 
 */


/**
 * \brief Callback called when connect timeout expires
 * \param data A KfxmppSession
 * \return FALSE
 **/
static gboolean kfxmpp_session_connect_timeout                  (gpointer data)
{
	KfxmppSession *self = data;

	kfxmpp_log ("Timeout expired...\n");
	
	/* Unset this timeout */
	self->connect_timeout_id = 0;

	/* Cancel connection */
	if (self->state == KFXMPP_SESSION_STATE_CONNECTING) {
		gnet_tcp_socket_connect_async_cancel (self->connect_id);
		self->state = KFXMPP_SESSION_STATE_CLOSED;
	}	
	kfxmpp_session_connect_failed (self, KFXMPP_ERROR_TIMEOUT);

	return FALSE;
}


/**
 * \brief Connect to remote host
 * \param self A session
 * \param callback Callback to be called when connection is estabilished
 * or when error is encountered. This callback will be called at most once per call to connect.
 * \param data User data passed to callback
 * \param error Location to store error information (may be NULL)
 * \return TRUE on success, FALSE on failure
 **/
gboolean kfxmpp_session_connect (KfxmppSession *self, KfxmppSessionConnectCallback callback, gpointer data, GError **error)
{
	const gchar *addr;
	g_return_val_if_fail (self, FALSE);

	if (self->state != KFXMPP_SESSION_STATE_CLOSED) {
		/* Error, connection is already opened */
		g_set_error (error, KFXMPP_ERROR,
				KFXMPP_ERROR_SESSION_ALREADY_OPEN,
				"Session is already connected");
		return FALSE;
	}

	addr = self->host_address ? self->host_address : self->server;

	kfxmpp_log ("Connecting to %s:%d\n", addr, self->port);
	
	self->connect_id = gnet_tcp_socket_connect_async (addr,
			self->port, kfxmpp_session_connected, self);
	
	self->callback = callback;
	self->callback_data = data;

	/* Setup a timeout */
	if (self->timeout > 0) {
		self->connect_timeout_id = g_timeout_add (self->timeout*1000,
				kfxmpp_session_connect_timeout,
				self);
	}

	self->state = KFXMPP_SESSION_STATE_CONNECTING;
	return TRUE;
}


/**
 * \brief Cancel connect to remote host
 * \param self A session
 *
 * Cancels a connection process to remote host. If session if not in the
 * state of connecting (i.e. session->state != KFXMPP_SESSION_STATE_CONNECTING),
 * this function does nothing.
 **/
void kfxmpp_session_cancel_connect (KfxmppSession *self)
{
	g_return_if_fail (self);

	if (self->state == KFXMPP_SESSION_STATE_CONNECTING) {
		gnet_tcp_socket_connect_async_cancel (self->connect_id);
		self->state = KFXMPP_SESSION_STATE_CLOSED;
	}	
}

static gboolean kfxmpp_session_ping_pong (gpointer data)
{
	KfxmppSession *self = data;
	GError *error = NULL;

	kfxmpp_log ("ping-pong\n");
	kfxmpp_session_send_raw (self, " ", 1, &error);
	if (error) {
		g_error_free (error);
	}
	return TRUE;
}


/**
 * \brief Function called when connection is opened
 * \param self A session
 **/
static void kfxmpp_session_connect_ok (KfxmppSession *self)
{
	/* Cancel connect timeout */
	if (self->connect_timeout_id != 0) {
		g_source_remove (self->connect_timeout_id);
		self->connect_timeout_id = 0;
	}
	
	self->state = KFXMPP_SESSION_STATE_OPEN;
	if (self->callback) {
		/* Inform user of succesful operation by calling callback */

		self->callback (self, KFXMPP_ERROR_NONE, self->callback_data);
	}

	/* Setup a ping pong event */
	self->ping_pong_id = g_timeout_add (5*1000,
			kfxmpp_session_ping_pong,
			self);
}


/**
 * \brief Function called when an error was encountered while trying to estabilish connection
 * \param self A session
 * \param error An error that caused failure
 **/
static void kfxmpp_session_connect_failed (KfxmppSession *self, KfxmppError error)
{
	/* Close underlying socket */
	gnet_tcp_socket_delete (self->socket);
	self->socket = NULL;
	/* Perform a clean-up */
	kfxmpp_session_close (self);
	
	self->state = KFXMPP_SESSION_STATE_CLOSED;
	if (self->callback) {
		/* Inform user of error by calling callback */

		self->callback (self, error, self->callback_data);
	}
}


/**
 * \brief Disconnect from remote host
 * \param self A session
 * \param error Location to store error information (may be NULL)
 * \return TRUE on success, FALSE otherwise
 **/
gboolean kfxmpp_session_disconnect (KfxmppSession *self, GError **error)
{
	g_return_val_if_fail (self, FALSE);

	kfxmpp_log ("Disconnecting\n");

	if (self->state == KFXMPP_SESSION_STATE_CLOSED) {
		/* Trying to close session that is not open */
		g_set_error (error, KFXMPP_ERROR,
				KFXMPP_ERROR_SESSION_NOT_OPEN,
				"Session is not connected");
		return FALSE;
	}
	
	/* Close XML stream to server */
	kfxmpp_session_send_raw (self, "</stream:stream>", 16, NULL);

	/* Close underlying socket */
	gnet_tcp_socket_delete (self->socket);
	self->socket = NULL;

	kfxmpp_session_close (self);

	self->state = KFXMPP_SESSION_STATE_CLOSED;

	return TRUE;
}


/**
 * \brief Set function that will be called when connection closes
 * \param self A session
 * \param callback A callback
 * \param data Data that will be passed to callback
 **/
void kfxmpp_session_set_disconnect_callback (KfxmppSession *self, KfxmppSessionDisconnectCallback callback, gpointer data)
{
	g_return_if_fail (self);

	self->disconnect_callback = callback;
	self->disconnect_data = data;
}


/**
 * \brief Close session and clean it up
 **/
static void kfxmpp_session_close (KfxmppSession *self)
{
	g_return_if_fail (self);

	/* Cancel connect timeout */
	if (self->connect_timeout_id != 0) {
		g_source_remove (self->connect_timeout_id);
		self->connect_timeout_id = 0;
	}

	if (self->ping_pong_id) {
		g_source_remove (self->ping_pong_id);
		self->ping_pong_id = 0;
	}

	/* Remove event handlers */
	int i;
	for (i = 0; i < 4; i++) {
		if (self->sources[i])
			g_source_destroy (self->sources[i]);
		self->sources[i] = NULL;
	}
}


/**
 * \brief Add a watch to session
 * \param self A KfxmppSession
 * \param condition Condition to meet
 * \func Callback called when condifion is fulfilled
 * \return A new GSource
 **/
static GSource *kfxmpp_create_watch (KfxmppSession *self, GIOCondition condition, GSourceFunc func)
{
	GSource *src;

	src = g_io_create_watch (self->io, condition);
	g_source_set_callback (src, (GSourceFunc) func, self, NULL);
	g_source_attach (src, self->context);

	return src;
}


/**
 * \brief A callback called when session connects to remote host
 * \param socket A socket object
 * \param status Status of operation
 * \data User data - KfxmppSession object passed by kfxmpp_session_connect
 **/
static void kfxmpp_session_connected (GTcpSocket *socket, GTcpSocketConnectAsyncStatus status, gpointer data)
{
	KfxmppSession *self = data;

	const gchar *addr = self->host_address ? self->host_address : self->server;

	/* TODO: handle errors */
	
	if (status == GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK) {
		/* Connection was succesful */

		kfxmpp_log ("Connected\n");

		self->socket = socket;
		self->io = gnet_tcp_socket_get_io_channel (socket);
		/* TODO: set appropriate status here */
		self->state = KFXMPP_SESSION_STATE_CONNECTED;

		/* Setup event notifications */
		self->sources[0] = kfxmpp_create_watch (self, G_IO_IN,   kfxmpp_session_io_event);
		self->sources[1] = kfxmpp_create_watch (self, G_IO_ERR,  kfxmpp_session_io_event);
		self->sources[2] = kfxmpp_create_watch (self, G_IO_HUP,  kfxmpp_session_io_event);
		self->sources[3] = kfxmpp_create_watch (self, G_IO_NVAL, kfxmpp_session_io_event);


//		g_io_add_watch (self->io, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
//				kfxmpp_session_io_event, self);
//		g_io_add_watch (self->io, G_IO_IN,
//				kfxmpp_session_io_event, self);
//		g_io_add_watch (self->io, G_IO_ERR,
//				kfxmpp_session_io_event, self);
//		g_io_add_watch (self->io, G_IO_HUP,
//				kfxmpp_session_io_event, self);
//		g_io_add_watch (self->io, G_IO_NVAL,
//				kfxmpp_session_io_event, self);

//		self->source = g_io_create_watch (self->io, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL);
//		g_source_set_callback (self->source, (GSourceFunc) kfxmpp_session_io_event, self, NULL);
//		g_source_attach (self->source, self->context);
		
		
		/* Open XML stream to remote host */
		kfxmpp_session_open_stream (self);
		
	} else if (status == GTCP_SOCKET_CONNECT_ASYNC_STATUS_INETADDR_ERROR) {
		/* Could not resolve hostname */
		
		kfxmpp_log ("Couldnt resolve %s:%d\n", addr, self->port);
//		self->state = KFXMPP_SESSION_STATE_CLOSED;
//
//		if (self->callback) {
//			/* Inform user of erro by calling callback */
//
//			self->callback (self, KFXMPP_ERROR_ADDRESS_LOOKUP_FAILED, self->callback_data);
//		}
		
		kfxmpp_session_connect_failed (self, KFXMPP_ERROR_ADDRESS_LOOKUP_FAILED);

	} else if (status == GTCP_SOCKET_CONNECT_ASYNC_STATUS_TCP_ERROR) {
		/* Could not connect to remote host */
		
		kfxmpp_log ("Couldnt connect to %s:%d\n", addr, self->port);
//		self->state = KFXMPP_SESSION_STATE_CLOSED;
//
//		if (self->callback) {
//			/* Inform user of erro by calling callback */
//
//			self->callback (self, KFXMPP_ERROR_CONNECT_FAILED, self->callback_data);
//		}
		
		kfxmpp_session_connect_failed (self, KFXMPP_ERROR_CONNECT_FAILED);
	}
}


/**
 * \brief Open stream tag to the server
 **/
static void kfxmpp_session_open_stream (KfxmppSession *self)
{
	gchar *tag;

	g_return_if_fail (self);

	tag = g_strdup_printf ("<?xml version='1.0'?>"
			"<stream:stream "
			"to='%s' "
			"xmlns='jabber:client' "
			"xmlns:stream='http://etherx.jabber.org/streams' "
			"version='1.0'>",
			self->server);

	kfxmpp_session_send_raw (self, tag, -1, NULL);
	g_free (tag);
}



/***********************************************************************
 *
 * TLS layer stuff
 * 
 */


/**
 * \brief Request TLS negotiation
 * \param session A session
 *
 * This is automatically performed by kfxmpp_session_connect, so, on
 * most cases, you'll probably don't need to call this ditectly.
 **/
void kfxmpp_session_starttls (KfxmppSession *self)
{
	g_return_if_fail (self);

	kfxmpp_session_send_raw (self, KFXMPP_SESSION_TLS, sizeof (KFXMPP_SESSION_TLS)-1, NULL);
									/* -1 is for '\0' character */
}


/**
 * \brief Perform TLS handshake
 * \param session A session
 **/
gint kfxmpp_session_tls_handshake (KfxmppSession *self)
{
	gint ret;
	
	g_return_val_if_fail (self, -99);

	/* TODO: check status of operations */
	/* Todo check certificate */

	/* Allocate certificate credentials */
	gnutls_certificate_allocate_credentials (&self->cred);
	/* Initialize gnutls session object */
	gnutls_init (&self->gnutls, GNUTLS_CLIENT);
	/* Set default priorities on the ciphers, key exchange methods, macs and
	 * compression methods. Think it should be fine */
	gnutls_set_default_priority (self->gnutls);
	/* Assign credentials to gnutls session */
	gnutls_credentials_set (self->gnutls, GNUTLS_CRD_CERTIFICATE, self->cred);

	/* Setup transport layer */
	gnutls_transport_set_push_function (self->gnutls, kfxmpp_session_tls_send);
	gnutls_transport_set_pull_function (self->gnutls, kfxmpp_session_tls_recv);
	gnutls_transport_set_ptr (self->gnutls, (gnutls_transport_ptr_t) self);
	gnutls_transport_set_lowat (self->gnutls, 0);

	ret = gnutls_handshake (self->gnutls);
	if (ret != 0) {
		/* Something has gone wrong */
		gnutls_deinit (self->gnutls);
		gnutls_certificate_free_credentials (self->cred);
		
		kfxmpp_log ("TLS handshake failed\n");

		/* Some kind of error */
		return -1;
	}

	/* Mark that we had secured the connection */
	self->secure = TRUE;
	return 0;
}


/**
 * \brief Send function for gnutls
 **/
static gssize kfxmpp_session_tls_send (gnutls_transport_ptr_t p, const void*data, gsize size)
{
	KfxmppSession *self = p;
	GError *error = NULL;
	gint bytes_written;
	GIOStatus status;
	
	status = g_io_channel_write_chars (self->io, data, size, &bytes_written, &error);
	return bytes_written;
}


/**
 * \brief Recv function for gnutls
 **/
static gssize kfxmpp_session_tls_recv (gnutls_transport_ptr_t p, void* data, gsize size)
{
	KfxmppSession *self = p;
	GError *error = NULL;
	gint bytes_read;
	GIOStatus status;
	
	status = g_io_channel_read_chars (self->io, data, size, &bytes_read, &error);
	return bytes_read;
}


/***********************************************************************
 *
 * Event handling
 * 
 */


/**
 * \brief Connect a handler for an event
 **/
void kfxmpp_session_add_handler (KfxmppSession *self, KfxmppEventType type, KfxmppEventHandler *handler, gint priority)
{
	g_return_if_fail (self);

	kfxmpp_event_add_handler (self->events[type], handler, priority);
}


/**
 * \brief Callback called when remote host opens stream
 **/
static void kfxmpp_session_got_stream (KfxmppStreamParser *parser, gint version, const gchar *id, gpointer data)
{
	KfxmppSession *self = data;

	kfxmpp_log ("got stream\n");

	if (self->protocol == KFXMPP_PROTOCOL_JABBER ||
		(self->protocol == KFXMPP_PROTOCOL_AUTO && version < 1)) {
		/* Legacy mode for older implementations */
		
		/* Use non-SASL authentication */
		kfxmpp_session_iq_auth (self);
	}

	if (self->protocol == KFXMPP_PROTOCOL_XMPP && version < 1) {
		/* We want only XMPP protocol, but it looks like remote host
		 * does not support it */
	}
}


/**
 * \brief Callback called when new XML is parsed
 **/
static void kfxmpp_session_got_xml (KfxmppStreamParser *parser, xmlNodePtr node, gpointer data)
{
	KfxmppSession *self = data;
	KfxmppStanza *stanza;

	kfxmpp_log ("Got <%s>\n", node->name);
	stanza = kfxmpp_stanza_new_from_xml (node);

	/* Trigger an event */
	kfxmpp_event_trigger (self->events[KFXMPP_EVENT_TYPE_XML], stanza);
}


/**
 * \brief Event handler for all incoming XML stanzas
 **/
static gboolean kfxmpp_session_xml_event (KfxmppEventHandler *handler, KfxmppSession *self, KfxmppStanza *stanza, gpointer data)
{
	xmlNodePtr root;
	const gchar *name; /* Tag name */
	root = stanza->node;
	name = root->name;

	kfxmpp_log ("EventHandler: got <%s>\n", name);

	/*
	 * Check if we are awaiting a response for previously sent message
	 */
	gchar *id = xmlGetProp (root, BAD_CAST "id");
	if (id) {
		/* Check if we have been waiting for that ID */

		kfxmpp_log ("Searching for id '%s'\n", id);

		KfxmppEventHandler *handler;

		handler = g_hash_table_lookup (self->response_ids, id);
		if (handler) {
			gboolean handled;

			kfxmpp_log ("Found handler, calling...\n");
			
			/* Call handler */
			handled = kfxmpp_event_handler_call (handler, self, stanza);

			/* Remove that handler */
			g_hash_table_remove (self->response_ids, id);

			if (handled) {
				/* That handler reports to have succesfully handled message.
				 * Stop other handlers */

				xmlFree (id);
				return TRUE;
			}
		}
		xmlFree (id);
	}

	
	/*
	 *  Check tag that we have received
	 *  We are interested in:
	 *  <message/>	- ordinary message
	 *  <presence/>	- presence notification
	 *  <iq/>	- query
	 *  <features/> - server informs us about features it provides
	 *  <proceed/>	- server wants us to proceed with STARTTLS
	 *  <success/>	- server informs us that user was succesfully authorized
	 *  <failure/>	- probably authorization failed
	 *  <error/>	- other error
	 */
	if (strcmp (name, "message") == 0) {
		KfxmppMessage *msg;

		msg = kfxmpp_message_new (NULL);
		kfxmpp_message_parse_stanza (msg, stanza);
		kfxmpp_event_trigger (self->events[KFXMPP_EVENT_TYPE_MESSAGE], msg);
		kfxmpp_message_unref (msg);
	} else if (strcmp (name, "features") == 0) {
		/* Server advertises features it supports */
		xmlNodePtr node;
		KfxmppSessionStreamFeatures features = KFXMPP_SESSION_STREAM_FEATURES_NONE;

		kfxmpp_log ("->features\n");

		for (node = root->children; node; node = node->next) {
			kfxmpp_log ("--> <%s>\n", node->name);
			if (strcmp (node->name, "starttls") == 0) {
				/* TLS support */
				features |= KFXMPP_SESSION_STREAM_FEATURES_STARTTLS;

//				kfxmpp_log ("Starting TLS\n");

				/* Request TLS encryption */
//				kfxmpp_session_starttls (self);
//				return TRUE;
				
			} else if (strcmp (node->name, "mechanisms") == 0) {
				/* SASL mechanisms */
				features |= KFXMPP_SESSION_STREAM_FEATURES_SASL;
//				kfxmpp_log ("SASL!\n");
//				kfxmpp_sasl_plain (self);
			} else if (strcmp (node->name, "bind") == 0) {
				/* Bind resource */
//				kfxmpp_session_bind_resource (self);
				features |= KFXMPP_SESSION_STREAM_FEATURES_BIND;
			}
		}

		/* TLS */
		if (self->use_tls != KFXMPP_TLS_POLICY_NEVER) {
			if (features & KFXMPP_SESSION_STREAM_FEATURES_STARTTLS) {
				/* STARTTLS is supported */
				/* Request TLS encryption */
				kfxmpp_session_starttls (self);
				return TRUE;
			} else {
				/* TLS is not supported, but we want it... */
				/* TODO */
			}
		}

		/* SASL */
		if ((self->secure || self->use_tls != KFXMPP_TLS_POLICY_ALWAYS) 
				&& features & KFXMPP_SESSION_STREAM_FEATURES_SASL) {
			/* Session is secure or we don't want it to be secure */
			/* Remote host supports SASL */
			kfxmpp_sasl_plain (self);
			self->state = KFXMPP_SESSION_STATE_AUTHENTICATING;
			return TRUE;
		}

		/* Resource binding */
		if (features & KFXMPP_SESSION_STREAM_FEATURES_BIND) {
			kfxmpp_session_bind_resource (self);
			return TRUE;
		}

		if (self->state == KFXMPP_SESSION_STATE_CONNECTED 
				&& ! (features & KFXMPP_SESSION_STREAM_FEATURES_SASL)) {
			/* We want to authenticate, but server does not support
			 * SASL */
			/* We'll start non-SASL authentication */
			if (self->protocol != KFXMPP_PROTOCOL_XMPP) {
				kfxmpp_session_iq_auth (self);
				return TRUE;
			} else {
				/* We want only XMPP protocol, but server doesn't want
				 * us to authenticate with SASL */
				/* Dunno what to do */
			}
		}
	
		kfxmpp_log ("Dead end\n");
		kfxmpp_session_connect_failed (self, KFXMPP_ERROR_TLS_HANDSHAKE_FAILED);

	} else if (strcmp (name, "proceed") == 0) {
		/* Server wants us to proceed with TLS handshake */
		kfxmpp_log ("->proceed\n");
		
		if (kfxmpp_session_tls_handshake (self) == 0) {

			/* Re-initialize the stream */
//			kfxmpp_stream_parser_unref (self->parser);
			self->parser = kfxmpp_stream_parser_new (kfxmpp_session_got_xml, self);

			kfxmpp_session_open_stream (self);
		} else {
			/* Do something with error */
			kfxmpp_log ("Error during TLS handshake\n");
			
			kfxmpp_session_connect_failed (self, KFXMPP_ERROR_TLS_HANDSHAKE_FAILED);
		}
	} else if (strcmp (name, "success") == 0) {
		/* We have succeeded with SASL authentication */

		/* Re-initialize the stream */
//		kfxmpp_stream_parser_unref (self->parser);
		self->parser = kfxmpp_stream_parser_new (kfxmpp_session_got_xml, self);
		kfxmpp_session_open_stream (self);
		self->state = KFXMPP_SESSION_STATE_OPEN;
	} else if (strcmp (name, "failure") == 0) {
		/* Some kind of a failure */

		kfxmpp_session_connect_failed (self, KFXMPP_ERROR_AUTH_FAILED);
	} else if (strcmp (name, "error") == 0) {
		/* Error condition */
		
		kfxmpp_log ("<error/>... giving up\n");
		gchar *description = NULL;

		/* Try to investigate kind of error */
		xmlNodePtr node;
		for (node = root->children; node; node = node->next) {
			kfxmpp_log ("--> <%s/>\n", node->name);
			if (strcmp (node->name, "text") == 0) {
				/* Textual description of error */
				xmlChar *tmp = xmlNodeGetContent (node);
				description = g_strdup (tmp);
				xmlFree (tmp);
			}
		}
		kfxmpp_session_connect_failed (self, KFXMPP_ERROR_AUTH_FAILED);
		g_free (description);
	}

	return FALSE;
}


/**
 * \brief Call a XML stanza event handler when message of given ID has been called
 * \param session A session
 * \param idstr ID
 * \param handler A handler
 * \return ID
 **/
void kfxmpp_session_await_response (KfxmppSession *self, const gchar *idstr, KfxmppEventHandler *handler)
{
	g_return_if_fail (self);
	g_return_if_fail (handler);

	g_hash_table_insert (self->response_ids, g_strdup (idstr), 
			kfxmpp_event_handler_ref (handler));
}


/**
 * \brief Cancel IQ request
 * \param session A session
 * \param id ID
 **/
void kfxmpp_session_cancel_response (KfxmppSession *self, gint id)
{
	gchar idstr[10];	/* Should be enough */

	g_return_if_fail (self);

	snprintf (idstr, 10, RESPONSE_STRING, id);
	g_hash_table_remove (self->response_ids, idstr);	
}


/**
 * \brief Bind resource to a given session (XMPP only)
 **/
static void kfxmpp_session_bind_resource (KfxmppSession *self)
{
	KfxmppStanza *stanza;
	KfxmppEventHandler *handler;
	xmlNodePtr node;

	kfxmpp_log ("kfxmpp_session_bind_resource\n");

	/* Prepare stanza */
	stanza = kfxmpp_stanza_new (NULL, KFXMPP_STANZA_KLASS_IQ);
	xmlSetProp (stanza->node, BAD_CAST "type", BAD_CAST "set");
	node = xmlNewChild (stanza->node, NULL, BAD_CAST "bind", NULL);
	xmlNewNs (node, BAD_CAST "urn:ietf:params:xml:ns:xmpp-bind", NULL);
	xmlNewTextChild (node, NULL, BAD_CAST "resource", BAD_CAST self->resource);

	/* Prepare handler */
	handler = kfxmpp_event_handler_new (kfxmpp_session_bind_resource_response, NULL, NULL);
	kfxmpp_session_send_await_response (self, stanza, handler, NULL);
	kfxmpp_event_handler_unref (handler);
	kfxmpp_stanza_free (stanza);
}


/**
 * \brief Callback for bind request
 **/
static gboolean kfxmpp_session_bind_resource_response (KfxmppEventHandler *handler, gpointer source,
					gpointer event, gpointer data)
{
	xmlChar *type;
	KfxmppStanza *stanza = event;
	KfxmppSession *self = source;
	type = xmlGetProp (stanza->node, BAD_CAST "type");
	if (type && strcmp (type, "result") == 0) {
		/* All OK */
		kfxmpp_log ("Bind: OK\n");
//		self->state = KFXMPP_SESSION_STATE_OPEN;
		kfxmpp_session_connect_ok (self);
	} else {
		/* Error */

		kfxmpp_session_connect_failed (self, KFXMPP_ERROR_UNKNOWN);
	}
	xmlFree (type);

	return TRUE;
}



/***********************************************************************
 *
 * Non-SASL authentication
 * 
 */

/**
 * \brief Start a non-SASL authentication
 * \param self A session
 **/
static void kfxmpp_session_iq_auth (KfxmppSession *self)
{
	KfxmppStanza *iq;
	xmlNodePtr query;
	KfxmppEventHandler *handler;
	
	g_return_if_fail (self);

	kfxmpp_log ("Starting Non-SASL authentication\n");
	self->state = KFXMPP_SESSION_STATE_AUTHENTICATING;

	/* Prepare stanza */
	iq = kfxmpp_stanza_new (self->server, KFXMPP_STANZA_KLASS_IQ);
	xmlSetProp (iq->node, BAD_CAST "type", BAD_CAST "get");
	query = xmlNewChild (iq->node, NULL, BAD_CAST "query", NULL);
	xmlNewNs (query, BAD_CAST "jabber:iq:auth", NULL);
	xmlNewTextChild (query, NULL, BAD_CAST "username", BAD_CAST self->username);

	/* Prepare handler */
	handler = kfxmpp_event_handler_new (kfxmpp_session_iq_auth_response, NULL, NULL);
	kfxmpp_session_send_await_response (self, iq, handler, NULL);
	kfxmpp_event_handler_unref (handler);
	kfxmpp_stanza_free (iq);
}


/**
 * \brief Callback for jabber:iq:auth request
 **/
static gboolean kfxmpp_session_iq_auth_response (KfxmppEventHandler *h, gpointer source,
					gpointer event, gpointer data)
{
	xmlChar *type;
	KfxmppStanza *stanza = event;
	KfxmppSession *self = source;

	KfxmppStanza *iq;
	xmlNodePtr query;
	KfxmppEventHandler *handler;

	gchar *authid;
	gchar *digest;
	GSHA *sha;

	/* Prepare stanza */
	iq = kfxmpp_stanza_new (self->server, KFXMPP_STANZA_KLASS_IQ);
	xmlSetProp (iq->node, BAD_CAST "type", BAD_CAST "set");
	query = xmlNewChild (iq->node, NULL, BAD_CAST "query", NULL);
	xmlNewNs (query, BAD_CAST "jabber:iq:auth", NULL);
	xmlNewTextChild (query, NULL, BAD_CAST "username", BAD_CAST self->username);
	xmlNewTextChild (query, NULL, BAD_CAST "resource", BAD_CAST self->resource);

	/* Compute digest value */
	authid = g_strdup_printf ("%s%s", kfxmpp_stream_parser_get_id (self->parser), self->password);
	sha = gnet_sha_new (authid, strlen (authid));
	g_free (authid);
	digest = gnet_sha_get_string (sha);
	gnet_sha_delete (sha);

	xmlNewTextChild (query, NULL, BAD_CAST "digest", BAD_CAST digest);
	g_free (digest);
	
	/* Prepare handler */
	handler = kfxmpp_event_handler_new (kfxmpp_session_iq_auth_response2, NULL, NULL);
	kfxmpp_session_send_await_response (self, iq, handler, NULL);
	kfxmpp_event_handler_unref (handler);
	kfxmpp_stanza_free (iq);
	return TRUE;
}


/**
 * \brief Callback for jabber:iq:auth request
 **/
static gboolean kfxmpp_session_iq_auth_response2 (KfxmppEventHandler *h, gpointer source,
					gpointer event, gpointer data)
{
	xmlChar *type;
	KfxmppStanza *stanza = event;
	KfxmppSession *self = source;

	type = xmlGetProp (stanza->node, BAD_CAST "type");
	if (xmlStrcmp (type, BAD_CAST "result") == 0) {
		/* ALL ok */
		kfxmpp_session_connect_ok (self);
	} else if (xmlStrcmp (type, BAD_CAST "error") == 0) {
		/* Error */
		kfxmpp_session_connect_failed (self, KFXMPP_ERROR_AUTH_FAILED);
	} else {
		/* Something weird */
	}
	return TRUE;
}
