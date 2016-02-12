/******************************************************************************
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License")
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "linux_nfc_api.h"
#include "tools.h"
#include "payload.h"
#include "tcp.h"
#include "functions.h"
//#include "wiringPi.h"

typedef enum eDevState
{
	eDevState_NONE,
	eDevState_WAIT_ARRIVAL, 
	eDevState_PRESENT, 
	eDevState_WAIT_DEPARTURE,
	eDevState_DEPARTED,
	eDevState_EXIT
}eDevState;

typedef enum eSnepClientState
{
	eSnepClientState_WAIT_OFF,
	eSnepClientState_OFF,
	eSnepClientState_WAIT_READY,
	eSnepClientState_READY,
	eSnepClientState_EXIT
}eSnepClientState;

typedef enum eHCEState
{
	eHCEState_NONE,
	eHCEState_WAIT_DATA,
	eHCEState_DATA_RECEIVED,
	eHCEState_EXIT
}eHCEState;

typedef enum eDevType
{
	eDevType_NONE,
	eDevType_TAG,
	eDevType_P2P,
	eDevType_READER
}eDevType;

typedef enum T4T_NDEF_EMU_state_t
{
	Ready,
	NDEF_Application_Selected,
	CC_Selected,
	NDEF_Selected
} T4T_NDEF_EMU_state_t;

static void* g_ThreadHandle = NULL;
static void* g_devLock = NULL;
static void* g_SnepClientLock = NULL;
static void* g_HCELock = NULL;
static eDevState g_DevState = eDevState_NONE;
static eDevType g_Dev_Type = eDevType_NONE;
static eSnepClientState g_SnepClientState = eSnepClientState_OFF;
static eHCEState g_HCEState = eHCEState_NONE;
static nfc_tag_info_t g_TagInfo;
static nfcTagCallback_t g_TagCB;
static nfcHostCardEmulationCallback_t g_HceCB;
static nfcSnepServerCallback_t g_SnepServerCB;
static nfcSnepClientCallback_t g_SnepClientCB;
unsigned char *HCE_data = NULL;
unsigned int HCE_dataLenght = 0x00;


void help(int mode);
int InitEnv();
int LookForTag(char** args, int args_len, char* tag, char** data, int format);
void PrintNDEFContent(nfc_tag_info_t* TagInfo, ndef_info_t* NDEFinfo, unsigned char* ndefRaw, unsigned int ndefRawLen);
int BuildNDEFMessage(int arg_len, char** arg, unsigned char** outNDEFBuffer, unsigned int* outNDEFBufferLen);

void onDataReceived(unsigned char *data, unsigned int data_length)
{
	framework_LockMutex(g_HCELock);
	
	HCE_dataLenght = data_length;
	HCE_data = malloc(HCE_dataLenght * sizeof(unsigned char));
	memcpy(HCE_data, data, data_length);
	
	if(eHCEState_NONE == g_HCEState)
	{
		g_HCEState = eHCEState_DATA_RECEIVED;
	}
	else if (eHCEState_WAIT_DATA == g_HCEState)
	{
		g_HCEState = eHCEState_DATA_RECEIVED;
		framework_NotifyMutex(g_HCELock, 0);
	}
	
	framework_UnlockMutex(g_HCELock);
}
 
void onTagArrival(nfc_tag_info_t *pTagInfo)
{
	framework_LockMutex(g_devLock);
	
	if(eDevState_WAIT_ARRIVAL == g_DevState)
	{	
		printf("\tNFC Tag Found\n\n");
		memcpy(&g_TagInfo, pTagInfo, sizeof(nfc_tag_info_t));
		g_DevState = eDevState_PRESENT;
		g_Dev_Type = eDevType_TAG;
		framework_NotifyMutex(g_devLock, 0);
	}
	else if(eDevState_WAIT_DEPARTURE == g_DevState)
	{	
		memcpy(&g_TagInfo, pTagInfo, sizeof(nfc_tag_info_t));
		g_DevState = eDevState_PRESENT;
		g_Dev_Type = eDevType_TAG;
		framework_NotifyMutex(g_devLock, 0);
	}
	else if(eDevState_EXIT == g_DevState)
	{
		g_DevState = eDevState_DEPARTED;
		g_Dev_Type = eDevType_NONE;
		framework_NotifyMutex(g_devLock, 0);
	}
	else
	{
		g_DevState = eDevState_PRESENT;
		g_Dev_Type = eDevType_TAG;
	}

	framework_UnlockMutex(g_devLock);
}

void onTagDeparture(void)
{	
	framework_LockMutex(g_devLock);
	
	
	if(eDevState_WAIT_DEPARTURE == g_DevState)
	{
		printf("\tNFC Tag Lost\n\n");
		g_DevState = eDevState_DEPARTED;
		g_Dev_Type = eDevType_NONE;
		framework_NotifyMutex(g_devLock, 0);
	}
	else if(eDevState_WAIT_ARRIVAL == g_DevState)
	{
	}	
	else if(eDevState_EXIT == g_DevState)
	{
	}
	else
	{
		g_DevState = eDevState_DEPARTED;
		g_Dev_Type = eDevType_NONE;
	}
	framework_UnlockMutex(g_devLock);
}

void onDeviceArrival (void)
{
	framework_LockMutex(g_devLock);
	
	switch(g_DevState)
	{
		case eDevState_WAIT_DEPARTURE:
		{
			g_DevState = eDevState_PRESENT;
			g_Dev_Type = eDevType_P2P;
			framework_NotifyMutex(g_devLock, 0);
		} break;
		case eDevState_EXIT:
		{
			g_Dev_Type = eDevType_P2P;
		} break;
		case eDevState_NONE:
		{
			g_DevState = eDevState_PRESENT;
			g_Dev_Type = eDevType_P2P;
		} break;
		case eDevState_WAIT_ARRIVAL:
		{
			g_DevState = eDevState_PRESENT;
			g_Dev_Type = eDevType_P2P;
			framework_NotifyMutex(g_devLock, 0);
		} break;
		case eDevState_PRESENT:
		{
			g_Dev_Type = eDevType_P2P;
		} break;
		case eDevState_DEPARTED:
		{
			g_Dev_Type = eDevType_P2P;
			g_DevState = eDevState_PRESENT;
		} break;
	}
	
	framework_UnlockMutex(g_devLock);
}


void onDeviceDeparture (void)
{
	framework_LockMutex(g_devLock);
	
	switch(g_DevState)
	{
		case eDevState_WAIT_DEPARTURE:
		{
			g_DevState = eDevState_DEPARTED;
			g_Dev_Type = eDevType_NONE;
			framework_NotifyMutex(g_devLock, 0);
		} break;
		case eDevState_EXIT:
		{
			g_Dev_Type = eDevType_NONE;
		} break;
		case eDevState_NONE:
		{
			g_Dev_Type = eDevType_NONE;
		} break;
		case eDevState_WAIT_ARRIVAL:
		{
			g_Dev_Type = eDevType_NONE;
		} break;
		case eDevState_PRESENT:
		{
			g_Dev_Type = eDevType_NONE;
			g_DevState = eDevState_DEPARTED;
		} break;
		case eDevState_DEPARTED:
		{
			g_Dev_Type = eDevType_NONE;
		} break;
	}

	framework_UnlockMutex(g_devLock);
	
	framework_LockMutex(g_SnepClientLock);
	
	switch(g_SnepClientState)
	{
		case eSnepClientState_WAIT_OFF:
		{
			g_SnepClientState = eSnepClientState_OFF;
			framework_NotifyMutex(g_SnepClientLock, 0);
		} break;
		case eSnepClientState_OFF:
		{
		} break;
		case eSnepClientState_WAIT_READY:
		{
			g_SnepClientState = eSnepClientState_OFF;
			framework_NotifyMutex(g_SnepClientLock, 0);
		} break;
		case eSnepClientState_READY:
		{
			g_SnepClientState = eSnepClientState_OFF;
		} break;
		case eSnepClientState_EXIT:
		{
		} break;
	}
	
	framework_UnlockMutex(g_SnepClientLock);
}


void onMessageReceived(unsigned char *message, unsigned int length)
{
	//unsigned int i = 0x00;
	printf("\n\t\tNDEF Message Received : \n");
	PrintNDEFContent(NULL, NULL, message, length);
}

void onSnepClientReady()
{
	framework_LockMutex(g_devLock);
	
	switch(g_DevState)
	{
		case eDevState_WAIT_DEPARTURE:
		{
			g_DevState = eDevState_PRESENT;
			g_Dev_Type = eDevType_P2P;
			framework_NotifyMutex(g_devLock, 0);
		} break;
		case eDevState_EXIT:
		{
			g_Dev_Type = eDevType_P2P;
		} break;
		case eDevState_NONE:
		{
			g_DevState = eDevState_PRESENT;
			g_Dev_Type = eDevType_P2P;
		} break;
		case eDevState_WAIT_ARRIVAL:
		{
			g_DevState = eDevState_PRESENT;
			g_Dev_Type = eDevType_P2P;
			framework_NotifyMutex(g_devLock, 0);
		} break;
		case eDevState_PRESENT:
		{
			g_Dev_Type = eDevType_P2P;
		} break;
		case eDevState_DEPARTED:
		{
			g_Dev_Type = eDevType_P2P;
			g_DevState = eDevState_PRESENT;
		} break;
	}

	framework_UnlockMutex(g_devLock);
	
	framework_LockMutex(g_SnepClientLock);
	
	switch(g_SnepClientState)
	{
		case eSnepClientState_WAIT_OFF:
		{
			g_SnepClientState = eSnepClientState_READY;
			framework_NotifyMutex(g_SnepClientLock, 0);
		} break;
		case eSnepClientState_OFF:
		{
			g_SnepClientState = eSnepClientState_READY;
		} break;
		case eSnepClientState_WAIT_READY:
		{
			g_SnepClientState = eSnepClientState_READY;
			framework_NotifyMutex(g_SnepClientLock, 0);
		} break;
		case eSnepClientState_READY:
		{
		} break;
		case eSnepClientState_EXIT:
		{
		} break;
	}
	
	framework_UnlockMutex(g_SnepClientLock);
}

void onSnepClientClosed()
{
	framework_LockMutex(g_devLock);
	
	switch(g_DevState)
	{
		case eDevState_WAIT_DEPARTURE:
		{
			g_DevState = eDevState_DEPARTED;
			g_Dev_Type = eDevType_NONE;
			framework_NotifyMutex(g_devLock, 0);
		} break;
		case eDevState_EXIT:
		{
			g_Dev_Type = eDevType_NONE;
		} break;
		case eDevState_NONE:
		{
			g_Dev_Type = eDevType_NONE;
		} break;
		case eDevState_WAIT_ARRIVAL:
		{
			g_Dev_Type = eDevType_NONE;
		} break;
		case eDevState_PRESENT:
		{
			g_Dev_Type = eDevType_NONE;
			g_DevState = eDevState_DEPARTED;
		} break;
		case eDevState_DEPARTED:
		{
			g_Dev_Type = eDevType_NONE;
		} break;
	}
	framework_UnlockMutex(g_devLock);
	
	framework_LockMutex(g_SnepClientLock);
	
	switch(g_SnepClientState)
	{
		case eSnepClientState_WAIT_OFF:
		{
			g_SnepClientState = eSnepClientState_OFF;
			framework_NotifyMutex(g_SnepClientLock, 0);
		} break;
		case eSnepClientState_OFF:
		{
		} break;
		case eSnepClientState_WAIT_READY:
		{
			g_SnepClientState = eSnepClientState_OFF;
			framework_NotifyMutex(g_SnepClientLock, 0);
		} break;
		case eSnepClientState_READY:
		{
			g_SnepClientState = eSnepClientState_OFF;
		} break;
		case eSnepClientState_EXIT:
		{
		} break;
	}
	
	framework_UnlockMutex(g_SnepClientLock);
}
 
int InitMode(int tag, int p2p, int hce)
{
	int res = 0x00;
	
	g_TagCB.onTagArrival = onTagArrival;
	g_TagCB.onTagDeparture = onTagDeparture;
		
	g_SnepServerCB.onDeviceArrival = onDeviceArrival;
	g_SnepServerCB.onDeviceDeparture = onDeviceDeparture;
	g_SnepServerCB.onMessageReceived = onMessageReceived;
	
	g_SnepClientCB.onDeviceArrival = onSnepClientReady;
	g_SnepClientCB.onDeviceDeparture = onSnepClientClosed;
		
	g_HceCB.onDataReceived = onDataReceived;

	
	if(0x00 == res)
	{
		res = nfcManager_doInitialize();
		if(0x00 != res)
		{
			printf("NfcService Init Failed\n");
		}
	}
	
	if(0x00 == res)
	{
		if(0x01 == tag)
		{
			nfcManager_registerTagCallback(&g_TagCB);
		}
		
		if(0x01 == p2p)
		{
			res = nfcSnep_registerClientCallback(&g_SnepClientCB);
			if(0x00 != res)
			{
				printf("SNEP Client Register Callback Failed\n");
			}
		}
	}
	
	if(0x00 == res && 0x01 == hce)
	{
		nfcHce_registerHceCallback(&g_HceCB);
	}

	if(0x00 == res)
	{

		nfcManager_enableDiscovery(DEFAULT_NFA_TECH_MASK, 0x00, hce, 0);

		if(0x01 == p2p)
		{
			res = nfcSnep_startServer(&g_SnepServerCB);
			if(0x00 != res)
			{
				printf("Start SNEP Server Failed\n");
			}
		}
	}
	
	return res;
}

int DeinitPollMode()
{
	int res = 0x00;
	
	nfcSnep_stopServer();
	
	nfcManager_disableDiscovery();
	
	nfcSnep_deregisterClientCallback();
	
	nfcManager_deregisterTagCallback();
	
	nfcHce_deregisterHceCallback();
	
	res = nfcManager_doDeinitialize();
	
	if(0x00 != res)
	{
		printf("NFC Service Deinit Failed\n");
	}
	return res;
}

int SnepPush(unsigned char* msgToPush, unsigned int len)
{
	int res = 0x00;
	
	framework_LockMutex(g_devLock);
	framework_LockMutex(g_SnepClientLock);
	
	if(eSnepClientState_READY != g_SnepClientState && eSnepClientState_EXIT!= g_SnepClientState && eDevState_PRESENT == g_DevState)
	{
		framework_UnlockMutex(g_devLock);
		g_SnepClientState = eSnepClientState_WAIT_READY;
		framework_WaitMutex(g_SnepClientLock, 0);
	}
	else
	{
		framework_UnlockMutex(g_devLock);
	}
	
	if(eSnepClientState_READY == g_SnepClientState)
	{
		framework_UnlockMutex(g_SnepClientLock);
		res = nfcSnep_putMessage(msgToPush, len);
		
		if(0x00 != res)
		{
			printf("\t\tPush Failed\n");
		}
		else
		{
			printf("\t\tPush successful\n");
		}
	}
	else
	{
		framework_UnlockMutex(g_SnepClientLock);
	}
	
	return res;
}

int WriteTag(nfc_tag_info_t TagInfo, unsigned char* msgToPush, unsigned int len)
{
	int res = 0x00;
	
	res = nfcTag_writeNdef(TagInfo.handle, msgToPush, len);
	if(0x00 != res)
	{
		printf("Write Tag Failed\n");
		res = 0xFF;
	}
	else
	{
		res = 0x00;
	}
	return res;
}


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
		printf("\n");
	}
	
	if(NULL != NDEFContent)
	{
		free(NDEFContent);
		NDEFContent = NULL;
	}
}

/* based on PrintNDEFContent, reads card payload as char* and returns it*/
char* getPayload(nfc_tag_info_t* TagInfo, ndef_info_t* NDEFinfo, unsigned char* ndefRaw, unsigned int ndefRawLen)
{
	unsigned char* NDEFContent = NULL;
	nfc_friendly_type_t lNDEFType = NDEF_FRIENDLY_TYPE_OTHER;
	unsigned int res = 0x00;
	unsigned int i = 0x00;
	char* TextContent = NULL;

	printf("getPayload:\n");
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
					return TextContent;
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
		printf("\n");
	}
	
	if(NULL != NDEFContent)
	{
		free(NDEFContent);
		NDEFContent = NULL;
	}
	return TextContent;
}

