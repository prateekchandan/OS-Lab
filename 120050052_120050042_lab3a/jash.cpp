#include <stdio.h>
#include <iostream>
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
using namespace std;

#define MAXLINE 1000
#define DEBUG 0

/* Function declarations and globals */
extern char **environ;
int parent_pid ;
char ** tokenize(char*) ;
int execute_command(char** tokens) ;
void quit(int signum);

int main(int argc, char** argv){
	parent_pid = getpid() ;
	
	/* Set (and define) appropriate signal handlers */
	/* Exact signals and handlers will depend on your implementation */
	signal(SIGINT, quit);
	signal(SIGQUIT, quit);

	char input[MAXLINE];
	char** tokens;
	//int status;
	
	while(1) { 
		printf("$ "); // The prompt
		fflush(stdin);

		char *in = fgets(input, MAXLINE, stdin); // Taking input one line at a time
		//Checking for EOF
		if (in == NULL){
			if (DEBUG) printf("jash: EOF found. Program will exit.\n");
			break ;
		}

		int status;
		// Calling the tokenizer function on the input line    
		tokens = tokenize(input);	
		// Executing the command parsed by the tokenizer
		status = execute_command(tokens); 
		cout<<"Status of return: "<<status<<endl;
		
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

/*The tokenizer function takes a string of chars and forms tokens out of it*/
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
		} else if (doubleQuotes == 1){
			token[tokenIndex++] = readChar;
		} else if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
			token[tokenIndex] = '\0';
			if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
			}
		} else {
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
	/*
	cout<<"Here are the tokens:"<<endl;
	for(int i=0; i<tokenNo; i++){
			cout<<tokens[i]<<" , ";
	}
	cout<<endl;
	*/
	/* ends here */
	
	return tokens;
}

int execute_command(char** tokens) {
	/* 
	Takes the tokens from the parser and based on the type of command 
	and proceeds to perform the necessary actions. 
	Returns 0 on success, -1 on failure. 
	*/
	if(tokens == NULL){
		// Null Command
		return -1 ; 				
	} 
	
	else if(tokens[0] == NULL){
		// Empty Command
		return 0 ;					
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
			if(errno==ENOTDIR){
				cerr<<"Error: The path provided is not that of a directory"<<endl;
			}
			else if(errno==ENOENT){
				cerr<<"Error: The path to the directory does not exist"<<endl;
			}
			else{
				cerr<<"Error: Following error code occurred while executing cd: "<<errno<<endl;
			}
			return -1;
		}
		else{
			char *curr_dir = get_current_dir_name();
			cout<<"Current Directory changed to: "<<curr_dir<<endl;
			free(curr_dir);
		}
		return 0 ;
	}
	
	else if(!strcmp(tokens[0],"parallel")){
		/* Analyze the command to get the jobs */
		/* Run jobs in parallel, or print error on failure */
		int i=0;
		vector<int> allpid;
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

			int pid = fork() ;
			if (pid == -1) {
				cerr<<"Error: Could not create child process"<<endl;
				continue;
			}
			else if(pid==0){
				// Child process
				execute_command(command);
				exit(0);
			}
			else{
				// Parent process
				allpid.push_back(pid);
			}
			
			free(command);
		}

		for(int i=0; i<= (int)allpid.size(); i++){
			int status;
			waitpid(allpid[i],&status,0);
		}
		
		return 0 ;
	}
	
	else if(!strcmp(tokens[0],"sequential")){
		/* Implementation of sequential commands */
		int i=0;
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

			int ret_val = execute_command(command);
			if(ret_val==-1){
					cerr<<"Error: Aborting sequential execution due to failed return"<<endl;
					return -1;
			}
			free(command);
		}
		
		return 0 ;					// Return value accordingly
	}
	
	else if(!strcmp(tokens[0],"sequential_or")){
		/* Implementation of sequential_or commands */
		int i=0;
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

			int ret_val = execute_command(command);
			if(ret_val==0){
					return 0;
			}
			free(command);
		}
		
		return -1 ;					// Return value accordingly
	}
	
	else{
		/* Either file is to be executed or batch file to be run */
		/* Child process creation (needed for both cases) */
		int pid = fork() ;
		if (pid == 0) {
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
					cerr<<"Error: The file name corresponds to a directory"<<endl;
					exit(-1);
				}
				
				FILE *file = fopen(tokens[1], "r" );
				if(file!=NULL){
					char command[MAXLINE];
					while (!feof(file)){
						if (fgets(command, MAXLINE, file)){
							char** inp_tokens = tokenize(command);
							execute_command(inp_tokens);
							for(int i=0; inp_tokens[i] != NULL;i++){
								free(inp_tokens[i]);
							}
							free(inp_tokens);
						}
					}
					fclose(file);
				}
				else{
					cerr<<"Error: Could not open the file (or) file does not exist"<<endl;
					exit(-1);
				}
				exit (0) ;
			}
			else {
				/* File Execution */
				/* Print error on failure, exit with error*/
				int err = execvp(tokens[0],tokens);
				if(err==-1){
					if(errno==ENOENT){
						cerr<<"Error: The file does not exist : "<<tokens[0]<<endl;
					}
					else{
						cerr<<"Error: Following error code occurred while file execution: "<<errno<<endl;
					}
					exit(-1);
				}
				exit(0);
			}
		}
		else{
			/* Parent Process */
			/* Wait for child process to complete */
			int status;
			while(waitpid(pid, &status, WNOHANG) != pid);
			if(status==0) return 0;
			else return -1;
		}
	}
	return 0;
}

void quit(int signum){
		cout<<endl;
}
