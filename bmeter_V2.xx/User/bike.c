
#include "stm8s.h"

#include "display.h"
#include "pcf8563.h"
#include "bike.h"
#include "YXT.h"

#ifdef JINPENG_4860
const uint16_t BatStatus48[8] = {420,426,432,439,445,451,457,464};
const uint16_t BatStatus60[8] = {520,528,536,542,550,558,566,574};
const uint16_t BatStatus72[8] = {0};
#elif defined JINPENG_6072
const uint16_t BatStatus48[8] = {0};
const uint16_t BatStatus60[8] = {480,493,506,519,532,545,558,570};
const uint16_t BatStatus72[8] = {550,569,589,608,628,647,667,686};
#elif defined LCD6040
const uint16_t BatStatus48[] = {425,432,444,456,468};
const uint16_t BatStatus60[] = {525,537,553,566,578};
const uint16_t BatStatus72[] = {630,641,661,681,701};
#else
const uint16_t BatStatus48[8] = {420,427,435,444,453,462,471,481};
const uint16_t BatStatus60[8] = {520,531,544,556,568,577,587,595};
const uint16_t BatStatus72[8] = {630,642,653,664,675,687,700,715};
#endif

volatile uint16_t  sys_tick = 0;
uint16_t tick_100ms=0;
uint16_t speed_buf[16];
uint16_t vol_buf[28];
uint16_t temp_buf[4];
const uint16_t* ucBatStatus;

#if ( TIME_ENABLE == 1 )
uint8_t uart1_buf[16];
uint8_t uart1_index=0;
#endif

BIKE_STATUS bike;
__no_init BIKE_CONFIG config;


