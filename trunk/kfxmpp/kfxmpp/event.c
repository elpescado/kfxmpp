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

#include "kfxmpp.h"
#include "event.h"

/**
 * \brief Container for stanza handling callback
 **/
struct _KfxmppEventHandler {
	KfxmppEventHandlerFunc callback;	/**< Function called when this handler is activated */
	gpointer data;			/**< Data provided by user */
	GDestroyNotify notify;		/**< Function called to free data when freeing handler */
	gint ref_count;			/**< Number of references to this object */	
};


struct _KfxmppEvent {
	gpointer obj;		/**< Event source */
	GList *handlers;	/**< List of handlers listening for this event */
	gint ref_count;		/**< Number of references to this object */
};


typedef struct {
	KfxmppEventHandler *handler;	/**< Event handler */
	gint priority;			/**< Priority of this handler */
} KfxmppEventEntry;


static gint kfxmpp_event_entry_compare (gconstpointer a, gconstpointer b);

/**
 * \brief Create a new event
 * \param source Event source, that is an object that emits this signal
 **/
KfxmppEvent *kfxmpp_event_new (gpointer source)
{
	KfxmppEvent *ret;
	ret = g_new0 (KfxmppEvent, 1);
	ret->obj = source;
	ret->ref_count = 1;
	return ret;
}


/**
 * \brief Free an event structure
 **/
void kfxmpp_event_free (KfxmppEvent *self)
{
	GList *tmp;

	g_return_if_fail (self);

	for (tmp = self->handlers; tmp; tmp = tmp->next) {
		KfxmppEventEntry *entry = tmp->data;
		KfxmppEventHandler *handler = entry->handler;

		kfxmpp_event_handler_unref (handler);
		g_free (entry);
	}

	g_list_free (self->handlers);

	g_free (self);
}


/**
 * \brief Add a reference to KfxmppEvent
 **/
KfxmppEvent* kfxmpp_event_ref (KfxmppEvent *self)
{
        g_return_val_if_fail (self, NULL);
        self->ref_count++;
        return self;
}


/**
 * \brief Remove a reference from KfxmppEvent
 *
 * Object will be deleted when reference count reaches 0
 **/
void kfxmpp_event_unref (KfxmppEvent *self)
{
        g_return_if_fail (self);
        self->ref_count--;
        if (self->ref_count == 0)
                kfxmpp_event_free (self);
}


/**
 * \brief Add a handler to this event
 * \param self An event
 * \param handler An event handler
 * \param priority A handler priority. Handlers with greater priority will be called earlier.
 **/
void kfxmpp_event_add_handler (KfxmppEvent *self, KfxmppEventHandler *handler, gint priority)
{
	KfxmppEventEntry *entry;
	
	g_return_if_fail (self);
	g_return_if_fail (handler);

	entry = g_new (KfxmppEventEntry, 1);
	entry->handler = kfxmpp_event_handler_ref (handler);
	entry->priority = priority;

	self->handlers = g_list_insert_sorted (self->handlers, entry, kfxmpp_event_entry_compare);
}


/**
 * \brief Remova a handler from this event
 * \param event An event
 * \param handler An event handler
 **/
void kfxmpp_event_remove_handler (KfxmppEvent *event, KfxmppEventHandler *handler)
{
	g_return_if_fail (event);
	g_return_if_fail (handler);

	GList *tmp;
	GList *found = NULL;

	for (tmp = event->handlers; tmp; tmp = tmp->next) {
		KfxmppEventEntry *entry = tmp->data;
		
		if (entry->handler == handler) {
			kfxmpp_event_handler_unref (entry->handler);
			found = tmp;
			break;
		}
	}
	
	if (found) {
		g_free (tmp->data);
		event->handlers = g_list_delete_link (event->handlers, found);
	}
}


/**
 * \brief Thigger an event
 * \param event The event to be triggered
 * \param data Event-specific data
 **/
gboolean kfxmpp_event_trigger (KfxmppEvent *event, gpointer data)
{
	g_return_val_if_fail (event, FALSE);

	GList *tmp;

	for (tmp = event->handlers; tmp; tmp = tmp->next) {
		KfxmppEventEntry *entry = tmp->data;
		KfxmppEventHandler *handler = entry->handler;

//		if (handler->callback (handler, event->obj, data, handler->data) == TRUE)
		if (kfxmpp_event_handler_call (handler, event->obj, data) == TRUE)
			return TRUE;
	}
	return FALSE;
}


/**
 * \brief compare two KfxmppEventEntries
 *
 * Comparison is based on priorities
 **/
static gint kfxmpp_event_entry_compare (gconstpointer a, gconstpointer b)
{
	return ((KfxmppEventEntry *) b)->priority - ((KfxmppEventEntry *) a)->priority;
}


/**
 * \brief Create a new event handler
 **/
KfxmppEventHandler *kfxmpp_event_handler_new (KfxmppEventHandlerFunc callback, gpointer data, GDestroyNotify notify)
{
	KfxmppEventHandler *self;

	self = g_new0 (KfxmppEventHandler, 1);
	
	self->callback = callback;
	self->data = data;
	self->notify = notify;
	self->ref_count = 1;

	return self;
}


/**
 * \brief Free a message handler
 **/
void kfxmpp_event_handler_free (KfxmppEventHandler *self)
{
	g_return_if_fail (self);

	if (self->notify)
		self->notify (self->data);

	g_free (self);
}


/**
 * \brief Add a reference to KfxmppEventHandler
 **/
KfxmppEventHandler* kfxmpp_event_handler_ref (KfxmppEventHandler *self)
{
        g_return_val_if_fail (self, NULL);
        self->ref_count++;
        return self;
}


/**
 * \brief Remove a reference from KfxmppEventHandler
 *
 * Object will be deleted when reference count reaches 0
 **/
void kfxmpp_event_handler_unref (KfxmppEventHandler *self)
{
        g_return_if_fail (self);
        self->ref_count--;
        if (self->ref_count == 0)
                kfxmpp_event_handler_free (self);
}

gboolean kfxmpp_event_handler_call (KfxmppEventHandler *handler, gpointer source, gpointer event_data)
{
	g_return_val_if_fail (handler, FALSE);
	g_return_val_if_fail (handler->callback, FALSE);

	return handler->callback (handler, source, event_data, handler->data);
}
