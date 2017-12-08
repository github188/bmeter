#include "TM16XX.h"

#define CLK_SET()	PC_ODR  &=~0x20;
#define CLK_CLR()	PC_ODR  |=0x20;
#define DAT_SET()	PC_ODR  |=0x40;
#define DAT_CLR()	PC_ODR  &=~0x40;


void TM16XX_Init(unsigned char st)
{
	
}

void TM16XX_WriteReg( unsigned char reg )
{
	unsigned char i;
	for(i=0;i<8;i++) {
		if((reg>>i)&0x01)	DAT_SET();
		else				DAT_CLR();
		asm("nop");
		CLK_CLR;asm("nop");  
		CLK_SET;asm("nop"); asm("nop"); asm("nop"); asm("nop"); 
	}
}


void TM16XX_Write_Data(unsigned char* buf,unsigned char len)
{
	unsigned char i;

	TM16XX_WriteReg(0x44);	//
	
	for(i=0;i<len;i++)
		TM16XX_WriteReg(buf[i]);
}