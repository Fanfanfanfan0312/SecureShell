#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "util.h"

using namespace std;

extern Connection linkage;

void* receiveInfo(void *arg){
	char buffer[512];
	int length;
	while(1){
		//memset(buffer, 0, sizeof(buffer));
		length = recvfrom(linkage.socket_descriptor,
				&buffer,sizeof(buffer),
				0,
				(struct sockaddr *)&linkage.address,
				&linkage.addrlen);
		//printf("%s\n", buffer);
		linkage.last_rec_time = time(NULL);

		linkage.dataIn(buffer, length);
	}

	return NULL;
}

void* detectTimeout(void *arg){
    
    int timeout_count = 0;
    
	while(true){

		while(!linkage.waiting_for_update);

		sleep(TIMEOUT);

		//printf("rec %d\nsec%d\n", linkage.last_rec_time, linkage.last_mess_time);

		if(linkage.last_rec_time<linkage.last_mess_time){
			printf("connection timeout, reconnecting\n");
			linkage.sendData(linkage.last_mess, linkage.last_mess_size);
            
            timeout_count++;
            if(timeout_count > 5) exit(1);
		}
        else{
            
            timeout_count = 0;
        }
	}
}

