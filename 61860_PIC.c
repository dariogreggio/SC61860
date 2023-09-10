// Local Header Files
#include <stdlib.h>
#include <string.h>
#include <xc.h>
#include "60860_PIC.h"

#include <sys/attribs.h>
#include <sys/kmem.h>

#include "Adafruit_ST77xx.h"
#include "Adafruit_ST7735.h"
#include "adafruit_gfx.h"



// PIC32MZ1024EFE064 Configuration Bit Settings

// 'C' source line config statements

// DEVCFG3
// USERID = No Setting
#pragma config FMIIEN = OFF             // Ethernet RMII/MII Enable (RMII Enabled)
#pragma config FETHIO = OFF             // Ethernet I/O Pin Select (Alternate Ethernet I/O)
#pragma config PGL1WAY = ON             // Permission Group Lock One Way Configuration (Allow only one reconfiguration)
#pragma config PMDL1WAY = ON            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO = ON            // USB USBID Selection (Controlled by the USB Module)

// DEVCFG2
/* Default SYSCLK = 200 MHz (8MHz FRC / FPLLIDIV * FPLLMUL / FPLLODIV) */
//#pragma config FPLLIDIV = DIV_1, FPLLMULT = MUL_50, FPLLODIV = DIV_2
#pragma config FPLLIDIV = DIV_1         // System PLL Input Divider (1x Divider)
#pragma config FPLLRNG = RANGE_5_10_MHZ// System PLL Input Range (5-10 MHz Input)
#pragma config FPLLICLK = PLL_FRC       // System PLL Input Clock Selection (FRC is input to the System PLL)
#pragma config FPLLMULT = MUL_51       // System PLL Multiplier (PLL Multiply by 50)
#pragma config FPLLODIV = DIV_2        // System PLL Output Clock Divider (2x Divider)
#pragma config UPLLFSEL = FREQ_24MHZ    // USB PLL Input Frequency Selection (USB PLL input is 24 MHz)

// DEVCFG1
#pragma config FNOSC = FRCDIV           // Oscillator Selection Bits (Fast RC Osc w/Div-by-N (FRCDIV))
#pragma config DMTINTV = WIN_127_128    // DMT Count Window Interval (Window/Interval value is 127/128 counter value)
#pragma config FSOSCEN = ON             // Secondary Oscillator Enable (Enable SOSC)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FCKSM = CSECME           // Clock Switching and Monitor Selection (Clock Switch Enabled, FSCM Enabled)
#pragma config WDTPS = PS16384          // Watchdog Timer Postscaler (1:16384)
  // circa 6-7 secondi, 24.7.19
#pragma config WDTSPGM = STOP           // Watchdog Timer Stop During Flash Programming (WDT stops during Flash programming)
#pragma config WINDIS = NORMAL          // Watchdog Timer Window Mode (Watchdog Timer is in non-Window mode)
#pragma config FWDTEN = ON             // Watchdog Timer Enable (WDT Enabled)
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window size is 25%)
#pragma config DMTCNT = DMT31           // Deadman Timer Count Selection (2^31 (2147483648))
#pragma config FDMTEN = OFF             // Deadman Timer Enable (Deadman Timer is disabled)

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Disabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#pragma config TRCEN = OFF              // Trace Enable (Trace features in the CPU are disabled)
#pragma config BOOTISA = MIPS32         // Boot ISA Selection (Boot code and Exception code is MIPS32)
#pragma config FECCCON = OFF_UNLOCKED   // Dynamic Flash ECC Configuration (ECC and Dynamic ECC are disabled (ECCCON bits are writable))
#pragma config FSLEEP = OFF             // Flash Sleep Mode (Flash is powered down when the device is in Sleep mode)
#pragma config DBGPER = PG_ALL          // Debug Mode CPU Access Permission (Allow CPU access to all permission regions)
#pragma config SMCLR = MCLR_NORM        // Soft Master Clear Enable bit (MCLR pin generates a normal system Reset)
#pragma config SOSCGAIN = GAIN_2X       // Secondary Oscillator Gain Control bits (2x gain setting)
#pragma config SOSCBOOST = ON           // Secondary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config POSCGAIN = GAIN_2X       // Primary Oscillator Gain Control bits (2x gain setting)
#pragma config POSCBOOST = ON           // Primary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config EJTAGBEN = NORMAL        // EJTAG Boot (Normal EJTAG functionality)

