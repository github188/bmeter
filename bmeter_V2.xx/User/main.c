
#include "stm8s.h"
#include "stm8s_adc1.h"   
#include "stm8s_flash.h"  
#include "stm8s_tim2.h" 
#include "stm8s_iwdg.h"

#include "bl55072.h"
#include "display.h"
#include "pcf8563.h"
#include "bike.h"
#include "YXT.h"

#define ContainOf(x) (sizeof(x)/sizeof(x[0]))

#ifdef JINPENG_4860
const unsigned int BatStatus48[8] = {420,426,432,439,445,451,457,464};
const unsigned int BatStatus60[8] = {520,528,536,542,550,558,566,574};
const unsigned int BatStatus72[8] = {0};
#elif defined JINPENG_6072
const unsigned int BatStatus48[8] = {0};
const unsigned int BatStatus60[8] = {480,493,506,519,532,545,558,570};
const unsigned int BatStatus72[8] = {550,569,589,608,628,647,667,686};
#elif defined LCD6040
const unsigned int BatStatus48[] = {425,432,444,456,468};
const unsigned int BatStatus60[] = {525,537,553,566,578};
const unsigned int BatStatus72[] = {630,641,661,681,701};
#else
const unsigned int BatStatus48[8] = {420,427,435,444,453,462,471,481};
const unsigned int BatStatus60[8] = {520,531,544,556,568,577,587,595};
const unsigned int BatStatus72[8] = {630,642,653,664,675,687,700,715};
#endif

volatile unsigned int  sys_tick = 0;
unsigned int tick_100ms=0;
unsigned int speed_buf[16];
unsigned int vol_buf[32];
unsigned int temp_buf[4];
unsigned char uart1_buf[16];
unsigned char uart1_index=0;
const unsigned int* BatStatus;

BIKE_STATUS bike;
__no_init BIKE_CONFIG config;


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
  IWDG_ReloadCounter();
}


unsigned int Get_SysTick(void)
{
	unsigned int tick;
	
	disableInterrupts();
	tick = sys_tick;
	enableInterrupts();
	
	return tick;
}

unsigned int Get_ElapseTick(unsigned int pre_tick)
{
	unsigned int tick = Get_SysTick();

	if ( tick >= pre_tick )	
		return (tick - pre_tick); 
	else 
		return (0xFFFF - pre_tick + tick);
}

void Init_timer(void)
{
	/** ÅäÖÃTimer2 **/ 
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	TIM2_TimeBaseInit(TIM2_PRESCALER_8, 1000);   //1ms
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);            
	TIM2_Cmd(ENABLE);     
}

const int32_t NTC_B3450[29][2] = 
{
	251783,	-400,	184546,	-350,	137003,	-300,	102936,	-250,	78219,	-200,
	60072,	-150,	46601,	-100,	36495,	-50,	28837,	0,		22980,	50,
	18460,	100,	14942,	150,	12182,	200,	10000,	250,	8263,	300,
	6869,	350,	5745,	400,	4832,	450,	4085,	500,	3472,	550,
	2965,	600,	2544,	650,	2193,	700,	1898,	750,	1649,	800,
	1439,	850,	1260,	900,	1108,	950,	977,	1000
};

int32_t NTCtoTemp(int32_t ntc)
{
	unsigned char i,j;

	if ( ntc > NTC_B3450[0][0] ){
		return NTC_B3450[0][1];
	} else {
		for(i=0;i<sizeof(NTC_B3450)/sizeof(NTC_B3450[0][0])/2-1;i++){
			if ( ntc <= NTC_B3450[i][0] && ntc > NTC_B3450[i+1][0] )
				break;
		}
		if ( i == sizeof(NTC_B3450)/sizeof(NTC_B3450[0][0])/2-1 ){
			return NTC_B3450[28][1];
		} else {
			for(j=0;j<50;j++){
				if ( NTC_B3450[i][0] - (j*(NTC_B3450[i][0] - NTC_B3450[i+1][0])/50) <= ntc )
					return NTC_B3450[i][1] + j;
			}
			return NTC_B3450[i+1][1];
		}
	}
}

