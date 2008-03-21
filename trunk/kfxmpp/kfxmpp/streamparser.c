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

/** file streamparser.h */

#include <libxml/parser.h>
#include "kfxmpp.h"
#include "streamparser.h"

struct _KfxmppStreamParser {
	xmlParserCtxtPtr parser;	/**< XML parser context */
	gint depth;			/**< Current depth of an xml tree */
	xmlNodePtr nodes;		/**< Parsed nodes */

	/* Callback */
	KfxmppStreamParserCallback callback; /**< Callback called when detected xml stanza */
	gpointer callback_data;		/**< Callback user data */

	/* Stream information */
	gint version;			/**< Stream version */
	gchar *id;			/**< Stream ID */

	/* Stream callback */
	KfxmppStreamParserStreamCallback stream_callback;	/**< Callback called when stream is opened */

	/* Misc stuff */
	gint ref_count;			/**< Reference count */
};


/***********************************************************************
 *
 * Static function prototypes
 *
 */

/* SAX handlers */
static void onInternalSubset (void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID);
static int onIsStandalone (void * ctx);
static int onHasInternalSubset (void * ctx);
static int onHasExternalSubset (void * ctx);
static xmlParserInputPtr onResolveEntity (void * ctx, const xmlChar * publicId, const xmlChar * systemId);
static xmlEntityPtr onGetEntity (void * ctx, const xmlChar * name);
static void onEntityDecl (void * ctx, const xmlChar * name, int type, const xmlChar * publicId, const xmlChar * systemId, xmlChar * content);
static void onNotationDecl (void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId);
static void onAttributeDecl (void * ctx, const xmlChar * elem, const xmlChar * fullname, int type, int def, const xmlChar * defaultValue, xmlEnumerationPtr tree);
static void onElementDecl (void * ctx, const xmlChar * name, int type, xmlElementContentPtr content);
static void onUnparsedEntityDecl (void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId, const xmlChar * notationName);
static void onSetDocumentLocator (void * ctx, xmlSAXLocatorPtr loc);
static void onStartDocument (void * ctx);
static void onEndDocument (void * ctx);
static void onReference (void * ctx, const xmlChar * name);
static void onCharacters (void * ctx, const xmlChar * ch, int len);
static void onIgnorableWhitespace (void * ctx, const xmlChar * ch, int len);
static void onProcessingInstruction (void * ctx, const xmlChar * target, const xmlChar * data);
static void onComment (void * ctx, const xmlChar * value);
static void onStartElement (void * ctx, const xmlChar * name, const xmlChar **attrs);
static void onEndElement (void * ctx, const xmlChar * name);


/**
 * \brief create a new Stream parser
 **/
KfxmppStreamParser *kfxmpp_stream_parser_new (KfxmppStreamParserCallback callback, gpointer data)
{
	KfxmppStreamParser *self;

	self = g_new0 (KfxmppStreamParser, 1);

	/* Setup SAX handler that will parse XML data. In most cases we'll
	 * use standard tree building functions from libxml library.
	 * It may be a dirty hack, but it works, and, as of now, I have no
	 * better idea. */
	xmlSAXHandler saxHandler = {
		onInternalSubset, //internalSubset,
		onIsStandalone, //isStandalone,
		onHasInternalSubset, //hasInternalSubset,
		onHasExternalSubset, //hasExternalSubset,
		onResolveEntity, //resolveEntity,
		onGetEntity, //getEntity,
		onEntityDecl, //entityDecl,
		onNotationDecl, //notationDecl,
		onAttributeDecl, //attributeDecl,
		onElementDecl, //elementDecl,
		onUnparsedEntityDecl, //unparsedEntityDecl,
		onSetDocumentLocator, //setDocumentLocator,
		onStartDocument, //startDocument,
		onEndDocument, //endDocument
		onStartElement, //startElement
		onEndElement, //endElement,
		onReference,  //reference,
		onCharacters, //onCharacters, // characters,
		onIgnorableWhitespace, //ignorableWhitespace,
		onProcessingInstruction, //processingInstruction,
		onComment, //comment,
		NULL, //warning,
		NULL, //error,
		NULL, //fatalError,
		NULL, //getParameterEntity,
		NULL //cdataBlock
	};

	xmlSAXHandlerPtr sax = &saxHandler;


	/* Create XML parser */
	self->parser = xmlCreatePushParserCtxt	(sax,	/* Our hacked SAX handler */
						self,	/* No data passed to SAX handler */
						NULL,	/* No initial characters passed to parse */
						0,	/* Length */
						"stream"); /* URI */

	self->callback = callback;
	self->callback_data = data;
	self->version = -1;
	self->ref_count = 1;

	return self;
}


