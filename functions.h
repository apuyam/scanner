#define KEY "GHIJKLMNOPQRSTUVWXYZYXWVUTSRQPON"
#define DATEFORMAT "%b %d %Y %H:%M:%S"
#define DATEPLFORMAT "%Y%m%d%H%M%S"
#define TRANSFERSOFF 1

char* encrypt(char* input, char* key, int inputlen);


/*Converts a string eg. "2.500000" to a float*/
float strToFloat(char* str, float* f);

/*Converts a string eg. "FFFFFFFF" to a float*/
float hexStrToFloat(char* str, float* f);

/*Converts float eg. 2.500000 to hex string*/
char* floatToHexStr(float f, char* str);

/*Converts string eg. "2147483647" to int*/
int strToInt(char* str, int* i);

/*Converts hex string to int eg. "18" to 24*/
int hexStrToInt(char*str, int* i);

/*Converts int eg. 2147483647 to hex string eg. "FFFFFFFF"*/
char* intToHexStr(int i, char* str);

/*Converts int to string eg. 24 to "24"*/
char* intToStr(int i, char* str);

/*Converts float to string eg 2.5 to "2.5"*/
char* floatToStr(float f, char* str);

char* createPLTime(struct tm* new_t, char* buf);

double comparePLTime(char* timestamp);