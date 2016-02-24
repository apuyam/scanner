/* Functions for TCP communication and creating messages to DB.
 * error, initServer, acceptClient, and sendMessageToServer based on
 * http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpserver.c,
 * http://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpclient.c
 * and http://stackoverflow.com/questions/2597608/c-socket-connection-timeout
 */

#define HOSTNAME "192.168.100.117"
//#define HOSTNAME "169.254.85.87"
//#define HOSTNAME "192.169.0.24"
#define DBPORT 5555
#define GUIPORT 5556
#define FORMAT_BLANK "0101"
#define LOGIN "scanner Capstone2015Scanner"
#define CACHEFILE "cache.xml"
#define MESSAGESON 1 //1 to enable sendMessageToServer calls
#define CACHING 1 //1 to enable getFullCache calls
#define REQTIME 1 //1 to enable initial time sync
#define BUFSIZE 256
#define TIMEOUTSEC 2 //timeout for sendMessageToServer

/*print error message and exit*/
void error(char *msg);

/*KIOSK MODE: create server socket at serverfd*/
int initServer(int serverfd, int portno);

/*KIOSK MODE: block while waiting for a client connection*/
int acceptClient(int clientfd, int parentfd, char* buf);

/* connect to hostname thru port, send buf, disconnect. 
reads before disconnecting if sending a get message
returns 0 if no connection, 1 on successful connection*/
int sendMessageToServer(char* hostname, int port, char* buf);

/*Example:
	char* msgBuf[BUFSIZE];
    char* msgParam = malloc(BUFSIZE);
    strcpy(msgBuf, createBalanceUpdate(cid, balance, msgParam));
    sendMessageToServer(hostname, port, msgBuf);
    free(msgParam);
*/

/* creates message to send to database of form 
"5 cid diff" and stores it in msgParam*/
char* createBalanceUpdate(int cid, float diff, char* msgParam);

/* creates message to send to database of form 
"6 cid balance" and stores it in msgParam*/
char* createEntry(int cid, float balance, char* msgParam);

/* creates message to send to database of form 
"7 cid" and stores it in msgParam*/
char* createGetEntry(int cid, char* msgParam);

/* creates message to send to database of form 
"7" and stores it in msgParam*/
char* createGetEntryAll(char* msgParam);

/* creates message to send to database of form 
"1" and stores it in msgParam*/
char* createGetTime(char* msgParam);

/* create message to send to database of form
"10 cid" and stores it in msgParam*/
char* createGetLastUpdate(int cid, char* msgParam);