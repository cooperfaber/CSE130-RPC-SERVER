#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <err.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include "LinkList.h"


union Data{
  int64_t value;
  char* name;
};


typedef struct NodeObj{
   union Data data;
   char* name;
   struct NodeObj* next;
   struct NodeObj* prev;
   bool recursive;
 } NodeObj;

typedef NodeObj* Node;

typedef struct LinkList{
   Node front;
   Node back;
   int length;
} LinkList;

typedef LinkList* List;

Node newNode(int64_t node_data, char* node_recursion, char* name,bool recursive){
   Node N = (Node) malloc(sizeof(NodeObj));
   N->recursive = recursive;
   if(recursive){
    N->data.name = node_recursion;
   }
   else N->data.value = node_data;
   N->name = name;
   N->next = NULL;
   N->prev = NULL;
   return(N);
}

void freeNode(Node* pN){
    (*pN)->data.value = NULL;
    (*pN)->data.name = NULL;
    (*pN)->name = NULL;
    (*pN)->recursive = false;
    if((*pN)->prev != NULL&&(*pN)->next != NULL){
      (*pN)->prev->next = (*pN)->next;
      (*pN)->next->prev = (*pN)->prev;
    }
    else if((((*pN)->next == NULL)&&((*pN)->prev != NULL))){
      (*pN)->prev->next = NULL;
    }
    else if((((*pN)->next != NULL)&&((*pN)->prev == NULL))){
      (*pN)->next->prev = NULL;
    }
    (*pN)->next = NULL;
    (*pN)->prev = NULL;
    free(*pN);
}

List newList(){
   List L = new LinkList;
   L->front = NULL;
   L->back = NULL;
   L->length = 0;
   return(L);
}

void freeList(List* pL){
      clear(*pL);
      free(*pL);
}

//use functions

int length(List L){
   if( L==NULL ){ 
      return -1;
   }
   return(L->length);
}

int64_t findEl(List L, int index){
    if( L==NULL ){
      return -1;
    }
    if(index+1 > L->length){
       return -2;
    }
    Node N = L->front;
    for(int i = 0; i < index;i++){
        N = N->next;
    }
   return N->data.value;
}


char* findName(List L, int index){
    if( L==NULL ){
      return (char*)("");
    }
    if(length(L) == 0){
      return (char*)("");
    }
    if(index > L->length){
      return (char*)("");
    }
    Node N = L->front;
    for(int i =0; i < index;i++){
        N = N->next;
    }
   return N->name;
}


int64_t findByName(List L, char* key,bool* errflag){
   if(( L==NULL) || (L->length == 0)){
    *errflag = true;
      return -1;
   }
   if(L->front == NULL)err(2,"list not properly initialized");
   Node N = L->front;
   for (int i = 0; i < L->length; i++){
      if(strcmp(key,N->name) == 0){
         //just straight fails if it's recursive
         if(N->recursive){
            *errflag = true;
            return -2;
         }
         else{
//            *errflag = false;        
            return N->data.value;
       }
      }
      else{
        if(N != L->back){
          N = N->next;
        }
      }
   }
   *errflag = true;
   return -1;
}

void append(List L,int64_t data,char* key, char* recursive,bool flag){
	if( L==NULL ){
		exit(1);
    }
	Node N = newNode(data,recursive,key,flag);
	if( L->length == 0){
		L->front = N;
		L->back = N;
	}
	else{
		L->back->next = N;
    N->prev = L->back;
		L->back = N;
	}
	L->length++;
}

void rmdata(List L, char* name){
   bool done = false;
   if(L->length >0){
      Node N = NULL;
      N = L->front;
      //index 0, list one element
      if(N == L->back){
        if(N->prev != NULL)L->back = N->prev;
        else L->back = NULL;
         freeNode(&N);
         done = true;
      }
      else if(N == L->front){
        L->front = N->next;
        freeNode(&N);
         done = true;
      }
      else for(int i =0; i < length(L);i++){
        if((strcmp(N->name,name) == 0) && !done){
          done = true;
          i = length(L);
          freeNode(&N);
        }
        else{
          if(N != L->back && !done)N = N->next;
        }
      }
      L->length--;
   }
}

void editData(List L, char* name, int64_t data, char* recursive,bool flag){
   if(L->length >0){
      Node N = L->front;
      for(int i = 0; i< length(L);i++){
         if(strcmp(N->name,name)==0){
            if(flag){
              N->data.name = recursive;
              N->recursive = true;
            }
            else{
              N->data.value = data;
              N->recursive = false;
              i = length(L);
            }
         }
         else{
            if(N != L->back){
               N = N->next;
            }
         }
      }
   }
}

bool isRecursive(List L, char* name, bool* flag){
  if(L==NULL){
    *flag = false;
    return false;
  }
  else if(L->length > 0){
    Node N = L->front;
    for(int i =0;i < L->length;i++){
      if(strcmp(N->name,name)==0){
        if(N->recursive){
          *flag = true;
          return true;
        }
        else{
          *flag = false;
          return false;
        } 
      }
      else if(N != L->back){
        N = N->next;
      }
    }
  }
  *flag = false;
  return false;
}

char* findRef(List L, char* name, bool* flag){
  if(L==NULL){
  }
  else if(L->length > 0){
    Node N = L->front;
    for(int i =0;i < L->length;i++){
      if(strcmp(N->name,name)==0){
        return N->data.name;
      }
      else if(N != L->back){
        N = N->next;
      }
    }
  }
  *flag = true;
  return (char*)("UP-SET");
}

void clear(List L){
  if(L == NULL){
  }
  else if(L->length > 0){
    Node N = L->front;
    Node temp = NULL;
    for (int i = 0; i < length(L); i++){
      if(N != L->back)temp = N->next;
      freeNode(&N);
      N = temp;
    }
  }
  L->length = 0;
}
//needed this simple function
bool checkExist(List L,char* name){
  if(L==NULL){
    return false;
  }
  else if(L->length > 0){
    Node N = L->front;
    for(int i =0;i < L->length;i++){
      if(strcmp(N->name,name)==0){
        return true;
      }
      else if(N != L->back){
        N = N->next;
      }
    }
  }
  return false;
}
