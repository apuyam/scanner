#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "tcp.h"

void parseCustomer (xmlDocPtr doc, xmlNodePtr cur);

void updateEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf);

void getEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf);
void parseDoc(char *docname);