/* writes message resp to card */
void writeMessage(char* resp, nfc_tag_info_t TagInfo, unsigned char* NDEFMsg, unsigned int NDEFMsgLen, ndef_info_t NDEFinfo)
{
	int res = 0x00;
	char  arg0[] = "--type=Text";
    char  arg1[] = "-l";
    char  arg2[] = "en";
    char arg3[] = "-r";
    char* arg4 = resp;
    char* argv[] = { &arg0[0], &arg1[0], &arg2[0], &arg3[0], &arg4[0], NULL };
    int   argc   = (int)(sizeof(argv) / sizeof(argv[0])) - 1;

	res = BuildNDEFMessage(argc, &argv[0], &NDEFMsg, &NDEFMsgLen);

	res = WriteTag(TagInfo, NDEFMsg, NDEFMsgLen);

	if(0x00 == res)
	{
		printf("Write Tag OK\n Read back data");
		res = nfcTag_isNdef(TagInfo.handle, &NDEFinfo);
		if(0x01 == res)
		{
			PrintfNDEFInfo(NDEFinfo);
			PrintNDEFContent(&TagInfo, &NDEFinfo, NULL, 0x00);
		}
	}
}

/* assuming decrypted pl, takes payload string pl, extracts info into payload struct, 
subtracts diff from balance, writes new encrypted payload with new balance to card,
sends update to databse*/
void transaction(char* pl, nfc_tag_info_t TagInfo, ndef_info_t NDEFinfo, float delta, int kiosk)
{
	char cid[9];
	char dev;
	char valid;
	char balance[9];
	char timestamp[15];

	/* split payload into strings*/
	strncpy(cid, pl, 8);
	cid[8] = '\0';

	dev = pl[8];

	valid = pl[9];

	strncpy(balance, pl+10, 8);
	balance[8] = '\0';

	strncpy(timestamp, pl+18, 14);
	timestamp[14] = '\0';

	/* convert to proper data types*/
	payload *p = malloc(sizeof(payload));
	p->cid = (int)strtol(cid, NULL, 16);
	p->dev = dev;
	p->valid = valid;
	
	int num;
	sscanf(balance, "%x", &num);
	p->balance = *((float*)&num);


	strncpy(p->timestamp, timestamp, 14);
	printf("CID/Dev/Valid/Balance/Timestamp:\n");
	printf("%i\n%c\n%c\n%f\n%s\n", p->cid, p->dev, p->valid, p->balance, p->timestamp);
	/*before transaction*/

	printf("\nTransaction in progress...\n\n");

	unsigned char * NDEFMsg = NULL;
	unsigned int NDEFMsgLen = 0x00;
	if ((dev == '0') && (valid == '1'))
	{

		//TODO: update timestamp
		//if time difference is more than 1 hr 15, charge balance and write new time stamp
		//else no charge
		double diffTime = 0;
		if (kiosk == 0)
		{
			char* buf[BUFSIZE];
			diffTime = comparePLTime(p->timestamp);
			printf("Time diff (s) %f\n", diffTime);
			if (diffTime > 4500 || TRANSFERSOFF)
			{
				printf("Transfer expired, charging card and setting new timestamp...\n");
				//generate current time string
				char buf[BUFSIZE];
				time_t cur_t;
				time(&cur_t);
				struct tm* tm_new;
				tm_new = localtime(&cur_t);
				createPLTime(tm_new, buf);
				printf("ayay%s\n", buf);

				printf("Change in balance: %f\n", delta);
				p->balance += delta;
				float f = p->balance;
				sprintf(balance, "%X", *((int*)&f) );

				char resp[strlen(pl)];
				strcpy(resp, pl);
				strncpy(resp+10, balance, 8);
				//strncpy(resp+18, buf, 14);
				resp[strlen(pl)] = '\0';

				printf("Writing new balance: %f...\n", f);

				printf("New payload: %s \n", resp);
				encrypt(resp, KEY, PL_LEN);
				writeMessage(resp, TagInfo, NDEFMsg, NDEFMsgLen, NDEFinfo);

			}
			else
			{
				printf("Transfer still valid, not charging card, not changing timestamp...\n");
				delta = 0;		

			}
			printf("Updating database..");

			char msgBuf[BUFSIZE];
			int port = DBPORT;
			char hostname[] = HOSTNAME;

			char* msgParam = malloc(BUFSIZE);
			strcpy(msgBuf, createBalanceUpdate(p->cid, delta, msgParam));
			
			if (MESSAGESON)
			{
				sendMessageToServer(hostname, port, msgBuf);
			}
			else
			{
				printf("MESSAGES OFF: NOT SENDING\n");
			}
			free(msgParam);
		}
		else
		{
			//adding balance
			printf("Adding balance to card...\n");

			printf("Change in balance: %f\n", delta);
			p->balance += delta;
			float f = p->balance;
			sprintf(balance, "%X", *((int*)&f) );

			char resp[strlen(pl)];
			strcpy(resp, pl);
			strncpy(resp+10, balance, 8);
			resp[strlen(pl)] = '\0';

			printf("Writing new balance: %f...\n", f);

			printf("New payload: %s \n", resp);
			encrypt(resp, KEY, PL_LEN);
			writeMessage(resp, TagInfo, NDEFMsg, NDEFMsgLen, NDEFinfo);
		}


		printf("Writing finished.\n");
		//TODO: Green LED
	}
	else
	{
		printf("Invalid\n");
		//TODO: Red LED
	}
}