// DEVCP0
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// SEQ3

// DEVADC0

// DEVADC1

// DEVADC2

// DEVADC3

// DEVADC4

// DEVADC7



const char CopyrightString[]= {'6','1','8','6','0',' ','E','m','u','l','a','t','o','r',' ','v',
	VERNUMH+'0','.',VERNUML/10+'0',(VERNUML % 10)+'0',' ','-',' ', '1','0','/','9','/','2','3', 0 };

const char Copyr1[]="(C) Dario's Automation 2023 - G.Dar\xd\xa\x0";



// Global Variables:
BOOL fExit,debug;
extern BYTE DoIRQ,DoNMI,DoHalt,DoReset,ColdReset;
extern BYTE ram_seg[];
extern BYTE rom_seg[],rom_seg2[];
extern BYTE Keyboard[1];
extern volatile BYTE TIMIRQ,VIDIRQ,KBDIRQ,SERIRQ,RTCIRQ;
extern BYTE LCDram[256 /* 4*40 per la geometria, vale così */],LCDfunction,LCDentry,LCDdisplay,LCDcursor;
extern signed char LCDptr;
extern BYTE IOExtPortI[4],IOExtPortO[4];
extern BYTE IOPortI,IOPortO,ClIRQPort,ClWDPort;
extern BYTE KBDataI,KBDataO,KBControl /*,KBStatus*/, KBRAM[32];
#define KBStatus KBRAM[0]   // pare...
extern const unsigned char fontLCD_eu[],fontLCD_jp[];
volatile PIC32_RTCC_DATE currentDate={1,1,0};
volatile PIC32_RTCC_TIME currentTime={0,0,0};
const BYTE dayOfMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};

WORD textColors[16]={BLACK,WHITE,RED,CYAN,MAGENTA,GREEN,BLUE,YELLOW,
	ORANGE,BROWN,BRIGHTRED,DARKGRAY,GRAY128,LIGHTGREEN,BRIGHTCYAN,LIGHTGRAY};
/*COLORREF Colori[16]={
	RGB(0,0,0),						 // nero
	RGB(0xff,0xff,0xff),	 // bianco
	RGB(0x80,0x00,0x00),	 // rosso
	RGB(0x00,0x80,0x80),	 // azzurro
	RGB(0x80,0x00,0x80),	 // porpora
	RGB(0x00,0x80,0x00),	 // verde
	RGB(0x00,0x00,0x80),	 // blu
	RGB(0x80,0x80,0x00),	 // giallo
	
	RGB(0xff,0x80,0x40),	 // arancio
	RGB(0x80,0x40,0x40),	 // marrone
	RGB(0xff,0x80,0x80),	 // rosso chiaro
	RGB(0x20,0x20,0x20),	 // grigio 1
	RGB(0x54,0x54,0x54),	 // grigio 2
	RGB(0xc0,0xc0,0xc0),	 // grigio chiaro
	RGB(0x80,0x80,0xff),	 // blu chiaro
	RGB(0xa8,0xa8,0xa8)		 // grigio 3
	};*/



WORD displayColor[3]={BLACK,BRIGHTRED,RED};

