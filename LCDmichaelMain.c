/***************************************************************************/
/*	main.c	--	Main program module for project							   */
/***************************************************************************/
/*	Author: 												               */
/*  Based upon code developed by Dion Moses of Digilent Inc.			   */
/***************************************************************************/
/*  Module Description: 												   */
/*	This program is an initial startup design for the Digilent			   */
/*	Robotic Starter Kit (RSK) with the Cerebot 32MX4 Microcontroller	   */
/*	board.  It uses two timers to drive two motors with output compare	   */
/*	modules.															   */
/*	This program is designed so that both PmodHB5 modules that come		   */
/*	standard with the Digilent RSK connect to JD of the Cerebot 32MX4.	   */
/*	Once the board and RSK are setup properly:							   */ 
/*	Note: reference design is for Rev. C of the Cerebot 32MX4		 	   */
/*																	  	   */
/***************************************************************************/
/*  Revision History:													   */
/*	 10/06/10 David Gnabasik: initial									   */
/*	 10/20/10 David Gnabasik: added SPI									   */
/*   11/20/2011 By Anh Nguyen, Justim Ohm: Modify to work with PmodCLP     */
/***************************************************************************/

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include <plib.h>
#include "stdtypes.h"
#include "config.h"
#include "MtrCtrl.h"
#include "LCD.h"
#include <stdio.h>
#include <string.h>


/* ------------------------------------------------------------ */
/*				Local Type Definitions							*/
/* ------------------------------------------------------------ */

#define		TCKPS22 			6
#define 	TCKPS21				5
#define 	TCKPS20				4

#define		TCKPS32 			6
#define 	TCKPS31				5
#define 	TCKPS30				4

#define 	BUTTON_1			1		// Button 1
#define 	BUTTON_2			2		// Button 2
#define		BOTH_MOTORS			0		// deal with both motors.
#define		LEFT_MOTOR			1		// deal with left motor.
#define		RIGHT_MOTOR			2		// deal with right motor.

#define 	TIME_FACTOR			1000	// 1 second; milliseconds to seconds


/* ------------------------------------------------------------ */
/*				Global Variables		*/
/* ------------------------------------------------------------ */
// configuration bit settings, Fcy=72MHz, Fpb=36MHz
#pragma config POSCMOD=XT, FNOSC=PRIPLL 
#pragma config FPLLIDIV=DIV_2, FPLLMUL=MUL_18, FPLLODIV=DIV_1
#pragma config FPBDIV=DIV_2, FWDTEN=OFF, CP=OFF, BWP=OFF


/* ------------------------------------------------------------ */
/*				Local Variables									*/
/* ------------------------------------------------------------ */

#define	stPressed	1
#define	stReleased	0
#define	cstMaxCnt	10 // number of consecutive reads required for
					   // the state of a button to be updated

struct btn {
	BYTE	stBtn;	// status of the button (pressed or released)
	BYTE	stCur;  // current read state of the button
	BYTE	stPrev; // previous read state of the button
	BYTE	cst;	// number of consecutive reads of the same button 
					// state
};

/* ------------------------------------------------------------ */
/*				Global Variables				                */
/* ------------------------------------------------------------ */

volatile	struct btn	btnBtn1;
volatile	struct btn	btnBtn2;
BYTE stBtn1;
BYTE stBtn2;
char ans[5];

/* ------------------------------------------------------------ */
/*				Forward Declarations							*/
/* ------------------------------------------------------------ */
void	DeviceInit(void);
void	myWaitMs(WORD ms);
void 	DelayUs( WORD tusDelay );
BOOL 	ButtonPressed(WORD button);
int 	ButtonPressed2(void);
void 	ClearAllLEDs(void);
void	Wait_Ms(WORD ms, WORD displayControl);

/* ------------------------------------------------------------ */
/*				Interrupt Service Routines						*/
/* ------------------------------------------------------------ */
/***	Timer5Handler
**
**	Parameters:
**		none
**
**	Return Value:
**		none
**
**	Errors:
**		none
**
**	Description:
**		Interrupt service routine for Timer 5 interrupt. Timer 5
**		is used to perform software debouncing of the on-board
**		buttons. It is also used as a time base for updating
**		the on-board LEDs and the Pmod8LD LEDs at a regular interval.
*/