/*init card with cid and balance*/
void initcard(char* cid, char* balance, nfc_tag_info_t TagInfo, ndef_info_t NDEFinfo)
{

	printf("Initializing Card...\n\n");

	//timestamp should be current time;

	char buf[BUFSIZE];
	time_t cur_t;
	time(&cur_t);
	struct tm* tm_new;
	tm_new = localtime(&cur_t);
	createPLTime(tm_new, buf);
	printf("Timestamp: %s %d\n", buf, strlen(buf));


	unsigned char * NDEFMsg = NULL;
	unsigned int NDEFMsgLen = 0x00;
	char resp[PL_LEN+1];
	strcpy(resp, cid);
	char initString[] = "01432a000020151010242424";
	strncpy(initString+2, balance, 8);
	strncpy(initString+10, buf, 14);
	//ffffffff01432a000020151010242424
	strcpy(resp+8, initString);
	resp[PL_LEN] = '\0';

	encrypt(resp, KEY, PL_LEN);
	printf("Encrypted payload: %s length %d\n", resp, strlen(resp));

	writeMessage(resp, TagInfo, NDEFMsg, NDEFMsgLen, NDEFinfo);

	encrypt(resp, KEY, PL_LEN);
	printf("Decrypted payload: %s length %d\n", resp, strlen(resp));

	printf("\nWriting finished.\n");
}

