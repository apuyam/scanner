/* libxml functions based on examples from
http://xmlsoft.org/tutorial/*/
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "tcp.h"
#include "functions.h"

/*looks for the val field under cid and changes it to buf*/
int updateEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf);

/*looks for the val field under cid and stores it in buf*/
int getEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf);

/*from file docname, if func = 1, will update cid's val with buf.
if func = 0, will store cid's val in buf.*/
int xmlWrapper(char* docname, int func, char* cid, char* val, char* buf);

/*from file docname, uses xmlWrapper, getEntry, and updateEntry to get a cid's cached balance
and add delta to it*/
int xmlUpdateBalance(char* docname, char* cid, float delta);

/*checks docname if cid is in the cache. returns 1 if found, 0 otherwise*/
int verifyCID(char* docname, char* cid);