int GetTemp(void)
{
	static unsigned char index = 0;
	int32_t temp;
	unsigned char i;

	GPIO_Init(GPIOD, GPIO_PIN_6, GPIO_MODE_IN_FL_NO_IT);  //Temp
	ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_6, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, ADC1_SCHMITTTRIG_CHANNEL6 ,\
				DISABLE);
	ADC1_Cmd(ENABLE);
	Delay(1000);
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	temp = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);
  
	temp_buf[index++] = temp;
	if ( index >= ContainOf(temp_buf) )
		index = 0;
	for(i=0,temp=0;i<ContainOf(temp_buf);i++)
		temp += temp_buf[i];
	temp /= ContainOf(temp_buf);

	//temp = 470UL*1024/(1024-temp)-470;
	//temp = NTCtoTemp(temp)/10;
	//temp = ((3600- (long)temp * 2905/1024)/10);

	temp = 10000*1024UL/(1024-temp)-10000;
	temp = NTCtoTemp(temp);
	if ( temp > 999  ) temp =  999;
	if ( temp < -999 ) temp = -999;
	
	return temp;
}
#if 0
unsigned int GetVol(void)
{
	static unsigned char index = 0;
	unsigned int vol;
	unsigned char i;

	GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);  //B+  
	ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_2, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, ADC1_SCHMITTTRIG_CHANNEL2,\
				DISABLE);
	ADC1_Cmd(ENABLE);
	Delay(5000);  
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	vol = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);

	vol_buf[index++] = vol;
	if ( index >= ContainOf(vol_buf) )
		index = 0;
	for(i=0,vol=0;i<ContainOf(vol_buf);i++)
		vol += vol_buf[i];
	vol /= ContainOf(vol_buf);
	vol = (unsigned long)vol*1050/1024 ;
	
	return vol;
}
#else
unsigned char GetVolStabed(unsigned int* vol)
{
	unsigned long mid;
	int buf[32];
	unsigned char i;
	
	GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);  //B+  
	ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_2, ADC1_PRESSEL_FCPU_D2, \
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, ADC1_SCHMITTTRIG_CHANNEL2,\
				DISABLE);

	ADC1_Cmd(ENABLE);
	for(i=0;i<32;i++){
		Delay(500);  
		ADC1_StartConversion(); 
		while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
		buf[i] = ADC1_GetConversionValue();
	}
	ADC1_Cmd(DISABLE);
	
	*vol = (unsigned long)buf[0]*1050UL/1024UL;

	for(i=0,mid=0;i<32;i++)	mid += buf[i];
	mid /= 32;
	for( i=0;i<32;i++){
		if ( mid > 5 && ((mid *100 / buf[i]) > 101 ||  (mid *100 / buf[i]) < 99) )
			return 0;
	}
	
	return 1;
}

#endif

#if ( PCB_VER == 0013 )
unsigned char GetSpeedAdj(void)
{
	static unsigned char index = 0;
	unsigned int adj;
	unsigned char i;

	ADC1_DeInit();  
	ADC1_Init(	ADC1_CONVERSIONMODE_CONTINUOUS, SPEEDV_ADJ_CH, ADC1_PRESSEL_FCPU_D2,\
				ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, SPEEDV_ADJ_SCH,DISABLE);

	ADC1_Cmd(ENABLE);
	Delay(1000);  
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	adj = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);
  	
	speed_buf[index++] = adj;
	if ( index >= ContainOf(speed_buf) )
		index = 0;

	for(i=0,adj=0;i<ContainOf(speed_buf);i++)
		adj += speed_buf[i];
	adj /= ContainOf(speed_buf);
	
	if ( adj > 99 )
		adj = 99;
	
  return adj;
}
#endif

