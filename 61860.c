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
#define WORKING_REG regs1.b[((Pipe1 & 0x38) ^ 8) >> 3]      // la parte bassa/alta � invertita...
#define WORKING_REG2 regs1.b[(Pipe1 ^ 1) & 7]
#define WORKING_BITPOS (1 << ((Pipe2.b.l & 0x38) >> 3))
#define WORKING_BITPOS2 (1 << ((Pipe2.b.h & 0x38) >> 3))
    
	SWORD _pc=0;
	BYTE _sp=0;
	BYTE _q=0,_r=0;
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
				break;

			case 1:   // LIJ n
				break;

			case 2:   // LIA n
				break;

			case 3:   // LIB n
				break;

			case 4:	// IX
				break;

			case 5:	// DX
				break;

			case 6:	// IY
				break;

			case 7:	// DY
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
				break;

			case 0x11:	// LIDL n
				break;

			case 0x12:  // LIP n
				break;

			case 0x13:	// LIQ n
				break;

			case 0x14:  // ADB
				break;

			case 0x15:	// SBB
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
				break;
			case 0x21:		// LDQ
				break;
			case 0x22:		// LDR
				break;

			case 0x23:		// CLRA
				break;

			case 0x24:		// IXL
				break;

			case 0x25:		// DXL
				break;

			case 0x26:		// IY s
				break;

			case 0x27:		// DY s
				break;

			case 0x15:
			case 0x1d:
			case 0x25:
			case 0x2d:
			case 0x3d:
				WORKING_REG --;
        res3.b.l=WORKING_REG;
        
aggDec:
      	_f.Zero=res3.b.l ? 0 : 1;
        _f.Sign=res3.b.l & 0x80 ? 1 : 0;
      	_f.Parity= res3.b.l == 0x7f ? 1 : 0; //P = 1    // DEC x         P = 1 if x=80H before, else 0
      	_f.AuxCarry= (res3.b.l & 0xf) == 0xf ? 1 : 0; // DEC x      1 if borrow from bit 4 else 0 
				break;

			case 6:   // MVI B,n ecc
			case 0xe:
			case 0x16:
			case 0x1e:
			case 0x26:
			case 0x2e:
			case 0x3e:
			  WORKING_REG=Pipe2.b.l;
			  _pc++;
				break;

			case 7:   // RLC
				_f.Carry=_a & 0x80 ? 1 : 0;
				_a <<= 1;
				_a |= _f.Carry;
        
