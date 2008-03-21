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

/** \file error.h */

#ifndef __ERROR_H__
#define __ERROR_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	KFXMPP_ERROR_NONE = 0,			/**< A special error that reports lack of errors;-) */
	KFXMPP_ERROR_UNKNOWN,			/**< Some generic error */
	KFXMPP_ERROR_CANCELLED,			/**< Action was cancelled by user */
	KFXMPP_ERROR_ADDRESS_LOOKUP_FAILED,	/**< Internet address lookup failed */
	KFXMPP_ERROR_CONNECT_FAILED,		/**< Unable to connect to remote host */
	KFXMPP_ERROR_TLS_NOT_AVAILABLE,		/**< TLS was not available */
	KFXMPP_ERROR_TLS_HANDSHAKE_FAILED,	/**< TLS handshake failed */
	KFXMPP_ERROR_AUTH_FAILED,		/**< Authorization failed */
	KFXMPP_ERROR_SESSION_ALREADY_OPEN,	/**< Trying to open already opened session */
	KFXMPP_ERROR_SESSION_NOT_OPEN,		/**< Session is not open */
	KFXMPP_ERROR_TIMEOUT			/**< Timeout expired */
} KfxmppError;

#define KFXMPP_ERROR kfxmpp_error_quark ()

GQuark kfxmpp_error_quark (void);

G_END_DECLS

#endif /* __ERROR_H__ */
