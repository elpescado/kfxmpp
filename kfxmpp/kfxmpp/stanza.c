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

/** \file stanza.h */

#include "kfxmpp.h"
#include "stanza.h"


static KfxmppStanza *kfxmpp_stanza_new_intern (KfxmppStanzaKlass klass);

	
static KfxmppStanza *kfxmpp_stanza_new_intern (KfxmppStanzaKlass klass)
{
	KfxmppStanza *self;

	self = g_new0 (KfxmppStanza, 1);
	self->klass = klass;

	return self;
}


KfxmppStanza *kfxmpp_stanza_new (const gchar *to, KfxmppStanzaKlass klass)
{
	KfxmppStanza *self;
	const gchar *names[] = {"message", "presence", "iq"};

	self = kfxmpp_stanza_new_intern (klass);
	self->node = xmlNewNode (NULL, names[klass]);
	if (to)
		xmlSetProp (self->node, "to", to);

	return self;
}



/**
 * \brief Create stanza from xml node
 * \param node An xml node
 **/
KfxmppStanza *kfxmpp_stanza_new_from_xml (xmlNodePtr node)
{
	KfxmppStanza *self;
	KfxmppStanzaKlass klass = KFXMPP_STANZA_KLASS_UNKNOWN;

	/* Should it be here? */
	g_return_val_if_fail (node, NULL);

	if (xmlStrcmp (node->name, "message") == 0)
		klass = KFXMPP_STANZA_KLASS_MESSAGE;
	else if (xmlStrcmp (node->name, "presence") == 0)
		klass = KFXMPP_STANZA_KLASS_PRESENCE;
	else if (xmlStrcmp (node->name, "iq") == 0)
		klass = KFXMPP_STANZA_KLASS_IQ;
	else {
		kfxmpp_log ("Strange things happen... <%s>\n", node->name);
	}

	self = kfxmpp_stanza_new_intern (klass);
	
	self->node = node;

	return self;
}


/**
 * \brief Free Stanza
 * \param self A stanza
 **/
void kfxmpp_stanza_free (KfxmppStanza *self)
{
	g_free (self);
}


/**
 * \brief Dump XML stanza to a string
 * \param self A xml stanza
 * \return A newly allocated string. This should be freed with g_free when no longer used.
 **/
gchar *kfxmpp_stanza_to_string (KfxmppStanza *self)
{
	xmlBufferPtr buffer;
	gchar *text;
	
	buffer = xmlBufferCreate ();
	xmlNodeDump (buffer, NULL, self->node, 0, 0);
	text = g_strdup (buffer->content);
	xmlBufferFree (buffer);
	
	return text;
}


