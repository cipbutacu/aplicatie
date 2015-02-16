/*
 * db.c:
 *      Homey DB related  Application.
  ***********************************************************************
 */

/* *************************************************************************
 * 	   Activate device associated with
	   the ID indicated by parameter

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
#include "db.h"
#include "db-EXT.h"
#include "homey-EXT.h"
#include "out-EXT.h"


//stdin , printf and stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

//for mysql
#include <mysql.h>

//sleep manipulations
#include <string.h>




/*GLOBAL VARIABLES*/



/*mysql connection details*/
MysqlDB_st mysqlConnect_st;
/*result pointer*/
MYSQL_RES *result_ptr = NULL;
/*number of fields in a query*/
uint32_t numOfFields_u16 = 0;
/*structure containing row data*/
MYSQL_ROW row_st;

char queryStr_au8[MAX_QUERY_STRING_LENGTH];
char tempStr_au8[MAX_QUERY_STRING_LENGTH];
/*flags for thread sync*/
uint8_t refreshQuerryRunning_b = FALSE;
/*indicates if a query is already running*/
uint8_t updateQuerryRunning_b = FALSE;

uint8_t updatePerformedSinceLastCheck_u8 = FALSE;

/* this variable is our reference to the second thread */
pthread_t pthreadRefreshFromDB_st;
/*thread will update db and send heyu command to relays*/
pthread_t pthreadUpdateDB_st;

/*global variable used to pass as an argument to the thread. It's value is modified once before the thread is created, so there's no need to protect it with semaphores*/
volatile uint8_t threadArgumentArr_au8[MAX_NR_OF_THREAD_PARAMS];

/*FUNCTION PROTOTYPES*/

/* *************************************************************************
 * 	   Activate device associated with
	   the ID indicated by parameter

	   @param[in]     deviceIndex_lu8 :  indicates the device index that has
	   to be activated (0 - 7)
	   @return 		  FALSE: error occured
	   	   	   	   	  TRUE:  function completed without error
 * *************************************************************************/
uint8_t updateDBActDevice( uint8_t deviceIndex_lu8, uint8_t stateToSet_lu8)
{
	static uint8_t firstRunUq_lb = TRUE;
	uint8_t refreshQuerryCompleted_lb;
	uint8_t updateQuerryCompleted_lb;
	uint8_t retVal_lu8 = TRUE;
	/*check if any other refresh/update is running*/
	refreshQuerryCompleted_lb  = getRefreshQueryCompleted();
	updateQuerryCompleted_lb =  getUpdateCompleted();

	if ((!refreshQuerryCompleted_lb) || (!updateQuerryCompleted_lb))
	{
		/*another update query did not finish*/
		fprintf(stdout,"\n Not updating this time! Query already running (Update: %d, Refresh: %d)", updateQuerryCompleted_lb, refreshQuerryCompleted_lb);
		retVal_lu8 = FALSE;
	}
	else
	{
		if (!firstRunUq_lb)
		{
			/* wait for the second thread to finish */
			if(pthread_join(pthreadUpdateDB_st, NULL))
			{
				fprintf(stdout,"Error joining thread\n");
				retVal_lu8 = FALSE;
			}
			else
			{
				/*do nothing*/
			}
		}
		else
		{
			//first run, not necessary to wait for other thread to die
		}

		/*get active selection.
		 * Note: as it's address  is passed as a parameter to the thread,
		 * it has to be a global variable, otherwise after the function terminates, the pointer will be invalid
		 * It does not need to be protected by semaphore, see explation at declaration*/
		/*This will cotain the index of the device you want to activate*/
		threadArgumentArr_au8[0] = deviceIndex_lu8;

		/*This will contain the state that needs to be set*/
		threadArgumentArr_au8[1] = stateToSet_lu8;

		/*create a new thread*/
		if(pthread_create(&pthreadUpdateDB_st, NULL, threadUpdateDB, (void*)&threadArgumentArr_au8))
			{

				fprintf(stdout,"\nError creating thread\n");
				retVal_lu8 = FALSE;
			}
			else
			{
				/*Critical Section*/
				ENTER_CRITICAL_SECTION(&mutexSemaphoreUpdateDB_st);
				updateQuerryRunning_b = TRUE;
				EXIT_CRITICAL_SECTION(&mutexSemaphoreUpdateDB_st);
				/*End Critical section*/
				firstRunUq_lb = FALSE;
			}

	}
	return (retVal_lu8);
}

