/******************************************************************************
 * Author:      Andrew Hoyle                                              	  *
 * Project:		Metronome/Battery life display                                *
 * Date:        12/8/12														  *
 * Notes:		-this uses the pre-existing Simon Says functions for		  *
 *				 debouncing, and makes a metronome allowing the user to       *
 *				 tap the tempo desired.  The BPM is displayed on the LCD      *
 *				 screen and the tempo is shown with LEDs.		              *
 ******************************************************************************/

// #include all necessary standard and user-defined libraries
#include <plib.h>
#include <string.h>
#include "config.h"
#include "stdtypes.h"
#include "LCD.h"
#include <stdio.h>

/* ------------------------------------------------------------ */
/*				Local Type Definitions							*/
/* ------------------------------------------------------------ */
#define		TCKPS22 			6			// Timer constants
#define 	TCKPS21				5
#define 	TCKPS20				4
#define		TCKPS32 			6
#define 	TCKPS31				5
#define 	TCKPS30				4
#define 	SIGNAL_ERROR       -1			// there's a problem
#define 	SIGNAL_RESET        0			// clear the LEDs
#define 	SIGNAL_BUTTON1      1			// please press button 1
#define 	SIGNAL_BUTTON       2			// please press button 2
#define 	SIGNAL_ROOT			3			// a root has been found
#define 	SIGNAL_FINISHED     4			// we're done!
#define 	BUTTON1				1			//
#define 	BUTTON2				2			//
#define 	TIME_FACTOR			1000		// 1 second; milliseconds to seconds
#define 	LED_BLINK_COUNTER   4
#define 	MAX_NUMBER 			15			// 4 LEDs can display this much; used in DisplayRandomLEDsequence().
#define		stPressed			1			// button state: pressed
#define		stReleased			0			// button state: released
#define		cstMaxCnt			10			// number of consecutive reads required for the state 
											//of a button to be updated (implicit debouncing)

/* ------------------------------------------------------------ */
/*				Configuration Pragmas							*/
/* ------------------------------------------------------------ */
// See http://www.microchip.com/forums/m394790-print.aspx for discussion on setting clock speed.
// Configuration Bit settings (written to Flash)
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV) = CPU FREQUENCY = 80000000L
// 80 MHz requires:#pragma FPLLIDIV = DIV_2, FPLLMUL = MUL_20, FPLLODIV = DIV_1
// PBCLK = 80 MHz
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF

#ifndef OVERRIDE_CONFIG_BITS
	#pragma config FPLLIDIV = DIV_2			// PLL Input Divider
	#pragma config FPLLMUL  = MUL_16  		// was MUL_24
	#pragma config FPLLODIV = DIV_1			// PLL Output Divider
 	#pragma config FPBDIV   = DIV_8			// Peripheral Clock divisor; was DIV_8
	#pragma config ICESEL   = ICS_PGx2		// ICE/ICD Comm Channel Select
	#pragma config BWP      = OFF			// Boot Flash Write Protect
	#pragma config CP       = OFF			// Code Protect
	#pragma config FNOSC    = PRIPLL		// Oscillator Selection
	#pragma config FSOSCEN  = OFF			// Secondary Oscillator Enable
	#pragma config IESO     = OFF			// Internal/External Switch-over
	#pragma config POSCMOD  = HS			// Primary Oscillator
	#pragma config OSCIOFNC = OFF			// CLKO Enable
	#pragma config FCKSM    = CSDCMD		// Clock Switching & Fail Safe Clock Monitor
	#pragma config WDTPS    = PS1			// Watchdog Timer Postscale
	#pragma config FWDTEN   = OFF			// Watchdog Timer
	#pragma config UPLLIDIV = DIV_2			// USB PLL Input Divider
	#pragma config UPLLEN   = OFF			// USB PLL Enabled
	#pragma config PWP      = OFF			// Program Flash Write Protect
	#pragma config DEBUG    = OFF			// Debugger Enable/Disable
#endif

