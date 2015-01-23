#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
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
	signal(SIGTERM, quit);

	char input[MAXLINE];
	char** tokens;
	
	while(1) { 
		printf("$ "); // The prompt
		fflush(stdin);

		char *in = fgets(input, MAXLINE, stdin); // Taking input one line at a time
		//Checking for EOF
		if (in == NULL){
			if (DEBUG) printf("jash: EOF found. Program will exit.\n");
			break ;
		}

		// Calling the tokenizer function on the input line    
		tokens = tokenize(input);	
		// Executing the command parsed by the tokenizer
		execute_command(tokens); 
		
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
	if (tokens == NULL) {
		// Null Command
		return -1 ; 				
	} else if (tokens[0] == NULL) {
		// Empty Command
		return 0 ;					
	} else if (!strcmp(tokens[0],"exit")) {
		/* Quit the running process */
		exit(0);
	} else if (!strcmp(tokens[0],"cd")) {
		/* Change Directory, or print error on failure */
		int err = chdir(tokens[1]);
		if(err == -1){
			if(errno==ENOTDIR){
				cerr<<"Error: The path provided is not that of a directory"<<endl;
			}
			else if(errno==ENOENT){
				cerr<<"Error: The path to the directory does not exist"<<endl;
			}
			else{
				cerr<<"Erro: Following error code occurred while executing cd: "<<errno<<endl;
			}
		}
		else{
			char *curr_dir = get_current_dir_name();
			cout<<"Current Directory changed to: "<<curr_dir<<endl;
			free(curr_dir);
		}
		return 0 ;
	} else if (!strcmp(tokens[0],"parallel")) {
		/* Analyze the command to get the jobs */
		/* Run jobs in parallel, or print error on failure */
		return 0 ;
	} else if (!strcmp(tokens[0],"sequential")) {
		/* Analyze the command to get the jobs */
		/* Run jobs sequentially, print error on failure */
		/* Stop on failure or if all jobs are completed */
		return 0 ;					// Return value accordingly
	} else {
		/* Either file is to be executed or batch file to be run */
		/* Child process creation (needed for both cases) */
		int pid = fork() ;
		if (pid == 0) {
			if (!strcmp(tokens[0],"run")) {
				/* Locate file and run commands */
				/* May require recursive call to execute_command */
				/* Exit child with or without error */
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
				}
				exit(0) ;
			}
		}
		else {
			/* Parent Process */
			/* Wait for child process to complete */
			int status;
			while(waitpid(pid, &status, WNOHANG) != pid);
		}
	}
	return 1 ;
}

void quit(int signum){
}