int UpdateScreen(WORD c) {
  int x,y,x1,y1;
  SWORD color;
	UINT8 i,j,lcdMax;
	BYTE *fontPtr,*lcdPtr;
  static BYTE cursorState=0,cursorDivider=0;

#define LCD_MAX_X 20
#define LCD_MAX_Y 1			// tipo Sharp PC 1403
#define DIGIT_X_SIZE 6
#define DIGIT_Y_SIZE 8
  
  
//  i8255RegR[0] |= 0x80; mah... fare?? v. di là

  y=(_TFTHEIGHT-(LCD_MAX_Y*DIGIT_Y_SIZE))/2 +20;
  
//	fillRect(x,y,DIGIT_X_SIZE+3,DIGIT_Y_SIZE+1,BLACK);
	gfx_drawRect((_TFTWIDTH-(LCD_MAX_X*DIGIT_X_SIZE))/2-1,(_TFTHEIGHT-(LCD_MAX_Y*DIGIT_Y_SIZE))/2 +16,
          DIGIT_X_SIZE*20+4,DIGIT_Y_SIZE*4+7,LIGHTGRAY);
  
  if(c)
    color=WHITE;
  else
    color=LIGHTGRAY;

  
//        LCDdisplay=7; //test cursore

  cursorDivider++;
  if(cursorDivider>=11) {
    cursorDivider=0;
    cursorState=!cursorState;
    }
          
        
  if(LCDdisplay & 4) {

  lcdMax=LCDfunction & 8 ? LCD_MAX_Y : LCD_MAX_Y/2;
  for(y1=0; y1<LCD_MAX_Y; y1++) {
    x=(_TFTWIDTH-(LCD_MAX_X*DIGIT_X_SIZE))/2;
    
//    LCDram[0]='A';LCDram[1]='1';LCDram[2]=1;LCDram[3]=40;
//    LCDram[21]='Z';LCDram[23]='8';LCDram[25]='0';LCDram[27]=64;LCDram[39]='.';
//    LCDram[84+4]='C';LCDram[84+5]='4';
    
    
    switch(y1) {    // 4x20 
      case 0:
        lcdPtr=&LCDram[0];
        break;
      case 1:
        lcdPtr=&LCDram[0x40];
        break;
      case 2:
        lcdPtr=&LCDram[20];
        break;
      case 3:
        lcdPtr=&LCDram[0x40+20];
        break;
      }

    for(x1=0; x1<LCD_MAX_X; x1++) {
//      UINT8 ch;
  
//      ch=*lcdPtr;
//      if(LCDdisplay & 2) {
//	      if(!(LCDdisplay & 1) || cursorState) { questo era per avere il bloccone, fisso o lampeggiante, ma in effetti sui LCD veri è diverso!
      if((lcdPtr-&LCDram[0]) == LCDptr) {
        if(LCDdisplay & 2) {
          for(j=6; j>1; j--) {    //lineetta bassa E FONT TYPE QUA??
            drawPixel(x+x1*6+j, y+7, color);
            }
          }
        if((LCDdisplay & 1) && cursorState) {
          int k=LCDdisplay & 2 ? 7 : 8;

          for(i=0; i<k; i++) {    //

            if(LCDfunction & 4)   // font type...
              ;

            for(j=6; j>1; j--) {    //+ piccolo..
              drawPixel(x+x1*6+j, y+i, color);
              }
            }
          }
        goto skippa;
        }
      
      fontPtr=fontLCD_eu+((UINT16)*lcdPtr)*10;
      for(i=0; i<8; i++) {
        UINT8 line;

        line = pgm_read_byte(fontPtr+i);

        if(LCDfunction & 4)   // font type...
          ;
        
        for(j=6; j>0; j--, line >>= 1) {
          if(line & 0x1)
            drawPixel(x+x1*6+j, y+i, color);
          else
            drawPixel(x+x1*6+j, y+i, BLACK);
          }
        }
      
skippa:
      lcdPtr++;
      }
      
    y+=DIGIT_Y_SIZE;
    }
    }
  
//  i8255RegR[0] &= ~0x7f;
  
//  LuceLCD=i8255RegW[1] &= 0x80; fare??

  
	gfx_drawRect(4,41,_TFTWIDTH-9,13,ORANGE);
  for(i=0,j=1; i<8; i++,j<<=1) {
    if(IOExtPortO[0] & j)
      fillCircle(10+i*9,47,3,RED);
    else
      fillCircle(10+i*9,47,3,DARKGRAY);
    }
  for(i=8,j=1; i<16; i++,j<<=1) {
    if(IOExtPortO[1] & j)
      fillCircle(12+i*9,47,3,RED);
    else
      fillCircle(12+i*9,47,3,DARKGRAY);
    }
  for(i=0,j=1; i<8; i++,j<<=1) {
    if(IOPortO /*ram_seg[0] /*MemPort1 = 0x8000 */ & j)
      fillCircle(10+i*7,36,2,RED);
    else
      fillCircle(10+i*7,36,2,DARKGRAY);
    }
          
	}