unsigned char GetSpeed(void)
{
	static unsigned char index = 0;
	unsigned int speed;
	unsigned char i;

	//GPIO_Init(GPIOD, GPIO_PIN_2, GPIO_MODE_IN_FL_NO_IT);
	ADC1_DeInit();  
	ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, SPEEDV_ADC_CH, ADC1_PRESSEL_FCPU_D2, \
			ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, SPEEDV_ADC_SCH,\
			DISABLE);

	ADC1_Cmd(ENABLE);
	Delay(1000);  
	ADC1_StartConversion(); 
	while ( ADC1_GetFlagStatus(ADC1_FLAG_EOC) == RESET );  
	speed = ADC1_GetConversionValue();
	ADC1_Cmd(DISABLE);
  	
	speed_buf[index++] = speed;
	if ( index >= ContainOf(speed_buf) )
		index = 0;

	for(i=0,speed=0;i<ContainOf(speed_buf);i++)
		speed += speed_buf[i];
	speed /= ContainOf(speed_buf);
	
	if ( config.SysVoltage	== 48 ){	// speed*5V*21/1024/24V*45 KM/H
		switch( bike.SpeedMode ){
		case 0:
		#ifdef JINPENG_4860
			speed = (unsigned long)speed*1505UL/8192UL;		//24V->43KM/H
		#elif (defined DENGGUAN_XUNYING) || (defined DENGGUAN_XUNYING_T)
			speed = (unsigned long)speed*1925UL/8192UL;		//24V->55KM/H
		#elif (defined OUPAINONG_4860)
			speed = (unsigned long)speed*57750UL/242688UL;	//23.7V->55KM/H
		#else
			speed = (unsigned long)speed*1925UL/8192UL;		//24V->555KM/H
		#endif
			break;
		case 1:
		case 2:
			speed = (unsigned long)speed*1645UL/8192UL;		//24V->47KM/H
			break;
		case 3:
			speed = (unsigned long)speed*15UL/64UL;			//24.5V->56KM/H
			break;
		default: break;
		}
	} else if ( config.SysVoltage	== 60 ) {	// speed*5V*21/1024/30V*45 KM/H
		switch( bike.SpeedMode ){
		case 0:
		#if ( defined JINPENG_4860 ) 
			speed = (unsigned long)speed*301UL/2048UL;		//30V->43KM/H
		#elif defined JINPENG_6072
			speed = (unsigned long)speed*1505UL/8192UL;		//24V->43KM/H
		#elif (defined DENGGUAN_XUNYING) || (defined DENGGUAN_XUNYING_T)
			speed = (unsigned long)speed*385UL/2048UL;		//30V->55KM/H
		#elif (defined OUPAINONG_4860)
			speed = (unsigned long)speed*63000UL/258048UL;	//25.2V->60KM/H
		#elif (defined OUPAINONG_6072)
			speed = (unsigned long)speed*68250UL/339968UL;	//33.2V->65KM/H
		#else
			speed = (unsigned long)speed*385/2048;			//30V->55KM/H
		#endif
			break;
		case 1:
		case 2:
			speed = (unsigned long)speed*4935UL/31744UL;	//31V->47KM/H
			break;
		case 3:
			speed = (unsigned long)speed*5880UL/32256UL;	//31.5V->56KM/H
			break;
		default: break;
		}
	} else if ( config.SysVoltage	== 72 )	{// speed*5V*21/1024/36V*45 KM/H
		switch( bike.SpeedMode ){
		case 0:
		#if defined JINPENG_6072
			speed = (unsigned long)speed*1505UL/12288UL;	//36V->43KM/H
		#elif (defined DENGGUAN_XUNYING) || (defined DENGGUAN_XUNYING_T)
			speed = (unsigned long)speed*1925UL/12288UL;	//36V->55KM/H
		#elif (defined OUPAINONG_6072)
			speed = (unsigned long)speed*68250UL/339968UL;	//33.2V->65KM/H
		#else
			speed = (unsigned long)speed*1925UL/12288UL;	//36V->55KM/H
		#endif
			break;
		case 1:
		case 2:
			speed = (unsigned long)speed*4935UL/36864UL;	//36V->47KM/H
			break;
		case 3:
			speed = (unsigned long)speed*5880UL/37376UL;	//36.5V->56KM/H
			break;
		default: break;
		}
	}
	if ( speed > 99 )
		speed = 99;
	
  return speed;
}

BitStatus GPIO_Read(GPIO_TypeDef* GPIOx, GPIO_Pin_TypeDef GPIO_Pin)
{
	GPIO_Init(GPIOx, GPIO_Pin, GPIO_MODE_IN_FL_NO_IT);
	return GPIO_ReadInputPin(GPIOx, GPIO_Pin);
}

