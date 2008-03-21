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

/** \file sasl.h */

#include <string.h>
#include <gnet.h>

#include "kfxmpp.h"
#include "session.h"
#include "sasl.h"
#include "stanza.h"


/**
 * \brief Authenticate using PLAIN mechanism
 **/
void kfxmpp_sasl_plain (KfxmppSession *session)
{
	gchar *msg;
	const gchar *authid = kfxmpp_session_get_username (session);
	const gchar *pass = kfxmpp_session_get_password (session);
	gint authid_len = strlen (authid);
	gint pass_len = strlen (pass);
	gchar *base;
	gint dstlen;

	msg = g_new (gchar, authid_len + pass_len + 2);
	msg[0] = '\0';
	memcpy (msg+1, authid, authid_len);
	msg[authid_len + 1] = '\0';
	memcpy (msg+authid_len+2, pass, pass_len);

	base = gnet_base64_encode (msg, authid_len + pass_len + 2, &dstlen, TRUE);
		
	xmlNodePtr auth;
	auth = xmlNewNode (NULL, "auth");
	xmlNewNs (auth, "urn:ietf:params:xml:ns:xmpp-sasl", NULL);
	xmlSetProp (auth, "mechanism", "PLAIN");
	xmlNodeSetContent (auth, base);

	KfxmppStanza *stanza;
	stanza = kfxmpp_stanza_new_from_xml (auth);
	kfxmpp_session_send (session, stanza, NULL);
	kfxmpp_stanza_free (stanza);

	g_free (msg);
	g_free (base);
}
