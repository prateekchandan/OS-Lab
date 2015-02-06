#include <iostream>
#include <vector>
#include <queue>

using namespace std;

struct event{
	int end_t;		//event occurrence time 
	string type;	//event type
	int pid;		//process id
};

class comp{
public:
 	int operator() ( const event& p1, const event &p2)
 	{
 		return p1.end_t>p2.end_t;
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