void __ISR(_TIMER_5_VECTOR, ipl7) Timer5Handler(void)
{
	static	WORD tusLeds = 0;
	
	mT5ClearIntFlag();
	
	// Read the raw state of the button pins.
	btnBtn1.stCur = ( prtBtn1 & ( 1 << bnBtn1 ) ) ? stPressed : stReleased;
	btnBtn2.stCur = ( prtBtn2 & ( 1 << bnBtn2 ) ) ? stPressed : stReleased;
	
	// Update state counts.
	btnBtn1.cst = ( btnBtn1.stCur == btnBtn1.stPrev ) ? btnBtn1.cst + 1 : 0;
	btnBtn2.cst = ( btnBtn2.stCur == btnBtn2.stPrev ) ? btnBtn2.cst + 1 : 0;
	
	// Save the current state.
	btnBtn1.stPrev = btnBtn1.stCur;
	btnBtn2.stPrev = btnBtn2.stCur;
	
	// Update the state of button 1 if necessary.
	if ( cstMaxCnt == btnBtn1.cst ) {
		btnBtn1.stBtn = btnBtn1.stCur;
		btnBtn1.cst = 0;
	}
	
	// Update the state of button 2 if necessary.
	if ( cstMaxCnt == btnBtn2.cst ) {
		btnBtn2.stBtn = btnBtn2.stCur;
		btnBtn2.cst = 0;
	}

}

/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */
/***	main
**
**	Synopsis:
**		st = main()
**
**	Parameters:
**		none
**
**	Return Values:
**		does not return
**
**	Errors:
**		none
**
**	Description:
**		Main program module. Performs basic board initialization
**		and then enters the main program loop.
*/
char * intToString(long int num)
{
	int i = 0;
	int j = 0;
	int k=0;
	long int ones = 0;
	char temp[5]; 					//5=num digits in 32676
	

	while (num!=0)
	{
		ones = num%10;				//get current ones digit
		temp[i]= (char)(ones+48); 	//48=(int)'0';
		num = num/10;					//remove current ones digit
		i++;						//length of number
	}

	for(j = i-1; j >= 0; j--)
	{
		ans[k] = temp[j]; 			//reorder string correctly
		k++;
	}

	ans[i]= '\0'; 					//add null char for end of string
	return (char *)ans;
}


void initADC( int amask)
 {
 	AD1PCFG = amask; // select analog input pins
 	AD1CON1 = 0x00E0; // automatic conversion after sampling
 	AD1CSSL = 0; // no scanning required
 	AD1CON2 = 0; // use MUXA, use AVdd & AVss as Vref+/-
 	AD1CON3 = 0x1F3F; // Tsamp = 32 x Tad;
 	AD1CON1bits.ADON = 1; // turn on the ADC

 } //initADC

int readADC( int ch)
{
	AD1CHSbits.CH0SA = ch; // 1. select input channel
	AD1CON1bits.SAMP = 1; // 2. start sampling
 	while (!AD1CON1bits.DONE); // 3. wait conversion
 	return ADC1BUF0; // 4. read conversion result

} // readADC

/* ------------------------------------------------------------ */
/***	DeviceInit
**
**	Synopsis:
**		DeviceInit()
**
**	Parameters:
**		none
**
**	Return Values:
**		none
**
**	Errors:
**		none
**
**	Description:
**		Initializes on chip peripheral devices to the default
**		state.
*/

