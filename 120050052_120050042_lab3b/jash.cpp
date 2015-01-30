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
#include <fcntl.h>
using namespace std;

#define MAXLINE 1000
// Debug = 1 will print various debug messages
#define DEBUG 0

/* Function declarations and globals */
extern char **environ;
int parent_pid ;
int old_outfd,old_infd;
char ** tokenize(char*) ;
int execute_command(char** tokens) ;
bool is_piped(char **tokens, char **temp1, char **temp2);
void false_quit(int signum);
void main_quit(int signum);
void make_default_stdio();

//declared global to free memory
char** tokens;

// Main function : Takes care of the Jash program itself
int main(int argc, char** argv){
	
	parent_pid = getpid() ;
	
	/* Set & defined appropriate signal handlers */
	signal(SIGINT, false_quit);
	signal(SIGQUIT, false_quit);
	signal(EOF, main_quit);

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
			cout<<"Return Status is: "<<status<<endl;
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
			close(1);       /* close normal stdout */
			dup(pfds[1]);   /* make stdout same as pfds[1] */
			close(pfds[0]);
			exit(execute_command(temp1));
		}
		else{
			/* In parent process */
			/* Wait for child process to complete & obtain status*/
			int status;
			while(waitpid(pid, &status, WNOHANG) != pid);
			if(status!=0) return -1;

			int pid1 = fork();
		
			if(pid1 == -1){
				// Fork Failed
				perror("Pipe: Forking failed in piped command");
				return -1;
			}
			else if(pid1 == 0){
				close(0);       /* close normal stdin */
				dup(pfds[0]);   /* make stdin same as pfds[0] */
				close(pfds[1]);
				exit(execute_command(temp2));
			}
			else{
				/* In parent process */
				/* Wait for child process to complete & obtain status*/
				
				while(waitpid(pid1, &status, WNOHANG) != pid1);
				if(status==0) return 0;
				else return -1;
			}
			
		}

		return 0;
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
		int st=0 , last = 0, gotout=0, gotin =0 ,outmode;
		char *infile , *outfile;

		while(tokens[st]!=NULL){
			if(!strcmp(tokens[st] , ">")){
				
				if(!(gotin || gotout))
					last = st;

				if(gotout){
					cerr<<"Error: Multiple output streams"<<endl;
					exit(-1);
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
					exit(-1);
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
					exit(-1);
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
			old_outfd = dup(fileno(stdout));
			old_infd = dup(fileno(stdin));

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
					cerr<<"Error: The file name corresponds to a directory"<<endl;
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
			else return -1;
		}
	}
	return 0;
}

/* Function for Signal Handling in main for ignored signals */
void false_quit(int signum){
	cout<<endl;
}

/* Function for Signal Handling in main for SIGHUP */
void main_quit(int signum){
	cout<<endl;
	fflush(stdout);
	for(int i=0;tokens[i]!=NULL;i++){
		free(tokens[i]);
	}
	free(tokens);
	exit(0);
}

void make_default_stdio(){
	
	close(fileno(stdin));
    dup2(old_infd, fileno(stdin));
    close(old_infd);

    close(fileno(stdout));
    dup2(old_outfd, fileno(stdout));
    close(old_outfd);
}