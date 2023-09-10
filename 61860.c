//https://github.com/utz82/SC61860-Instruction-Set
//https://shop-pdp.net/ashtml/as6186.htm

// 10/9/23

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
//#include <graph.h>
//#include <dos.h>
//#include <malloc.h>
//#include <memory.h>
//#include <fcntl.h>
//#include <io.h>
#include <xc.h>


#include "61860_PIC.h"
#include "Adafruit_ST77xx.h"      // per LOBYTE ecc...


#pragma check_stack(off)
// #pragma check_pointer( off )
#pragma intrinsic( _enable, _disable )


extern BYTE fExit;
extern BYTE debug;

extern volatile BYTE keysFeedPtr;
extern volatile BYTE TIMIRQ;

extern BYTE ram_seg[MAX_RAM],*stack_seg;
BYTE DoReset=0,DoWait=0;
#define MAX_WATCHDOG 100      // x30mS v. sotto
WORD WDCnt=MAX_WATCHDOG;
BYTE ColdReset=1;
BYTE Pipe1;
union /*__attribute__((__packed__))*/ {
	BYTE bb[3];
	struct /*__attribute__((__packed__))*/ {
		BYTE l;
		BYTE h;
		} b;
	} Pipe2;



union /*__attribute__((__packed__))*/ Z_REG {
  SWORD x;
  struct /*__attribute__((__packed__))*/ { 
    BYTE l;
    BYTE h;
    } b;
  struct /*__attribute__((__packed__))*/ { 
    unsigned int l:7;
    } b7;
//    } _I,_J,_A,_B,_XL,_XH,_YL,_YH,_K,_L,_M,_N;
  };
union /*__attribute__((__packed__))*/ Z_REGISTERS {
  BYTE  b[12];
  union Z_REG r[12];
  };

#define ID_CARRY 0x1
#define ID_ZERO 0x2
union /*__attribute__((__packed__))*/ REGISTRO_F {
  BYTE b;
  struct /*__attribute__((__packed__))*/ {
    unsigned int Carry: 1;
    unsigned int Zero: 1;
    };
  };
union /*__attribute__((__packed__))*/ OPERAND {
  BYTE *reg8;
  WORD *reg16;
  WORD mem;
  };
union /*__attribute__((__packed__))*/ RESULT {
  struct /*__attribute__((__packed__))*/ {
    BYTE l;
    BYTE h;
    } b;
  WORD x;
  };
    
