
#include "stm8s.h"
#include "stm8s_adc1.h"   
#include "stm8s_flash.h"  
#include "stm8s_tim2.h" 
#include "stm8s_iwdg.h"

#include "bl55072.h"
#include "TM16XX.h"
#include "display.h"
#include "pcf8563.h"
#include "bike.h"
#include "YXT.h"

/*----------------------------------------------------------*/
#ifdef JINPENG_4860
const uint16_t uiBatStatus48[8] = {420,426,432,439,445,451,457,464};
const uint16_t uiBatStatus60[8] = {520,528,536,542,550,558,566,574};
const uint16_t uiBatStatus72[8] = {0xFFFF};
#elif defined JINPENG_6072
const uint16_t uiBatStatus48[8] = {0xFFFF};
const uint16_t uiBatStatus60[8] = {480,493,506,519,532,545,558,570};
const uint16_t uiBatStatus72[8] = {550,569,589,608,628,647,667,686};
#elif defined JINPENG_MR9737
const uint16_t uiBatStatus48[8] = {300,410,438,464,490,516};
const uint16_t uiBatStatus60[8] = {0xFFFF};
const uint16_t uiBatStatus72[8] = {0xFFFF};
#elif defined JINPENG_12080
const uint16_t uiBatStatus48[8] = {420,426,433,440,447,454,462,470};
const uint16_t uiBatStatus60[8] = {520,528,537,546,555,563,571,579};
const uint16_t uiBatStatus72[8] = {0xFFFF};
#elif defined LCD6040
const uint16_t uiBatStatus48[] 	= {425,432,444,456,468};
const uint16_t uiBatStatus60[] 	= {525,537,553,566,578};
const uint16_t uiBatStatus72[] 	= {630,641,661,681,701};
#else
const uint16_t uiBatStatus48[8] = {420,427,435,444,453,462,471,481};
const uint16_t uiBatStatus60[8] = {520,531,544,556,568,577,587,595};
const uint16_t uiBatStatus72[8] = {630,642,653,664,675,687,700,715};
#endif

/*----------------------------------------------------------*/
uint16_t uiSpeedBuf[16];
uint16_t uiVolBuf[32];
uint16_t uiTempBuf[4];

#if ( TIME_ENABLE == 1 )
uint8_t ucUart1Buf[16];
uint8_t ucUart1Index=0;
#endif

/*----------------------------------------------------------*/
/**
  * @brief  Configures the IWDG to generate a Reset if it is not refreshed at the
  *         correct time. 
  * @param  None
  * @retval None
  */
static void IWDG_Config(void)
{
  /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
  IWDG_Enable();
  
  /* Enable write access to IWDG_PR and IWDG_RLR registers */
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  
  /* IWDG counter clock: LSI/128 */
  IWDG_SetPrescaler(IWDG_Prescaler_128);
  
  /* Set counter reload value LsiFreq/128/256 = 512ms */
  IWDG_SetReload(0xFF);
  
  /* Reload IWDG counter */
  FEED_DOG();
}

void InitTimer(void)
{
	/** ÅäÖÃTimer2 **/ 
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	TIM2_TimeBaseInit(TIM2_PRESCALER_8, 1000);   //1ms
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);            
	TIM2_Cmd(ENABLE);     
}

BitStatus GPIO_Read(GPIO_TypeDef* GPIOx, GPIO_Pin_TypeDef GPIO_Pin)
{
	GPIO_Init(GPIOx, GPIO_Pin, GPIO_MODE_IN_FL_NO_IT);
	return GPIO_ReadInputPin(GPIOx, GPIO_Pin);
}

void Delay(uint32_t nCount)
{
  for(; nCount != 0; nCount--);
}

