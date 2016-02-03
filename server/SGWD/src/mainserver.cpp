#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "util.h"

using namespace std;

int sin_len;
char message[256];
struct in_addr incomingip;
int incomingport;
int socket_descriptor;
struct sockaddr_in sin;

vector<Connection> list;
vector<Key> key_list;

void* cleaner(void *arg);

int main(int argc, char** argv) {
    char keyword[128] = "";
	char filepath[128] = "";
    int port, length;
	
    if(argc != 3){
        printf("USAGE: -p [port] -g [File Path] (generate keyfile)\n");
		exit(1);
    }
    for(int i = 1; i < argc; i++){	
		if(memcmp(argv[i], "-p", 2) == 0){
	    	i++;
	    	sscanf(argv[i], "%d", &port);
		}
		
		if(memcmp(argv[i], "-g", 2) == 0){
			i++;
			sscanf(argv[i], "%s", filepath);
			if(generateKey(filepath)!=0){
				printf("failed to generate keyfile\n");
				exit(2);
			}
			exit(0);
		}
    }
    if(port == 0){
		printf("USAGE: -p [port] -g [File Path] (generate keyfile)\n");
		exit(1);
    }

	memcpy(filepath, SERVERKEY, sizeof(SERVERKEY));
	if(loadKeyfile(filepath)!=0){
		printf("unable to load keyfile\n");
		exit(1);
	}
	
    bzero(&sin,sizeof(sin));
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr=htonl(INADDR_ANY);
    sin.sin_port=htons(port);
    sin_len=sizeof(sin);

    socket_descriptor=socket(AF_INET,SOCK_DGRAM,0);
    bind(socket_descriptor,(struct sockaddr *)&sin,sizeof(sin));

    pthread_t Cleaner;
    pthread_create(&Cleaner, NULL, cleaner, (void *)NULL);
    while(1)
    {
		memset(message, 0, sizeof(message));
        length=recvfrom(socket_descriptor,message,sizeof(message),0,(struct sockaddr *)&sin,(socklen_t*)&sin_len);

        memcpy(&incomingip,&sin.sin_addr,sizeof(struct in_addr));
        incomingport = sin.sin_port;

		for(int i = 0; i < list.size(); i++){
	   	 	if(list[i].ip_addr == incomingip.s_addr && list[i].port == incomingport){
				list[i].feedData(message, length);
				break;
	   		 }
	   	 	if(i == list.size()-1){
				list.push_back(Connection(incomingip.s_addr, incomingport));
				list[list.size()-1].feedData(message, length);
	    	}
		}
		if(list.size()==0){
			list.push_back(Connection(incomingip.s_addr, incomingport));
			list[list.size()-1].feedData(message, length);
		}

//        memset(message, 0, sizeof(message));
//        sprintf(message, "received");
//        sin.sin_addr = incomingip;
//        sin.sin_port = incomingport;
//        sendto(socket_descriptor,message,sizeof(message), 0, (struct sockaddr *)&sin, sizeof(sin));
//        printf("feedback sent\n");
    }

    return (EXIT_SUCCESS);
}

void* cleaner(void *arg){
    while(1){
        sleep(300);
        unsigned long cur_time = time(NULL);
        for(int i = 0; i < list.size(); i++){
       	    if(cur_time - list[i].last_message_time >= 300){
				printf("timeout with %x\n", list[i].ip_addr);
				list[i].endConnection(0);
	        	list.erase(list.begin()+i);
	        	i--;
 	    	}
        }
    }
    return NULL;
}