uint16_t Get_ElapseTick(uint16_t pre_tick)
{
	uint16_t tick = Get_SysTick();

	if ( tick >= pre_tick )	
		return (tick - pre_tick); 
	else 
		return (0xFFFF - pre_tick + tick);
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
	uint8_t i,j;

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
	static uint8_t index = 0;
	int32_t temp;
	uint8_t i;

	temp_buf[index++] = GetTempADC();
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
uint16_t GetVol(void)
{
	static uint8_t index = 0;
	uint16_t vol;
	uint8_t i;

	vol_buf[index++] = GetVolADC();
	if ( index >= ContainOf(vol_buf) )
		index = 0;
	for(i=0,vol=0;i<ContainOf(vol_buf);i++)
		vol += vol_buf[i];
	vol /= ContainOf(vol_buf);
	vol = (uint32_t)vol*1050/1024 ;
	
	return vol;
}
#else
	
void GetVolSample(void)
{
	static unsigned char index = 0;
	
    vol_buf[index++] = GetVolADC();
    if ( index >= ContainOf(vol_buf) )
      index = 0;
}

uint8_t GetVolStabed(uint16_t* vol)
{
	uint32_t mid;
	uint8_t i;
	
	for(i=0,mid=0;i<ContainOf(vol_buf);i++)
		mid += vol_buf[i];
	mid /= ContainOf(vol_buf);
	
	*vol = (uint32_t)buf[0]*1050UL/1024UL;

	for(i=0,mid=0;i<32;i++)	mid += buf[i];
	mid /= 32;
	for( i=0;i<32;i++){
		if ( mid > 5 && ((mid *100 / buf[i]) > 101 ||  (mid *100 / buf[i]) < 99) )
			return 0;
	}
	
	return 1;
}

#endif

uint8_t GetSpeed(void)
{
	static uint8_t index = 0;
	uint16_t speed;
	uint8_t i;

	speed_buf[index++] = GetSpeedADC();
	if ( index >= ContainOf(speed_buf) )
		index = 0;

	for(i=0,speed=0;i<ContainOf(speed_buf);i++)
		speed += speed_buf[i];
	speed /= ContainOf(speed_buf);
	
	if ( config.ucSysVoltage	== 48 ){	// speed*5V*21/1024/24V*45 KM/H
		#ifdef JINPENG_4860
			speed = (uint32_t)speed*1505UL/8192UL;		//24V->43KM/H
		#elif (defined DENGGUAN_XUNYING) || (defined DENGGUAN_XUNYING_T)
			speed = (uint32_t)speed*1925UL/8192UL;		//24V->55KM/H
		#elif (defined OUPAINONG_4860)
			speed = (uint32_t)speed*57750UL/242688UL;	//23.7V->55KM/H
		#else
			speed = (uint32_t)speed*1925UL/8192UL;		//24V->555KM/H
		#endif
	} else if ( config.ucSysVoltage	== 60 ) {	// speed*5V*21/1024/30V*45 KM/H
		#if ( defined JINPENG_4860 ) 
			speed = (uint32_t)speed*301UL/2048UL;		//30V->43KM/H
		#elif defined JINPENG_6072
			speed = (uint32_t)speed*1505UL/8192UL;		//24V->43KM/H
		#elif (defined DENGGUAN_XUNYING) || (defined DENGGUAN_XUNYING_T)
			speed = (uint32_t)speed*385UL/2048UL;		//30V->55KM/H
		#elif (defined OUPAINONG_4860)
			speed = (uint32_t)speed*63000UL/258048UL;	//25.2V->60KM/H
		#elif (defined OUPAINONG_6072)
			speed = (uint32_t)speed*68250UL/339968UL;	//33.2V->65KM/H
		#else
			speed = (uint32_t)speed*385/2048;			//30V->55KM/H
		#endif
	} else if ( config.ucSysVoltage	== 72 )	{// speed*5V*21/1024/36V*45 KM/H
		#if defined JINPENG_6072
			speed = (uint32_t)speed*1505UL/12288UL;	//36V->43KM/H
		#elif (defined DENGGUAN_XUNYING) || (defined DENGGUAN_XUNYING_T)
			speed = (uint32_t)speed*1925UL/12288UL;	//36V->55KM/H
		#elif (defined OUPAINONG_6072)
			speed = (uint32_t)speed*68250UL/339968UL;	//33.2V->65KM/H
		#else
			speed = (uint32_t)speed*1925UL/12288UL;	//36V->55KM/H
		#endif
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
	uint8_t speed_mode=0;

	if( GPIO_Read(NearLight_PORT, NearLight_PIN	) ) bike.bNearLight = 1; else bike.bNearLight = 0;
	//if( GPIO_Read(TurnRight_PORT, TurnRight_PIN	) ) bike.bTurnRight = 1; else bike.bTurnRight = 0;
	//if( GPIO_Read(TurnLeft_PORT	, TurnLeft_PIN	) ) bike.bTurnLeft  = 1; else bike.bTurnLeft  = 0;
	//if( GPIO_Read(Braked_PORT		, Braked_PIN	) ) bike.bBraked    = 1; else bike.bBraked  	= 0;
	
	if ( bike.bYXTERR ){
		speed_mode = 0;
		if( GPIO_Read(SPMODE1_PORT,SPMODE1_PIN) ) speed_mode |= 1<<0;
		if( GPIO_Read(SPMODE2_PORT,SPMODE2_PIN) ) speed_mode |= 1<<1;
		if( GPIO_Read(SPMODE3_PORT,SPMODE3_PIN) ) speed_mode |= 1<<2;
	#ifdef SPMODE4_PORT
		if( GPIO_Read(SPMODE4_PORT,SPMODE4_PIN) ) speed_mode |= 1<<3;
	#endif
		switch(speed_mode){
			case 0x01: 	bike.ucSpeedMode = 1; break;
			case 0x02: 	bike.ucSpeedMode = 2; break;
			case 0x04: 	bike.ucSpeedMode = 3; break;
			case 0x08: 	bike.ucSpeedMode = 4; break;
			default:	bike.ucSpeedMode = 0; break;
		}
		bike.ucPHA_Speed= (uint32_t)GetSpeed();
		bike.ucSpeed 	= (uint32_t)bike.ucPHA_Speed*1000UL/config.ucSpeedScale + bike.Speed_dec;
	}
}

void HotReset(void)
{
	if (config.ucBike[0] == 'b' &&
		config.ucBike[1] == 'i' && 
		config.ucBike[2] == 'k' && 
		config.ucBike[3] == 'e' ){
		bike.bHotReset = 1;
	} else {
		bike.bHotReset = 0;
	}
}

void WriteConfig(void)
{
	uint8_t *cbuf = (uint8_t *)&config;
	uint8_t i;

	FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);
	FLASH_Unlock(FLASH_MEMTYPE_DATA);  
	Delay(5000);

	config.ucBike[0] = 'b';
	config.ucBike[1] = 'i';
	config.ucBike[2] = 'k';
	config.ucBike[3] = 'e';
	for(config.ucSum=0,i=0;i<sizeof(BIKE_CONFIG)-1;i++)
		config.ucSum += cbuf[i];
		
	for(i=0;i<sizeof(BIKE_CONFIG);i++)
		FLASH_ProgramByte(0x4000+i, cbuf[i]);

	Delay(5000);
	FLASH_Lock(FLASH_MEMTYPE_DATA);
}

void InitConfig(void)
{
	uint8_t *cbuf = (uint8_t *)&config;
	uint8_t i,sum;

	for(i=0;i<sizeof(BIKE_CONFIG);i++)
		cbuf[i] = FLASH_ReadByte(0x4000 + i);

	for(sum=0,i=0;i<sizeof(BIKE_CONFIG)-1;i++)
		sum += cbuf[i];
		
	if (config.ucBike[0] != 'b' || 
		config.ucBike[1] != 'i' || 
		config.ucBike[2] != 'k' || 
		config.ucBike[3] != 'e' || 
		sum != config.Sum ){
		config.ucSysVoltage = 60;
		config.ucVolScale  	= 1000;
		config.ucTempScale 	= 1000;
		config.ucSpeedScale	= 1000;
		config.ucYXT_SpeedScale= 1000;
		config.ulMile		= 0;
	}

#ifdef LCD6040
 	bike.ulMile = 0; 
#else
	bike.ulMile = config.ulMile;
#endif
#if ( TIME_ENABLE == 1 )
	bike.bHasTimer = 0;
#endif
	//bike.ucSpeedMode = SPEEDMODE_DEFAULT;
	bike.bYXTERR = 1;
	
#if ( PCB_VER == 0041 )
	config.ucSysVoltage = 60;
#else
	#if defined BENLING_OUSHANG
		uint16_t vol;
		for(i=0;i<0xFF;i++){
			if ( GetVolStabed(&vol) && (vol > 120) ) break;
			//IWDG_ReloadCounter();  
		}
		if ( 720 <= vol && vol <= 870 ){
			config.ucSysVoltage = 72;
			WriteConfig();
		} else if ( 480 <= vol && vol <= 600 ){
			config.ucSysVoltage = 60;
			WriteConfig();
		}
	#elif defined BENLING_BL48_60
		uint16_t vol;
		for(i=0;i<0xFF;i++){
			if ( GetVolStabed(&vol) && (vol > 120) ) break;
			//IWDG_ReloadCounter();  
		}
		if ( 610 <= vol && vol <= 720 ){
			config.ucSysVoltage = 60;
			WriteConfig();
		}	else if ( 360 <= vol && vol <= 500 ){
			config.ucSysVoltage = 48;
			WriteConfig();
		}		
	#elif defined BENLING_ZHONGSHA
		config.ucSysVoltage = 72;
	#elif (defined OUJUN) || (defined OUPAINONG_6072)
		//GPIO_Init(VMODE1_PORT, VMODE1_PIN, GPIO_MODE_IN_PU_NO_IT);
		GPIO_Init(VMODE2_PORT, VMODE2_PIN, GPIO_MODE_IN_PU_NO_IT);
		if ( GPIO_ReadInputPin(VMODE2_PORT, VMODE2_PIN) == RESET ){
			config.ucSysVoltage = 72;
		} else {
			config.ucSysVoltage = 60;
		}
	#elif defined OUPAINONG_4860
		GPIO_Init(VMODE2_PORT, VMODE2_PIN, GPIO_MODE_IN_PU_NO_IT);
		if ( GPIO_ReadInputPin(VMODE2_PORT, VMODE2_PIN) == RESET ){
			config.ucSysVoltage = 48;
		} else {
			config.ucSysVoltage = 60;
		}
	#elif defined LCD9040_4860
		GPIO_Init(VMODE2_PORT, VMODE2_PIN, GPIO_MODE_IN_PU_NO_IT);
		if ( GPIO_ReadInputPin(VMODE2_PORT, VMODE2_PIN) == RESET ){
			config.ucSysVoltage = 60;
		} else {
			config.ucSysVoltage = 48;
		}
	#else
		GPIO_Init(VMODE1_PORT, VMODE1_PIN, GPIO_MODE_IN_PU_NO_IT);
		GPIO_Init(VMODE2_PORT, VMODE2_PIN, GPIO_MODE_IN_PU_NO_IT);
		if ( GPIO_ReadInputPin(VMODE1_PORT, VMODE1_PIN) == RESET ){
			config.ucSysVoltage = 72;
		} else {
			if ( GPIO_ReadInputPin(VMODE2_PORT, VMODE2_PIN) == RESET ){
				config.ucSysVoltage = 48;
			} else {
				config.ucSysVoltage = 60;
			}
		}
	#endif
#endif

	switch ( config.ucSysVoltage ){
	case 48:ucBatStatus = BatStatus48;break;
	case 60:ucBatStatus = BatStatus60;break;
	case 72:ucBatStatus = BatStatus72;break;
	default:ucBatStatus = BatStatus60;break;
	}
}

uint8_t GetBatStatus(uint16_t vol)
{
	uint8_t i;

	for(i=0;i<ContainOf(BatStatus60);i++)
		if ( vol < ucBatStatus[i] ) break;
	return i;
}

#ifdef LCD8794GCT

const uint16_t BatEnergy48[8] = {420,490};
const uint16_t BatEnergy60[8] = {520,620};
const uint16_t BatEnergy72[8] = {630,740};
const uint16_t* BatEnergy;

uint8_t GetBatEnergy(uint16_t vol)
{
	uint16_t energy ;
	
	switch ( config.ucSysVoltage ){
	case 48:BatEnergy = BatEnergy48;break;
	case 60:BatEnergy = BatEnergy60;break;
	case 72:BatEnergy = BatEnergy72;break;
	default:BatEnergy = BatEnergy60;break;
	}

	if ( bike.ucVoltage <= BatEnergy[0] ) energy = 0;
	else if ( bike.ucVoltage >= BatEnergy[1] ) energy = 100;
	else {
		energy = (bike.ucVoltage - BatEnergy[0])*100/(BatEnergy[1] - BatEnergy[0]);
	}
	return energy;
}
#endif

uint8_t MileResetTask(void)
{
	static uint16_t pre_tick=0;
	static uint8_t TaskFlag = TASK_INIT;
	static uint8_t count = 0;
	
    if ( TaskFlag == TASK_EXIT )
        return 0;
    
	if ( Get_ElapseTick(pre_tick) > 10000 | bike.bBraked | bike.ucSpeed )
		TaskFlag = TASK_EXIT;

	switch( TaskFlag ){
	case TASK_INIT:
		if ( Get_SysTick() < 3000 && bike.bTurnRight == 1 ){
			TaskFlag 	= TASK_STEP1;
			count 		= 0;
		}
		break;
	case TASK_STEP1:
		if ( bike.bLastNear == 0 && bike.bNearLight == 1 ){
			pre_tick = Get_SysTick();
			if ( ++count >= 8 ){
				TaskFlag 		= TASK_STEP2;
				bike.bMileFlash = 1;
				bike.ulMile 	= config.ulMile;
			}
			}
		bike.bLastNear = bike.bNearLight;
		break;
	case TASK_STEP2:
		if ( bike.bTurnRight == 0 && bike.bTurnLeft == 1 ) {
			TaskFlag 		= TASK_EXIT;
			bike.ulFMile 	= 0;
			bike.ulMile 	= 0;
			config.ulMile 	= 0;
			WriteConfig();
		}
		break;
	case TASK_EXIT:
	default:
		bike.bMileFlash = 0;
		break;
	}
	return 0;
}

void MileTask(void)
{
	static uint16_t time = 0;
	uint8_t speed;
	
	speed = bike.ucSpeed;
	if ( speed > DISPLAY_MAX_SPEED )
		speed = DISPLAY_MAX_SPEED;
	
#ifdef SINGLE_TRIP
	time ++;
	if ( time < 20 ) {	//2s
		bike.ulMile = config.ulMile;
	} else if ( time < 50 ) { 	//5s
		if ( speed ) {
			time = 50;
			bike.ulMile = 0;
		}
	} else if ( time == 50 ){
		bike.ulMile = 0;
	} else 
#endif	
	{
		time = 51;
		
		bike.ulFMile = bike.ulFMile + speed;
		if(bike.ulFMile >= 36000)
		{
			bike.ulFMile = 0;
			bike.ulMile++;
			if ( bike.ulMile > 99999 	) bike.ulMile = 0;
			config.ulMile ++;
			if ( config.ulMile > 99999 	) config.ulMile = 0;
			WriteConfig();
		}  
	}
}

#if ( TIME_ENABLE == 1 )
void TimeTask(void)
{
	static uint8_t time_task=0,left_on= 0,NL_on = 0;
	static uint16_t pre_tick;
	
	if (!bike.bHasTimer)
		return ;
	
	if (bike.ucSpeed > 1)
		time_task = 0xff;
	
	switch ( time_task ){
	case 0:
		if ( Get_SysTick() < 5000 && bike.bNearLight == 0 && bike.bTurnLeft == 1 ){
			pre_tick = Get_SysTick();
			time_task++;
		}
		break;
	case 1:
		if ( bike.bTurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.bNearLight ) time_task = 0xFF;
		}
		break;
	case 2:
		if ( bike.bTurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.bNearLight ) time_task = 0xFF;
		}
		break;
	case 3:
		if ( bike.bTurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.bNearLight ) time_task = 0xFF;
		}
		break;
	case 4:
		if ( bike.bTurnLeft == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.bNearLight ) time_task = 0xFF;
		}
		break;
	case 5:
		if ( bike.bTurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.bNearLight ) time_task = 0xFF;
		}
		break;
	case 6:
		if ( bike.bTurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.bNearLight ) time_task = 0xFF;
		}
	case 7:
		if ( bike.bTurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.bNearLight ) time_task = 0xFF;
		}
		break;
	case 8:
		if ( bike.bTurnLeft == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.bNearLight ) time_task = 0xFF;
		}
	case 9:
		if ( bike.bTurnLeft == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.bNearLight ) time_task = 0xFF;
		}
		break;
	case 10:
		if ( bike.bTurnRight == 1 ){
			if ( Get_ElapseTick(pre_tick) > 1000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000  || bike.bNearLight ) time_task = 0xFF;
		}
	case 11:
		if ( bike.bTurnRight == 0 ){
			if ( Get_ElapseTick(pre_tick) < 2000  ) time_task = 0xFF;	else { pre_tick = Get_SysTick(); time_task++; }
		} else {
			if ( Get_ElapseTick(pre_tick) > 10000 || bike.bNearLight ) time_task = 0xFF;
		}
		break;
	case 12:
		if ( bike.bTurnLeft == 1 || bike.bNearLight ){
			 time_task = 0xFF;
		} else {
			if ( Get_ElapseTick(pre_tick) > 1000 ) {
				time_task= 0;
				bike.ucTimePos = 0;
				bike.bTimeSet = 1; 
				pre_tick = Get_SysTick();
			}
		}
		break;
	default:
		bike.ucTimePos = 0;
		bike.bTimeSet = 0; 
		time_task = 0;
		break;
	}

	if ( bike.bTimeSet ){
		if ( bike.bTurnLeft ) { left_on = 1; }
		if ( bike.bTurnLeft == 0 ) {
			if ( left_on == 1 ){
				bike.ucTimePos ++;
				bike.ucTimePos %= 4;
				left_on = 0;
				pre_tick = Get_SysTick();
			}
		}
		if ( bike.bNearLight ) { NL_on = 1; pre_tick = Get_SysTick(); }
		if ( bike.bNearLight == 0 && NL_on == 1 ) {
			NL_on = 0;
			if ( Get_ElapseTick(pre_tick) < 5000 ){
				switch ( bike.ucTimePos ){
				case 0:
					bike.ucHour += 10;
					bike.ucHour %= 20;
					break;
				case 1:
					if ( bike.ucHour % 10 < 9 )
						bike.ucHour ++;
					else 
						bike.ucHour -= 9;
					break;
				case 2:
					bike.ucMinute += 10;
					bike.ucMinute %= 60;
					break;
				case 3:
					if ( bike.ucMinute % 10 < 9 )
						bike.ucMinute ++;
					else 
						bike.ucMinute -= 9;
					break;
				default:
					bike.bTimeSet = 0;
					break;
				}
			}
			RtcTime.RTC_Hours 	= bike.ucHour;
			RtcTime.RTC_Minutes = bike.ucMinute;
			PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
		}
		if ( Get_ElapseTick(pre_tick) > 30000 ){
			bike.bTimeSet = 0;
		}
	}		
	
	 PCF8563_GetTime(PCF_Format_BIN,&RtcTime);
	 bike.ucHour 		= RtcTime.RTC_Hours%12;
	 bike.ucMinute 	= RtcTime.RTC_Minutes;
}


