
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
const uint16_t uiBatStatus48[] = BAT_STATUS_48V;
const uint16_t uiBatStatus60[] = BAT_STATUS_60V;
const uint16_t uiBatStatus72[] = BAT_STATUS_72V;

uint16_t uiSpeedBuf[16];
uint16_t uiVolBuf[16];
#ifdef BATV_ADC_CH2
uint16_t uiVol2Buf[8];
#endif
uint16_t uiTempBuf[8];

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
	TIM2_TimeBaseInit(TIM2_PRESCALER_8, 10000);   //10ms
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);            
	TIM2_Cmd(ENABLE);     
}

#ifdef SPEED_HALL_PORT
 void SpeedHallInit(void)
{
	GPIO_Init(SPEED_HALL_PORT, SPEED_HALL_PIN, GPIO_MODE_IN_FL_IT);
    EXTI_SetExtIntSensitivity(SPEED_EXTI_PORT,EXTI_SENSITIVITY_RISE_ONLY);
}
#endif

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
	uint16_t uiBuf[ContainOf(uiTempBuf)];
	int32_t slTemp;
	uint16_t sample;
	uint8_t i;

	GPIO_Init(GPIOD, GPIO_PIN_6, GPIO_MODE_IN_FL_NO_IT);  //Temp
	ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_6, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, ADC1_SCHMITTTRIG_CHANNEL6 ,\
				DISABLE);
	ADC1_Cmd(ENABLE);
	Delay(1000);
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	sample = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);
  
	uiTempBuf[ucIndex++] = sample;
	if ( ucIndex >= ContainOf(uiTempBuf) )
		ucIndex = 0;
	
	for(i=0;i<ContainOf(uiBuf);i++) uiBuf[i] = uiTempBuf[i];
	exchange_sort16(uiBuf,ContainOf(uiBuf));
	sample = get_average16(uiBuf+2,ContainOf(uiBuf)-2*2);

	//slTemp = 10000*1024UL/(1024-sample)-10000;
	slTemp = 10000UL*(int32_t)sample/(1024-sample);
	slTemp = NTCtoTemp(slTemp);
	if ( slTemp > 999  ) slTemp =  999;
	if ( slTemp < -999 ) slTemp = -999;
	
	return slTemp;
}

uint16_t GetVol(void)
{
	static uint8_t ucIndex = 0;
	uint16_t uiVol;
	uint16_t uiBuf[ContainOf(uiVolBuf)];
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

	uiVolBuf[ucIndex++] = get_average16(uiBuf,ContainOf(uiBuf));
	if ( ucIndex >= ContainOf(uiVolBuf) )
		ucIndex = 0;

	for(i=0;i<ContainOf(uiBuf);i++) uiBuf[i] = uiVolBuf[i];
	exchange_sort16(uiBuf,ContainOf(uiBuf));
	for(i=0;i<ContainOf(uiBuf);i++){
		if ( uiBuf[i] > 100 )	break;
	}
	uiVol = 0;
	if ( i + 3*2 < ContainOf(uiBuf) )
		uiVol = get_average16(uiBuf+i+3,ContainOf(uiBuf)-i-3*2);

//	uiVol = (uint32_t)uiVol / 1024UL / 10 * (200+10) * 5 * 10;	// 200k+10k
	uiVol = VOL_CALC(uiVol);
	
	return uiVol;
}

#ifdef BATV_ADC_CH2
uint16_t GetVol2(void)
{
	static uint8_t ucIndex = 0;
	uint16_t uiBuf[ContainOf(uiVol2Buf)];
	uint16_t uiVol;
	uint8_t i;

	GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);  //B+  
	ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, BATV_ADC_CH2, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, BATV_ADC_SCH2,\
				DISABLE);
	ADC1_Cmd(ENABLE);
	for(i=0;i<ContainOf(uiBuf);i++){
		Delay(500);  
		ADC1_StartConversion(); 
		while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
		uiBuf[i] = ADC1_GetConversionValue();
	}
	ADC1_Cmd(DISABLE);

	uiVol2Buf[ucIndex++] = get_average16(uiBuf,ContainOf(uiBuf));
	if ( ucIndex >= ContainOf(uiVol2Buf) )
		ucIndex = 0;
	
	for(i=0;i<ContainOf(uiBuf);i++) uiBuf[i] = uiVol2Buf[i];
	exchange_sort16(uiBuf,ContainOf(uiBuf));
	for(i=0;i<ContainOf(uiBuf);i++){
		if ( uiBuf[i] > 100 )	break;
	}
	uiVol = 0;
	if ( i + 3*2 < ContainOf(uiBuf) )
		uiVol = get_average16(uiBuf+i+3,ContainOf(uiBuf)-i-3*2);
	