int Emulate(int mode) {
/*Reg     Address         Common use
---     -------         ----------
i, j    0, 1            Length of block operations
a, b    2, 3            Accumulator       
xl, xh  4, 5            Pointer for read operations
yl, yh  6, 7            Pointer for write operations
k - n   8 - 0x0b        General purpose (counters ...)
  -     0x0c - 0x5b     Stack
ia      0x5c            Inport A
ib      0x5d            Inport B
fo      0x5e            Outport F
cout    0x5f            Control port*/
#define _i regs1.r[0].b.l
#define _j regs1.r[1].b.l
#define _a regs1.r[2].b.l
#define _b regs1.r[3].b.l
#define _xl regs1.r[4].b.l
#define _xh regs1.r[5].b.l
#define _yl regs1.r[6].b.l
#define _yh regs1.r[7].b.l
#define _x regs1.r[4].x
#define _y regs1.r[6].x
#define _k regs1.r[8].b.l
#define _l regs1.r[9].b.l
#define _m regs1.r[10].b.l
#define _n regs1.r[11].b.l
#define WORKING_REG regs1.b[((Pipe1 & 0x38) ^ 8) >> 3]      // la parte bassa/alta è invertita...
#define WORKING_REG2 regs1.b[(Pipe1 ^ 1) & 7]
#define WORKING_BITPOS (1 << ((Pipe2.b.l & 0x38) >> 3))
#define WORKING_BITPOS2 (1 << ((Pipe2.b.h & 0x38) >> 3))
    
	SWORD _pc=0;
	BYTE _q=0,_r=0;
#define _sp _r
	BYTE _d;
  union Z_REGISTERS regs1,regs2;
  union RESULT res1,res2,res3;
//  union OPERAND op1,op2;
	union REGISTRO_F _f;
	union REGISTRO_F _f1;
	/*register*/ SWORD i;
  int c=0;


	_pc=0;
  
  
  
  
	do {

		c++;
		if(!(c & 0x3ffff)) {
      ClrWdt();
// yield()
#ifndef USING_SIMULATOR      
			UpdateScreen(1);
#endif
      LED1^=1;    // 42mS~ con SKYNET 7/6/20; 10~mS con Z80NE 10/7/21; 35mS GALAKSIJA 16/10/22; 30mS ZX80 27/10/22
      // QUADRUPLICO/ecc! 27/10/22
      
      }

		if(ColdReset) {
			ColdReset=0;
      DoReset=1;
			continue;
      }



    
		/*
		if((_pc >= 0xa000) && (_pc <= 0xbfff)) {
			printf("%04x    %02x\n",_pc,GetValue(_pc));
			}
			*/
		if(debug) {
//			printf("%04x    %02x\n",_pc,GetValue(_pc));
			}
		/*if(kbhit()) {
			getch();
			printf("%04x    %02x\n",_pc,GetValue(_pc));
			printf("281-284: %02x %02x %02x %02x\n",*(p1+0x281),*(p1+0x282),*(p1+0x283),*(p1+0x284));
			printf("2b-2c: %02x %02x\n",*(p1+0x2b),*(p1+0x2c));
			printf("33-34: %02x %02x\n",*(p1+0x33),*(p1+0x34));
			printf("37-38: %02x %02x\n",*(p1+0x37),*(p1+0x38));
			}*/
		if(DoReset) {
			_pc=0;
      _i=0;
			DoReset=0;//DoWait=0;
      keysFeedPtr=255; //meglio ;)
      continue;
			}

  
		if(DoWait) {
      //mettere ritardino per analogia con le istruzioni?
//      __Dlay_ns(100);
			continue;		// esegue cmq IRQ?? penso di no... sistemare
      }

//printf("Pipe1: %02x, Pipe2w: %04x, Pipe2b1: %02x,%02x\n",Pipe1,Pipe2.word,Pipe2.bytes.byte1,Pipe2.bytes.byte2);
    
    
      if(!SW1) {        // test tastiera, me ne frego del repeat/rientro :)
       // continue;
        __Dlay_ms(100); ClrWdt();
        DoReset=1;
        }
      if(!SW2) {        // test tastiera
        if(keysFeedPtr==255)      // debounce...
          keysFeedPtr=254;
        }

      LED2^=1;    // ~700nS 7/6/20, ~600 con 32bit 10/7/21 MA NON FUNZIONA/visualizza!! verificare; 5-700nS 27/10/22

    
/*      if(_pc == 0x069d ab5 43c Cd3) {
        ClrWdt();
        }*/
  
		switch(GetPipe(_pc++)) {
			case 0:   // LII n
				_i=Pipe2.b.l;
				break;

			case 1:   // LIJ n
				_j=Pipe2.b.l;
				break;

			case 2:   // LIA n
				_a=Pipe2.b.l;
				break;

			case 3:   // LIB n
				_b=Pipe2.b.l;
				break;

			case 4:	// IX
				_x++;
				break;

			case 5:	// DX
				_x--;
				break;

			case 6:	// IY
				_y++;
				break;

			case 7:	// DY
				_y--;
				break;

			case 8:   // MVW
				break;

			case 9:   // EXW
				break;

			case 0xa:   // MVB
				break;

			case 0xb:   // EXB
				break;

			case 0xc:   // ADN
				break;

			case 0xd:   // SBN
				break;

			case 0xe:   // ADW
				break;

			case 0xf:   // SBW
				break;

			case 0x10:  // LIDP nm
				_dx=Pipe2.x;
				break;

			case 0x11:	// LIDL n
				_dl=Pipe2.b.l;
				break;

			case 0x12:  // LIP n
				_p=Pipe2.b.l;
				break;

			case 0x13:	// LIQ n
				_q=Pipe2.b.l;
				break;

			case 0x14:  // ADB
				res1.b.l=GetByte(_p);
				res2.b.l=_a;
				res1.b.h=GetByte(_p+1);
				res2.b.h=_b;
				goto aggAdd16;
				break;

			case 0x15:	// SBB
				res1.b.l=GetByte(_p);
				res2.b.l=_a;
				res1.b.h=GetByte(_p+1);
				res2.b.h=_b;
				goto aggSub16;
				break;

			case 0x16:	// 
			case 0x17:	// 
				break;

			case 0x18:   // MVWD
				break;

			case 0x19:   // EXWD
				break;

			case 0x1a:   // MVBD
				break;

			case 0x1b:   // EXBD
				break;

			case 0x1c:   // SRW
				break;

			case 0x1d:   // SLW
				break;

			case 0x1e:   // FILM
				break;

			case 0x1f:   // FILD
				break;

			case 0x20:		// LDP
				_a=_p;
				break;

			case 0x21:		// LDQ
				_a=_q;
				break;

			case 0x22:		// LDR
				_a=_r;
				break;

			case 0x23:		// CLRA
				_a=0;
				break;

			case 0x24:		// IXL
				_x++;
				_dp=_x;
				_a=GetByte(_dp);
				break;

			case 0x25:		// DXL
				_x--;
				_dp=_x;
				_a=GetByte(_dp);
				break;

			case 0x26:		// IYS
				_y++;
				_dp=_y;
				PutByte(_dp_a);
				break;

			case 0x27:		// DYS
				_y--;
				_dp=_y;
				PutByte(_dp_a);
				break;

			case 0x28:		// JRNZP n
				break;

			case 0x29:		// JRNZM n
				break;

			case 0x2a:		// JRNCP n
				break;

			case 0x2b:		// JRNCM n
				break;

			case 0x2c:		// JRP n
				break;

			case 0x2d:		// JRM n
				break;

			case 0x2e:	// 
				break;

			case 0x2f:	// LOOP n
				break;

			case 0x30:		// STP
				_p=_a;
				break;

			case 0x31:		// STQ
				_q=_a;
				break;

			case 0x32:		// STR
				_r=_a;
				break;

			case 0x33:   // NOPT
				break;
                                         
			case 0x34:   // PUSH
				_sp--;
				PutByte(_sp,_a);
				break;

			case 0x35:   // MV WP
				break;

			case 0x36:	// 
				break;

			case 0x37:	// RTN
				break;

			case 0x38:		// JRZP n
				break;

			case 0x39:		// JRZM n
				break;

			case 0x3a:		// JRCP n
				break;

			case 0x3b:		// JRCM n
				break;

			case 0x3c:	// 
			case 0x3d:	// 
			case 0x3e:	// 
			case 0x3f:	// 
				break;

			case 0x40:		// INCI
				_i++;
				break;

			case 0x41:		// DECI
				_i--;
				break;

			case 0x42:		// INCA
				_a++;
				break;

			case 0x43:		// DECA
				_a--;
				break;

			case 0x44:		// ADM
				res1.b.l=GetByte(_p);
				res2.b.l=_a;
				goto aggAdd;
				break;

			case 0x45:		// SBM
				res1.b.l=GetByte(_p);
				res2.b.l=_a;
				goto aggSub;
				break;

			case 0x46:		// ANMA
				res1.b.l=GetByte(_p);
				res2.b.l=_a;
				res3.b.l=res1.b.l & res.b.l;
				PutByte(_p,res3.b.l);
				goto aggFlagZ;
				break;

			case 0x47:		// ORMA
				res1.b.l=GetByte(_p);
				res2.b.l=_a;
				res3.b.l=res1.b.l | res.b.l;
				PutByte(_p,res3.b.l);
				goto aggFlagZ;
				break;

			case 0x48:		// INCK
				_k++;
				break;

			case 0x49:		// DECK
				_k--;
				break;

			case 0x4a:		// INCM (unofficial)
				_m++;
				break;

			case 0x4b:		// DECM (unofficial)
				_m--;
				break;

			case 0x4c:		// INA
				break;

			case 0x4d:		// NOPW
				break;

			case 0x4e:		// WAIT n
				break;

			case 0x4f:		// IPXL
				break;

			case 0x50:		// INCP
				_p++;
				break;

			case 0x51:		// DECP
				_p--;
				break;

			case 0x52:		// STD
				PutByte(_dp,_a);
				break;

			case 0x53:		// MVDM
				PutByte(_dp,GetByte(_p));
				break;

			case 0x54:		// MVMP
				break;

			case 0x55:		// MVMD
				PutByte(_p,GetByte(_dp));
				break;

			case 0x56:		// LDPC
				break;

			case 0x57:		// LDD
				_a=GetByte(_dp);
				break;

			case 0x58:		// SWP
				break;

			case 0x59:		// LDM
				_a=GetByte(_p);
				break;

			case 0x5a:		// SL
				break;

			case 0x5b:		// POP
				_a=GetByte(_sp);
				_sp++;
				break;

			case 0x5c:		// 
				break;

			case 0x5d:		// OUTA
				break;

			case 0x5e:		// 
				break;

			case 0x5f:		// OUTF
				break;

			case 0x60:		// ANIM n
				res1.b.l=GetByte(_p);
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l & res.b.l;
				PutByte(_p,res3.b.l);
				goto aggFlagZ;
				break;

			case 0x61:		// ORIM n
				res1.b.l=GetByte(_p);
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l | res.b.l;
				PutByte(_p,res3.b.l);
				goto aggFlagZ;
				break;

			case 0x62:		// TSIM n
				res1.b.l=GetByte(_p);
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l & res.b.l;
				goto aggFlagZ;
				break;

			case 0x63:		// CPIM n
				res1.b.l=GetByte(_p);
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l - res.b.l;
				goto aggFlag;
				break;

			case 0x64:		// ANIA n
				res1.b.l=_a;
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l & res.b.l;
				_a=res3.b.l;
				goto aggFlagZ;
				break;

			case 0x65:		// ORIA n
				res1.b.l=_a;
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l | res.b.l;
				_a=res3.b.l;
				goto aggFlagZ;
				break;

			case 0x66:		// TSIA n
				res1.b.l=_a;
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l & res.b.l;
				goto aggFlagZ;
				break;

			case 0x67:		// CPIA n
				res1.b.l=_a;
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l - res.b.l;
				goto aggFlag;
				break;

			case 0x68:		// NOPT
				break;

			case 0x69:		// CASE
				break;

			case 0x6a:		// NOPT
				break;

			case 0x6b:		// TEST n
				break;

			case 0x6c:		// 
			case 0x6d:		// 
			case 0x6e:		// 
				break;

			case 0x6f:		// IPXH
				break;

			case 0x70:		// ADIM n
				res1.b.l=GetByte(_p);
				res2.b.l=Pipe2.b.l;

aggAdd:
				res1.b.h=0;
				res2.b.h=0;

aggAdd16:
				res3.x=res1.x+res2.x;
				PutByte(_p,res3.b.l);
				goto aggFlag
				break;

			case 0x71:		// SBIM n
				res1.b.l=GetByte(_p);
				res2.b.l=Pipe2.b.l;

aggSub:
				res1.b.h=0;
				res2.b.h=0;

aggSub16:
				res3.x=res1.x-res2.x;
				PutByte(_p,res3.b.l);

aggFlag:
				_f.Carry=!!res3.b.h;
aggFlagZ:
				_f.Zero=!res3.b.l;
				break;

			case 0x72:		// RZ n
				break;

			case 0x73:		// RZ n
				break;

			case 0x74:		// ADIA n
				res1.b.l=_a;
				res2.b.l=Pipe2.b.l;

aggAddA:
				res1.b.h=0;
				res2.b.h=0;
				res3.x=res1.x+res2.x;
				_a=res3.b.l;

				goto aggFlag;
				break;

			case 0x75:		// SBIA n
				res1.b.l=_a;
				res2.b.l=Pipe2.b.l;

aggSubA:
				res1.b.h=0;
				res2.b.h=0;
				res3.x=res1.x-res2.x;
				_a=res3.b.l;
				goto aggFlag;
				break;

			case 0x76:		// RZ n
				break;

			case 0x77:		// RZ n
				break;

			case 0x78:		// CALL nm
				break;

			case 0x79:		// JP nm
				break;

			case 0x7a:		// SET knm
				break;

			case 0x7b:		// 
				break;

			case 0x7c:		// JPNZ nm
			case 0x7d:		// JPNC nm
			case 0x7e:		// JPZ nm
			case 0x7f:		// JPC nm
				break;


			case 0x80:		// LP 1
			case 0x81:
			case 0x82:
			case 0x83:
			case 0x84:
			case 0x85:
			case 0x86:
			case 0x87:
			case 0x88:
			case 0x89:
			case 0x8a:
			case 0x8b:
			case 0x8c:
			case 0x8d:
			case 0x8e:
			case 0x8f:
			case 0x90:
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95:
			case 0x96:
			case 0x97:
			case 0x98:
			case 0x99:
			case 0x9a:
			case 0x9b:
			case 0x9c:
			case 0x9d:
			case 0x9e:
			case 0x9f:
			case 0xa0:
			case 0xa1:
			case 0xa2:
			case 0xa3:
			case 0xa4:
			case 0xa5:
			case 0xa6:
			case 0xa7:
			case 0xa8:
			case 0xa9:
			case 0xaa:
			case 0xab:
			case 0xac:
			case 0xad:
			case 0xae:
			case 0xaf:
			case 0xb0:
			case 0xb1:
			case 0xb2:
			case 0xb3:
			case 0xb4:
			case 0xb5:
			case 0xb6:
			case 0xb7:
			case 0xb8:
			case 0xb9:
			case 0xba:
			case 0xbb:
			case 0xbc:
			case 0xbd:
			case 0xbe:
			case 0xbf:
				_p=Pipe1 & 0x3f;
				break;

			case 0xc0:		// INCJ
				_j++;
				break;

			case 0xc1:		// DECJ
				_j--;
				break;

			case 0xc2:		// INCB
				_b++;
				break;

			case 0xc3:		// DECB
				_b--;
				break;

			case 0xc4:		// ADCM
				res1.b.l=GetByte(_p);
				res2.b.l=_a+_f.Carry;
				goto aggAdd;
				break;

			case 0xc5:		// SBCM
				res1.b.l=GetByte(_p);
				res2.b.l=_a     +_f.Carry;		// VERIFICARE!
				goto aggSub;
				break;

			case 0xc6:		// TSMA
				break;

			case 0xc7:		// CPMA
				break;

			case 0xc8:		// INCL
				_l++;
				break;

			case 0xc9:		// DECL
				_l--;
				break;

			case 0xca:		// INCN	(unofficial)
				_n++;
				break;

			case 0xcb:		// DECN (unofficial)
				_n--;
				break;

			case 0xcc:		// INB
				break;

			case 0xcd:		// NOPW
				break;

			case 0xce:		// NOPT
				break;

			case 0xcf:		// 
				break;

			case 0xd0:		// SC
				break;

			case 0xd1:		// RC
				break;

			case 0xd2:		// SR
				break;

			case 0xd3:		// NOPW
				break;

			case 0xd4:		// ANID n
				res1.b.l=GetByte(_dp);
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l & res.b.l;
				PutByte(_dp,res3.b.l);
				goto aggFlagZ;
				break;

			case 0xd5:		// ORID n
				res1.b.l=GetByte(_dp);
				res2.b.l=Pipe2.b.l;
				res3.b.l=res1.b.l | res.b.l;
				PutByte(_dp,res3.b.l);
				break;

			case 0xd6:		// TSID n
				break;

			case 0xd7:		// SZ n
				break;

			case 0xd8:		// LEAVE
				PutByte(_sp,0);
				break;

			case 0xd9:		// NOPW
				break;

			case 0xda:		// EXAB
				i=_a;
				_a=_b;
				_b=i;
				break;

			case 0xdb:		// EXAM
				i=GetByte(_p);
				PutByte(_p,_a);
				_a=i;
				break;

			case 0xdc:		// 
				break;

			case 0xdd:		// OUTB
				break;

			case 0xde:		// 
				break;

			case 0xdf:		// OUTC
				break;


			case 0xe0:		// CAL 1n
			case 0xe1:
			case 0xe2:
			case 0xe3:
			case 0xe4:
			case 0xe5:
			case 0xe6:
			case 0xe7:
			case 0xe8:
			case 0xe9:
			case 0xea:
			case 0xeb:
			case 0xec:
			case 0xed:
			case 0xee:
			case 0xef:
			case 0xf0:
			case 0xf1:
			case 0xf2:
			case 0xf3:
			case 0xf4:
			case 0xf5:
			case 0xf6:
			case 0xf7:
			case 0xf8:
			case 0xf9:
			case 0xfa:
			case 0xfb:
			case 0xfc:
			case 0xfd:
			case 0xfe:
			case 0xff:
				break;
			
			}
		} while(!fExit);
	}


