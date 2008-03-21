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

/** \file kfxmpp.h */

#include "kfxmpp.h"

#ifdef HAVE_GNUTLS
#  include <gnutls/gnutls.h>
#endif

/**
 * \brief Initialize kfxmpp library
 *
 * This function should be run prior any other kfxmpp function, preferably
 * at startup of an application.
 **/
void kfxmpp_init (void)
{
	kfxmpp_log ("Hello! This is kfxmpp speaking.\n");
	gnet_init ();
#ifdef HAVE_GNUTLS
	gnutls_global_init ();
#endif
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