void DeviceInit() {

	// Configure LEDs as digital outputs.
	trisLed1Clr = ( 1 << bnLed1 );
	trisLed2Clr = ( 1 << bnLed2 );
	trisLed3Clr = ( 1 << bnLed3 );
	trisLed4Clr = ( 1 << bnLed4 );
	
	// Turn off the LEDs.
	prtLed1Clr	= ( 1 << bnLed1 );
	prtLed2Clr	= ( 1 << bnLed2 );
	prtLed3Clr	= ( 1 << bnLed3 );
	prtLed4Clr	= ( 1 << bnLed4 );

	// Configure left motor direction pin and set default direction.
	trisMtrLeftDirClr	= ( 1 << bnMtrLeftDir );
	prtMtrLeftDirClr	= ( 1 << bnMtrLeftDir );	// forward
	
	// Configure right motor diretion pin and set default direction.
	trisMtrRightDirClr	= ( 1 << bnMtrRightDir );	//modify for JD
	prtMtrRightDirSet	= ( 1 << bnMtrRightDir );	// forward

	// Configure Output Compare 2 to drive the left motor.
	OC2CON	= ( 1 << 2 ) | ( 1 << 1 );	// pwm
	OC2R	= dtcMtrStopped;
	OC2RS	= dtcMtrStopped;

	// Configure Output Compare 3.
	OC3CON = ( 1 << 3 ) | ( 1 << 2 ) | ( 1 << 1 );	// pwm
	OC3R	= dtcMtrStopped;
	OC3RS	= dtcMtrStopped;

	// Configure Timer 2.
	TMR2	= 0;									// clear timer 2 count
	PR2		= 9999;

	// Configure Timer 3.
	TMR3	= 0;
	PR3		= 9999;

	// Start timers and output compare units.
	T2CON		= ( 1 << 15 ) | ( 1 << TCKPS20 )|(1 << TCKPS21);		// timer 2 prescale = 8
	OC2CONSET	= ( 1 << 15 );	// enable output compare module 2
	OC3CONSET	= ( 1 << 15 );	// enable output compare module 3
	T3CON		= ( 1 << 15 ) | ( 1 << TCKPS31 ) | ( 1 << TCKPS30); //timer3 prescale = 8

	// Configure Timer 5.
	TMR5	= 0;
	PR5		= 99; // period match every 100 us
	IPC5SET	= ( 1 << 4 ) | ( 1 << 3 ) | ( 1 << 2 ) | ( 1 << 1 ) | ( 1 << 0 ); // interrupt priority level 7, sub 3
	IFS0CLR = ( 1 << 20);
	IEC0SET	= ( 1 << 20);
	
	// Start timers.
	T5CON = ( 1 << 15 ) | ( 1 << 5 ) | ( 1 << 4 ); // fTimer5 = fPb / 8
    
	// Enable multi-vector interrupts.
	INTEnableSystemMultiVectoredInt();
}

/* ------------------------------------------------------------ */
/***	Wait_ms
**
**	Synopsis:
**		Wait_ms(WORD)
**
**	Parameters:
**		WORD (range from 0 to 65535)
**
**	Return Values:
**		none
**
**	Errors:
**		none
**
**	Description:
**		Will wait for specified number of milliseconds.  Using a 
**		word variable allows for delays up to 65.535 seconds.  The value
**		in the for statement may need to be altered depending on how your
**		compiler translates this code into AVR assembly.  In assembly, it is
**		possible to generate exact delay values by counting the number of clock
**		cycles your loop takes to execute.  When writing code in C, the compiler
**		interprets the code, and translates it into assembly.  This process is 
**		notoriously inefficient and may vary between different versions of AVR Studio
**		and WinAVR GCC.  A handy method of calibrating the delay loop is to write a 
**		short program that toggles an LED on and off once per second using this 
**		function and using a watch to time how long it is actually taking to
**		complete. 
**
*/

void Wait_ms(WORD delay, WORD displayControl) 
{
	
	WORD i;

	if(displayControl == 0)
	{
		cmdLCD(0x00 | 0x00);
		putsLCD("Robot's starting");
		cmdLCD(0x80 | 0x40);
		putsLCD("Please wait.......");
		delay += 1500;
	}
	else
	{
		cmdLCD(0x00 | 0x00);
		putsLCD("Change direction..");
		cmdLCD(0x80 | 0x40);
		putsLCD("Please wait.......");
	}
	
	

	while(delay > 0)
	{
	
		for( i = 0; i < 375; i ++)
		{
			;;
		}

		delay -= 1;
	}
	
	cmdLCD(0x00 | 0x00);
}

