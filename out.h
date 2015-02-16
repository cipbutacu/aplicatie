/*
 * out.h
 *
 *  Created on: Aug 22, 2013
 *      Author: Trex
 */

#ifndef OUT_H_
#define OUT_H_

//custom type definitions like TRUE, FALSE
#include "type-custom.h"

#include "out-EXT.h"


/*DEFINES*/

#define BUTTON_MENU		  3
#define BUTTON_DOWN		  1
#define BUTTON_UP		  2
#define BUTTON_OK		  0

/*no button is selected*/
#define LED_NONE_SELECTED	  MAX_NR_OF_LEDS

/*debounce time for switches  - each is read every 50ms*/
#define INPUT_DEBOUNCE_TIME 						3
#define MENU_BUTTON_LONGPRESS_TIME					60
#define MENU_BUTTON_TIMER_STOPPED					0xFF

/*menu is deactivate if no button is pressed for 1200*50 ms = 60 seconds*/
#define MENU_TIMER_INACTIVE  1200u

/*MACROS*/

/*returns true if there was a rising edge on the button*/
#define wasRisingEdgeOnButton(button)  (inButtons_ast[button].risingEdgeDeb_b ==1)



/* Structure containing the output state of the leds
 */
typedef struct outLed_stype
{
	uint8_t ledState_b:1; //ON or OFF
	uint8_t ledBlink_b:1; //BLinking or not
}LedOut_st;


/*Input buttons*/
typedef struct  inButton_stype
{
	uint8_t 	debounceTime_u8; 	//debunce counter
	uint8_t 	buttonState_b:1;  	//button state without debouncing
	uint8_t		buttonStateDeb_b:1; //button state after debouncing
	uint8_t		risingEdge_b:1;     //rising edge
	uint8_t		risingEdgeDeb_b:1;     //rising edge debounced
}Button_st;


typedef struct menu_styp
{
	uint8_t  menuActive_b:1;
	uint8_t  selectedItem_u8:4;  // 0-7  leds, 8 - none
	uint8_t  menuButton:1;			//current state of menu button
	uint8_t  menuButtonOld:1;		//old state of menu button
}Menu_st;


/*FUNCTION DECLARATIONS*/
/*retrieve target led duty cycle*/
uint16_t CalculatePWMDutyCycle(void);


#endif /* OUT_H_ */