/*mode = 1 => poll mode = 2 => push mode = 3 => ndef write 4 => HCE*/
int WaitDeviceArrival(int mode, unsigned char* msgToSend, unsigned int len)
{
	int res = 0x00;
	unsigned int i = 0x00;
	//int block = 0x15;
	//unsigned char key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	//ndef_info_t NDEFinfo;
	eDevType DevTypeBck = eDevType_NONE;
	/*Cmd Mifare Auth Key A : 0x60U*/
	// unsigned char MifareAuthCmd[12] = {0x60U, 0x00 /*block*/, 0x02, 0x02, 0x02, 0x02, 0x00 /*key*/, 0x00 /*key*/, 0x00 /*key*/, 0x00 /*key*/ , 0x00 /*key*/, 0x00 /*key*/};
	// unsigned char MifareAuthResp[255];
	// unsigned char MifareAuthRespOK[2] = {0x40, 0x00};
	// unsigned char MifareAuthRespKO[2] = {0x40, 0x03};
	// /*Cmd Mifare Read 16 : 0x30U*/
	// unsigned char MifareReadCmd[2] = {0x30U,  /*block*/ 0x00};
	// unsigned char MifareReadResp[255];
	
	//nfc_tag_info_t TagInfo;
	
	// MifareAuthCmd[1] = block;
	// memcpy(&MifareAuthCmd[6], key, 6);
	// MifareReadCmd[1] = block;

	/* vars for kiosk mode (0x06)*/
	int n;
	int kioskWait = 1;
	char cmd;
	char argu[BUFSIZE];
	char argb[BUFSIZE];
	int serverfd;
	int clientfd;
	int port = 5556;
	int connected = 0;
	//UNCOMMENT message[BUFSIZE] AND COMMENT OUT message = "etc...." IF TESTING TCP
	char message[BUFSIZE];
	//command from GUI converted from bytes
	//char* message = "0FFFFFFFF"; // 0 for init, FFFFFFFF for cid
	//char* message = "142c80000";//1 for add, 42c80000 ($100) for Balance?;
	//char* message = "12.50";
	//char* message = "02147483647";
	if (MESSAGESON && REQTIME)
	{
		printf("Updating time from database...\n");
		char msgBuf[BUFSIZE];
		int port = DBPORT;
		char hostname[] = HOSTNAME;
		char* msgParam = malloc(BUFSIZE);
		strcpy(msgBuf, createGetTime(msgBuf));
		sendMessageToServer(hostname, port, msgBuf);
		char dbTime[23];
		strncpy(dbTime, "\"", 1);
		strncpy(dbTime+1, msgBuf+1, 20);
		strncpy(dbTime+21, "\"", 1);
		dbTime[23] = '\0';
		printf("Time: %s\n", dbTime);
		char cmd[BUFSIZE] = "sudo date -s ";
		strcat(cmd, dbTime);
		printf("%s\n",cmd);
		printf("Setting system time...\n");
		system(cmd);

	}
	if (0x05 == mode && CACHING)
	{
		//TODO: ask for database cache
		printf("Requesting database cache...\n");
		char msgBuf[BUFSIZE];
		int port = DBPORT;
		char hostname[] = HOSTNAME;
		char* msgParam = malloc(BUFSIZE);
		strcpy(msgBuf, createGetEntryAll(msgBuf));
		
		if (MESSAGESON)
		{
			sendMessageToServer(hostname, port, msgBuf);

		}
		else
		{
			printf("MESSAGES OFF: NOT SENDING\n");	
		}

	}
	if (0x06 == mode)
	{
		//initialize server
		serverfd = initServer(serverfd, port);
	}

	do
	{
		ndef_info_t NDEFinfo;
		nfc_tag_info_t TagInfo;
		if (0x06 == mode)
		{
			while (kioskWait)
			{
				//KIOSK SCANNER MODE
				//block for IPC
				printf("Waiting for client...\n");
				if (!connected)
			    {
			      clientfd = acceptClient(clientfd, serverfd, message);
			      connected = 1;
			    }
			    
			    printf("Reading from client...\n");
			    bzero(message, BUFSIZE);
			    n = read(clientfd, message, BUFSIZE);
			    if (n <= 0){ 
			      close(clientfd);
			      connected = 0;
			      printf("Disconnected from client.\n");
			    }//error("ERROR reading from socket");
			    printf("Server received %d bytes: %s \n", n, message);


				// blocks while waiting for GUI

			    if (connected)
			    {
			    	cmd = message[0];
					strcpy(argu, message+1);
					argu[strlen(argu)] = '\0';
					printf("Cmd argument: %s\n", argu);
					if(cmd == (char)-1)
					{
						//kiosk requests disconnection
						close(clientfd);
						connected = 0;
						printf("Disconnected from client by request.\n");
					}
					else
					{
						//sleep(3);
						printf("Resuming...\n");
						kioskWait = 0;
					}
					if(cmd == '2')
					{
						//extract balance from message and save in argb
						strcpy(argb, message+11);
						argb[strlen(argb)] = '\0';
					}
					
					
			    }
				
			}
			
		}
	
		framework_LockMutex(g_devLock);
		if(eDevState_EXIT == g_DevState)
		{
			framework_UnlockMutex(g_devLock);
			break;
		}
		
		else if(eDevState_PRESENT != g_DevState)
		{
			printf("Waiting for a Tag/Device...\n\n");
			g_DevState = eDevState_WAIT_ARRIVAL;
			framework_WaitMutex(g_devLock, 0);
		}
		
		if(eDevState_EXIT == g_DevState)
		{
			framework_UnlockMutex(g_devLock);
			break;
		}
		
		if(eDevState_PRESENT == g_DevState)
		{
			DevTypeBck = g_Dev_Type;
			if(eDevType_TAG == g_Dev_Type)
			{
				memcpy(&TagInfo, &g_TagInfo, sizeof(nfc_tag_info_t));
				framework_UnlockMutex(g_devLock);
				printf("		Type : ");
				switch (TagInfo.technology)
				{
					case TARGET_TYPE_UNKNOWN:
					{
						printf("		'Type Unknown'\n");
					} break;
					case TARGET_TYPE_ISO14443_3A:
					{
						printf("		'Type A'\n");
					} break;
					case TARGET_TYPE_ISO14443_3B:
					{
						printf("		'Type 4B'\n");
					} break;
					case TARGET_TYPE_ISO14443_4:
					{
						printf("		'Type 4A'\n");
					} break;
					case TARGET_TYPE_FELICA:
					{
						printf("		'Type F'\n");
					} break;
					case TARGET_TYPE_ISO15693:
					{
						printf("		'Type V'\n");
					} break;
					case TARGET_TYPE_NDEF:
					{
						printf("		'Type NDEF'\n");
					} break;
					case TARGET_TYPE_NDEF_FORMATABLE:
					{
						printf("		'Type Formatable'\n");
					} break;
					case TARGET_TYPE_MIFARE_CLASSIC:
					{
						printf("		'Type A - Mifare Classic'\n");
					} break;
					case TARGET_TYPE_MIFARE_UL:
					{
						printf("		'Type A - Mifare Ul'\n");
					} break;
					case TARGET_TYPE_KOVIO_BARCODE:
					{
						printf("		'Type A - Kovio Barcode'\n");
					} break;
					case TARGET_TYPE_ISO14443_3A_3B:
					{
						printf("		'Type A/B'\n");
					} break;
					default:
					{
						printf("		'Type %d (Unknown or not supported)'\n", TagInfo.technology);
					} break;
				}
				/*32 is max UID len (Kovio tags)*/
				if((0x00 != TagInfo.uid_length) && (32 >= TagInfo.uid_length))
				{
					if(4 == TagInfo.uid_length || 7 == TagInfo.uid_length || 10 == TagInfo.uid_length)
					{
						printf("		NFCID1 :    \t'");
					}
					else if(8 == TagInfo.uid_length)
					{
						printf("		NFCID2 :    \t'");
					}
					else
					{
						printf("		UID :       \t'");
					}
					
					for(i = 0x00; i < TagInfo.uid_length; i++)
					{
						printf("%02X ", (unsigned char) TagInfo.uid[i]);
					}
					printf("'\n");
				}

				res = nfcTag_isNdef(TagInfo.handle, &NDEFinfo);
				if(0x01 == res)
				{
					PrintfNDEFInfo(NDEFinfo);
					
					PrintNDEFContent(&TagInfo, &NDEFinfo, NULL, 0x00);
					
					if(0x03 == mode)
					{
						res = WriteTag(TagInfo, msgToSend, len);
						if(0x00 == res)
						{
							printf("Write Tag OK\n Read back data");
							res = nfcTag_isNdef(TagInfo.handle, &NDEFinfo);
							if(0x01 == res)
							{
								PrintfNDEFInfo(NDEFinfo);
								PrintNDEFContent(&TagInfo, &NDEFinfo, NULL, 0x00);
							}
						}
					}
					if(0x05 == mode)
					{
						//BUS SCANNER MODE


						char* pl = getPayload(&TagInfo, &NDEFinfo, NULL, 0x00);

						encrypt(pl, KEY, PL_LEN);

						printf("Decrypted: %s\n", pl);

						transaction(pl, TagInfo, NDEFinfo, FEE, 0);
						// for(k = 0; k<2; k++)
						// {
						// 	digitalWrite(28, HIGH);delay(500);
						// }


					}
					if(0x06 == mode && connected)
					{
						//KIOSK SCANNER MODE

						if (cmd == '0')
						{
							//blank card
							printf("Blanking card...\n");
							unsigned char * NDEFMsg = NULL;
							unsigned int NDEFMsgLen = 0x00;
							char blank[] = FORMAT_BLANK;
							encrypt(blank, KEY, strlen(FORMAT_BLANK));
							writeMessage(blank, TagInfo, NDEFMsg, NDEFMsgLen, NDEFinfo);
							n = write(clientfd, "ack0", strlen("ack0"));
    							if (n < 0) 
      								error("ERROR writing to socket");
      						char end[BUFSIZE];
    						end[0] = (char)-128;
    						end[1] = '\0';
							n = write(clientfd, end, strlen(end));
							if (n < 0)
        							error("ERROR writing to socket");
						}
						else if (cmd == '1')
						{
							//Format check
							printf("Checking format...\n");
							char* pl = getPayload(&TagInfo, &NDEFinfo, NULL, 0x00);
							encrypt(pl, KEY, NDEFinfo.current_ndef_length - 7);
							char blankCompare[strlen(FORMAT_BLANK)+1];
							strncpy(blankCompare, pl, strlen(FORMAT_BLANK));
							blankCompare[strlen(FORMAT_BLANK)] = '\0';
							if (strcmp(blankCompare, FORMAT_BLANK) == 0)
							{
								printf("Verified blank card...\n");
								//send "12"
								n = write(clientfd, "ack12", strlen("ack12"));
    							if (n < 0) 
      								error("ERROR writing to socket");
      							char end[BUFSIZE];
    							end[0] = (char)-128;
    							end[1] = '\0';
							    n = write(clientfd, end, strlen(end));
							    if (n < 0)
        							error("ERROR writing to socket");
							}
							else if (NDEFinfo.current_ndef_length == PL_LEN + 7)
							{
								printf("Checking card...\n");
								char cid[9];
						        char dev;
						        char valid;
						        char balance[9];
						        char timestamp[15];
								sscanf(pl, "%8c%c%c%8c%s", cid, &dev, &valid, balance, timestamp);
    							cid[8] = '\0';
    							balance[8] = '\0';
    							timestamp[14] = '\0';
        						printf("Payload: %s %c %c %s %s\n", cid, dev, valid, balance, timestamp);


        						int cidint;
        						cidint = hexStrToInt(cid, &cidint);
        						char cidstr[BUFSIZE];
        						intToStr(cidint, cidstr);

        						char validstr[2];
        						validstr[0] = valid;
        						validstr[1] = '\0';

        						float balfloat;
        						balfloat = hexStrToFloat(balance, &balfloat);
        						char balstr[BUFSIZE];
        						floatToStr(balfloat, balstr);
        						char end[BUFSIZE];
    							end[0] = (char)-128;
    							end[1] = '\0';
							    
							    printf("CID: %s VALID: %s BAL: %s\n", cidstr, validstr, balstr);

							    char resp1[BUFSIZE];
							    bzero(resp1, BUFSIZE);
							    strcat(resp1, "ack13");
							    strcat(resp1, cidstr);
							    strcat(resp1, validstr);
							    strcat(resp1, balstr);
							    strcat(resp1, end);
							    printf("Sending ack: %s\n", resp1);
							    n = write(clientfd, resp1, strlen(resp1));
        						if (n < 0)
        							error("ERROR writing to socket");


							}
							else
							{
								// incorrectly formatted
								printf("Incorrectly formatted card...\n");	
								n = write(clientfd, "ack11", strlen("ack11"));
    							if (n < 0) 
      								error("ERROR writing to socket");
      							char end[BUFSIZE];
    							end[0] = (char)-128;
    							end[1] = '\0';
							    write(clientfd, end, strlen(end));
								
							}
						}
						else if (cmd == '2')
						{
							if (NDEFinfo.current_ndef_length == 7 + strlen(FORMAT_BLANK)) // if blank
							{
								printf("Initializing card...");
								char argcid[10];
								strncpy(argcid, argu, 10);
								int cid;
								strToInt(argcid, &cid);
								printf("CIDINT: %d\n", cid);
								char cidhex[BUFSIZE];
								intToHexStr(cid, cidhex);
								printf("HEXSTR: %s\n", cidhex);
								   
								float balance;
								strToFloat(argb, &balance);
								printf("BALANCE: %f\n", balance);

								char hexbal[9];
								floatToHexStr(balance, hexbal);
								hexbal[8] = '\0';

								//encrypted in initcard
								initcard(cidhex, hexbal, TagInfo, NDEFinfo);
								n = write(clientfd, "ack2", strlen("ack2"));
    							if (n < 0) 
      								error("ERROR writing to socket");
      							char end[BUFSIZE];
    							end[0] = (char)-128;
    							end[1] = '\0';
							    write(clientfd, end, strlen(end));	
							}
							else
							{
								//don't overwrite
								printf("Error: Card already initilized.\n" );
								//give option to erase?
							}
						}
						else if (cmd == '3')
						{
							printf("Adding balance to card...\n");
							char* pl = getPayload(&TagInfo, &NDEFinfo, NULL, 0x00);
							encrypt(pl, KEY, PL_LEN);
							float credit;
							strToFloat(argu, &credit);
							transaction(pl, TagInfo, NDEFinfo, credit, 1);

							n = write(clientfd, "ack3", strlen("ack3"));
    							if (n < 0) 
      								error("ERROR writing to socket");	
      						char end[BUFSIZE];
    						end[0] = (char)-128;
    						end[1] = '\0';
							write(clientfd, end, strlen(end));

						}
						//else if etc....		
						kioskWait = 1;
					}
				}
				else
				{

					//HCE SCANNER
					printf("\t\tNDEF Content : NO\n");
					
					unsigned char SelectAIDCommand[10] = {0x00, 0xA4, 0x04, 0x00, 0x05, 0x01, 0x23, 0x45, 0x67, 0x89};
					unsigned char SelectAIDResponse[255];

					memset(SelectAIDResponse, 0x00, 255);

					memcpy(&TagInfo, &g_TagInfo, sizeof(nfc_tag_info_t));
					res = nfcTag_transceive(TagInfo.handle, SelectAIDCommand, 10, SelectAIDResponse, 255, 500);
					char cidstr[BUFSIZE];
					char dev;
					char valid;
					if(0x00 == res)
					{
					printf("\n\t\tRAW APDU transceive failed\n");
					}
					else
					{
					printf("\n\t\tAPDU Select AID command sent\n\t\tResponse : \n\t\t");

						for(i = 0x00; i < (unsigned int)res; i++)
							{	
								printf("%02X ", SelectAIDResponse[i]);
							}
							printf("\n\n");
							//TODO: decrypt
	    					encrypt(SelectAIDResponse, KEY, PL_LEN);
	    					sscanf(SelectAIDResponse, "%8c%c%c", cidstr, &dev, &valid);
	    					cidstr[8] = '\0';
	    					printf("CID STRING:%s DEV:%c VALID:%c\n",cidstr,dev,valid);
					}
					if ((dev == '1') && (valid == '1')){ 
					printf("Updating database...\n");
					char msgBuf[BUFSIZE];
					int port = DBPORT;
					char hostname[] = HOSTNAME;
					int cidint;
    					cidint = (int)strtol(cidstr, NULL, 16);
					char* msgParam = malloc(BUFSIZE);
					strcpy(msgBuf, createBalanceUpdate(cidint, FEE, msgParam));
					
					if (MESSAGESON)
					{
						sendMessageToServer(hostname, port, msgBuf);
					}
					else
					{
						printf("MESSAGES OFF: NOT SENDING\n");
					}
					
					free(msgParam);					
					}
					else{
						printf("Error: phone not valid device\n");
					}
				}
 				framework_LockMutex(g_devLock);
			}
			else if(eDevType_P2P == g_Dev_Type)/*P2P Detected*/
			{
				framework_UnlockMutex(g_devLock);
 				printf("\tDevice Found\n");
				
				if(2 == mode)
				{
					SnepPush(msgToSend, len);
				}
				
				framework_LockMutex(g_SnepClientLock);
	
				if(eSnepClientState_READY == g_SnepClientState)
				{
					g_SnepClientState = eSnepClientState_WAIT_OFF;
					framework_WaitMutex(g_SnepClientLock, 0);
				}
				
				framework_UnlockMutex(g_SnepClientLock);
				framework_LockMutex(g_devLock);
		
			}
			else
			{
				framework_UnlockMutex(g_devLock);
				break;
			}
			
			if(eDevState_PRESENT == g_DevState)
			{
				g_DevState = eDevState_WAIT_DEPARTURE;
				framework_WaitMutex(g_devLock, 0);
				if(eDevType_P2P == DevTypeBck)
				{
					printf("\tDevice Lost\n\n");
				}
				DevTypeBck = eDevType_NONE;
			}
			else if(eDevType_P2P == DevTypeBck)
			{
				printf("\tDevice Lost\n\n");
			}
		}
		
		framework_UnlockMutex(g_devLock);
	}while(0x01);
	
	return res;
}

