/* libxml functions based on examples from
http://xmlsoft.org/tutorial/*/

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "xmlfunctions.h"

int updateEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf)
{
    int found = 0;
        xmlChar *key;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {   
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"CID"))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if((!xmlStrcmp(key, (const xmlChar *)cid)))
                    {
                        printf("Found CID: %s\n", key);
                        found = 1;
                        
                        while (xmlStrcmp(cur->name, (const xmlChar *)val))
                        {
                            cur = cur->next;
                        }
                        printf("Updating %s entry: %s\n", val, buf);
                        xmlNodeSetContent(cur, (const xmlChar *)buf);                      
                        
                    }
            xmlFree(key);
        }
            cur = cur->next;
        }
        return found;
}

int getEntry(xmlDocPtr doc, xmlNodePtr cur, char* cid, char* val, char* buf)
{
    int found = 0;
        xmlChar *key;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {   
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"CID"))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if((!xmlStrcmp(key, (const xmlChar *)cid)))
                    {
                        printf("Found CID: %s\n", key);
                        found = 1;
                        
                        if ((!xmlStrcmp((const xmlChar*)cid, (const xmlChar *)val)))
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
                            strncpy(buf, (const char*)ent, strlen((const char*)ent));
                            buf[strlen((const char*)ent)] = '\0';
                            printf("Found: %s\n", buf);
                        }
                        
                    }
            xmlFree(key);
        }
            cur = cur->next;
        }
        return found;
}

int xmlWrapper(char* docname, int func, char* cid, char* val, char* buf)
{
    int result = 0;
    xmlDocPtr doc;
    xmlNodePtr cur;
    doc = xmlParseFile(docname);
    if (doc == NULL ) {
            fprintf(stderr,"Document not parsed successfully. \n");
            return 0;
    }

    cur = xmlDocGetRootElement(doc);

    if (cur == NULL) {
            fprintf(stderr,"empty document\n");
            xmlFreeDoc(doc);
            return 0;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) "Data")) {
            fprintf(stderr,"document of the wrong type, root node != Data");
            xmlFreeDoc(doc);
            return 0;
    }
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
            if ((!xmlStrcmp(cur->name, (const xmlChar *)"Customer"))){
                if (func == 0)
                {
                    if(getEntry(doc, cur, cid, val, buf))
                    {
                        result = 1;
                    }
                }
                else if (func == 1)
                {
                    if(updateEntry(doc, cur, cid, val, buf))
                    {
                        result = 1;
                    }
                }

            }

        cur = cur->next;

    }
    xmlSaveFormatFile (docname, doc, 1);
    xmlFreeDoc(doc);
    
    return result;
    
}

int xmlUpdateBalance(char* docname, char* cid, float delta)
{
    int result = 0;
    char* buf = malloc(BUFSIZE);
    result = xmlWrapper(docname, 0, cid, "Balance", buf);
    if (result)
    {
        printf("%s\n", buf);
        float bal;
        strToFloat(buf, &bal);
        bal += delta;
        floatToStr(bal, buf);
        printf("%s\n", buf);
        xmlWrapper(docname, 1, cid, "Balance", buf);
        printf("%s\n", buf);
    }
    else
    {
        printf("Error: CID not found\n");
    }
    
    free(buf);
    return result;
}

int verifyCID(char* docname, char* cid)
{
    int found = 0;
    char* cidcheck = malloc(BUFSIZE);
    if(xmlWrapper(docname, 0, cid, "CID", cidcheck))
    {
        printf("CID %s found in DB\n", cidcheck);
        found = 1;
    }
    else
    {
        printf("Error: CID %s not found in DB\n", cid);
    }
    free(cidcheck);
    
    return found;
}