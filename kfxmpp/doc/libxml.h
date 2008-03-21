//@{ \name Namespaces
/**
 * \brief Set namespace for a node
 **/
xmlNsPtr	xmlNewNs		(xmlNodePtr node, 
					 const xmlChar * href, 
					 const xmlChar * prefix);
//@}

//@{ \name Nodes
/**
 * \brief Add a sub-node
 **/
xmlNodePtr	xmlNewChild		(xmlNodePtr parent, 
					 xmlNsPtr ns, 
					 const xmlChar * name, 
					 const xmlChar * content);
/**
 * \brief Add a sub-node
 **/
xmlNodePtr	xmlNewTextChild		(xmlNodePtr parent, 
					 xmlNsPtr ns, 
					 const xmlChar * name, 
					 const xmlChar * content);
//@}

//@{ \name Properties
/**
 * \brief Get attribute
 **/
xmlChar *	xmlGetProp		(xmlNodePtr node, 
					 const xmlChar * name);
/**
 * \brief Set attribute
 **/
xmlAttrPtr	xmlSetProp		(xmlNodePtr node, 
					 const xmlChar * name, 
					 const xmlChar * value);
//@}
