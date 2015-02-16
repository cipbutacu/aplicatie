/*
 * shmem.h
 *
 *  Created on: Aug 22, 2013
 *      Author: Trex
 */

#ifndef SHMEM_H_
#define SHMEM_H_

//custom type definitions like TRUE, FALSE
#include "type-custom.h"

#include "shmem-EXT.h"
#include <sys/sem.h>		//Used for semaphores

/*size of the shared memory data*/
#define SHARED_MEMORY_SIZE  1

/*tupedefs and structures*/

//On linux systems this union is probably already defined in the included sys/sem.h, but if not use this default basic definition:
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

struct semid_ds semid_ds;

//----- SHARED MEMORY -----
typedef struct sharedMemory_stype {
	uint8_t data_au8[SHARED_MEMORY_SIZE];
}sharedMemory_st;

#define	SHARED_MEMORY_KEY 		672213396   		//Shared memory unique key
#define	SEMAPHORE_KEY			291623581


#endif /* SHMEM_H_ */