int main(void) {

  // disable JTAG port
//  DDPCONbits.JTAGEN = 0;
  
  CFGCONbits.IOLOCK = 0;      // PPS Unlock
  RPB15Rbits.RPB15R = 4;        // Assign RPB15 as U6TX, pin 30
  U6RXRbits.U6RXR = 2;      // Assign RPB14 as U6RX, pin 29 
#ifdef USA_SPI_HW
  RPG8Rbits.RPG8R = 6;        // Assign RPG8 as SDO2, pin 6
//  SDI2Rbits.SDI2R = 1;        // Assign RPG7 as SDI2, pin 5
#endif
  RPD5Rbits.RPD5R = 12;        // Assign RPD5 as OC1, pin 53; anche vaga uscita audio :)
  CFGCONbits.IOLOCK = 1;      // PPS Lock

//  PPSOutput(4,RPC4,OC1);   //buzzer 4KHz , qua rimappabile 

#ifdef DEBUG_TESTREFCLK
// test REFCLK
  PPSOutput(4,RPC4,REFCLKO2);   // RefClk su pin 1 (RG15, buzzer)
	REFOCONbits.ROSSLP=1;
	REFOCONbits.ROSEL=1;
	REFOCONbits.RODIV=0;
	REFOCONbits.ROON=1;
	TRISFbits.TRISF3=1;
#endif

//	PPSLock;

   // Disable all Interrupts
  __builtin_disable_interrupts();
  
//  SPLLCONbits.PLLMULT=10;
  
  OSCTUN=0;
  OSCCONbits.FRCDIV=0;
  
  // Switch to FRCDIV, SYSCLK=8MHz
  SYSKEY=0xAA996655;
  SYSKEY=0x556699AA;
  OSCCONbits.NOSC=0x00; // FRC
  OSCCONbits.OSWEN=1;
  SYSKEY=0x33333333;
  while(OSCCONbits.OSWEN) {
    Nop();
    }
    // At this point, SYSCLK is ~8MHz derived directly from FRC
 //http://www.microchip.com/forums/m840347.aspx
  // Switch back to FRCPLL, SYSCLK=200MHz
  SYSKEY=0xAA996655;
  SYSKEY=0x556699AA;
  OSCCONbits.NOSC=0x01; // SPLL
  OSCCONbits.OSWEN=1;
  SYSKEY=0x33333333;
  while(OSCCONbits.OSWEN) {
    Nop();
    }
  // At this point, SYSCLK is ~200MHz derived from FRC+PLL
//***
  mySYSTEMConfigPerformance();
  //myINTEnableSystemMultiVectoredInt(();

    
	TRISB=0b0000000000110000;			// AN4,5 (rb4..5)
	TRISC=0b0000000000000000;
	TRISD=0b0000000000001100;			// 2 pulsanti
	TRISE=0b0000000000000000;			// 3 led
	TRISF=0b0000000000000000;			// 
	TRISG=0b0000000000000000;			// SPI2 (rg6..8)

  ANSELB=0;
  ANSELE=0;
  ANSELG=0;

  CNPUDbits.CNPUD2=1;   // switch/pulsanti
  CNPUDbits.CNPUD3=1;
  CNPUGbits.CNPUG6=1;   // I2C tanto per
  CNPUGbits.CNPUG8=1;  

      
  
  Timer_Init();
  PWM_Init();
  UART_Init(/*230400L*/ 115200L);

  myINTEnableSystemMultiVectoredInt();
  ShortDelay(50000); 

  
//    	ColdReset=0;    Emulate(0);

#ifndef USING_SIMULATOR
//#ifndef __DEBUG
  Adafruit_ST7735_1(0,0,0,0,-1);
  Adafruit_ST7735_initR(INITR_BLACKTAB);
  
//  displayInit(NULL);
  
#ifdef m_LCDBLBit
  m_LCDBLBit=1;
#endif
  
//	begin();
	clearScreen();

// init done
	setTextWrap(1);
//	setTextColor2(WHITE, BLACK);

	drawBG();
  
  __delay_ms(200);
  
	gfx_fillRect(3,_TFTHEIGHT-20,_TFTWIDTH-6,16,BLACK);
 	setTextColor(BLUE);
	LCDXY(3,14);
	gfx_print("(emulating 4x20 LCD)");


//#endif
#endif
  

//  memcpy(rom_seg,CHIESA_BIN,0xa08);
  memcpy(rom_seg,Z803_BIN,0x16be);
//  memcpy(rom_seg2,CASANET_BIN,0x997);
  memcpy(rom_seg2,CASANET3_BIN,0x280);
//  memcpy(rom_seg2,SKYBASIC_BIN,0x31fa);
        
	ColdReset=0;

  Emulate(0);

  }


