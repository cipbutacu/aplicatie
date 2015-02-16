/*
 * homey.c:
 *      Homey Main Application.
  ***********************************************************************

*/

#include "homey.h"
#include "homey-EXT.h"
#include "db-EXT.h"
#include "out-EXT.h"

//stdin , printf and stuff
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//for threads
#include <pthread.h>


/*GLOBAL VARIABLES*/

/*The number of configured devices. This will be used to limit the navigation through the menu*/
uint8_t numOfConfiguredDevices_u8 = 0;

/* *************************************************************************
 *       Activate device associated with
      the ID indicated by parameter

      @param[in]     selectedItem_lu8 : item that is selected
      	  	  	  	 stateToSet_lu8: state that will be set. TRUE = ON,  FALSE = OFF
      @return        none
 * *************************************************************************/
void ActivateItem(uint8_t selectedItem_lu8, uint8_t stateToSet_lu8)
{
   //UPDATE mysql
   //SEND HEYU COMMAND
   //Call any additional stuff before activating something
   if (!updateDBActDevice(selectedItem_lu8, stateToSet_lu8))
   {
	   	  //item could not be activated this time, as probably another query was already running
   }
   else
   {
	   //deactivate menu
	   setMenuInactive();
   }
}


/* *************************************************************************
 *   Cleanup at shutdown

      @param[in]      none
      @return         none
 * *************************************************************************/
void CleanupApp(void)
{

}

