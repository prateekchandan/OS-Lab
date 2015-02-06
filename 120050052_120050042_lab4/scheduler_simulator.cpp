#include <iostream>
#include <sstream>
#include <fstream>
#include "event_manager.h"
#include <climits>
using namespace std;

int debug=0;

void handling_PROCESS_SPEC_file(){
	string line, line2;
	int pid, prior;
	int adm;
	int iter;
	int cpu_t, io_t;
	ifstream infile("PROCESS_SPEC");
	while (std::getline(infile, line))
	{
		if(line=="PROCESS"){
			process proc;
			getline(infile, line2);
			std::istringstream iss(line2);
		        if (!(iss >> pid >> prior >> adm)) { break; } // error
		    
			proc.pid = pid;
			proc.start_priority = prior;
			proc.admission = adm;

			getline(infile, line2);
			while(line2 != "END"){
				std::istringstream iss(line2);
				process_phase pp;
			        if (!(iss >> iter >> cpu_t >> io_t)) { break; } // error
			    
				pp.itrs = iter;
			    	pp.cpu_b = cpu_t;
			    	pp.cpu_left = cpu_t;
			    	pp.io_b = io_t;
			    	(proc.phases).push_back(pp);
			    	getline(infile, line2);
			}
			p_list.push_back(proc);
			em.add_event(proc.admission,1,proc.pid);	//event type "1" represents "process admission event"

		}
	}
	
	for(int i = 0; i<p_list.size(); i++){
		p_map[p_list[i].pid] = &p_list[i];
	}
}

int string_to_integer(string str)
{
	int r=1,s=0,l=str.length(),i;
	for(i=l-1;i>=0;i--)
	{
		s = s + ((str[i] - '0')*r);
		r *= 10;
	}
	return s;
}

void handling_SCHEDULER_SPEC_file(){
	string line, line2;
	int level_count;
	int prior;
	int s_lvl;
	int t_slice;
	string t_slice1;
	ifstream infile("SCHEDULER_SPEC");
	while (std::getline(infile, line))
	{
		if(line=="SCHEDULER"){
			getline(infile, line2);
			std::istringstream iss(line2);
		    if (!(iss >> level_count)) { break; } // error
			
			Scheduler.no_of_levels = level_count;
			for(int i=0; i<level_count; i++){
				getline(infile, line2);
				std::istringstream iss(line2);
				if (!(iss >> s_lvl >> prior >> t_slice1)) { break; } // error
				scheduling_level scl;
				if(t_slice1 == "N")
					t_slice = 9999999;
				else
					t_slice = string_to_integer(t_slice1);
				scl.level_no = s_lvl;
				scl.priority = prior;
				scl.time_slice = t_slice;
				
				Scheduler.levels_arr.push_back(scl);
			}
		}
	}

	for (int i = 0; i < Scheduler.levels_arr.size(); ++i)
	{
		scheduling_level s = Scheduler.levels_arr[i];
		inv_priority[s.priority]= i;
	}
}

class comp2{
	public:
 	int operator() ( const process *p1, const process *p2)
 	{
 		if(p1->start_priority == p2->start_priority)
 			return p1->ready_state_time > p2->ready_state_time;
 		else
 			return p1->start_priority < p2->start_priority;
 	}
};

// Other Global terms
priority_queue<process*, vector<process*>, comp2> ready_processes;
bool cpu_free = true;
int curr_pid = -1;
event next;

// Insert a process into ready state queue
void push_into_ready(int pid,int mode = 0){
	process *p = p_map[pid];
	process_phase *pp = &(p->phases[p->curr_phase]);
	if(mode == 1)
		pp->cpu_left = pp->cpu_b;
	else
		pp->cpu_left -= (next.end_t - p->started_time); 

	p->ready_state_time = next.end_t;
	pp->mode = 0;
	ready_processes.push(p);
}

// Starts a process i.e. Starting CPU burst
void start_process(int pid){
	process *p = p_map[pid];
	process_phase *pp = &(p->phases[p->curr_phase]);
	
	curr_pid = pid;			// Chnaged current running pid
	p->started_time = next.end_t;
	pp->mode = 0;

	cpu_free =false;

	int time_slice;
	if(inv_priority.find(p->start_priority) == inv_priority.end()){
		time_slice = INT_MAX;
		//cout<<p->start_priority<<"  &&\n";
	}
	else{
		scheduling_level s = Scheduler.levels_arr[inv_priority[p->start_priority]];
		time_slice = s.time_slice;
	}
	
	if(debug)
		cout<<pid<<" : "<<pp->cpu_left<<"  $$ \n";

	if(time_slice < pp->cpu_left)
		em.add_event(next.end_t + time_slice,2,pid);
	else
		em.add_event(next.end_t + pp->cpu_left,3,pid);

	cout<<"PID :: "<<pid<<"  TIME :: "<<next.end_t<<"  EVENT :: CPU started\n";
}