#define SYS_FREQ 				(80000000L)

/* ------------------------------------------------------------ */
/*				Local Structures								*/
/* ------------------------------------------------------------ */
struct btn {
	BYTE	stBtn;	// status of the button (pressed or released)
	BYTE	stCur;  // current read state of the button
	BYTE	stPrev; // previous read state of the button
	BYTE	cst;	// number of consecutive reads of the same button state
};

//new variables for Metronome project-------------------------------------------------

volatile struct btn	btnBtn1;
volatile struct btn	btnBtn2;

volatile unsigned int timerCount = 0;
volatile unsigned int tempo = 0;
BOOL btn2Edge = fFalse;
BOOL btn1Edge = fFalse;
const unsigned int allowedTime = 4000;			//should be in ms b/c TMR1 resolution
BOOL stillHaveTime = fTrue;
const int blinkyLength = 100;
unsigned int tempCount;



//LCD screen new variables------------------------------------------------------------
char ans[5];

//old variables for Simon Says assignment
WORD BLINK_INTERVAL		= 200;		// milliseconds; used in SignalStatus(), DisplaySuccess().
WORD DISPLAY_INTERVAL   = 1000;		// milliseconds; used in DisplayRandomLEDsequence().
WORD DIFFICULTY 		= 3;		//RANGE 1 TO MAX_SEQUENCES.
									// Used in DisplayRandomLEDsequence(), AcceptInput().
size_t MAX_SEQUENCES    = 3; 		// used in main()
WORD RESPONSE_TIME		= 2000;		// milliseconds; used in AcceptInput().
WORD REFLEX_TIME        = 500;      //ms, used for AcceptInput()

/* ------------------------------------------------------------ */
/*				Forward Declarations / Public interface			*/
/* ------------------------------------------------------------ */
// old...
void	DeviceInit(void);
void	Wait_ms(WORD ms);
void 	DelayUs( WORD tusDelay );
void 	ClearAllLEDs(void);
BOOL 	SignalStatus(WORD status);
// new...
void InitializeButtons();
WORD ButtonPressed();
void DisplaySuccess( BOOL success );
void DisplayRandomLEDsequence(int *array);
void AcceptInput(int *array);
//LCD...
void initADC( int amask);
int readADC( int ch);
char * intToString(long int num);

// ISRs ---------------------------------------------------

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

#define PB_DIV         		8
#define PRESCALE       		256
#define TOGGLES_PER_SEC		1000			//resolution
#define T1_TICK       		(SYS_FREQ/PB_DIV/PRESCALE/TOGGLES_PER_SEC)

void __ISR(_TIMER_1_VECTOR, ipl2) Timer1Handler(void)
{
    // clear the interrupt flag
    mT1ClearIntFlag();

	//increment the timer
	++timerCount;
}

// new metronome functions -------------------------------------------------

//for LCD display
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

//for battery life display
void initADC( int amask)
 {
 	AD1PCFG = amask;		// select analog input pins
 	AD1CON1 = 0x00E0;		// automatic conversion after sampling
 	AD1CSSL = 0;			// no scanning required
 	AD1CON2 = 0;			// use MUXA, use AVdd & AVss as Vref+/-
 	AD1CON3 = 0x1F3F;		// Tsamp = 32 x Tad;
 	AD1CON1bits.ADON = 1;	// turn on the ADC

 }

int readADC( int ch)
{
	AD1CHSbits.CH0SA = ch;		// 1. select input channel
	AD1CON1bits.SAMP = 1;		// 2. start sampling
 	while (!AD1CON1bits.DONE);	// 3. wait conversion
 	return ADC1BUF0;			// 4. read conversion result
}

//put initial values into volatile button structs
void InitializeButtons() {
	// Initialize the state of button 1.
	btnBtn1.stBtn 	= stReleased;
	btnBtn1.stCur 	= stReleased;
	btnBtn1.stPrev 	= stReleased;
	btnBtn1.cst		= 0;

	// Initialize the state of button 2.
	btnBtn2.stBtn 	= stReleased;
	btnBtn2.stCur 	= stReleased;
	btnBtn2.stPrev 	= stReleased;
	btnBtn2.cst		= 0;
}

