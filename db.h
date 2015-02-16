/*
 * db.h
 *
 *  Created on: Aug 22, 2013
 *      Author: Trex
 */

#ifndef DB_H_
#define DB_H_
//custom type definitions like TRUE, FALSE
#include "type-custom.h"

#include "db-EXT.h"

//for threads
#include <pthread.h>

//for mysql
#include <mysql.h>

/*MACRO DEFINITIONS*/



/*TYPEDEF STRUCTURES*/




/*LOCAL FUNCTION DECLARATIONS*/
/*function will be run as a different thead*/
void *threadRefreshFromDB(void *arg_lptr);
void *threadUpdateDB(void *arg_lptr);
int8_t tActivateDevice(char *Housecode_lstr, uint8_t unitCode_lu8, uint8_t stateToSet_lu8);
uint8_t getUpdateCompleted();
uint8_t getRefreshQueryCompleted();


#endif /* DB_H_ */
