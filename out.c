/*
 * out.c:
 *      Homey IO  Related.
  ***********************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//for sleep and stuff
#include <unistd.h>

#include "out.h"
#include "out-EXT.h"
//homey application
#include "homey-EXT.h"


#include <softPwm.h>
//piface board
#include <wiringPi.h>
#include <piFace.h>

/*GLOBAL VARIABLES*/



/*Contains the state of the led outputs*/
LedOut_st  outLeds_st[MAX_NR_OF_LEDS], outLedsOld_st[MAX_NR_OF_LEDS];

/*Input states*/
Button_st  inButtons_ast[MAX_NR_OF_BUTTONS];//current state
Button_st inButtonsOld_ast[MAX_NR_OF_BUTTONS];//old state

/*structure containing current menu state*/
Menu_st  currentMenu_st;
uint32_t menuAutoDeactivate_u32 = MENU_TIMER_INACTIVE;

uint8_t animationRunningTimer_u8 = 0;
uint8_t animationRunning_u8 = FALSE;


/* *************************************************************************
 * 	   Process Data
	   This function will do data processing

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void process_Buttons(void)
{
  uint8_t  forIdx_lu8;
  /*check if menu is active*/
  if (wasRisingEdgeOnButton(BUTTON_MENU))
  {
	  //fprintf(stdout,"Menu button pressed");
	  //toggle menu active on rising edge
	  currentMenu_st.menuActive_b =!currentMenu_st.menuActive_b;
	  if ( currentMenu_st.menuActive_b)
	  {
		  currentMenu_st.selectedItem_u8 = 0; //first one is active;
	  }
	  else
	  {
		  //do nothing
	  }
  }
  else
  {
	  //do not change
  }


  if (currentMenu_st.menuActive_b )
  {

	  if (menuAutoDeactivate_u32)
	  {
		  menuAutoDeactivate_u32--;
	  }
	  else
	  {
		  //auto deactivation timer elapsed (no button was pressed in MENU_TIMER_INACTIVE time)
		  setMenuInactive();
	  }

	  if (wasRisingEdgeOnButton(BUTTON_UP))
	  {
		  //button up pressed
		  menuAutoDeactivate_u32 = MENU_TIMER_INACTIVE;
		  if (
				  (currentMenu_st.selectedItem_u8<(MAX_NR_OF_LEDS-1))
				  &&
				  (currentMenu_st.selectedItem_u8<(numOfConfiguredDevices_u8-1))
			 )
		  {
			  currentMenu_st.selectedItem_u8++;
		  }
		  else
		  {
			  /*do not increase any more*/
		  }
		  //fprintf(stdout,"\nSelected Item: %d",currentMenu_st.selectedItem_u8);
	  }
	  else
	  {
		  if (wasRisingEdgeOnButton(BUTTON_DOWN))
		  {
			  //button down pressed
			  menuAutoDeactivate_u32 = MENU_TIMER_INACTIVE;
			  if (currentMenu_st.selectedItem_u8)
			  {
			  	 currentMenu_st.selectedItem_u8--;
			  }
			  else
			  {
				  /*do not decrease any more*/
			  }
			  //fprintf(stdout,"\nSelected Item: %d",currentMenu_st.selectedItem_u8);
		  }
		  else
		  {
			  if (wasRisingEdgeOnButton(BUTTON_OK))
			  {
				  //button ok pressed
				  menuAutoDeactivate_u32 = MENU_TIMER_INACTIVE;
				  /*activate relay. Update mysql and send heyu command*/
				  ActivateItem(currentMenu_st.selectedItem_u8, outLeds_st[currentMenu_st.selectedItem_u8].ledState_b);
			  }
			  else
			  {
				  //no button pressed
			  }
		  }
	  }
	  //menu is active
  }
  else
  {
	  //menu is not active
	  currentMenu_st.selectedItem_u8 = LED_NONE_SELECTED;
	  menuAutoDeactivate_u32 = MENU_TIMER_INACTIVE;
  }

  //update button selected status
  for (forIdx_lu8=0; forIdx_lu8< MAX_NR_OF_LEDS; forIdx_lu8++ )
  {
	  if (forIdx_lu8==currentMenu_st.selectedItem_u8)
	  {
		  outLeds_st[forIdx_lu8].ledBlink_b = 1;
	  }
	  else
	  {
		  outLeds_st[forIdx_lu8].ledBlink_b = 0;
	  }
  }
 #ifdef _DEBUG_ENABLED
 #endif
}