/* *************************************************************************
 * 	   Returns TRUE if update has completed or no update is currently ungoing

	   @param[in]
	   @return 		  TRUE:  update completed or no update running currently
	   	   	   	   	  FALSE: update is currently running
 * *************************************************************************/
uint8_t getUpdateCompleted()
{
	uint8_t temp_lb;

	ENTER_CRITICAL_SECTION(&mutexSemaphoreUpdateDB_st);
	temp_lb = updateQuerryRunning_b;
	EXIT_CRITICAL_SECTION(&mutexSemaphoreUpdateDB_st);
	return (!temp_lb);
}


/* *************************************************************************
 * 	   Returns TRUE if refresh has completed or no update is currently ungoing

	   @param[in]
	   @return 		  TRUE:  update completed or no update running currently
	   	   	   	   	  FALSE: update is currently running
 * *************************************************************************/
uint8_t getRefreshQueryCompleted()
{
	uint8_t temp_lb;

	ENTER_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
	temp_lb = refreshQuerryRunning_b;
	EXIT_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);

	return (!temp_lb);
}

/* *************************************************************************
 * 	   Returns TRUE if refresh of LED states is needed (update was performed)

	   @param[in]
	   @return 		  TRUE:  refresh needed
	   	   	   	   	  FALSE: refresh not needed
 * *************************************************************************/
uint8_t getRefreshNeeded(void)
{
	uint8_t temp_lb;
	ENTER_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
	temp_lb = updatePerformedSinceLastCheck_u8;
	updatePerformedSinceLastCheck_u8 = FALSE;
	EXIT_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
	return (temp_lb);


}


/* *************************************************************************
 * 	   Will call the heyu CLI to send the x10 command to the relays

	   @param[in]     Housecode_lc :  indicates the housecode of the device
	   	   	   	      that needs to be activated ('A' - 'K'
	   	   	   	      unitCode_lu8 :  unit code of the device that will be activated (0 -15)
	   @return 		  none
	   @info:		  This will be run as a separate thread
 * *************************************************************************/
int8_t tActivateDevice(char *Housecode_lstr, uint8_t unitCode_lu8, uint8_t stateToSet_lu8)
{
	int8_t retVal=0;
	/*build up string*/
	strcpy (queryStr_au8, "/usr/local/bin/heyu ");
	if (stateToSet_lu8)
	{
		strcat (queryStr_au8, "on ");
	}
	else
	{
		strcat (queryStr_au8, "off ");
	}

	if (Housecode_lstr != NULL)
	{
		strcat (queryStr_au8, Housecode_lstr);
	}
	else
	{
		//no need to go further
		return -1;
	}

	snprintf(tempStr_au8, MAX_QUERY_STRING_LENGTH, "%d", unitCode_lu8);
	strcat (queryStr_au8, tempStr_au8);

	//fprintf(stdout,"\n System call will be made with this string %s", queryStr_au8);

	//call the system command
	retVal =  system (queryStr_au8);
     if (retVal == -1)
     {
        fprintf(stdout,"\nSystem call failed with errno = %d\n", errno);
     }
     else
     {
    	 //no failure
     }
     return (retVal);
}

/* *************************************************************************
 * 	   Will call the heyu CLI to send the x10 command to the relays

	   @param[in]     Housecode_lc :  indicates the housecode of the device
	   	   	   	      that needs to be activated ('A' - 'K'
	   	   	   	      unitCode_lu8 :  unit code of the device that will be activated (0 -15)
	   @return 		  none
	   @info:		  This will be run as a separate thread
 * *************************************************************************/
int8_t tGetDeviceStatus(char *Housecode_lstr, uint8_t unitCode_lu8)
{
	int8_t  retVal = 0;
	/*build up string*/
	strcpy (queryStr_au8, "/usr/local/bin/heyu status ");
	if (Housecode_lstr != NULL)
	{
		strcat (queryStr_au8, Housecode_lstr);
	}
	else
	{
		//no need to go further
		return -1;
	}

	snprintf(tempStr_au8, MAX_QUERY_STRING_LENGTH, "%d", unitCode_lu8);
	strcat (queryStr_au8, tempStr_au8);

	fprintf(stdout,"\n System call will be made with this string %s", queryStr_au8);

	//call the system command
	retVal =  system (queryStr_au8);
     if (retVal == -1)
     {
        fprintf(stdout,"\nSystem call failed with errno = %d\n", errno);
     }
     else
     {
    	 //no failure
     }
     return (retVal);
}


