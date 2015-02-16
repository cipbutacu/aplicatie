/*
 * main.h
 *
 *  Created on: Aug 22, 2013
 *      Author: Trex
 */

#ifndef SHMEM_EXT_H_
#define SHMEM_EXT_H_

//undefine this to not to change the UID of the semaphore
//#define _CHANGE_UID


/*DECLARATION OF FUNCTIONS*/


/*Set UID and GID to be the same with the user under which the lightthttpd is running*/
#define SEMAPHORE_UID 33
#define SEMAPHORE_GID 33

extern void InitShm(void);
extern void CleanupShm(void);
extern int32_t semaphoreGetAccess(void);
extern int32_t semaphoreReleaseAccess(void);
uint8_t getShmValue(void);
void setShmValue(uint8_t value_lu8);
#endif /* SHMEM_EXT_H_ */


