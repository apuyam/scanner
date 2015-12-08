#include <stdio.h> 
#include <string.h> 
#include <semaphore.h> 
#include "linux_nfc_api.h"

static sem_t sem; 
static int mTagHandle; 

/* Callback for the tag arrival event */ 
void onTagArrival (nfc_tag_info_t *pTag) 
{ 
 mTagHandle = pTag->handle; 
 sem_post(&sem); 
} 

 

/* Callback for the tag departure event */ 
void onTagDeparture (void ){} 

/* Main function: waits for an NFC tag and shows if it contains an NDEF 
message*/ 
int main(int argc, char *argv[]) 
{ 
 printf("\n########## Our first Linux libnfc-nci application ##########\n\n"); 
 

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
 if (info.is_ndef) 
 printf("The tag contains an NDEF message\n\n"); 
 else 
 printf("The tag does not contain an NDEF message\n\n"); 

 

 /* Deinitialize stack */ 
 sem_destroy(&sem); 
 nfcManager_disableDiscovery(); 
 nfcManager_deregisterTagCallback(); 
 nfcManager_doDeinitialize (); 

 

 return 0; 
} 