void mySYSTEMConfigPerformance(void) {
  unsigned PLLIDIV;
  unsigned PLLMUL;
  unsigned PLLODIV;
  float CLK2USEC;
  unsigned SYSCLK;
  static unsigned char PLLODIVVAL[]={
    2,2,4,8,16,32,32,32
    };
	unsigned int cp0;

  PLLIDIV=SPLLCONbits.PLLIDIV+1;
  PLLMUL=SPLLCONbits.PLLMULT+1;
  PLLODIV=PLLODIVVAL[SPLLCONbits.PLLODIV];

  SYSCLK=(FOSC*PLLMUL)/(PLLIDIV*PLLODIV);
  CLK2USEC=SYSCLK/1000000.0f;

  SYSKEY = 0x0;
  SYSKEY = 0xAA996655;
  SYSKEY = 0x556699AA;

  if(SYSCLK<=60000000)
    PRECONbits.PFMWS=0;
  else if(SYSCLK<=120000000)
    PRECONbits.PFMWS=1;
  else if(SYSCLK<=200000000)
    PRECONbits.PFMWS=2;
  else if(SYSCLK<=252000000)
    PRECONbits.PFMWS=4;
  else
    PRECONbits.PFMWS=7;

  PRECONbits.PFMSECEN=0;    // non c'è nella versione "2019" ...
  PRECONbits.PREFEN=0x1;

  SYSKEY = 0x0;

  // Set up caching
  cp0 = _mfc0(16, 0);
  cp0 &= ~0x07;
  cp0 |= 0b011; // K0 = Cacheable, non-coherent, write-back, write allocate
  _mtc0(16, 0, cp0);  
  }

void myINTEnableSystemMultiVectoredInt(void) {

  PRISS = 0x76543210;
  INTCONSET = _INTCON_MVEC_MASK /*0x1000*/;    //MVEC
  asm volatile ("ei");
  //__builtin_enable_interrupts();
  }

/* CP0.Count counts at half the CPU rate */
#define TICK_HZ (CPU_HZ / 2)

/* wait at least usec microseconds */
#if 0
void delay_usec(unsigned long usec) {
unsigned long start, stop;

  /* get start ticks */
  start = readCP0Count();

  /* calculate number of ticks for the given number of microseconds */
  stop = (usec * 1000000) / TICK_HZ;

  /* add start value */
  stop += start;

  /* wait till Count reaches the stop value */
  while (readCP0Count() < stop)
    ;
  }
#endif

void xdelay_us(uint32_t us) {
  
  if(us == 0) {
    return;
    }
  unsigned long start_count = ReadCoreTimer /*_CP0_GET_COUNT*/();
  unsigned long now_count;
  long cycles = ((GetSystemClock() + 1000000U) / 2000000U) * us;
  do {
    now_count = ReadCoreTimer /*_CP0_GET_COUNT*/();
    } while ((unsigned long)(now_count-start_count) < cycles);
  }

void __attribute__((used)) DelayUs(unsigned int usec) {
  unsigned int tWait, tStart;

  tWait=(GetSystemClock()/2000000)*usec;
  tStart=_mfc0(9,0);
  while((_mfc0(9,0)-tStart)<tWait)
    ClrWdt();        // wait for the time to pass
  }

void __attribute__((used)) DelayMs(unsigned int ms) {
  
  for(;ms;ms--)
    DelayUs(1000);
  }

// ===========================================================================
// ShortDelay - Delays (blocking) for a very short period (in CoreTimer Ticks)
// ---------------------------------------------------------------------------
// The DelayCount is specified in Core-Timer Ticks.
// This function uses the CoreTimer to determine the length of the delay.
// The CoreTimer runs at half the system clock. 100MHz
// If CPU_CLOCK_HZ is defined as 80000000UL, 80MHz/2 = 40MHz or 1LSB = 25nS).
// Use US_TO_CT_TICKS to convert from uS to CoreTimer Ticks.
// ---------------------------------------------------------------------------

void ShortDelay(                       // Short Delay
  DWORD DelayCount)                   // Delay Time (CoreTimer Ticks)
{
  DWORD StartTime;                    // Start Time
  StartTime = ReadCoreTimer();         // Get CoreTimer value for StartTime
  while( (DWORD)(ReadCoreTimer() - StartTime) < DelayCount)
    ClrWdt();
  }
 

