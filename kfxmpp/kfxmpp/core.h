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

/** \file core.h */

#ifndef __CORE_H__
#define __CORE_H__

#include <glib.h>

#include "config.h"

G_BEGIN_DECLS


/**
 * \brief Protocol version
 **/
typedef enum {
	KFXMPP_PROTOCOL_AUTO,	/**< Automatically detect protocol version	*/
	KFXMPP_PROTOCOL_XMPP,	/**< XMPP 1.0 protocol				*/
	KFXMPP_PROTOCOL_JABBER	/**< Legacy Jabber protocol 			*/
} KfxmppProtocol;


/** Default XMPP/Jabber port */
#define KFXMPP_DEFAULT_PORT 5222

//#include "session.h"
//#include "streamparser.h"

#define kfxmpp_log(args...) g_log("kfxmpp", G_LOG_LEVEL_INFO, args)

void kfxmpp_init (void);
void kfxmpp_deinit (void);

G_END_DECLS

#endif /* __CORE_H__ */
