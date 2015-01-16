#include <iostream>
#include <vector>
#include <map>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
using namespace std;

pair<string, string> get_id_domain(string &s){
	int pos = s.find('@');
	string a="",b="";
	
	if(pos==s.length()){
		cout<<"Invalid email ID format\n";
		return make_pair(a,b);
	}
	
	a = s.substr(0,pos);
	b = s.substr(pos+1);
	return make_pair(a,b);
}

void child(){
	
	map
int main(){

	// Creating shared memory
	int shmID,count=10;

	while(shmID<0){
		if(count==0){
			cout<<"Could not create shared memory! Program quitting\n";
			return 0;
		}
		shmID = shmget(IPC_PRIVATE, 1000, IPC_CREAT);
		if(shmID<0){
			cout<<"Failed to create shared memory. Retrying again !\n";
		}
		count--;
	}
	
	// Initializing map of domain name : PID
	map<string,pid_t> mail_servers;
	pid_t pid;
	
	// Keep processing requests in a loop
	while(true){
		
		string a, email_id;
		map<string,pid_t>::iterator it;
		cin>>a;
		
		if(a.compare("add_email")==0){
			cin>>email_id;
			pair<string,string> id = get_id_domain(email_id);
			it = mail_servers.find(id.second);
			
			// If process doesn't exist
			if(it==mail_servers.end()){
				if((pid = fork())==0){
					mail_servers[id.second] = pid;
					child();
				}
				else{
					cout<<"Failed to create process for the new mail server"<<endl;
				}
			}
			
			// Else, process exists
		}
		else if(a.compare("search_email")==0){
		}
		else if(a.compare("delete_email")==0){
		}
		else if(a.compare("delete_domain")==0){
		}
		else if(a.compare("send_email")==0){
		}
		else{
			cout<<"Unknown Function\n";
		}
	}
	
	return 0;
	
}
