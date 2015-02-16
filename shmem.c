/*
 * shmem.c:
 *      Shared memory related  Related.
  ***********************************************************************
 */
#include "shmem.h"
#include "shmem-EXT.h"

#include <sys/shm.h>		//Used for shared memory
#include <sys/sem.h>		//Used for semaphores
#include <errno.h>

//stdin , printf and stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*GLOBAL VARIABLES*/

/*shared memory id*/
int32_t sharedMemoryId_s32 = 0;
/*System V semaphore ID*/
int32_t semaphoreID_s32 = 0;


/*pointer to shared memory location*/
void *sharedMemory_ptr = NULL;
sharedMemory_st *sharedMemoryAcc_ptr = NULL;

/*FUNCTION DEFINITIONS*/

void InitShm(void)
{
	uint8_t ForIndex_u8=0;
	union semun sem_union_init;


	//-----------------------------------------------
	//----- CREATE SHARED MEMORY WITH SEMAPHORE -----
	//-----------------------------------------------
	fprintf(stdout,"\nCreating semaphore...");
	semaphoreID_s32 = semget((key_t)SEMAPHORE_KEY, 1, 0666 | IPC_CREAT);		//Semaphore key, number of semaphores required, flags
	//	Semaphore key
	//	Unique non zero integer (usually 32 bit).  Needs to avoid clashing with another other processes semaphores (you just have to pick a random value and hope - ftok() can help with this but it still doesn't guarantee to avoid colision)
    //Initialize the semaphore using the SETVAL command in a semctl call (required before it can be used)
	sem_union_init.val = 1;
	if (semctl(semaphoreID_s32, 0, SETVAL, sem_union_init) == -1)
	{
			fprintf(stdout,"\nCreating semaphore failed to initialize %d", errno );
			exit(EXIT_FAILURE);
	}
	else
	{
		//nothing
	}
#ifdef _CHANGE_UID
	sem_union_init.buf = &semid_ds;
	if (semctl(semaphoreID_s32, 0, IPC_STAT, sem_union_init) == -1)
	{
		fprintf(stdout,"\nError getting semaphore status: %d", errno );
		exit(EXIT_FAILURE);
	}
	else
	{
			//nothing
	}
	//set new UID AND GID, otherwise the non root user will not be able to use it :(
	semid_ds.sem_perm.uid = SEMAPHORE_UID;
	semid_ds.sem_perm.gid = SEMAPHORE_GID;
	semid_ds.sem_perm.cuid = SEMAPHORE_UID;
	semid_ds.sem_perm.cgid = SEMAPHORE_GID;

	if (semctl(semaphoreID_s32, 0, IPC_SET, sem_union_init) == -1)
	{
		fprintf(stdout,"\nError setting semaphore status: %d", errno );
		exit(EXIT_FAILURE);
	}
	else
	{
			//nothing
	}
#endif

	fprintf(stdout,"\nCreating shared memory...");

	//Create the shared memory
	sharedMemoryId_s32 = shmget((key_t)SHARED_MEMORY_KEY, sizeof(sharedMemory_st), 0666 | IPC_CREAT);		//Shared memory key , Size in bytes, Permission flags
	//	Shared memory key
	//		Unique non zero integer (usually 32 bit).  Needs to avoid clashing with another other processes shared memory (you just have to pick a random value and hope - ftok() can help with this but it still doesn't guarantee to avoid colision)
	//	Permission flags
	//		Operation permissions 	Octal value
	//		Read by user 			00400
	//		Write by user 			00200
	//		Read by group 			00040
	//		Write by group 			00020
	//		Read by others 			00004
	//		Write by others			00002
	//		Examples:
	//			0666 Everyone can read and write

	if (sharedMemoryId_s32 == -1)
	{
		fprintf(stdout,"\nShared memory shmget() failed\n");
		exit(EXIT_FAILURE);
	}

	//Make the shared memory accessible to the program
	sharedMemory_ptr = shmat(sharedMemoryId_s32, (void *)0, 0);
	if (sharedMemory_ptr == (void *)-1)
	{
		fprintf(stdout,"\nShared memory shmat() failed\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stdout,"\nShared memory attached at %X\n", (int)sharedMemory_ptr);

	//Assign the shared_memory segment
	sharedMemoryAcc_ptr = (sharedMemory_st *)sharedMemory_ptr;

	//----- SEMAPHORE GET ACCESS -----
	if (!semaphoreGetAccess())
	{
		exit(EXIT_FAILURE);
	}
	else
	{
		//function will block until semaphore is released
	}
	//----- WRITE SHARED MEMORY -----
	for (ForIndex_u8 = 0; ForIndex_u8 < sizeof(sharedMemory_st); ForIndex_u8++)
		sharedMemoryAcc_ptr->data_au8[ForIndex_u8] = 0x00;

	//----- SEMAPHORE RELEASE ACCESS -----
	if (!semaphoreReleaseAccess())
	{
		exit(EXIT_FAILURE);
	}
	else
	{
		//semaphore will be unlocked
	}

}

/* *************************************************************************
 * 	   Stall if another process has the semaphore,
	   then assert it to stop another process taking it

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
int32_t semaphoreGetAccess(void)
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1; /* P() */
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sharedMemoryId_s32, &sem_b, 1) == -1)		//Wait until free
	{
		fprintf(stdout,"\nSemaphore_get_access failed with error:%s",strerror(errno));
		return(0);
	}
	else
	{
		//nothing
	}
	return(1);
}



