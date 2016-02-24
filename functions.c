#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "functions.h"
#include "tcp.h"

void encrypt(char* input, char* key, int inputlen)
{
    int i;
    int keylen = strlen(key);
    for(i=0; i<inputlen; i++)
    {
        input[i]=input[i]^key[i%keylen];
    }
}

void strToFloat(char* str, float* f)
{
    *f = strtof(str, NULL);
}

void hexStrToFloat(char* str, float* f)
{
    int num;
    sscanf(str, "%x", &num);
    *f = *((float*)&num);
}

void floatToHexStr(float f, char* str)
{
    sprintf(str, "%08x", *((int*)&f));
}

void strToInt(char* str, int* i)
{
    *i = (int)strtol(str, NULL, 10);
}

void hexStrToInt(char*str, int* i)
{
    *i = (int)strtol(str, NULL, 16);
}

void intToHexStr(int i, char* str)
{
    sprintf(str, "%08x", i);
    str[strlen(str)] = '\0';
}

void intToStr(int i, char* str)
{
    sprintf(str, "%010d", i);
    str[strlen(str)] = '\0';
}

void floatToStr(float f, char* str)
{
    sprintf(str, "%.2f", f);
    str[strlen(str)] = '\0';
}

void createPLTime(struct tm* new_t, char* buf)
{
    char temp[BUFSIZE];
    strftime(temp, sizeof(temp), DATEPLFORMAT, new_t);
    strcpy(buf, temp);
    buf[15] = '\0';
    printf("PL timestamp: %s\n", buf);
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

void getFullCache(char* hostname, int port)
{
    printf("Requesting database cache...\n");
    char msgBuf[BUFSIZE];
    char* msgParam = malloc(BUFSIZE);
    strcpy(msgBuf, createGetEntryAll(msgBuf));
    
    if (MESSAGESON)
    {
        if(sendMessageToServer(hostname, port, msgBuf)<1)
        {
            printf("No connection to database\n");
        }
    }
    else
    {
        printf("MESSAGES OFF: NOT SENDING\n");  
    }
    free(msgParam);
}