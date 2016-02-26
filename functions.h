/* Contains general helper functions for encryption, 
	type conversion and timestamp mangement */

#define KEY "GHIJKLMNOPQRSTUVWXYZYXWVUTSRQPON"
#define INITDATE "19700101101010"
#define DATEFORMAT "%b %d %Y %H:%M:%S"
#define DATEPLFORMAT "%Y%m%d%H%M%S"
#define DATETSFORMAT "%Y-%m-%d %H:%M:%S"
#define TRANSFERSOFF 1 //1 to disable transfer system

/*takes input of length inputlen and uses XOR encryption
with key. Decryption of encrypted input is done with same 
function and same key*/
void encrypt(char* input, char* key, int inputlen);

/*Converts a string eg. "2.500000" to a float*/
void strToFloat(char* str, float* f);

/*Converts a string eg. "FFFFFFFF" to a float*/
void hexStrToFloat(char* str, float* f);

/*Converts float eg. 2.500000 to hex string*/
void floatToHexStr(float f, char* str);

/*Converts string eg. "2147483647" to int*/
void strToInt(char* str, int* i);

/*Converts hex string to int eg. "18" to 24*/
void hexStrToInt(char*str, int* i);

/*Converts int eg. 2147483647 to hex string eg. "FFFFFFFF"*/
void intToHexStr(int i, char* str);

/*Converts int to string, width of 10 eg. 24 to "0000000024"*/
void intToStr(int i, char* str);

/*Converts float to string eg 2.5 to "2.5"*/
void floatToStr(float f, char* str);

/*Convets int to CID string without leading zeroes for XML cache reading.
eg. 24 to "24"*/
void createCacheCID(int cidint, char* cidstrsmall);

/*Creates date string of form DATEPLFORMAT from new_t and stores in buf*/
void createPLTime(struct tm* new_t, char* buf);

/*Checks date string timestamp and compares to current system time.
Returns difference (curr-timestamp) in seconds*/
double comparePLTime(char* timestamp);

/*Retrieves all database entries from DB and stores in cache.xml*/
void getFullCache(char* hostname, int port);