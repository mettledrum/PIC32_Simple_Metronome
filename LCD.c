
#include "LCD.h"

//--------------------- Code Section for LCD ------------------
//********************* LCD Initialization ********************
void initLCD( void)
{
    // PMP initialization changed considerably!!!
    PMCON = 0x8303;    // Enable the PMP, demuxed, RD/WR and E  
    PMMODE = 0x3FF;    // 8-bit, Master Mode 1, max wait states
	PMAEN = 0x0001;    // only PMA0 enabled
    
    // init TMR1
    T1CON = 0x8030;    // Fpb/2, prescaled 1:256, 6us/tick

    // wait for >30ms
    TMR1 = 0; while( TMR1<6000);// 6000 x 6us = 36ms   
    
    //initiate the HD44780 display 8-bit init sequence
    PMADDR = LCDCMD;            // command register
    PMDATA = 0x38;              // 8-bit int, 2 lines, 5x7
    TMR1 = 0; while( TMR1<8);   // 8 x 6us = 48us   
    
    PMDATA = 0x0c;              // disp.ON, no cursor, no blink
    TMR1 = 0; while( TMR1<8);   // 8 x 6us = 48us   
    
    PMDATA = 1;                 // clear display
    TMR1 = 0; while( TMR1<300); // 300 x 6us = 1.8ms   
    
    PMDATA = 6;                 // increment cursor, no shift
    TMR1 = 0; while( TMR1<300); // 300 x 6us = 1.8ms   
} // initLCD


//************************** Read CLD *************************
char readLCD( int addr)
{
    int dummy;
    while( PMMODEbits.BUSY);    // wait for PMP to be available
    PMADDR = addr;              // select the command address
    dummy = PMDATA;             // init read cycle, dummy read
    while( PMMODEbits.BUSY);    // wait for PMP to be available
    return( PMDATA);            // read the status register
    
} // readLCD

//************************* Write LCD *************************
void writeLCD( int addr, char c)    
{
    while(busyLCD());
    while(PMMODEbits.BUSY);    // wait for PMP to be available
    PMADDR = addr;
    PMDATA = c;
} // writeLCD
   
//*************************** Put CLD *************************
void putsLCD( char *s)
{
    while( *s) putLCD( *s++);
} //putsLCD

//---------------- End of Code Section for LCD ----------------
