#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "xmlfunctions.h"

void updateEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf)
{
        xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {   
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"CID"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    if(strcmp(key, cid) == 0)
                    {
                        printf("Found CID: %s\n", key);
                        
                        while (xmlStrcmp(cur->name, (const xmlChar *)val))
                        {
                            cur = cur->next;
                        }
                        printf("Updating cache balance: %s\n", buf);
                        xmlNodeSetContent(cur, (const xmlChar *)buf);                      
                        
                    }
		    xmlFree(key);
 	    }
            cur = cur->next;
        }
        return;
}

void getEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf)
{
        xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {   
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"CID"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    if(strcmp(key, cid) == 0)
                    {
                        printf("Found CID: %s\n", key);
                        
                        if (!strcmp(cid, val))
                        {
                            strncpy(buf, val, strlen(val));
                            buf[strlen(val)] = '\0';
                            printf("Found: %s\n", buf);
                        }
                        else
                        {
                            
                            while (xmlStrcmp(cur->name, (const xmlChar *)val))
                            {
                                cur = cur->next;
                            }
                            xmlChar* ent = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                            strncpy(buf, ent, strlen(ent));
                            buf[strlen(ent)] = '\0';
                            printf("Found: %s\n", buf);
                        }
                        
                    }
		    xmlFree(key);
 	    }
            cur = cur->next;
        }
        return;
}

void xmlWrapper(char* docname, int func, char* cid, char* val, char* buf)
{
    xmlDocPtr doc;
    xmlNodePtr cur;
    doc = xmlParseFile(docname);
    if (doc == NULL ) {
            fprintf(stderr,"Document not parsed successfully. \n");
            return;
    }

    cur = xmlDocGetRootElement(doc);

    if (cur == NULL) {
            fprintf(stderr,"empty document\n");
            xmlFreeDoc(doc);
            return;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) "Data")) {
            fprintf(stderr,"document of the wrong type, root node != Data");
            xmlFreeDoc(doc);
            return;
    }
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"Customer"))){
                if (func == 0)
                {
                    getEntry(doc, cur, cid, val, buf);
                }
                else if (func == 1)
                {
                    updateEntry(doc, cur, cid, val, buf);
                }

            }

        cur = cur->next;

    }
    xmlSaveFormatFile (docname, doc, 1);
    xmlFreeDoc(doc);
    
}

void xmlUpdateBalance(char* docname, char* cid, float delta)
{
    char* buf = malloc(BUFSIZE);
    xmlWrapper(docname, 0, cid, "Balance", buf);
	printf("%s\n", buf);
    float bal;
    strToFloat(buf, &bal);
    bal += delta;
    floatToStr(bal, buf);
	printf("%s\n", buf);
    xmlWrapper(docname, 1, cid, "Balance", buf);

    printf("%s\n", buf);
    free(buf);
}