/* *************************************************************************
 * 	   Read Inputs
	   This function will Read the 4 button states

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void get_Inputs(void)
{
  uint8_t  forIdx_lu8, result_lu8=0;
  for (forIdx_lu8=0; forIdx_lu8< MAX_NR_OF_BUTTONS; forIdx_lu8++ )
  {
	  result_lu8 = digitalRead(PIFACE+forIdx_lu8);
	  inButtons_ast[forIdx_lu8].buttonState_b = !result_lu8; //buttons are active low
	  if (inButtons_ast[forIdx_lu8].buttonState_b != inButtonsOld_ast[forIdx_lu8].buttonState_b)
	  {
		  //reset debunce counter
		  inButtons_ast[forIdx_lu8].debounceTime_u8 = INPUT_DEBOUNCE_TIME;
	  }
	  else
	  {
		  if (!(inButtons_ast[forIdx_lu8].debounceTime_u8))
		  {
			 //debounce counter has reached 0
			  inButtons_ast[forIdx_lu8].buttonStateDeb_b = inButtons_ast[forIdx_lu8].buttonState_b;
		  }
		  else
		  {
			  //decrement debounce counter
			  inButtons_ast[forIdx_lu8].debounceTime_u8--;
		  }
	  }
	   //save old button state
	  if ((inButtons_ast[forIdx_lu8].buttonState_b) && (!inButtonsOld_ast[forIdx_lu8].buttonState_b))
	  {
		  /*set rising edge*/
		  inButtons_ast[forIdx_lu8].risingEdge_b = 1u;
	  }
	  else
	  {
		  inButtons_ast[forIdx_lu8].risingEdge_b = 0u;
	  }
	  if ((inButtons_ast[forIdx_lu8].buttonStateDeb_b) && (!inButtonsOld_ast[forIdx_lu8].buttonStateDeb_b))
	  {
		  /*set rising edge*/
		  inButtons_ast[forIdx_lu8].risingEdgeDeb_b = 1u;
	  }
	  else
	  {
		  inButtons_ast[forIdx_lu8].risingEdgeDeb_b = 0u;
	  }
	  inButtonsOld_ast[forIdx_lu8] = inButtons_ast[forIdx_lu8];
  }//end for
}



/* *************************************************************************
 * 	   Deactivate the menu
	   This is called after the x10 command has been sent

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void setMenuInactive( void )
{
	//deactivate menu
	currentMenu_st.menuActive_b = 0;
	currentMenu_st.selectedItem_u8 = LED_NONE_SELECTED;
	menuAutoDeactivate_u32 = MENU_TIMER_INACTIVE;
}

/* *************************************************************************
 * 	   Init function of IO handling
	   This function will be called at init, after board init

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void InitIO(void)
{
	    //add IO related init here if necessary
	    uint8_t  forIdx_lu8;

		// Always initialise wiringPi. Use wiringPiSys() if you don't need
		//      (or want) to run as root
		//wiringPiSetupSys();
		 wiringPiSetup();
		 //wiringPiSetupGpio();
		// Setup the PiFace board
		piFaceSetup(PIFACE);

		// Enable internal pull-ups, and init all outputs to 0

		for (forIdx_lu8 = 0 ; forIdx_lu8 < MAX_INPUT_PINS ; ++forIdx_lu8)
		{
			pullUpDnControl (PIFACE+forIdx_lu8, PUD_UP);
			/*simple animation to know then the process has started up*/
			digitalWrite (PIFACE+forIdx_lu8, HIGH) ;  // On
			usleep(40000);               // mS
			digitalWrite (PIFACE+forIdx_lu8, LOW) ;   // Off
			usleep(40000);
	 	}
		/*initalise software PWM driver*/

		for (forIdx_lu8 = 0 ; forIdx_lu8 < MAX_NR_OF_LEDS ; forIdx_lu8++)
		{
			softPwmCreate(PIFACE+forIdx_lu8,0,PWM_RANGE);
		}

		/*INITIALIZE SW PWM for status led */
		softPwmCreate(STATUS_LED_PIN, 0, PWM_STATUS_BLINK_RANGE);

}