void Light_Task(void)
{
	unsigned char speed_mode=0;

	if( GPIO_Read(NearLight_PORT, NearLight_PIN	) ) bike.NearLight = 1; else bike.NearLight = 0;
	//if( GPIO_Read(TurnRight_PORT, TurnRight_PIN	) ) bike.TurnRight = 1; else bike.TurnRight = 0;
	//if( GPIO_Read(TurnLeft_PORT	, TurnLeft_PIN	) ) bike.TurnLeft  = 1; else bike.TurnLeft  = 0;
	//if( GPIO_Read(Braked_PORT		, Braked_PIN	) ) bike.Braked    = 1; else bike.Braked  	= 0;
	
	if ( bike.YXTERR ){
		speed_mode = 0;
		if( GPIO_Read(SPMODE1_PORT,SPMODE1_PIN) ) speed_mode |= 1<<0;
		if( GPIO_Read(SPMODE2_PORT,SPMODE2_PIN) ) speed_mode |= 1<<1;
		if( GPIO_Read(SPMODE3_PORT,SPMODE3_PIN) ) speed_mode |= 1<<2;
	#ifdef SPMODE4_PORT
		if( GPIO_Read(SPMODE4_PORT,SPMODE4_PIN) ) speed_mode |= 1<<3;
	#endif
		switch(speed_mode){
			case 0x01: 	bike.SpeedMode = 1; break;
			case 0x02: 	bike.SpeedMode = 2; break;
			case 0x04: 	bike.SpeedMode = 3; break;
			case 0x08: 	bike.SpeedMode = 4; break;
			default:	bike.SpeedMode = 0; break;
		}
		bike.Speed = (unsigned long)GetSpeed()*1000UL/config.SpeedScale;
	}
}

void HotReset(void)
{
	if (config.bike[0] == 'b' &&
		config.bike[1] == 'i' && 
		config.bike[2] == 'k' && 
		config.bike[3] == 'e' ){
		bike.HotReset = 1;
	} else {
		bike.HotReset = 0;
	}
}

void WriteConfig(void)
{
	unsigned char *cbuf = (unsigned char *)&config;
	unsigned char i;

	FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);
	FLASH_Unlock(FLASH_MEMTYPE_DATA);  
	Delay(5000);

	config.bike[0] = 'b';
	config.bike[1] = 'i';
	config.bike[2] = 'k';
	config.bike[3] = 'e';
	for(config.Sum=0,i=0;i<sizeof(BIKE_CONFIG)-1;i++)
		config.Sum += cbuf[i];
		
	for(i=0;i<sizeof(BIKE_CONFIG);i++)
		FLASH_ProgramByte(0x4000+i, cbuf[i]);

	Delay(5000);
	FLASH_Lock(FLASH_MEMTYPE_DATA);
}

