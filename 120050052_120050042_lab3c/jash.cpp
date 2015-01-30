#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <fcntl.h>
#include <time.h>
using namespace std;

#define MAXLINE 1000
// Debug = 1 will print various debug messages
#define DEBUG 0

/* Function declarations and globals */
extern char **environ;
int parent_pid ;
char ** tokenize(char*) ;
int execute_command(char** tokens) ;
bool is_piped(char **tokens, char **temp1, char **temp2);
void false_quit(int signum);
void main_quit();
void sigchild_handler(int sig);
bool is_backgroud(char **tokens);

// Structure  for a cron job
struct cron{
	int h,m;
	char **command;
	int wait_time;

	cron(char **tokens){
		if(!strcmp(tokens[0],"*"))
			m = -1;
		else
			m = atoi(tokens[0]);

		if(!strcmp(tokens[1],"*"))
			h = -1;
		else
			h = atoi(tokens[1]);

		command = tokens + 2;

		if(is_backgroud(command));

		int i=0;
		while(command[i]!=NULL){
			cout<<command[i]<<" ";
			i++;
		}
		wait_time = find_wait_time();

	}

	int find_wait_time(){
		time_t now;
		struct tm *now_tm;

		now = time(NULL);
		now_tm = localtime(&now);
		int hour = now_tm->tm_hour;
		int minute = now_tm->tm_min;

		if(h == -1 && m == -1)
			return 0;

		if(h==-1)
			return ((m+60-minute)%60)*60;
		
		if(m==-1){
			if(h==hour)
				return 0;
			else{
				int hdiff = (h+24 - hour)%24 - 1;
				return hdiff * 60 * 60 + (60-minute) * 60;
			}
		}

		else{
			int m1 = h*60+m;
			int m2 = hour*60+minute;

			return ((m1 + 1440 -m2) % 1440) * 60;
		}

	}
};

//declared global to free memory
char** tokens;

// To store all runing background proceeses
vector<int> backgroud_child_processes;

// Main function : Takes care of the Jash program itself
int main(int argc, char** argv){
	
	parent_pid = getpid() ;
	
	/* Set & defined appropriate signal handlers */
	signal(SIGINT, false_quit);
	signal(SIGQUIT, false_quit);
	atexit(main_quit);

	/* Variables concerning command and its token */
	char input[MAXLINE];
	
	while(1) { 
		printf("$ "); // The prompt
		fflush(stdout);
		fflush(stdin);

		char *in = fgets(input, MAXLINE, stdin); // Taking input one line at a time
		
		//Checking for EOF
		if (in == NULL){
			if (DEBUG) printf("jash: EOF found. Program will exit.\n");
			break ;
		}

		int status; // return status of a command
		
		// Calling the tokenizer function on the input line    
		tokens = tokenize(input);	
		
		// Executing the command parsed by the tokenizer
		status = execute_command(tokens); 
		if(DEBUG){
			cout<<"Return Status is: "<<(status<0 ? -1: 0)<<endl;
			fflush(stdout);
		}
		
		// Freeing the allocated memory	
		int i ;
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);
	}
	
	/* Kill all Children Processes and Quit Parent Process */
	return 0 ;
}

/*The tokenizer function takes a string of characters and forms tokens out of it*/
char ** tokenize(char* input){
	
	int i, doubleQuotes = 0, tokenIndex = 0, tokenNo = 0 ;
	char *token = (char *)malloc(MAXLINE*sizeof(char));
	char **tokens;

	tokens = (char **) malloc(MAXLINE*sizeof(char*));

	for(i =0; i < (int)strlen(input); i++){
		char readChar = input[i];

		if (readChar == '"'){
			doubleQuotes = (doubleQuotes + 1) % 2;
			if (doubleQuotes == 0){
				token[tokenIndex] = '\0';
				if (tokenIndex != 0){
					tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
					strcpy(tokens[tokenNo++], token);
					tokenIndex = 0; 
				}
			}
		}
		else if (doubleQuotes == 1){
			token[tokenIndex++] = readChar;
		}
		else if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
			token[tokenIndex] = '\0';
			if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
			}
		}
		else{
			token[tokenIndex++] = readChar;
		}
	}

	if (doubleQuotes == 1){
		token[tokenIndex] = '\0';
		if (tokenIndex != 0){
			tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
			strcpy(tokens[tokenNo++], token);
		}
	}

	tokens[tokenNo] = NULL ;
	
	/* printing tokens for debug purpose */
	if(DEBUG){
		cout<<"Here are the tokens:"<<endl;
		for(int i=0; i<tokenNo; i++){
			cout<<tokens[i]<<" , ";
		}
		cout<<endl;
	}
	///////////////////////////////////////
	
	return tokens;
}

