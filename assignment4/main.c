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
#define N_ITERATIONS 10000/N_DEST
#define LENGTH 4096
//static int counter;
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
  char person_id;       // Specifies the person
  unsigned char from_floor[N_DEST];      // Specify source and destion for the LIFT_TRAVEL message.
  unsigned char to_floor[N_DEST];
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
	  int size = sizeof(*m); // Ska vara en * h�r!
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
		  person_data_type passenger;
		  int passenger_index = -1;

		  while((passenger_index = next_passenger_to_leave(Lift, Lift->floor)) != -1){
		    passenger = Lift->passengers_in_lift[passenger_index];
		    
		    //printf("%d hoppa AV hiss at %d!\n",passenger.id, Lift->floor);

		    leave_lift(Lift, passenger_index);
		    //		    counter++;
		    // Om vi rest alla planerade rutter sa ska personen fa ta over och gora nya beslut
		    if(passenger.trips <= 0){
		      //printf("%d: Travel done??\n", passenger.id);
		      //printf("%d: %d\n",passenger.id,counter);
		      m->type = LIFT_TRAVEL_DONE;
		      message_send((char*)m, sizeof(*m), PORT_FIRSTPERSON + passenger.id, 0);
		    }else{
		      passenger.trips--;
		      //printf("Person %d enters floor %d for trip %d\n",passenger.id, passenger.from_floor[passenger.trips], passenger.trips);
		      enter_floor(Lift,passenger.id, passenger.from_floor,passenger.to_floor, passenger.trips);
		    }
		    
		    
		  }
		  
		  while((passenger_index = next_passenger_to_enter(Lift,Lift->floor)) != -1){
		    passenger = Lift->persons_to_enter[Lift->floor][passenger_index];
		    
		    // Jumps on the lift
		    //printf("%d hoppa PA hiss at %d!\n", passenger.id, Lift->floor);
		    enter_lift(Lift, passenger);
		    leave_floor(Lift, passenger.id, Lift->floor);
		    
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
		  // Create the person at the enter_floor
		  //printf("Create person %d with trip %d\n", m->person_id, N_DEST);
		  //printf("##Person %d enters floor %d for trip %d\n",m->person_id, m->from_floor[N_DEST-1], N_DEST-1);
		  enter_floor(Lift, m->person_id, m->from_floor, m->to_floor, N_DEST-1);
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

	  m->type = LIFT_TRAVEL;
	  m->person_id = id;
	  int i;
	  //printf("Person %d: ",id);
	  for(i = 0; i < N_DEST; i++){
	    to = get_random_value(id, N_FLOORS-1);
	    while((from = get_random_value(id, N_FLOORS-1)) == to);
	    // * Travel between these floors
	    //printf("%d=>%d\t",from,to);
	    m->from_floor[i] = from;
	    m->to_floor[i] = to;
	  }//printf("\n");
	  
	  //printf("%d: Skickar LIFT_TRAVEL\n",id);
	  message_send((char*)m,sizeof(*m),PORT_LIFT,0);
	  	  
	  // Wait for a message
	  message_receive(buf, LENGTH, PORT_FIRSTPERSON+id);
	  
	  
	  
	}

	gettimeofday(&endtime, NULL);
	timediff = (endtime.tv_sec*1000000ULL + endtime.tv_usec) - (starttime.tv_sec*1000000ULL + starttime.tv_usec);
	//printf("Person %d took %lldms\n", id, timediff);
	printf("%lld\n", timediff);


}

int main(int argc, char **argv)
{
  //counter=0;
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
