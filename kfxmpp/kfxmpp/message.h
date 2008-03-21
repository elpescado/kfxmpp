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

/** \file message.h */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <glib.h>
#include <kfxmpp/session.h>


/**
 * \brief Message type
 */
typedef enum {
	KFXMPP_MESSAGE_TYPE_NORMAL,	/**< Normal message	*/
	KFXMPP_MESSAGE_TYPE_CHAT,	/**< Chat message	*/
	KFXMPP_MESSAGE_TYPE_HEADLINE	/**< Headline message	*/
} KfxmppMessageType;


/**
 * \brief A message
 */
typedef struct {
	gchar *from;	/**< Message sender	*/
	gchar *to;	/**< Message recipient	*/
	KfxmppMessageType type;	/**< Message type */
	gchar *subject;	/**< Message subject	*/
	gchar *body;	/**< Message body	*/
	gint ref_count;	/**< Reference count	*/
} KfxmppMessage;


KfxmppMessage *kfxmpp_message_new (const gchar *to);
void kfxmpp_message_free (KfxmppMessage *self);
KfxmppMessage* kfxmpp_message_ref (KfxmppMessage *self);
void kfxmpp_message_unref (KfxmppMessage *self);

void kfxmpp_message_set_from (KfxmppMessage *self, const gchar *from);
const gchar *kfxmpp_session_get_from (KfxmppMessage *self);
void kfxmpp_message_set_to (KfxmppMessage *self, const gchar *to);
const gchar *kfxmpp_session_get_to (KfxmppMessage *self);
void kfxmpp_message_set_type (KfxmppMessage *self, KfxmppMessageType type);
KfxmppMessageType kfxmpp_session_get_type (KfxmppMessage *self);
void kfxmpp_message_set_subject (KfxmppMessage *self, const gchar *subject);
const gchar *kfxmpp_session_get_subject (KfxmppMessage *self);
void kfxmpp_message_set_body (KfxmppMessage *self, const gchar *body);
const gchar *kfxmpp_session_get_body (KfxmppMessage *self);
	
void kfxmpp_message_send (KfxmppMessage *self, KfxmppSession *session);

void kfxmpp_message_parse_stanza (KfxmppMessage *self, KfxmppStanza *stanza);

void kfxmpp_message_send_simple (KfxmppSession *session, const gchar *to, const gchar *body);
	
#endif /* __MESSAGE_H__ */
