#include "header.h" 

extern char* external_commands[153];

char *builtins[] = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs", "let", "eval",
						"set", "unset", "export", "declare", "typeset", "readonly", "getopts", "source",
						"exit", "exec", "shopt", "caller", "true", "type", "hash", "bind", "help", "jobs", "fg", "bg", NULL};

struct stop stopped_processes[50];				// to store the stopped processes 

char* get_command(char* input_string,char* cmd)
{
	int i=0;
	while( input_string[i] != ' ' && input_string[i] != '\0' )
	{
		cmd[i] = input_string[i];
		i++;
	}
	cmd[i] = '\0';
	return cmd;
}

void extract_external_commands(char **external_commands)
{
	int fd = open("external_commands.txt",O_RDONLY);
	
	int i=0,index=0;
	char ch;
	char arr[50];

	while( read(fd,&ch,1) > 0 )
	{
		if( ch == '\n' )
		{
			arr[i] = '\0';
			external_commands[index] = malloc(strlen(arr)+1);
			strcpy(external_commands[index],arr);
			index++;
			i=0;
		}
		else
		{
			arr[i++] = ch;
		}
	}

	external_commands[index] = NULL;
	close(fd);
}


int check_command_type(char* command)
{
	int i=0;
	while( builtins[i] != NULL )
	{
		if( strcmp(command,builtins[i]) == 0 )
		{
			return BUILTIN;
		}
		i++;
	}
	
	i=0;

	while( external_commands[i] != NULL )
	{
		if( strncmp(command,external_commands[i],strlen(command)) == 0 )
		{
			return EXTERNAL;
		}
		i++;
	}

	return NO_COMMAND;
}

extern int status;
extern int index_stop;
void execute_internal_commands(char* input_string)
{
	if (strncmp(input_string, "exit", 4) == 0 &&
        (input_string[4] == '\0' || input_string[4] == ' '))
	{
		exit(0);
	}
	else if ( (strcmp(input_string, "pwd") == 0) && (input_string[3] == '\0') )
	{
		char buff[100];
		getcwd(buff,100);
		printf("%s\n",buff);
	}
	else if (strncmp(input_string, "cd", 2) == 0 &&
             input_string[2] == ' ')
	{
		char* token = strtok(input_string, " ");
		token = strtok(NULL," ");
		chdir(token);
	}
	else if (strcmp(input_string, "echo $$") == 0)  // print the pid of the shell or current process
	{
		printf("%d\n", getpid());
	}
	else if (strcmp(input_string, "echo $?") == 0)  // print the exit status of last command
	{
		if( WIFEXITED(status) )
		{
			printf("%d\n", WEXITSTATUS(status));
		}
		else
		{
			printf("%d\n", 555);
		}
	}
	else if (strcmp(input_string, "echo $SHELL") == 0)  // print the shell name
	{
		printf("%s\n",getenv("SHELL"));
	}
	else if (strcmp(input_string, "jobs") == 0)  // print the list of stopped processes
	{
		for( int i=0; i<index_stop; i++ )
		{
			printf("[%d] Stopped\t %s [%d]\n", i+1, stopped_processes[i].name, stopped_processes[i].pid );
		}
	}
	else if (strcmp(input_string, "fg") == 0)  // bring the last stopped process to foreground
	{
		if( index_stop > 0 )
		{
			int last_index = index_stop - 1;
			pid_t pid = stopped_processes[last_index].pid;

			// removing the last stopped process from the list
			index_stop--;

			kill(pid, SIGCONT);  // sending continue signal to the stopped process

			int wstatus;
			waitpid(pid, &wstatus, WUNTRACED);
			
		}
		else
		{
			printf("fg: no current job\n");
		}
	}
	else if (strcmp(input_string, "bg") == 0)  // bring the last stopped process to background
	{
		if( index_stop > 0 )
		{
			signal(SIGCHLD, signal_handler);
			int last_index = index_stop - 1;
			pid_t pid = stopped_processes[last_index].pid;

			// removing the last stopped process from the list
			index_stop--;

			kill(pid, SIGCONT);  // sending continue signal to the stopped process
			
		}
		else
		{
			printf("bg: no current job\n");
		}

	}
	else
	{
		printf("yet to be done\n");
	}
}
void execute_external_commands(char *input_string)
{
	int c = 0;
	for (int i = 0; input_string[i] != '\0'; i++)
	{
		if (input_string[i] == ' ' || input_string[i + 1] == '\0')
			c++;
	}
	char **argv = malloc((c + 1) * sizeof(char *));
	int v = 0;
	int p = 0;
	char a[50];
	for (int i = 0, j = 0;; i++)
	{
		if (input_string[i] == ' ' || input_string[i] == '\0')
		{
			if (j > 0)
			{
				a[j] = '\0';
				argv[v] = malloc(strlen(a) + 1);
				strcpy(argv[v], a);
				v++;
				j = 0;
				if (input_string[i] == '\0')
					break;
			}
		}
		else
		{
			a[j++] = input_string[i];
		}
	}
	argv[v] = NULL;
	for (int i = 0; i < v; i++)
	{
		if (strcmp(argv[i], "|") == 0)
		{
			p = 1;
			break;
		}
	}

	if (p == 0)
	{
		if (execvp(argv[0], argv) < 0)
		{
			printf("Command not found\n");
		}
	}

	else
	{
		int arr[v];
		int pipe_count = 0;
		int idx = 0;

		arr[idx++] = 0;

		for (int i = 0; i < v; i++)
		{
			if (strcmp(argv[i], "|") == 0)
			{
				argv[i] = NULL;
				arr[idx++] = i + 1;
				pipe_count++;
			}
		}

		int prev_fd = -1;
		int fd[2];

		for (int i = 0; i <= pipe_count; i++)
		{
			if (i != pipe_count)
			{
				if (pipe(fd) == -1)
				{
					perror("pipe");
					exit(1);
				}
			}

			pid_t pid = fork();

			if (pid == 0) // Child
			{
				if (prev_fd != -1)
				{
					dup2(prev_fd, 0);
					close(prev_fd);
				}

				if (i != pipe_count)
				{
					dup2(fd[1], 1);
					close(fd[0]);
					close(fd[1]);
				}

				execvp(argv[arr[i]], argv + arr[i]);
				perror("execvp");
				exit(1);
			}

			// Parent
			if (prev_fd != -1)
				close(prev_fd);

			if (i != pipe_count)
			{
				close(fd[1]);
				prev_fd = fd[0];
			}
		}

		// Wait for all children
		for (int i = 0; i <= pipe_count; i++)
			wait(NULL);
	}
}