aggRotate:
        _f.AuxCarry=0;
				break;
                                         
			case 9:   // DAD H,B ecc
			case 0x19:
			case 0x29:
        res1.x=_H;
        res2.x=WORKING_REG16;
			  res3.d=(DWORD)res1.x+(DWORD)res2.x;
			  _H = res3.x;
        _f.AuxCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
        goto aggFlagWC;
        break;

			case 0xa:   // LDAX B
			  _a=GetValue(_B);
				break;

      case 0xb:   // DCX BC ecc
      case 0x1b:
      case 0x2b:
				WORKING_REG16 --;
				break;

			case 0xf:   // RRC
				_f.Carry=_a & 1;
				_a >>= 1;
				if(_f.Carry)
					_a |= 0x80;
        goto aggRotate;
				break;

			case 0x12:    // STAX D
			  PutValue(_D,_a);
				break;

      case 0x17:    // RAL
				_f1=_f;
				_f.Carry=_a & 0x80 ? 1 : 0;
				_a <<= 1;
				_a |= _f1.Carry;
        goto aggRotate;
				break;

			case 0x1a:    // LDAX D
			  _a=GetValue(_D);
				break;

			case 0x1f:    // RAR
				_f1=_f;
				_f.Carry=_a & 1;
				_a >>= 1;
				if(_f1.Carry)
					_a |= 0x80;
        goto aggRotate;
				break;

			case 0x22:    // SHLD nn
			  PutIntValue(Pipe2.x,_H);
			  _pc+=2;
				break;

			case 0x27:		// DAA
        res3.x=res1.x=_a;
        i=_f.Carry;
        _f.Carry=0;
        if((_a & 0xf) > 9 || _f.AuxCarry) {
          res3.x+=6;
          _a=res3.b.l;
          _f.Carry= i || HIBYTE(res3.x);
          _f.AuxCarry=1;
          }
        else
          _f.AuxCarry=0;
        if((res1.b.l>0x99) || i) {
          _a+=0x60;  
          _f.Carry=1;
          }
        else
          _f.Carry=0;
        goto calcParity;
				break;

			case 0x2a:    // LHLD (nn)
			  _H=GetIntValue(Pipe2.x);
			  _pc+=2;
				break;

  		case 0x2f:    // CMA
        _a=~_a;
        _f.AuxCarry=1;
				break;

			case 0x31:    // LXI SP,nn (v. anche H ecc)
			  _sp=Pipe2.x;
			  _pc+=2;
				break;

			case 0x32:    // STA (nn)
			  PutValue(Pipe2.x,_a);
			  _pc+=2;
				break;

			case 0x33:    // INX SP (v. anche INX B ecc)
			  _sp++;
				break;

			case 0x34:    // INR (H)
        res3.b.l=GetValue(_H)+1;
			  PutValue(_H,res3.b.l);
        goto aggInc;
				break;

			case 0x35:    // DCR (H)
        res3.b.l=GetValue(_H)-1;
			  PutValue(_H,res3.b.l);
        goto aggDec;
				break;

			case 0x36:    // MVI (H),n
			  PutValue(_H,Pipe2.b.l);
			  _pc++;
				break;
              
      case 0x37:    // STC
        _f.Carry=1;
        _f.AuxCarry=0;
        break;
                                   
			case 0x39:    // DAD SP
        res1.x=_H;
        res2.x=_sp;
			  res3.d=(DWORD)res1.x+(DWORD)res2.x;
        _H = res3.x;
        _f.AuxCarry = ((res1.x & 0xfff) + (res2.x & 0xfff)) >= 0x1000 ? 1 : 0;   // 
        goto aggFlagWC;
				break;

			case 0x3a:    // LDA (nn)
			  _a=GetValue(Pipe2.x);
			  _pc+=2;
				break;

      case 0x3b:    // DCX SP (v. anche DEC B ecc)
			  _sp--;
				break;

      case 0x3f:    // CMC
        _f.AuxCarry=_f.Carry;
        _f.Carry=!_f.Carry;
        break;
                                   
			case 0x40:    // MOV r,r
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x47:
			case 0x48:                             // 
			case 0x49:
			case 0x4a:
			case 0x4b:
			case 0x4c:
			case 0x4d:
			case 0x4f:
			case 0x50:                             // 
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x54:
			case 0x55:
			case 0x57:
			case 0x58:                             // 
			case 0x59:
			case 0x5a:
			case 0x5b:
			case 0x5c:
			case 0x5d:
			case 0x5f:
			case 0x60:                             // 
			case 0x61:
			case 0x62:
			case 0x63:
			case 0x64:
			case 0x65:
			case 0x67:
			case 0x68:                             // 
			case 0x69:
			case 0x6a:
			case 0x6b:
			case 0x6c:
			case 0x6d:
			case 0x6f:
			case 0x78:                             // 
			case 0x79:
			case 0x7a:
			case 0x7b:
			case 0x7c:
			case 0x7d:
			case 0x7f:
				WORKING_REG=WORKING_REG2;
				break;

			case 0x46:    // MOV r,(HL)
			case 0x4e:
			case 0x56:
			case 0x5e:
			case 0x66:
			case 0x6e:
			case 0x7e:
				WORKING_REG=GetValue(_H);
				break;

			case 0x70:                             // 
			case 0x71:
			case 0x72:
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x77:
				PutValue(_H,/* regs1.b[((Pipe1 & 7) +1) & 7]*/ WORKING_REG2);
				break;
        
			case 0x76:    // HLT
			  DoHalt=1;
				break;

			case 0x80:    // ADD r
			case 0x81:
			case 0x82:
			case 0x83:
			case 0x84:
			case 0x85:
			case 0x87:
        res2.b.l=WORKING_REG2;
        
aggSomma:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x+res2.x;
        _a=res3.b.l;
        _f.AuxCarry = ((res1.b.l & 0xf) + (res2.b.l & 0xf)) >= 0x10 ? 1 : 0;   // 

aggFlagB:
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x40) != !!(((res1.b.l & 0x80) + (res2.b.l & 0x80)) & 0x80);
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
  //(M^result)&(N^result)&0x80 is nonzero. That is, if the sign of both inputs is different from the sign of the result. (Anding with 0x80 extracts just the sign bit from the result.) 
  //Another C++ formula is !((M^N) & 0x80) && ((M^result) & 0x80)
//        _f.Parity = !!((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80);
//        _f.Parity = !!(!((res1.b.l ^ res2.b.l) & 0x80) && ((res1.b.l ^ res3.b.l) & 0x80));
//**        _f.Parity = ((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80) ? 1 : 0;
  // Calculate the overflow by sign comparison.