/* Function to check for pipe and break commands suitably */
bool is_piped(char **tokens, char **temp1, char **temp2){
	
	int i=0;	
	// Collect the tokens of commands one after another
	while(tokens[i]!=NULL && strcmp(tokens[i],"|")){
		temp1[i] = tokens[i];
		i++;
	}
	temp1[i] = NULL;
	
	if(tokens[i]==NULL){
		return false;
	}
	i++;
	
	int j=0;
	while(tokens[i]!=NULL){
		temp2[j] = tokens[i];
		i++;
		j++;
	}
	temp2[j] = NULL;
	return true;
}

bool is_backgroud(char **tokens){

	int i=0;
	while(tokens[i]!=NULL){
		if(!strcmp(tokens[i],"&") && tokens[i+1] == NULL){
			free(tokens[i]);
			tokens[i]=NULL;
			return true;
		}
		i++;
	}
	return false;
}

/* Function for taking a particular command and executing it */
int execute_command(char** tokens){
	/* 
	Takes the tokens from the parser and based on the type of command 
	and proceeds to perform the necessary actions. 
	Returns 0 on success, -1 on failure. 
	*/
	
	char **temp1, **temp2; // Used if pipe is present
	temp1 = (char **) malloc(MAXLINE*sizeof(char*));
	temp2 = (char **) malloc(MAXLINE*sizeof(char*));
	
	if(tokens == NULL){
		// Null Command
		return -1 ; 				
	} 
	
	else if(tokens[0] == NULL){
		// Empty Command
		return 0 ;					
	}

	else if(is_backgroud(tokens)){
		
		signal(SIGCHLD,sigchild_handler);
		int pid=fork();

		if(pid==-1){
			perror("Failed to create background process");
			return -1;
		}
		else if(pid==0){
			// To define signal handler
			
			/*signal(SIGINT, false_quit);
			signal(SIGQUIT, false_quit);*/

			setsid();

			int ret_status=execute_command(tokens);
			cout<<"\b\bCommand : ";
			int i=0;
			while(tokens[i]!=NULL)
				cout<<tokens[i++]<<" ";

			//int pid_num = (ret_status>0 ? ret_status : -ret_status);
			cout<<"\nPID : "<<getpid()+1<<endl;
			if(ret_status >= 0)
				cout<<"Status : Successful ";
			else
				cout<<"Status : Failed ";
			cout<<"\n$ ";

			int pid = getpid();
			vector<int>::iterator it = find(backgroud_child_processes.begin(), backgroud_child_processes.end(), pid);
			
			if(it != backgroud_child_processes.end()){
				backgroud_child_processes.erase(it);
			}
			exit(0);
		}
		else{
			backgroud_child_processes.push_back(pid);
		}
	}

	else if(!strcmp(tokens[0],"cron")){

		// Check if the path is a file
		if(tokens[1]==NULL){
			perror("No argument given to run command");
			exit(-1);
		}

		// Check if path is a directory
		struct stat statbuf;
		stat(tokens[1], &statbuf);
		if(S_ISDIR(statbuf.st_mode)){
			perror("The file name corresponds to a directory");
			exit(-1);
		}

		int pid=fork();

		// Process creation failed
		if(pid==-1){
			perror("Failed to create background process");
			return -1;
		}
		if(pid==0){

			// Detaching this file
			setsid();

			// Open the file and run the commands line by line
			FILE *file = fopen(tokens[1], "r" );
			if(file!=NULL){

				vector<cron> task;

				char command[MAXLINE];
				while (!feof(file)){
					if (fgets(command, MAXLINE, file)){
						char** inp_tokens = tokenize(command); // got tokens
						
						int i=0;
						while(inp_tokens[i]!=NULL)
							cout<<inp_tokens[i++]<<" ";

						// 1 or 2 in null
						if(inp_tokens==NULL || inp_tokens[0]==NULL || inp_tokens [1]==NULL)
							continue;	

						task.push_back(cron(inp_tokens));	
					}
				}

				fclose(file);

				if(task.size()>0){
					int l = task.size();
					while(1){
						for (int i = 0; i < l; ++i)
							task[i].wait_time = task[i].find_wait_time();
						
						for (int i = 0; i < l; ++i){
							if(task[i].wait_time == 0)
							{
								execute_command(task[i].command);
								task[i].wait_time = task[i].find_wait_time();
							}
						}

						int mintime = 99999999;
						for (int i = 0; i < l; ++i){
							if(task[i].wait_time < mintime)
								mintime = task[i].wait_time;
						}

						if(mintime == 0)
							sleep(60);

						else 
							sleep(mintime);

					}
				}

			}
			else{
				perror("Could not open the file (or) file does not exist");

				int pid = getpid();
				vector<int>::iterator it = find(backgroud_child_processes.begin(), backgroud_child_processes.end(), pid);
				
				if(it != backgroud_child_processes.end()){
					backgroud_child_processes.erase(it);
				}
				exit(-1);
			}

			int pid = getpid();
			vector<int>::iterator it = find(backgroud_child_processes.begin(), backgroud_child_processes.end(), pid);
			
			if(it != backgroud_child_processes.end()){
				backgroud_child_processes.erase(it);
			}
			exit(0);
		}
		else{
			backgroud_child_processes.push_back(pid);
		}
				
	}
	
	else if(!strcmp(tokens[0],"parallel")){
		/* Analyze the command to get the jobs */
		/* Run jobs in parallel, or print error on failure */
		int i=0;
		vector<int> allpid;
		
		// Collect the tokens of commands one after another
		while(tokens[i]!=NULL){
			i++;
			char **command = (char **) malloc(MAXLINE*sizeof(char*));
			int j=0;
			while(tokens[i]!=NULL && strcmp(tokens[i],":::")){
				command[j] = tokens[i];
				i++;
				j++;
			}
			command[j] = NULL;

			// Fork() to create a process for each command
			int pid = fork() ;
			if (pid == -1) {
				cerr<<"Error: Could not create child process"<<endl;
			}
			else if(pid==0){
				// Child process
				execute_command(command);
				free(command);
				exit(0);
			}
			else{
				// Parent process
				allpid.push_back(pid);
			}
			
			free(command);
		}

		// Wait for the child processes to finish
		for(int i=0; i<= (int)allpid.size(); i++){
			int status;
			waitpid(allpid[i],&status,0);
		}
		
		return 0 ;
	}
	
	else if(!strcmp(tokens[0],"sequential")){
		/* Implementation of sequential commands */
		int i=0;
		
		// Collect the tokens of commands one after another
		while(tokens[i]!=NULL){
			i++;
			char **command = (char **) malloc(MAXLINE*sizeof(char*));
			int j=0;
			while(tokens[i]!=NULL && strcmp(tokens[i],":::")){
				command[j] = tokens[i];
				i++;
				j++;
			}
			command[j] = NULL;

			// Execute the collected command
			int ret_val = execute_command(command);
			
			if(ret_val==-1){
				// Command execution failed
				cerr<<"Error: Aborting sequential execution due to failed return"<<endl;
				free(command);
				return -1;
			}
			free(command);
		}
		
		return 0 ;					// Return value = success
	}
	
	else if(!strcmp(tokens[0],"sequential_or")){
		/* Implementation of sequential_or commands */
		int i=0;
		
		// Collect the tokens of commands one after another
		while(tokens[i]!=NULL){
			i++;
			char **command = (char **) malloc(MAXLINE*sizeof(char*));
			int j=0;
			while(tokens[i]!=NULL && strcmp(tokens[i],":::")){
				command[j] = tokens[i];
				i++;
				j++;
			}
			command[j] = NULL;

			// Execute the collected command
			int ret_val = execute_command(command);
			
			if(ret_val==0){
				// Command execution succeeded for the first time, so return
				free(command);
				return 0;
			}
			free(command);
		}
		
		return -1 ;					// Return value = failed (since OR is false)
	}
	
	else if(is_piped(tokens,temp1,temp2)){
		
		/* Implementation of piped command */
		int pfds[2];
		pipe(pfds);
		int pid = fork();
		
		if(pid == -1){
			// Fork Failed
			perror("Pipe: Forking failed in piped command");
			return -1;
		}
		else if(pid == 0){
			close(pfds[0]);
			dup2(pfds[1],1);
			//execvp(temp1[0], temp);
			execute_command(temp1);
			exit(0);
		}

		int pid1 = fork();
		
		if(pid1 == -1){
			// Fork Failed
			perror("Pipe: Forking failed in piped command");
			return -1;
		}
		else if(pid1 == 0){
			close(pfds[1]);
			dup2(pfds[0],0);
			//execvp(temp2[0], temp2);
			execute_command(temp2);
			exit(0);
		}

		close(pfds[0]) ;
		waitpid(-1,NULL,0);
		close(pfds[1]) ;
		waitpid(-1,NULL,0);

		return getpid();
	}
		
	else if(!strcmp(tokens[0],"exit")){
		/* Quit the running process */
		exit(0);
	}
	
	else if(!strcmp(tokens[0],"cd")){
		/* Change Directory, or print error on failure */
		char *path = tokens[1];
		if(tokens[1]==NULL || !strcmp(tokens[1],"~")){
			path = getpwuid(getuid())->pw_dir;
		}
		int err = chdir(path);
		if(err == -1){
			// Cases of directory access error
			perror("Error in cd ");
			return -1;
		}
		else{ 
			// directory opened successfully
			char *curr_dir = get_current_dir_name();
			if(DEBUG){
				cout<<"Current Directory changed to: "<<curr_dir<<endl;
				fflush(stdout);
			}
			free(curr_dir);
		}
		return 0 ;
	}
	
	else{
		/* Either file is to be executed or batch file to be run */
		/* Child process creation (needed for both cases) */
		
		/* Also file I/O redirection is to be incorporated */
		int st=0 , last = 0, gotout=0, gotin =0 ,outmode;
		char *infile , *outfile;

		while(tokens[st]!=NULL){
			if(!strcmp(tokens[st] , ">")){
				
				if(!(gotin || gotout))
					last = st;

				if(gotout){
					cerr<<"Error: Multiple output streams"<<endl;
					return -1;
				}

				gotout=1;
				outfile = tokens[st+1];
				outmode=1;
				st++;
			}

			if(!strcmp(tokens[st] , ">>")){
				
				if(!(gotin || gotout))
					last = st;

				if(gotout){
					cerr<<"Error: Multiple output streams"<<endl;
					return -1;
				}

				gotout=1;
				outfile = tokens[st+1];
				outmode=2;
				st++;
			}

			if(!strcmp(tokens[st] , "<")){
				
				if(!(gotin || gotout))
					last = st;

				if(gotin){
					cerr<<"Error: Multiple Input streams"<<endl;
					return -1;
				}

				gotin=1;
				infile = tokens[st+1];
				st++;
			}

			st++;
		}
		if(gotout || gotin)
			tokens[last]=NULL;

		int pid = fork();
		
		if (pid == 0) {

			int infd,outfd;
		
			if(gotin){
				infd = open(infile, O_RDWR);
				if (infd < 0){
					cerr<<"Error: Invalid Input File"<<endl;
					exit(-1);
				}

				dup2(infd,0);
				close(infd);
			}

			if(gotout){
				if(outmode==1)
					outfd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0777);
				else{
					if(access(outfile, F_OK) == 0){
						outfd = open(outfile, O_RDWR | O_APPEND);
					}
					else
						outfd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0777);
				}

				if (outfd < 0){
					cerr<<"Error: Writing to file"<<endl;
					exit(-1);
				}

				dup2(outfd,1);
				close(outfd);
			}

			if (!strcmp(tokens[0],"run")) {
				/* Locate file and run commands */
				/* May require recursive call to execute_command */
				/* Exit child with or without error */
				if(tokens[1]==NULL){
					cerr<<"Error: No argument given to run command"<<endl;
					exit(-1);
				}
				
				// check whether the file is actually a directory
				struct stat statbuf;
				stat(tokens[1], &statbuf);
				if(S_ISDIR(statbuf.st_mode)){
					perror("The file name corresponds to a directory");
					exit(-1);
				}
				
				// Open the file and run the commands line by line
				FILE *file = fopen(tokens[1], "r" );
				if(file!=NULL){
					char command[MAXLINE];
					while (!feof(file)){
						if (fgets(command, MAXLINE, file)){
							char** inp_tokens = tokenize(command); // got tokens
							execute_command(inp_tokens);			// executed command
							
							// Free memory of tokens
							for(int i=0; inp_tokens[i] != NULL;i++){
								free(inp_tokens[i]);
							}
							free(inp_tokens);
						}
					}
					fclose(file);
				}
				else{
					perror("Could not open the file (or) file does not exist");
					exit(-1);
				}
				exit (0) ;
			}
			else {
				/* File Execution */
				/* Print error on failure, exit with error*/
				int err = execvp(tokens[0],tokens);
				if(err==-1){
					perror("Error");
					exit(-1);
				}
				exit(0);
			}
		}
		else{
			/* Parent Process */
			/* Wait for child process to complete & obtain status*/
			int status;
			while(waitpid(pid, &status, WNOHANG) != pid);
			if(status==0) return 0;
			else return -1*pid;
		}
		return getpid();
	}
	return 1;
}

/* Function for Signal Handling in main for ignored signals */
void false_quit(int signum){
	cout<<endl<<"$ ";
	fflush(stdout);
}

/* Function for Signal Handling in main for SIGHUP */
void main_quit(){
	for (int i = 0; i < (int)backgroud_child_processes.size(); ++i)
	{
		cout<<" Killed : "<<backgroud_child_processes[i]<<endl;
		killpg(backgroud_child_processes[i],SIGTERM);

		vector<int>::iterator it = find(backgroud_child_processes.begin(), backgroud_child_processes.end(), backgroud_child_processes[i]);
			
		if(it != backgroud_child_processes.end()){
			backgroud_child_processes.erase(it);
		}
	}
	exit(0);
}

/* Handler for parent of background processes (on child's death) */
void sigchild_handler(int sig){
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}
