#include "stm8s.h"

#include "bike.h"
#include "TM16XX.h"

#define TM16XX_PORT	GPIOC
#define TM16XX_CLK	GPIO_PIN_0
#define TM16XX_DAT	GPIO_PIN_1
//#define TM16XX_CLK	GPIO_PIN_5
//#define TM16XX_DAT	GPIO_PIN_6
//#define TM16XX_CS	GPIO_PIN_7

#define CLK_SET()	TM16XX_PORT->ODR  |= TM16XX_CLK
#define CLK_CLR()	TM16XX_PORT->ODR  &=~TM16XX_CLK
#define DAT_SET()	TM16XX_PORT->ODR  |= TM16XX_DAT
#define DAT_CLR()	TM16XX_PORT->ODR  &=~TM16XX_DAT
//#define CS_SET()	TM16XX_PORT->ODR  |= TM16XX_CS
//#define CS_CLR()	TM16XX_PORT->ODR  &=~TM16XX_CS

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
	unsigned char i,dat;
	
	GPIO_Init(TM16XX_PORT, TM16XX_CLK, GPIO_MODE_OUT_PP_HIGH_FAST);
	GPIO_Init(TM16XX_PORT, TM16XX_DAT, GPIO_MODE_OUT_PP_HIGH_FAST);
	//GPIO_Init(TM16XX_PORT, TM16XX_CS , GPIO_MODE_OUT_PP_HIGH_FAST);
	
	switch(st){
		case 0:	dat = 0x00; break;	
		case 1:	dat = 0xFF; break;	
		default: break;
	}
	for(i=0;i<16;i++)	TM16XX[i] = dat;
#ifdef TM1624	
	TM1624_Write_Data(TM16XX,14);
#elif defined TM1640
	TM1640_Write_Data(TM16XX,16);
#endif
}

void TM16XX_WriteReg( unsigned char reg )
{
	unsigned char i;
	for(i=0;i<8;i++) {
		if((reg>>i)&0x01)	DAT_SET();
		else				DAT_CLR();
		asm("nop");
		CLK_CLR();asm("nop");
		CLK_SET();asm("nop");
		asm("nop"); 
	}
}

#ifdef TM1624	

void TM1624_Write_Data(unsigned char* buf,unsigned char len)
{
	unsigned char i;
	CS_CLR();
	TM16XX_WriteReg(0x03);
	CS_SET();
	
	CS_CLR();
	TM16XX_WriteReg(CONFIG_CMD|ADDR_AUTO);
	CS_SET();
	
	CS_CLR();
	TM16XX_WriteReg(ADDRESS_CMD|0x00);
	
	for(i=0;i<len;i++)
		TM16XX_WriteReg(buf[i]);
	CS_SET();
	
	CS_CLR();
	TM16XX_WriteReg(DISPLAY_CMD|DISPLAY_14);
	CS_SET();
}
#elif defined TM1640
void TM1640_Write_Data(unsigned char* buf,unsigned char len)
{
	unsigned char i;
	CLK_SET();
	DAT_SET();
	CLK_CLR();
	TM16XX_WriteReg(CONFIG_CMD|ADDR_AUTO);
	CLK_CLR();
	DAT_CLR();
	CLK_SET();
	DAT_SET();
	DAT_CLR();
	CLK_CLR();
		
	TM16XX_WriteReg(ADDRESS_CMD|0x00);
	
	for(i=0;i<len;i++)
		TM16XX_WriteReg(buf[i]);
	
	CLK_CLR();
	DAT_CLR();
	CLK_SET();
	DAT_SET();
	DAT_CLR();
	CLK_CLR();

	TM16XX_WriteReg(DISPLAY_CMD|DISPLAY_14);
	CLK_CLR();
	DAT_CLR();
	CLK_SET();
	DAT_SET();
}
#endif