//	uiVol = (uint32_t)uiVol / 1024UL / 10 * (200+10) * 5 * 10;	// 200k+10k
	uiVol = VOL_CALC(uiVol);
	
	return uiVol;
}
#endif

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
	uint16_t uiBuf[ContainOf(uiSpeedBuf)];
	uint16_t uiSpeed;
	uint8_t i;

	if ( sBike.bYXTERR ){
	#ifdef SPEEDV_ADC_CH
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
		
		if ( sConfig.uiSysVoltage		== 48 ){
			uiSpeed = SPEED_CALC_48V((uint32_t)uiSpeed);
		} else if ( sConfig.uiSysVoltage== 60 ) {
			uiSpeed = SPEED_CALC_60V((uint32_t)uiSpeed);
		} else if ( sConfig.uiSysVoltage== 72 )	{
			uiSpeed = SPEED_CALC_72V((uint32_t)uiSpeed);
		} else
			uiSpeed = 0;
		uiSpeed = (uint32_t)uiSpeed*1000UL/sConfig.uiSpeedScale;
	#else	
		uiSpeed = GetSpeedHall();
	#endif

		uiSpeedBuf[ucIndex++] = uiSpeed;
		if ( ucIndex >= ContainOf(uiSpeedBuf) )
			ucIndex = 0;

		for(i=0;i<ContainOf(uiBuf);i++)	uiBuf[i] = uiSpeedBuf[i];
		exchange_sort16(uiBuf,ContainOf(uiBuf));
		uiSpeed = get_average16(uiBuf+4,ContainOf(uiBuf)-4*2);
	} else {
		uiSpeed = (uint32_t)sBike.ucYXT_Speed*1000UL/sConfig.uiYXT_SpeedScale;
	}

	if ( uiSpeed > 99 )
		uiSpeed = 99;
	
  return uiSpeed;
}