void strtolower(char * string) 
{
    unsigned int i = 0x00;
    
    for(i = 0; i < strlen(string); i++) 
    {
        string[i] = tolower(string[i]);
    }
}

char* strRemovceChar(const char* str, char car)
{
	unsigned int i = 0x00;
	unsigned int index = 0x00;
	char * dest = (char*)malloc((strlen(str) + 1) * sizeof(char));
	
	for(i = 0x00; i < strlen(str); i++)
	{
		if(str[i] != car)
		{
			dest[index++] = str[i];
		}
	}
	dest[index] = '\0';
	return dest;
}

int convertParamtoBuffer(char* param, unsigned char** outBuffer, unsigned int* outBufferLen)
{
	int res = 0x00;
	unsigned int i = 0x00;
	int index = 0x00;
	char atoiBuf[3];
	atoiBuf[2] = '\0';
	
	if(NULL == param || NULL == outBuffer || NULL == outBufferLen)	
	{
		printf("Parameter Error\n");
		res = 0xFF;
	}
	
	if(0x00 == res)
	{
		param = strRemovceChar(param, ' ');
	}
	
	if(0x00 == res)
	{
		if(0x00 == strlen(param) % 2)
		{
			*outBufferLen = strlen(param) / 2;
			
			*outBuffer = (unsigned char*) malloc((*outBufferLen) * sizeof(unsigned char));
			if(NULL != *outBuffer)
			{
				for(i = 0x00; i < ((*outBufferLen) * 2); i = i + 2)
				{
					atoiBuf[0] = param[i];
					atoiBuf[1] = param[i + 1];
					(*outBuffer)[index++] = strtol(atoiBuf, NULL, 16);
				}
			}
			else
			{
				printf("Memory Allocation Failed\n");
				res = 0xFF;
			}
		}
		else
		{
			printf("Invalid NDEF Message Param\n");
		}
		free(param);
	}
		
	return res;
}