/**
 * \brief Free a stream parser and all of its resources
 * \param self A stream parser
 **/
void kfxmpp_stream_parser_free (KfxmppStreamParser *self)
{
	kfxmpp_log ("Freeing parser %p\n", self);
	xmlFreeDoc (self->parser->myDoc);
	xmlFreeParserCtxt (self->parser);
	g_free (self->id);
	g_free (self);
}


/**
 * \brief Add a reference to KfxmppStreamParser
 **/
KfxmppStreamParser* kfxmpp_stream_parser_ref (KfxmppStreamParser *self)
{
        g_return_val_if_fail (self, NULL);
        self->ref_count++;
        return self;
}


/**
 * \brief Remove a reference from KfxmppStreamParser
 *
 * Object will be deleted when reference count reaches 0
 **/
void kfxmpp_stream_parser_unref (KfxmppStreamParser *self)
{
        g_return_if_fail (self);
        self->ref_count--;
        if (self->ref_count == 0)
                kfxmpp_stream_parser_free (self);
}


/**
 * \brief Feed SreamParser with data
 * \param self A stream parser
 * \param data A character data to parse
 * \param len Length of data
 **/
void kfxmpp_stream_parser_feed (KfxmppStreamParser *self, const gchar *data, gsize len)
{
	g_return_if_fail (self);

	/* Pass data to parser */
	xmlParseChunk (self->parser, data, len, 0);

	/* Extract nodes from document */
	xmlNodePtr node = self->nodes;

	while (node) {
		xmlNodePtr tmp = node->next;

		kfxmpp_log ("parser: found <%s/>\n", node->name);

		/* Remove it from xml tree, so it doesn't mess up memory */
		xmlUnlinkNode (node);

		/* Inform that we have found a node */
		if (self->callback)
			self->callback (self, node, self->callback_data);

		/* Delete that node */
		xmlFreeNode (node);

		node = tmp;
	}
	self->nodes = NULL;
}


/**
 * \brief Get major version number of parsed stream
 **/
gint kfxmpp_stream_parser_get_version (KfxmppStreamParser *self)
{
	return self->version;
}


/**
 * \brief Get ID of stream being parsed
 **/
const gchar *kfxmpp_stream_parser_get_id (KfxmppStreamParser *self)
{
	return self->id;
}


/**
 * \brief Set callback called when stream is opened
 **/
void kfxmpp_stream_parser_set_stream_callback (KfxmppStreamParser *parser, KfxmppStreamParserStreamCallback callback)
{
	g_return_if_fail (parser);

	parser->stream_callback = callback;
}


/***********************************************************************
 *
 * SAX handlers
 * ------------
 *  Basically following functions are wrappers around libxml functions.
 * Probably many of them are not needed, some even are never called.
 * Thus, this code can be optimized a lot.
 *
 */

static void onInternalSubset (void * ctx, const xmlChar * name, const xmlChar * ExternalID, const xmlChar * SystemID)
{
	xmlSAX2InternalSubset (((KfxmppStreamParser *) ctx)->parser, name, ExternalID, SystemID);
}

static int onIsStandalone (void * ctx)
{
	return xmlSAX2IsStandalone (((KfxmppStreamParser *) ctx)->parser);
}

static int onHasInternalSubset (void * ctx)
{
	return xmlSAX2HasInternalSubset (((KfxmppStreamParser *) ctx)->parser);
}

static int onHasExternalSubset (void * ctx)
{
	return xmlSAX2HasExternalSubset (((KfxmppStreamParser *) ctx)->parser);
}

static xmlParserInputPtr onResolveEntity (void * ctx, const xmlChar * publicId, const xmlChar * systemId)
{
	return xmlSAX2ResolveEntity (((KfxmppStreamParser *) ctx)->parser, publicId, systemId);
}

static xmlEntityPtr onGetEntity (void * ctx, const xmlChar * name)
{
	return xmlSAX2GetEntity (((KfxmppStreamParser *) ctx)->parser, name);
}

static void onEntityDecl (void * ctx, const xmlChar * name, int type, const xmlChar * publicId, const xmlChar * systemId, xmlChar * content)
{
	xmlSAX2EntityDecl (((KfxmppStreamParser *) ctx)->parser, name, type, publicId, systemId, content);
}