int GetTemp(void)
{
	static uint8_t ucIndex = 0;
	int32_t slTemp;

	//GPIO_Init(GPIOD, GPIO_PIN_6, GPIO_MODE_IN_FL_NO_IT);  //Temp
	//ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_6, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, ADC1_SCHMITTTRIG_CHANNEL6 ,\
				DISABLE);
	ADC1_Cmd(ENABLE);
	Delay(1000);
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	slTemp = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);
  
	uiTempBuf[ucIndex++] = slTemp;
	if ( ucIndex >= ContainOf(uiTempBuf) )
		ucIndex = 0;
	slTemp = get_average16(uiTempBuf,ContainOf(uiTempBuf));

	//slTemp = 470UL*1024/(1024-slTemp)-470;
	//slTemp = NTCtoTemp(slTemp)/10;
	//slTemp = ((3600- (long)slTemp * 2905/1024)/10);

	slTemp = 10000*1024UL/(1024-slTemp)-10000;
	slTemp = NTCtoTemp(slTemp);
	if ( slTemp > 999  ) slTemp =  999;
	if ( slTemp < -999 ) slTemp = -999;
	
	return slTemp;
}

uint16_t GetVol(void)
{
	static uint8_t ucIndex = 0;
	uint16_t uiVol;

	//GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);  //B+  
	//ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, BATV_ADC_CH, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, BATV_ADC_SCH,\
				DISABLE);
	ADC1_Cmd(ENABLE);
	Delay(5000);  
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	uiVol = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);

	uiVolBuf[ucIndex++] = uiVol;
	if ( ucIndex >= ContainOf(uiVolBuf) )
		ucIndex = 0;
	uiVol = get_average16(uiVolBuf,ContainOf(uiVolBuf));
	uiVol = (uint32_t)uiVol*1050UL/1024UL;
	
	return uiVol;
}

#ifdef BATV2_ADC_CH
uint16_t GetVol2(void)
{
	static uint8_t ucIndex = 0;
	uint16_t uiVol;

	GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);  //B+  
	ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, BATV2_ADC_CH, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, BATV2_ADC_SCH,\
				DISABLE);
	ADC1_Cmd(ENABLE);
	Delay(5000);  
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	uiVol = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);

	uiVol2Buf[ucIndex++] = uiVol;
	if ( ucIndex >= ContainOf(uiVol2Buf) )
		ucIndex = 0;
	uiVol = get_average16(uiVol2Buf,ContainOf(uiVol2Buf));
	uiVol = (uint32_t)uiVol*1050UL/1024UL;
	
	return uiVol;
}
#endif



uint8_t GetVolStabed(uint16_t* uiVol)
{
	static uint8_t ucIndex = 0;
	uint32_t ulMid;
	uint16_t uiBuf[32];
	uint8_t i;
	
	//GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);  //B+  
	//ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, BATV_ADC_CH, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, BATV_ADC_SCH,\
				DISABLE);

	ADC1_Cmd(ENABLE);
	for(i=0;i<ContainOf(uiBuf);i++){
		Delay(500);  
		ADC1_StartConversion(); 
		while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
		uiBuf[i] = ADC1_GetConversionValue();
	}
	ADC1_Cmd(DISABLE);
	
	ulMid = get_average16(uiBuf,ContainOf(uiBuf));
	// for( i=0;i<ContainOf(uiBuf);i++){
		// if ( ulMid > 2 && ((ulMid *100 / uiBuf[i]) > 101 || (ulMid *100 / uiBuf[i]) < 99) )
			// return 0;
	// }

	uiVolBuf[ucIndex++] = ulMid;
	if ( ucIndex >= ContainOf(uiVolBuf) )
		ucIndex = 0;
		
	for(i=0;i<ContainOf(uiVolBuf);i++)
		uiBuf[i] = uiVolBuf[i];
	
	exchange_sort16(uiBuf,ContainOf(uiVolBuf));
	ulMid = 0;
	for(i=0;i<ContainOf(uiVolBuf);i++){
		if ( uiBuf[i] > 10 )
			ulMid = get_average16(uiBuf+i,ContainOf(uiVolBuf)-i);
	}
	*uiVol = ulMid*1050UL/1024UL;
	return 1;
}

