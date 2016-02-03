#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "password.h"

int set_disp_mode(int fd,int option)
{
   int err;
   struct termios term;
   if(tcgetattr(fd,&term)==-1){
     perror("Cannot get the attribution of the terminal");
     return 1;
   }
   if(option)
        term.c_lflag|=ECHOFLAGS;
   else
        term.c_lflag &=~ECHOFLAGS;
   err=tcsetattr(fd,TCSAFLUSH,&term);
   if(err==-1 && err==EINTR){
        perror("Cannot set the attribution of the terminal");
        return 1;
   }
   return 0;
}

int readPasswd(char* passwd, int size)
{
   int c;
   int n = 0;


   do{
      c=fgetc(stdin);
      //if (c != '\n' | c!='\r'){
         passwd[n++] = c;
      //}
         fputc(c, stdout);
   }while(c != '\n' && c !='\r' && n < (size - 1));
   passwd[n-1] = '\0';
   return n;
}

int getPasswd(char* passwd, size_t size){

	set_disp_mode(STDIN_FILENO,0);
	int rc = readPasswd(passwd, size);

	set_disp_mode(STDIN_FILENO,1);

	return rc;
}

