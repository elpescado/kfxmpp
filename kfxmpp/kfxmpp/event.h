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
 
/** \file event.h */

#ifndef __EVENT_H__
#define __EVENT_H__

#include <glib.h>


/**
 * \brief event type indicator
 **/
typedef enum {
	KFXMPP_EVENT_TYPE_XML,		/**< XML stanza received */
	KFXMPP_EVENT_TYPE_MESSAGE,	/**< Incoming message	*/
	KFXMPP_N_EVENT_TYPES
} KfxmppEventType;


/**
 * \brief Some predefined priority values
 **/
typedef enum {
	KFXMPP_EVENT_HANDLER_PRIORITY_LOW = 10,		/**< Very low priority */
	KFXMPP_EVENT_HANDLER_PRIORITY_KFXMPP = 20,	/**< Priority of internal handlers */
	KFXMPP_EVENT_HANDLER_PRIORITY_NORMAL = 30,	/**< Normal priority */
	KFXMPP_EVENT_HANDLER_PRIORITY_HIGH = 40		/**< High priority */
} KfxmppEventHandlerPriority;

/**
 * \brief An event
 **/
typedef struct _KfxmppEvent KfxmppEvent;

/**
 * \brief An object that listens for certain events
 **/
typedef struct _KfxmppEventHandler KfxmppEventHandler;


/**
 * \callback function called by a handler
 * \param handler A handler
 * \param source Object that triggered this event
 * \param event Event structure
 * \param data User supplied data
 * \return TRUE if event was handled succesfully
 * 	FALSE to call more handlers
 **/
typedef gboolean (*KfxmppEventHandlerFunc) (KfxmppEventHandler *handler, gpointer source,
					gpointer event, gpointer data);


KfxmppEvent *kfxmpp_event_new (gpointer source);
void kfxmpp_event_free (KfxmppEvent *self);
KfxmppEvent* kfxmpp_event_ref (KfxmppEvent *self);
void kfxmpp_event_unref (KfxmppEvent *self);
void kfxmpp_event_add_handler (KfxmppEvent *self, KfxmppEventHandler *handler, gint priority);
void kfxmpp_event_remove_handler (KfxmppEvent *event, KfxmppEventHandler *handler);
gboolean kfxmpp_event_trigger (KfxmppEvent *event, gpointer data);

KfxmppEventHandler *kfxmpp_event_handler_new (KfxmppEventHandlerFunc callback, gpointer data, GDestroyNotify notify);
void kfxmpp_event_handler_free (KfxmppEventHandler *self);
KfxmppEventHandler* kfxmpp_event_handler_ref (KfxmppEventHandler *self);
void kfxmpp_event_handler_unref (KfxmppEventHandler *self);
gboolean kfxmpp_event_handler_call (KfxmppEventHandler *handler, gpointer source, gpointer event_data);

#endif /* __HANDLER_H__ */
