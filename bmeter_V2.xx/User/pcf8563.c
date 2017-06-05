
#include "pcf8563.h"
#include "stm8s.h"
#include "bl55072.h"

#define HOUR   16
#define MIN    24


/******************************************************************************
                            ?¡§¨°??¨¤1?¦Ì?¡À?¨¢?o¡¥¨ºy
******************************************************************************/

unsigned char buffer[6];
_PCF8563_Time_Typedef 	RtcTime;
_PCF8563_Date_Typedef   RtcDate;


#define RTC_BinToBcd2(BINValue) ( ((BINValue/10) << 4) | (BINValue%10))

unsigned char RTC_Bcd2ToBin(unsigned char BCDValue)
{
	return ((BCDValue>>4)*10 + (BCDValue & 0x0F));
}

/**
  *****************************************************************************
  * @Name   : PCF8563?3??¡ä??¡ÂD¡ä¨¨?¨°???¡Á??¨²¨ºy?Y
  *
  * @Brief  : none
  *
  * @Input  : REG_ADD¡êo¨°a2¨´¡Á¡Â??¡ä??¡Â¦Ì??¡¤
  *           dat¡êo    ¨°aD¡ä¨¨?¦Ì?¨ºy?Y
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_Write_Byte(unsigned char REG_ADD, unsigned char dat)
{
	unsigned char buf[2];
	
	buf[0] = REG_ADD;
	buf[1] = dat;
	I2C_WriteBuf(PCF8563_Write, buf, 2);
} 

/**
  *****************************************************************************
  * @Name   : PCF8563?3??¡ä??¡Â?¨¢¨¨?¨°???¡Á??¨²¨ºy?Y
  *
  * @Brief  : none
  *
  * @Input  : REG_ADD¡êo¨°a2¨´¡Á¡Â??¡ä??¡Â¦Ì??¡¤
  *
  * @Output : none
  *
  * @Return : ?¨¢¨¨?¦Ì?¦Ì?¦Ì???¡ä??¡Â¦Ì??¦Ì
  *****************************************************************************
**/
unsigned char PCF8563_Read_Byte(unsigned char REG_ADD)
{
	//unsigned char ReData;
	unsigned char buf[2];
	
	I2C_ReadBuf(PCF8563_Write, REG_ADD, buf, 1);
	return buf[0];
}


/**
  *****************************************************************************
  * @Name   : PCF8563?¨¬2a¨º?¡¤?¡ä??¨²
  *
  * @Brief  : ?¨°?¡§¨º¡À?¡Â¦Ì1??¨º¡À??¡ä??¡ÂD¡ä¨¨?¨°???¨ºy?¦Ì?¨´?¨¢¨¨?3?¨¤¡ä¡Á???¡À¨¨¡ê??¨¤¨ª??y¨¨¡¤¡ê?2?¨ª??¨°¡ä¨ª?¨®
  *
  * @Input  : none
  *
  * @Output : none
  *
  * @Return : 0: ?y3¡ê
  *           1: PCF8563¡ä¨ª?¨®?¨°???e?¦Ì
  *****************************************************************************
**/
unsigned char PCF8563_Check(void)
{
	//unsigned char test_value = 0;
	
	// PCF8563_Write_Byte(PCF8563_Address_Timer, PCF_Timer_Close);  
	// PCF8563_Write_Byte(PCF8563_Address_Timer_VAL, PCF8563_Check_Data);  
	// test_value = PCF8563_Read_Byte(PCF8563_Address_Timer_VAL);  
	// if(test_value != PCF8563_Check_Data)  return 1;  
	// PCF8563_Write_Byte(PCF8563_Address_Control_Status_1, 0);  //run
	
	buffer[0] = PCF8563_Address_Seconds;
	buffer[1] = 0x55;
	buffer[2] = RtcTime.RTC_Minutes;
	buffer[3] = RtcTime.RTC_Hours;
	I2C_WriteBuf(PCF8563_Write, buffer, 4);
	I2C_ReadBuf(PCF8563_Write, PCF8563_Address_Seconds,buffer, 3);

	//PCF8563_Write_Byte(PCF8563_Address_Seconds, 0x55);  
	I2C_ReadBuf(PCF8563_Write, PCF8563_Address_Seconds,buffer, 3);
	I2C_ReadBuf(PCF8563_Write, 0,buffer, 0x0f);
	//test_value = PCF8563_Read_Byte(PCF8563_Address_Seconds);  

	return 0;
}

