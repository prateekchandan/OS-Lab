#include <iostream>
#include <sstream>
#include <fstream>
#include "event_manager.h"

using namespace std;

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
					t_slice = 9999;
				else
					t_slice = string_to_integer(t_slice1);
				scl.level_no = s_lvl;
				scl.priority = prior;
				scl.time_slice = t_slice;
				
				Scheduler.levels_arr.push_back(scl);
			}
		}
	}
}

class comp2{
public:
 	int operator() ( const process *p1, const process *p2)
 	{
 		return p1->start_priority< p2->start_priority;
 	}
};

void push_into_ready(int pid, int time_now){
	process *p = p_map[pid];
	p->phases[p->curr_phase].mode = 0;
	p->phases[curr_phase].cpu_left = cpu_b;
	p->phases[curr_phase].started_time = time_now;
	ready_processes.push(p_map[pid]);
}

process* get_top_process(){
	process *p = ready_processes.top();
	ready_processes.pop();
	return p;
}

void set_mode(process *p, int mode){
	p->phases[p->curr_phase].mode = 0;
}

void set_started_time(process *p, int start_time){
	p->phases[p->curr_phase].started_time = start_time;
}

int main()
{
	
	handling_PROCESS_SPEC_file();
	handling_SCHEDULER_SPEC_file();
	priority_queue<process*, vector<process*>, comp2> ready_processes;
	event next;
	
	bool cpu_free = true;
	int curr_pid = -1;
	process *it;
	
	while(!em.is_empty()){
			next = em.next_event();
			
			//routine for handling process admission event
			if(next.type=="Process admission"){
				cout<<"PID :: "<< next.pid<<" TIME :: "<<next.end_t<<" EVENT :: Process Admitted\n";
				push_into_ready(next.pid, next.end_t);
				
				if(cpu_free){
					
					// CPU is free, so dispatch a process
					process *p = get_top_process();
					cout<<"PID :: "<< p->pid << " TIME :: " << next.end_t << " EVENT :: Process dispatched\n";
					curr_pid = p->pid;
					cpu_free = false;
					set_mode(p,0);
					set_started_time(p,next.end_t);
					
					
					em.add_event(next.end_t+p->phases[p->curr_phase].cpu_left,3,p->pid);
				}
				else{
					// other process is running
					if(p_map[curr_pid]->start_priority < p_map[next.pid]->priority){
						cout<<"PID :: "<<curr_pid<<" TIME :: "<<next.end_t<<" EVENT :: Process preempted\n";
						it = p_map[curr_pid];
						it->phases[it->curr_phase].cpu_left = it->phases[it->curr_phase].cpu_b - (next.end_t - it->phases[it->curr_phase].started_time);
						ready_processes.push(p_map[curr_pid]);
						
						process *p = p_map[next.pid];
						cout<<"PID :: "<< p->pid<<" TIME :: "<<next.end_t<<" EVENT :: Process dispatched\n";
						curr_pid = p->pid;
						p->phases[p->curr_phase].mode = 0;
						p->phases[p->curr_phase].started_time = next.end_t;
						em.add_event(next.end_t+p->phases[p->curr_phase].cpu_left,3,p->pid);
						cpu_free = false;
					}
			}
			//routine for handling IO start or CPU end event
			else if(next.type=="IO start"){
				cout<<"PID :: "<< next.pid<<" TIME :: "<<next.end_t<<" EVENT :: CPU burst completed\n";
				cout<<"PID :: "<< next.pid<<" TIME :: "<<next.end_t<<" EVENT :: IO started\n";
				cpu_free = true;
				process *p = p_map[next.pid];
				process_phase *pp = &(p->phases[p->curr_phase]);
				em.add_event(next.end_t+pp->io_b,4,next.pid);
				if(!ready_processes.empty()){
					p = ready_processes.top();
					ready_processes.pop();
					cout<<"PID :: "<< p->pid<<" TIME :: "<<next.end_t<<" EVENT :: Process dispatched\n";
					cpu_free = false;
					curr_pid = p->pid;
					p->phases[p->curr_phase].mode = 0;
					p->phases[p->curr_phase].started_time = next.end_t;
					em.add_event(next.end_t+p->phases[p->curr_phase].cpu_b,3,p->pid);
				}
			}
			//routine for handling IO end event
			else if(next.type=="IO end"){
				cout<<"PID :: "<< next.pid<<" TIME :: "<<next.end_t<<" EVENT :: IO ended\n";
				process *p = p_map[next.pid];
				process_phase *pp = &(p->phases[p->curr_phase]);
				bool process_finished = false;
				pp->curr_itr++;
				pp->mode = 0;
				if(pp->curr_itr==pp->itrs){
					pp->curr_itr = 0;
					p->curr_phase++;
					if(p->curr_phase==p->phases.size()){
						cout<<"PID :: "<< next.pid<<" TIME :: "<<next.end_t<<" EVENT :: Process finished\n";
						process_finished = true;
					}
				}
				if(!process_finished){
					ready_processes.push(p_map[next.pid]);
					if(cpu_free){
						p = ready_processes.top();
						ready_processes.pop();
						cout<<"PID :: "<< p->pid<<" TIME :: "<<next.end_t<<" EVENT :: Process dispatched\n";
						curr->pid = p->pid;
						cpu_free = false;
						p->phases[p->curr_phase].mode = 0;
						em.add_event(next.end_t+p->phases[p->curr_phase].cpu_b,3,p->pid);
					}	
				}
			}
	}
	
	return 0;
}
