#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include "util.h"
#include "password.h"

using namespace std;

extern void *receiveInfo(void *arg);
extern void *detectTimeout(void *arg);

extern Connection linkage;

void sigHandler(int sig){
    if(sig == 2) //ctrl+c
    {
        char buf[2];
        buf[0] = 3;
        buf[1] = '\0';
        linkage.sendUnencryptedMessage(buf, 2);
        
    }
    
}

void getRandom(unsigned char* rand_buf, int Bytes){
	srand(time(NULL));
	char buffer[128];
	for(int i = 0; i < Bytes; i++){
		rand_buf[i] = rand()%128;
	}
	RAND_seed(rand_buf, Bytes);
	RAND_bytes(rand_buf, Bytes);
}

void getSHA1(const unsigned char* input, int size, unsigned char* hash){
	SHA(input, size, hash);
}

void weaveString(const unsigned char* s1, const unsigned char* s2, size_t size, unsigned char* result){
	for(int i = 0; i < size; i++){
		result[i*2] = s1[i];
		result[i*2+1] = s2[i];
	}
}

void combine(unsigned char* s1, int startpos, const unsigned char* s2, size_t s2_size){
	for(int i = startpos; i < startpos+s2_size; i++){
		s1[i] = s2[i - startpos];
	}
}

Passphase::Passphase(){
}

Connection::Connection(){

}

unsigned char Connection::loadFile(const char* filepath){
	unsigned char key_buf[16];
	unsigned char name_buf[16];
	FILE *f_in = fopen(filepath, "r");
	if(f_in == NULL) return 1;

	fseek(f_in, 0, SEEK_END);
	long length = ftell(f_in);
	fseek(f_in, 0, SEEK_SET);

	if(length!=32*128) return 1;

		for(int i = 0; i < 128; i++){
			pass_list.push_back(Passphase());
			fread(pass_list.back().key, 16, 1, f_in);
			fread(pass_list.back().name, 16, 1, f_in);
			//memcpy(temp_pass.key, key_buf, sizeof(temp_pass.key));
			//memcpy(temp_pass.name, name_buf, sizeof(temp_pass.name));
			//temp_key.pass_list.push_back(Passphase(temp_pass.key, temp_pass.name));
			//printf("%s\n", key_list.back().pass_list.back().key);
		}


	fseek(f_in, 0, SEEK_SET);

	unsigned char file_buffer[32*128];
	unsigned char sha_hash[20];

		fread(file_buffer, 32, 128, f_in);
		SHA(file_buffer, sizeof(file_buffer), sha_hash);
		memcpy(file_sha, sha_hash, sizeof(sha_hash));


	fclose(f_in);

	return 0;
}

void Connection::config(in_addr_t _dest_addr, int _dest_port, const char* _keyfile, size_t _keysize){
	dest_addr = _dest_addr;
	dest_port = _dest_port;
	memcpy(keyfile, _keyfile, _keysize);
	if(loadFile(keyfile)!=0){
		fprintf(stderr, "failed to load file\n");
		exit(1);
	}
	encoder_num = 0;
}

unsigned char Connection::startConnection(){
	last_rec_time = time(NULL)-1;

	bzero(&address,sizeof(address));
	address.sin_family=AF_INET;
	address.sin_addr.s_addr=dest_addr;
	address.sin_port=htons(dest_port);
	addrlen=sizeof(address);
	socket_descriptor=socket(AF_INET,SOCK_DGRAM,0);

	unsigned char pack_head[40];
	getRandom(pack_head, 20);
	unsigned char mesh_key[40];
	weaveString(pack_head, file_sha, 20, mesh_key);
	unsigned char hash[20];
	SHA(mesh_key, 40, hash);
	combine(pack_head, 20, hash, 20);

	sendData(pack_head, sizeof(pack_head));

	pthread_t receiveThread;
	pthread_create(&receiveThread, NULL, receiveInfo, (void *)NULL);

	pthread_t timeoutThread;
	pthread_create(&timeoutThread, NULL, detectTimeout, (void *)NULL);
}