int BuildNDEFMessage(int arg_len, char** arg, unsigned char** outNDEFBuffer, unsigned int* outNDEFBufferLen)
{
	int res = 0x00;
	nfc_handover_cps_t cps = HANDOVER_CPS_UNKNOWN;
	unsigned char* ndef_msg = NULL;
	unsigned int ndef_msg_len = 0x00;
	char *type = NULL;
	char *uri = NULL;
	char *text = NULL;
	char* lang = NULL;
	char* mime_type = NULL;
	char* mime_data = NULL;
	char* carrier_state = NULL;
	char* carrier_name = NULL;
	char* carrier_data = NULL;
	
	if(0xFF == LookForTag(arg, arg_len, "-t", &type, 0x00) && 0xFF == LookForTag(arg, arg_len, "--type", &type, 0x01))
	{
		res = 0xFF;
		printf("Record type missing (-t)\n");
		help(0x02);
	}

	if(0x00 == res)
	{
		strtolower(type);
		if(0x00 == strcmp(type, "uri"))
		{
			if(0x00 == LookForTag(arg, arg_len, "-u", &uri, 0x00) || 0x00 == LookForTag(arg, arg_len, "--uri", &uri, 0x01))
			{
				*outNDEFBufferLen = strlen(uri) + 30; /*TODO : replace 30 by URI NDEF message header*/
				*outNDEFBuffer = (unsigned char*) malloc(*outNDEFBufferLen * sizeof(unsigned char));
				res = ndef_createUri(uri, *outNDEFBuffer, *outNDEFBufferLen);
				if(0x00 >= res)
				{
					printf("Failed to build URI NDEF Message\n");
				}
				else
				{
					*outNDEFBufferLen = res;
					res = 0x00;
				}
			}
			else
			{
				printf("URI Missing (-u)\n");
				help(0x02);
				res = 0xFF;
			}
		}
		else if(0x00 == strcmp(type, "text"))
		{
			if(0xFF == LookForTag(arg, arg_len, "-r", &text, 0x00) && 0xFF == LookForTag(arg, arg_len, "--rep", &text, 0x01))
			{
				printf("Representation missing (-r)\n");
				res = 0xFF;
			}
			
			if(0xFF == LookForTag(arg, arg_len, "-l", &lang, 0x00) && 0xFF == LookForTag(arg, arg_len, "--lang", &lang, 0x01))
			{
				printf("Language missing (-l)\n");
				res = 0xFF;
			}
			if(0x00 == res)
			{
				//TODO: change strlen
				*outNDEFBufferLen = strlen(text) + strlen(lang) + 30; /*TODO : replace 30 by TEXT NDEF message header*/
				*outNDEFBuffer = (unsigned char*) malloc(*outNDEFBufferLen * sizeof(unsigned char));
				res = ndef_createText(lang, text, *outNDEFBuffer, *outNDEFBufferLen);
				if(0x00 >= res)
				{
					printf("Failed to build TEXT NDEF Message\n");
				}
				else
				{
					*outNDEFBufferLen = res;
					res = 0x00;
				}	
			}
			else
			{
				help(0x02);
			}
		}
		else if(0x00 == strcmp(type, "mime"))
		{
			if(0xFF == LookForTag(arg, arg_len, "-m", &mime_type, 0x00) && 0xFF == LookForTag(arg, arg_len, "--mime", &mime_type, 0x01))
			{
				printf("Mime-type missing (-m)\n");
				res = 0xFF;
			}
			if(0xFF == LookForTag(arg, arg_len, "-d", &mime_data, 0x00) && 0xFF == LookForTag(arg, arg_len, "--data", &mime_data, 0x01))
			{
				printf("NDEF Data missing (-d)\n");
				res = 0xFF;
			}
			if(0x00 == res)
			{
				*outNDEFBufferLen = strlen(mime_data) +  strlen(mime_type) + 30; /*TODO : replace 30 by MIME NDEF message header*/
				*outNDEFBuffer = (unsigned char*) malloc(*outNDEFBufferLen * sizeof(unsigned char));
				

				res = convertParamtoBuffer(mime_data, &ndef_msg, &ndef_msg_len);
				
				if(0x00 == res)
				{
					res = ndef_createMime(mime_type, ndef_msg, ndef_msg_len, *outNDEFBuffer, *outNDEFBufferLen);
					if(0x00 >= res)
					{
						printf("Failed to build MIME NDEF Message\n");
					}
					else
					{
						*outNDEFBufferLen = res;
						res = 0x00;
					}
				}
			}
			else
			{
				help(0x02);
			}
		}
		else if(0x00 == strcmp(type, "hs"))
		{
			
			if(0xFF == LookForTag(arg, arg_len, "-cs", &carrier_state, 0x00) && 0xFF == LookForTag(arg, arg_len, "--carrierState", &carrier_state, 0x01))
			{
				printf("Carrier Power State missing (-cs)\n");
				res = 0xFF;
			}
			if(0xFF == LookForTag(arg, arg_len, "-cn", &carrier_name, 0x00) && 0xFF == LookForTag(arg, arg_len, "--carrierName", &carrier_name, 0x01))
			{
				printf("Carrier Reference Name missing (-cn)\n");
				res = 0xFF;
			}
			
			if(0xFF == LookForTag(arg, arg_len, "-d", &carrier_data, 0x00) && 0xFF == LookForTag(arg, arg_len, "--data", &carrier_data, 0x01))
			{
				printf("NDEF Data missing (-d)\n");
				res = 0xFF;
			}
			
			if(0x00 == res)
			{
				*outNDEFBufferLen = strlen(carrier_name) + strlen(carrier_data) + 30;  /*TODO : replace 30 by HS NDEF message header*/
				*outNDEFBuffer = (unsigned char*) malloc(*outNDEFBufferLen * sizeof(unsigned char));
				
				strtolower(carrier_state);
				
				if(0x00 == strcmp(carrier_state, "inactive"))
				{
					cps = HANDOVER_CPS_INACTIVE;
				}
				else if(0x00 == strcmp(carrier_state, "active"))
				{
					cps = HANDOVER_CPS_ACTIVE;
				}
				else if(0x00 == strcmp(carrier_state, "activating"))
				{
					cps = HANDOVER_CPS_ACTIVATING;
				}
				else
				{
					printf("Error : unknown carrier power state %s\n", carrier_state);
					res = 0xFF;
				}
				
				if(0x00 == res)
				{
					res = convertParamtoBuffer(carrier_data, &ndef_msg, &ndef_msg_len);
				}
				
				if(0x00 == res)
				{
					res = ndef_createHandoverSelect(cps, carrier_name, ndef_msg, ndef_msg_len, *outNDEFBuffer, *outNDEFBufferLen);
					if(0x00 >= res)
					{
						printf("Failed to build handover select message\n");
					}
					else
					{
						*outNDEFBufferLen = res;
						res = 0x00;
					}
				}
			}
			else
			{
				help(0x02);
			}
		}
		else
		{
			printf("NDEF Type %s not supported\n", type);
			res = 0xFF;
		}
	}
	
	if(NULL != ndef_msg)
	{
		free(ndef_msg);
		ndef_msg = NULL;
		ndef_msg_len = 0x00;
	}
	
	return res;
}