/**
  *****************************************************************************
  * @Name   : PCF8563???¡¥
  *
  * @Brief  : none
  *
  * @Input  : none
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_Start(void)
{
	unsigned char temp = 0;
	
	temp = PCF8563_Read_Byte(PCF8563_Address_Control_Status_1);  //?¨¢¨¨?????/¡Á¡ä¨¬???¡ä??¡Â1
	if (temp & PCF_Control_ChipStop)
	{
		temp &= PCF_Control_ChipRuns;  //??DDD???
	}
	if ((temp & (1<<7)) == 0)  //??¨ª¡§?¡ê¨º?
	{
		temp &= PCF_Control_TestcClose;  //¦Ì??¡ä?¡ä???¡ê¨º?¨º¡ìD¡ì
	}
	PCF8563_Write_Byte(PCF8563_Address_Control_Status_1, temp);  //?¨´D¡ä¨¨?¨ºy?¦Ì
}

/**
  *****************************************************************************
  * @Name   : PCF8563¨ª¡ê?1
  *
  * @Brief  : ¨º¡À?¨®?¦Ì?¨º¨º?3?CLKOUT ?¨² 32.768kHz ¨º¡À?¨¦¨®?
  *
  * @Input  : none
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_Stop(void)
{
	unsigned char temp = 0;
	
	temp = PCF8563_Read_Byte(PCF8563_Address_Control_Status_1);  //?¨¢¨¨?????/¡Á¡ä¨¬???¡ä??¡Â1
	temp |= PCF_Control_ChipStop;  //¨ª¡ê?1??DD
	PCF8563_Write_Byte(PCF8563_Address_Control_Status_1, temp);  //?¨´D¡ä¨¨?¨ºy?¦Ì
}

/**
  *****************************************************************************
  * @Name   : PCF8563¨¦¨¨????DD?¡ê¨º?
  *
  * @Brief  : none
  *
  * @Input  : Mode: ??DD?¡ê¨º?
  *                 = PCF_Mode_Normal
  *                 = PCF_Mode_EXT_CLK
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_SetMode(unsigned char Mode)
{
	unsigned char temp = 0;
	
	temp = PCF8563_Read_Byte(PCF8563_Address_Control_Status_1);  //?¨¢¨¨???¡ä??¡Â?¦Ì
	if (Mode == PCF_Mode_EXT_CLK)  //EXT_CLK2a¨º??¡ê¨º?
	{
		temp |= PCF_Control_Status_EXT_CLKMode;
	}
	else if (Mode == PCF_Mode_Normal)
	{
		temp &= PCF_Control_Status_NormalMode;
		temp &= ~(1<<3);  //¦Ì??¡ä?¡ä??1|?¨¹¨º¡ìD¡ì
	}
	PCF8563_Write_Byte(PCF8563_Address_Control_Status_1, temp);
}

/**
  *****************************************************************************
  * @Name   : PCF8563¨¦¨¨??¦Ì??¡ä?¡ä??1|?¨¹?a??¨®?1?¡À?
  *
  * @Brief  : none
  *
  * @Input  : NewState: ¡Á¡ä¨¬?¡ê?PCF8563_PowerResetEnablePCF8563_PowerResetDisable
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_SetPowerReset(unsigned char NewState)
{
	unsigned char TestC = 0;
	
	TestC = PCF8563_Read_Byte(PCF8563_Address_Control_Status_1);  //??¨¨???¡ä??¡Â?¦Ì
	TestC &= ~(1<<3);  //??3y???¡ã¨¦¨¨??
	if (NewState == PCF8563_PowerResetEnable)  //?¡ä??1|?¨¹¨®DD¡ì
	{
		TestC |= PCF8563_PowerResetEnable;
	}
	else if (NewState == PCF8563_PowerResetDisable)
	{
		TestC &= ~PCF8563_PowerResetEnable;  //¨º¡ìD¡ì¡ê???¨ª¡§?¡ê¨º?¨º??¦Ì???-0¡ê??¡ä¨º¡ìD¡ì
	}
	PCF8563_Write_Byte(PCF8563_Address_Control_Status_1, TestC);  //D¡ä¨¨?¨ºy?¦Ì
}

/**
  *****************************************************************************
  * @Name   : PCF8563¨¦¨¨??¨º?3??¦Ì?¨º
  *
  * @Brief  : none
  *
  * @Input  :*PCF_CLKOUTStruct: ?¦Ì?¨º?¨¢11????
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_SetCLKOUT(_PCF8563_CLKOUT_Typedef* PCF_CLKOUTStruct)
{
	unsigned char tmp = 0;
	
	tmp = PCF8563_Read_Byte(PCF8563_Address_CLKOUT);  //?¨¢¨¨???¡ä??¡Â?¦Ì
	tmp &= 0x7c;  //??3y???¡ã¨¦¨¨??
	if (PCF_CLKOUTStruct->CLKOUT_NewState == PCF_CLKOUT_Open)
	{
		tmp |= PCF_CLKOUT_Open;
	}
	else
	{
		tmp &= PCF_CLKOUT_Close;
	}
	tmp |= PCF_CLKOUTStruct->CLKOUT_Frequency;
	
	PCF8563_Write_Byte(PCF8563_Address_CLKOUT, tmp);
}

/**
  *****************************************************************************
  * @Name   : PCF8563¨¦¨¨???¡§¨º¡À?¡Â
  *
  * @Brief  : none
  *
  * @Input  :*PCF_TimerStruct: ?¡§¨º¡À?¡Â?¨¢11????
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_SetTimer(_PCF8563_Timer_Typedef* PCF_TimerStruct)
{
	unsigned char Timer_Ctrl = 0;
	unsigned char Timer_Value = 0;
	
	Timer_Ctrl = PCF8563_Read_Byte(PCF8563_Address_Timer);  //??¦Ì???????¡ä??¡Â?¦Ì
	Timer_Value = PCF8563_Read_Byte(PCF8563_Address_Timer_VAL);  //??¨¨?¦Ì1??¨º¡À¨ºy?¦Ì
	//
	//?¨¨¨ª¡ê?1?¡§¨º¡À?¡Â
	//
	Timer_Ctrl &= PCF_Timer_Close;
	PCF8563_Write_Byte(PCF8563_Address_Timer, Timer_Ctrl);
	
	Timer_Ctrl &= 0x7c;  //??3y?¡§¨º¡À?¡Â???¡ã¨¦¨¨??
	
	if (PCF_TimerStruct->RTC_Timer_NewState == PCF_Timer_Open)  //?a??
	{
		Timer_Ctrl |= PCF_Timer_Open;
		Timer_Ctrl |= PCF_TimerStruct->RTC_Timer_Frequency;  //¨¬?¨¦?D?¦Ì?1¡è¡Á¡Â?¦Ì?¨º
		if (PCF_TimerStruct->RTC_Timer_Value)  //D¨¨¨°a¨¬?¨¦?D?¦Ì???¨ºy?¦Ì
		{
			Timer_Value = PCF_TimerStruct->RTC_Timer_Value;  //¨¬?¨¦?D?¦Ì???¨ºy?¦Ì
		}
	}
	else
	{
		Timer_Ctrl &= PCF_Timer_Close;
	}
	PCF8563_Write_Byte(PCF8563_Address_Timer_VAL, Timer_Value);  //D¡ä¨¨?¦Ì1??¨º¡À¨ºy?¦Ì
	
	if (PCF_TimerStruct->RTC_Timer_Interrupt == PCF_Time_INT_Open)  //?a??¨¢??D??¨º?3?
	{
		Timer_Value = PCF8563_Read_Byte(PCF8563_Address_Control_Status_2);  //??¨¨?????/¡Á¡ä¨¬???¡ä??¡Â2¨ºy?¦Ì
		Timer_Value &= PCF_Time_INT_Close;  //??3y?¡§¨º¡À?¡Â?D??¨º1?¨¹
		Timer_Value &= ~(1<<2);  //??3y?¡§¨º¡À?¡Â?D??¡À¨º??
		Timer_Value &= ~(1<<4);  //¦Ì¡À TF ¨®DD¡ì¨º¡À INT ¨®DD¡ì (¨¨???¨®¨² TIE ¦Ì?¡Á¡ä¨¬?) 
		Timer_Value |= PCF_Time_INT_Open;  //?a???¡§¨º¡À?¡Â?D??¨º?3?
		PCF8563_Write_Byte(PCF8563_Address_Control_Status_2, Timer_Value);
	}
	else
	{
		Timer_Value = PCF8563_Read_Byte(PCF8563_Address_Control_Status_2);  //??¨¨?????/¡Á¡ä¨¬???¡ä??¡Â2¨ºy?¦Ì
		Timer_Value &= PCF_Time_INT_Close;  //??3y?¡§¨º¡À?¡Â?D??¨º1?¨¹
		Timer_Value |= PCF_Time_INT_Open;  //?a???¡§¨º¡À?¡Â?D??¨º?3?
		PCF8563_Write_Byte(PCF8563_Address_Control_Status_2, Timer_Value);
	}
	
	PCF8563_Write_Byte(PCF8563_Address_Timer, Timer_Ctrl);  //¨¦¨¨???¡§¨º¡À?¡Â??????¡ä??¡Â
}

/**
  *****************************************************************************
  * @Name   : ¨¦¨¨??¨º¡À??¡ê??¡Â¨°a¨®?¨®¨²o¨®¨¬¡§¦Ì¡Â¨®?¡ê??¨°??3?¨º??¡¥¨º¡À??¨®?
  *
  * @Brief  : ????¨¨??¨ª¨¦¨¨??3¨¦0x00¨¢?¡ê?D?2?¨¤???2?¨¬???¡ê?D??¨²?¦Ì¡¤??¡ì¡êo0 ~ 6
  *
  * @Input  : PCF_Format:  ¨ºy?Y??¨º?
  *                        = PCF_Format_BIN
  *                        = PCF_Format_BCD
  *           PCF_Century: ¨º¨¤?¨ª??¨¦¨¨?¡§
  *                        = PCF_Century_19xx
  *                        = PCF_Century_20xx
  *           Year:        ?¨º
  *           Month:       ??
  *           Date:        ¨¨?
  *           Week:        D??¨²
  *           Hour:        ¨º¡À
  *           Minute:      ¡¤?
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_Set_Times(unsigned char PCF_Format,\
                       unsigned char PCF_Century,\
	                   unsigned char Year, unsigned char Month, unsigned char Date, unsigned char Week,\
                       unsigned char Hour, unsigned char Minute)
{
	_PCF8563_Time_Typedef Time_InitStructure;
	_PCF8563_Date_Typedef Date_InitStructure;
	
	if (PCF_Format == PCF_Format_BIN)
	{
		//
		//?D??¨ºy?Y¨º?¡¤?¡¤?o?¡¤??¡ì
		//
		if (Year > 99)   Year  = 0;  //???¡ä00?¨º
		if (Month > 12)  Month = 1;  //???¡ä1??
		if (Date > 31)   Date  = 1;  //???¡ä1¨¨?
		if (Week > 6)    Week  = 1;  //???¡äD??¨²¨°?
		
		if (Hour > 23)    Hour   = 0;  //???¡ä0D?¨º¡À
		if (Minute > 59)  Minute = 0;  //???¡ä0¡¤??¨®
		
		//
		//¡Áa??¨°???
		//
		Date_InitStructure.RTC_Years    = RTC_BinToBcd2(Year);
		Date_InitStructure.RTC_Months   = RTC_BinToBcd2(Month);
		Date_InitStructure.RTC_Days     = RTC_BinToBcd2(Date);
		Date_InitStructure.RTC_WeekDays = RTC_BinToBcd2(Week);
		
		Time_InitStructure.RTC_Hours    = RTC_BinToBcd2(Hour);
		Time_InitStructure.RTC_Minutes  = RTC_BinToBcd2(Minute);
	}
	Time_InitStructure.RTC_Seconds = 0x00;  //???¡ä0??
	Time_InitStructure.RTC_Seconds &= PCF_Accuracy_ClockYes;  //¡À¡ê?¡è¡Á?¨¨¡¤¦Ì?¨º¡À??
	//
	//?D??¨º¨¤?¨ª??
	//
	if (PCF_Century == PCF_Century_19xx)
	{
		Date_InitStructure.RTC_Months |= PCF_Century_SetBitC;
	}
	else
	{
		Date_InitStructure.RTC_Months &= ~PCF_Century_SetBitC;
	}
	//
	//D¡ä¨¨?D??¡é¦Ì???¡ä??¡Â
	//
	
	buffer[0] = PCF8563_Address_Seconds;
	buffer[1] = Time_InitStructure.RTC_Seconds;
	buffer[2] = Time_InitStructure.RTC_Minutes;
	buffer[3] = Time_InitStructure.RTC_Hours;
	I2C_WriteBuf(PCF8563_Write, buffer, 4);
	
	buffer[0] = PCF8563_Address_Days;
	buffer[1] = Date_InitStructure.RTC_Days;
	buffer[2] = Date_InitStructure.RTC_WeekDays;
	buffer[3] = Date_InitStructure.RTC_Months;
	buffer[4] = Date_InitStructure.RTC_Years;
	I2C_WriteBuf(PCF8563_Write, buffer, 5);
}

/**
   ============================================================================
                     #### ?¨´¨®D??¡ä??¡Â¨¨?2?2¨´¡Á¡Â1|?¨¹o¡¥¨ºy ####
   ============================================================================
**/

