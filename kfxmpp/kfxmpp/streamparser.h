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

/** \file streamparser.h */

#ifndef __STREAMPARSER_H__
#define __STREAMPARSER_H__

#include <glib.h>
#include <libxml/tree.h>

G_BEGIN_DECLS

typedef struct _KfxmppStreamParser KfxmppStreamParser;


/**
 * \brief Callback called when xmlNode is read
 * \param parser A parser
 * \param node xml node
 * \param data User data
 **/
typedef void (*KfxmppStreamParserCallback) (KfxmppStreamParser *parser, xmlNodePtr node, gpointer data);


/**
 * \brief Callback called when stream starts
 * \param parser A parser
 * \param version A stream version
 * \param id Stream ID
 * \param data User data
 **/
typedef void (*KfxmppStreamParserStreamCallback) (KfxmppStreamParser *parser, gint version, const gchar *id, gpointer data);

KfxmppStreamParser *kfxmpp_stream_parser_new (KfxmppStreamParserCallback callback, gpointer data);
void kfxmpp_stream_parser_free (KfxmppStreamParser *self);
KfxmppStreamParser* kfxmpp_stream_parser_ref (KfxmppStreamParser *self);
void kfxmpp_stream_parser_unref (KfxmppStreamParser *self);

void kfxmpp_stream_parser_feed (KfxmppStreamParser *self, const gchar *data, gsize len);

gint kfxmpp_stream_parser_get_version (KfxmppStreamParser *self);
const gchar *kfxmpp_stream_parser_get_id (KfxmppStreamParser *self);

void kfxmpp_stream_parser_set_stream_callback (KfxmppStreamParser *parser, KfxmppStreamParserStreamCallback callback);

G_END_DECLS

#endif /* __STREAMPARSER_H__ */