#if ( PCB_VER == 0013 )
uint8_t GetSpeedAdj(void)
{
	static uint8_t ucIndex = 0;
	uint16_t uiAdj;
	uint8_t i;

	ADC1_DeInit();  
	ADC1_Init(	ADC1_CONVERSIONMODE_CONTINUOUS, SPEEDV_ADJ_CH, ADC1_PRESSEL_FCPU_D2,\
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, SPEEDV_ADJ_SCH,DISABLE);

	ADC1_Cmd(ENABLE);
	Delay(1000);  
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	uiAdj = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);
  	
	uiSpeedBuf[ucIndex++] = uiAdj;
	if ( ucIndex >= ContainOf(uiSpeedBuf) )
		ucIndex = 0;

	uiAdj = get_average16(uiSpeedBuf,ContainOf(uiSpeedBuf));
	
	if ( uiAdj > 99 )
		uiAdj = 99;
	
  return uiAdj;
}
#endif

uint8_t GetSpeed(void)
{
	static uint8_t ucIndex = 0;
	uint16_t uiSpeed;

	//GPIO_Init(GPIOD, GPIO_PIN_2, GPIO_MODE_IN_FL_NO_IT);
	//ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, SPEEDV_ADC_CH, ADC1_PRESSEL_FCPU_D2, \
			ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, SPEEDV_ADC_SCH,\
			DISABLE);

	ADC1_Cmd(ENABLE);
	Delay(1000);  
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	uiSpeed = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);
  	
	uiSpeedBuf[ucIndex++] = uiSpeed;
	if ( ucIndex >= ContainOf(uiSpeedBuf) )
		ucIndex = 0;

	uiSpeed = get_average16(uiSpeedBuf,ContainOf(uiSpeedBuf));
	
	if ( sConfig.uiSysVoltage		== 48 ){
		uiSpeed = SPEED_CALC_48V((uint32_t)uiSpeed);
	} else if ( sConfig.uiSysVoltage== 60 ) {
		uiSpeed = SPEED_CALC_60V((uint32_t)uiSpeed);
	} else if ( sConfig.uiSysVoltage== 72 )	{
		uiSpeed = SPEED_CALC_72V((uint32_t)uiSpeed);
	}
	if ( uiSpeed > 99 )
		uiSpeed = 99;
	
  return uiSpeed;
}

void GetSysVoltage(void)
{	
#if defined BENLING_OUSHANG
	uint16_t uiVol;
	for(i=0;i<0xFF;i++){
		if ( GetVolStabed(&uiVol) && (uiVol > 120) ) break;
		FEED_DOG();  
	}
	if ( 720 <= uiVol && uiVol <= 870 ){
		sConfig.uiSysVoltage = 72;
		WriteConfig();
	} else if ( 480 <= uiVol && uiVol <= 600 ){
		sConfig.uiSysVoltage = 60;
		WriteConfig();
	}
#elif defined VD61723650
	uint16_t uiVol;
	for(i=0;i<0xFF;i++){
		if ( GetVolStabed(&uiVol) && (uiVol > 120) ) break;
		FEED_DOG();  
	}
	if ( 610 <= uiVol && uiVol <= 720 ){
		sConfig.uiSysVoltage = 60;
		WriteConfig();
	}	else if ( 360 <= uiVol && uiVol <= 500 ){
		sConfig.uiSysVoltage = 48;
		WriteConfig();
	}		
#elif defined VD48
	sConfig.uiSysVoltage = 48;
#elif defined VD60
	sConfig.uiSysVoltage = 60;
#elif defined VD72
	sConfig.uiSysVoltage = 72;
#elif defined VD48N72L
	if ( GPIO_Read(V48_PORT, V48_PIN) == RESET ){
		sConfig.uiSysVoltage = 72;
	} else {
		sConfig.uiSysVoltage = 60;
	}
#elif defined VD48L72N
	if ( GPIO_Read(V48_PORT, V48_PIN) == RESET ){
		sConfig.uiSysVoltage = 48;
	} else {
		sConfig.uiSysVoltage = 60;
	}
#elif defined VD48H72N
	if ( GPIO_Read(V48_PORT, V48_PIN) == RESET ){
		sConfig.uiSysVoltage = 60;
	} else {
		sConfig.uiSysVoltage = 48;
	}
#elif defined VD72L48L
	if ( GPIO_Read(V72_PORT, V72_PIN) == RESET ){
		sConfig.uiSysVoltage = 72;
	} else {
		if ( GPIO_Read(V48_PORT, V48_PIN) == RESET ){
			sConfig.uiSysVoltage = 48;
		} else {
			sConfig.uiSysVoltage = 60;
		}
	}
#else
	#error "Please select a system voltage detection method!!!"
#endif
}

