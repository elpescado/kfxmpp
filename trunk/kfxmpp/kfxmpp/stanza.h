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

#ifndef __STANZA_H__
#define __STANZA_H__

#include <glib.h>
#include <libxml/tree.h>

G_BEGIN_DECLS

/**
 * \brief Stanza class
 **/
typedef enum {
	KFXMPP_STANZA_KLASS_MESSAGE,
	KFXMPP_STANZA_KLASS_PRESENCE,
	KFXMPP_STANZA_KLASS_IQ,
	KFXMPP_STANZA_KLASS_UNKNOWN
} KfxmppStanzaKlass;

/**
 * \brief An XML stanza
 **/
typedef struct {
	xmlNodePtr	node;	/** Pointer to root node of that stanza */
	KfxmppStanzaKlass klass;/** Stanza class */
} KfxmppStanza;

KfxmppStanza *kfxmpp_stanza_new (const gchar *to, KfxmppStanzaKlass klass);
KfxmppStanza *kfxmpp_stanza_new_from_xml (xmlNodePtr node);
void kfxmpp_stanza_free (KfxmppStanza *self);
gchar *kfxmpp_stanza_to_string (KfxmppStanza *self);

G_END_DECLS

#endif /* __STANZA_H__ */