void Timer_Init(void) {

  T2CON=0;
  T2CONbits.TCS = 0;                  // clock from peripheral clock
  T2CONbits.TCKPS = 7;                // 1:256 prescaler (pwm clock=390625Hz)
  T2CONbits.T32 = 0;                  // 16bit
//  PR2 = 2000;                         // rollover every n clocks; 2000 = 50KHz
  PR2 = 65535;                         // per ora faccio solo onda quadra
  T2CONbits.TON = 1;                  // start timer per PWM
  
  // TIMER 3 INITIALIZATION (TIMER IS USED AS A TRIGGER SOURCE FOR ALL CHANNELS).
  T3CON=0;
  T3CONbits.TCS = 0;                  // clock from peripheral clock
  T3CONbits.TCKPS = 4;                // 1:16 prescaler
  PR3 = (GetPeripheralClock()/16)/1600;         // 1600Hz
  T3CONbits.TON = 1;                  // start timer 

  IPC3bits.T3IP=4;            // set IPL 4, sub-priority 2??
  IPC3bits.T3IS=0;
  IEC0bits.T3IE=1;             // enable Timer 3 interrupt se si vuole

	}

void PWM_Init(void) {

  CFGCONbits.OCACLK=0;      // sceglie timer per PWM
  
  OC1CON = 0x0006;      // TimerX ossia Timer2; PWM mode no fault; Timer 16bit, TimerX
//  OC1R    = 500;		 // su PIC32 è read-only!
//  OC1RS   = 1000;   // 50%, relativo a PR2 del Timer2
  OC1R    = 32768;		 // su PIC32 è read-only!
  OC1RS   = 0;        // per ora faccio solo onda quadra, v. SID reg. 0-1
  OC1CONbits.ON = 1;   // on

  }

void UART_Init(DWORD baudRate) {
  
  U6MODE=0b0000000000001000;    // BRGH=1
  U6STA= 0b0000010000000000;    // TXEN
  DWORD baudRateDivider = ((GetPeripheralClock()/(4*baudRate))-1);
  U6BRG=baudRateDivider;
  U6MODEbits.ON=1;
  
#if 0
  ANSELDCLR = 0xFFFF;
  CFGCONbits.IOLOCK = 0;      // PPS Unlock
  RPD11Rbits.RPD11R = 3;        // Assign RPD11 as U1TX
  U1RXRbits.U1RXR = 3;      // Assign RPD10 as U1RX
  CFGCONbits.IOLOCK = 1;      // PPS Lock

  // Baud related stuffs.
  U1MODEbits.BRGH = 1;      // Setup High baud rates.
  unsigned long int baudRateDivider = ((GetSystemClock()/(4*baudRate))-1);
  U1BRG = baudRateDivider;  // set BRG

  // UART Configuration
  U1MODEbits.ON = 1;    // UART1 module is Enabled
  U1STAbits.UTXEN = 1;  // TX is enabled
  U1STAbits.URXEN = 1;  // RX is enabled

  // UART Rx interrupt configuration.
  IFS1bits.U1RXIF = 0;  // Clear the interrupt flag
  IFS1bits.U1TXIF = 0;  // Clear the interrupt flag

  INTCONbits.MVEC = 1;  // Multi vector interrupts.

  IEC1bits.U1RXIE = 1;  // Rx interrupt enable
  IEC1bits.U1EIE = 1;
  IPC7bits.U1IP = 7;    // Rx Interrupt priority level
  IPC7bits.U1IS = 3;    // Rx Interrupt sub priority level
#endif
  }

char BusyUART1(void) {
  
  return(!U6STAbits.TRMT);
  }

void putsUART1(unsigned int *buffer) {
  char *temp_ptr = (char *)buffer;

    // transmit till NULL character is encountered 

  if(U6MODEbits.PDSEL == 3)        /* check if TX is 8bits or 9bits */
    {
        while(*buffer) {
            while(U6STAbits.UTXBF); /* wait if the buffer is full */
            U6TXREG = *buffer++;    /* transfer data word to TX reg */
        }
    }
  else {
        while(*temp_ptr) {
            while(U6STAbits.UTXBF);  /* wait if the buffer is full */
            U6TXREG = *temp_ptr++;   /* transfer data byte to TX reg */
        }
    }
  }

unsigned int ReadUART1(void) {
  
  if(U6MODEbits.PDSEL == 3)
    return (U6RXREG);
  else
    return (U6RXREG & 0xFF);
  }

