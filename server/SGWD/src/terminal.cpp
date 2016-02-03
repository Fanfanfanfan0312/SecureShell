#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#define __USE_BSD
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "util.h"

extern vector<Connection> list;

int max(int x, int y){
	if(x >= y) return x;
	else return y;
}

void* handleTerminal(void* argv){

		int rc;
		int fdm, fds;
		int command_pipe[2], key_num;
		fd_set fd_in;

		TerminalHandler t;
		memcpy(&t, argv, sizeof(t));

		fdm = t.fdm;
		fds = t.fds;
		key_num = t.key_num;
		command_pipe[0] = t.command_pipe[0];
		command_pipe[1] = t.command_pipe[1];

		char input[256];
		unsigned char send_buf[256];
			
 	 	while (1)
  		{
    		// Wait for data from standard input and master side of PTY
    		FD_ZERO(&fd_in);
    		//FD_SET(0, &fd_in);
			FD_SET(command_pipe[0], &fd_in);
    		FD_SET(fdm, &fd_in);

    		//rc = select(fdm + 1, &fd_in, NULL, NULL, NULL);
			rc = select(max(fdm, command_pipe[0]) + 1, &fd_in, NULL, NULL, NULL);
    		switch(rc)
    		{
      			case -1 : fprintf(stderr, "Error %d on select()\n", errno);
                			return NULL;

      			default :
      			{	
        			// If data on standard input
        			//if (FD_ISSET(0, &fd_in))
					if (FD_ISSET(command_pipe[0], &fd_in))
       			 	{			
				
          				//rc = read(0, input, sizeof(input));
						rc = read(command_pipe[0], input, sizeof(input));
						
          				if (rc > 0)
          				{
            				// Send data on the master side of PTY
            				write(fdm, input, rc);

          				}
          				else
          				{
            				if (rc < 0)
            				{
              					fprintf(stderr, "Error %d on read standard input\n", errno);
             				 	return NULL;
            				}
          				}
       				}

        			// If data on master side of PTY
        			if (FD_ISSET(fdm, &fd_in))
        			{
         				rc = read(fdm, input, sizeof(input));
          				if (rc > 0)
          				{
            				// Send data on standard output
            				//write(1, input, rc);
							
							memcpy(send_buf, input, rc);
							for(int it = 0; it < list.size(); it++){
								if(list[it].key_num == key_num){
									list[it].sendUnencryptedData(send_buf, rc);
									break;
								}
							}
							
          				}
          				else
          				{
            				if (rc < 0)
            				{
              					fprintf(stderr, "Error %d on read master PTY\nProbably exited\n", errno);
								for(int it = 0; it < list.size(); it++){
									if(list[it].key_num == key_num){
										list[it].endConnection(0);
										break;
									}
								}
              					return NULL;
            				}
          				}
        			}
      			}
    		} // End switch
  		} // End while
	return NULL;
}

unsigned char Connection::openTerminal()
{
	pid_t fork_pid;
	int rc;

	pipe(command_pipe);
	
	fdm = posix_openpt(O_RDWR);
	if (fdm < 0)
	{
		fprintf(stderr, "Error %d on posix_openpt()\n", errno);
		return 1;
	}

	rc = grantpt(fdm);
	if (rc != 0)
	{
		fprintf(stderr, "Error %d on grantpt()\n", errno);
		return 1;
	}

	rc = unlockpt(fdm);
	if (rc != 0)
	{
		fprintf(stderr, "Error %d on unlockpt()\n", errno);
		return 1;
	}

	
	fds = open(ptsname(fdm), O_RDWR);

	// Create the child process
	fork_pid = fork();
	if (fork_pid)
	{	
  		// FATHER

 	 	close(fds);
		
		child_pid = fork_pid;
		
		signal(SIGCHLD, SIG_IGN);


		TerminalHandler t;
		t.key_num = key_num;
		t.fdm = fdm;
		t.fds = fds;
		t.command_pipe[0] = command_pipe[0];
		t.command_pipe[1] = command_pipe[1];
		pthread_create(&terminalThread, NULL, handleTerminal, (void *)&t);
	}
	else
	{
		
		struct termios slave_orig_term_settings; // Saved terminal settings
		struct termios new_term_settings; // Current terminal settings

  		// CHILD

  		// Close the master side of the PTY
  		close(fdm);

  		// Save the defaults parameters of the slave side of the PTY
  		rc = tcgetattr(fds, &slave_orig_term_settings);

  		// Set RAW mode on slave side of PTY
  		new_term_settings = slave_orig_term_settings;
  		cfmakeraw (&new_term_settings);
  		tcsetattr (fds, TCSANOW, &new_term_settings);

  		// The slave side of the PTY becomes the standard input and outputs of the child process
  		close(0); // Close standard input (current terminal)
  		close(1); // Close standard output (current terminal)
  		close(2); // Close standard error (current terminal)

  		dup(fds); // PTY becomes standard input (0)
  		dup(fds); // PTY becomes standard output (1)
  		dup(fds); // PTY becomes standard error (2)

  		// Now the original file descriptor is useless
  		close(fds);

  		// Make the current process a new session leader
  		setsid();

  		// As the child is a session leader, set the controlling terminal to be the slave side of the PTY
  		// (Mandatory for programs like the shell to make them manage correctly their outputs)
  		ioctl(0, TIOCSCTTY, 1);

  		// Execution of the program
  		{
 			char **child_av;

    		// Build the command line
    		child_av = (char **)malloc(2 * sizeof(char *));
    		
      		child_av[0] = strdup("/bin/sh");
    		
    		child_av[1] = NULL;
   			rc = execvp(child_av[0], child_av);
  		}

  		// if Error...
  		return 1;
	}

	return 0;
} 


