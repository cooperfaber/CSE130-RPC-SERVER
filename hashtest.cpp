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
#include <sys/socket.h>
#include <string.h>
#include "hash.h"


Hash H;

void test(char* token, uint8_t len, int64_t val){
	ins(H,token,len,val);
}

int main(){
	Hash *pH = &H;
	*pH = newHash(4);
	char* token;
	token = (char*)("chungus");
	test(token,7,69);
	token = (char*)("x");
	test(token,1,10);
	test((char*)("blood"),5,420);
	print(H);
	int x = findByKey(H,"x",1);
	printf("%d\n",x);
}