//configure timers, LEDs
void DeviceInit() {
	InitializeButtons();

	//for LCD
	initADC(4);
   	initLCD();	

	// Configure onboard buttons as inputs.
	trisBtn1Set = ( 1 << bnBtn1 );
	trisBtn2Set = ( 1 << bnBtn2 );

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

	// Configure Timer 5.
	TMR5	= 0;
	PR5		= 99; // period match every 100 us
	//interrupt priority level 7, sub 3
	IPC5SET	= ( 1 << 4 ) | ( 1 << 3 ) | ( 1 << 2 ) | ( 1 << 1 ) | ( 1 << 0 );
	IFS0CLR = ( 1 << 20);
	IEC0SET	= ( 1 << 20);

	//start TMR5
	T5CON = ( 1 << 15 ) | ( 1 << 5 ) | ( 1 << 4 ); // fTimer5 = fPb / 8

	//TMR1 stuff
	SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_256, T1_TICK);
    ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);

	// Enable multi-vector interrupts.
	INTEnableSystemMultiVectoredInt();
}

//calls assembly NOP function for debouncing
void Wait_ms( WORD tmsDelay )
{
	while ( 0 < tmsDelay ) {
		DelayUs(TIME_FACTOR);
		tmsDelay--;
	}
}

//assembly NOP delay for button debouncing
void DelayUs( WORD tusDelay )
{
	tusDelay -= 2;

    while ( 0 < tusDelay )
    {
        asm volatile("nop");
        tusDelay--;
    }
}

void ClearAllLEDs(void)
{
	prtLed1Clr	= ( 1 << bnLed1 );
	prtLed2Clr	= ( 1 << bnLed2 );
	prtLed3Clr	= ( 1 << bnLed3 );
	prtLed4Clr	= ( 1 << bnLed4 );
}

BOOL SignalStatus(WORD status)
{
	switch (status) {
		case SIGNAL_ERROR : { // blink all LEDs
			WORD i = 0;
			for (i = 0; i < LED_BLINK_COUNTER; i++) {
				prtLed1Set = (1 << bnLed1);
				prtLed2Set = (1 << bnLed2);
				prtLed3Set = (1 << bnLed3);
				prtLed4Set = (1 << bnLed4);
				Wait_ms(BLINK_INTERVAL);
				ClearAllLEDs();
				Wait_ms(BLINK_INTERVAL);
			}
			break;
		}

		case SIGNAL_RESET :
			ClearAllLEDs();
			break;

		case SIGNAL_BUTTON1 : { // steady 1 LED
			prtLed1Set = (1 << bnLed1);
			break;
		}

		case SIGNAL_BUTTON2 : { // steady 2 LEDs
			//prtLed1Set = (1 << bnLed1);
			prtLed2Set = (1 << bnLed2);
			break;
		}

		case SIGNAL_ROOT : { // steady 3 LEDs
			prtLed1Set = (1 << bnLed1);
			prtLed2Set = (1 << bnLed2);
			prtLed3Set = (1 << bnLed3);
			break;
		}

		case SIGNAL_FINISHED: { // steady 4 LEDs
			prtLed1Set = (1 << bnLed1);
			prtLed2Set = (1 << bnLed2);
			prtLed3Set = (1 << bnLed3);
			prtLed4Set = (1 << bnLed4);
		break;
		}

		default : return fFalse;
	}
	return fTrue;
}

// *************** NEW FUNCTIONS ***************

