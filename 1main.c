#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <semaphore.h> 
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "linux_nfc_api.h"
#include "tools.h"

static sem_t sem; 
static int mTagHandle; 
static nfc_tag_info_t TagInfo;

pthread_mutex_t end_mutex;
pthread_cond_t end_cv; 

int running = 1;
/* Callback for the tag arrival event */ 
void onTagArrival (nfc_tag_info_t *pTag) 
{ 
 mTagHandle = pTag->handle; 
 sem_post(&sem); 

 memcpy(&TagInfo, pTag, sizeof(nfc_tag_info_t));
} 

 

/* Callback for the tag departure event */ 
void onTagDeparture (void ){} 


void PrintfNDEFInfo(ndef_info_t pNDEFinfo)
{
	if(0x01 == pNDEFinfo.is_ndef)
	{
		printf("\t\tRecord Found :\n");
		printf("\t\t\t\tNDEF Content Max size : 	'%d bytes'\n", pNDEFinfo.max_ndef_length);
		printf("\t\t\t\tNDEF Actual Content size : 	'%d bytes'\n", pNDEFinfo.current_ndef_length);
		if(0x01 == pNDEFinfo.is_writable)
		{
			printf("\t\t\t\tReadOnly :                      'FALSE'\n");
		}
		else
		{
			printf("\t\t\t\tReadOnly : 					    'TRUE'\n");
		}
	}
	else	
	{
		printf("\t\tNo Record found\n");
	}
}

void PrintNDEFContent(nfc_tag_info_t* TagInfo, ndef_info_t* NDEFinfo, unsigned char* ndefRaw, unsigned int ndefRawLen)
{
	unsigned char* NDEFContent = NULL;
	nfc_friendly_type_t lNDEFType = NDEF_FRIENDLY_TYPE_OTHER;
	unsigned int res = 0x00;
	unsigned int i = 0x00;
	char* TextContent = NULL;
	char* URLContent = NULL;
	if(NULL != NDEFinfo)
	{
		ndefRawLen = NDEFinfo->current_ndef_length;
		NDEFContent = malloc(ndefRawLen * sizeof(unsigned char));
		res = nfcTag_readNdef(TagInfo->handle, NDEFContent, ndefRawLen, &lNDEFType);
	}
	else if (NULL != ndefRaw && 0x00 != ndefRawLen)
	{
		NDEFContent = malloc(ndefRawLen * sizeof(unsigned char));
		memcpy(NDEFContent, ndefRaw, ndefRawLen);
		res = ndefRawLen;
		if((NDEFContent[0] & 0x7) == NDEF_TNF_WELLKNOWN && 0x55 == NDEFContent[3])
		{
			lNDEFType = NDEF_FRIENDLY_TYPE_URL;
		}
		if((NDEFContent[0] & 0x7) == NDEF_TNF_WELLKNOWN && 0x54 == NDEFContent[3])
		{
			lNDEFType = NDEF_FRIENDLY_TYPE_TEXT;
		}
	}
	else
	{
		printf("\t\t\t\tError : Invalid Parameters\n");
	}
	
	if(res != ndefRawLen)
	{
		printf("\t\t\t\tRead NDEF Content Failed\n");
	}
	else
	{
		switch(lNDEFType)
		{
			case NDEF_FRIENDLY_TYPE_TEXT:
			{
				TextContent = malloc(res * sizeof(char));
				res = ndef_readText(NDEFContent, res, TextContent, res);
				if(0x00 == res)
				{
					printf("\t\t\t\tType : 				'Text'\n");
					printf("\t\t\t\tText : 				'%s'\n\n", TextContent);
				}
				else
				{
					printf("\t\t\t\tRead NDEF Text Error\n");
				}
				if(NULL != TextContent)
				{
					free(TextContent);
					TextContent = NULL;
				}
			} break;	
			default:
			{
			} break;
		}
		printf("\n\t\t%d bytes of NDEF data received :\n\t\t", ndefRawLen);
		for(i = 0x00; i < ndefRawLen; i++)
		{
			printf("%02X ", NDEFContent[i]);
			if(i%30 == 0 && 0x00 != i)
			{
				printf("\n\t\t");
			}
		}
		printf("\n\n");
	}
	
	if(NULL != NDEFContent)
	{
		free(NDEFContent);
		NDEFContent = NULL;
	}
}

void *polling()
{
 while(running)
 {
 	/* Initialize variables */ 

 sem_init(&sem, 0, 0); 
 nfcTagCallback_t tagCb; 
 tagCb.onTagArrival = onTagArrival; 
 tagCb.onTagDepature = onTagDeparture;


 

 /* Initialize stack */ 
 nfcManager_doInitialize (); 
 nfcManager_registerTagCallback(&tagCb); 
 nfcManager_enableDiscovery(DEFAULT_NFA_TECH_MASK, 1, 0, 0); 

 

 /* Wait for tag */ 
 printf("Waiting for an NFC tag...\n\n"); 
 sem_wait(&sem); 
 /* Check if the tag contains an NDEF message */ 
 printf("Tag found!\n"); 
 ndef_info_t info; 
 memset(&info, 0, sizeof(ndef_info_t)); 
 nfcTag_isNdef(mTagHandle, &info); 
 if (info.is_ndef){ 
 printf("The tag contains an NDEF message\n\n");
 printf("PrintNDEFInfo:\n");
 PrintfNDEFInfo(info);
 printf("PrintNDEFContent:\n");
 PrintNDEFContent(&TagInfo, &info, NULL, 0x00);

 }
 else{ 
 printf("The tag does not contain an NDEF message\n\n"); 
 }

 

 /* Deinitialize stack */ 
 sem_destroy(&sem); 
 nfcManager_disableDiscovery(); 
 nfcManager_deregisterTagCallback(); 
 nfcManager_doDeinitialize ();
 }

pthread_exit(NULL);
}

 
void *checkEnd()
{
	fd_set rfds;

	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = 1;

	char buf;
	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);

	while (running)
	{
		tv.tv_usec = 0;
		tv.tv_sec = 1;

		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		
		/*	block until input, timeout to check for gameover.*/
		select(1, &rfds, NULL, NULL, &tv);
		
		if (FD_ISSET(STDIN_FILENO, &rfds))
		{
			buf = getchar();
			if (buf == '\n')
			{
				pthread_mutex_lock(&end_mutex);
				pthread_cond_broadcast(&end_cv);
				pthread_mutex_unlock(&end_mutex);
				running = 0;
			}			
		}		
		
	}

	printf("checkEndEnding\n");
	pthread_exit(NULL);
}

/* Main function: waits for an NFC tag and shows if it contains an NDEF 
message*/ 
int main(int argc, char *argv[]) 
{ 
 pthread_t id1;
 pthread_t id2;
 pthread_mutex_init(&end_mutex, NULL);
 pthread_cond_init(&end_cv, NULL);

 printf("\n########## Our first Linux libnfc-nci application ##########\n\n"); 

 pthread_create(&id1, NULL, checkEnd, NULL);
 pthread_create(&id2, NULL, polling, NULL);
 pthread_mutex_lock(&end_mutex);
 printf("Waiting for checkEnd\n");
 pthread_cond_wait(&end_cv, &end_mutex);
 pthread_mutex_unlock(&end_mutex);
 pthread_join(id2, NULL);
 pthread_join(id1, NULL);
 printf("Exiting...\n");
 



 

 return 0; 
} 