static void onNotationDecl (void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId)
{
	xmlSAX2NotationDecl (((KfxmppStreamParser *) ctx)->parser, name, publicId, systemId);
}

static void onAttributeDecl (void * ctx, const xmlChar * elem, const xmlChar * fullname, int type, int def, const xmlChar * defaultValue, xmlEnumerationPtr tree)
{
	xmlSAX2AttributeDecl (((KfxmppStreamParser *) ctx)->parser, elem, fullname, type, def, defaultValue, tree);
}

static void onElementDecl (void * ctx, const xmlChar * name, int type, xmlElementContentPtr content)
{
	xmlSAX2ElementDecl (((KfxmppStreamParser *) ctx)->parser, name, type, content);
}

static void onUnparsedEntityDecl (void * ctx, const xmlChar * name, const xmlChar * publicId, const xmlChar * systemId, const xmlChar * notationName)
{
	xmlSAX2UnparsedEntityDecl (((KfxmppStreamParser *) ctx)->parser, name, publicId, systemId, notationName);
}

static void onSetDocumentLocator (void * ctx, xmlSAXLocatorPtr loc)
{
	xmlSAX2SetDocumentLocator (((KfxmppStreamParser *) ctx)->parser, loc);
}

static void onStartDocument (void * ctx)
{
	xmlSAX2StartDocument (((KfxmppStreamParser *) ctx)->parser);
}

static void onEndDocument (void * ctx)
{
	xmlSAX2EndDocument (((KfxmppStreamParser *) ctx)->parser);
}

static void onReference (void * ctx, const xmlChar * name)
{
	xmlSAX2Reference (((KfxmppStreamParser *) ctx)->parser, name);
}

static void onCharacters (void * ctx, const xmlChar * ch, int len)
{
	xmlSAX2Characters (((KfxmppStreamParser *) ctx)->parser, ch, len);
}

static void onIgnorableWhitespace (void * ctx, const xmlChar * ch, int len)
{
	xmlSAX2IgnorableWhitespace (((KfxmppStreamParser *) ctx)->parser, ch, len);
}

static void onProcessingInstruction (void * ctx, const xmlChar * target, const xmlChar * data)
{
	xmlSAX2ProcessingInstruction (((KfxmppStreamParser *) ctx)->parser, target, data);
}

static void onComment (void * ctx, const xmlChar * value)
{
	xmlSAX2Comment (((KfxmppStreamParser *) ctx)->parser, value);
}

/* Those two functions are really needed :-) */

/**
 * \brief SAX callback called when opening tag is detected
 **/
static void onStartElement (void * ctx, const xmlChar * name, const xmlChar **attrs)
{
	KfxmppStreamParser *self = ctx;
	xmlSAX2StartElement (self->parser, name, attrs);

	/* Note that with opening tag depth of processed
	 * XML stream increases */
	self->depth++;

	if (self->depth == 1) {
		/* <stream> tag. Scan for version and id attributes */
		int i;
		self->version = 0;

		for (i = 0; attrs[i]; i += 2) {
			if (xmlStrcmp (attrs[i], "version") == 0) {
				/* Version attribute */
				self->version = atoi (attrs[i+1]);
			} else if (xmlStrcmp (attrs[i], "id") == 0) {
				/* ID attribute */
				self->id = g_strdup (attrs[i+1]);
			}
		}

		/* Report that tag to user */
		if (self->stream_callback)
			self->stream_callback (self, self->version, self->id, self->callback_data);
	}
}


/**
 * \brief SAX callback called when closing tag is detected
 **/
static void onEndElement (void * ctx, const xmlChar * name)
{
	KfxmppStreamParser *self = ctx;
	xmlNodePtr node = NULL;
	
	/* Depth has decreased with closing tag */
	--(self->depth);
	
	if (self->depth == 1) {
		/* This is second top-level node */
		node = self->parser->node;
	}
	
	xmlSAX2EndElement (self->parser, name);

	if (node) {
		/* We found node */
		kfxmpp_stream_parser_ref (self);

		/* Remove it from xml tree, so it doesn't mess up memory */
		xmlUnlinkNode (node);

		/* Store node */
		if (self->nodes)
			xmlAddSibling (self->nodes, node);
		else
			self->nodes = node;
		
		/* Inform that we have found a node */
//		if (self->callback)
//			self->callback (self, node, self->callback_data);

		/* Delete that node */
//		xmlFreeNode (node);

		kfxmpp_stream_parser_unref (self);
	}
}