void LightTask(void)
{
	uint8_t ucSpeedMode;

	if( GPIO_Read(NearLight_PORT, 	NearLight_PIN) ) sBike.bNearLight = 1; else sBike.bNearLight = 0;
	//if( GPIO_Read(TurnRight_PORT, TurnRight_PIN) ) sBike.bTurnRight = 1; else sBike.bTurnRight = 0;
	//if( GPIO_Read(TurnLeft_PORT, 	TurnLeft_PIN ) ) sBike.bTurnLeft  = 1; else sBike.bTurnLeft  = 0;
#ifdef OverSpeed_PORT
	if( GPIO_Read(OverSpeed_PORT, 	OverSpeed_PIN) ) sBike.bOverSpeed = 1; else sBike.bOverSpeed = 0;
#endif
	
	if ( sBike.bYXTERR ){
		ucSpeedMode = 0;
		if( GPIO_Read(SPMODE1_PORT,SPMODE1_PIN) ) ucSpeedMode |= 1<<0;
		if( GPIO_Read(SPMODE2_PORT,SPMODE2_PIN) ) ucSpeedMode |= 1<<1;
		if( GPIO_Read(SPMODE3_PORT,SPMODE3_PIN) ) ucSpeedMode |= 1<<2;
	#ifdef SPMODE4_PORT
		if( GPIO_Read(SPMODE4_PORT,SPMODE4_PIN) ) ucSpeedMode |= 1<<3;
	#endif
		switch(ucSpeedMode){
			case 0x01: 	sBike.ucSpeedMode = 1; break;
			case 0x02: 	sBike.ucSpeedMode = 2; break;
			case 0x04: 	sBike.ucSpeedMode = 3; break;
			case 0x08: 	sBike.ucSpeedMode = 4; break;
			default:	sBike.ucSpeedMode = 0; break;
		}
		sBike.ucPHA_Speed	= GetSpeed();
		sBike.ucSpeed 		= (uint32_t)sBike.ucPHA_Speed*1000UL/sConfig.uiSpeedScale;
	}
}

#if ( TIME_ENABLE == 1 )
void InitUART(void)
{
	if ( sBike.bUart == 0 )
		return ;
	
	/* USART configured as follow:
		- BaudRate = 9600 baud  
		- Word Length = 8 Bits
		- One Stop Bit
		- Odd parity
		- Receive and transmit enabled
		- UART Clock disabled
	*/
	UART1_Init((uint32_t)9600, UART1_WORDLENGTH_8D,UART1_STOPBITS_1, UART1_PARITY_ODD,
				   UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_RX_ENABLE|UART1_MODE_TX_DISABLE);

	/* Enable the UART Receive interrupt: this interrupt is generated when the UART
	receive data register is not empty */
	UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);

	/* Enable UART */
	UART1_Cmd(ENABLE);
}

