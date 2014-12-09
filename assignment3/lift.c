#include "lift.h"

/* Simple_OS include */ 
#include <pthread.h>

/* drawing module */ 
//#include "draw.h"

/* standard includes */ 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//#include "si_ui.h"

/* panic function, to be called when fatal errors occur */ 
static void lift_panic(const char message[])
{
    printf("LIFT_PANIC!!! "); 
    printf("%s", message); 
    printf("\n"); 
    exit(0); 
}

/* --- monitor data type for lift and operations for create and delete START --- */

/* lift_create: creates and initialises a variable of type lift_type */
lift_type lift_create(void) 
{
    /* the lift to be initialised */
    lift_type lift;

    /* floor counter */ 
    int floor; 

    /* loop counter */
    int i;

    /* allocate memory */
    lift = (lift_type) malloc(sizeof(lift_data_type));

    /* initialise variables */

    /* initialise floor */
    lift->floor = 0; 
    
    /* set direction of lift travel to up */
    lift->up = 1;

    /* the lift is not moving */ 
    lift->moving = 0; 

    /* initialise person information */
    for (floor = 0; floor < N_FLOORS; floor++)
    {
        for (i = 0; i < MAX_N_PERSONS; i++)
        {
            lift->persons_to_enter[floor][i].id = NO_ID; 
            lift->persons_to_enter[floor][i].to_floor = NO_FLOOR; 
        }
    }

    /* initialise passenger information */
    for (i = 0; i < MAX_N_PASSENGERS; i++) 
    {
        lift->passengers_in_lift[i].id = NO_ID; 
        lift->passengers_in_lift[i].to_floor = NO_FLOOR; 
    }

    /* initialise mutex and event variable */
    pthread_mutex_init(&lift->mutex,NULL);
    pthread_cond_init(&lift->change,NULL);

    return lift;
}

/* lift_delete: deallocates memory for lift */
void lift_delete(lift_type lift)
{
    free(lift);
}


/* --- monitor data type for lift and operations for create and delete END --- */


/* --- functions related to lift task START --- */

/* MONITOR function lift_next_floor: computes the floor to which the lift 
   shall travel. The parameter *change_direction indicates if the direction 
   shall be changed */
void lift_next_floor(lift_type lift, int *next_floor, int *change_direction)
{
  // Skulle gärna ha nån form av tidsstämpel på när de steg på / tröck på knappen.
  // Nu prioriterar de passagerare som står i hissen, och tar sen och åker till de som står utanför. 
  // Men skulle hissen passera en person och det finns rum i hissen får hen såklart stiga på!
  
  // Move to the nearest target level among the persons in the lift
  /*  int nearest = 100;
  int candidate = -1;
  for (i = 0; i < MAX_N_PASSENGERS; i++) 
    {
      if(lift->passengers_in_lift[i].to_floor != NO_FLOOR && 
	 ABS(lift->passengers_in_lift[i].to_floor - lift->floor) < nearest)
	  {
	    nearest = ABS(lift->passengers_in_lift[i].to_floor - lift->floor);
	    candidate = lift->passengers_in_lift[i].to_floor;
	  }
    }
  if(candidate == -1)
    {
      
    }
  
  */
  *next_floor = lift->floor;
  if(lift->up)
    (*next_floor)++;
  else
    (*next_floor)--;


  if((*next_floor) == 0 || (*next_floor) == N_FLOORS-1)
    *change_direction = 1;
  else
    *change_direction = 0;

  
}