/*if data = NULL this tag is not followed by dataStr : for example -h --help
if format = 0 tag format -t "text" if format=1 tag format : --type=text*/
int LookForTag(char** args, int args_len, char* tag, char** data, int format)
{
	int res = 0xFF;
	int i = 0x00;
	int found = 0xFF;
	
	for(i = 0x00; i < args_len; i++)
	{
		found = 0xFF;
		strtolower(args[i]);
		if(0x00 == format)
		{
			found = strcmp(args[i], tag);
		}
		else
		{
			found = strncmp(args[i], tag, strlen(tag));
		}
		
		if(0x00 == found)
		{
			if(NULL != data)
			{
				if(0x00 == format)
				{
					if(i < (args_len - 1))
					{
						*data = args[i + 1];
						res = 0x00;
						break;
					}
					else
					{
						printf("Argument missing after %s\n", tag);
					}
				}
				else
				{
					*data = &args[i][strlen(tag) + 1]; /* +1 to remove '='*/
					res = 0x00;
					break;
				}
			}
			else
			{
				res = 0x00;
				break;
			}
		}
	}
	
	return res;
}
 

void cmd_poll(int arg_len, char** arg)
{
	int res = 0x00;
	
	printf("#########################################################################################\n");
	printf("##                                       NFC demo                                      ##\n");
	printf("#########################################################################################\n");
	printf("##                                 Poll mode activated                                 ##\n");
	printf("#########################################################################################\n");
	
	InitEnv();
	

	if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
	{
		help(0x01);
	}
	else
	{
		res = InitMode(0x01, 0x01, 0x00);
		
		if(0x00 == res)
		{
			WaitDeviceArrival(0x01, NULL , 0x00);
		}
	
		res = DeinitPollMode();
	}
	
	
	printf("Leaving ...\n");
}

 
void cmd_push(int arg_len, char** arg)
{
	int res = 0x00;
	unsigned char * NDEFMsg = NULL;
	unsigned int NDEFMsgLen = 0x00;
	
	printf("#########################################################################################\n");
	printf("##                                       NFC demo                                      ##\n");
	printf("#########################################################################################\n");
	printf("##                                 Push mode activated                                 ##\n");
	printf("#########################################################################################\n");
	
	InitEnv();
	
	if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
	{
		help(0x02);
	}
	else
	{
		res = InitMode(0x01, 0x01, 0x00);
		
		if(0x00 == res)
		{
			res = BuildNDEFMessage(arg_len, arg, &NDEFMsg, &NDEFMsgLen);
		}
		
		if(0x00 == res)
		{
			WaitDeviceArrival(0x02, NDEFMsg, NDEFMsgLen);
		}
		
		if(NULL != NDEFMsg)
		{
			free(NDEFMsg);
			NDEFMsg = NULL;
			NDEFMsgLen = 0x00;
		}
		
		res = DeinitPollMode();
	}
	
	
	printf("Leaving ...\n");
}

void cmd_write(int arg_len, char** arg)
{
	int res = 0x00;
	unsigned char * NDEFMsg = NULL;
	unsigned int NDEFMsgLen = 0x00;
	
	printf("#########################################################################################\n");
	printf("##                                       NFC demo                                      ##\n");
	printf("#########################################################################################\n");
	printf("##                                 Write mode activated                                ##\n");
	printf("#########################################################################################\n");
	
	InitEnv();
	
	if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
	{
		help(0x02);
	}
	else
	{
		res = InitMode(0x01, 0x01, 0x00);
		
		if(0x00 == res)
		{
			res = BuildNDEFMessage(arg_len, arg, &NDEFMsg, &NDEFMsgLen);
		}
		
		if(0x00 == res)
		{
			WaitDeviceArrival(0x03, NDEFMsg, NDEFMsgLen);
		}
		
		if(NULL != NDEFMsg)
		{
			free(NDEFMsg);
			NDEFMsg = NULL;
			NDEFMsgLen = 0x00;
		}
		
		res = DeinitPollMode();
	}
	
	
	printf("Leaving ...\n");
}

