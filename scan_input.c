#include<stdio.h>
#include<string.h>
#include "header.h"

extern char prompt[100];
extern char *input_string[100];
int pid;
int status;


extern struct stop stopped_processes[50];				// to store the stopped processes
int index_stop = 0;					// to keep track of number of stopped processes
char ip_string[100];

void signal_handler(int sig_num)
{
	if (sig_num == SIGINT )
	{
		if ( pid == 0 )
		{
			printf("\n%s", prompt);
			fflush(stdout);
		}
	}

	if (sig_num == SIGTSTP)
	{
		if ( pid == 0 )
		{
			printf("\n%s", prompt);
			fflush(stdout);
		}
		else
		{
			stopped_processes[index_stop].pid = pid; 	 // storing the pid of stopped process
			strcpy(stopped_processes[index_stop].name, ip_string); // storing the name of stopped process
			index_stop++;
		}
	}

	if (sig_num == SIGCHLD)
	{
		waitpid(-1, NULL, WNOHANG);//do not block
	}
}

char *external_commands[153];
void scan_input(char* prompt, char* input_string)
{
	signal(SIGINT, signal_handler);
	signal(SIGTSTP, signal_handler);

	extract_external_commands(external_commands);

	while(1)
	{
		printf("%s",prompt);
		
		scanf("%[^\n]",input_string);
		getchar();

		strcpy(ip_string, input_string);  // copying input string to global variable

		if(strncmp(input_string,"PS1=",4) == 0 )
		{
			if(input_string[4] != ' ')
			{
				strcpy(prompt,input_string+4);
				continue;
			}
			else
			{
				printf("Command not found\n");
				continue;
			}
		}

		char cmd[50];
		char* command = get_command(input_string,cmd);

		int type = check_command_type(command);

		if( type == BUILTIN )
		{
			execute_internal_commands(input_string);
		}
		else if( type == EXTERNAL )
		{
			//printf("EXTERNAL COMMAND\n");
			pid = fork();
			if( pid == 0 )
			{
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
				execute_external_commands(input_string);
				exit(0);
			}
			else
			{
				waitpid(pid,&status,WUNTRACED); //even though it is getting stopped it will return to parent
				pid=0;
			}
			//printf("EXTERNAL COMMAND\n");

		}
		/*else
		{
			printf("NO COMMAND\n");
		} */
	}
}


