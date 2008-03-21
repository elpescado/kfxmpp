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

#include "kfxmpp.h"

#ifdef HAVE_GNUTLS
#  include <gnutls/gnutls.h>
#endif

gboolean debug_net = FALSE;
gboolean debug_msg = FALSE;

static void foo (const gchar *log_domain,
                                             GLogLevelFlags log_level,
                                             const gchar *message,
                                             gpointer user_data)
{
}

/**
 * \brief Initialize kfxmpp library
 *
 * This function should be run prior any other kfxmpp function, preferably
 * at startup of an application.
 **/
void kfxmpp_init (void)
{
	gnet_init ();
#ifdef HAVE_GNUTLS
	gnutls_global_init ();
#endif

#ifdef DEBUG
	gchar *debug = getenv ("KFXMPP_DEBUG");
	if (debug) {
		if (strcmp (debug, "all") == 0) {
			debug_net = TRUE;
			debug_msg = TRUE;
		} else if (strcmp (debug, "net") == 0) {
			debug_net = TRUE;
		}
	}

	if (debug_msg == FALSE) {
		g_log_set_handler ("kfxmpp", G_LOG_LEVEL_INFO, foo, NULL);
	}
#endif

	kfxmpp_log ("Hello! This is kfxmpp speaking.\n");
}


/**
 * \brief Deinit kfxmpp library
 *
 * Clean up and free any resources held by kfxmpp library.
 **/
void kfxmpp_deinit (void)
{
#ifdef HAVE_GNUTLS
	gnutls_global_deinit ();
#endif
}



