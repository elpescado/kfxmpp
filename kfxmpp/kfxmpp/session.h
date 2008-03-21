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

/**
 * \file session.h
 **/

#ifndef __SESSION_H__
#define __SESSION_H__

#include <glib.h>
#include <kfxmpp/core.h>
#include <kfxmpp/event.h>
#include <kfxmpp/stanza.h>
#include <kfxmpp/error.h>

G_BEGIN_DECLS

typedef struct _KfxmppSession KfxmppSession;


/**
 * \brief state of a session
 **/
typedef enum {
	KFXMPP_SESSION_STATE_CLOSED,		/**< Connection is closed */
	KFXMPP_SESSION_STATE_CONNECTING,	/**< Connection is connecting to server */
	KFXMPP_SESSION_STATE_CONNECTED,		/**< Connection has estabilished connection to server but has not yet authenticated */
	KFXMPP_SESSION_STATE_AUTHENTICATING,	/**< Connection tries to authenticate itself */
	KFXMPP_SESSION_STATE_OPEN,		/**< Connection is ready for use */
} KfxmppSessionState;


/**
 * \brief Features supported by remote host
 **/
typedef enum {
	KFXMPP_SESSION_STREAM_FEATURES_NONE = 0,		/**< None */
	KFXMPP_SESSION_STREAM_FEATURES_STARTTLS	= 1 << 0,	/**< STARTTLS encryption layer */
	KFXMPP_SESSION_STREAM_FEATURES_SASL	= 1 << 1,	/**< SASL authentication mechanism */
	KFXMPP_SESSION_STREAM_FEATURES_BIND	= 1 << 2	/**< Resource binding */
} KfxmppSessionStreamFeatures;


/**
 * \brief Reason of disconnection
 **/
typedef enum {
	KFXMPP_SESSION_DISCONNECT_STATUS_USER,		/**< User closed connection */
	KFXMPP_SESSION_DISCONNECT_STATUS_REMOTE_HOST,	/**< Remote host closed connection */
	KFXMPP_SESSION_DISCONNECT_STATUS_UNKNOWN	/**< Unknown reason */
} KfxmppSessionDisconnectStatus;

/**
 * \brief TLS usage policy
 **/
typedef enum {
	KFXMPP_TLS_POLICY_ALWAYS,	/**< Always use TLS */
	KFXMPP_TLS_POLICY_IF_AVAILABLE,	/**< Use TLS only if it is available (default policy) */
	KFXMPP_TLS_POLICY_NEVER		/**< Never use TLS, even if it is available */
} KfxmppTlsUsagePolicy;

/**
 * \brief Callback called when connection estabilishes or an error is encountered.
 * \param session Calling session
 * \param error Status of an operation. KFXMPP_ERROR_NONE means success.
 * \param data User supplied data
 **/
typedef void (*KfxmppSessionConnectCallback) (KfxmppSession *session, KfxmppError error, gpointer data);
typedef void (*KfxmppSessionDisconnectCallback) (KfxmppSession *session, KfxmppSessionDisconnectStatus status, gpointer data);



/* General usage */
KfxmppSession *kfxmpp_session_new (const gchar *server);
void kfxmpp_session_free (KfxmppSession *self);
KfxmppSession* kfxmpp_session_ref (KfxmppSession *self);
void kfxmpp_session_unref (KfxmppSession *self);

/* Public access functions */
void kfxmpp_session_set_username (KfxmppSession *self, const gchar *username);
const gchar *kfxmpp_session_get_username (KfxmppSession *self);
void kfxmpp_session_set_server (KfxmppSession *self, const gchar *server);
const gchar *kfxmpp_session_get_server (KfxmppSession *self);
void kfxmpp_session_set_password (KfxmppSession *self, const gchar *pass);
const gchar *kfxmpp_session_get_password (KfxmppSession *self);
void kfxmpp_session_set_resource (KfxmppSession *self, const gchar *resource);
const gchar *kfxmpp_session_get_resource (KfxmppSession *self);
void kfxmpp_session_set_host_address (KfxmppSession *self, const gchar *host_address);
const gchar *kfxmpp_session_get_host_address (KfxmppSession *self);
void kfxmpp_session_set_port (KfxmppSession *self, int port);
int kfxmpp_session_get_port (KfxmppSession *self);
void kfxmpp_session_set_priority (KfxmppSession *self, int priority);
int kfxmpp_session_get_priority (KfxmppSession *self);
void kfxmpp_session_set_use_tls (KfxmppSession *self, KfxmppTlsUsagePolicy use_tls);
KfxmppTlsUsagePolicy kfxmpp_session_get_use_tls (KfxmppSession *self);
void kfxmpp_session_set_protocol (KfxmppSession *self, KfxmppProtocol proto);
KfxmppProtocol kfxmpp_session_get_protocol (KfxmppSession *self);
void kfxmpp_session_set_timeout (KfxmppSession *self, gint timeout);
KfxmppProtocol kfxmpp_session_get_timeout (KfxmppSession *self);

/* Network I/O */
gssize kfxmpp_session_read (KfxmppSession *self, gchar *buffer, gssize size, GError **error);
gssize kfxmpp_session_send (KfxmppSession *self, KfxmppStanza *stanza, GError **error);
gint kfxmpp_session_send_await_response (KfxmppSession *self, KfxmppStanza *stanza, KfxmppEventHandler *handler, GError **error);
gssize kfxmpp_session_send_raw (KfxmppSession *self, const gchar *buffer, gssize size, GError **error);

/* Networking */
gboolean kfxmpp_session_connect (KfxmppSession *self, KfxmppSessionConnectCallback callback, gpointer data, GError **error);
void kfxmpp_session_cancel_connect (KfxmppSession *self);
gboolean kfxmpp_session_disconnect (KfxmppSession *self, GError **error);
void kfxmpp_session_set_disconnect_callback (KfxmppSession *self, KfxmppSessionDisconnectCallback callback, gpointer data);

/* TLS */
void kfxmpp_session_starttls (KfxmppSession *self);
gint kfxmpp_session_tls_handshake (KfxmppSession *self);

/* Events */
void kfxmpp_session_add_handler (KfxmppSession *self, KfxmppEventType type, KfxmppEventHandler *handler, gint priority);
void kfxmpp_session_await_response (KfxmppSession *self, const gchar *id, KfxmppEventHandler *handler);
void kfxmpp_session_cancel_response (KfxmppSession *self, gint id);



G_END_DECLS

#endif /* __SESSION_H__ */