uint8_t SpeedCaltTask(void)
{
	static uint16_t pre_tick=0;
	static uint8_t TaskFlag = TASK_INIT;
	static uint8_t lastSpeed = 0;
	static uint8_t count = 0;
    static signed char SpeedInc=0;
	
    if ( TaskFlag == TASK_EXIT )
      	return 0;
    
	if ( Get_ElapseTick(pre_tick) > 10000 || bike.bBraked )
		TaskFlag = TASK_EXIT;

	switch( TaskFlag ){
	case TASK_INIT:
		if ( Get_SysTick() < 3000 && bike.bTurnLeft == 1 ){
			TaskFlag = TASK_STEP1;
			count = 0;
		}
		break;
	case TASK_STEP1:
		if ( bike.bLastNear == 0 && bike.bNearLight == 1 ){
			if ( ++count >= 8 ){
				TaskFlag 	= TASK_STEP2;
				count 		= 0;
				SpeedInc 	= 0;
				bike.bSpeedFlash = 1;
			}
			pre_tick = Get_SysTick();
		}
		bike.bLastNear = bike.bNearLight;
		break;
	case TASK_STEP2:
		if ( bike.bLastLeft == 0 && bike.bTurnLeft == 1 ) {
			pre_tick 	= Get_SysTick();
			count 		= 0;
			if ( bike.ucSpeed + SpeedInc )
				SpeedInc --;
		}
        bike.bLastLeft = bike.bTurnLeft;

        if ( bike.bLastRight == 0 && bike.bTurnRight == 1 ) {
			pre_tick 	= Get_SysTick();
			count 		= 0;
			SpeedInc ++;
        }
        bike.bLastRight = bike.bTurnRight;
        
		if ( bike.bLastNear == 0 && bike.bNearLight == 1 ){
			pre_tick = Get_SysTick();
			if ( ++count >= 5 ){
				TaskFlag = TASK_EXIT;
				if ( bike.ucSpeed ) {
					if ( bike.bYXTERR )
						config.uiSpeedScale 	= (uint32_t)bike.ucSpeed*1000UL/(bike.ucSpeed+SpeedInc);
					else
						config.uiYXT_SpeedScale = (uint32_t)bike.ucSpeed*1000UL/(bike.ucSpeed+SpeedInc);
					WriteConfig();
				}
			}
		}
		bike.bLastNear = bike.bNearLight;
    
		if ( lastSpeed && bike.ucSpeed == 0 ){
			TaskFlag = TASK_EXIT;
		}
        
		if ( bike.ucSpeed )
			pre_tick  = Get_SysTick();

        bike.ucSpeed += SpeedInc;
		lastSpeed 	  = bike.ucSpeed;
		break;
	case TASK_EXIT:
	default:
		bike.bSpeedFlash = 0;
		break;
	}
	return 0;
}

