#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <termios.h>
#include "lift.h"
#include "messages.h"


#define PORT_UI 0
#define PORT_LIFT 1
#define PORT_FIRSTPERSON 10
#define N_ITERATIONS 100
//#define N_DESTINATIONS 256
#define LENGTH 1024

// These variables keeps track of the process IDs of all processes
// involved in the application so that they can be killed when the
// exit command is received.
static pid_t lift_pid;
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
	char from_floor;      // Specify source and destion for the LIFT_TRAVEL message.
	char to_floor;
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
	  //    Send a message to the lift process to move the lift.
	  m->type = LIFT_MOVE;
	  int size = sizeof(*m); // Ska vara en * här!
	  //printf("size1: %lu, size2: %lu, hela:%lu\n", sizeof(m), sizeof(*m), sizeof(struct lift_msg));
	  message_send((char *)m, size, PORT_LIFT, 0);
	}
}


static void lift_process(void)
{
        lift_type Lift;
	Lift = lift_create();
	char msgbuf[LENGTH];
	while(1){
		struct lift_msg *m;

		int len = message_receive(msgbuf, LENGTH, PORT_LIFT); // Wait for a message
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
		    //printf("%d hoppa av hiss!\n",passenger_id);
		    leave_lift(Lift,passenger_id, passenger_index);
		    
		    m->type = LIFT_TRAVEL_DONE;
		    m->person_id = passenger_id;
		    message_send((char*)m, sizeof(*m), PORT_FIRSTPERSON + passenger_id, 0);

		  }
		  
		  while((passenger_index = next_passenger_to_enter(Lift,Lift->floor)) != -1){
		    passenger_id = Lift->persons_to_enter[Lift->floor][passenger_index].id;
		    int to_floor = Lift->persons_to_enter[Lift->floor][passenger_index].to_floor;
		    
		    // Jumps on the lift
		    //printf("%d hoppa pa hiss!\n",passenger_id);
		    enter_lift(Lift, passenger_id, to_floor);
		    leave_floor(Lift, passenger_id, Lift->floor);
		    
		    //m->type = LIFT_TRAVEL;
		    //m->person_id = passenger_id;
		    //message_send((char*)m, sizeof(*m), PORT_FIRSTPERSON + passenger_id, 0);
		  }
		  
		  
		  int next_floor, change_direction;
		  lift_next_floor(Lift, &next_floor, &change_direction);
		  //printf("Floor: %d, Direction: %d",next_floor, change_direction);
		  lift_move(Lift, next_floor, change_direction);

		  break;
		case LIFT_TRAVEL:
		  // TODO:
		  //    Update the Lift structure so that the person with the given ID  is now present on the floor
		  // Cxoreate the person at the enter_floor
		  //printf("Fixar in gubbe på våning\n");
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
	char buf[LENGTH];
	struct lift_msg* m = malloc(sizeof(struct lift_msg));
	char to;//[N_DESTINATIONS];
	char from;//[N_DESTINATIONS];
	int i; 


	struct timeval starttime;
	struct timeval endtime;
	long long int timediff;
	gettimeofday(&starttime, NULL);
	for(i = 0; i < N_ITERATIONS; i++){
	  // TODO:
	  //    Generate a to and from floor
	  //    Send a LIFT_TRAVEL message to the lift process
	  //    Wait for a LIFT_TRAVEL_DONE message
	  //    Wait a little while
	  // Select random floors
	  //for(j = 0; j < N_DESTINATIONS; j++){
	  //  to[j] = get_random_value(id, N_FLOORS-1);
	  //  while((from[j] = get_random_value(id, N_FLOORS-1)) == to[j]);
	  //}


	  to = get_random_value(id, N_FLOORS-1);
	  while((from = get_random_value(id, N_FLOORS-1)) == to);
	  // * Travel between these floors
	  m->type = LIFT_TRAVEL;
	  m->person_id = id;
	  m->from_floor = from;
	  m->to_floor = to;
	  //printf("Skickar LIFT_TRAVEL\n");
	  message_send((char*)m,sizeof(*m),PORT_LIFT,0);
	  
	  //If future destinations of a person is added, check the size of m (shouldn't exceed 1024)
	  /*size = sizeof(*m);
	  if (size > 1024)
	    printf("size of m is to big");
	  else 
	    message_send((char*)m,sizeof(*m),PORT_LIFT,0);
	  */
	  
	  // Wait for a message
	  message_receive(buf, LENGTH, PORT_FIRSTPERSON+id);

	}

	gettimeofday(&endtime, NULL);
	timediff = (endtime.tv_sec*1000000ULL + endtime.tv_usec) - (starttime.tv_sec*1000000ULL + starttime.tv_usec);
	printf("Person %d took %lldms\n", id, timediff);


}

int main(int argc, char **argv)
{
	message_init();

	lift_pid = fork();
	if(!lift_pid) {
	  lift_process(/*NULL*/);
	}
	liftmove_pid = fork();
	if(!liftmove_pid){
		liftmove_process();
	}
	
	int i;
	for(i = 0; i < MAX_N_PERSONS; i++){
	  pid_t p_pid = fork();
	  if(!p_pid) {
	    //printf("################Skapar person process %d\n",i);
	    person_process(i);
	    exit(0);
	  }
	  person_pid[i] = p_pid;
	}
	
	
	
	//while (getchar() != '\n');
	getchar();	
	// The code below sends the SIGINT signal to
	// all processes involved in this application
	// except for the uicommand process itself
	// (which is exited by calling exit())
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