/*  carryIns = ((a ^ b) ^ 0x80) & 0x80;
  if (carryIns) // if addend signs are the same
  {
    // overflow if the sum sign differs from the sign of either of addends
    carryIns = ((*acc ^ a) & 0x80) != 0;
  }*/
	// per overflow e AuxCarry https://stackoverflow.com/questions/8034566/overflow-and-carry-flags-on-z80
/*The overflow checks the most significant bit of the 8 bit result. This is the sign bit. If we add two negative numbers (MSBs=1) then the result should be negative (MSB=1), whereas if we add two positive numbers (MSBs=0) then the result should be positive (MSBs=0), so the MSB of the result must be consistent with the MSBs of the summands if the operation was successful, otherwise the overflow bit is set.*/        
/*        if(!_f.Sign) {
          _f.Parity=(res1.b.l & 0x80 || res2.b.l & 0x80) ? 1 : 0;
          }
        else {
          _f.Parity=(res1.b.l & 0x80 && res2.b.l & 0x80) ? 1 : 0;
          }*/
/*        if(res1.b.l & 0x80 && res2.b.l & 0x80)
          _f.Parity=res3.b.l & 0x80 ? 0 : 1;
        else if(!(res1.b.l & 0x80) && !(res2.b.l & 0x80))
          _f.Parity=res3.b.l & 0x80 ? 1 : 0;
        else
          _f.Parity=0;*/
        _f.Parity = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
        
aggFlagBC:    // http://www.z80.info/z80sflag.htm
				_f.Carry=!!res3.b.h;
        
        _f.Zero=res3.b.l ? 0 : 1;
        _f.Sign=res3.b.l & 0x80 ? 1 : 0;
				break;

			case 0x86:    // ADD (HL)
        res2.b.l=GetValue(_H);
        goto aggSomma;
				break;

			case 0x88:    // ADC r
			case 0x89:
			case 0x8a:
			case 0x8b:
			case 0x8c:
			case 0x8d:
			case 0x8f:
        res2.b.l=WORKING_REG2;
        
aggSommaC:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x+res2.x+_f.Carry;
        _a=res3.b.l;
        _f.AuxCarry = ((res1.b.l & 0xf) + (res2.b.l & 0xf)) >= 0x10 ? 1 : 0;   // 
//#warning CONTARE IL CARRY NELL overflow?? no, pare di no (v. emulatore ma io credo di s�
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
/*        if(res1.b.l & 0x80 && res2.b.l & 0x80)
          _f.Parity=res3.b.l & 0x80 ? 0 : 1;
        else if(!(res1.b.l & 0x80) && !(res2.b.l & 0x80))
          _f.Parity=res3.b.l & 0x80 ? 1 : 0;
        else
          _f.Parity=0;*/
        goto aggFlagB;
				break;

			case 0x8e:    // ADC A,(HL)
        res2.b.l=GetValue(_H);
        goto aggSommaC;
				break;

			case 0x90:    // SUB r
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95:
			case 0x97:
        res2.b.l=WORKING_REG2;
        
aggSottr:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x-res2.x;
        _a=res3.b.l;
        _f.AuxCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0 ? 1 : 0;   // 
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
//        _f.Parity = ((res1.b.l ^ res3.b.l) & (res2.b.l ^ res3.b.l) & 0x80) ? 1 : 0;
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.Parity=1;
          else
            _f.Parity=0;
          }
        else
          _f.Parity=0;*/
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.Parity=1;
          else
            _f.Parity=0;
          }
        else
          _f.Parity=0;*/
        _f.Parity = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
  			goto aggFlagBC;
				break;

			case 0x96:    // SUB (HL)
        res2.b.l=GetValue(_H);
				goto aggSottr;
				break;

			case 0x98:    // SBB r
			case 0x99:
			case 0x9a:
			case 0x9b:
			case 0x9c:
			case 0x9d:
			case 0x9f:
        res2.b.l=WORKING_REG2;
        
aggSottrC:
				res1.b.l=_a;
        res1.b.h=res2.b.h=0;
        res3.x=res1.x-res2.x-_f.Carry;
        _a=res3.b.l;
        _f.AuxCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0  ? 1 : 0;   // 
