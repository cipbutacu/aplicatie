/*
 * homey.c:
 *      Homey Main Application.
  ***********************************************************************
 */


#include <softPwm.h>

//piface board
#include <wiringPi.h>
#include <piFace.h>

//for catching system signals (SIGINT)
#include  <signal.h>


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//sttring manipulations
#include <string.h>

//for sleep and stuff
#include <unistd.h>

//for threads
#include <pthread.h>


#include "main.h"

#include "db-EXT.h"


/*DEFINES*/



/*Undefine this to disable debug output to console*/
#define _DEBUG_ENABLED

//max value for the task counter
#define TASK_COUNTER_MAX_VAL  20u


/*TASK RELATED*/
#define TASK_CYCLE_TIME_50ms  1u
#define TASK_CYCLE_TIME_100ms 2u
#define TASK_CYCLE_TIME_500ms 10u
#define TASK_CYCLE_TIME_1s 	  20u

/*force update every X seconds, even if no update was done on the webserver */
#define TIMER_VAL_SECONDS  120u


//FUNCTION LIKE MACROS


/*GLOBAL VARIABLES*/

//if CTRL+C is pressed, this is set
uint8_t  sigINTReceived = 0;

uint8_t g_TaskCounter_u8 =0u;


/*timer to be used to refresh periodically the status of the leds from the DB*/
uint8_t updateTimerExpired_u8 = 0;

/*startup retry counter*/
uint32_t startupRetryCnt_u32 = 0;

/*DEBUG enabled declarations*/
#ifdef _DEBUG_ENABLED
#endif


/*FUNCTION DECLARATIONS*/




/* *************************************************************************
 * 	   50ms task
	   This function will be called every 50ms

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void Task_50ms()
{
	//fprintf(stdout,"\n50ms");
	/*read the switches*/
	get_Inputs();
	/*process data*/
    process_Buttons();
}


/* *************************************************************************
 * 	   100ms task
	   This function will be called every 100ms

	   @param[in]     nonevoid TaskScheduler(void)
	   @return 		  none
 * *************************************************************************/
void Task_100ms()
{
	//fprintf(stdout,"\n100ms");
	//fprintf(stdout,"\n50ms");

		/*set the led outputs*/
	set_Outputs();
}


/* *************************************************************************
 * 	   500ms task
	   This function will be called every 500ms

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void Task_500ms()
{
	//fprintf(stdout,"\n500ms");
}


/* *************************************************************************
 * 	   1000ms task
	   This function will be called every 1000ms

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void Task_1s()
{
	uint8_t tempVal;
	uint8_t refreshNeededAfterUpdateCompleted_lu8 = FALSE;
	//read the value of the semaphore
	tempVal =  getShmValue();
	refreshNeededAfterUpdateCompleted_lu8 = getRefreshNeeded();
	if (refreshNeededAfterUpdateCompleted_lu8)
	{
		//visual feedback that update has performed (blink all leds)
		FlashAllLeds();
	}
	else
	{
		//no visual feedback needed
	}

	if (tempVal)
	{
		setShmValue(0);
		fprintf(stdout,"\n Update performed by webif. Refresh started");
	}
	else
	{
		//no need
	}

	if (
			(!updateTimerExpired_u8)
			||
			(tempVal)
			||
			(refreshNeededAfterUpdateCompleted_lu8)
       )
	{
		updateTimerExpired_u8 = TIMER_VAL_SECONDS;
		//call function to refresh state from DB
		if (!RefreshStateFromDB())
		{
			//most probably another query is already running , what shall we do here ? Probably nothing, retry next time :)
		}
		else
		{
			//all ok
		}
	}
	else
	{
		updateTimerExpired_u8--;
	}
}



/* *************************************************************************
 * 	   Init board function
	   This function will call the initialization functions of the board

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void InitBoard(void)
{
	InitIO();
}

/* *************************************************************************
 * 	   Init SW board function
	   This function will be called at init, after board init

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void InitSW(void)
{
	InitShm();
	InitDB();
}





/* *************************************************************************
 * 	   TaskScheduler function
	   This is the task scheduler

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void TaskScheduler(void)
{
  /*call the 50ms task*/
  if (!(g_TaskCounter_u8 % TASK_CYCLE_TIME_50ms))
  {
	  Task_50ms();
  }
  else
  {
	  //nothing
  }

  /*call the 100ms task*/
  if (!(g_TaskCounter_u8 % TASK_CYCLE_TIME_100ms))
  {
	Task_100ms();
  }
  else
  {
		  //nothing
  }

  /*call the 500ms task*/
  if (!(g_TaskCounter_u8 % TASK_CYCLE_TIME_500ms))
  {
	Task_500ms();
  }
  else
  {
		  //nothing
  }

  /*call the 1s task*/
  if (!(g_TaskCounter_u8 % TASK_CYCLE_TIME_1s))
  {
	Task_1s();
  }
  else
  {
		  //nothing
  }
}

