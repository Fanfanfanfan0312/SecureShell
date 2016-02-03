/*
 * password.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Ben
 */

#ifndef SRC_PASSWORD_H_
#define SRC_PASSWORD_H_

#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL)

int set_disp_mode(int fd,int option);
int readPasswd(char* passwd, int size);
int getPasswd(char* passwd, size_t size);



#endif /* SRC_PASSWORD_H_ */
