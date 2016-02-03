#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <fcntl.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include "util.h"

using namespace std;

extern int sin_len;
extern int socket_descriptor;
extern struct sockaddr_in sin;
extern vector<Key> key_list;
extern vector<Connection> list;

void combine(unsigned char* s1, int startpos, const unsigned char* s2, size_t s2_size){
	for(int i = startpos; i < startpos+s2_size; i++){
		s1[i] = s2[i - startpos];
	}
}

void weaveString(const unsigned char* s1, const unsigned char* s2, size_t size, unsigned char* result){
	for(int i = 0; i < size; i++){
		result[i*2] = s1[i];
		result[i*2+1] = s2[i];
	}
}

unsigned char generateKey(const char* filepath){
	FILE *save = fopen(filepath, "w+");
	FILE *server = fopen(SERVERKEY, "a");
	if(save == NULL) return 1;
	Passphase p;
	
	for(int i =0; i < 128; i++){
		p.refresh();
		for(int j = 0; j < 16; j++){
			fputc(p.key[j], save);
			fputc(p.key[j], server);
		}
		for(int j = 0; j < 16; j++){
			fputc(p.name[j], save);
			fputc(p.name[j], server);
		}
		//fprintf(save, "%s\n", pass_list[i].key);
		//fprintf(save, "%s\n", pass_list[i].name);
	}
	fclose(save);
	fclose(server);
	return 0;
}

