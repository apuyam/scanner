#define HOSTNAME "192.168.100.117"
//#define HOSTNAME "169.254.85.87"
//#define HOSTNAME "192.169.0.24"
#define DBPORT 5555
#define GUIPORT 5556
#define BUFSIZE 64
#define FORMAT_BLANK ""
#define LOGIN "scanner Capstone2015Scanner"
#define CONNECTED
#define MESSAGESON 0

void error(char *msg);

/*KIOSK MODE: create server socket at parentfd*/
int initServer(int parentfd, int portno);

/*KIOSK MODE: block while waiting for a client connection*/
int acceptClient(int childfd, int parentfd, char* buf);

/* connect to hostname thru port, send buf, disconnect. */
void sendMessageToServer(char* hostname, int port, char* buf);

/* creates message to send to database of form 
"5 cid diff" and stores it in msgParam*/
char* createBalanceUpdate(int cid, float diff, char* msgParam);

/* creates message to send to database of form 
"6 cid balance" and stores it in msgParam*/
char* createEntry(int cid, float balance, char* msgParam);

/* creates message to send to database of form 
"7 cid" and stores it in msgParam*/
char* createGetEntry(int cid, char* msgParam);

/*Example:
    char* msgParam = malloc(BUFSIZE);
    strcpy(msgBuf, createBalanceUpdate(cid, balance, msgParam));
    sendMessageToServer(hostname, port, msgBuf);
    free(msgParam);
*/

/* connect to hostname thru port, read and store in buf, disconnect. */
char* readMessageFromServer(char* hostname, int port, char* buf);