/* *************************************************************************
 * 	   Set Outputs
	   This function will set the 8 led outputs

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void set_Outputs(void)
{
	uint8_t  forIdx_lu8;
	if (animationRunningTimer_u8)
	{
			animationRunningTimer_u8--;

	}//animation timer running
	else
	{
			if (animationRunning_u8)
			{
				//fprintf(stdout,"\nAnimation timer ended");
				//animation was running, but timer expired already
				//setup back the normal PWM range
				for (forIdx_lu8 = 0 ; forIdx_lu8 < MAX_NR_OF_LEDS ; forIdx_lu8++)
				{
					softPwmCreate(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),0,PWM_RANGE);
					if (outLeds_st[forIdx_lu8].ledState_b)
					{
						//led shall be active
						if (outLeds_st[forIdx_lu8].ledBlink_b)
						{
							//led shall blink
							softPwmWrite(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),PWM_ACTIVE_DUTY_CYCLE);
						}
						else
						{
							//led shall be steady on
							//digitalWrite(PIFACE + forIdx_lu8, 1);
							softPwmWrite(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),PWM_STEADY_ON);
						}
					}//led on
					else
					{
						//led shall be inactive
						if (outLeds_st[forIdx_lu8].ledBlink_b)
						{
							//led shall blink
							softPwmWrite(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),PWM_INACTIVE_DUTY_CYCLE);
						}
						else
						{
							//led shall be steady off
							//digitalWrite(PIFACE + forIdx_lu8, 0) ;
							softPwmWrite(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),PWM_STEADY_OFF);
						}
					}
				}//end for
				animationRunning_u8 = FALSE;


			}
			else
			{
				//do not set the PWM again
				//process all outputs normally
				for (forIdx_lu8=0; forIdx_lu8< MAX_NR_OF_LEDS; forIdx_lu8++ )
				{

					if (
							(outLeds_st[forIdx_lu8].ledState_b != outLedsOld_st[forIdx_lu8].ledState_b)
							||
							(outLeds_st[forIdx_lu8].ledBlink_b != outLedsOld_st[forIdx_lu8].ledBlink_b)
					   )
					{
						if (outLeds_st[forIdx_lu8].ledState_b)
						{
							//led shall be active
							if (outLeds_st[forIdx_lu8].ledBlink_b)
							{
								//led shall blink
								softPwmWrite(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),PWM_ACTIVE_DUTY_CYCLE);
							}
							else
							{
								//led shall be steady on
								//digitalWrite(PIFACE + forIdx_lu8, 1);
								softPwmWrite(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),PWM_STEADY_ON);
							}

						}
						else
						{
							//led shall be inactive
							if (outLeds_st[forIdx_lu8].ledBlink_b)
							{
								//led shall blink
								softPwmWrite(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),PWM_INACTIVE_DUTY_CYCLE);
							}
							else
							{
								//led shall be steady off
								//digitalWrite(PIFACE + forIdx_lu8, 0) ;
								softPwmWrite(PIFACE+((MAX_NR_OF_LEDS-1)-forIdx_lu8),PWM_STEADY_OFF);
							}

						}
						/*save old state*/
						outLedsOld_st[forIdx_lu8] = outLeds_st[forIdx_lu8];
					}
					else
					{
						//nothing has changed
					}
				}
			}
	}//end animation timer running

	//now set the new PWM value of the status led
	softPwmWrite(STATUS_LED_PIN, CalculatePWMDutyCycle());
}