unsigned char loadKeyfile(const char* filepath){
	unsigned char key_buf[16];
	unsigned char name_buf[16];
	FILE *f_in = fopen(filepath, "r");
	if(f_in == NULL) return 1;

	fseek(f_in, 0, SEEK_END);
	long length = ftell(f_in);
	fseek(f_in, 0, SEEK_SET);	
	
	for(int n = 0; n < length/(32*128); n++){
		key_list.push_back(Key());
		for(int i = 0; i < 128; i++){
			key_list.back().pass_list.push_back(Passphase());
			fread(key_list.back().pass_list.back().key, 16, 1, f_in);
			fread(key_list.back().pass_list.back().name, 16, 1, f_in);
			//memcpy(temp_pass.key, key_buf, sizeof(temp_pass.key));
			//memcpy(temp_pass.name, name_buf, sizeof(temp_pass.name));
			//temp_key.pass_list.push_back(Passphase(temp_pass.key, temp_pass.name));
			//printf("%s\n", key_list.back().pass_list.back().key);
		}
	}

	fseek(f_in, 0, SEEK_SET);

	unsigned char file_buffer[32*128];
	unsigned char sha_hash[20];
	for(int n = 0; n < length/(32*128); n++){
		fread(file_buffer, 32, 128, f_in);
		SHA(file_buffer, sizeof(file_buffer), sha_hash);
		memcpy(key_list[n].hash, sha_hash, sizeof(sha_hash));
	}

	fclose(f_in);
	return 0;
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

Passphase::Passphase(){
}

Passphase::Passphase(const unsigned char* _key, const unsigned char* _name){
	memcpy(key, _key, sizeof(key));
	memcpy(name, _name, sizeof(name));
}

void Passphase::refresh(){
	getRandom(key, sizeof(key));
	getRandom(name, sizeof(name));
}

Connection::Connection(in_addr_t _ip_addr, int _port){
    ip_addr = _ip_addr;
    port = _port;
    last_message_time = time(NULL);
    initialized = 0;
    error_attempts = 0;
    blacklisted = 0;
	encoder_num = 0;
	key_num = -1;
	
	dev_null = open("/dev/null", O_WRONLY);
	if(dev_null == NULL){
		printf("dev_null open failure");
		exit(3);
	}
}

void Connection::sendData(const unsigned char* datapointer, size_t size){
	sin.sin_addr.s_addr = ip_addr;
	sin.sin_port = port;
	sendto(socket_descriptor,datapointer,size, 0, (struct sockaddr *)&sin, sizeof(sin));
}

void Connection::decode(const unsigned char* input, size_t size, unsigned char* output){

	AES_set_decrypt_key(key_list[key_num].pass_list[encoder_num].key, 128, &dec_key);
	
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

	AES_set_encrypt_key(key_list[key_num].pass_list[encoder_num].key, 128, &enc_key);
	
	if(size % 16 != 0) printf("warning! wrong encoding may happened!\n");

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

void Connection::sendUpdate(){

	unsigned char rand_num;
	getRandom(&rand_num, 1);
	rand_num = rand_num % 128;

	unsigned char new_encoder_name[16];

	memcpy(new_encoder_name, key_list[key_num].pass_list[rand_num].name, sizeof(new_encoder_name));
	
	unsigned char hash[20];
	SHA(new_encoder_name, sizeof(new_encoder_name), hash);

	unsigned char raw_pack[48];
	unsigned char finished_pack[48];
	memcpy(raw_pack, new_encoder_name, sizeof(new_encoder_name));
	combine(raw_pack, sizeof(new_encoder_name), hash, 20);
	
	encode(raw_pack,sizeof(raw_pack), finished_pack);
	
	sendData(finished_pack, sizeof(finished_pack));

	/*unsigned char test[48];
	AES_set_decrypt_key(encoder, 8*sizeof(encoder), &dec_key);
	decode(finished_pack, sizeof(finished_pack), test);
	if(memcmp(raw_pack, test, sizeof(raw_pack))==0) printf("CORRECT!\n");
	else printf("ERROR\n");*/
	
	memcpy(lastUpdate, finished_pack, sizeof(lastUpdate));
	encoder_num = rand_num;
}

void Connection::feedData(const char* datapointer, size_t size){
	if(blacklisted) return;
	last_message_time = time(NULL);
    if(initialized == 0){
		if(size!=40){
			return;
		}
		unsigned char random_head[size-20];
    	memcpy(random_head, datapointer, size-20);
    	unsigned char hash_rec[20];

    	for(int i = size-20; i< size; i++){
     	   hash_rec[i +20-size] = datapointer[i];
    	}

    	unsigned char buffer[sizeof(random_head)*2];
		unsigned char sha_result[20];

		for(int i = 0; i < key_list.size(); i++){
    		weaveString(random_head, key_list[i].hash, sizeof(random_head), buffer);  //mark
    		
    		SHA(buffer, sizeof(random_head)*2, sha_result);

    		if(memcmp(sha_result, hash_rec, 20)==0){
				initialized = 1;
				printf("connected with %x\n", ip_addr);
				key_num = i;
				encoder_num = 0;
				
				sendUpdate();
				if(openTerminal()!=0){
					printf("failed to open terminal\n");
					endConnection(0);
					exit(3);
				}

				break;
			}
			else{
				continue;
			}

			if(i == key_list.size()-1){
				error_attempts++;
				if(error_attempts > 3){
					blacklisted = 1;
				}
			
				printf("rejected\n");
			}
		}
    }else{
		last_message_time = time(NULL);

		if(memcmp(datapointer, last_message, size)==0){
			sendData(lastUpdate, sizeof(lastUpdate));
		}
		else{
			if(size%16!=0) return;
	
			memset(last_message, 0, sizeof(last_message));
			memcpy(last_message, datapointer, size);
			unsigned char encrypted_data[size];
			unsigned char raw_data[size];

			memcpy(encrypted_data, datapointer, size);
			decode(encrypted_data, size, raw_data);

			sendUpdate();

			processDecryptedData(raw_data, size);
		}
	}
}

void Connection::endConnection(unsigned char killThread){

	unsigned char buffer[15];
	memcpy(buffer, "disconnect_sgw", sizeof("disconnect_sgw"));

	sendUnencryptedData(buffer, 15);	

	kill(child_pid, SIGKILL);
	
	if(killThread)
		pthread_kill(terminalThread, SIGKILL);

	ip_addr = NULL;
    port = NULL;
	last_message_time = time(NULL);
    initialized = 0;
    error_attempts = 0;
    blacklisted = 0;
	encoder_num = 0;
	key_num = -1;
}

void Connection::processDecryptedData(const unsigned char* input, size_t size){
	//for(int i = 0; i < size; i++){
	//	fputc(input[i], stdout);
	//	fflush(stdout);
	//}
	//printf("\n");

	write(dev_null, input, size);

	if(size>=8&&memcmp(input, "stop_sgw", 8) == 0){

		endConnection(0);
		return;
	}
	
	//sendUnencryptedData(input, size);
	char command[size+2];
	memcpy(command, input, size);

	command[strlen(command)] = '\n';
	command[strlen(command)] = '\0';
	executeCommand(command, sizeof(command));
}

void Connection::sendUnencryptedData(const unsigned char* datapointer, size_t size){
	
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

void Connection::executeCommand(const char* command, size_t size){
	//unsigned char buffer[size+1];
	//memcpy(buffer, command, size);
	//buffer[size] = '\0';
	//sendUnencryptedData(buffer, size+1);

	//write(1, "executing", sizeof("executing"));
	write(command_pipe[1], command, size);
}