/* *************************************************************************
 * 	   Release the semaphore and allow another process to take it
	   then assert it to stop another process taking it

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
int32_t semaphoreReleaseAccess(void)
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = 1; /* V() */
	sem_b.sem_flg = SEM_UNDO;
	if (semop(semaphoreID_s32, &sem_b, 1) == -1)
	{
			fprintf(stdout,"\nSemaphore_release_access failed with error:%s", strerror(errno));
			return(0);
	}
	else
	{
		//nothing
	}
	return(1);
}


/* *************************************************************************
 * 	   Cleanup Function

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void CleanupShm(void)
{
	//----- DETACH SHARED MEMORY -----
	if (shmdt(sharedMemory_ptr) == -1)
	{
		fprintf(stdout,"\n shmdt failed");
    }
	else
	{
		//empty
	}
    if (shmctl(sharedMemoryId_s32, IPC_RMID, 0) == -1)
	{
		fprintf(stdout,"\nshmctl(IPC_RMID) failed");

	}
    //Delete the Semaphore
    //It's important not to unintentionally leave semaphores existing after program execution. It also may cause problems next time you run the program.
    union semun sem_union_delete;
    if (semctl(semaphoreID_s32, 0, IPC_RMID, sem_union_delete) == -1)
    {
    	fprintf(stdout,"\nFailed to delete semaphore");
    }
    else
    {
    	//nothing
    }
}


/* *************************************************************************
 * 	   Returns the value of the shared memory

	   @param[in]     none
	   @return 		  value of the one and only byte :)
 * *************************************************************************/
uint8_t getShmValue(void)
{
		uint8_t retVal_lu8 = 0;

		//----- SEMAPHORE GET ACCESS -----
		if (!semaphoreGetAccess())
		{
			printf ("\n Semaphore could not be locked");
			exit(EXIT_FAILURE);
		}
		else
		{
			//function will block until semaphore is released
		}


		retVal_lu8 = ((uint8_t) sharedMemoryAcc_ptr->data_au8[0]);

		//----- SEMAPHORE RELEASE ACCESS -----
		if (!semaphoreReleaseAccess())
		{
			printf ("\n Semaphore could not be  unlocked");
			exit(EXIT_FAILURE);
		}
		else
		{
			//semaphore will be unlocked
		}
		return (retVal_lu8);
}

/* *************************************************************************
 * 	   Sets the value of the shared memory

	   @param[in]     value to set
	   @return 		  none
 * *************************************************************************/
void setShmValue(uint8_t value_lu8)
{
	//----- SEMAPHORE GET ACCESS -----
	if (!semaphoreGetAccess())
	{
		exit(EXIT_FAILURE);
	}
	else
	{
		//function will block until semaphore is released
	}
	//----- WRITE SHARED MEMORY -----

	//Write initial values '0' - data has been processed, '1' - new data from php
	sharedMemoryAcc_ptr->data_au8[0] = value_lu8;

	//----- SEMAPHORE RELEASE ACCESS -----
	if (!semaphoreReleaseAccess())
	{
		exit(EXIT_FAILURE);
	}
	else
	{
		//semaphore will be unlocked
	}
}