/* *************************************************************************
 * 	   Will refresh the state of the leds accordin to setting in the DB

	   @param[in]     none
	   @return 		  FALSE: error occured
	   	   	   	   	  TRUE:  function completed without error
 * *************************************************************************/
uint8_t RefreshStateFromDB(void)
{
	static uint8_t firstRun_lb = TRUE;
	uint8_t retVal_lu8 = TRUE;
	uint8_t refreshQuerryCompleted_lb;
	uint8_t updateQuerryCompleted_lb;

	/*check if any other refresh/update is running*/
	refreshQuerryCompleted_lb  = getRefreshQueryCompleted();
	updateQuerryCompleted_lb =  getUpdateCompleted();

	if ((!refreshQuerryCompleted_lb) || (!updateQuerryCompleted_lb))
	{
		/*another refresh query did not finish*/
		fprintf(stdout,"\n Not updating this time! Query already running (Update: %d, Refresh: %d)", updateQuerryCompleted_lb, refreshQuerryCompleted_lb);
		retVal_lu8 = FALSE;
	}
	else
	{
		if (!firstRun_lb)
		{
			/* wait for the second thread to finish */
			if(pthread_join(pthreadRefreshFromDB_st, NULL))
			{
				fprintf(stdout,"Error joining thread\n");
				retVal_lu8 = FALSE;
			}
			else
			{
				/*do nothing*/
			}
		}
		else
		{
			//first run, not necessary to wait for other thread to die
		}
		/*create a new thread*/
		if(pthread_create(&pthreadRefreshFromDB_st, NULL, threadRefreshFromDB, NULL))
			{

				fprintf(stdout,"\nError creating thread\n");
				retVal_lu8 = FALSE;
			}
			else
			{
				/*Critical Section*/
				ENTER_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
				refreshQuerryRunning_b = TRUE;
				EXIT_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
				/*End Critical section*/
				firstRun_lb = FALSE;
			}
	}
	return (retVal_lu8);
}

/* *************************************************************************
 * 	   Update the DB with the newly activated device and send x10 command
 * 	   This is ran as a different thread
	   @param[in]     arg_lptr - pointer argument of type void
	   @return 		  none
 * *************************************************************************/