//Indicates success by blinking all LEDs rapidly n 
// times and failure by running individual LEDs in sequence 3 times.
void DisplaySuccess( BOOL success )
{
  WORD i,j;
  SignalStatus( SIGNAL_RESET );

  if ( success )
  {
     SignalStatus( SIGNAL_ERROR );
  }
  else // failure
    for (i = 0; i < 2; i++)
    {
      for (j = 1; j <= 4; j++)
      {
		SignalStatus( j );
        Wait_ms(BLINK_INTERVAL);
      }
	  SignalStatus( SIGNAL_RESET );
    }
  SignalStatus( SIGNAL_RESET );
}

// Display random set of LEDs and return those values in imported array.
void DisplayRandomLEDsequence(int *array)
{
	WORD i;
	// generate random numbers from 1 to 15
    for (i = 0; i < DIFFICULTY; i++)
	{
		do { // do not accept zero.
                    array[i] = rand() % MAX_NUMBER;
		} while (array[i] <= 0);

            // display values as (binary) LEDs
		switch (array[i])
		{
		  case 0 : 	break;
		  case 1 : 	prtLed1Set = (1 << bnLed1);		// 1
				break;
		  case 2 : 	prtLed2Set = (1 << bnLed2);		// 2
				break;
		  case 3 : 	prtLed1Set = (1 << bnLed1);		// 1
				prtLed2Set = (1 << bnLed2);			// 2
				break;
		  case 4 : 	prtLed3Set = (1 << bnLed3);		// 4
				break;
		  case 5 : 	prtLed3Set = (1 << bnLed3);		// 4
				prtLed1Set = (1 << bnLed1);			// 1
				break;
		  case 6 : 	prtLed3Set = (1 << bnLed3);		// 4
				prtLed2Set = (1 << bnLed2);			// 2
				break;
		  case 7 : 	prtLed3Set = (1 << bnLed3);		// 4
				prtLed2Set = (1 << bnLed2);			// 2
				prtLed1Set = (1 << bnLed1);			// 1
				break;
		  case 8 : 	prtLed4Set = (1 << bnLed4);		// 8
				break;
		  case 9 : 	prtLed4Set = (1 << bnLed4);		// 8
				prtLed1Set = (1 << bnLed1);			// 1
				break;
		  case 10:	prtLed4Set = (1 << bnLed4);		// 8
				prtLed2Set = (1 << bnLed2);			// 2
				break;
		  case 11:	prtLed4Set = (1 << bnLed4);		// 8
				prtLed2Set = (1 << bnLed2);			// 2
				prtLed1Set = (1 << bnLed1);			// 1
				break;
		  case 12:	prtLed4Set = (1 << bnLed4);		// 8
				prtLed3Set = (1 << bnLed3);			// 4
				break;
		  case 13:	prtLed4Set = (1 << bnLed4);		// 8
				prtLed3Set = (1 << bnLed3);			// 4
				prtLed1Set = (1 << bnLed1);			// 1
				break;
		  case 14:	prtLed4Set = (1 << bnLed4);		// 8
				prtLed3Set = (1 << bnLed3);			// 4
				prtLed2Set = (1 << bnLed2);			// 2
				break;
		  case 15:	prtLed4Set = (1 << bnLed4);		// 8
				prtLed3Set = (1 << bnLed3);			// 4
				prtLed2Set = (1 << bnLed2);			// 2
				prtLed1Set = (1 << bnLed1);			// 1
				break;
		} // switch
		Wait_ms(DISPLAY_INTERVAL);
		ClearAllLEDs();
	}	// for (i)
	Wait_ms(DISPLAY_INTERVAL);
}

/* ------------------------------------------------------------ */

//only returns 1, 2 or 3
WORD ButtonPressed()
{
	BYTE	stBtn1 = 0;
	BYTE	stBtn2 = 0;

	//InitializeButtons();
	while ((stBtn1 + stBtn2) < 1)
	{
		INTDisableInterrupts();
		stBtn1 = btnBtn1.stBtn;
		stBtn2 = btnBtn2.stBtn;
		INTEnableInterrupts();
	}

	if ((stPressed == stBtn1) && (stPressed == stBtn2))
		return BUTTON1 + BUTTON2;
	if (stPressed == stBtn1)
		return BUTTON1;
	if (stPressed == stBtn2)
		return BUTTON2;
	return 0;
}

