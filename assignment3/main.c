#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>
#include "lift.h"
#include "debug.h"
//#include "si_ui.h"

#define N_ITERATIONS 10000



// Unfortunately the rand() function is not thread-safe. However, the
// rand_r() function is thread-safe, but need a pointer to an int to
// store the current state of the pseudo-random generator.  This
// pointer needs to be unique for every thread that might call
// rand_r() simultaneously. The functions below are wrappers around
// rand_r() which should work in the environment encountered in
// assignment 3.
//

static unsigned int rand_r_state[MAX_N_PERSONS];
// Get a random value between 0 and maximum_value. The passenger_id
// parameter is used to ensure that the rand_r() function is called in
// a thread-safe manner.

static int get_random_value(int passenger_id, int maximum_value)
{
	return rand_r(&rand_r_state[passenger_id]) % (maximum_value + 1);
}

static lift_type Lift;

/* semaphore for mutual exclusion */ 
static sem_t mutex; 

// Initialize the random seeds used by the get_random_value() function
// above.
static void init_random(void)
{
	int i;
	for(i=0; i < MAX_N_PERSONS; i++){
		// Use this statement if the same random number sequence
		// shall be used for every execution of the program.
		rand_r_state[i] = i;

		// Use this statement if the random number sequence
		// shall differ between different runs of the
		// program. (As the current time (as measured in
		// seconds) is used as a seed.)
		//rand_r_state[i] = i+time(NULL);
	}
}



static void *lift_thread(void *unused)
{
  //printf("Started lift_thread\n");
  //The functions lift_next_floor, lift_move and lift_has_arrived shall be called by the lift thread
	while(1){
	  int next_floor, change_direction;
	  lift_next_floor(Lift, &next_floor, &change_direction);
	  lift_move(Lift, next_floor, change_direction);
	  lift_has_arrived(Lift);
	  // Move lift one floor
	}
	return NULL;
}

static void *passenger_thread(void *idptr)
{
  //printf("Person skapad\n");
  // Code that reads the passenger ID from the idptr pointer
	// (due to the way pthread_create works we need to first cast
	// the void pointer to an int pointer).
	int *tmp = (int *) idptr;
	int id = *tmp;
	sem_post(&mutex);


	struct timeval starttime;
	struct timeval endtime;
	long long int timediff;
	gettimeofday(&starttime, NULL);
	
	int to, from;
	int i;
	for(i = 0; i < N_ITERATIONS; i++){
	  // * Select random floors
	  to = get_random_value(id, N_FLOORS-1);
	  while((from = get_random_value(id, N_FLOORS-1)) == to);
	  // * Travel between these floors
	  lift_travel(Lift, id, from, to);
	  // * Wait a little while
	  //sleep(1);
	}
	
	gettimeofday(&endtime, NULL);
	timediff = (endtime.tv_sec*1000000ULL + endtime.tv_usec) - (starttime.tv_sec*1000000ULL + starttime.tv_usec);
	printf("%d: %lld\n",id,timediff);
	
	return NULL;
}
/*
static void *user_thread(void *unused)
{
	int current_passenger_id = 0;
	char message[SI_UI_MAX_MESSAGE_SIZE]; 
	
	
	si_ui_set_size(670, 700); 
	
	while(1){
		// Read a message from the GUI
		si_ui_receive(message);
		if(!strcmp(message, "new")){
		  // create a new passenger if possible, else
		  // use si_ui_show_error() to show an error
		  // message if too many passengers have been
		  // created. Make sure that each passenger gets
		  // a unique ID between 0 and MAX_N_PERSONS-1.
		  if(current_passenger_id < MAX_N_PERSONS){
		    pthread_t handle;
		    // Create a new person
		    pthread_create(&handle, NULL, passenger_thread, (void*) &current_passenger_id);


		    sem_wait(&mutex);
		    pthread_detach(handle); // Ensure resources are reclaimed appropriately
		    current_passenger_id++;
		  }else{
		    si_ui_show_error("Can't create more persons at the moment");
		  }
		  
		}else if(!strcmp(message, "exit")){
			lift_delete(Lift);
			exit(0);
		}
	}
	return NULL;
}
*/

int main(int argc, char **argv)
{
  
  //si_ui_init();
	debug_init();
	init_random();
	Lift = lift_create();
	sem_init(&mutex,0,0);
        // Create tasks as appropriate here
	/* create tasks */ 
	//pthread_t user_thread_handle;
	pthread_t lift_thread_handle;
	//pthread_create(&user_thread_handle, NULL, user_thread, 0);
	pthread_create(&lift_thread_handle, NULL, lift_thread, 0);

		
	int i;
	for(i = 0; i < MAX_N_PERSONS; i++){
	  pthread_t handle;
	  pthread_create(&handle, NULL, passenger_thread, (void*) &i);
	  
	  sem_wait(&mutex);
	  
	  // Set the real-time priority of the current thread to 5
	  // If you want to set the priority of another thread, specify the
	  // appropriate pthread_t instead of the pthread_self() function.
	  struct sched_param p;
	  if(i == 2){
	    p.sched_priority = 10;
	  }else{
	    p.sched_priority = 1;
	  }
	  if(pthread_setschedparam(handle, SCHED_RR, &p) != 0){
	    perror("Could not set the thread priority");
	  }
	  
	  
	  pthread_detach(handle); // Ensure resources are reclaimed appropriately
	}
	
	
	pthread_join(lift_thread_handle, NULL);
	//pthread_join(user_thread_handle, NULL);
	
	
	return 0;
}
