/*
 * out-EXT.h
 *
 *  Created on: Aug 22, 2013
 *      Author: Trex
 */

#ifndef OUT_EXT_H_
#define OUT_EXT_H_

/*DEFINES*/

// Use 200 as the pin-base for the PiFace board, and pick a pin
#define PIFACE  200

// for the LED that's not connected to a relay
#define LEDBASE     (PIFACE+2)

/*PWM blinking duty cycles*/
#define PWM_RANGE			    4000
#define PWM_ACTIVE_DUTY_CYCLE   2700
#define PWM_INACTIVE_DUTY_CYCLE 2000
#define PWM_STEADY_ON			4000
#define PWM_STEADY_OFF			0

//for animation
#define PWM_ANIMATION_DUTY_CYCLE 200
#define PWM_ANIMATION_RANGE	     400

//for led status
#define PWM_STATUS_BLINK_RANGE		200
//all leds blink as confirmation animation timer
#define MAX_ANIMATION_TIME_VAL 7u //0.7 seconds
#define STATUS_ANIMATION_MAX_DUTY_CYCLE  4u  //around 70% to prevent too much brightness
#define STATUS_ANIMATION_MIN_DUTY_CYCLE  0u


#define HOLDUP_TIMER_VAL_ON 30u //wait 25*100ms = 2.5s
#define HOLDUP_TIMER_VAL_OFF 100u //wait 20*100ms = 10s


#define STATUS_LED_PIN		4  ///Pin (GPIO23)
//DO NOT USE HW PWM PIN, it's wired to IR receiver




/*FUNCTION-LIKE MACROS*/
extern void setLedState(uint8_t led_lu8, uint8_t value_lb);

/*FUNCTION DECLARATIONS*/

/*IO init*/
extern void InitIO(void);
/*get switches*/
extern void get_Inputs(void);
/*get switches*/
extern void set_Outputs(void);
/*Process the data*/
extern void process_Buttons(void);
/*deactivate menu*/
extern void setMenuInactive(void);
/*simple LED flashing animation*/
void FlashAllLeds(void);
/*cleanup IO*/
extern void CleanupIO(void);
#endif /* OUT_EXT_H_ */
