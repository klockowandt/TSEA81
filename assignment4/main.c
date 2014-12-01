#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
//#include <iostream.h>
//#include <conio.h>
#include <termios.h>
#include "lift.h"
#include "si_ui.h"
#include "messages.h"

#include "draw.h"

#define PORT_UI 0
#define PORT_LIFT 1
#define PORT_FIRSTPERSON 10
#define N_ITERATIONS 100



// These variables keeps track of the process IDs of all processes
// involved in the application so that they can be killed when the
// exit command is received.
static pid_t lift_pid;
static pid_t uidraw_pid;
static pid_t liftmove_pid;
static pid_t person_pid[MAX_N_PERSONS];

typedef enum {LIFT_TRAVEL, // A travel message is sent to the list process when a person would
	                   // like to make a lift travel
	      LIFT_TRAVEL_DONE, // A travel done message is sent to a person process when a
	                        // lift travel is finished
	      LIFT_MOVE         // A move message is sent to the lift task when the lift shall move
	                        // to the next floor
} lift_msg_type; 

struct lift_msg{
	lift_msg_type type;  // Type of message
	int person_id;       // Specifies the person
	int from_floor;      // Specify source and destion for the LIFT_TRAVEL message.
	int to_floor;
};


// Since we use processes now and not 
static int get_random_value(int person_id, int maximum_value)
{
	return rand() % (maximum_value + 1);
}


// Initialize the random seeds used by the get_random_value() function
// above.
static void init_random(void)
{
	srand(getpid()); // The pid should be a good enough initialization for
                         // this case at least.
}


static void liftmove_process(void)
{
	struct lift_msg* m = malloc(sizeof(struct lift_msg));
	while(1){
		// TODO:
		//    Sleep 2 seconds
                //    Send a message to the lift process to move the lift.
	  sleep(1);
	  m->type = LIFT_MOVE;
	  int size = sizeof(*m); // Ska vara en * h�r!
	  //printf("size1: %lu, size2: %lu, hela:%lu\n", sizeof(m), sizeof(*m), sizeof(struct lift_msg));
	  message_send((char *)m, size, PORT_LIFT, 0);
	}
}


static void lift_process(void)
{
        lift_type Lift;
	Lift = lift_create();
	int change_direction, next_floor;
	
	char msgbuf[4096];
	while(1){
		int i;
		struct lift_msg reply;
		struct lift_msg *m;
		message_send((char *) Lift, sizeof(*Lift), PORT_UI,0); // Draw the lift
		int len = message_receive(msgbuf, 4096, PORT_LIFT); // Wait for a message
		if(len < sizeof(struct lift_msg)){
			fprintf(stderr, "Message too short\n");
			continue;
		}
		
		m = (struct lift_msg *) msgbuf;
		switch(m->type){
		case LIFT_MOVE:
		  // TODO: 
		  //    Check if passengers want to leave elevator
		  //        Remove the passenger from the elevator
		  //        Send a LIFT_TRAVEL_DONE for each passenger that leaves
		  //        the elevator
		  //    Check if passengers want to enter elevator
		  //        Remove the passenger from the floor and into the elevator
		  //    Move the lift
		  ;

		  int passenger_id;
		  int passenger_index = -1;

		  while((passenger_index = next_passenger_to_leave(Lift,Lift->floor)) != -1){
		    passenger_id = Lift->passengers_in_lift[passenger_index].id;
		    printf("%d hoppa av hiss!\n",passenger_id);
		    leave_lift(Lift,passenger_id, passenger_index);
		    
		    m->type = LIFT_TRAVEL_DONE;
		    m->person_id = passenger_id;
		    message_send((char*)m, sizeof(*m), PORT_FIRSTPERSON + passenger_id, 0);
		  }
		  
		  while((passenger_index = next_passenger_to_enter(Lift,Lift->floor)) != -1){
		    passenger_id = Lift->persons_to_enter[Lift->floor][passenger_index].id;
		    int to_floor = Lift->persons_to_enter[Lift->floor][passenger_index].to_floor;
		    
		    // Jumps on the lift
		    printf("%d hoppa pa hiss!\n",passenger_id);
		    enter_lift(Lift, passenger_id, to_floor);
		    leave_floor(Lift, passenger_id, Lift->floor);
		    
		    //m->type = LIFT_TRAVEL;
		    //m->person_id = passenger_id;
		    //message_send((char*)m, sizeof(*m), PORT_FIRSTPERSON + passenger_id, 0);
		  }
		  
		  // TODO: Move lift
		  int next_floor, change_direction;
		  lift_next_floor(Lift, &next_floor, &change_direction);
		  lift_move(Lift, next_floor, change_direction);

		  break;
		case LIFT_TRAVEL:
		  // TODO:
		  //    Update the Lift structure so that the person with the given ID  is now present on the floor
		  // Cxoreate the person at the enter_floor
		  printf("Fixar in gubbe p� v�ning\n");
		  enter_floor(Lift, m->person_id, m->from_floor, m->to_floor);
		  
		  break;
		case LIFT_TRAVEL_DONE:
		  break;
		}
	}
	return/* NULL*/;
}