void InitConfig(void)
{
	unsigned char *cbuf = (unsigned char *)&config;
	unsigned char i,sum;

	for(i=0;i<sizeof(BIKE_CONFIG);i++)
		cbuf[i] = FLASH_ReadByte(0x4000 + i);

	for(sum=0,i=0;i<sizeof(BIKE_CONFIG)-1;i++)
		sum += cbuf[i];
		
	if (config.bike[0] != 'b' || 
		config.bike[1] != 'i' || 
		config.bike[2] != 'k' || 
		config.bike[3] != 'e' || 
		sum != config.Sum ){
		config.VolScale  	= 1000;
		config.TempScale 	= 1000;
		config.SpeedScale	= 1000;
		config.Mile			= 0;
		config.SysVoltage 	= 60;
	}

#ifdef LCD6040
 	bike.Mile = 0; 
#else
	bike.Mile = config.Mile;
#endif
	bike.HasTimer = 0;
	//bike.SpeedMode = SPEEDMODE_DEFAULT;
	bike.YXTERR = 1;
	
#if ( PCB_VER == 0041 )
	config.SysVoltage = 60;
#else
	#if defined BENLING_OUSHANG
		unsigned int vol;
		for(i=0;i<0xFF;i++){
			if ( GetVolStabed(&vol) && (vol > 120) ) break;
			IWDG_ReloadCounter();  
		}
		if ( 720 <= vol && vol <= 870 ){
			config.SysVoltage = 72;
			WriteConfig();
		} else if ( 480 <= vol && vol <= 600 ){
			config.SysVoltage = 60;
			WriteConfig();
		}
	#elif defined BENLING_BL48_60
		unsigned int vol;
		for(i=0;i<0xFF;i++){
			if ( GetVolStabed(&vol) && (vol > 120) ) break;
			IWDG_ReloadCounter();  
		}
		if ( 610 <= vol && vol <= 720 ){
			config.SysVoltage = 60;
			WriteConfig();
		}	else if ( 360 <= vol && vol <= 500 ){
			config.SysVoltage = 48;
			WriteConfig();
		}		
	#elif defined BENLING_ZHONGSHA
		config.SysVoltage = 72;
	#elif (defined OUJUN) || (defined OUPAINONG_6072)
		//GPIO_Init(VMODE1_PORT, VMODE1_PIN, GPIO_MODE_IN_PU_NO_IT);
		GPIO_Init(VMODE2_PORT, VMODE2_PIN, GPIO_MODE_IN_PU_NO_IT);
		if ( GPIO_ReadInputPin(VMODE2_PORT, VMODE2_PIN) == RESET ){
			config.SysVoltage = 72;
		} else {
			config.SysVoltage = 60;
		}
	#elif defined OUPAINONG_4860
		GPIO_Init(VMODE2_PORT, VMODE2_PIN, GPIO_MODE_IN_PU_NO_IT);
		if ( GPIO_ReadInputPin(VMODE2_PORT, VMODE2_PIN) == RESET ){
			config.SysVoltage = 48;
		} else {
			config.SysVoltage = 60;
		}
	#elif defined LCD9040_4860
		GPIO_Init(VMODE2_PORT, VMODE2_PIN, GPIO_MODE_IN_PU_NO_IT);
		if ( GPIO_ReadInputPin(VMODE2_PORT, VMODE2_PIN) == RESET ){
			config.SysVoltage = 60;
		} else {
			config.SysVoltage = 48;
		}
	#else
		GPIO_Init(VMODE1_PORT, VMODE1_PIN, GPIO_MODE_IN_PU_NO_IT);
		GPIO_Init(VMODE2_PORT, VMODE2_PIN, GPIO_MODE_IN_PU_NO_IT);
		if ( GPIO_ReadInputPin(VMODE1_PORT, VMODE1_PIN) == RESET ){
			config.SysVoltage = 72;
		} else {
			if ( GPIO_ReadInputPin(VMODE2_PORT, VMODE2_PIN) == RESET ){
				config.SysVoltage = 48;
			} else {
				config.SysVoltage = 60;
			}
		}
	#endif
#endif

	switch ( config.SysVoltage ){
	case 48:BatStatus = BatStatus48;break;
	case 60:BatStatus = BatStatus60;break;
	case 72:BatStatus = BatStatus72;break;
	default:BatStatus = BatStatus60;break;
	}
}

unsigned char GetBatStatus(unsigned int vol)
{
	unsigned char i;

	for(i=0;i<ContainOf(BatStatus60);i++)
		if ( vol < BatStatus[i] ) break;
	return i;
}

#ifdef LCD8794GCT

const unsigned int BatEnergy48[8] = {420,490};
const unsigned int BatEnergy60[8] = {520,620};
const unsigned int BatEnergy72[8] = {630,740};
const unsigned int* BatEnergy;

unsigned char GetBatEnergy(unsigned int vol)
{
	unsigned int energy ;
	
	switch ( config.SysVoltage ){
	case 48:BatEnergy = BatEnergy48;break;
	case 60:BatEnergy = BatEnergy60;break;
	case 72:BatEnergy = BatEnergy72;break;
	default:BatEnergy = BatEnergy60;break;
	}

	if ( bike.Voltage <= BatEnergy[0] ) energy = 0;
	else if ( bike.Voltage >= BatEnergy[1] ) energy = 100;
	else {
		energy = (bike.Voltage - BatEnergy[0])*100/(BatEnergy[1] - BatEnergy[0]);
	}
	return energy;
}
#endif

unsigned char MileResetTask(void)
{
	static unsigned char TaskFlag = 0;
	static unsigned char lastLight = 0;
	static unsigned char count = 0;
	
	if ( TaskFlag == 2 ){
		return 0;
	}

	if ( Get_SysTick() < 3000 ){
		if ( bike.TurnRight == 1 && TaskFlag == 0 ){
			TaskFlag = 1;
			count = 0;
		}			
	} else if ( Get_SysTick() > 10000 )
		TaskFlag = 2;
		
	if ( TaskFlag == 1 ) {
		if ( lastLight == 0 && bike.NearLight){
			count ++;
			if ( count >= 8 ){
				TaskFlag = 2;
				return 1;
			}
		}
		lastLight = bike.NearLight;
	}	
	return 0;
}

