#define LOGIN "scanner Capstone2015Scanner"
#define BUFSIZE 64

void error(char *msg);

void sendMessageToServer(char* hostname, int port, char* buf);

char* createBalanceUpdate(int cid, float diff, char* msgParam);

char* createEntry(int cid, float balance, char* msgParam);

char* createGetEntry(int cid, char* msgParam);

char* readMessageFromServer(char* hostname, int port, char* buf);