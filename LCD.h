
#include <p32xxxx.h>

//**************************************************************
//***************** Local Type Definition for LCD **************
#define HLCD 16 // LCD width=16 characters
#define VLCD 2 // LCD height=2 rows
#define LCDDATA 1 // address of data register
#define LCDCMD 0 // address of command register


#define putLCD( d) writeLCD( LCDDATA, (d))
#define cmdLCD( c) writeLCD( LCDCMD, (c))
#define clrLCD() writeLCD( LCDCMD, 1)
#define homeLCD() writeLCD( LCDCMD, 2)
#define setLCDG( a) writeLCD( LCDCMD, (a & 0x3F) | 0x40)
#define setLCDC( a) writeLCD( LCDCMD, (a & 0x7F) | 0x80)

#define busyLCD() ( readLCD( LCDCMD) & 0x80)
#define addrLCD() ( readLCD( LCDCMD) & 0x7F)
#define getLCD() readLCD( LCDDATA)

#define PMDATA  PMDIN

#define TIP 0x00
#define BRICK 0xff
#define FALSE 0
#define TRUE !FALSE
#define FCY 72000000L
#define FPB 36000000L
//**************************************************************

//Initialize LCD
void initLCD( void);

//Read LCD
char readLCD( int addr);

//Write to LCD
void writeLCD( int addr, char c);

//Put character to LCD
void putsLCD( char *s);

void Delayms( unsigned);

void drawProgressBar( int index, int imax, int size);
