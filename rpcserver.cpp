#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <err.h>
#include <semaphore.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>
#include <string.h>
#include "hash.h"

struct Thread {
    sem_t mutex;
    int cl;
    int fd;
    Hash H;
    bool inUse;
};
typedef struct Thread Thread;

sem_t mainMutex,fileMutex;

uint8_t getByte(int cl, uint8_t *buffer){
    int retval = 0;
    retval = recv(cl,&(*buffer),1,0);
    return retval;
}

int64_t identity (int64_t x){
        return x;
}

uint8_t putBytes(int cl, uint8_t *buffer, uint8_t amount){
    int retval = 0;
    retval = send(cl,&(*buffer),amount,0);
    return retval;
}

int64_t createnum(uint8_t num,uint8_t pos){
    uint8_t temp = num;
    uint8_t temp2 = num;
    temp2 = num/16;
    temp =num% 16;
    uint64_t temp3 = (temp)+(temp2<<(4));
    temp3 = temp3<<(8*pos);
    return temp3;
}

void devolvenum(int64_t num,uint8_t* retarr,uint8_t power){
    int i =0;
    uint8_t waremove;
    do{
        (*retarr)=num&0xFF;
        num>>=8;
        //you can't increment a pointer alone without throwing a warning,so...
        //seriously tempted to assign specifically to int because i like to live dangerously, on the edge of the volcano, invoking demons
        //seriously though it's a dummy variable to prevent a warning, it's not used otherwise clearly
        waremove = *retarr++;
    } while ((i++) < power);
}

int64_t getnum(int cl, uint8_t* buffer){
    int cond = 0;
    uint8_t num[8];
    uint64_t math = 0;
    for(int i = 0;i<8;i++){
        cond = getByte(cl,buffer);
        if(cond > 0){
            num[i] = buffer[0];
        }
    }
    //create 64bit numbers out of 8bit arrays
    for(int i=0;i<8;i++){
            math = math + createnum(num[7-i],i);
    }
    return math;
}

bool checkVar(char* varname,uint8_t len){
    //covers upper case
    if((varname[0] > 64)&&(varname[0] < 91)){
        //we're in business
    }
    //covers lower case
    else if((varname[0] > 96) && (varname[0]< 123)){
        //still in business
    }
    else{
        //definitely not in business
        return false;
    }
    //same as above, but also checks for '_' and numerical
    for(int i =1;i<len;i++){
        if((varname[i] > 64)&&(varname[i] < 91)){
            //we're in business
        }
        //covers lower case
        else if((varname[i] > 96) && (varname[i]< 123)){
            //still in business
        }
        else if((varname[i]>47) && (varname[i]<58)){
            //it's numerical, so we're good
        }
        //if it's not '_' at this point, it's trash
        else if (varname[i] != 95){
            //definitely not in business
            return false;
        }
    }
    //we made it here, so it's probably an ok name
    return true;
}

bool checkNum(char* varname,uint8_t len){
    //covers upper case
    if((varname[0] > 47)&&(varname[0] < 58)){
        //we're in business
        //positive number
    }
    //covers lower case
    else if(varname[0] == 45){
        //it's negative but we're vibing
    }
    else{
        //definitely not in business
        return false;
    }
    //same as above, but only checks for numerical
    for(int i =1;i<len-1;i++){
        if((varname[i]>47) && (varname[i]<58)){
            //it's numerical, so we're good
        }
        //if it's not good, it's trash
        else {
            //definitely not in business
            return false;
        }
    }
    //we made it here, so it's probably a number
    return true;
}

void persistIt(int fd,char* name,int64_t value, char* ref,bool recursive,bool deletion){
    if(!deletion){
        if(recursive)dprintf(fd,"%s=%s \n",name,ref);
        else dprintf(fd,"%s=%ld \n",name,value);
    }
    else dprintf(fd,"%s!=420\n",name);
}

