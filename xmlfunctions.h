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
void updateEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf);

/*looks for the val field under cid and stores it in buf*/
void getEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf);

/*if func = 1, will update cid's val with buf.
if func = 0, will store cid's val in buf.*/
void xmlWrapper(char* docname, int func, char* cid, char* val, char* buf);

/*uses xmlWrapper, getEntry, and updateEntry to get a cid's cached balance
and add delta to it*/
void xmlUpdateBalance(char* docname, char* cid, float delta);