/**
  *****************************************************************************
  * @Name   : ?D??¨º¡À??D??¡é¨º?¡¤?¡¤?o?¡¤??¡ì¡ê?3?3??????¡ä3??¦Ì
  *
  * @Brief  : D??¨²?¦Ì¡¤??¡ì¡êo0 ~ 6
  *
  * @Input  : PCF_DataStruct: ??¡ä??¡Â?¨¢11????
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
static void IS_PCF8563_Data(_PCF8563_Register_Typedef* PCF_DataStruct)
{
	if (PCF_DataStruct->Years > 99)           PCF_DataStruct->Years          = 0;  //???¡ä00?¨º
	if (PCF_DataStruct->Months_Century > 12)  PCF_DataStruct->Months_Century = 1;  //???¡ä1??
	if (PCF_DataStruct->Days > 31)            PCF_DataStruct->Days           = 1;  //???¡ä1¨¨?
	if (PCF_DataStruct->WeekDays > 6)         PCF_DataStruct->WeekDays       = 1;  //???¡äD??¨²¨°?
	
	if (PCF_DataStruct->Hours > 23)           PCF_DataStruct->Hours          = 0;  //???¡ä0D?¨º¡À
	if (PCF_DataStruct->Minutes > 59)         PCF_DataStruct->Minutes        = 0;  //???¡ä0¡¤??¨®
	if (PCF_DataStruct->Seconds > 59)         PCF_DataStruct->Seconds        = 0;  //???¡ä0??
}

/**
  *****************************************************************************
  * @Name   : PCF8563D¡ä¨¨???¡ä??¡Â
  *
  * @Brief  : D??¨²¨ºy?¦Ì¡¤??¡ì¨º?: 0 ~ 6¡ê?¨º???????¨º?
  *
  * @Input  : PCF_Format:     ¨ºy?Y??¨º?
  *                           = PCF_Format_BIN
  *                           = PCF_Format_BCD
  *           PCF_Century:    ¨º¨¤?¨ª??¨¦¨¨?¡§
  *                           = PCF_Century_19xx
  *                           = PCF_Century_20xx
  *           PCF_DataStruct: ??¡ä??¡Â?¨¢11????
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_SetRegister(unsigned char PCF_Format, unsigned char PCF_Century, _PCF8563_Register_Typedef* PCF_DataStruct)
{
	if (PCF_Format == PCF_Format_BIN)  //¨º???????¨º?¡ê?D¨¨¨°a¡Áa??¨°???
	{
		//
		//?D??¨ºy?¦Ì¨º?¡¤??¨²¡¤??¡ì???¨²
		//
		IS_PCF8563_Data(PCF_DataStruct);
		//
		//D¨¨¨°a¡Áa??¨°???
		//
		PCF_DataStruct->Years          = RTC_BinToBcd2(PCF_DataStruct->Years);
		PCF_DataStruct->Months_Century = RTC_BinToBcd2(PCF_DataStruct->Months_Century);
		PCF_DataStruct->Days           = RTC_BinToBcd2(PCF_DataStruct->Days);
		PCF_DataStruct->WeekDays       = RTC_BinToBcd2(PCF_DataStruct->WeekDays);
		
		PCF_DataStruct->Hours          = RTC_BinToBcd2(PCF_DataStruct->Hours);
		PCF_DataStruct->Minutes        = RTC_BinToBcd2(PCF_DataStruct->Minutes);
		PCF_DataStruct->Seconds        = RTC_BinToBcd2(PCF_DataStruct->Seconds);
		
		PCF_DataStruct->Day_Alarm      = RTC_BinToBcd2(PCF_DataStruct->Day_Alarm);
		PCF_DataStruct->WeekDays_Alarm = RTC_BinToBcd2(PCF_DataStruct->WeekDays_Alarm);
		
		PCF_DataStruct->Hour_Alarm     = RTC_BinToBcd2(PCF_DataStruct->Hour_Alarm);
		PCF_DataStruct->Minute_Alarm   = RTC_BinToBcd2(PCF_DataStruct->Minute_Alarm);
	}
	//
	//?D??¨º¨¤?¨ª??
	//
	if (PCF_Century == PCF_Century_19xx)
	{
		PCF_DataStruct->Months_Century |= PCF_Century_SetBitC;
	}
	else
	{
		PCF_DataStruct->Months_Century &= ~PCF_Century_SetBitC;
	}
	//
	//1?¡À??¨´¨®D??¨¢?¡ê??D??¨º?3?
	//
	PCF_DataStruct->Timer_Control    &= ~(1<<7);
	PCF_DataStruct->CLKOUT_Frequency &= ~(1<<7);
	PCF_DataStruct->WeekDays_Alarm   &= ~(1<<7);
	PCF_DataStruct->Day_Alarm        &= ~(1<<7);
	PCF_DataStruct->Hour_Alarm       &= ~(1<<7);
	PCF_DataStruct->Minute_Alarm     &= ~(1<<7);
	PCF_DataStruct->Control_Status_2 &= ~(3<<0);

	//
	//D¡ä¨¨?¨ºy?Y¦Ì???¡ä??¡Â
	//
	I2C_WriteBuf(PCF8563_Write, (unsigned char*)PCF_DataStruct, 17);
}

/**
  *****************************************************************************
  * @Name   : PCF8563?¨¢¨¨???¡ä??¡Â
  *
  * @Brief  : D??¨²¨ºy?¦Ì¡¤??¡ì¨º?: 0 ~ 6¡ê?¨º???????¨º?
  *
  * @Input  : PCF_Format:     ¨ºy?Y??¨º?
  *                           = PCF_Format_BIN
  *                           = PCF_Format_BCD
  *
  * @Output : *PCF_Century:   ¨º¨¤?¨ª??¡ê?0¡êo21¨º¨¤?¨ª¡ê?1:20¨º¨¤?¨ª
  *           PCF_DataStruct: ??¡ä??¡Â?¨¢11????
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_GetRegister(unsigned char PCF_Format, unsigned char *PCF_Century, _PCF8563_Register_Typedef* PCF_DataStruct)
{
	//
	//?¨¢¨¨?¨¨?2???¡ä??¡Â¨ºy?¦Ì
	//
	I2C_ReadBuf(PCF8563_Write, PCF8563_Address_Control_Status_1,(unsigned char*)PCF_DataStruct, 16);
	//
	//?D??¨º¨¤?¨ª??¨ºy?¦Ì
	//
	if (PCF_DataStruct->Months_Century & PCF_Century_SetBitC)
	{
		*PCF_Century = 1;
	}
	else
	{
		*PCF_Century = 0;
	}
	//
	//?¨¢¡À??TD¡ì??
	//
	PCF_DataStruct->Years          &= PCF8563_Shield_Years;
	PCF_DataStruct->Months_Century &= PCF8563_Shield_Months_Century;
	PCF_DataStruct->Days           &= PCF8563_Shield_Days;
	PCF_DataStruct->WeekDays       &= PCF8563_Shield_WeekDays;
	
	PCF_DataStruct->Hours          &= PCF8563_Shield_Hours;
	PCF_DataStruct->Minutes        &= PCF8563_Shield_Minutes;
	PCF_DataStruct->Seconds        &= PCF8563_Shield_Seconds;
	
	PCF_DataStruct->Minute_Alarm   &= PCF8563_Shield_Minute_Alarm;
	PCF_DataStruct->Hour_Alarm     &= PCF8563_Shield_Hour_Alarm;
	PCF_DataStruct->Day_Alarm      &= PCF8563_Shield_Day_Alarm;
	PCF_DataStruct->WeekDays_Alarm &= PCF8563_Shield_WeekDays_Alarm;
	
	//
	//?D??D¨¨¨°a¦Ì?¨ºy?Y??¨º?
	//
	if (PCF_Format == PCF_Format_BIN)
	{
		PCF_DataStruct->Years           = RTC_Bcd2ToBin(PCF_DataStruct->Years);
		PCF_DataStruct->Months_Century  = RTC_Bcd2ToBin(PCF_DataStruct->Months_Century);
		PCF_DataStruct->Days            = RTC_Bcd2ToBin(PCF_DataStruct->Days);
		PCF_DataStruct->WeekDays        = RTC_Bcd2ToBin(PCF_DataStruct->WeekDays);
		
		PCF_DataStruct->Hours           = RTC_Bcd2ToBin(PCF_DataStruct->Hours);
		PCF_DataStruct->Minutes         = RTC_Bcd2ToBin(PCF_DataStruct->Minutes);
		PCF_DataStruct->Seconds         = RTC_Bcd2ToBin(PCF_DataStruct->Seconds);
		
		PCF_DataStruct->Day_Alarm       = RTC_Bcd2ToBin(PCF_DataStruct->Day_Alarm);
		PCF_DataStruct->WeekDays_Alarm  = RTC_Bcd2ToBin(PCF_DataStruct->WeekDays_Alarm);
		
		PCF_DataStruct->Hour_Alarm      = RTC_Bcd2ToBin(PCF_DataStruct->Hour_Alarm);
		PCF_DataStruct->Minute_Alarm    = RTC_Bcd2ToBin(PCF_DataStruct->Minute_Alarm);
	}
}

/**
   ============================================================================
                      #### ¨º¡À??D??¡é2¨´¡Á¡Â1|?¨¹o¡¥¨ºy ####
   ============================================================================
**/