void cmd_bus(int arg_len, char** arg)
{
	int res = 0x00;
	unsigned char * NDEFMsg = NULL;
	unsigned int NDEFMsgLen = 0x00;
	
	printf("#########################################################################################\n");
	printf("##                                     G16 Capstone                                    ##\n");
	printf("#########################################################################################\n");
	printf("##                                  Bus mode activated                                 ##\n");
	printf("#########################################################################################\n");
	
	InitEnv();
	
	if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
	{
		help(0x02);
	}
	else
	{
		res = InitMode(0x01, 0x01, 0x00);
		
		if(0x00 == res)
		{
			//res = BuildNDEFMessage(arg_len, arg, &NDEFMsg, &NDEFMsgLen);
		}
		
		if(0x00 == res)
		{
			WaitDeviceArrival(0x05, NDEFMsg, NDEFMsgLen);
		}
		
		if(NULL != NDEFMsg)
		{
			free(NDEFMsg);
			NDEFMsg = NULL;
			NDEFMsgLen = 0x00;
		}
		
		res = DeinitPollMode();
	}
	
	
	printf("Leaving ...\n");
}

void cmd_kiosk(int arg_len, char** arg)
{
	int res = 0x00;
	unsigned char * NDEFMsg = NULL;
	unsigned int NDEFMsgLen = 0x00;
	
	printf("#########################################################################################\n");
	printf("##                                     G16 Capstone                                    ##\n");
	printf("#########################################################################################\n");
	printf("##                                 Kiosk mode activated                                ##\n");
	printf("#########################################################################################\n");
	
	InitEnv();
	
	if(0x00 == LookForTag(arg, arg_len, "-h", NULL, 0x00) || 0x00 == LookForTag(arg, arg_len, "--help", NULL, 0x01))
	{
		help(0x02);
	}
	else
	{
		res = InitMode(0x01, 0x01, 0x00);
		
		if(0x00 == res)
		{
			//res = BuildNDEFMessage(arg_len, arg, &NDEFMsg, &NDEFMsgLen);
		}
		
		if(0x00 == res)
		{
			WaitDeviceArrival(0x06, NDEFMsg, NDEFMsgLen);
		}
		
		if(NULL != NDEFMsg)
		{
			free(NDEFMsg);
			NDEFMsg = NULL;
			NDEFMsgLen = 0x00;
		}
		
		res = DeinitPollMode();
	}
	
	
	printf("Leaving ...\n");
}


void* ExitThread(void* pContext)
{
	printf("                              ... press enter to quit ...\n\n");
	
	getchar();
	
	framework_LockMutex(g_SnepClientLock);
	
	if(eSnepClientState_WAIT_OFF == g_SnepClientState || eSnepClientState_WAIT_READY == g_SnepClientState)
	{
		g_SnepClientState = eSnepClientState_EXIT;
		framework_NotifyMutex(g_SnepClientLock, 0);
	}
	else
	{
		g_SnepClientState = eSnepClientState_EXIT;
	}
	framework_UnlockMutex(g_SnepClientLock);
	
	framework_LockMutex(g_devLock);
	
	if(eDevState_WAIT_ARRIVAL == g_DevState || eDevState_WAIT_DEPARTURE == g_DevState)
	{
		g_DevState = eDevState_EXIT;
		framework_NotifyMutex(g_devLock, 0);
	}
	else
	{
		g_DevState = eDevState_EXIT;
	}
	
	framework_UnlockMutex(g_devLock);
	return NULL;
}

int InitEnv()
{
	eResult tool_res = FRAMEWORK_SUCCESS;
	int res = 0x00;
	
	tool_res = framework_CreateMutex(&g_devLock);
	if(FRAMEWORK_SUCCESS != tool_res)
	{
		res = 0xFF;
	}
	
	if(0x00 == res)
	{
		tool_res = framework_CreateMutex(&g_SnepClientLock);
		if(FRAMEWORK_SUCCESS != tool_res)
		{
			res = 0xFF;
		}
 	}
 	
 	if(0x00 == res)
	{
		tool_res = framework_CreateMutex(&g_HCELock);
		if(FRAMEWORK_SUCCESS != tool_res)
		{
			res = 0xFF;
		}
 	}
 	if(0x00 == res)
	{
		tool_res = framework_CreateThread(&g_ThreadHandle, ExitThread, NULL);
		if(FRAMEWORK_SUCCESS != tool_res)
		{
			res = 0xFF;
		}
	}
	return res;
}

int CleanEnv()
{
	if(NULL != g_ThreadHandle)
	{
		framework_JoinThread(g_ThreadHandle);
		framework_DeleteThread(g_ThreadHandle);
		g_ThreadHandle = NULL;
	}
		
	if(NULL != g_devLock)
	{
		framework_DeleteMutex(g_devLock);
		g_devLock = NULL;
	}
	
	if(NULL != g_SnepClientLock)
	{
		framework_DeleteMutex(g_SnepClientLock);
		g_SnepClientLock = NULL;
	}
	if(NULL != g_HCELock)
	{
		framework_DeleteMutex(g_HCELock);
		g_HCELock = NULL;
	}
	return 0x00;
}
 
 int main(int argc, char ** argv)
 {
 // 	wiringPiSetup();

	// pinMode(28, OUTPUT);
	// digitalWrite(28, LOW);

	if (argc<2)
	{
		printf("Missing argument\n");
		help(0x00);
	}
	else if (strcmp(argv[1],"poll") == 0)
	{
		cmd_poll(argc - 2, &argv[2]);
	}
	else if(strcmp(argv[1],"write") == 0)
	{
		cmd_write(argc - 2, &argv[2]);
	}
	else if(strcmp(argv[1],"push") == 0)
	{
		cmd_push(argc - 2, &argv[2]);
	}
	else if(strcmp(argv[1],"bus") == 0)
	{
		cmd_bus(argc - 2, &argv[2]);
	}
	else if(strcmp(argv[1],"kiosk") == 0)
	{

		cmd_kiosk(argc - 2, &argv[2]);
	}
	else
	{
		help(0x00);
	}

	printf("\n");
	
	CleanEnv();
	
	return 0;
 }
 
 
void help(int mode)
{
	printf("\nCOMMAND: \n");
	printf("\tpoll\tPolling mode \t e.g. <nfcDemoApp poll >\n");
	printf("\twrite\tWrite tag \t e.g. <nfcDemoApp write --type=Text -l en -r \"Test\">\n");
	printf("\tshare\tTag Emulation Mode \t e.g. <nfcDemoApp share -t URI -u http://www.nxp.com>\n");
	printf("\tpush\tPush to device \t e.g. <nfcDemoApp push -t URI -u http://www.nxp.com>\n");
	printf("\t    \t               \t e.g. <nfcDemoApp push --type=mime -m \"application/vnd.bluetooth.ep.oob\" -d \"2200AC597405AF1C0E0947616C617879204E6F74652033040D0C024005031E110B11\">\n");
	printf("\n");
	printf("Help Options:\n");
	printf("-h, --help                       Show help options\n");
	printf("\n");
	
	if(0x01 == mode)
	{
		printf("No Application Options for poll mode\n");
	}

	if(0x02 == mode)
	{
		printf("Supported NDEF Types : \n");
		printf("\t URI\n");
		printf("\t TEXT\n");
		printf("\t MIME\n");
		printf("\t HS (handover select)\n");
		printf("\n");
		
		printf("Application Options : \n");
		printf("\t-l,  --lang=en                   Language\n");
		printf("\t-m,  --mime=audio/mp3            Mime-type\n");
		printf("\t-r,  --rep=sample text           Representation\n");
		printf("\t-t,  --type=Text                 Record type (Text, URI...\n");
		printf("\t-u,  --uri=http://www.nxp.com    URI\n");
		printf("\t-d,  --data=00223344565667788    NDEF Data (for type MIME & Handover select)\n");
		printf("\t-cs, --carrierState=active       Carrier State (for type Handover select)\n");
		printf("\t-cn, --carrierName=ble           Carrier Reference Name (for type Handover select)\n");
	}
	printf("\n");
}