/* *************************************************************************
 * 	   Main.c function
	   This is the main function of the module

	   @param[in]     _argc number of command line arguments
	   @param[in]    *argv array containing command line parameters
	   @return 		  false:  Don't start, it was called with help parameter
	   	   	   	   	  true:   All is OK
 * *************************************************************************/
uint8_t ProcessCLIArguments(int32_t argc_s32, const char **argv_ptru8)
{
	int32_t  option_ls32;
	uint8_t  retVal_u8 = TRUE;
	while ((option_ls32 = getopt (argc_s32, (char * const*)argv_ptru8, "h:u:p:d:t:?")) != -1)
	  {
		switch(option_ls32)
		{
			case 'h'://host
				strcpy (mysqlConnect_st.server_au8, optarg);
			break;
			case 'u'://username db
				strcpy (mysqlConnect_st.user_au8, optarg);
			break;
			case 'p': //password db
				strcpy (mysqlConnect_st.password_au8, optarg);
			break;
			case 'd':  //database
				strcpy (mysqlConnect_st.database_au8, optarg);
			break;
			case 't':  //table
				strcpy (mysqlConnect_st.table_au8, optarg);
			break;
			case '?':  //HELP, NO ARGUMENT
				fprintf(stdout,"\n Usage:  homey [-h HOSTNAME] [-u DB_USERNAME] [-p DB_PASWORD] [-d DB_DATABASE_NAME] [-t DB_TABLE_NAME] [-?]");
				retVal_u8 = FALSE;
				break;
			default:
			break;
		}//end switch
	  }

	fprintf(stdout,"\n Using following connect parameters:");
	fprintf(stdout,"\n Mysql host: %s", mysqlConnect_st.server_au8);
    fprintf(stdout,"\n Mysql user: %s", mysqlConnect_st.user_au8);
	fprintf(stdout,"\n Mysql password: %s", mysqlConnect_st.password_au8);
	fprintf(stdout,"\n Mysql database: %s", mysqlConnect_st.database_au8);
	fprintf(stdout,"\n Mysql table: %s", mysqlConnect_st.table_au8);
	return retVal_u8;
}

/* *************************************************************************
 * 	   Main.c function
	   This is the main function of the module

	   @param[in]     _argc number of command line arguments
	   @param[out]    *argv array containing command line parameters
	   @return error code.
 * *************************************************************************/
int main (int argc, char **argv)
{
  fprintf(stdout,"Welcome to Homey\n");
  fprintf(stdout,"================\n");

  signal(SIGINT, intHandler);
  signal(SIGKILL, intHandler);
  signal(SIGQUIT, intHandler);

  /*Initialize the Rapsberry board*/
  InitBoard();
  /*init sw variables*/
  InitSW();

  /*process command line arguments  - call this after init sw*/
  if (!ProcessCLIArguments(argc, (const char **)argv))
  {
	  CleanupAfterSigINT();
	  exit(0);
  }
  else
  {
	  //nothing
  }

  do
  {
	  /*check how many devices are configured - needed by the menu system*/
	  	numOfConfiguredDevices_u8 = getNumberOfConfiguredDevices();
	    if (numOfConfiguredDevices_u8<1)
	    {
	      startupRetryCnt_u32++;
	      fprintf(stdout,"\n No configured devices available. Retry (%d) in 10 seconds.", startupRetryCnt_u32);
	  	  //*sleep for 10 seconds*/
	  	  sleep (10);
	    }
	    else
	    {
	  	  //proceed

	    }
  }
  while((numOfConfiguredDevices_u8<1) && (startupRetryCnt_u32 <= MAX_STARTUP_RETRY_COUNT)) ;

  if (numOfConfiguredDevices_u8<1)
  {
	  fprintf(stdout,"\n No configured devices available. Process will now exit");
	  //call cleanup functions
	  CleanupAfterSigINT();
	  exit(0);
  }
  else
  {
	  //proceed
	  fprintf(stdout,"\n Number of configured devices: %d", numOfConfiguredDevices_u8);
  }


  //entering main loops
  while(1)
  {
	  if (sigINTReceived)
	  {
			fprintf(stdout,"\n Exiting now...\n");
			//call cleanup functions
			CleanupAfterSigINT();
			exit(0);
	  }
	  else
	  {

	  }
	  /*call the simple task scheduler*/
	  TaskScheduler();
	  /*advance task counter*/
	  if (g_TaskCounter_u8 <= TASK_COUNTER_MAX_VAL)
	  {
		 		  //increment value
		 g_TaskCounter_u8++;
	  }
	  else
	  {
		 g_TaskCounter_u8 = 1;
	  }
	  //sleep for 50ms
	  usleep(50000);
	  // mS
  }

}

/* *************************************************************************
 * 	   System Signal handler function
	   This function will be called after user presses CTRL+C (SIGINT)

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void intHandler(int signal_s32)
{
	sigINTReceived = TRUE;
}

/* *************************************************************************
 * 	   Cleanup function
	   This function will be called after user presses CTRL+C (SIGINT)

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void CleanupAfterSigINT(void)
{
	 CleanupDB();
	 CleanupApp();
	 CleanupShm();
	 CleanupIO();
	 /* exit */
	 exit(0);
}