/* ------------------------------------------------------------ */
// With 4 LEDS, can input from 1 to 15 using button1 as a counter,
// and button2 as "enter"
//uses REFLEX_TIME as "debouncing" so user doesn't accidentally
// enter a larger number
void AcceptInput(int *array)
{
    int but1count = 0, but2count = 0;

    while(but2count < DIFFICULTY)
    {
        but1count = 0;
        while(ButtonPressed() != 2)
        {
            if(ButtonPressed() == 1)
            {
                but1count = but1count + 1;
                Wait_ms(REFLEX_TIME);
            }
        }
        array[but2count] = but1count;
        but2count = but2count + 1;

        Wait_ms(REFLEX_TIME);
    }
}

/* ------------------------------------------------------------ */
//compares the contents of arrays, returns true if they're
// similar.
BOOL compArrs(int randArr [], int userArr[])
{
    int i;
    for(i=0; i<DIFFICULTY; ++i)
    {
        if(randArr[i]!=userArr[i])
            return fFalse;
    }
    return fTrue;
}

/* ------------------------------------------------------------ */
//user must hold down button 1 for a pseudo random
// seed value.  Uses user's "slow" reflexes compared
// to time granularity of chip.
//this resulted from the mistakes of making the AcceptInput()
// function, because the but1count value was in the thousands
// after one click!
int getSeed(void)
{
    int userSeed = 0;

    while(ButtonPressed() != 2 || userSeed == 0)
    {
        if(ButtonPressed() == 1)
            ++userSeed;
    }
    return userSeed;
}

void timer1Wait(unsigned int theTempo)
{
	timerCount=0;	

	while(1)
	{
		if(timerCount >= theTempo)
			break;
	}
	
}

int main(void)
{
	//buttons, LEDs, timers, ISRs
	DeviceInit();	

	//too much time LCD display
	clrLCD();
	cmdLCD(0x00 | 0x00);
	putsLCD("give 2 taps:");
	cmdLCD(0x80 | 0x40);
	putsLCD("btn2 then btn1");

	while(1)
	{
		// restart timerCount when btn2 pressed
		// only 1, 2 or 3 (don't use 3)
		if(ButtonPressed()==2)
		{
			//rising edge only
			if(btn2Edge==fFalse)
			{
				//reset timerCount for tempo variable
				timerCount = 0;
				btn2Edge = fTrue;	
			}
			SignalStatus(SIGNAL_BUTTON2);
			Wait_ms(10);
			SignalStatus(SIGNAL_RESET);
			
			break;
		}
	}

	//now, get the click from button1
	while(1)
	{
		WORD buttonTemp = ButtonPressed();

		//while loop limit... user has exceeded allowed_time
		if(timerCount >= allowedTime)
		{
			tempo=0;
			break;
		}

		else if(buttonTemp==1 && btn1Edge==fFalse)
		{
			btn1Edge=fTrue;
			tempo=timerCount;
			break;
		}
	}

	//value has NOT been entered within allowed_time
	if(tempo==0)
	{
		//too much time LCD display
		clrLCD();
		cmdLCD(0x00 | 0x00);
		putsLCD("You ran out");
		cmdLCD(0x80 | 0x40);
		putsLCD("of time!");		

		while(1)
		{
			SignalStatus(SIGNAL_ERROR);
		}
	}

	//it has been entered within allowed_time!
	char bpm[20] = "BPM = ";
	float tempoFloat = (float)tempo;
	int BPM = (60000.0f * 0.79f)/tempoFloat;	//our fudge factor stuff!
	strcat(bpm, intToString(BPM));
	clrLCD();
	putsLCD(bpm);

	while(1)
	{
		timer1Wait(tempo);
		SignalStatus(SIGNAL_BUTTON1);
		timer1Wait(5);
		SignalStatus(SIGNAL_RESET);
	}
	
    exit(0);
}  //end main

