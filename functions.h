char* encrypt(char* input, char* key, int inputlen);


/*Converts a string eg. "2.500000" to a float*/
float strToFloat(char* str, float* f);

/*Converts a string eg. "FFFFFFFF" to a float*/
float hexStrToFloat(char* str, float* f);

/*Converts float eg. 2.500000 to hex string*/
char* floatToStr(float f, char* str);

/*Converts string eg. "2147483647" to int*/
int strToInt(char* str, int* i);

/*Converts int eg. 2147483647 to hex string eg. "FFFFFFFF"*/
char* intToHexStr(int i, char* str);