void lift_move(lift_type lift, int next_floor, int change_direction)
{
    /* reserve lift */ 
    pthread_mutex_lock(&lift->mutex); 

    /* the lift is moving */ 
    lift->moving = 1; 
        
    /* release lift */ 
    pthread_mutex_unlock(&lift->mutex); 
        
    /* it takes two seconds to move to the next floor */ 
    //usleep(500000);
        
    /* reserve lift */ 
    pthread_mutex_lock(&lift->mutex); 
        
    /* the lift is not moving */ 
    lift->moving = 0; 

    /* the lift has arrived at next_floor */ 
    lift->floor = next_floor; 

    /* check if direction shall be changed */ 
    if (change_direction)
    {
        lift->up = !lift->up; 
    }
    
    /* draw, since a change has occurred */ 
    //draw_lift(lift); 
    
    /* release lift */ 
    pthread_mutex_unlock(&lift->mutex); 
    
}

/* this function is used also by the person tasks */ 
static int n_passengers_in_lift(lift_type lift)
{
    int n_passengers = 0; 
    int i; 
        
    for (i = 0; i < MAX_N_PASSENGERS; i++)
    {
        if (lift->passengers_in_lift[i].id != NO_ID)
        {
            n_passengers++; 
        }
    }
    return n_passengers; 
}

char lift_is_full(lift_type lift){
  return n_passengers_in_lift(lift) == MAX_N_PASSENGERS;
}


char everyone_has_jumped_off(lift_type lift){
  int i;
  for(i = 0; i < MAX_N_PASSENGERS; i++)
    if(lift->passengers_in_lift[i].to_floor == lift->floor)
      return 0;
  return 1;
}

char everyone_has_jumped_on(lift_type lift){  
  int i;
  if(lift_is_full(lift))
    return 1;

  for(i = 0; i < MAX_N_PERSONS; i++){
    if(lift->persons_to_enter[lift->floor][i].id != NO_ID)
      return 0;
  }
  
  return 1;
}

/* MONITOR function lift_has_arrived: shall be called by the lift task
   when the lift has arrived at the next floor. This function indicates
   to other tasks that the lift has arrived, and then waits until the lift
   shall move again. */
void lift_has_arrived(lift_type lift)
{
  //printf("Lift arrived\n");
  pthread_cond_broadcast(&lift->change);
  
  
  // Lock lift 
  pthread_mutex_lock(&lift->mutex);
  
  // Kontrollera så att alla hinner hoppa på och/eller av som vill göra det på denna våning
  while(!everyone_has_jumped_off(lift)){
    //printf("Wait for off\n");
    pthread_cond_wait(&lift->change, &lift->mutex);
  }  
  while(!everyone_has_jumped_on(lift)){
    //printf("Wait for on\n");
    pthread_cond_wait(&lift->change, &lift->mutex);
  }

  // release lift
  pthread_mutex_unlock(&lift->mutex);
}




/* --- functions related to lift task END --- */


/* --- functions related to person task START --- */

/* passenger_wait_for_lift: returns non-zero if the passenger shall
   wait for the lift, otherwise returns zero */
static int passenger_wait_for_lift(lift_type lift, int wait_floor)
{
  //  printf("Moving: %d\t floor: %d\t wait_floor: %d\t pass: %d\n", lift->moving,lift->floor,wait_floor,n_passengers_in_lift(lift));
  // if(!lift->moving){
  //    printf("#################################################################\n");
  //
  //  }
    int waiting_ready =
        /* the lift is not moving */ 
        !lift->moving && 
        /* and the lift is at wait_floor */ 
        lift->floor == wait_floor && 
        /* and the lift is not full */ 
        n_passengers_in_lift(lift) < MAX_N_PASSENGERS; 
    return !waiting_ready;
}

/* passenger_wait_for_target: returns non-zero if the passenger shall
   wait for the exit floor, otherwise returns zero */
static int passenger_wait_for_exit(lift_type lift, int target_floor)
{
    int waiting_ready =
      /* the lift is not moving */ 
      !lift->moving && 
      /* and the lift is at wait_floor */ 
      lift->floor == target_floor;
    return !waiting_ready;

}