void *threadUpdateDB(void *arg_lptr)
{
	/*threaded function*/
	/*device index*/
	uint8_t deviceIndex_lu8 =*((uint8_t *)arg_lptr + 0);
	/*new state. Note: state has to inverted, as the new state will be the opposite of the current state*/
	uint8_t stateToSet_lu8 =!(*((uint8_t *)arg_lptr + 1));
	/*variable holding the current unixtime*/
	uint32_t unixtime_lu32 = 0;
	/*operation results are kept here:  -1:something went wrong, 0 otherwise*/
	int8_t result_lu8 = 0;
	uint32_t  temp_lu32=0, temp2_lu32=0;
	/*storage of x10 relay data*/
    char name_lstr[MAX_QUERY_STRING_LENGTH], houseCode_lstr[MAX_QUERY_STRING_LENGTH];
    uint8_t unitCode_lu8;
    uint8_t supportsStatusRequest_lu8 =0;


	mysqlConnect_st.conn_ptrst = mysql_init(NULL);
	if (!mysqlConnect_st.conn_ptrst )
	{
		/*connect error*/
		fprintf(stdout,"\nMySQL null pointer error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
		result_lu8 = -1;
	}
	else
	{
		if (mysql_real_connect(mysqlConnect_st.conn_ptrst, mysqlConnect_st.server_au8, mysqlConnect_st.user_au8,
								   mysqlConnect_st.password_au8,
								   mysqlConnect_st.database_au8, 0, NULL, 0) == NULL)
		{
			    /*connect error*/
				fprintf(stdout,"\nMySQL connect error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
				result_lu8 = -1;
		}
		else
		{
			//get current time
			unixtime_lu32 = time(NULL);

			//get the housecode and unitcode, last update time  of the device based on the index call the heyu system app to send the x10 command
			strcpy (queryStr_au8, "SELECT *  FROM devices WHERE id='");
			snprintf(tempStr_au8, MAX_QUERY_STRING_LENGTH, "%d", deviceIndex_lu8);
			strcat (queryStr_au8, tempStr_au8);
			strcat (queryStr_au8, "'");
			//fprintf(stdout,"\n Select Query string: %s", queryStr_au8);

			//run querry
			if (mysql_query(mysqlConnect_st.conn_ptrst, queryStr_au8))
			{
				/*connect error*/
				fprintf(stdout,"\nMySQL query error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
				result_lu8 = -1;
			}
			else
			{
				result_ptr = mysql_store_result(mysqlConnect_st.conn_ptrst);
				if (result_ptr == NULL)
				{
					/* error*/
					fprintf(stdout,"\nMySQL result ptr error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
					result_lu8 = -1;
				}
				else
				{
					  numOfFields_u16 = mysql_num_fields(result_ptr);
					  if (numOfFields_u16 != 7)
					  {
						  fprintf(stdout,"\n Mysql Table is empty or structure not OK !");
						  result_lu8 = -1;
					  }
					  else
					  {
						  while ((row_st = mysql_fetch_row(result_ptr)))
						  {

						      strcpy(name_lstr,row_st[1]);
						      strcpy(houseCode_lstr,row_st[2]);
						      unitCode_lu8 = strtol(row_st[3], (char **)NULL, 10);
						      supportsStatusRequest_lu8 = strtol(row_st[6], (char **)NULL, 10);

						      /*convert id string to integer and range check*/
							  //check first if there are at least 10 seconds elapsed since this device was updated last time
							  temp_lu32= strtol(row_st[5], (char **)NULL, 10);
							  if (temp_lu32>=unixtime_lu32)
							  {
								  fprintf(stdout,"\n System date is wrong! Unit's last modified state is in the future!");
								  result_lu8 = -1;
							  }
							  else
							  {
								  temp2_lu32 = (unixtime_lu32 - temp_lu32);
								  if (temp2_lu32 < SECONDS_MIN)
								  {
									  fprintf(stdout,"\n Only %d seconds have elapsed since last update. You still need to wait %d seconds before a new update", temp2_lu32, SECONDS_MIN- temp2_lu32);
									  result_lu8 = -1;
								  }
								  else
								  {
									  //activate the relay
									  result_lu8 = tActivateDevice(houseCode_lstr, unitCode_lu8, stateToSet_lu8);
									  if (result_lu8!= -1)
									  {
										fprintf(stdout,"\nRelay activation command sent");
									  }
									  else
									  {
										 //error occured
									  }
								  }
							  }
						  }
					  }//num fields not ok

				}
				  mysql_free_result(result_ptr);
			}

			if (result_lu8 == -1)
			{
				//no need to proceed
			}
			else
			{
			    fprintf(stdout,"\nUpdating relay %s (index=%d, [H:%s U:%d]) to state %d",name_lstr, deviceIndex_lu8, houseCode_lstr, unitCode_lu8, stateToSet_lu8);
				//perform update of database
				//build query string
				strcpy (queryStr_au8, "UPDATE devices SET  LastState='");
				snprintf(tempStr_au8, MAX_QUERY_STRING_LENGTH, "%u", stateToSet_lu8);
				strcat (queryStr_au8, tempStr_au8);

				strcat (queryStr_au8, "' ,LastModified='");
				snprintf(tempStr_au8, MAX_QUERY_STRING_LENGTH, "%u", unixtime_lu32);
				strcat (queryStr_au8, tempStr_au8);

				strcat (queryStr_au8, "' WHERE id='");

				snprintf(tempStr_au8, MAX_QUERY_STRING_LENGTH, "%d", deviceIndex_lu8);
				strcat (queryStr_au8, tempStr_au8);
				strcat (queryStr_au8, "'");
				//fprintf(stdout,"\n Update Query string: %s", queryStr_au8);

				//run querry
				if (mysql_query(mysqlConnect_st.conn_ptrst, queryStr_au8))
				{
						  /*connect error*/
						 fprintf(stdout,"\nMySQL query error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
						 result_lu8 = -1;
				}
				else
				{
					numOfFields_u16 =  mysql_affected_rows(mysqlConnect_st.conn_ptrst);
					if (numOfFields_u16 != 1u)
					{
						/*error occured*/
						fprintf(stdout,"\nMySQL error (no rows affected, probably device ID not existing): %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
						result_lu8 = -1;
					}
					else
					{
						//no error
					}
				}
				if(supportsStatusRequest_lu8)
				{
					//get status from relay
					fprintf(stdout,"\nStatus reported by relay after activation:");
					result_lu8 = tActivateDevice(houseCode_lstr, unitCode_lu8, stateToSet_lu8);
					//no need to check for result. If ok, system command displayed status, otherwise error was displayed inside function
				}
				else
				{
					//do nothing
				}
			}
		}

	}
	//close connection
	mysql_close(mysqlConnect_st.conn_ptrst);

	ENTER_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
	updateQuerryRunning_b = FALSE;
	if (result_lu8 != -1)
	{
		updatePerformedSinceLastCheck_u8 = TRUE;
	}
	else
	{
		//no need to perform update
		updatePerformedSinceLastCheck_u8 = FALSE;
	}
	EXIT_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
	//fprintf(stdout,"\n Update Thread ended");
	return NULL;
}


/* *************************************************************************
 * 	   Refresh the led states from the setting from DB.
 * 	   This is ran as a different thread
	   @param[in]     arg_lptr - pointer argument of type void
	   @return 		  none
 * *************************************************************************/
void *threadRefreshFromDB(void *arg_lptr)
{
	/*threaded function*/
	uint32_t  temp_lu32=0,temp2_lu32=0;
	//init mysql
	mysqlConnect_st.conn_ptrst = mysql_init(NULL);
	if (!mysqlConnect_st.conn_ptrst )
	{
	    /*connect error*/
		fprintf(stdout,"\nMySQL null pointer error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
	}
	else
	{
		if (mysql_real_connect(mysqlConnect_st.conn_ptrst, mysqlConnect_st.server_au8, mysqlConnect_st.user_au8,
							   mysqlConnect_st.password_au8,
							   mysqlConnect_st.database_au8, 0, NULL, 0) == NULL)
		{
		    /*connect error*/
			fprintf(stdout,"\nMySQL connect error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));;
		}
		else
		{
			//build query string
			strcpy (queryStr_au8, "SELECT id, Name, Housecode, UnitCode, LastState FROM ");
			strcat (queryStr_au8, MYSQL_TABLE_NAME);
			strcat (queryStr_au8, " WHERE 1 LIMIT ");
			snprintf(tempStr_au8, MAX_QUERY_STRING_LENGTH, "%d", MAX_INPUT_PINS);
			strcat (queryStr_au8, tempStr_au8);
			//fprintf(stdout,"\n Query String: %s", queryStr_au8);
			//run querry
			if (mysql_query(mysqlConnect_st.conn_ptrst, queryStr_au8))
			{
				  /*connect error*/
				 fprintf(stdout,"\nMySQL query error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
			}
			else
			{
				  result_ptr = mysql_store_result(mysqlConnect_st.conn_ptrst);
				  if (result_ptr == NULL)
				  {
					 /*connect error*/
					 fprintf(stdout,"\nMySQL result ptr error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
				  }
				  else
				  {
					  numOfFields_u16 = mysql_num_fields(result_ptr);
					  if (numOfFields_u16 != 5)
					  {
						  fprintf(stdout,"\n Mysql Table is empty or structure not OK !");
					  }
					  else
					  {
						  while ((row_st = mysql_fetch_row(result_ptr)))
						  {
						      /*fprintf(stdout,"\n\nId: %s", row_st[0]);
						      fprintf(stdout,"\nName: %s", row_st[1]);
						      fprintf(stdout,"\nHouseCode: %s", row_st[2]);
						      fprintf(stdout,"\nUnitCode: %s", row_st[3]);
						      fprintf(stdout,"\nLastState: %s\n", row_st[4]);*/
						      /*convert id string to integer and range check*/
						      temp_lu32= strtol(row_st[0], (char **)NULL, 10);
						      //fprintf(stdout,"\n Converted ID:%d", temp_lu32);
						      if (temp_lu32>=MAX_NR_OF_LEDS)
						      {
						    	  //id is too big
						    	  fprintf(stdout,"\nId is too big: %d. Id shall be sorted and ascending...", temp_lu32);
						      }
						      else
						      {
							      /*convert state string to integer and range check*/
								  temp2_lu32= strtol(row_st[4], (char **)NULL, 10);
								  //fprintf(stdout,"\nConverted state: %d", temp2_lu32);
								  if (temp2_lu32>1)//state can be 0 or 1
								  {
									  fprintf(stdout,"\nState is invalid: It's value shall be boolean, but it's: %d.", temp2_lu32);
								  }
								  else
								  {
							    	  //set the leds
							    	  if (temp2_lu32)
							    	  {
							    		  setLedState(temp_lu32, TRUE);
							    	  }
							    	  else
							    	  {
							    		  setLedState(temp_lu32, FALSE);
							    	  }
								  }//state is ok
						      }
						  }//end while
					  }//end fields ok
					  mysql_free_result(result_ptr);
				  }//end result ptr NULL
			  }//end query error
			  //close connection
			  mysql_close(mysqlConnect_st.conn_ptrst);
		}//no connect error

	}//mysql  no  error

	ENTER_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
	refreshQuerryRunning_b = FALSE;
	EXIT_CRITICAL_SECTION(&mutexSemaphoreQueryDB_st);
	//fprintf(stdout,"\nRefresh Thread ended");
	return NULL;
}


/* *************************************************************************
 * 	   Init function of DB handling
	   This function will be called at init, after board and IO init

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
uint8_t getNumberOfConfiguredDevices(void)
{
	uint8_t nrOfResults_lu8 = 0;
	mysqlConnect_st.conn_ptrst = mysql_init(NULL);
	if (!mysqlConnect_st.conn_ptrst )
	{
	    /*connect error*/
		fprintf(stdout,"\nMySQL null pointer error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
	}
	else
	{
		if (mysql_real_connect(mysqlConnect_st.conn_ptrst, mysqlConnect_st.server_au8, mysqlConnect_st.user_au8,
							   mysqlConnect_st.password_au8,
							   mysqlConnect_st.database_au8, 0, NULL, 0) == NULL)
		{
		    /*connect error*/
			fprintf(stdout,"\nMySQL connect error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));;
		}
		else
		{
			//build query string
			strcpy (queryStr_au8, "SELECT id FROM ");
			strcat (queryStr_au8, MYSQL_TABLE_NAME);
			strcat (queryStr_au8, " WHERE 1 LIMIT ");
			snprintf(tempStr_au8, MAX_QUERY_STRING_LENGTH, "%d", MAX_INPUT_PINS);
			strcat (queryStr_au8, tempStr_au8);
			//fprintf(stdout,"\n Query String to get nr. of configured devices: %s", queryStr_au8);
			//run querry
			if (mysql_query(mysqlConnect_st.conn_ptrst, queryStr_au8))
			{
				  /*connect error*/
				 fprintf(stdout,"\nMySQL query error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
			}
			else
			{
				  result_ptr = mysql_store_result(mysqlConnect_st.conn_ptrst);
				  if (result_ptr == NULL)
				  {
					 /*connect error*/
					 fprintf(stdout,"\nMySQL result ptr error: %s\n", mysql_error(mysqlConnect_st.conn_ptrst));
				  }
				  else
				  {
					  nrOfResults_lu8 = mysql_num_rows(result_ptr);
					  mysql_free_result(result_ptr);
				  }//end result ptr NULL
			  }//end query error
			  //close connection
			  mysql_close(mysqlConnect_st.conn_ptrst);
		}//no connect error
	}//mysql  no  error
	return (nrOfResults_lu8);
}


/* *************************************************************************
 * 	   Init function of DB handling
	   This function will be called at init, after board and IO init

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void InitDB(void)
{
	strcpy (mysqlConnect_st.server_au8, MYSQL_HOST);
	strcpy (mysqlConnect_st.user_au8, MYSQL_USERNAME);
	strcpy (mysqlConnect_st.password_au8, MYSQL_PASSWORD);
	strcpy (mysqlConnect_st.database_au8, MYSQL_DB);
	strcpy (mysqlConnect_st.table_au8, MYSQL_TABLE_NAME);


	/*Semaphore init*/
	sem_init(&mutexSemaphoreQueryDB_st, 0, 1);      /* initialize mutex to 1 - binary semaphore */
	                                 /* second param = 0 - semaphore is local */
	sem_init(&mutexSemaphoreUpdateDB_st, 0, 1);      /* initialize mutex to 1 - binary semaphore */
	                                 /* second param = 0 - semaphore is local */


}


/* *************************************************************************
 *   Cleanup at shutdown

      @param[in]      none
      @return         none
 * *************************************************************************/
void CleanupDB(void)
{
	  /* wait for thread (if any) to  finish */
	pthread_join(pthreadRefreshFromDB_st, NULL);
	pthread_join(pthreadUpdateDB_st, NULL);

	/*destroy semaphores*/
	sem_destroy(&mutexSemaphoreQueryDB_st); /* destroy semaphore */
	sem_destroy(&mutexSemaphoreUpdateDB_st); /* destroy semaphore */

}
