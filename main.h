/*
 * main.h
 *
 *  Created on: Aug 22, 2013
 *      Author: Trex
 */

#ifndef MAIN_H_
#define MAIN_H_

//custom type definitions like TRUE, FALSE
#include "type-custom.h"

//db processing
#include "db-EXT.h"
//IO management
#include "out-EXT.h"
//SHared memory between PHP and C
#include "shmem-EXT.h"

//homey application
#include "homey-EXT.h"

/*TYPEDEFS*/

/*FUNCTIONS*/



/*Local Function Header declarations*/
void InitBoard();
void InitSW(void);

void intHandler(int signal_s32);
void CleanupAfterSigINT(void);
uint8_t ProcessCLIArguments(int argc_s32, const char **argv_ptru8);

/*Task Scheduler*/
void TaskScheduler(void);
/* Tasks*/
void Task_50ms();
void Task_100ms();
void Task_500ms();
void Task_1s();

#endif /* MAIN_H_ */