// Promotes or demotes a process
void promote_demote(const int &pid , int mode){
	process *p = p_map[pid];
	process_phase *pp = &(p->phases[p->curr_phase]);
	
	if(inv_priority.find(p->start_priority) == inv_priority.end()){
		if(debug)
			cout<<"Priority level not found \n";
		return;
	}

	int index=inv_priority[p->start_priority];
	//cout<<"Pid : "<<pid<<" , index : "<<p->start_priority<<endl;;
	if(mode == 0)
		index++;
	else
		index--;
	if(index==-1 || index == Scheduler.no_of_levels)
		return ;
	//cout<<Scheduler.no_of_levels<< " : "<<index<<endl;

	p->start_priority = Scheduler.levels_arr[index].priority;
	if(mode==0)
		cout<<"PID :: "<<pid<<"  TIME :: "<<next.end_t<<"  EVENT :: Priority promoted to level "<<p->start_priority<<"\n";
	else
		cout<<"PID :: "<<pid<<"  TIME :: "<<next.end_t<<"  EVENT :: Priority demoted to level "<<p->start_priority<<"\n";
}

void promote(const int &pid){promote_demote(pid,0);}
void demote(const int &pid){promote_demote(pid,1);}

// TO check if a process is terminated
bool check_terminate(const int &pid){
	process *p = p_map[pid];
	process_phase *pp = &(p->phases[p->curr_phase]);
	pp->mode=0;
	pp->curr_itr++;

	if(pp->curr_itr == pp->itrs){
		p->curr_phase++;
		if(p->curr_phase==p->phases.size()){
			cout<<"PID :: "<<pid<<"  TIME :: "<<next.end_t<<"  EVENT :: Process terminated\n";
			return true;
		}
	}
	return false;
}

int main()
{
	
	handling_PROCESS_SPEC_file();
	handling_SCHEDULER_SPEC_file();
	
	while(!em.is_empty()){
		next = em.next_event();

		// Process Admission case
		if(next.type == "Process admission"){
			cout<<"PID :: "<<next.pid<<"  TIME :: "<<next.end_t<<"  EVENT :: Process Admitted\n";
			push_into_ready(next.pid,1);
			process *p = ready_processes.top();
			if(cpu_free){
				ready_processes.pop();
				start_process(p->pid);
			}
			else if(p_map[curr_pid]->start_priority < p->start_priority){
				cout<<"PID :: "<<curr_pid<<"  TIME :: "<<next.end_t<<"  EVENT :: Process pre-empted\n";
				push_into_ready(curr_pid,0);
				ready_processes.pop();
				start_process(p->pid);
			}
		}
		// End of timeslice case
		else if(next.type == "End of time-slice"){
			if(cpu_free && debug)
				cout<<"ERROR : CPU IS FREE AT THE END OF TIME SLICE\n";
			cout<<"PID :: "<<curr_pid<<"  TIME :: "<<next.end_t<<"  EVENT :: Time slice ended\n";
			demote(next.pid);
			push_into_ready(next.pid,0);
			process *p = ready_processes.top();
			ready_processes.pop();
			start_process(p->pid);			
		}
		// IO start case or end of CPU Burst
		else if(next.type == "IO start"){
			process *p = p_map[curr_pid];
			if(curr_pid != next.pid) continue;
			if(next.end_t - (p->started_time) < p->phases[p->curr_phase].cpu_left) continue;
			cout<<"PID :: "<<curr_pid<<"  TIME :: "<<next.end_t<<"  EVENT :: CPU burst completed\n";
			cout<<"PID :: "<<curr_pid<<"  TIME :: "<<next.end_t<<"  EVENT :: IO started\n";
			promote(curr_pid);
			em.add_event(next.end_t + p->phases[p->curr_phase].io_b , 4 , curr_pid);
			cpu_free = true;
			if(!ready_processes.empty()){
				p = ready_processes.top();
				ready_processes.pop();
				start_process(p->pid);
			}
		}
		// IO end Case
		else if(next.type == "IO end"){
			cout<<"PID :: "<<next.pid<<"  TIME :: "<<next.end_t<<"  EVENT :: IO burst completed\n";
			bool status = check_terminate(next.pid);
			if(!status){
				push_into_ready(next.pid , 1);
				process *p = ready_processes.top();
				if(cpu_free){
					ready_processes.pop();
					start_process(p->pid);
				}
				else if(p_map[curr_pid]->start_priority < p->start_priority){
					cout<<"PID :: "<<curr_pid<<"  TIME :: "<<next.end_t<<"  EVENT :: Process pre-empted\n";
					push_into_ready(curr_pid,0);
					ready_processes.pop();
					start_process(p->pid);
				}
			}
		}
	}
	
	return 0;
}
