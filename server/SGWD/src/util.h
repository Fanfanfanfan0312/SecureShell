#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <vector>
#include <pthread.h>
#include <openssl/aes.h>

#define SHELL "/bin/sh"
#define SERVERKEY "/var/SGWD/key"

using namespace std;

void* cleaner(void *arg);

struct TerminalHandler{
	int key_num;
	int fdm;
	int fds;
	int command_pipe[2];
};

struct Passphase{
	unsigned char key[16];
	unsigned char name[16];
	Passphase();
	Passphase(const unsigned char* _key, const unsigned char* _name);
	void refresh();
};

struct Key{
	vector<Passphase> pass_list;
	unsigned char hash[20];
	
};

unsigned char generateKey(const char* filepath);
unsigned char loadKeyfile(const char* filepath);

class Connection{
public:
    in_addr_t ip_addr;
    int port, error_attempts, encoder_num;
    unsigned long last_message_time;
    unsigned char initialized, blacklisted;
	unsigned char lastUpdate[36];
	unsigned char last_message[256];
    AES_KEY enc_key;
	AES_KEY dec_key;
	int key_num;

	TerminalHandler terminal_handler;
	pid_t child_pid;
	int fdm, fds;
	pthread_t terminalThread;
	int command_pipe[2];

	int dev_null;

    Connection(in_addr_t _ip_addr, int _port);
	void sendUpdate(void);
	void endConnection(unsigned char killThread);
    void feedData(const char* datapointer, size_t size);
	void encode(const unsigned char* input, size_t size, unsigned char* output);
	void decode(const unsigned char* input, size_t size, unsigned char* output);
	void processDecryptedData(const unsigned char* input, size_t size);
	void sendUnencryptedData(const unsigned char* input, size_t size);
	void sendData(const unsigned char* datapointer, size_t size);

	unsigned char openTerminal(void);
	void executeCommand(const char* command, size_t size);

};

#endif