//#warning CONTARE IL CARRY NELL overflow?? no, pare di no (v. emulatore ma io credo di s�..
//        _f.Parity = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.Parity=1;
          else
            _f.Parity=0;
          }
        else
          _f.Parity=0;*/
        goto aggFlagB;
				break;

			case 0x9e:    // SBB (HL)
        res2.b.l=GetValue(_H);
				goto aggSottrC;
				break;

			case 0xa0:    // ANA r
			case 0xa1:
			case 0xa2:
			case 0xa3:
			case 0xa4:
			case 0xa5:
			case 0xa7:
				_a &= WORKING_REG2;
        _f.AuxCarry=1;
        
aggAnd:
        res3.b.l=_a;
aggAnd2:
        _f.Carry=0;
aggAnd3:      // usato da IN 
        _f.Zero=_a ? 0 : 1;
        _f.Sign=_a & 0x80 ? 1 : 0;
        // AuxCarry � 1 fisso se AND e 0 se OR/XOR
        
calcParity:
          {
          BYTE par;
          par= res3.b.l >> 1;			// Microchip AN774
          par ^= res3.b.l;
          res3.b.l= par >> 2;
          par ^= res3.b.l;
          res3.b.l= par >> 4;
          par ^= res3.b.l;
          _f.Parity=par & 1 ? 1 : 0;
          }
				break;

			case 0xa6:    // ANA (HL)
				_a &= GetValue(_H);
        _f.AuxCarry=1;
        goto aggAnd;
				break;

			case 0xa8:    // XRA r
			case 0xa9:
			case 0xaa:
			case 0xab:
			case 0xac:
			case 0xad:
			case 0xaf:
				_a ^= WORKING_REG2;
        _f.AuxCarry=0;
        goto aggAnd;
				break;

			case 0xae:    // XRA (HL)
				_a ^= GetValue(_H);
        _f.AuxCarry=0;
        goto aggAnd;
				break;

			case 0xb0:    // ORA r
			case 0xb1:
			case 0xb2:
			case 0xb3:
			case 0xb4:
			case 0xb5:
			case 0xb7:
				_a |= WORKING_REG2;
        _f.AuxCarry=0;
        goto aggAnd;
				break;

			case 0xb6:    // ORA (HL)
				_a |= GetValue(_H);
        _f.AuxCarry=0;
        goto aggAnd;
				break;

			case 0xb8:    // CMP r
			case 0xb9:
			case 0xba:
			case 0xbb:
			case 0xbc:
			case 0xbd:
			case 0xbf:
				res2.b.l=WORKING_REG2;
				goto compare;
				break;

			case 0xbe:    // CMP (HL)
				res2.b.l=GetValue(_H);
  			goto compare;
				break;

			case 0xc0:    // RNZ
			  if(!_f.Zero)
			    goto Return;
				break;

			case 0xc1:    // POP B ecc
			case 0xd1:    
			case 0xe1:    
#define WORKING_REG16B regs1.r[(Pipe1 & 0x30) >> 4].b
				WORKING_REG16B.l=GetValue(_sp++);
				WORKING_REG16B.h=GetValue(_sp++);
				break;
			case 0xf1:    
				_f.b=_f_PSW=GetValue(_sp++);
				_a=GetValue(_sp++);
				break;

			case 0xc2:    // JNZ
			  if(!_f.Zero)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xc3:    // JMP
			case 0xcb:
Jump:
				_pc=Pipe2.x;
				break;

			case 0xc4:    // CNZ
			  if(!_f.Zero)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xc5:    // PUSH B ecc
			case 0xd5:    // 
			case 0xe5:    // 
				PutValue(--_sp,WORKING_REG16B.h);
				PutValue(--_sp,WORKING_REG16B.l);
				break;
			case 0xf5:    // push af..
        _f_PSW=_f.b;
				PutValue(--_sp,_a);
				PutValue(--_sp,_f_PSW);
				break;

			case 0xc6:    // ADI n
			  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSomma;
				break;

			case 0xc7:    // RST
			case 0xcf:
			case 0xd7:
			case 0xdf:
			case 0xe7:
			case 0xef:
			case 0xf7:
			case 0xff:
			  i=Pipe1 & 0x38;
RST:
				PutValue(--_sp,HIBYTE(_pc));
				PutValue(--_sp,LOBYTE(_pc));
				_pc=i;
				break;
				
			case 0xc8:    // RZ
			  if(_f.Zero)
			    goto Return;
				break;

			case 0xc9:    // RET
			case 0xd9:    // 