void UartTask(void)
{   
	uint16_t uiVol,i;
	
	if ( sBike.bUart == 0 )
		return ;
	
	if ( ucUart1Index > 0 && ucUart1Buf[ucUart1Index-1] == '\n' ){
		if ( ucUart1Index >= 11 && ucUart1Buf[0] == 'T' /*&& ucUart1Buf[1] == 'i' && ucUart1Buf[2] == 'm' && ucUart1Buf[3] == 'e' */) {
			RtcTime.RTC_Hours 	= (ucUart1Buf[5]-'0')*10 + (ucUart1Buf[6] - '0');
			RtcTime.RTC_Minutes = (ucUart1Buf[8]-'0')*10 + (ucUart1Buf[9] - '0');
			PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
		} else if ( ucUart1Index >= 5 && ucUart1Buf[0] == 'C' /*&& ucUart1Buf[1] == 'a' && ucUart1Buf[2] == 'l' && ucUart1Buf[3] == 'i' */){
			for(i=0;i<0xFF;i++){
				if ( GetVolStabed(&uiVol) && (uiVol > 120) ) break;
				FEED_DOG();  
			}
			sBike.uiBatVoltage	 	= uiVol;
			sBike.siTemperature = GetTemp();
			sBike.ucSpeed		= GetSpeed();

			sConfig.uiVolScale	= (uint32_t)sBike.uiBatVoltage*1000UL/VOL_CALIBRATIOIN;					
		//	sConfig.TempScale 	= (long)sBike.siTemperature*1000UL/TEMP_CALIBRATIOIN;	
		//	sConfig.uiSpeedScale= (uint32_t)sBike.ucSpeed*1000UL/SPEED_CALIBRATIOIN;				
			WriteConfig();
		}
		ucUart1Index = 0;
	}
}
#endif 

void Calibration(void)
{
	uint8_t i;
	uint16_t uiVol;
	
	CFG->GCR = CFG_GCR_SWD;
	//¶Ì½ÓµÍËÙ¡¢SWIMÐÅºÅ
	GPIO_Init(GPIOD, GPIO_PIN_1, GPIO_MODE_OUT_OD_HIZ_SLOW);

	for(i=0;i<32;i++){
		GPIO_WriteLow (GPIOD,GPIO_PIN_1);
		Delay(1000);
		if( GPIO_Read(SPMODE1_PORT	, SPMODE1_PIN) ) break;
		GPIO_WriteHigh (GPIOD,GPIO_PIN_1);
		Delay(1000);
		if( GPIO_Read(SPMODE1_PORT	, SPMODE1_PIN)  == RESET ) break;
	}
	if ( i == 32 ){
		for(i=0;i<0xFF;i++){
			if ( GetVolStabed(&uiVol) && (uiVol > 500) ) break;
			FEED_DOG();  
		}
		sBike.uiBatVoltage		= uiVol;
		//sBike.siTemperature= GetTemp();
		//sBike.ucSpeed		= GetSpeed();

		sConfig.uiVolScale	= (uint32_t)sBike.uiBatVoltage*1000UL/VOL_CALIBRATIOIN;	//60.00V
		//sConfig.TempScale	= (long)sBike.siTemperature*1000UL/TEMP_CALIBRATIOIN;	//25.0C
		WriteConfig();
	}

	CFG->GCR &= ~CFG_GCR_SWD;
}

void main(void)
{
	uint8_t i;
	uint16_t uiTick=0;
	uint16_t uiCount=0;
	uint16_t uiVol=0;
	uint16_t tick_100ms=0;
	
	/* select Clock = 8 MHz */
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV2);
	CLK_HSICmd(ENABLE);
	//IWDG_Config();
	
#ifdef RESET_CONFIG
	ResetConfig();
	while(1){ 
		if ( Get_ElapseTick(uiTick) > 1000 ){
			uiTick = Get_SysTick();
			if ( i ){ i = 0; DisplayInit(i); } 
			else 	{ i = 1; DisplayInit(i); }
			MenuUpdate(&sBike);
		}
		FEED_DOG(); 
	}
