#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "xmlfunctions.h"

void parseCustomer (xmlDocPtr doc, xmlNodePtr cur) {

	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"CID"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    printf("%s %s\n",(const xmlChar*)cur->name, key);
		    xmlFree(key);
 	    }
            else if ((!xmlStrcmp(cur->name, (const xmlChar *)"Valid"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    printf("%s %s\n",(const xmlChar*)cur->name, key);
		    xmlFree(key);
 	    }
            else if ((!xmlStrcmp(cur->name, (const xmlChar *)"Dev"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    printf("%s %s\n",(const xmlChar*)cur->name, key);
		    xmlFree(key);
 	    }
            else if ((!xmlStrcmp(cur->name, (const xmlChar *)"Balance"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    printf("%s %s\n",(const xmlChar*)cur->name, key);
		    xmlFree(key);
 	    }
            else if ((!xmlStrcmp(cur->name, (const xmlChar *)"Email"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    printf("%s %s\n",(const xmlChar*)cur->name, key);
		    xmlFree(key);
 	    }
            else if ((!xmlStrcmp(cur->name, (const xmlChar *)"Valid"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    printf("%s %s\n",(const xmlChar*)cur->name, key);
		    xmlFree(key);
 	    }
            else if ((!xmlStrcmp(cur->name, (const xmlChar *)"LastUpdated"))) {
		    key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		    printf("%s %s\n",(const xmlChar*)cur->name, key);
		    xmlFree(key);
 	    }
	cur = cur->next;
	}
    return;
}

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

void parseDoc(char *docname) {

	xmlDocPtr doc;
	xmlNodePtr cur;
        //char* test = "<Data><Customer><CID>158358493</CID><Valid>1</Valid><Dev>0</Dev><Balance>65</Balance><Email>villard@myumanitoba.ca</Email><LastUpdated>2016-01-29 10:45:51.0</LastUpdated></Customer></Data>";
	doc = xmlParseFile(docname);
	//doc = xmlParseMemory(test, 4096);
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
			parseCustomer (doc, cur);
                       printf("\n");
		}
		 
           cur = cur->next;
       
       
       }
//         char buf[BUFSIZE];
// //        cur = xmlDocGetRootElement(doc);
// //        cur = cur->xmlChildrenNode;
// //	while (cur != NULL) {
// //		if ((!xmlStrcmp(cur->name, (const xmlChar *)"Customer"))){
// //			updateEntry(doc, cur, "1044419670", "Balance", "333.25");
// //		}
// //		 
// //            cur = cur->next;
// //        
// //        }
//         bzero(buf, BUFSIZE);
//         cur = xmlDocGetRootElement(doc);
//         cur = cur->xmlChildrenNode;
//         while (cur != NULL) {
// 		if ((!xmlStrcmp(cur->name, (const xmlChar *)"Customer"))){
// 			getEntry(doc, cur, "1044419670", "Balance", buf);
// 		}
		 
//             cur = cur->next;
        
//         }
//         printf("BUF: %s\n", buf);
	xmlSaveFormatFile (docname, doc, 1);
	xmlFreeDoc(doc);
	return;
}
