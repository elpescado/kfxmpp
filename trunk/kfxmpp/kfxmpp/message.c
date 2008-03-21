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

#include "kfxmpp.h"
#include "session.h"
#include "message.h"


/**
 * \brief Create new message
 * \param to Recipient JID
 **/
KfxmppMessage *kfxmpp_message_new (const gchar *to)
{
	KfxmppMessage *self;

	self = g_new0 (KfxmppMessage, 1);

	self->type = KFXMPP_MESSAGE_TYPE_NORMAL;

	if (to) {
		self->to = g_strdup (to);
	}

	self->ref_count = 1;

	return self;
}



/**
 * \brief Free message
 **/
void kfxmpp_message_free (KfxmppMessage *self)
{
	g_free (self->from);
	g_free (self->to);
	g_free (self->subject);
	g_free (self->body);
	g_free (self);
}


/**
 * \brief Add a reference to KfxmppMessage
 **/
KfxmppMessage* kfxmpp_message_ref (KfxmppMessage *self)
{
        g_return_val_if_fail (self, NULL);
        self->ref_count++;
        return self;
}


/**
 * \brief Remove a reference from KfxmppMessage
 *
 * Object will be deleted when reference count reaches 0
 **/
void kfxmpp_message_unref (KfxmppMessage *self)
{
        g_return_if_fail (self);
        self->ref_count--;
        if (self->ref_count == 0)
                kfxmpp_message_free (self);
}


/* Public access functions */


/**
 * \brief Set 
 * \param self A message
 * \param 
 **/
void kfxmpp_message_set_from (KfxmppMessage *self, const gchar *from)
{
	g_return_if_fail (self);

	if (self->from)
		g_free (self->from);
	self->from = from ? g_strdup (from) : NULL;
}


/**
 * \brief Get 
 * \param self A message
 * \return 
 **/
const gchar *kfxmpp_session_get_from (KfxmppMessage *self)
{
	g_return_val_if_fail (self, NULL);

	return self->from;
}


/**
 * \brief Set 
 * \param self A message
 * \param 
 **/
void kfxmpp_message_set_to (KfxmppMessage *self, const gchar *to)
{
	g_return_if_fail (self);

	if (self->to)
		g_free (self->to);
	self->to = to ? g_strdup (to) : NULL;
}


/**
 * \brief Get 
 * \param self A message
 * \return 
 **/
const gchar *kfxmpp_session_get_to (KfxmppMessage *self)
{
	g_return_val_if_fail (self, NULL);

	return self->to;
}


/**
 * \brief Get 
 * \param self A message
 * \return 
 **/
void kfxmpp_message_set_type (KfxmppMessage *self, KfxmppMessageType type)
{
	g_return_if_fail (self);

	self->type = type;
}


/**
 * \brief Get 
 * \param self A message
 * \return 
 **/
KfxmppMessageType kfxmpp_session_get_type (KfxmppMessage *self)
{
	g_return_val_if_fail (self, -9);

	return self->type;
}



/**
 * \brief Set 
 * \param self A message
 * \param 
 **/
void kfxmpp_message_set_subject (KfxmppMessage *self, const gchar *subject)
{
	g_return_if_fail (self);

	if (self->subject)
		g_free (self->subject);
	self->subject = subject ? g_strdup (subject) : NULL;
}


/**
 * \brief Get 
 * \param self A message
 * \return 
 **/
const gchar *kfxmpp_session_get_subject (KfxmppMessage *self)
{
	g_return_val_if_fail (self, NULL);

	return self->subject;
}


/**
 * \brief Set 
 * \param self A message
 * \param 
 **/
void kfxmpp_message_set_body (KfxmppMessage *self, const gchar *body)
{
	g_return_if_fail (self);

	if (self->body)
		g_free (self->body);
	self->body = body ? g_strdup (body) : NULL;
}


/**
 * \brief Get 
 * \param self A message
 * \return 
 **/
const gchar *kfxmpp_session_get_body (KfxmppMessage *self)
{
	g_return_val_if_fail (self, NULL);

	return self->body;
}


/**
 * \brief Create KfxmppStanza for this message
 * \return A new KfxmppStanza. Free it with kfxmpp_stanza_free
 **/
static KfxmppStanza *kfxmpp_message_create_stanza (KfxmppMessage *self)
{
	KfxmppStanza *msg;

	msg = kfxmpp_stanza_new (self->to, KFXMPP_STANZA_KLASS_MESSAGE);
	
	if (self->type == KFXMPP_MESSAGE_TYPE_CHAT)
		xmlSetProp (msg->node, "type", "chat");

	/* Construct new xml message */
	if (self->subject)
		xmlNewTextChild (msg->node, NULL, "subject", self->subject);
	
	xmlNewTextChild (msg->node, NULL, "body", self->body);
	
	return msg;
}


/**
 * \brief Send a message
 * \param self A message
 * \param session A session
 **/
void kfxmpp_message_send (KfxmppMessage *self, KfxmppSession *session)
{
	KfxmppStanza *stanza;

	stanza = kfxmpp_message_create_stanza (self);
	kfxmpp_session_send (session, stanza, NULL);
	kfxmpp_stanza_free (stanza);
}


/**
 * \brief Parse XML stanza to KfxmppMessage
 * \param stanza A KfxmppStanza
 **/
void kfxmpp_message_parse_stanza (KfxmppMessage *self, KfxmppStanza *stanza)
{
	g_return_if_fail (stanza);
	g_return_if_fail (stanza->node);

	xmlNodePtr root = stanza->node;
	
	xmlChar *to	= xmlGetProp (root, BAD_CAST "to");
	xmlChar *from	= xmlGetProp (root, BAD_CAST "from");
	xmlChar *type	= xmlGetProp (root, BAD_CAST "type");

	kfxmpp_message_set_to (self, to);
	kfxmpp_message_set_from (self, from);

	xmlNodePtr node;
	for (node = root->children; node; node = node->next) {
		kfxmpp_log ("   -> parsing <%s/>\n", node->name);
		xmlChar *content = xmlNodeGetContent (node);
		if (xmlStrcmp (node->name, BAD_CAST "body") == 0)
			kfxmpp_message_set_body (self, content);
		else if (xmlStrcmp (node->name, BAD_CAST "subject") == 0)
			kfxmpp_message_set_subject (self, content);
		xmlFree (content);
	}

	xmlFree (to);
	xmlFree (from);
	xmlFree (type);
}


/**
 * \brief Simple interface for sending messages
 * \param session A KfxmppSession
 * \param to Recipient JID
 * \param body Message body
 **/
void kfxmpp_message_send_simple (KfxmppSession *session, const gchar *to, const gchar *body)
{
	KfxmppStanza *msg;

	msg = kfxmpp_stanza_new (to, KFXMPP_STANZA_KLASS_MESSAGE);
	xmlNewTextChild (msg->node, NULL, "body", body);
	
	kfxmpp_session_send (session, msg, NULL);
	kfxmpp_stanza_free (msg);
}