/* ------------------------------------------------------------ */
/***	myWaitMs
**
**	Synopsis:
**		myWaitMs(WORD)
**
**	Parameters:
**		WORD (range from 0 to 65535)
**
**	Return Values:
**		none
**
**	Errors:
**		none
**
**	Description:
**		Will wait for specified number of milliseconds.  Using a 
**		word variable allows for delays up to 65.535 seconds.  The value
**		in the for statement may need to be altered depending on how your
**		compiler translates this code into AVR assembly.  In assembly, it is
**		possible to generate exact delay values by counting the number of clock
**		cycles your loop takes to execute.  When writing code in C, the compiler
**		interprets the code, and translates it into assembly.  This process is 
**		notoriously inefficient and may vary between different versions of AVR Studio
**		and WinAVR GCC.  A handy method of calibrating the delay loop is to write a 
**		short program that toggles an LED on and off once per second using this 
**		function and using a watch to time how long it is actually taking to
**		complete. 
*/
void myWaitMs( WORD tmsDelay )
{	
	while ( 0 < tmsDelay ) 
	{
		DelayUs(TIME_FACTOR);
		tmsDelay--;
	}
}

/* ------------------------------------------------------------ */
/***    DelayUs
**
**	Synopsis:
**		DelayUs(tusDelay)
**
**	Parameters:
**		tusDelay - the amount of time you wish to delay in microseconds
**
**	Return Values:
**      none
**
**	Errors:
**		none
**
**	Description:
**		This procedure delays program execution for the specified number
**      of microseconds. Please note that the minimal delay supported by
**		this routine is 3 microseconds.
**		
**	Note:
**		This routine is written with the assumption that the
**		system clock is 64 MHz.
*/
void DelayUs( WORD tusDelay )
{
	tusDelay -= 2;
	
    while ( 0 < tusDelay )
    {
	    asm volatile("nop");
        tusDelay--;
    }   
}   // DelayUs


/* ------------------------------------------------------------ */
/*  StopMotor(WORD motor)
**	Parameters:
**		BOTH_MOTORS, LEFT_MOTOR, RIGHT_MOTOR
**	Return Values:
**		Return TRUE if imported motor is stopped; else FALSE.
**	Errors:
**		none
**	Description:
**		Stop a particular motor.
*
BOOL StopMotor(WORD motor)	
{
	MtrCtrlStop();  	
	UpdateMotors(); 
	myWaitMs(0x0600);	// <<< was 0x0A00 = 2560
	return fTrue;
}*/

/* ------------------------------------------------------------ */
/*	ButtonPressed(WORD)
**	Parameters:
**		WORD: expected button # (1, 2)
**	Return Values:
**		Imported button value has been pressed. 
**	Errors:
**		none
**	Description:
**		Uses interrupts: ???
**		Spin here until correct button pressed.
*/
BOOL ButtonPressed(WORD button)	
{	
//	BYTE	stBtn1;
//	BYTE	stBtn2;

	INTDisableInterrupts();
	stBtn1 = btnBtn1.stBtn;
	stBtn2 = btnBtn2.stBtn;
	INTEnableInterrupts();

	if (button == BUTTON_1) {
		while (((stReleased == btnBtn1.stBtn) && (stReleased == btnBtn2.stBtn)) ||
		  ((stPressed == btnBtn1.stBtn) && (stPressed == btnBtn2.stBtn))    ||
		  ((stReleased == btnBtn1.stBtn) && (stPressed == btnBtn2.stBtn))) { }
	}

	if (button == BUTTON_2) {
		while (((stReleased == btnBtn1.stBtn) && (stReleased == btnBtn2.stBtn)) ||
		  ((stPressed == btnBtn1.stBtn) && (stPressed == btnBtn2.stBtn))    ||
		  ((stPressed == btnBtn1.stBtn) && (stReleased == btnBtn2.stBtn))) { }
	}

	return fTrue;	
}


int ButtonPressed2(void)	
{	
	BYTE	stBtn1;
	BYTE	stBtn2;

	INTDisableInterrupts();
	stBtn1 = btnBtn1.stBtn;
	stBtn2 = btnBtn2.stBtn;
	INTEnableInterrupts();

    while(1)
	{
		if((stPressed == btnBtn1.stBtn) && (stReleased == btnBtn2.stBtn)) {
            Delayms(400);
			return 1;
		}
		if((stReleased == btnBtn1.stBtn) && (stPressed == btnBtn2.stBtn)) {
            Delayms(400);
			return 2;
		}
	}
}