void math(uint8_t b, int64_t math1,int64_t math2, int cl,uint8_t* buffer, char* key, uint8_t len,bool varresult,Thread* thread){
    uint8_t retarr[8];
    bool divByZed = false;
    uint8_t total = 0;
    int64_t result = 0;
    int64_t overflow = 0;
    int err2 = 0;
    bool overflower = false;
    switch(b){
        case 0x01 :{
            //add
            result = math1+math2;
            if(result < 0 && math1 > 0 && math2 > 0){
                overflower = true;
            }
            else if (result > 0 && math1 < 0 && math2 < 0)overflower = true;
            else if((identity(result)-identity(math1))!=math2)overflower = true;
            break;
            }
        case 0x02 :{
        //sub
            result = math1-math2;
            if(math1 >0 && math2 > math1){
                if(result > 0)overflower = true;
            }
            else if(math1 < 0 && math2 > 0){
                if(result > 0)overflower = true;
            }
            else if(math1 > 0 && math2 <0){
                if(result < 0)overflower = true;
            }
            else if(math1 < 0 && math2 <0){
                if(math1 > math2 && result <0)overflower = true;
                else if(math1 < math2 && result > 0)overflower = true;
            }
            break;
        break;
        }

        case 0x03 :{
        //mult
            if(math1 == INT64_MIN && math2 == -1){
                overflower = true;
                result = 0;
            }
            else result = math1*math2;
            if(math2!=0){
                overflow = identity(result)/identity(math2);
            }
            //math2 == 0 => result = 0 => no overflow
            else{
                overflow = math1;
            }
            if(overflow != math1){
                overflower = true;
            }
            if(math1 > 0 && math2 >0){
                if(result < 0)overflower = true;
            }
            else if(math1 < 0 && math2 < 0){
                if(result < 0)overflower = true;
            }
            else if(math1 < 0 && math2 > 0){
                if(result > 0)overflower = true;
            }
            else if(math1 > 0 && math2 < 0){
                if(result > 0)overflower = true;
            }
            //check that piazza edge case
            break;
        }
        case 0x04 :{
        //div
        if(math2 == 0){
            //div by 0 err
            divByZed = true;
        }
        else{
            //don't think we need to check for overflow on div, assuming numbers are whole
            result = math1/math2;
            overflow = math1;
        }
        break; 
        }
        case 0x05:{
            //mod
        if(math2 == 0){
            //mod by 0 err
            divByZed = true;
        }
        else{
            //don't think we need to check for overflow on mod, assuming numbers are whole
            result = math1%math2;
            overflow = math1;
        }
        break; 
        }
        //del
    }
    if(divByZed){
        buffer[0] = 22;
        putBytes(cl,buffer,1);
    }
    if(varresult == true && !divByZed &&!overflower){
        if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
        ins(thread->H,key,len,(char*)(""),result,false);
        persistIt(thread->fd,key,result,(char*)(""),false,false);
        if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
    }
    //devolve 64bit number into 8bit array, push back
    if(!overflower && !divByZed){
        devolvenum(result,retarr,8);
        int k = 0;
        int j = 7;
        while (k < j)
            {
                int temp = retarr[k]; 
                retarr[k] = retarr[j];
                retarr[j] = temp;
                k++;
                j--;
            }
            buffer[0] = 0;
            putBytes(cl,buffer,1);
            do{err2 = putBytes(cl,retarr,8);total=+err2;}while(total<4);
    }
    else if (!divByZed && overflower){
        buffer[0] = 75;
        putBytes(cl,buffer,1);
    }
}