void WriteUART1(unsigned int data) {
  
  if(U6MODEbits.PDSEL == 3)
    U6TXREG = data;
  else
    U6TXREG = data & 0xFF;
  }

void __ISR(_UART1_RX_VECTOR) UART1_ISR(void) {
  
  LATDbits.LATD4 ^= 1;    // LED to indicate the ISR.
  char curChar = U1RXREG;
  IFS3bits.U1RXIF = 0;  // Clear the interrupt flag!
  }


int emulateKBD(BYTE ch) {
  int i;


  
no_irq:
    ;
  }

BYTE whichKeysFeed=0;
char keysFeed[32]={0};
volatile BYTE keysFeedPtr=255;
const char *keysFeed1="02E1\r";

void __ISR(_TIMER_3_VECTOR,ipl4SRS) TMR_ISR(void) {
// https://www.microchip.com/forums/m842396.aspx per IRQ priority ecc
  static BYTE divider,divider2;
  static WORD dividerTim;
  static WORD dividerEmulKbd;
  static BYTE keysFeedPhase=0;
  int i;

#define TIMIRQ_DIVIDER 32   // 
  
  //LED2 ^= 1;      // check timing: 1600Hz, 9/11/19 (fuck berlin day)) 2022 ok fuck UK ;) & anyone
  
  divider++;
#ifdef USING_SIMULATOR
  if(divider>=1) {   // 
#else
  if(divider>=TIMIRQ_DIVIDER) {   //
#endif
    divider=0;
//    CIA1IRQ=1;
    }


  dividerTim++;
  if(dividerTim>=1600) {   // 1Hz RTC
		} 
  

  if(keysFeedPtr==255)      // EOL
    goto fine;
  if(keysFeedPtr==254) {    // NEW string
    keysFeedPtr=0;
    keysFeedPhase=0;
		switch(whichKeysFeed) {
			case 0:
				strcpy(keysFeed,keysFeed1);
				break;
      }
		whichKeysFeed++;
		if(whichKeysFeed>=1)
			whichKeysFeed=0;
//    goto fine;
		}
  if(keysFeed[keysFeedPtr]) {
    dividerEmulKbd++;
    if(dividerEmulKbd>=500 /*300*/) {   // ~.2Hz per emulazione tastiera! (più veloce di tot non va...))
      dividerEmulKbd=0;
      if(!keysFeedPhase) {
        keysFeedPhase=1;
        emulateKBD(keysFeed[keysFeedPtr]);
        }
      else {
        keysFeedPtr++;
wait_kbd: ;
        }
      }
    }
  else
    keysFeedPtr=255;

    
fine:
  IFS0CLR = _IFS0_T3IF_MASK;
  }

// ---------------------------------------------------------------------------------------
// declared static in case exception condition would prevent
// auto variable being created
static enum {
	EXCEP_IRQ = 0,			// interrupt
	EXCEP_AdEL = 4,			// address error exception (load or ifetch)
	EXCEP_AdES,				// address error exception (store)
	EXCEP_IBE,				// bus error (ifetch)
	EXCEP_DBE,				// bus error (load/store)
	EXCEP_Sys,				// syscall
	EXCEP_Bp,				// breakpoint
	EXCEP_RI,				// reserved instruction
	EXCEP_CpU,				// coprocessor unusable
	EXCEP_Overflow,			// arithmetic overflow
	EXCEP_Trap,				// trap (possible divide by zero)
	EXCEP_IS1 = 16,			// implementation specfic 1
	EXCEP_CEU,				// CorExtend Unuseable
	EXCEP_C2E				// coprocessor 2
  } _excep_code;

static unsigned int _epc_code;
static unsigned int _excep_addr;

void __attribute__((weak)) _general_exception_handler(uint32_t __attribute__((unused)) code, uint32_t __attribute__((unused)) address) {
  }

void __attribute__((nomips16,used)) _general_exception_handler_entry(void) {
  
	asm volatile("mfc0 %0,$13" : "=r" (_epc_code));
	asm volatile("mfc0 %0,$14" : "=r" (_excep_addr));

	_excep_code = (_epc_code & 0x0000007C) >> 2;

  _general_exception_handler(_excep_code, _excep_addr);

	while (1)	{
		// Examine _excep_code to identify the type of exception
		// Examine _excep_addr to find the address that caused the exception
    }
  }


