#include "TM16XX.h"
#include "bike.h"

#define TM16XX_PORT	GPIOC
#define TM16XX_CLK	GPIO_PIN_0
#define TM16XX_DAT	GPIO_PIN_1

#define CLK_SET()	TM16XX_PORT->ODR  &=~TM16XX_CLK;
#define CLK_CLR()	TM16XX_PORT->ODR  |= TM16XX_CLK;
#define DAT_SET()	TM16XX_PORT->ODR  |= TM16XX_DAT;
#define DAT_CLR()	TM16XX_PORT->ODR  &=~TM16XX_DAT;

#define CONFIG_CMD	0x40
#define ADDR_AUTO	0x00
#define ADDR_CONST	0x04

#define DISPLAY_CMD	0x80
#define DISPLAY_ON	0x08
#define DISPLAY_OFF	0x00
#define DISPLAY_14	0x0F

#define ADDRESS_CMD	0xC0

unsigned char TM16XX[16];


void TM16XX_Init(unsigned char st)
{
	unsigned char i;
	
	GPIO_Init(TM16XX_PORT, TM16XX_CLK, GPIO_MODE_OUT_OD_HIZ_SLOW);
	GPIO_Init(TM16XX_PORT, TM16XX_DAT, GPIO_MODE_OUT_OD_HIZ_SLOW);
	
	for(i=0;i<16;i++)	TM16XX[i] = 0;
	TM16XX_Write_Data(TM16XX,16);
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

	TM16XX_WriteReg(CONFIG_CMD|ADDR_AUTO);
	TM16XX_WriteReg(ADDRESS_CMD|0x00);
	
	for(i=0;i<len;i++)
		TM16XX_WriteReg(buf[i]);
	TM16XX_WriteReg(DISPLAY_CMD|DISPLAY_14);
}