/**
  *****************************************************************************
  * @Name   : PCF8563D¡ä¨¨?¨º¡À??D??¡é
  *
  * @Brief  : none
  *
  * @Input  : PCF_Format:     ¨ºy?Y??¨º?
  *                           = PCF_Format_BIN
  *                           = PCF_Format_BCD
  *           PCF_DataStruct: ¨º¡À???¨¢11????
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_SetTime(unsigned char PCF_Format, _PCF8563_Time_Typedef* PCF_DataStruct)
{
	if (PCF_Format == PCF_Format_BIN)  //¨º???????¨º?¡ê?D¨¨¨°a¡Áa??¨°???
	{
		//
		//?D??¨ºy?¦Ì¨º?¡¤??¨²¡¤??¡ì???¨²
		//
		if (PCF_DataStruct->RTC_Hours > 23)    PCF_DataStruct->RTC_Hours   = 0;  //???¡ä0D?¨º¡À
		if (PCF_DataStruct->RTC_Minutes > 59)  PCF_DataStruct->RTC_Minutes = 0;  //???¡ä0¡¤??¨®
		if (PCF_DataStruct->RTC_Seconds > 59)  PCF_DataStruct->RTC_Seconds = 0;  //???¡ä0??
		//
		//D¨¨¨°a¡Áa??¨°???
		//
		PCF_DataStruct->RTC_Hours   = RTC_BinToBcd2(PCF_DataStruct->RTC_Hours);
		PCF_DataStruct->RTC_Minutes = RTC_BinToBcd2(PCF_DataStruct->RTC_Minutes);
		PCF_DataStruct->RTC_Seconds = RTC_BinToBcd2(PCF_DataStruct->RTC_Seconds);
	}
	//
	//??¡À¡ä¨ºy?Y
	//
	buffer[0] = PCF8563_Address_Seconds;
	buffer[1] = PCF_DataStruct->RTC_Seconds;
	buffer[2] = PCF_DataStruct->RTC_Minutes;
	buffer[3] = PCF_DataStruct->RTC_Hours;
	//
	//D¡ä¨¨?¨ºy?Y¦Ì???¡ä??¡Â
	//
	I2C_WriteBuf(PCF8563_Write, buffer, 4);
}

/**
  *****************************************************************************
  * @Name   : PCF8563?¨¢¨¨?¨º¡À??D??¡é
  *
  * @Brief  : none
  *
  * @Input  : PCF_Format:     ¨ºy?Y??¨º?
  *                           = PCF_Format_BIN
  *                           = PCF_Format_BCD
  *
  * @Output : PCF_DataStruct: ¨º¡À???¨¢11????
  *
  * @Return : none
  *****************************************************************************
**/
unsigned char PCF8563_GetTime(unsigned char PCF_Format, _PCF8563_Time_Typedef* PCF_DataStruct)
{
	unsigned char ret=0;
	//
	//?¨¢¨¨???¡ä??¡Â¨ºy?¦Ì
	//
	ret = I2C_ReadBuf(PCF8563_Write, PCF8563_Address_Seconds,buffer, 3);
	if ( ret == 0 ) return 0;
	//
	//?¨¢¡À??TD¡ì??
	//
	buffer[0] &= PCF8563_Shield_Seconds;
	buffer[1] &= PCF8563_Shield_Minutes;
	buffer[2] &= PCF8563_Shield_Hours;
	
	//
	//?D??D¨¨¨°a¦Ì?¨ºy?Y??¨º?
	//
	if (PCF_Format == PCF_Format_BIN)
	{
		PCF_DataStruct->RTC_Hours   = RTC_Bcd2ToBin(buffer[2]);
		PCF_DataStruct->RTC_Minutes = RTC_Bcd2ToBin(buffer[1]);
		PCF_DataStruct->RTC_Seconds = RTC_Bcd2ToBin(buffer[0]);
	}
	// else if (PCF_Format == PCF_Format_BCD)
	// {
		// //
		// //??¡À¡ä¨ºy?Y
		// //
		// PCF_DataStruct->RTC_Hours   = buffer[2];
		// PCF_DataStruct->RTC_Minutes = buffer[1];
		// PCF_DataStruct->RTC_Seconds = buffer[0];
	// }
	return 1;
}

