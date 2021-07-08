#ifndef _LinkList_H_INCLUDE_
#define _LinkList_H_INCLUDE_
#include<stdio.h>
#include<stdlib.h>

typedef struct LinkList* List;
List newList(void);

void freeList(List* pL);

int length(List L);
// Search Functions
int64_t findEl(List L, int index);
char* findName(List L, int index);
int64_t findByName(List L,char* key,bool* errflag);
void editData(List L, char* name, int64_t data, char* recursive,bool flag);

// Manipulation functions
void append(List L,int64_t data,char* key, char* recursive,bool flag);
void rmdata(List L, char* name);
bool isRecursive(List L, char* name,bool* flag);
char* findRef(List L, char* name, bool* flag);
void clear(List L);
bool checkExist(List L,char* name);

#endif
