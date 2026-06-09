/**
 * @file controller_events.h
 * @author Dylan Geske
 * @brief 
 * @version 0.1
 * @date 2026-04-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
 #ifndef __controller_events_H__  
 #define  __controller_events_H__

 typedef struct controller_events
{

    unsigned int horn : 1;               /* Falling edge of SW1 detected length of 1 bit* NEEDED*/
    unsigned int pair : 1;                /* Falling edge of SW2 detected  */
    unsigned int reset : 1;              /* Falling edge of SW3 detected  */
    unsigned int accelerate : 1;         /* Falling edge of SW4 detected NEEDED */
    unsigned int brake : 1;              /* Falling edge of SW5 detected  */
    unsigned int turn_left : 1;          /* JS Left position detected    NEEDED  */
    unsigned int turn_right : 1;         /* JS Right position detected   NEEDED */
    unsigned int gear_sel : 1;           /* 0 for forward, 1 for reverse NEEDED */
    unsigned int record : 1;             /* Falling edge of JS Record BTT */
    
} controller_events_t;

//extern volatile ece353_events_t ECE353_Events;
extern volatile controller_events_t Controller_Events;

#endif