/**
   ============================================================================
                         #### ¨¨??¨²D??¡é2¨´¡Á¡Â1|?¨¹o¡¥¨ºy ####
   ============================================================================
**/

/**
  *****************************************************************************
  * @Name   : PCF8563D¡ä¨¨?¨¨??¨²D??¡é
  *
  * @Brief  : D??¨²¨ºy?¦Ì¡¤??¡ì¨º?: 0 ~ 6¡ê?¨º???????¨º?
  *
  * @Input  : PCF_Format:     ¨ºy?Y??¨º?
  *                           = PCF_Format_BIN
  *                           = PCF_Format_BCD
  *           PCF_Century:    ¨º¨¤?¨ª??¨¦¨¨?¡§
  *                           = PCF_Century_19xx
  *                           = PCF_Century_20xx
  *           PCF_DataStruct: ¨¨??¨²?¨¢11????
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_SetDate(unsigned char PCF_Format, _PCF8563_Date_Typedef* PCF_DataStruct)
{
	if (PCF_Format == PCF_Format_BIN)  //¨º???????¨º?¡ê?D¨¨¨°a¡Áa??¨°???
	{
		//
		//?D??¨ºy?¦Ì¨º?¡¤??¨²¡¤??¡ì???¨²
		//
		if (PCF_DataStruct->RTC_Years > 99)    PCF_DataStruct->RTC_Years    = 0;  //???¡ä00?¨º
		if (PCF_DataStruct->RTC_Months > 12)   PCF_DataStruct->RTC_Months   = 1;  //???¡ä1??
		if (PCF_DataStruct->RTC_Days > 31)     PCF_DataStruct->RTC_Days     = 1;  //???¡ä1¨¨?
		if (PCF_DataStruct->RTC_WeekDays > 6)  PCF_DataStruct->RTC_WeekDays = 1;  //???¡äD??¨²¨°?
		//
		//D¨¨¨°a¡Áa??¨°???
		//
		PCF_DataStruct->RTC_Years    = RTC_BinToBcd2(PCF_DataStruct->RTC_Years);
		PCF_DataStruct->RTC_Months   = RTC_BinToBcd2(PCF_DataStruct->RTC_Months);
		PCF_DataStruct->RTC_Days     = RTC_BinToBcd2(PCF_DataStruct->RTC_Days);
		PCF_DataStruct->RTC_WeekDays = RTC_BinToBcd2(PCF_DataStruct->RTC_WeekDays);
	}
	//
	//¨ºy?Y??¡À¡ä
	//
	buffer[0] = PCF8563_Address_Days;
	buffer[1] = PCF_DataStruct->RTC_Days;
	buffer[2] = PCF_DataStruct->RTC_WeekDays;
	buffer[3] = PCF_DataStruct->RTC_Months;
	buffer[4] = PCF_DataStruct->RTC_Years;
	//
	//D¡ä¨¨?¨ºy?Y¦Ì???¡ä??¡Â
	//
	I2C_WriteBuf(PCF8563_Write, buffer, 5);
}

/**
  *****************************************************************************
  * @Name   : PCF8563?¨¢¨¨?¨¨??¨²D??¡é
  *
  * @Brief  : D??¨²¨ºy?¦Ì¡¤??¡ì¨º?: 0 ~ 6¡ê?¨º???????¨º?
  *
  * @Input  : PCF_Format:     ¨ºy?Y??¨º?
  *                           = PCF_Format_BIN
  *                           = PCF_Format_BCD
  *
  * @Output : 
  *           PCF_DataStruct: ¨¨??¨²?¨¢11????
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_GetDate(unsigned char PCF_Format, _PCF8563_Date_Typedef* PCF_DataStruct)
{
	//
	//?¨¢¨¨?¨¨?2???¡ä??¡Â¨ºy?¦Ì
	//
	I2C_ReadBuf(PCF8563_Write, PCF8563_Address_Days, buffer, 4);
	//
	//?¨¢¡À??TD¡ì??
	//
	buffer[0] &= PCF8563_Shield_Days;
	buffer[1] &= PCF8563_Shield_WeekDays;
	buffer[2] &= PCF8563_Shield_Months_Century;
	buffer[3] &= PCF8563_Shield_Years;
	
	//
	//?D??D¨¨¨°a¦Ì?¨ºy?Y??¨º?
	//
	if (PCF_Format == PCF_Format_BIN)
	{
		PCF_DataStruct->RTC_Years    = RTC_Bcd2ToBin(buffer[3]);
		PCF_DataStruct->RTC_Months   = RTC_Bcd2ToBin(buffer[2]);
		PCF_DataStruct->RTC_Days     = RTC_Bcd2ToBin(buffer[0]);
		PCF_DataStruct->RTC_WeekDays = RTC_Bcd2ToBin(buffer[1]);		
	}
	else if (PCF_Format == PCF_Format_BCD)
	{
		//
		//??¡À¡ä¨ºy?Y
		//
		PCF_DataStruct->RTC_Years    = buffer[3];
		PCF_DataStruct->RTC_Months   = buffer[2];
		PCF_DataStruct->RTC_Days     = buffer[0];
		PCF_DataStruct->RTC_WeekDays = buffer[1];
	}
}

/**
   ============================================================================
                         #### ??¨¢?D??¡é2¨´¡Á¡Â1|?¨¹o¡¥¨ºy ####
   ============================================================================
**/