void Connection::decode(const unsigned char* input, size_t size, unsigned char* output){

	AES_set_decrypt_key(pass_list[encoder_num].key, 128, &dec_key);

	if(size % 16 != 0) printf("warning! wrong encoding may happened!\n");

	unsigned char in_buf[16];
	unsigned char out_buf[16];

	for(int i = 0; i < size/16; i++){

		memset(in_buf, 0, sizeof(in_buf));
		memset(out_buf, 0, sizeof(out_buf));

		for(int j = 0; j < 16; j++){
			in_buf[j] = input[i * 16 + j];
		}

		AES_ecb_encrypt(in_buf, out_buf, &dec_key, AES_DECRYPT);

		for(int j = 0; j < 16; j++){
			output[i * 16 + j] = out_buf[j];
		}
	}
}

void Connection::encode(const unsigned char* input, size_t size, unsigned char* output){

	AES_set_encrypt_key(pass_list[encoder_num].key, 128, &enc_key);

	if(size % 16 != 0) fprintf(stderr, "warning! wrong encoding may happened!\n");

	unsigned char in_buf[16];
	unsigned char out_buf[16];

	for(int i = 0; i < size/16; i++){

		memset(in_buf, 0, sizeof(in_buf));
		memset(out_buf, 0, sizeof(out_buf));

		for(int j = 0; j < 16; j++){
			in_buf[j] = input[i * 16 + j];
		}
		AES_ecb_encrypt(in_buf, out_buf, &enc_key, AES_ENCRYPT);
		for(int j = 0; j < 16; j++){
			output[i * 16 + j] = out_buf[j];
		}
	}
}

void Connection::dataIn(const char* datapointer, size_t datasize){

	if(datasize%16 != 0) return;

	unsigned char encrypted_mess[datasize];
	unsigned char decrypted_mess[datasize];

	memcpy(encrypted_mess, datapointer, datasize);

	//AES_set_decrypt_key(pass_list[encoder_num].key, 128, &dec_key);

	decode(encrypted_mess, datasize, decrypted_mess);

	if(memcmp(decrypted_mess, "disconnect_sgw", sizeof("disconnect_sgw")) == 0){
		printf("disconnected by server\n");
		exit(2);
	}

	if(datasize == 48){
		unsigned char buffer[16];
		unsigned char sha_result[20];
		unsigned char sha_rec[20];

		memcpy(buffer, decrypted_mess, sizeof(buffer));
		for(int i = 16; i < 36; i++){
			sha_rec[i-16] = decrypted_mess[i];
		}

		SHA(buffer, sizeof(buffer), sha_result);

		if(memcmp(sha_result, sha_rec, sizeof(sha_result))==0){
			//printf("receive update\n");
			for(int i = 0; i < pass_list.size(); i++){
				if(memcmp(pass_list[i].name, buffer, sizeof(buffer))==0){
					encoder_num = i;
					waiting_for_update = false;
					//printf("update to %d\n", i);

					return;
				}
				if(i == pass_list.size()-1){
					fprintf(stderr, "not synchronized\n");
					exit(2);
				}
			}
		}
	}

	if(waiting_for_update){
		sendData(last_mess, last_mess_size);
		return;
	}
	processDecryptedData(decrypted_mess, datasize);
}
void Connection::processDecryptedData(const unsigned char* databuffer, size_t size){
	for(int i = 0; i < size; i++){
		fputc(databuffer[i], stdout);
	}
	fflush(stdout);

	char buffer[size];
	memcpy(buffer, databuffer, size);
	if(strstr(buffer, "[sudo]") != NULL)
		if(strstr(buffer, "password") != NULL)
			if(strstr(buffer, "for") != NULL){
				char pass[256];
				getPasswd(pass, sizeof(pass));
				sendUnencryptedMessage(pass, strlen(pass));

			}

	//printf("\n");

}
void Connection::sendData(const unsigned char* dataPointer, size_t datasize){

	waiting_for_update = true;
	sendto(socket_descriptor, dataPointer, datasize,0,(struct sockaddr *)&address,sizeof(address));

	memset(last_mess, 0, sizeof(last_mess));
	memcpy(last_mess, dataPointer, datasize);
	last_mess_size = datasize;
	last_mess_time = time(NULL);

}

void Connection::sendUnencryptedMessage(const char* datapointer, size_t size){

	size_t closest16;
	if(size%16==0){
		closest16 = size;
	}
	else{
		closest16 = (size/16+1) * 16;
	}

	unsigned char data[closest16];
	unsigned char encrypted_data[closest16];
	memset(data, 0, sizeof(data));
	memcpy(data, datapointer, size);


	encode(data, closest16, encrypted_data);
	sendData(encrypted_data, closest16);

}