static void person_process(int id)
{
	init_random();
	char buf[4096];
	struct lift_msg* m = malloc(sizeof(struct lift_msg));
	int to, from;
	int i; 
	for(i = 0; i < N_ITERATIONS; i++){
	  // TODO:
	  //    Generate a to and from floor
	  //    Send a LIFT_TRAVEL message to the lift process
	  //    Wait for a LIFT_TRAVEL_DONE message
	  //    Wait a little while
	  // Select random floors
	  to = get_random_value(id, N_FLOORS-1);
	  while((from = get_random_value(id, N_FLOORS-1)) == to);


	  // * Travel between these floors
	  m->type = LIFT_TRAVEL;
	  m->person_id = id;
	  m->from_floor = from;
	  m->to_floor = to;
	  printf("Skickar LIFT_TRAVEL\n");
	  message_send((char*)m,sizeof(*m),PORT_LIFT,0);
	  
	  
	  // Wait for a message
	  int len;
	  
	  do{
	    len = message_receive(buf, 4096, PORT_FIRSTPERSON+id);
	    m = (struct lift_msg *) buf;
	    printf("ID: %d, from: %d, to: %d, Type: %d\n",m->person_id, m->from_floor, m->to_floor, m->type);
	  }while(m->person_id != id);
	  printf("�kt klart!\n");
	  // * Wait a little while
	  /* sleep(2); */
	  //printf("Tillbaka i byggnad\n");
	}
}

// This is the final process called by main()
// It is responsible for:
//   * Receiving and executing commands from the java GUI
//   * Killing off all processes when exiting the application
/*
void uicommand_process(void)
{
	int i;
	int current_person_id = 0;
	char message[SI_UI_MAX_MESSAGE_SIZE]; 
	while(1){
		// Read a message from the GUI
		si_ui_receive(message);
		if(!strcmp(message, "new")){
			// TODO:
			// * Check that we don't create too many persons
			// * fork and create a new person process (and
			//   record the new pid in person_pid[])
		  if(current_person_id < MAX_N_PERSONS){
		    // Create a new person
		    
		    pid_t p_pid = fork();
		    if(!p_pid) {
		      //printf("################Skapar person process\n");
		      person_process(current_person_id);
		    }
		    person_pid[current_person_id] = p_pid;
		    current_person_id++;
                  }else{
                    si_ui_show_error("Can't create more persons at the moment");
                  }

		  
		}else if(!strcmp(message, "exit")){
			// The code below sends the SIGINT signal to
			// all processes involved in this application
			// except for the uicommand process itself
			// (which is exited by calling exit())
			kill(uidraw_pid, SIGINT);
			kill(lift_pid, SIGINT);
			kill(liftmove_pid, SIGINT);
			for(i=0; i < MAX_N_PERSONS; i++){
				if(person_pid[i] > 0){
					kill(person_pid[i], SIGINT);
				}
			}
			exit(0);
		}
	}
}
*/
// This process is responsible for drawing the lift. Receives lift_type structures
// as messages.
void uidraw_process(void)
{
	char msg[1024];
	si_ui_set_size(670, 700); 
	while(1){
		message_receive(msg, 1024, PORT_UI);
		lift_type Lift = (lift_type) &msg[0];
		draw_lift(Lift);
	}
}

int main(int argc, char **argv)
{
	message_init();
        si_ui_init(); // Initialize user interface. (Must be done
		      // here!)

	lift_pid = fork();
	if(!lift_pid) {
	  lift_process(/*NULL*/);
	}
	uidraw_pid = fork();
	if(!uidraw_pid){
		uidraw_process();
	}
	liftmove_pid = fork();
	if(!liftmove_pid){
		liftmove_process();
	}
	//uicommand_process();
	
	int i;
	for(i = 0; i < MAX_N_PERSONS; i++){
	  pid_t p_pid = fork();
	  if(!p_pid) {
	    printf("################Skapar person process\n");
	    person_process(i);
	  }
	  person_pid[i] = p_pid;
	}
	
	
	
	
	//while (getchar() != '\n');
	getchar();	
	// The code below sends the SIGINT signal to
	// all processes involved in this application
	// except for the uicommand process itself
	// (which is exited by calling exit())
	kill(uidraw_pid, SIGINT);
	kill(lift_pid, SIGINT);
	kill(liftmove_pid, SIGINT);
	for(i=0; i < MAX_N_PERSONS; i++){
	  if(person_pid[i] > 0){
	    kill(person_pid[i], SIGINT);
	  }
	}
	//exit(0);
	
	
	return 0;
}