/**
  *****************************************************************************
  * @Name   : PCF8563D¡ä¨¨???¨¢?D??¡é
  *
  * @Brief  : D??¨²¨ºy?¦Ì¡¤??¡ì¨º?: 0 ~ 6¡ê?¨º???????¨º?
  *
  * @Input  : PCF_Format:     ¨ºy?Y??¨º?
  *                           = PCF_Format_BIN
  *                           = PCF_Format_BCD
  *           PCF_DataStruct: ??¨¢??¨¢11????
  *
  * @Output : none
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_SetAlarm(unsigned char PCF_Format, _PCF8563_Alarm_Typedef* PCF_DataStruct)
{
	unsigned char Alarm_State = 0;
	unsigned char Alarm_Interrupt = 0;  //????/¡Á¡ä¨¬???¡ä??¡Â??¨¢??D???o¡ä?
	
	if (PCF_Format == PCF_Format_BIN)  //¨º???????¨º?¡ê?D¨¨¨°a¡Áa??¨°???
	{
		//
		//?D??¨ºy?¦Ì¨º?¡¤??¨²¡¤??¡ì???¨²
		//
		if (PCF_DataStruct->RTC_AlarmMinutes > 59)  PCF_DataStruct->RTC_AlarmMinutes  = 0;  //???¡ä0¡¤??¨®
		if (PCF_DataStruct->RTC_AlarmHours > 23)    PCF_DataStruct->RTC_AlarmHours    = 0;  //???¡ä0D?¨º¡À
		if (PCF_DataStruct->RTC_AlarmDays > 31)     PCF_DataStruct->RTC_AlarmDays     = 1;  //???¡ä1¨¨?
		if (PCF_DataStruct->RTC_AlarmWeekDays > 6)  PCF_DataStruct->RTC_AlarmWeekDays = 1;  //???¡äD??¨²¨°?
		//
		//D¨¨¨°a¡Áa??¨°???
		//
		PCF_DataStruct->RTC_AlarmMinutes  = RTC_BinToBcd2(PCF_DataStruct->RTC_AlarmMinutes);
		PCF_DataStruct->RTC_AlarmHours    = RTC_BinToBcd2(PCF_DataStruct->RTC_AlarmHours);
		PCF_DataStruct->RTC_AlarmDays     = RTC_BinToBcd2(PCF_DataStruct->RTC_AlarmDays);
		PCF_DataStruct->RTC_AlarmWeekDays = RTC_BinToBcd2(PCF_DataStruct->RTC_AlarmWeekDays);
	}
	//
	//?D??¨º?¡¤??a????¨¢?????
	//
	if (PCF_DataStruct->RTC_AlarmNewState == RTC_AlarmNewState_Open)  //??¡ä¨°?a??¨¢?
	{
		Alarm_State = 1;
	}
	else if (PCF_DataStruct->RTC_AlarmNewState == RTC_AlarmNewState_Open_INT_Enable)  //¡ä¨°?a??¨¢?2¡é¡ä¨°?a?D??¨º?3?
	{
		Alarm_State = 2;
	}
	else if (PCF_DataStruct->RTC_AlarmNewState == RTC_AlarmNewState_Close)  //1?¡À???¨¢?¡ê?2¡é?¨°1?¡À??D??¨º?3?
	{
		Alarm_State = 3;
	}
	//
	//?¨¢¨¨?????/¡Á¡ä¨¬???¡ä??¡Â2?¦Ì
	//
	Alarm_Interrupt = PCF8563_Read_Byte(PCF8563_Address_Control_Status_2);
	Alarm_Interrupt &= PCF_Alarm_INT_Close;  //?¨¨1?¡À??D??¨º?3?
	Alarm_Interrupt &= PCF_Control_ClearAF;;  //??3y¡À¨º??
	PCF8563_Write_Byte(PCF8563_Address_Control_Status_2, Alarm_Interrupt);
	
	//
	//?¨´?Y?a??¨¤¨¤D¨ª??DD?¨¤¨®|2¨´¡Á¡Â
	//
	if (Alarm_State == 1 || Alarm_State == 2)  //¡ä¨°?a??¨¢?
	{
		if (PCF_DataStruct->RTC_AlarmType & RTC_AlarmType_Days)      PCF_DataStruct->RTC_AlarmDays     &= PCF_Alarm_DaysOpen;  //¨¨??¨²??¨¢?
		if (PCF_DataStruct->RTC_AlarmType & RTC_AlarmType_Hours)     PCF_DataStruct->RTC_AlarmHours    &= PCF_Alarm_HoursOpen;  //D?¨º¡À??¨¢?
		if (PCF_DataStruct->RTC_AlarmType & RTC_AlarmType_Minutes)   PCF_DataStruct->RTC_AlarmMinutes  &= PCF_Alarm_MinutesOpen;  //¡¤??¨®??¨¢?
		if (PCF_DataStruct->RTC_AlarmType & RTC_AlarmType_WeekDays)  PCF_DataStruct->RTC_AlarmWeekDays &= PCF_Alarm_WeekDaysOpen;  //¡¤??¨®??¨¢?
	}
	if (Alarm_State == 3)  //1?¡À?
	{
		if (PCF_DataStruct->RTC_AlarmType & RTC_AlarmType_Days)      PCF_DataStruct->RTC_AlarmDays     |= PCF_Alarm_DaysClose;  //¨¨??¨²??¨¢?
		if (PCF_DataStruct->RTC_AlarmType & RTC_AlarmType_Hours)     PCF_DataStruct->RTC_AlarmHours    |= PCF_Alarm_HoursClose;  //D?¨º¡À??¨¢?
		if (PCF_DataStruct->RTC_AlarmType & RTC_AlarmType_Minutes)   PCF_DataStruct->RTC_AlarmMinutes  |= PCF_Alarm_MinutesClose;  //¡¤??¨®??¨¢?
		if (PCF_DataStruct->RTC_AlarmType & RTC_AlarmType_WeekDays)  PCF_DataStruct->RTC_AlarmWeekDays |= PCF_Alarm_WeekDaysClose;  //¡¤??¨®??¨¢?
	}
	//
	//?D??¨º?¡¤??a???D??¨º?3?
	//
	if (Alarm_State == 2)
	{
		Alarm_Interrupt |= PCF_Alarm_INT_Open;
		Alarm_Interrupt &= PCF_Control_ClearAF;;  //??3y¡À¨º??
	}
	//
	//??¡À¡ä¨ºy?Y
	//
	buffer[0] = PCF8563_Alarm_Minutes;
	buffer[1] = PCF_DataStruct->RTC_AlarmMinutes;
	buffer[2] = PCF_DataStruct->RTC_AlarmHours;
	buffer[3] = PCF_DataStruct->RTC_AlarmDays;
	buffer[4] = PCF_DataStruct->RTC_AlarmWeekDays;
	//
	//D¡ä¨¨?¨ºy?Y¦Ì???¡ä??¡Â
	//
	I2C_WriteBuf(PCF8563_Write, buffer, 5);
	//
	//D¡ä¨¨?????/¡Á¡ä¨¬???¡ä??¡Â2¨ºy?¦Ì
	//
	buffer[0] = PCF8563_Address_Control_Status_2;
	buffer[1] = Alarm_Interrupt;
	I2C_WriteBuf(PCF8563_Write, buffer, 2);
}

/**
  *****************************************************************************
  * @Name   : PCF8563?¨¢¨¨???¨¢?D??¡é
  *
  * @Brief  : D??¨²¨ºy?¦Ì¡¤??¡ì¨º?: 0 ~ 6¡ê?¨º???????¨º?¡ê???¨º?¡¤¦Ì??¨¢???¨¢???¡ä??¡Â¨ºy?¦Ì¡ê??a1????¡é?D??¨º?3?¨º2?¡ä¦Ì?2?¡¤¦Ì??
  *
  * @Input  : PCF_Format:     ¨ºy?Y??¨º?
  *                           = PCF_Format_BIN
  *                           = PCF_Format_BCD
  *
  * @Output : PCF_DataStruct: ??¨¢??¨¢11????
  *
  * @Return : none
  *****************************************************************************
**/
void PCF8563_GetAlarm(unsigned char PCF_Format, _PCF8563_Alarm_Typedef* PCF_DataStruct)
{
	//
	//?¨¢¨¨?¨¨?2???¡ä??¡Â¨ºy?¦Ì
	//
	I2C_ReadBuf(PCF8563_Write, PCF8563_Alarm_Minutes,buffer, 4);
	//
	//?¨¢¡À??TD¡ì??¡ê?¡¤??¨®¡À¡§?¡¥?¦Ì¨¨?2?¡¤¦Ì??
	//
	buffer[0] &= PCF8563_Shield_Minute_Alarm;  //¡¤??¨®¡À¡§?¡¥?¦Ì
	buffer[1] &= PCF8563_Shield_Hour_Alarm;  //D?¨º¡À¡À¡§?¡¥?¦Ì
	buffer[2] &= PCF8563_Shield_Day_Alarm;  //¨¨??¨²¡À¡§?¡¥?¦Ì
	buffer[3] &= PCF8563_Shield_WeekDays_Alarm;  //D??¨²¡À¡§?¡¥?¦Ì
	
	//
	//?D??D¨¨¨°a¦Ì?¨ºy?Y??¨º?
	//
	if (PCF_Format == PCF_Format_BIN)
	{
		PCF_DataStruct->RTC_AlarmDays     = RTC_Bcd2ToBin(buffer[2]);
		PCF_DataStruct->RTC_AlarmHours    = RTC_Bcd2ToBin(buffer[1]);
		PCF_DataStruct->RTC_AlarmMinutes  = RTC_Bcd2ToBin(buffer[0]);
		PCF_DataStruct->RTC_AlarmWeekDays = RTC_Bcd2ToBin(buffer[3]);
	}
	else if (PCF_Format == PCF_Format_BCD)
	{
		//
		//??¡À¡ä¨ºy?Y
		//
		PCF_DataStruct->RTC_AlarmDays     = buffer[2];
		PCF_DataStruct->RTC_AlarmHours    = buffer[1];
		PCF_DataStruct->RTC_AlarmMinutes  = buffer[0];
		PCF_DataStruct->RTC_AlarmWeekDays = buffer[3];
	}
}
/**
  *****************************************************************************
  * @Name   : PCF85633?¨º??¡¥¨¦¨¨¡À?¨º¡À??
  *
  * @Brief  : ?¨²PCF8563¦Ì¨²¨°?¡ä?1¡è¡Á¡Â¨º¡À¡ê?¨¦¨¨??¨°???3?¨º?¦Ì?¨º¡À??o¨ª¨¨??¨²
  *
  * @Input  : NONE
  *
  * @Output : NONE
  *
  * @Return : none
  *****************************************************************************
**/  
void PCF8563_SetFunc(void) 
{ 
//    _PCF8563_Date_Typedef 	Date_InitStructure; 
    _PCF8563_Time_Typedef 	Timer_InitStruct;

  //  Date_InitStructure.RTC_Years    = 17; 
  //  Date_InitStructure.RTC_Months   = 1; 
  //  Date_InitStructure.RTC_Days     = 14; 
 //   Date_InitStructure.RTC_WeekDays = 6;

    Timer_InitStruct.RTC_Hours = HOUR; 
    Timer_InitStruct.RTC_Minutes = MIN; 
    Timer_InitStruct.RTC_Seconds = 02; 

    PCF8563_SetMode(PCF_Mode_Normal); 
    PCF8563_Stop(); 
    PCF8563_SetTime(PCF_Format_BIN,&Timer_InitStruct); 
  //  PCF8563_SetDate(PCF_Format_BIN,&Date_InitStructure); 
    PCF8563_Start(); 
} 


void Init_pcf8563(void)
{
	 //IIC_GPIO_Init();
	 PCF8563_Check();
  //PCF8563_SetFunc();
}
