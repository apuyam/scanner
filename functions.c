#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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