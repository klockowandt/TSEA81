#ifndef LIFT_H
#define LIFT_H

#include <pthread.h>

/* fig_begin lift_h_defs */ 
/* size of building */ 
#define N_FLOORS 5

/* maximum number of persons in the lift system */ 
#define MAX_N_PERSONS 10

/* maximum number of passengers in lift */ 
#define MAX_N_PASSENGERS 5
/* fig_end lift_h_defs */ 

/* fig_begin person_data_type */ 
/* data structure for person information */ 
typedef struct
{
    /* identity */ 
    int id; 
    /* destination floor */ 
    int to_floor; 
} person_data_type; 
/* fig_end person_data_type */ 

/* special numbers, to define no identity and no destination */ 
#define NO_ID -1
#define NO_FLOOR -1

/* fig_begin lift_mon_type */ 
/* definition of monitor data type for lift */

typedef struct
{
    /* the floor where the lift is positioned */ 
    int floor; 

    /* a flag to indicate if the lift is moving */ 
    int moving; 

    /* variable to indicate if the lift is travelling in the up 
       direction, which is defined as the direction where the 
       floor number is increasing */
    int up;

    /* persons on each floor waiting to enter */ 
    person_data_type persons_to_enter[N_FLOORS][MAX_N_PERSONS];

    /* passengers in the lift */
    person_data_type passengers_in_lift[MAX_N_PASSENGERS];

    /* mutex for mutual exclusion */
    pthread_mutex_t mutex; 

    /* condition variable, to indicate that something has happend */ 
    pthread_cond_t change; 

} lift_data_type;

typedef lift_data_type* lift_type;
/* fig_end lift_mon_type */ 

/* lift_create: creates and initialises a variable of type lift_type */
lift_type lift_create(void); 

/* lift_delete: deallocates memory for lift */
void lift_delete(lift_type lift); 

/* fig_begin mon_functions */ 
/* MONITOR function lift_next_floor: computes the floor to which 
   the lift shall travel. The parameter *change_direction 
   indicates if the direction shall be changed */
void lift_next_floor(
    lift_type lift, int *next_floor, int *change_direction); 

/* MONITOR function lift_move: makes the lift move from its current 
   floor to next_floor. The parameter change_direction indicates if 
   the move includes a change of direction. This function shall be 
   called by the lift process when the lift shall move */ 
void lift_move(
    lift_type lift, int next_floor, int change_direction); 

/* MONITOR function lift_has_arrived: shall be called by the lift 
   process when the lift has arrived at the next floor. This function 
   indicates to other processes that the lift has arrived, and then waits 
   until the lift shall move again. */
void lift_has_arrived(lift_type lift); 

/* MONITOR function lift_travel: makes the person with id id perform 
   a journey with the lift, starting at from_floor and ending 
   at to_floor */ 
void lift_travel(
    lift_type lift, int id, int from_floor, int to_floor);
/* fig_end mon_functions */ 

#endif