#else

	InitTimer();  
	HotReset();
    //sBike.bHotReset = 0;
	if ( sBike.bHotReset == 0 ) {
		DisplayInit(1);
	} else
		DisplayInit(0);
	
	for(i=0;i<32;i++){	GetVol();	/*FEED_DOG(); */ }
//	for(i=0;i<16;i++){	GetSpeed();	/*FEED_DOG(); */ }
	for(i=0;i<4;i++) {	GetTemp();	FEED_DOG(); }

	InitConfig();
	Calibration();
	
#if ( TIME_ENABLE == 1 )	
	//sBike.bHasTimer = !PCF8563_Check();
	sBike.bHasTimer = PCF8563_GetTime(PCF_Format_BIN,&RtcTime);
	#ifndef DENGGUAN_XUNYING_T
	InitUART();
	#endif
#endif

#if ( YXT_ENABLE == 1 )
	YXT_Init();  
#endif
  
	ENABLE_INTERRUPTS();
	
	if ( sBike.bHotReset == 0 ) {
		while ( Get_SysTick() < PON_ALLON_TIME ) FEED_DOG();
		DisplayInit(0);
	}
	
	GetVolStabed(&uiVol);
	sBike.uiBatVoltage = (uint32_t)uiVol*1000UL/sConfig.uiVolScale;
	sBike.siTemperature = GetTemp();
	
	while(1){
		uiTick = Get_SysTick();
		
		if ( (uiTick >= tick_100ms && (uiTick - tick_100ms) >= 100 ) || \
			 (uiTick <  tick_100ms && (0xFFFF - tick_100ms + uiTick) >= 100 ) ) {
			tick_100ms = uiTick;
			uiCount ++;
			
			//if ( (uiCount % 5) == 0 ) 
			{
			#ifdef JINPENG_MR9737
					sBike.uiBatVoltage  = GetVol();
					sBike.uiBatVoltage2	= GetVol2();
			#else
				if ( GetVolStabed(&uiVol) ){
					sBike.uiBatVoltage  = (uint32_t)(uiVol+300)*1000UL/sConfig.uiVolScale;
				}
				//sBike.uiBatVoltage  = (uint32_t)GetVol()+300;
			#endif
			}
			if ( (uiCount % 10) == 0 ){
			//	sBike.siTemperature= (long)GetTemp()	*1000UL/sConfig.TempScale;
				sBike.siTemperature= GetTemp();
			}
		
			LightTask();
			MileTask(); 
			
		#if ( YXT_ENABLE == 1 )
			YXT_Task(&sBike,&sConfig);  
		#endif
		
			SpeedCaltTask();
		
		#if ( TIME_ENABLE == 1 )	
			TimeTask();   
		#endif
      
		#ifdef LCD_SEG_TEST
			if ( uiCount >= 100 ) uiCount = 0;
			sBike.uiBatVoltage 	= uiCount/10 + uiCount/10*10UL + uiCount/10*100UL + uiCount/10*1000UL;
			sBike.siTemperature	= uiCount/10 + uiCount/10*10UL + uiCount/10*100UL;
			sBike.ucSpeed		= uiCount/10 + uiCount/10*10;
			sBike.ulMile		= uiCount/10 + uiCount/10*10UL + uiCount/10*100UL + uiCount/10*1000UL + uiCount/10*10000UL;
			sBike.ucHour       	= uiCount/10 + uiCount/10*10;
			sBike.ucMinute     	= uiCount/10 + uiCount/10*10;
			#ifdef LCD8794GCT
		//	sBike.ucEnergy     	= uiCount/10 + uiCount/10*10UL;
			#endif
		#endif
	
			Display(&sBike);
			
			/* Reload IWDG counter */
			FEED_DOG();  
		} 

	#if ( TIME_ENABLE == 1 )
		#ifndef DENGGUAN_XUNYING_T
		UartTask();
		#endif
	#endif
	}
#endif	
}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