/* *************************************************************************
 * 	   Init function of IO handling
	   This function will be called at init, after board init

	   @param[in]     - led_lu8 : which led to set
	   	   	   	      - value_b:  TRUE - led is on/blinking;  FALSE: - led is OFF
	   @return 		  none
 * *************************************************************************/
void setLedState(uint8_t led_lu8, uint8_t value_lb)
{
	outLeds_st[led_lu8].ledState_b = value_lb;
}


/* *************************************************************************
 * 	   This function will adjust  the HW PWM duty cycle  value to achieve
 * 	   a pulsing effect of a LED

	   @param[in]     none
	   @return 		  computed duty cycle
 * *************************************************************************/
uint16_t CalculatePWMDutyCycle(void)
{
	/*PWM duty cycle for pulsing led (status indicator)*/
	static uint16_t pwmDutyCycle_lu16 = STATUS_ANIMATION_MIN_DUTY_CYCLE;
	static uint8_t  pulseChgDirection = 1u ; //up= 1  down =0
	static uint8_t  holdupTMR_lu8 = HOLDUP_TIMER_VAL_OFF;
	if (holdupTMR_lu8)
	{
		holdupTMR_lu8--;
	}
	else
	{
		if (pulseChgDirection)
			{
				//up
				if (pwmDutyCycle_lu16<STATUS_ANIMATION_MAX_DUTY_CYCLE)
				{
					pwmDutyCycle_lu16++;
				}
				else
				{
					//change direction
					pulseChgDirection =0 ;
					pwmDutyCycle_lu16 = STATUS_ANIMATION_MAX_DUTY_CYCLE;
					holdupTMR_lu8 = HOLDUP_TIMER_VAL_ON;
				}
			}
			else
			{
				//down
				if (pwmDutyCycle_lu16>STATUS_ANIMATION_MIN_DUTY_CYCLE)
				{
					pwmDutyCycle_lu16--;
				}
				else
				{
					//change direction
					pulseChgDirection =1 ;
					pwmDutyCycle_lu16 = STATUS_ANIMATION_MIN_DUTY_CYCLE;
					holdupTMR_lu8 = HOLDUP_TIMER_VAL_OFF;
				}
			}
	}

	return (pwmDutyCycle_lu16);
}

/* *************************************************************************
 * 	   This function will init a 2s flashing of all LEDS. After timer expires,
 * 	   normal operation state will return automatically

	   @param[in]     none
	   @return 		  none
 * *************************************************************************/
void FlashAllLeds(void)
{
	uint8_t forIdx_lu8;
	//fprintf(stdout,"\n Flashing all leds");
	//simple flashing animation of all leds to have a visual feedback.
	if (animationRunning_u8)
	{
		//do nothing
	}
	else
	{
		//setup new PWM range
		for (forIdx_lu8 = 0 ; forIdx_lu8 < MAX_NR_OF_LEDS ; forIdx_lu8++)
		{
			softPwmCreate(PIFACE+forIdx_lu8,0,PWM_ANIMATION_RANGE);
			//led shall blink
			softPwmWrite(PIFACE+forIdx_lu8,PWM_ANIMATION_DUTY_CYCLE);
		}
		animationRunningTimer_u8 = MAX_ANIMATION_TIME_VAL;
		animationRunning_u8 = TRUE;
		//call this to set the outputs
		set_Outputs();

	}
}


/* *************************************************************************
 *   Cleanup at shutdown

      @param[in]      none
      @return         none
 * *************************************************************************/
void CleanupIO(void)
{
	uint8_t forIdx_lu8;
	//turn all outputs off
	//fprintf(stdout,"\n Turning all outputs off");
	for (forIdx_lu8 = 0 ; forIdx_lu8 < MAX_NR_OF_LEDS ; forIdx_lu8++)
	{
		softPwmWrite(PIFACE+forIdx_lu8,PWM_STEADY_OFF);
		//turn all outputs off
		digitalWrite (PIFACE+forIdx_lu8,0);

	}
	softPwmWrite(STATUS_LED_PIN,PWM_STEADY_OFF);
	//turn all outputs off
	digitalWrite (STATUS_LED_PIN,0);


}
