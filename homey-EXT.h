/*
 * homey-EXT.h
 *
 *  Created on: Aug 22, 2013
 *      Author: Trex
 */


#ifndef HOMEY_EXT_H_
#define HOMEY_EXT_H_

//for semaphores
#include <semaphore.h>




/* DEFINES */
/*=======*/

//Critical section macros
#define ENTER_CRITICAL_SECTION(sem)   sem_wait(sem)
#define EXIT_CRITICAL_SECTION(sem)   sem_post(sem)


/*buttons*/
#define MAX_NR_OF_BUTTONS 4  //4 buttons on piface
#define MAX_NR_OF_LEDS	  8  //8 leds
#define MAX_INPUT_PINS	  8  //8 input pins

/*at least SECONDS_MIN must elapse before updating the same relay again*/
#define SECONDS_MIN  10u

/*will fail if cannot connect for a day*/
#define MAX_STARTUP_RETRY_COUNT 3600u

//GLOBAL VARIABLES
extern uint8_t numOfConfiguredDevices_u8;

/* semaphores are declared global so they can be accessed
   in main() and in thread routine,
   here, the semaphore is used as a mutex */
sem_t mutexSemaphoreQueryDB_st;
sem_t mutexSemaphoreUpdateDB_st;




/*FUNCTION DECLARATIONS*/

extern void ActivateItem(uint8_t selectedItem_lu8, uint8_t stateToSet_lu8);
extern void CleanupApp(void);
#endif /* HOMEY_EXT_H_ */