/* enter_floor: makes a person with id id stand at floor floor */ 
static void enter_floor(
    lift_type lift, int id, int floor)
{
    int i; 
    int floor_index; 
    int found; 

    /* stand at floor */ 
    found = 0; 
    for (i = 0; i < MAX_N_PERSONS && !found; i++)
    {
        if (lift->persons_to_enter[floor][i].id == NO_ID)
        {
            found = 1; 
            floor_index = i; 
        }
    }
        
    if (!found)
    {
        lift_panic("cannot enter floor"); 
    }

    /* enter floor at index floor_index */ 
    lift->persons_to_enter[floor][floor_index].id = id;
    lift->persons_to_enter[floor][floor_index].to_floor = NO_FLOOR; 
}

/* leave_floor: makes a person with id id at enter_floor leave 
   enter_floor */ 
static void leave_floor(lift_type lift, int id, int enter_floor)

/* fig_end lift_c_prot */ 
{
  int i; 
  int floor_index; 
  int found; 

  /* leave the floor */
  found = 0; 
  for (i = 0; i < MAX_N_PERSONS && !found; i++)
    {
      if (lift->persons_to_enter[enter_floor][i].id == id)
        {
	  found = 1; 
	  floor_index = i; 
        }
    }
        
  if (!found)
    {
      lift_panic("cannot leave floor"); 
    }

  /* leave floor at index floor_index */ 
  lift->persons_to_enter[enter_floor][floor_index].id = NO_ID; 
  lift->persons_to_enter[enter_floor][floor_index].to_floor = NO_FLOOR; 
}





/* leave_lift: makes a person with id id at lift_index leave the lift */ 
static void leave_lift(lift_type lift, int id, int lift_index)
{
      lift->passengers_in_lift[lift_index].id = NO_ID;
      lift->passengers_in_lift[lift_index].to_floor = NO_FLOOR;  
}

// Returns the index of the person if the person can jump on the lift (ie. the lift had space left), otherwise -1
static char enter_lift(lift_type lift, int id, int to_floor){
  int i;

  // NOTE: DONT COUNT PERSONS THAT SHOULD GO OFF AT THIS FLOOR!!
  for(i = 0; i < MAX_N_PASSENGERS; i++){
    if(lift->passengers_in_lift[i].id == NO_ID){
      lift->passengers_in_lift[i].id = id;
      lift->passengers_in_lift[i].to_floor = to_floor;
      return i;
    }
  }
  return -1;
}


/* MONITOR function lift_travel: performs a journey with the lift
   starting at from_floor, and ending at to_floor */ 
void lift_travel(lift_type lift, int id, int from_floor, int to_floor)
{

  pthread_mutex_lock(&lift->mutex);
  
  
  // Create the person at the enter_floor
  enter_floor(lift, id, from_floor);
  
  // Waits until the lift is at the from_floor
  //conditional_wait(passenger_wait_for_lift(lift, from_floor));
  while(passenger_wait_for_lift(lift, from_floor)){
    //printf("%d: Wait for lift to jump on %d\n",id,from_floor);
    //draw_lift(lift); 
    pthread_cond_wait(&lift->change, &lift->mutex);
  }
  
  // Jumps on the lift
  int index = enter_lift(lift, id, to_floor);
  if (index != -1){
    //printf("%d hoppa pa hiss!\n",id);
    leave_floor(lift, id, from_floor);
  }
  pthread_cond_broadcast(&lift->change);
  
  // Waits until the lift is at the to_floor
  //conditional_wait(passenger_wait_for_exit(lift, to_floor));
  while(passenger_wait_for_exit(lift, to_floor)){
    //printf("%d: Wait for floor to jump off at %d\n",id,to_floor);
    pthread_cond_wait(&lift->change, &lift->mutex);
  }
  //printf("%d hoppa av hiss!\n",id);
  leave_lift(lift,id,index);
  pthread_cond_broadcast(&lift->change);
  

  pthread_mutex_unlock(&lift->mutex); 

}

/* --- functions related to person task END --- */
