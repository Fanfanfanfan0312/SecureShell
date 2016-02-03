#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "util.h"
#include "password.h"

using namespace std;

Connection linkage;

int main(int argc, char **argv){
	if(argc != 7){
		printf("USAGE: -s server name -p port -l keyfile\n");
		exit(1);
	}

	char servername[15]="";
	int port=0;
	char keyfile[128]="";
	char buffer[256];

	for(int i = 1; i < argc; i++){
		if(memcmp(argv[i], "-s", 2) == 0){
			i++;
			memcpy(servername, argv[i], sizeof(servername));
		}else if(memcmp(argv[i], "-p", 2) == 0){
			i++;
			sscanf(argv[i], "%d", &port);
		}else if(memcmp(argv[i], "-l", 2) == 0){
			i++;
			memcpy(keyfile, argv[i], sizeof(keyfile));
		}
	}

	if(strlen(servername)==0 || strlen(keyfile)==0 || port == 0){
		printf("USAGE: -s server name -p port -l keyfile\n");
		exit(1);
	}


	linkage.config(inet_addr(servername), port, keyfile, sizeof(keyfile));
	linkage.startConnection();

	while(1){
		memset(buffer, 0, sizeof(buffer));
		gets(buffer);
		linkage.sendUnencryptedMessage(buffer, strlen(buffer));
	}

	return 0;
}