Return:
        _pc=GetValue(_sp++);
        _pc |= ((SWORD)GetValue(_sp++)) << 8;
				break;

			case 0xca:    // JZ
			  if(_f.Zero)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xcc:    // CZ
			  if(_f.Zero)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xcd:		// CALL
			case 0xdd:
			case 0xed:      // 
			case 0xfd:
Call:
				i=Pipe2.x;
		    _pc+=2;
				goto RST;
				break;

			case 0xce:    // ACI n
			  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSommaC;
				break;

			case 0xd0:    // RNC
			  if(!_f.Carry)
			    goto Return;
				break;

			case 0xd2:    // JNC
			  if(!_f.Carry)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xd3:    // OUT
				OutValue(MAKEWORD(Pipe2.b.l,_a),_a);
				_pc++;
				break;

			case 0xd4:    // CNC
			  if(!_f.Carry)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xd6:    // SUI n
  		  res2.b.l=Pipe2.b.l;
			  _pc++;
        goto aggSottr;
				break;

			case 0xd8:    // RC
			  if(_f.Carry)
			    goto Return;
				break;

			case 0xda:    // JC
			  if(_f.Carry)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xdb:    // IN a,  NON tocca flag
				_pc++;
				_a=InValue(MAKEWORD(Pipe2.b.l,_a));
				break;

			case 0xdc:    // CC
			  if(_f.Carry)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xde:    // SBI n
				_pc++;
				res2.b.l=Pipe2.b.l;
        goto aggSottrC;
				break;

			case 0xe0:    // RPO
			  if(!_f.Parity)
			    goto Return;
				break;

			case 0xe2:    // JPO
			  if(!_f.Parity)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xe3:    // XTHL
				res3.x=GetIntValue(_sp);
				PutIntValue(_sp,_H);
				_H=res3.x;
				break;

			case 0xe4:    // CPO
			  if(!_f.Parity)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xe6:    // ANI n
				_a &= Pipe2.b.l;
        _f.AuxCarry=1;
				_pc++;
        goto aggAnd;
				break;

			case 0xe8:    // RPE
			  if(_f.Parity)
			    goto Return;
				break;

			case 0xe9:    // PCHL
			  _pc=_H;
				break;

			case 0xea:    // JPE
			  if(_f.Parity)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xeb:    // XCHG
				res3.x=_D;
				_D=_H;
				_H=res3.x;
				break;

			case 0xec:    // CPE
			  if(_f.Parity)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xee:    // XRI n
				_a ^= Pipe2.b.l;
        _f.AuxCarry=0;
				_pc++;
        goto aggAnd;
				break;

			case 0xf0:    // RP
			  if(!_f.Sign)
			    goto Return;
				break;

			case 0xf2:    // JP
			  if(!_f.Sign)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xf3:    // DI
			  IRQ_Enable1=IRQ_Enable2=0;
				break;

			case 0xf4:    // CP
			  if(!_f.Sign)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xf6:    // ORI n
				_a |= Pipe2.b.l;
        _f.AuxCarry=0;
				_pc++;
        goto aggAnd;
				break;

			case 0xf8:    // RM
			  if(_f.Sign)
			    goto Return;
				break;

			case 0xf9:    // SPHL
			  _sp=_H;
				break;

			case 0xfa:    // JM
			  if(_f.Sign)
			    goto Jump;
			  else
			    _pc+=2;
				break;

			case 0xfb:    // EI
			  IRQ_Enable1=IRQ_Enable2=1;
				break;

			case 0xfc:    // CM
			  if(_f.Sign)
			    goto Call;
			  else
			    _pc+=2;
				break;

			case 0xfe:    // CPI n
				res2.b.l=Pipe2.b.l;
				_pc++;
        
compare:        
				res1.b.l=_a;
				res1.b.h=res2.b.h=0;
				res3.x=res1.x-res2.x;
        _f.AuxCarry = ((res1.b.l & 0xf) - (res2.b.l & 0xf)) & 0xf0 ? 1 : 0;   // 
/*        if((res1.b.l & 0x80) != (res2.b.l & 0x80)) {
          if(((res1.b.l & 0x80) && !(res3.b.l & 0x80)) || (!(res1.b.l & 0x80) && (res3.b.l & 0x80)))
            _f.Parity=1;
          else
            _f.Parity=0;
          }
        else
          _f.Parity=0;*/
        _f.Parity = !!res3.b.h != !!((res3.b.l & 0x80) ^ (res1.b.l & 0x80) ^ (res2.b.l & 0x80));
  			goto aggFlagBC;
				break;
			
			}
		} while(!fExit);
	}


