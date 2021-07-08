#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "hash.h"
#include "LinkList.h"

typedef struct ChainHash{
    uint16_t buckets;
    struct LinkList** table;
    uint8_t iter;
}ChainHash;

typedef ChainHash* Hash;

Hash newHash(uint16_t bucket){
    Hash H = new ChainHash;
    H->buckets = bucket;
    H->table = (List*) malloc(bucket*sizeof(List));
    for(int i = 0;i<bucket;i++){
        H->table[i] = newList();
    }
    return H;
}

unsigned int DJBHash(char* str, uint8_t length) {
	unsigned int hash = 5381;
	unsigned int i = 0;
	for (i = 0; i < length; str++, i++)
	{
		hash = ((hash << 5) + hash) + (*str);
	}
	return hash;
}

void ins(Hash H, char* key, int len, char* recursion, int64_t value,bool flag){
    unsigned int index = DJBHash(key,len);
    List L = H->table[(index%(H->buckets))];
    char* inter =  (char*)calloc (8,len);
    char* interAlso = {0};
    //hoping this one is ok too! was running into tons of issues without copying the name
    //this more or less needs to be done if nodes are to hold varnames
    strcpy(inter,key);
    //have to do it again, in order to hold recursive reference
    if(flag){
        interAlso = (char*)calloc (8,strlen(recursion));
        strcpy(interAlso,recursion);
    }
    else{
        interAlso = (char*)("");
    }
    if(L == NULL) err(2,"list didn't get created right");
    if(length(L) == 0){
        append(L,value,inter,interAlso,flag);
    }
    else{
        if(checkExist(L,inter)){
            editData(L,inter,value,interAlso,flag);
        }
        else {
            append(L,value,inter,interAlso,flag);
        }
    }
}

void setIter(Hash H, uint8_t iter_set){
    if(H != NULL){
        H->iter = iter_set;
    }
}

int64_t findByKey(Hash H, char* key, uint8_t len,bool* flag){
    unsigned int index = (DJBHash(key,len));
    List L = H->table[(index%(H->buckets))];
    if(L == NULL || length(L) == 0){
        *flag = true;
        return 0;
    }
    else if (length(L)>0){
        int64_t value = findByName(L,key,flag);
        if(*flag == true && value == -2){
            return -2;
        }
        else if(*flag == true){
            return -1;
        }
        else return value;
    }
    *flag = true;
    return 69;
}

int64_t rmByKey(Hash H, char* key, uint8_t len,bool* flag){
    unsigned int index = (DJBHash(key,len));
    List L = H->table[(index%(H->buckets))];
    if(L == NULL){
        return 0;
    }
    else if (length(L)>0){
        int64_t value = findByName(L,key,flag);
        if(*flag == true && value == -1){
            return 22;
        }
        else{
            rmdata(L,key);
            return 0;
        } 
    }
    return 22;
}

void clear(Hash H){
    for (int i =0; i < H->buckets;i++){
        clear(H->table[i]);
    }
}

int dumpster(Hash H, int bucket,char** name, int64_t* value, int index, char** reference, bool* whichOne){
    List L = H->table[bucket];
    bool dummy = false;
    if((length(L)!= 0)&&(index < length(L))){
        *name = findName(L,index); 
        if(isRecursive(L,*name,whichOne))*reference = findRef(L,*name,&dummy);
        else *value = findEl(L,index);

    }
    return length(L);
}

uint8_t buck(Hash H){
    return H->buckets;
}

//recursive function, because how could we not have one?
int64_t findRecursion(Hash H, char* key, uint8_t len,bool* flag,uint8_t counter){
    if(counter == H->iter){
        *flag = true;
        return -2;
    }
    unsigned int index = (DJBHash(key,len));
    //dummy var since dumpster needs the extra bool pointer
    bool no_exist = false;
    List L = H->table[(index%(H->buckets))];
    if(L == NULL){
        return 0;
    }
    else if (length(L)>0){
        if(checkExist(L,key)){
            if(isRecursive(L,key,&no_exist)){
                return findRecursion(H,findRef(L,key,flag),strlen(findRef(L,key,flag)),flag,counter+1);
            }
            else{
                *flag = false;
                return findByName(L,key,flag);
            }
        }
        else{
            *flag = true;
            return -1;
        }
    }
    *flag = true;
    return -1;
}

char* checkRecursion(Hash H, char* key, uint8_t len,bool* flag,bool* numer){
    unsigned int index = (DJBHash(key,len));
    List L = H->table[(index%(H->buckets))];
    if(L == NULL){
        *flag = true;
        *numer = false;
        return (char*)("z");
    }
    else if (length(L)>0){
        if(isRecursive(L,key,flag)){
            *flag = false;
            *numer = false;
            return findRef(L,key,flag);
        }
        else{
            *flag = true;
            *numer = true;
            return (char*)("num");
            }
        }
    *flag = true;
    *numer = false;
    return (char*)("z");
    }

