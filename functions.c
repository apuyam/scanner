#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "functions.h"
#include "tcp.h"

char* encrypt(char* input, char* key, int inputlen)
{
    // for xor, decrypting is just re-encrypting with same key
    int i;
    int keylen = strlen(key);
    for(i=0; i<inputlen; i++)
    {
        input[i]=input[i]^key[i%keylen];
    }
    return input;
}

float strToFloat(char* str, float* f)
{
    *f = strtof(str, NULL);
    
    return *f;
}

float hexStrToFloat(char* str, float* f)
{
    int num;
    sscanf(str, "%x", &num);
    *f = *((float*)&num);
    
    return *f;
}

char* floatToHexStr(float f, char* str)
{
    sprintf(str, "%08x", *((int*)&f) );
    
    return str;
}

int strToInt(char* str, int* i)
{
    
    *i = (int)strtol(str, NULL, 10);
    
    return *i;
}

int hexStrToInt(char*str, int* i)
{
    *i = (int)strtol(str, NULL, 16);
    
    return *i;
}

char* intToHexStr(int i, char* str)
{
    sprintf(str, "%08x", i);
    str[strlen(str)] = '\0';

    return str;
}

char* intToStr(int i, char* str)
{
    sprintf(str, "%010d", i);
    str[strlen(str)] = '\0';

    return str;
}

char* floatToStr(float f, char* str)
{
    sprintf(str, "%f", f);
    str[strlen(str)] = '\0';

    return str;
}

char* createPLTime(struct tm* new_t, char* buf)
{
    char temp[BUFSIZE];
    strftime(temp, sizeof(temp), DATEPLFORMAT, new_t);
    strcpy(buf, temp);
    buf[15] = '\0';
    printf("New timestamp: %s\n", buf);
    
    return buf;
}

double comparePLTime(char* timestamp)
{
    double diff;
    struct tm tmpl;
    strptime(timestamp, DATEPLFORMAT, &tmpl);
    time_t end_t;
    time_t pl_t = mktime(&tmpl);
    time(&end_t);
    diff = difftime(end_t,pl_t);
    
    return diff;
}