void mathSort(uint8_t b, int cl, uint8_t* buffer, Thread* thread){
    uint8_t varcode = b >> 4;
    uint8_t opcode = b & 0x0F;
    int64_t math1 = 0;
    int64_t math2 = 0;
    char* dummy = {0};
    //dummy key for non var result ops
    //declare holders for variable names
    char* varname = {0};
    int errcode = 0;
    uint8_t len = 0;
    bool badVar = false;
    //make threads doing math wait until previous threads have finished
    switch(varcode){
        //general idea is to split math() into var result and non var result
        case(0):{
            //covers del,getv,setv,and basic add/sub/mod/mult/div
            if(opcode == 0x0F){
                //get varname for deletion
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                //find varname in list
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math1 = rmByKey(thread->H,varname,len,&badVar);
                persistIt(thread->fd,varname,0,(char*)(""),false,true);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                //pop result back to client
                buffer[0] = math1;
                putBytes(cl,buffer,1);
            }
            else if (opcode == 0x08){
                //get
                //get varname
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                //call isRecursive
                //check varname
                /*if(!checkVar(varname,len)){
                    buffer[0] = 22;
                    putBytes(cl,buffer,1);
                }*/
                bool num = false;
                dummy = checkRecursion(thread->H,varname,len,&badVar,&num);
                if(!badVar){
                    //send back success
                    len = strlen(dummy);
                    buffer[0] = 0;
                    putBytes(cl,buffer,1);
                    //send back length
                    buffer[0] = len;
                    putBytes(cl,buffer,1);
                    for(int i = 0;i < len;i++){
                        buffer[0] = dummy[i];
                        putBytes(cl,buffer,1);
                    }
                    //send back new name
                }
                else{
                    //reference was not another variable
                    if(num){
                        //it's numerical
                        buffer[0] = 14;
                        putBytes(cl,buffer,1);
                    }
                    else{
                        buffer[0] = 2;
                        putBytes(cl,buffer,1);
                        //it's not there
                    }
                    //either not found or numerical
                    //send back error!
                }
                free(varname);
            }
            else if (opcode == 0x09){
                //set
                //get recipient varname
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                //save length of varname, since it's needed to go in table
                //verify that we got a goodun!
                bool nameCheck = checkVar(varname,len);
                uint8_t targetLen = len;
                //get target varname
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                dummy = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    dummy[i] = buffer[0];
                }
                //verify that we got a goodun, again!
                bool nameCheckAlso = checkVar(dummy,len);
                //make sure we're not putting garbage in the varstore
                if((nameCheck) && (nameCheckAlso)){
                    if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                    ins(thread->H,varname,targetLen,dummy,0,true);
                    persistIt(thread->fd,varname,0,dummy,true,false);
                    if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                    //set target to recipient
                    //return success
                    buffer[0] = 0;
                    putBytes(cl,buffer,1);
                }
                else{
                    //bad names, mission failed
                    buffer[0] = 22;
                    putBytes(cl,buffer,1);
                }
                free(dummy);
                free(varname);
                }
            else{
                math1 = getnum(cl,buffer);
                math2 = getnum(cl,buffer);
                math(opcode,math1,math2,cl,buffer,dummy,0,false,thread);
            }
            break;   
        }
        case(1):{
            //printf("first argument var\n");
            //getBytes() for var name
            //maybe have separate function for that purpose
            //find var in hash table, assign to math1
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
            math1 = findByKey(thread->H,varname,len,&badVar);
            if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
            free(varname);
            if(badVar){
                //still need to take full command
                math2 = getnum(cl,buffer);
                //check if ENOENT or EFAULT
                if(math1 == -2){
                    buffer[0] = 14;
                    putBytes(cl,buffer,1);
                }
                else{
                    buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
            }
            else{
                math2 = getnum(cl,buffer);
                math(opcode,math1,math2,cl,buffer,dummy,0,false,thread);
            }
            break;
        }
        case(2):{
            //printf("second argument var\n");
            math1 = getnum(cl,buffer);
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
            math2 = findByKey(thread->H,varname,len,&badVar);
            if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
            free(varname);
            if(badVar){
                //check if ENOENT or EFAULT
                if(math2 == -2){
                    buffer[0] = 14;
                    putBytes(cl,buffer,1);
                }
                else{
                    buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
            }
            else math(opcode,math1,math2,cl,buffer,dummy,0,false,thread);
            break;
        } 
        case(3):{
            //printf("first and second vars");
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
            math1 = findByKey(thread->H,varname,len,&badVar);
            if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
            free(varname);
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
            math2 = findByKey(thread->H,varname,len,&badVar);
            if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
            free(varname);
            if(badVar){
                //check if ENOENT or EFAULT
                if((math1 == -2)||(math2 == -2)){
                    buffer[0] = 14;
                    putBytes(cl,buffer,1);
                }
                else{
                    buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
            }
            else math(opcode,math1,math2,cl,buffer,dummy,0,false,thread);
            break;
        }
        case(4):{
            math1 = getnum(cl,buffer);
            math2 = getnum(cl,buffer);
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if(checkVar(varname,len))math(opcode,math1,math2,cl,buffer,varname,len,true,thread);
            else{
                //try again with a name that doesn't suck bro
                buffer[0] = 22;
                putBytes(cl,buffer,1);
            }
            free(varname);
            break;
        } 
        case(5):{
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
            math1 = findByKey(thread->H,varname,len,&badVar);
            if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
            free(varname);
            math2 = getnum(cl,buffer);
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if(checkVar(varname,len) && !badVar)math(opcode,math1,math2,cl,buffer,varname,len,true,thread);
            else if(badVar){
                //check if ENOENT or EFAULT
                if(math1 == -2){
                    buffer[0] = 14;
                    putBytes(cl,buffer,1);
                }
                else{
                    buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
            }
            else{
                //try again with a name that doesn't suck bro
                buffer[0] = 22;
                putBytes(cl,buffer,1);
            }
            free(varname);
            //"first, result var"
            break;
        }
        case(6):{
            math1 = getnum(cl,buffer);
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
            math2 = findByKey(thread->H,varname,len,&badVar);
            if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
            free(varname);
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if(checkVar(varname,len) && !badVar)math(opcode,math1,math2,cl,buffer,varname,len,true,thread);
            else if(badVar){
                //check if ENOENT or EFAULT
                if(math1 == -2){
                    buffer[0] = 14;
                    putBytes(cl,buffer,1);
                }
                else{
                    buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
            }
            else{
                //try again with a name that doesn't suck bro
                buffer[0] = 22;
                putBytes(cl,buffer,1);
            }
            free(varname);
            //"first, result var"
            break;
        }
        case(7):{
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
            math1 = findByKey(thread->H,varname,len,&badVar);
            if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
            free(varname);
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
            math2 = findByKey(thread->H,varname,len,&badVar);
            if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
            free(varname);
            errcode = getByte(cl,buffer);
            if(errcode != 0) len = buffer[0];
            varname = (char*) calloc (8,len);
            for(int i =0; i < len; i++){
                errcode = getByte(cl,buffer);
                varname[i] = buffer[0];
            }
            if(checkVar(varname,len) && !badVar)math(opcode,math1,math2,cl,buffer,varname,len,true,thread);
            else if(badVar){
                //check if ENOENT or EFAULT
                if(math1 == -2){
                    buffer[0] = 14;
                    putBytes(cl,buffer,1);
                }
                else{
                    buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
            }
            else{
                //try again with a name that doesn't suck bro
                buffer[0] = 22;
                putBytes(cl,buffer,1);
            }
            free(varname);
            break;
        }
    }
}

//here to deal with recursion
void mathSend(uint8_t b, int cl, uint8_t* buffer, Thread* thread){
    uint8_t varcode = b >> 4;
    uint8_t opcode = b & 0x0F;
    int64_t math1 = 0;
    int64_t math2 = 0;
    char* dummy = {0};
    //dummy key for non var result ops
    //declare holders for variable names
    char* varname = {0};
    int errcode = 0;
    uint8_t len = 0;
    bool badVar = false;
    bool loop = false;
    //make threads doing math wait until previous threads have finished
    if(varcode < 8){
        mathSort(b,cl,buffer,thread);
    }
    else{
        switch(varcode){
            //recursive op with no variables
            case(8):{
                    math1 = getnum(cl,buffer);
                    math2 = getnum(cl,buffer);
                    math(opcode,math1,math2,cl,buffer,dummy,0,false,thread);
                }   
                break;
            case(9):{
                //printf("first argument var\n");
                //getBytes() for var name
                //find var in hash table, assign to math1
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math1 = findRecursion(thread->H,varname,len,&badVar,0);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                free(varname);
                if(badVar){
                    //still need to take full command
                    math2 = getnum(cl,buffer);
                    if(math1 == -2)buffer[0]=40;
                    else buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
                else{
                    math2 = getnum(cl,buffer);
                    math(opcode,math1,math2,cl,buffer,dummy,0,false,thread);
                }
                break;
            }
            case(10):{
                //printf("second argument var\n");
                math1 = getnum(cl,buffer);
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math2 = findRecursion(thread->H,varname,len,&badVar,0);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                if(badVar && math2 == -2){
                    loop = true;
                }
                free(varname);
                if(badVar){
                    if(loop)buffer[0]=40;
                    else buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
                else math(opcode,math1,math2,cl,buffer,dummy,0,false,thread);
                break;
            } 
            case(11):{
                //printf("first and second vars");
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math1 = findRecursion(thread->H,varname,len,&badVar,0);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                if(badVar && math1 == -2){
                    loop = true;
                }
                free(varname);
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math2 = findRecursion(thread->H,varname,len,&badVar,0);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                if(badVar && math2 == -2){
                    loop = true;
                }
                free(varname);
                if(badVar){
                    if(loop)buffer[0]=40;
                    else buffer[0] = 2;
                    putBytes(cl,buffer,1);
                }
                else math(opcode,math1,math2,cl,buffer,dummy,0,false,thread);
                break;
            }
            case(12):{
                math1 = getnum(cl,buffer);
                math2 = getnum(cl,buffer);
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if(checkVar(varname,len))math(opcode,math1,math2,cl,buffer,varname,len,true,thread);
                else{
                    //try again with a name that doesn't suck bro
                    buffer[0] = 22;
                    putBytes(cl,buffer,1);
                }
                free(varname);
                break;
            } 
            case(13):{
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math1 = findRecursion(thread->H,varname,len,&badVar,0);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                if(badVar && math1 == -2){
                    loop = true;
                }
                free(varname);
                math2 = getnum(cl,buffer);
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if(checkVar(varname,len) && !badVar)math(opcode,math1,math2,cl,buffer,varname,len,true,thread);
                else if(badVar){
                    //check if ENOENT or EFAULT
                    if(loop){
                        buffer[0]=40;
                        putBytes(cl,buffer,1);
                    }
                    else{
                        buffer[0] = 2;
                        putBytes(cl,buffer,1);
                    }
                }
                else{
                    //try again with a name that doesn't suck bro
                    buffer[0] = 22;
                    putBytes(cl,buffer,1);
                }
                free(varname);
                //"first, result var"
                break;
            }
            case(14):{
                math1 = getnum(cl,buffer);
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math2 = findRecursion(thread->H,varname,len,&badVar,0);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                if(badVar && math2 == -2){
                    loop = true;
                }
                free(varname);
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if(checkVar(varname,len) && !badVar)math(opcode,math1,math2,cl,buffer,varname,len,true,thread);
                else if(badVar){
                    //check if ENOENT or EFAULT
                    if(loop){
                        buffer[0]=40;
                        putBytes(cl,buffer,1);
                    }
                    else{
                        buffer[0] = 2;
                        putBytes(cl,buffer,1);
                    }
                }
                else{
                    //try again with a name that doesn't suck bro
                    buffer[0] = 22;
                    putBytes(cl,buffer,1);
                }
                free(varname);
                //"first, result var"
                break;
            }
            case(15):{
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math1 = findRecursion(thread->H,varname,len,&badVar,0);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                if(badVar && math1 == -2){
                    loop = true;
                }
                free(varname);
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in math_sort");
                math2 = findRecursion(thread->H,varname,len,&badVar,0);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
                if(badVar && math2 == -2){
                    loop = true;
                }                
                free(varname);
                errcode = getByte(cl,buffer);
                if(errcode != 0) len = buffer[0];
                varname = (char*) calloc (8,len);
                for(int i =0; i < len; i++){
                    errcode = getByte(cl,buffer);
                    varname[i] = buffer[0];
                }
                if(checkVar(varname,len) && !badVar)math(opcode,math1,math2,cl,buffer,varname,len,true,thread);
                else if(badVar){
                    //check if ENOENT or EFAULT
                    if(loop){
                        buffer[0]=40;
                        putBytes(cl,buffer,1);
                    }
                    else{
                        buffer[0] = 2;
                        putBytes(cl,buffer,1);
                    }
                }
                else{
                    //try again with a name that doesn't suck bro
                    buffer[0] = 22;
                    putBytes(cl,buffer,1);
                }
                free(varname);
                break;
            }
             //printf("it's vars all the way down");
        }
    }
}


uint8_t rwop(uint8_t* buffer,int cl,uint8_t b){
    uint8_t namelength[2];
    uint16_t len = 0; 
    int8_t err = 1;
    uint8_t interm = 0;
    uint64_t offset = 0;
    int8_t fileint = 0;
    uint16_t readam = 0;
    uint16_t rbuffsize = 1;
    //used in like 3 loops later
    uint16_t i = 0;
    uint64_t filesize = 0;
    bool failure = false;
    //pull filename length
    //create two points for later dynamic memory allocation
    char* filenombre;
    //get two bytes for namelength
    for(i = 0;i < 2;i++){
        if(err != 0){
            err = getByte(cl,buffer);
            namelength[i] = buffer[0];
        }
    }
    len = (namelength[0]<<8) + (namelength[1]);
    //there's the memory allocation
    filenombre = (char*) calloc (8,len);
    //get file name w length bytes
    for(i = 0;i < len;i++){
        if(err != 0){
            err = getByte(cl,buffer);
            filenombre[i] = buffer[0];
            filenombre[i+1] = '\0';
        }
    }
    //get offset, convert it as it comes in
    for(i = 0;i < 8;i++){
        if(err != 0){
            err = getByte(cl,buffer);
            interm = buffer[0];
            offset += createnum(interm,7-i);
        }
    }
    //last two bytes are not useless, dumbass
    for(i = 0;i < 2;i++){
        if(err != 0){
            err = getByte(cl,buffer);
            //reuse namelength, why not
            namelength[i] = buffer[0];
        }
    }
    //convert read amount to uint64
    readam = (namelength[0]<<8) + (namelength[1]);
    //switch for write, to retrieve rest of bytes sent
    fileint = open(filenombre,O_RDWR);
    //done with that allocation, it never liked me anyway

    if(fileint <= 0){
        //ends r/w
        buffer[0] = errno;
        putBytes(cl,buffer,1);
        failure = true;
    }
    //switch for read or write
    else if(b == 0x01){
        //check size, to verify we're not reading too much
        struct stat statbuff; //need this to run stat() according to manpage
        filesize = stat(filenombre,&statbuff);
        if(filesize >=0)filesize = statbuff.st_size;
        //subtract one because not reading EOF character
        if((offset+readam > filesize-1)||filesize == 0){
            //can't do this read, sorry fam
            buffer[0] = 22;
            putBytes(cl,buffer,1);
            return -1;
        }
        //because we really don't want to read unless we have to
        //not entirely trusting of the return
        else{
            lseek(fileint,offset,0);
            i = 1;
            //read through entire file once to know how much to send
            filesize = 1;
            err = (read(fileint, &interm, 1) > 0);
            while( err >0 && (i < readam)){
                err = (read(fileint, &interm, 1) > 0);
                rbuffsize++;
                i++;
            }
            close(fileint);
            //close and reopen file to reset offset
            fileint = open(filenombre,O_RDWR);
            //reoffset
            lseek(fileint,offset,0);
            free(filenombre);
            //success! send 0 as error code
            buffer[0] = 0;
            putBytes(cl,buffer,1);
            //breaks on EOF, but that seems to be ok for now
            //break buffsize into pieces
            devolvenum(readam,namelength,2);
            for(i = 0; i<2;i++){
                buffer[0] = namelength[1-i];
                putBytes(cl,buffer,1);
            }
            for(i =0; i < readam; i++){
                if(read(fileint,buffer,1)>0){
                    putBytes(cl,buffer,1);
                }
            }
        }
        close(fileint);
        return readam;
    }
    if(b == 0x02){
        bool writer = true;
        uint64_t badNum = 18446744073709551615;
        //success up to this point, so offset, and take bytes to write
        //verify that we're not gonna write an upsetting amount
        if(!failure){
            if((offset + readam >= badNum)||(offset+readam < 0)|| identity(offset)<0 ){
                buffer[0] = 22;
                putBytes(cl,buffer,1);
                writer = false;
            }
        }
        if(writer && !failure)lseek(fileint,offset,0);
        for(i = 0;(i < readam);i++){
                err = getByte(cl,buffer);
                if(writer && !failure)err = write(fileint,buffer,1);
        }
        if(writer && !failure)buffer[0] = 0;
        if(writer && !failure)putBytes(cl,buffer,1);
        if(!failure)close(fileint);
        free(filenombre);
        return 1;
    }
    //if, somehow, it break
    return 69;
}

uint8_t cresizeop(uint8_t* buffer, int cl, uint8_t b){
    //if this looks familiar, it's because it's exactly the same as above
    uint16_t len;
    uint8_t namelength[2];
    uint8_t retarr[8] = {0};
    char* filenombre;
    uint8_t errcode;
    //staying as int as that's what open() sends back
    int fileint;
    uint64_t filesize;
    bool creationSuccess = false;
    //get two bytes for namelength
    for(int i = 0;i < 2;i++){
        if(errcode != 0){
            errcode = getByte(cl,buffer);
            namelength[i] = buffer[0];
        }
    }
    len = (namelength[0]<<8) + (namelength[1]);
    //there's the memory allocation
    filenombre = (char*) calloc (8,len);
    //get file name w length bytes
    for(int i = 0;i < len;i++){
        if(errcode != 0){
            errcode = getByte(cl,buffer);
            filenombre[i] = buffer[0];
        }
    }
    //there's all the bytes we need. now switch based on operation
    if(b == 0x10){
        //if (0 != sem_wait(&fileMutex)) err(2,"sem_post of main by thread");
        //check if file exists

        fileint = open(filenombre,O_RDWR);
        if (errno == 2){
            fileint = open(filenombre,O_CREAT,00777);
            printf("%d",errno);
            if(fileint < 0){
                buffer[0] = 2;
                putBytes(cl,buffer,1);
                return 0;
            }
            else creationSuccess = true;
        }
        else if(errno == 17){
            close(fileint);
            //file already existed, probably because it has a cool name
            buffer[0] = 17;
            putBytes(cl,buffer,1);
            //extra important to free
            free(filenombre);
            return 1;
        }
        if(errno == 2 && fileint >= 0 && creationSuccess){
            close(fileint);
            //extra important to free
            free(filenombre);
            //it worked, so send back 0
            buffer[0] = 0;
            putBytes(cl,buffer,1);
            return -1;
        }
        else{
            //unknown error that didn't get caught, so it's getting sent to the client
            close(fileint);
            buffer[0] = errno;
            putBytes(cl,buffer,1);
            //extra important to free
            free(filenombre);
            return 1;
        }
    }
    else if(b == 0x20){
        struct stat statbuff; //need this to run stat() according to manpage
        fileint = stat(filenombre,&statbuff);
        free(filenombre);
        if(fileint == -1){
            //ends op
            buffer[0] = errno;
            putBytes(cl,buffer,1);
            return 1;
        }
        else{
            filesize = statbuff.st_size;
            devolvenum(filesize,retarr,8);
            //great success (very nice?)
            buffer[0] = 0;
            putBytes(cl,buffer,1);
            for(int i = 0;i < 8;i++){
                buffer[0] = retarr[7-i];
                putBytes(cl,buffer,1);
            }
            return 1;
        }
    }
    //this should never be seen, but without it the compiler gets scared
    return 1;
}


uint8_t dumpLoad(int cl,Thread* thread, uint8_t b,uint8_t* buffer){
    //if this looks familiar, it's because it's (again) exactly the same as above
    //these three functions could likely be combined, but it's honestly easier for my purposes to keep them separated
    //that's modularity, innit it
    uint16_t len = 0;
    uint8_t namelength[2];
    uint8_t retarr[8] = {0};
    uint8_t localbuf[1];
    char* filenombre;
    char* name = {0};
    char* reference;
    bool recursive;
    int64_t value = 0;
    uint8_t errcode;
    //staying as int as that's what open() sends back
    int fileint;
    uint64_t magic = 0;
    int index = 0;
    //put an if here so we can do dump/load/clear in same function
    //get two bytes for namelength
    if((b == 0x01)|| b == 0x02){
        for(int i = 0;i < 2;i++){
                errcode = getByte(cl,localbuf);
                namelength[i] = localbuf[0];
        }
        len = (namelength[0]<<8) + (namelength[1]);
        //there's the memory allocation
        filenombre = (char*) calloc (8,len);
        //get file name w length bytes
        for(int i = 0;i < len;i++){
            errcode = getByte(cl,localbuf);
            filenombre[i] = localbuf[0];
        }
    }
    //there's all the bytes we need. now switch based on operation
    switch(b){
        case(0x01):{
            //DUMP
            //try not to touch, is very fragile
            fileint = open(filenombre,O_CREAT,0777);
            close(fileint);
            //not sure why I have to reopen the file to write to it, but it's not surprising
            fileint = open(filenombre,O_RDWR);
            free(filenombre);
            //done with that allocation, it never liked me anyway
            if(fileint <= 0){
                //ends r/w
                buffer[0] = errno;
                putBytes(cl,buffer,1);
                return -1;
            }
            FILE* fname = fdopen(fileint,"w");
            for(int i =0; i < buck(thread->H); i++){
                errcode = 0;
                index = 0;
                do{
                    if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in dump");
                    errcode = dumpster(thread->H,i,&name,&value,index,&reference,&recursive);
                    if (0 != sem_post(&mainMutex)) err(2,"sem_post in dump");
                    if(errcode != 0){
                        if(recursive)fprintf(fname,"%s=%s\n",name,reference);
                        else fprintf(fname,"%s=%ld\n",name,value);
                    }
                }while(errcode > (index++)+1);
            }
            fclose(fname);
            buffer[0] = 0;
            putBytes(cl,buffer,1);
            return 0;
        }
        case(0x02):{
            //LOAD
            fileint = open(filenombre,O_RDWR);
            free(filenombre);
            if(fileint <= 0){
                //ends load
                buffer[0] = errno;
                putBytes(cl,buffer,1);
                return -1;
            }
            FILE* fname = fdopen(fileint,"r");
            uint8_t end;
            char* target;
            bool flag = false;
            bool goodVars = true;
            //reading input loop
            if (0 != sem_wait(&fileMutex)) err(2,"sem_wait (file) in load");

            do{
                end = fscanf(fname,"%m[^=]=%ms ",&name,&target);
                if(end != 0 && end != 255){
                    //name = strtok(end,"=");
                    //hopefully this usage of strlen is ok! i need length for the hash that Jim recommended
                    len = strlen(name);
                    if(!(checkVar(name,len))){
                        goodVars = false;
                    }
                    //target = strtok(NULL,"\n");
                    if(checkNum(target,strlen(target))){
                        //constant target
                        //flag stays false
                        value = strtol(target,&reference,0);
                    }
                    else if(checkVar(target,strlen(target))){
                        //variable target
                        //flag becomes true
                        flag = true;
                    }
                    else{
                        goodVars = false;
                    }
                    if(goodVars){
                        //don't be putting trash in there now
                        if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in load");
                        ins(thread->H,name,len,target,value,flag);
                        persistIt(thread->fd,name,value,target,flag,false);
                        if (0 != sem_post(&mainMutex)) err(2,"sem_post in load");
                    }
                    flag = false;
                }
            }while((end != 255)&&goodVars);
            if (0 != sem_post(&fileMutex)) err(2,"sem_post (file) in load");
            fclose(fname);
            if(!goodVars)buffer[0] = 22;
            else buffer[0] = 0;
            putBytes(cl,buffer,1);
            return 0;
        }
        case(0x10):{
            //CLEAR
            for(int i = 0;i<4;i++){
                errcode = getByte(cl,buffer);
                retarr[i] = buffer[0];
            }
            for(int i = 0;i<4;i++){
                magic = magic + createnum(retarr[3-i],i);
            }
            if(magic == 0x0BADBAD0){
                //easy way to wipe file contents
                FILE* destroyer = fdopen(thread->fd,"w");
                destroyer=freopen(NULL,"w",destroyer);
                clear(thread->H);
                buffer[0] = 0;
                putBytes(cl,buffer,1);
                return 0;
            }
            else{
                buffer[0] = 22;
                putBytes(cl,buffer,1);
                return 22;
            } 
        }       
    }
    //for the compiler's sake
    return 69;
}

uint8_t initLoad(Hash H,char* dir, int* fd){
    //copy pasted load
    //start right here if the file doesn't get written to
    int dir_fd = open (dir, O_DIRECTORY | O_PATH);
    if(dir_fd <= 0){
        //it's over
        return 2;
    }
    int fileint = openat (dir_fd, (char*)("data.txt"), O_CREAT | O_RDWR, 0644);
    if(fileint <= 0){
        //ends load
        return 1;
    }
    *fd = fileint;
    FILE* fname = fdopen(fileint,"r");
    uint8_t end = 0;
    char* target = (char*)("");
    bool flag = false;
    bool goodVars = true;
    int len = 0;
    char* name = (char*)("");
    int64_t value = 0;
    char* reference;
    int64_t counter = 0;
    bool deletion = false;
    //reading input loop
    do{
        end = fscanf(fname,"%m[^=]=%ms ",&name,&target);
        if(end != 0 && end != 255){
            //hopefully this usage of strlen is ok! i need length for the hash that Jim recommended
            len = strlen(name);
            if(name[len-1]== 33){
                name = strtok(name,"!");
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in load");
                rmByKey(H,name,len,&flag);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post in load");
                deletion = true;
            }
            if(!(checkVar(name,len))){
                goodVars = false;
            }
            //target = strtok(NULL,"\n");
            else if(checkNum(target,strlen(target))){
                //constant target
                //flag stays false
                value = strtol(target,&reference,0);
            }
            else if(checkVar(target,strlen(target))){
                //variable target
                //flag becomes true
                flag = true;
            }
            else{
                goodVars = false;
            }
            if(goodVars && !deletion){
                //don't be putting trash in there now
                if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in load");
                ins(H,name,len,target,value,flag);
                if (0 != sem_post(&mainMutex)) err(2,"sem_post in load");
            }
            deletion = false;
            flag = false;
            counter++;
        }
    }while((end != 255)&&goodVars);
    printf("read in %ld vars!\n",counter);
    if(!goodVars)return 3;
    else return 0;
}


void brancher(int cl,int8_t repeat, Thread* thread){
    if(repeat == -1){
        printf("Processing %d\n",cl);
    }
    uint8_t retval = 0;
    uint8_t buffer[1] = {0};
    uint8_t b1 = 0;
    uint8_t b2 = 0;
    uint8_t identity[4];
    uint8_t errcode = 0;
    uint8_t total = 0;
    //get first two bytes for function call
    //switch based on whether it's the first time through
    if(repeat == -1){
        if(getByte(cl,buffer)== 1){;
            b1 = buffer[0];
        }
    }
    else b1 = repeat;
    if(getByte(cl,buffer)== 1)b2 = buffer[0];
    //get identifier and send it back, posthaste
    for(int i = 0;i < 4;i++){
        if(getByte(cl,buffer) == 1)identity[i] = buffer[i];
    }
    //making sure 4 bytes get sent
    do{errcode = putBytes(cl,identity,4);total+=errcode;}while(total<4);
    switch(b1){
        case 0x01:{
            mathSend(b2,cl,buffer,thread);
            break;
        }
        case 0x02:{
            if((b2 == 0x01) || (b2 == 0x02)){
                retval = rwop(buffer,cl,b2);
                break;
            }
            else if((b2 == 0x10) || (b2 == 0x20)){
                retval = cresizeop(buffer,cl,b2);
                break;
            }
        }
        case 0x03:{
            retval = dumpLoad(cl,thread,b2,buffer);
            break;
        }
    }
    if(getByte(cl,buffer) == 1){
        //thread has another process
        brancher(cl,buffer[0],thread);
    }
}
// Thread 
void* start(void* arg) {
    Thread* thread = (Thread*)arg;
    while (true) {
        printf("Thread waiting!It's quite patient though\n");
        if (0 != sem_wait(&thread->mutex)) err(2,"sem_wait in thread");
        thread->inUse = true;
        brancher(thread->cl,-1,thread);
        printf("Processing %d complete\n",thread->cl);
        thread->inUse = false;
        thread->cl = 0;
       // if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
    }
    return 0;
}

int main(int argc, char *argv[]) {
    //use getopt(), initialize hash
    int port = 8912;
    char* token = (char*)("localhost");
    int numThread = 4;
    uint16_t buckets = 32;
    int optcode = 0;
    uint8_t iter = 50;
    char* dir = (char*)("data");
    int fd;
    bool loadfail = false;
    bool inputAddress = false;

    while((optcode = getopt(argc,argv,"H:N:P:I:d:a:")) != -1){
        switch(optcode){
            case 'N': 
                numThread = atoi(optarg);
                break;
            case 'H': 
                buckets = atoi(optarg);
                break;
            case 'a':{
                token = strtok(optarg,":");
                port = atoi(strtok(NULL," "));
                printf("using port %d\n",port);
                inputAddress = true;
                break;
            }
            case 'I':{
                iter = atoi(optarg);
                break;
            }
            case 'd':{
                //dirty little workaround
                dir = (char*)calloc(8,strlen(optarg));
                strcpy(dir,optarg);
                break;
            }
        } 
    }
    //shoutout silly little alien on the cse discord for this shell
    if(inputAddress){
        for(; optind < argc; ++optind)
        {
            token = strtok(argv[optind],":");
            port = atoi(strtok(NULL," "));
            printf("using port %d\n",port);
        }
    }
    Thread threads[numThread];
    if (0 != sem_init(&mainMutex, 0, 0)) err(2,"sem_init mainMutex");
    if(0 != sem_init(&fileMutex,0,0)) err(2,"sem_init fileMutex");
    pthread_t threadPointer; // value not used in program.

    //initialize 'global' hash
    Hash varstore = newHash(buckets);
    setIter(varstore,iter);
    // initialize and start the threads
    for (int i = 0; i < numThread; i++) {
        Thread* thread = threads+i;
        if (0 != sem_init(&(thread->mutex), 0, 0)) err(2,"sem_init for thread");
        thread->cl = 0;
        thread->H = varstore;
        thread->inUse = false;
        if (0 != pthread_create(&threadPointer,0,start,thread)) err(2,"pthread_create");
    }
    //load initial data
    //post mutex first so it can access varstore
    if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
    if (0 != sem_post(&fileMutex)) err(2,"sem_post (file) in load");
    optcode = initLoad(varstore,dir,&fd);
    if(optcode >0){
        printf("intial load failed with error code %d",optcode);
        loadfail = true;
        //exit(optcode);
    }
    if(!loadfail){
        struct hostent *hent = gethostbyname(token /* eg "localhost" */);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
        
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        int enable = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
        bind(sock, (struct sockaddr *)&addr, sizeof(addr));

        int cl = 0;
        uint8_t threader = 0;

        printf("initialized with %d threads,%d buckets\n",numThread,buckets);
        if (listen(sock,128) != 0) err(2,"unable to listen");
        //accept loop
        while(true){
            write(1,"waiting for new connection.....\n",32);
            cl = accept(sock, NULL, NULL);
            Thread* thread = threads+threader;
            while(thread->inUse == true){
                if(threader < numThread-1) threader++;
                else thread = threads+threader;
            }
            thread->cl = cl;
            thread->fd = fd;
            if (0 != sem_post(&thread->mutex)) err(2,"sem_post of main by thread");
            //uses next thread every time one is used to maximize time before conflict
            if(threader < numThread-1) threader++;
            else threader = 0;
        }
    }
}