void MileTask(void)
{
	static unsigned int Fmile = 0;
	static unsigned int time = 0;
	unsigned char speed;
	
	speed = bike.Speed;
	if ( speed > DISPLAY_MAX_SPEED )
		speed = DISPLAY_MAX_SPEED;
	
#ifdef SINGLE_TRIP
	time ++;
	if ( time < 20 ) {	//2s
		bike.Mile = config.Mile;
	} else if ( time < 50 ) { 	//5s
		if ( speed ) {
			time = 50;
			bike.Mile = 0;
		}
	} else if ( time == 50 ){
		bike.Mile = 0;
	} else 
#endif	
	{
		time = 51;
		
		Fmile = Fmile + speed;
		if(Fmile >= 36000)
		{
			Fmile = 0;
			bike.Mile++;
			if ( bike.Mile > 99999 )	bike.Mile = 0;
			config.Mile ++;
			if ( config.Mile > 99999 )	config.Mile = 0;
			WriteConfig();
		}  
	}
#ifdef RESET_MILE_ENABLE	
	if ( MileResetTask() ){
		Fmile = 0;
		bike.Mile = 0;
		config.Mile = 0;
		WriteConfig();
	}
#endif
}

#if ( TIME_ENABLE == 1 )
void TimeTask(void)
{
	static unsigned char time_task=0,left_on= 0,NL_on = 0;
	static unsigned int pre_tick;
	
	if (!bike.HasTimer)
		return ;
	
	if (bike.Speed > 1)
		time_task = 0xff;
	
	switch ( time_task ){
	case 0:
		if ( Get_SysTick() < 5000 && bike.NearLight == 0 && bike.TurnLeft == 1 ){
			pre_tick = Get_SysTick();
			time_task++;
		}
		break;
	case 1:
		if ( bike.TurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 2:
		if ( bike.TurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 3:
		if ( bike.TurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 4:
		if ( bike.TurnLeft == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 5:
		if ( bike.TurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 6:
		if ( bike.TurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
	case 7:
		if ( bike.TurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 8:
		if ( bike.TurnLeft == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
	case 9:
		if ( bike.TurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 10:
		if ( bike.TurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.NearLight ) time_task = 0xFF;
		}
	case 11:
		if ( bike.TurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.NearLight ) time_task = 0xFF;
		}
		break;
	case 12:
		if ( bike.TurnLeft == 1 || bike.NearLight ){
			 time_task = 0xFF;
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000 ) {
				time_task= 0;
				bike.time_pos = 0;
				bike.time_set = 1; 
				pre_tick = Get_SysTick();
			}
		}
		break;
	default:
		bike.time_pos = 0;
		bike.time_set = 0; 
		time_task = 0;
		break;
	}

	if ( bike.time_set ){
		if ( bike.TurnLeft ) { left_on = 1; }
		if ( bike.TurnLeft == 0 ) {
			if ( left_on == 1 ){
				bike.time_pos ++;
				bike.time_pos %= 4;
				left_on = 0;
				pre_tick = Get_SysTick();
			}
		}
		if ( bike.NearLight ) { NL_on = 1; pre_tick = Get_SysTick(); }
		if ( bike.NearLight == 0 && NL_on == 1 ) {
			NL_on = 0;
			if ( Get_ElapseTick(pre_tick) < 5000 ){
				switch ( bike.time_pos ){
				case 0:
					bike.Hour += 10;
					bike.Hour %= 20;
					break;
				case 1:
					if ( bike.Hour % 10 < 9 )
						bike.Hour ++;
					else 
						bike.Hour -= 9;
					break;
				case 2:
					bike.Minute += 10;
					bike.Minute %= 60;
					break;
				case 3:
					if ( bike.Minute % 10 < 9 )
						bike.Minute ++;
					else 
						bike.Minute -= 9;
					break;
				default:
					bike.time_set = 0;
					break;
				}
			}
			RtcTime.RTC_Hours 	= bike.Hour;
			RtcTime.RTC_Minutes = bike.Minute;
			PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
		}
		if ( Get_ElapseTick(pre_tick) > 30000 ){
			bike.time_set = 0;
		}
	}		
	
	 PCF8563_GetTime(PCF_Format_BIN,&RtcTime);
	 bike.Hour 		= RtcTime.RTC_Hours%12;
	 bike.Minute 	= RtcTime.RTC_Minutes;
}


void InitUART(void)
{
	if ( bike.uart == 0 )
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
	unsigned int vol,i;
	
	if ( bike.uart == 0 )
		return ;
	
	if ( uart1_index > 0 && uart1_buf[uart1_index-1] == '\n' ){
		if ( uart1_index >= 11 && uart1_buf[0] == 'T' /*&& uart1_buf[1] == 'i' && uart1_buf[2] == 'm' && uart1_buf[3] == 'e' */) {
			RtcTime.RTC_Hours 	= (uart1_buf[5]-'0')*10 + (uart1_buf[6] - '0');
			RtcTime.RTC_Minutes = (uart1_buf[8]-'0')*10 + (uart1_buf[9] - '0');
			PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
		} else if ( uart1_index >= 5 && uart1_buf[0] == 'C' /*&& uart1_buf[1] == 'a' && uart1_buf[2] == 'l' && uart1_buf[3] == 'i' */){
			for(i=0;i<0xFF;i++){
				if ( GetVolStabed(&vol) && (vol > 120) ) break;
				IWDG_ReloadCounter();  
			}
			bike.Voltage	 = vol;
			bike.Temperature = GetTemp();
			bike.Speed		 = GetSpeed();

			config.VolScale	 = (unsigned long)bike.Voltage*1000UL/VOL_CALIBRATIOIN;					
		//	config.TempScale = (long)bike.Temperature*1000UL/TEMP_CALIBRATIOIN;	
			config.SpeedScale= (unsigned long)bike.Speed*1000UL/SPEED_CALIBRATIOIN;				
			WriteConfig();
		}
		uart1_index = 0;
	}
}
#endif 

void Calibration(void)
{
	unsigned char i;
	unsigned int vol;
	
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
			if ( GetVolStabed(&vol) && (vol > 120) ) break;
			IWDG_ReloadCounter();  
		}
		bike.Voltage		= vol;
		//bike.Temperature	= GetTemp();
		//bike.Speed		= GetSpeed();

		config.VolScale		= (unsigned long)bike.Voltage*1000UL/VOL_CALIBRATIOIN;					//60.00V
		//config.TempScale	= (long)bike.Temperature*1000UL/TEMP_CALIBRATIOIN;	//25.0C
		//config.SpeedScale = (unsigned long)bike.Speed*1000UL/SPEED_CALIBRATIOIN;				//30km/h
		//config.Mile = 0;
		WriteConfig();
	}
	
	for(i=0;i<32;i++){
		GPIO_WriteLow (GPIOD,GPIO_PIN_1);
		Delay(1000);
		if( GPIO_Read(SPMODE2_PORT	, SPMODE2_PIN) ) break;
		GPIO_WriteHigh (GPIOD,GPIO_PIN_1);
		Delay(1000);
		if( GPIO_Read(SPMODE2_PORT	, SPMODE2_PIN)  == RESET ) break;
	}
	if ( i == 32 ){
		bike.uart = 1;
	} else
		bike.uart = 0;

	CFG->GCR &= ~CFG_GCR_SWD;
}

void main(void)
{
	unsigned char i;
	unsigned int tick;
	unsigned int count=0;
	unsigned int vol=0,vol_index=0,temp;
	
	/* select Clock = 8 MHz */
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV2);
	CLK_HSICmd(ENABLE);
	//IWDG_Config();

	Init_timer();  
	HotReset();
	if ( bike.HotReset == 0 ) {
		BL55072_Config(1);

	#if ( PCB_VER == 0041 )
		GPIO_Init(TurnLeftOut_PORT, TurnLeftOut_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);
		GPIO_WriteLow (TurnLeftOut_PORT,TurnLeftOut_PIN);
		GPIO_Init(TurnRightOut_PORT, TurnRightOut_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);
		GPIO_WriteLow (TurnRightOut_PORT,TurnRightOut_PIN);
		CFG->GCR = CFG_GCR_SWD;
		GPIO_Init(NearLightOut_PORT, NearLightOut_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);
		GPIO_WriteLow (NearLightOut_PORT,NearLightOut_PIN);
	#endif
	} else
		BL55072_Config(0);

//	for(i=0;i<32;i++){	GetVol();	IWDG_ReloadCounter();  }
	for(i=0;i<16;i++){	GetSpeed();	IWDG_ReloadCounter();  }
	for(i=0;i<4;i++) {	GetTemp();	IWDG_ReloadCounter();  }

	InitConfig();
	Calibration();
	if ( bike.HotReset == 0 ) {
	#if ( PCB_VER == 0041 )
		CFG->GCR = CFG_GCR_SWD;
		GPIO_Init(NearLightOut_PORT, NearLightOut_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);
		GPIO_WriteLow (TurnLeftOut_PORT,TurnLeftOut_PIN);
	#endif
	}
	
#if ( TIME_ENABLE == 1 )	
	//bike.HasTimer = !PCF8563_Check();
	bike.HasTimer = PCF8563_GetTime(PCF_Format_BIN,&RtcTime);
	#ifndef DENGGUAN_XUNYING_T
	InitUART();
	#endif
#endif

#if ( YXT_ENABLE == 1 )
	YXT_Init();  
#endif
  
	enableInterrupts();
	
	if ( bike.HotReset == 0 ) {
		while ( Get_SysTick() < PON_ALLON_TIME ) IWDG_ReloadCounter();
		BL55072_Config(0);
	#if ( PCB_VER == 0041 )
		GPIO_WriteHigh (TurnLeftOut_PORT	,TurnLeftOut_PIN);
		GPIO_WriteHigh (TurnRightOut_PORT	,TurnRightOut_PIN);
		GPIO_WriteHigh (NearLightOut_PORT	,NearLightOut_PIN);
	#endif
	}
	
	GetVolStabed(&vol);
	bike.Voltage = (unsigned long)vol*1000UL/config.VolScale;
	bike.Temperature = GetTemp();
	
	while(1){
		tick = Get_SysTick();
		
		if ( (tick >= tick_100ms && (tick - tick_100ms) > 100 ) || \
			 (tick <  tick_100ms && (0xFFFF - tick_100ms + tick) > 100 ) ) {
			tick_100ms = tick;
			count ++;
			
			if ( (count % 4 ) == 0 ){
				if ( GetVolStabed(&vol) )
					bike.Voltage = (unsigned long)vol*1000UL/config.VolScale;
			}
			if ( (count % 10) == 0 ){
				if ( bike.uart == 0 ){
				//	bike.Temperature= (long)GetTemp()	*1000UL/config.TempScale;
					bike.Temperature= GetTemp();
				}
			}
			bike.BatStatus 	= GetBatStatus(bike.Voltage);
		#ifdef LCD8794GCT
			bike.Energy 	= GetBatEnergy(bike.Voltage);
		#endif
		
			Light_Task();
			MileTask(); 
			
		#if ( YXT_ENABLE == 1 )
			YXT_Task(&bike);  
		#endif
		
		#if ( TIME_ENABLE == 1 )	
			TimeTask();   
		#endif
      
		#ifdef LCD_SEG_TEST
			if ( ++count >= 100 ) count = 0;
			bike.Voltage 	= count/10 + count/10*10UL + count/10*100UL + count/10*1000UL;
			bike.Temperature= count/10 + count/10*10UL + count/10*100UL;
			bike.Speed		= count/10 + count/10*10;
			bike.Mile		= count/10 + count/10*10UL + count/10*100UL + count/10*1000UL + count/10*10000UL;
			bike.Hour       = count/10 + count/10*10;
			bike.Minute     = count/10 + count/10*10;
			#ifdef LCD8794GCT
			bike.Energy     = count/10 + count/10*10UL;
			#endif
		#endif
       
			MenuUpdate(&bike);
			
			/* Reload IWDG counter */
			IWDG_ReloadCounter();  
		} 

	#if ( TIME_ENABLE == 1 )
		#ifndef DENGGUAN_XUNYING_T
		UartTask();
		#endif
	#endif
	}
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