void GetSysVoltage(void)
{	
#if defined BENLING_OUSHANG
	uint16_t uiVol,i;
	for(i=0;i<0xFF;i++){
		uiVol = GetVolStabed();
		if ( uiVol > 120 ) break;
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
	uint16_t uiVol,i;
	for(i=0;i<0xFF;i++){
		uiVol = GetVolStabed();
		if ( uiVol > 120 ) break;
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
#elif defined VD72L_48PIN
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
#elif defined VD48L72L
	if ( GPIO_Read(V48_PORT, V48_PIN) == RESET ){
		sConfig.uiSysVoltage = 48;
	} else {
		if ( GPIO_Read(V72_PORT, V72_PIN) == RESET ){
			sConfig.uiSysVoltage = 72;
		} else {
			sConfig.uiSysVoltage = 60;
		}
	}
#else
	#error "Please select a system voltage detection method!!!"
#endif
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
	uint16_t uiVol,uiVol2,i;
	
	if ( sBike.bUart == 0 )
		return ;
	
	if ( ucUart1Index > 0 && ucUart1Buf[ucUart1Index-1] == '\n' ){
		if ( ucUart1Index >= 11 && ucUart1Buf[0] == 'T' /*&& ucUart1Buf[1] == 'i' && ucUart1Buf[2] == 'm' && ucUart1Buf[3] == 'e' */) {
			RtcTime.RTC_Hours 	= (ucUart1Buf[5]-'0')*10 + (ucUart1Buf[6] - '0');
			RtcTime.RTC_Minutes = (ucUart1Buf[8]-'0')*10 + (ucUart1Buf[9] - '0');
			PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
		} else if ( ucUart1Index >= 5 && ucUart1Buf[0] == 'C' /*&& ucUart1Buf[1] == 'a' && ucUart1Buf[2] == 'l' && ucUart1Buf[3] == 'i' */){
			for(i=0;i<0xFF;i++){
				uiVol = GetVol();
				if ( uiVol > 120 ) break;
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
	
	//¶Ì½ÓCALI_PIN¡¢SWIMÐÅºÅ
	CFG->GCR = CFG_GCR_SWD;
	GPIO_Init(GPIOD, GPIO_PIN_1, GPIO_MODE_OUT_OD_HIZ_SLOW);

	#if ( PCB_VER == MR9737 )
		DisplayInit(0);
        Display(&sBike);
	#endif
       
    for(i=0;i<64;i++){
		GPIO_WriteLow  (GPIOD,GPIO_PIN_1); FEED_DOG(); Delay(10000);
		if( GPIO_Read(CALI_PORT	, CALI_PIN) ) break;
		GPIO_WriteHigh (GPIOD,GPIO_PIN_1); FEED_DOG(); Delay(10000);
		if( GPIO_Read(CALI_PORT	, CALI_PIN)  == RESET ) break;
	}
	
	if ( i == 64 ){
		for(i=0;i<64;i++){ GetVol();	FEED_DOG();  }
		sBike.uiBatVoltage	= GetVol();
		sConfig.uiVolScale	= (uint32_t)sBike.uiBatVoltage*1000UL /VOL_CALIBRATIOIN;	//60.00V

	#if ( PCB_VER == MR9737 )
		for(i=0;i<64;i++){ GetVol2();	FEED_DOG();  }
		sBike.uiBatVoltage2	= GetVol2();
		sConfig.uiVolScale2	= (uint32_t)sBike.uiBatVoltage2*1000UL /VOL_CALIBRATIOIN;	//60.00V
	#endif
        
	//	sBike.siTemperature	= GetTemp();
	//	sConfig.TempScale	= (uint32_t)sBike.siTemperature*1000UL/TEMP_CALIBRATIOIN;	//25.0C

		WriteConfig();
	}

	CFG->GCR &= ~CFG_GCR_SWD;
}

void main(void)
{
	uint8_t i;
	uint16_t uiPreTick=0;
	uint16_t uiCount=0;
	uint16_t uiVol=0,uiVol2=0;
	
	/* select Clock = 8 MHz */
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV2);
	CLK_HSICmd(ENABLE);
	IWDG_Config();
	
#ifdef RESET_CONFIG
	ResetConfig();
	while(1){ 
		if ( Get_ElapseTick(uiPreTick) > 1000 ){
			uiPreTick = Get_SysTick();
			if ( i ){ i = 0; DisplayInit(i); } 
			else 	{ i = 1; DisplayInit(i); }
			Display(&sBike);
		}
		FEED_DOG(); 
	}
#else

	InitTimer();  
	//HotReset();
    sBike.bHotReset = 0;
	if ( sBike.bHotReset == 0 ) {
		DisplayInit(1);
	} else
		DisplayInit(0);

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

#ifdef SPEED_HALL_PORT
	SpeedHallInit();
#endif
  
	ENABLE_INTERRUPTS();
	
	while ( Get_SysTick() < PON_ALLON_TIME ) FEED_DOG();

#if ( PCB_VER == MR9737 )
	for(i=0;i<ContainOf(uiVolBuf )/2;i++) GetVol();  FEED_DOG();
	for(i=0;i<ContainOf(uiVol2Buf)/2;i++) GetVol2(); FEED_DOG();
#else
	for(i=0;i<ContainOf(uiVolBuf );	 i++) GetVol();  FEED_DOG();
#endif
//	for(i=0;i<ContainOf(uiSpeedBuf); i++) GetSpeed();FEED_DOG();
	for(i=0;i<ContainOf(uiTempBuf);  i++) GetTemp(); FEED_DOG();

	if ( sBike.bHotReset == 0 ) {
		DisplayInit(0);
	}
	
	while(1){
		if ( Get_ElapseTick(uiPreTick) >= 100 ){
			uiPreTick = Get_SysTick();
			
			if ( (uiCount % 2) == 0 ) 
			{
				uiVol  = GetVol();
				sBike.uiBatVoltage  = (uint32_t)(uiVol +0)*1000UL/sConfig.uiVolScale;
			#if ( PCB_VER == MR9737 )
				uiVol2 = GetVol2();
				sBike.uiBatVoltage2 = (uint32_t)(uiVol2+0)*1000UL/sConfig.uiVolScale2;
			#endif
			}
			if ( (uiCount % 10) == 0 ){
			//	sBike.siTemperature= (long)GetTemp()	*1000UL/sConfig.TempScale;
				sBike.siTemperature= GetTemp();
			}

			sBike.ucSpeed 			= GetSpeed();

			uiCount ++;
		
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
