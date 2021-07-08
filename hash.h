#ifndef _hash_H_INCLUDE
#define _hash_H_INCLUDE
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct ChainHash* Hash;
Hash newHash(uint16_t buckets);
unsigned int DJBHash(char* str, uint8_t length);
void ins(Hash H, char* key, int len, char* recursion, int64_t value, bool flag);
void setIter(Hash H, uint8_t iter_set);
int64_t findByKey(Hash H, char* key, uint8_t length,bool* flag);
int64_t rmByKey(Hash H, char* key, uint8_t length,bool* flag);
void clear(Hash H);
int dumpster(Hash H, int bucket, char** name, int64_t* value, int index,char** recursive, bool* whichOne);
uint8_t buck(Hash H);
int64_t findRecursion(Hash H, char* key, uint8_t len,bool* flag,uint8_t counter);
char* checkRecursion(Hash H, char* key, uint8_t len,bool* flag,bool* numer);
#endif