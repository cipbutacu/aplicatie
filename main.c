/*
 * homey.c:
 *      Homey Main Application.
  ***********************************************************************
 */


#include <softPwm.h>

//piface board
#include <wiringPi.h>
#include <piFace.h>
#include <libwebsockets.h>

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

static int callback_http(struct libwebsocket_context * this,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len)
{
    return 0;
}

static int callback_dumb_increment(struct libwebsocket_context * this,
                                   struct libwebsocket *wsi,
                                   enum libwebsocket_callback_reasons reason,
                                   void *user, void *in, size_t len)
{

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
            printf("connection established\n");
            break;
        case LWS_CALLBACK_RECEIVE: { // the funny part
            // create a buffer to hold our response
            // it has to have some pre and post padding. You don't need to care
            // what comes there, libwebsockets will do everything for you. For more info see
            // http://git.warmcat.com/cgi-bin/cgit/libwebsockets/tree/lib/libwebsockets.h#n597
            unsigned char *buf = (unsigned char*) malloc(LWS_SEND_BUFFER_PRE_PADDING + len +
                                                         LWS_SEND_BUFFER_POST_PADDING);

            int i;

            // pointer to `void *in` holds the incomming request
            // we're just going to put it in reverse order and put it in `buf` with
            // correct offset. `len` holds length of the request.
            for (i=0; i < len; i++) {
                buf[LWS_SEND_BUFFER_PRE_PADDING + (len - 1) - i ] = ((char *) in)[i];
            }

            // log what we recieved and what we're going to send as a response.
            // that disco syntax `%.*s` is used to print just a part of our buffer
            // http://stackoverflow.com/questions/5189071/print-part-of-char-array
            printf("received data: %s, replying: %.*s\n", (char *) in, (int) len,
                 buf + LWS_SEND_BUFFER_PRE_PADDING);

            // send response
            // just notice that we have to tell where exactly our response starts. That's
            // why there's `buf[LWS_SEND_BUFFER_PRE_PADDING]` and how long it is.
            // we know that our response has the same length as request because
            // it's the same message in reverse order.
            libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], len, LWS_WRITE_TEXT);

            // release memory back into the wild
            free(buf);
            break;
        }
        default:
            break;
    }


    return 0;
}

 struct libwebsocket_protocols protocols[] = {
    /* first protocol must always be HTTP handler */
    {
        "http-only",   // name
        callback_http, // callback
        0              // per_session_data_size
    },
    {
        "dumb-increment-protocol", // protocol name - very important!
        callback_dumb_increment,   // callback
        0                          // we don't use any per session data

    },
    {
        NULL, NULL, 0   /* End of list */
    }
};

int main (int argc, char **argv)
{
	// server url will be http://localhost:9000
	int port = 9000;
	const char *interface = NULL;
	struct libwebsocket_context *context;
	// we're not using ssl
	const char *cert_path = NULL;
	const char *key_path = NULL;
	// no special options
	int opts = 0;

	struct lws_context_creation_info info;

	memset(&info, 0, sizeof info);
	info.port = port;
	info.iface = interface;
	info.protocols = protocols;
	info.extensions = libwebsocket_get_internal_extensions();
	//if (!use_ssl) {
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	//} else {
	// info.ssl_cert_filepath = LOCAL_RESOURCE_PATH"/libwebsockets-test-server.pem";
	// info.ssl_private_key_filepath = LOCAL_RESOURCE_PATH"/libwebsockets-test-server.key.pem";
	//}
	info.gid = -1;
	info.uid = -1;
	info.options = opts;

	context = libwebsocket_create_context(&info);
	//------

  /*Initialize the Rapsberry board*/
  InitBoard();
  /*init sw variables*/
 // InitSW();

  //context = libwebsocket_create_context(port, interface, protocols,
   //                                     libwebsocket_internal_extensions,
   //                                     cert_path, key_path, -1, -1, opts);

     if (context == NULL) {
         fprintf(stderr, "libwebsocket init failed\n");
         return -1;
     }
     printf("starting server...\n");
     printf("starting server...\n");

  //entering main loops
  while(1)
  {
	  libwebsocket_service(context, 50);
	         // libwebsocket_service will process all waiting events with their
	         // callback functions and then wait 50 ms.
	         // (this is single threaded webserver and this will keep
	         // our server from generating load while there are not
	         // requests to process)

  }
  libwebsocket_context_destroy(context);
  return 0;
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