/* ------------------------------------------------------------ */
/*  ClearAllLEDs(void)
**	Parameters:
**		none
**	Return Values:
**		none
**	Errors:
**		none
**	Description:
**		Clear All LEDs
*/
void ClearAllLEDs(void)
{
	prtLed1Clr	= ( 1 << bnLed1 );
	prtLed2Clr	= ( 1 << bnLed2 );
	prtLed3Clr	= ( 1 << bnLed3 );
	prtLed4Clr	= ( 1 << bnLed4 );
}

/************************************************************************/
/*  main()
**	Parameters:
**		none
**	Return Values:
**		does not return
**	Errors:
**		none
**	Description:
**		Main program module. Performs basic board initialization
**		and then enters the main program loop.
*/
int main(void) 
{
   	// initializations
   	initADC(4);
   	initLCD();
   	DeviceInit();
	INTEnableSystemMultiVectoredInt();

   	// variables
   	int index = 0;
   	int btnpressed;
   	char s[8];
	WORD delay;
	unsigned int bwdCount = 0;
	unsigned int fwdCount = 0;
	unsigned int direction = 0;
	unsigned int backward = 0;
 	unsigned int forward = 0;
	char battery[20] = "Batt life = ";
	unsigned int control = 0;
	unsigned int control1 = 0;
	WORD startUp = 0;

	//strcat(battery, intToString((readADC(8))));
	strcat(battery, intToString((readADC(8) / 4) - 6));
	strcat(battery, "%");

	cmdLCD(0x00 | 0x00);
	putsLCD("Cerebot 32MX4");
	cmdLCD(0x80 | 0x40);
	putsLCD(battery);
	Delayms(3500);

   while(index != 101)
   {	
		clrLCD();
		putsLCD("Loading Data");
		cmdLCD(0x80 | 0x40);	
   	 	sprintf( s, "%2d", index);
      	putsLCD( s);
		putLCD( '%');

      	// draw bar
      	drawProgressBar( index, 100, HLCD-3);
      	index++;

      	// slow down the action
      	Delayms( 50);
	

   } // main loop

	Delayms(2500);

//////////////////////
//---- MAIN LOOP ----
//////////////////////
   while(1)
   {
   		// Write to LCD
      	clrLCD();
   		putsLCD("BTN #1:Metronome");
   		cmdLCD(0x80 | 0x40); // Display text on second row
   		putsLCD("BTN #2:Team");
      	btnpressed = ButtonPressed2();


//---------------------------
// --- Button #1 Pressed ---
// --- Execute program 1 ---
//---------------------------
   	if (btnpressed == 1)	
	{
   	
		while(fTrue)
		{
			if(control == 0)
			{
		  		if(control == 0)
				{
					cmdLCD(0x00 | 0x00);
					putsLCD("Cerebot 32MX4");
					cmdLCD(0x80 | 0x40);
					putsLCD(battery);
					delay = 2000;

					while(delay > 0)
					{	
						INTDisableInterrupts();
						stBtn1 = btnBtn1.stBtn;
						stBtn2 = btnBtn2.stBtn;
						INTEnableInterrupts();

						if(!((stPressed == stBtn2) && (stReleased == stBtn1)) || !((stPressed == stBtn1) && (stReleased == stBtn2)))
						{
							WORD i;
							for( i = 0; i < 375; i ++)
							{
								INTDisableInterrupts();
								stBtn1 = btnBtn1.stBtn;
								stBtn2 = btnBtn2.stBtn;
								INTEnableInterrupts();

								if(((stPressed == stBtn2) && (stReleased == stBtn1)) || ((stPressed == stBtn1) && (stReleased == stBtn2)))
								{
									break;
								}
							}

							delay -= 1;
						}
						else
						{
							control = 1;
							break;
						}
					}	
				}			
			
				if(control == 0)
				{
					cmdLCD(0x00 | 0x00);       //instructions for metronome
					putsLCD("Press BTN1 2X");
					cmdLCD(0x80 | 0x40);
					putsLCD("to set tempo");
					delay = 2200;

					while(delay > 0)
					{
						INTDisableInterrupts();
						stBtn1 = btnBtn1.stBtn;
						stBtn2 = btnBtn2.stBtn;
						INTEnableInterrupts();

						if(!((stPressed == stBtn2) && (stReleased == stBtn1)) || !((stPressed == stBtn1) && (stReleased == stBtn2)))
						{
							WORD i;
							for( i = 0; i < 375; i ++)
							{
								INTDisableInterrupts();
								stBtn1 = btnBtn1.stBtn;
								stBtn2 = btnBtn2.stBtn;
								INTEnableInterrupts();

								if(((stPressed == stBtn2) && (stReleased == stBtn1)) || ((stPressed == stBtn1) && (stReleased == stBtn2)))
								break;
							}

							delay -= 1;
						}
						else
						{
							control = 1;
							break;
						}
					}
				}
			}

			INTDisableInterrupts();
			stBtn1 = btnBtn1.stBtn;
			stBtn2 = btnBtn2.stBtn;
			INTEnableInterrupts();

			// Stop motor
			if ((stPressed == stBtn2) && (stReleased == stBtn1))
			{
				OC2R	= 0;
				OC2RS	= 0;
				OC3R	= 0;
				OC3RS	= 0;
			
				cmdLCD(0x00 | 0x00);
				putsLCD("The robot has");
				cmdLCD(0x80 | 0x40);
				putsLCD("been stopped");
			
				WORD delay = 4000;

				while(delay > 0)
				{
					WORD i;
					for( i = 0; i < 375; i ++)
					{
						;;
					}

					delay -= 1;

				}
			
				if(fwdCount == 0 && bwdCount == 1)
				{
					bwdCount = 0;
					fwdCount = 1;
				}
			
				if(bwdCount == 1)
				{
					fwdCount = 1;
				}
			
				if(bwdCount > fwdCount)
				{
					int temp;
					temp = bwdCount;
					bwdCount = fwdCount;
					fwdCount = temp;
				}

				char fwdValue[20] = "No. Of Fwd: ";

				if(fwdCount == 0)
				{
					strcat(fwdValue, "0");
				}
				else
				{
					strcat(fwdValue, intToString(fwdCount));
				}
			
				char bwdValue[20] = "No. Of Bwd: ";

				if(bwdCount == 0)
				{
					strcat(bwdValue, "0");
				}
				else
				{	
					strcat(bwdValue, intToString(bwdCount));
				}
			
				cmdLCD(0x00 | 0x00);
				putsLCD(fwdValue);
				cmdLCD(0x80 | 0x40);
				putsLCD(bwdValue);
			
				delay = 16000;

				while(delay > 0)
				{
					WORD i;
					for( i = 0; i < 375; i ++)
					{
						;;
					}

					delay -= 1;

				}

				control = 0;
				bwdCount = 0;
				fwdCount = 0;
				direction = 0;
				backward = 0;
 				forward = 0;
				char battery[20] = "Battery = ";
				control = 0;
				control1 = 0;
				startUp = 0;	

			}
			//configure OCR to go forward or backward
			else if((stPressed == stBtn1) && (stReleased == stBtn2))
			{	

				control = 1;
			}       
 		}     
      }
//---------------------------
// --- Button #2 Pressed ----
//---- Execute program 2 ----
//---------------------------
	ClearAllLEDs();

	if (btnpressed == 2)
	{
   		//int run = 1; // stay inside the menu
   		//while( run == 1) 
		{
   			// Clear the previous messages on the LCD
   			clrLCD();
        	ClearAllLEDs();
        	prtLed1Set = (1 << bnLed1);
   			    clrLCD();
   				putsLCD("David Chau");
   				cmdLCD(0x80 | 0x40); // Display text on second row
   				putsLCD("James Brayton");
                myWaitMs(2500);

                clrLCD();
   				putsLCD("Andrew Hoyle");
                //myWaitMs(500);
                cmdLCD(0x80 | 0x40); // Display text on second row
   				putsLCD("Michael Dewar");
                myWaitMs(2500);
   		}
   	}
    
      	clrLCD();
		// Write to LCD
   		putsLCD("Main Menu");
   		myWaitMs(400);
      	putsLCD(".");
   		myWaitMs(400);
      	putsLCD(".");
   		myWaitMs(400);
      	putsLCD(".");
   		myWaitMs(400);
   	}

	exit(0);

	
}  //end main

