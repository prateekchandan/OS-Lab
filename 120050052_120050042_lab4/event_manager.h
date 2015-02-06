#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <map>
using namespace std;

//// Structs relevant to Scheduler and processes ////////////////////////

struct scheduling_level {
	int level_no;
	int priority;
	int time_slice;	//either some integer or N
};

struct scheduler {
	int no_of_levels;
	vector<scheduling_level> levels_arr;
};

struct process_phase {
	int itrs;	//number of iterations
	int cpu_b;	//cpu burst time
	int io_b;	//IO burst time
	
	// These variables are for maintaining state of a process phase
	int curr_itr;
	int mode;
	int cpu_left;
	int started_time;
	
	process_phase(){
		curr_itr = 0;
		mode = 0; // 0 = CPU burst, 1 = IO burst
	}
};

struct process {
	int pid;
	int start_priority;
	int admission;
	vector<process_phase> phases;
	
	// This variable is held for maintaining process state
	int state; // 0 = ready, 1 = running, 2 = blocked, 3 = terminated
	int curr_phase;
	
	process(){
		curr_phase = 0;
	}
	
};

scheduler Scheduler;
vector<process> p_list;
map<int,process*> p_map;

//////  Structs relevant to events ///////////////////////////////

struct event{
	int end_t;		//event occurrence time 
	string type;	//event type
	int pid;		//process id
};

class comp{
public:
 	int operator() ( const event& p1, const event &p2)
 	{
 		if(p1.end_t>p2.end_t) return true;
 		if(p1.end_t<p2.end_t) return false;
 		if(p_map.find(p1.pid)==p_map.end() || p_map.find(p2.pid)==p_map.end()){
			cout<<p1.pid<<" "<<p2.pid<<endl;
			return false;
		}
 		return p_map[p1.pid]->start_priority < p_map[p2.pid]->start_priority;
 	}
};

class event_mgnr {
	public:
		priority_queue<event, vector<event>, comp> event_table;

	//function for adding an event in the event table
	void add_event(int end_t, int type, int pid)
	{
		event ev;
		ev.end_t = end_t;
		switch (type) {
			case 1:
				ev.type = "Process admission";
				break;
			case 2:
				ev.type = "End of time-slice";
				break;
			case 3:
				ev.type = "IO start";
				break;
			case 4:
				ev.type = "IO end";
				break;
			case 5:
				ev.type = "Process pre-empted";
				break;
			default:
				break;
		}
		
		ev.pid = pid;
		event_table.push(ev);
	}

	//Is event table empty..?
	bool is_empty()
	{
		return event_table.empty();
	}
	
	//function for returning the top_most entry of the event table
	event next_event()
	{
		event ev = event_table.top();
		event_table.pop();
		return ev;
	}

};

event_mgnr em;

