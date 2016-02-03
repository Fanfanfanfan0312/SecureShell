/*
 * util.h
 *
 *  Created on: Jan 30, 2016
 *      Author: Ben
 */

#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <openssl/aes.h>

using namespace std;

#define TIMEOUT 2

void sigHandler(int sig);

struct Passphase{
	unsigned char key[16];
	unsigned char name[16];
	Passphase();
	Passphase(const unsigned char* _key, const unsigned char* _name);
	void refresh();
};

class Connection{
public:
	in_addr_t dest_addr;
	int dest_port;
	int socket_descriptor;
	int encoder_num;
	struct sockaddr_in address;
	socklen_t addrlen;
	char buf[512];
	char keyfile[128];
	unsigned char file_sha[20];
	unsigned char last_mess[512];
	unsigned long last_mess_time;
	unsigned long last_rec_time;
	size_t last_mess_size;
	vector<Passphase> pass_list;
	AES_KEY dec_key;
	AES_KEY enc_key;
	bool waiting_for_update;

	Connection();
	void config(in_addr_t _dest_addr, int _dest_port, const char* _keyfile, size_t keysize);
	unsigned char startConnection();
	void dataIn(const char* dataPointer, size_t datasize);
	void processDecryptedData(const unsigned char* databuffer, size_t size);
	void sendData(const unsigned char* dataPointer, size_t datasize);
	void sendUnencryptedMessage(const char* datapointer, size_t size);
	unsigned char loadFile(const char* filepath);
	void decode(const unsigned char* input, size_t size, unsigned char* output);
	void encode(const unsigned char* input, size_t size, unsigned char* output);

};



#endif /* SRC_UTIL_H_ */
