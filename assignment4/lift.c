#include "lift.h"


/* standard includes */ 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


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
    int i,j;

    
    /* allocate memory */
    lift = (lift_type) malloc(sizeof(lift_data_type));

    /* initialise variables */

    /* initialise floor */
    lift->floor = 0; 
    
    /* set direction of lift travel to up */
    lift->up = 1;

    /* the lift is not moving */ 
    //lift->moving = 0; 

    /* initialise person information */
    for (floor = 0; floor < N_FLOORS; floor++)
    {
        for (i = 0; i < MAX_N_PERSONS; i++)
        {
            lift->persons_to_enter[floor][i].id = NO_ID; 
            lift->persons_to_enter[floor][i].trips = 0; 
	    for(j = 0; j < N_DEST; j++){
	      lift->persons_to_enter[floor][i].to_floor[j] = NO_FLOOR; 
	      lift->persons_to_enter[floor][i].from_floor[j] = NO_FLOOR; 
	    }
        }
    }

    /* initialise passenger information */
    for (i = 0; i < MAX_N_PASSENGERS; i++) 
    {
        lift->passengers_in_lift[i].id = NO_ID; 
        lift->passengers_in_lift[i].trips = 0; 
	for(j = 0; j < N_DEST; j++){
	  lift->passengers_in_lift[i].to_floor[j] = NO_FLOOR; 
	  lift->passengers_in_lift[i].from_floor[j] = NO_FLOOR; 
	}
    }


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
    /* the lift has arrived at next_floor */ 
    lift->floor = next_floor; 

    /* check if direction shall be changed */ 
    if (change_direction)
    {
        lift->up = !lift->up; 
    }
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

int next_passenger_to_leave(lift_type lift, int floor){
  int i;
  for (i = 0; i < MAX_N_PASSENGERS; i++){
    //printf("Person %d trip: %d\n",lift->passengers_in_lift[i].id,lift->passengers_in_lift[i].trips);
    if (lift->passengers_in_lift[i].id != NO_ID && lift->passengers_in_lift[i].to_floor[lift->passengers_in_lift[i].trips] == floor){
      return i;
    }
  }
  return -1;
}
/* next_passenger_to_enter: returns the index of the next passenger at this floor, 
   Returns -1 if no one wants/is allowed to enter the lift here */
int next_passenger_to_enter(lift_type lift, int floor){
  if(lift_is_full(lift))
    return -1;

  int i;
  for (i = 0; i < MAX_N_PERSONS; i++){
    if (lift->persons_to_enter[floor][i].id != NO_ID){
      return i;//lift->persons_to_enter[floor][i].id;
    }
  }
  return -1;
}



char lift_is_full(lift_type lift){
  return n_passengers_in_lift(lift) == MAX_N_PASSENGERS;
}


char everyone_has_jumped_off(lift_type lift){
  int i;
  for(i = 0; i < MAX_N_PASSENGERS; i++)
    if(lift->passengers_in_lift[i].to_floor[lift->passengers_in_lift[i].trips] == lift->floor)
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

  
  // Kontrollera så att alla hinner hoppa på och/eller av som vill göra det på denna våning
  while(!everyone_has_jumped_off(lift)){
    //printf("Wait for off\n");
  }  
  while(!everyone_has_jumped_on(lift)){
    //printf("Wait for on\n");
  }

}




/* --- functions related to lift task END --- */


/* --- functions related to person task START --- */

/* enter_floor: makes a person with id id stand at floor floor */ 
void enter_floor(lift_type lift, unsigned char id, unsigned char from_floor[N_DEST], unsigned char to_floor[N_DEST], unsigned char trip)
{
    int i; 
    int floor_index = -1; 
    int found; 

    /* stand at floor */ 
    found = 0; 
    for (i = 0; i < MAX_N_PERSONS && !found; i++)
    {
        if (lift->persons_to_enter[from_floor[trip]][i].id == NO_ID)
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
    lift->persons_to_enter[from_floor[trip]][floor_index].id = id;
    lift->persons_to_enter[from_floor[trip]][floor_index].trips = trip;
    memcpy(lift->persons_to_enter[from_floor[trip]][floor_index].to_floor, to_floor, N_DEST); 
    memcpy(lift->persons_to_enter[from_floor[trip]][floor_index].from_floor, from_floor, N_DEST); 
}

void setup_person(unsigned char id, unsigned char floor[], unsigned char to_floor[], unsigned char trip){
  return;
}



/* leave_floor: makes a person with id id at enter_floor leave 
   enter_floor */ 
void leave_floor(lift_type lift, int id, int enter_floor/*, int *to_floor*/)

/* fig_end lift_c_prot */ 
{
  int i; 
  int floor_index = -1; 
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
  //lift->persons_to_enter[enter_floor][floor_index].to_floor = NO_FLOOR; 
}





/* leave_lift: makes a person with id id at lift_index leave the lift */ 
void leave_lift(lift_type lift, int lift_index)
{
      lift->passengers_in_lift[lift_index].id = NO_ID;
      //lift->passengers_in_lift[lift_index].to_floor = NO_FLOOR;  
}

// Returns the index of the person if the person can jump on the lift (ie. the lift had space left), otherwise -1
char enter_lift(lift_type lift, person_data_type passenger){
  int i;

  // NOTE: DONT COUNT PERSONS THAT SHOULD GO OFF AT THIS FLOOR!!
  for(i = 0; i < MAX_N_PASSENGERS; i++){
    if(lift->passengers_in_lift[i].id == NO_ID){
      lift->passengers_in_lift[i] = passenger;
      return i;
    }
  }
  return -1;
}
