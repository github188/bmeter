/**
  ******************************************************************************
  * @file    
  * @author  ahzzc@sina.com
  * @version V2.xx
  * @date    
  * @brief   
  ******************************************************************************
  * @Changlog
  * V2.04
  * V2.02
  * 统一各版本采用release进行定义
  * 增加金彭产品支持，JINPENG_4060、JINPENG_6072
  * 删除对老PCB版本的支持，
  *
  * V2.01
  * 一线通改为多协议同时支持；
  * 速度分档修正；
  *
  * V2.00
	* 功能：
	* 1.系统电压通过MODE1,MODE2自动检测；
	* 2.时钟功能自动检测，无芯片不显示；
	* 3.通过左右转向进行时间调整模式；
	* 4.通过串口可进行参数标定,	短接低速、SWIM信号进行参数标定；
	* 5.参数保存于EEPROM;
	* 6.一线通功能；
	* 7.档位信息优先读取一线通数据，实现四档信息判断，一线通中断后采用档把数据；
	* 8.开启开门狗功能；
	* 9.通过PCB_VER定义不同的硬件版本，支持0011、0012、0022、0041；
	*10.通过YXT_XX定义不同的控制器版本；
  *
  ******************************************************************************
  */
/******************************************************************************/

#ifndef __BIKE_H__
#define __BIKE_H__

/******************************************************************************/
//release define
//#define JINPENG_4860
//#define JINPENG_6072
//#define LCD6040
//#define LCD9040
//#define LCD9040T
//#define LCD8794GCT
//#define DENGGUAN_XUNYING		//DG55,BLSP55
//#define DENGGUAN_XUNYING_T	//TDG55
//#define BENLING_OUSHANG		//BL60_72
//#define BENLING_BL48_60		//BL48_60
//#define BENLING_ZHONGSHA		//BLV72
#define OUJUN					//M2_60_72

#ifdef JINPENG_4860
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1				
#elif defined JINPENG_6072
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1				
#elif defined LCD9040
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1				
#elif defined LCD9040T
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 1
	#define YXT_ENABLE  1				
#elif defined LCD6040
	#define PCB_VER		0100
	#define LCD6040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1				
#elif defined DENGGUAN_XUNYING
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1
#elif defined DENGGUAN_XUNYING_T
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 1
	#define YXT_ENABLE  0
#elif defined BENLING_OUSHANG
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1
#elif defined BENLING_BL48_60
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1
#elif defined BENLING_ZHONGSHA
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1
#elif defined OUJUN
	#define PCB_VER		0100
	#define LCD9040
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1
#elif defined LCD8794GCT
	#define PCB_VER		0041
	#define LCD8794GCT
	#define TIME_ENABLE 0
	#define YXT_ENABLE  1
#else
	#error "Please select release type!!!"
#endif

/******************************************************************************/
#if (defined LCD9040) || (defined LCD5535) || (defined LCD8794GCT) || (defined LCD6040 )
#else
#error Not defined LCD_TYPE
#endif
    
#ifndef PCB_VER
	#define PCB_VER	0100
//	#define PCB_VER	0041
#endif

#ifndef TIME_ENABLE
	#define TIME_ENABLE 	0				
#endif

#ifndef YXT_ENABLE
	#define YXT_ENABLE      1				
#endif

/******************************************************************************/
#define VOL_CALIBRATIOIN	600UL	//60.0V
#define TEMP_CALIBRATIOIN	250UL	//25.0C
#define SPEED_CALIBRATIOIN	30UL	//30km/h

#define PON_ALLON_TIME		1000UL	//1000ms

#define DISPLAY_MAX_SPEED	40UL	//40km/h
#define SPEEDMODE_DEFAULT	1		//1档

/******************************************************************************/
typedef struct {
	unsigned char NearLight	:1;
	unsigned char CRZLight	:1;
	unsigned char Cruise	:1;
	unsigned char ECUERR	:1;
	unsigned char Braked	:1;
	unsigned char PhaseERR	:1;
	unsigned char HallERR	:1;
	unsigned char WheelERR	:1;
	unsigned char YXTERR	:1;
	unsigned char HasTimer	:1;
	unsigned char time_set	:1;
	unsigned char uart		:1;	
	unsigned char HotReset	:1;	
	
	unsigned char TurnLeft;
	unsigned char TurnRight;
	unsigned char SpeedMode;
			 int  Temperature;
	unsigned int  Voltage;
	unsigned char BatStatus;
	unsigned char Energy;
	unsigned char Speed;
	unsigned long Mile;
	
	unsigned char Hour;
	unsigned char Minute;
	unsigned char Second;
	unsigned char time_pos;
	
} BIKE_STATUS,*pBIKE_STATUS;
	
typedef struct {
	unsigned char bike[4];
	unsigned int  SysVoltage	;
	unsigned int  VolScale	;
	unsigned int  TempScale	;
	unsigned int  SpeedScale;
	unsigned long Mile;
	unsigned char Sum;
} BIKE_CONFIG,*pBIKE_CONFIG;
	
extern BIKE_STATUS bike;
extern BIKE_CONFIG config;
extern volatile unsigned int sys_tick;

unsigned int Get_SysTick(void);
unsigned int Get_ElapseTick(unsigned int pre_tick);

/******************************************************************************/

#if ( PCB_VER == 0100 )
	#define SPEEDV_ADC_CH	ADC1_CHANNEL_3
	#define SPEEDV_ADC_SCH	ADC1_SCHMITTTRIG_CHANNEL3

	#define VMODE1_PORT		GPIOA
	#define VMODE1_PIN		GPIO_PIN_2
	#define VMODE2_PORT		GPIOD
	#define VMODE2_PIN		GPIO_PIN_3

	#define SPMODE1_PORT	GPIOD
	#define SPMODE1_PIN		GPIO_PIN_4
	#define SPMODE2_PORT	GPIOD
	#define SPMODE2_PIN		GPIO_PIN_5
	#define SPMODE3_PORT	GPIOA
	#define SPMODE3_PIN		GPIO_PIN_1
	#define SPMODE4_PORT	GPIOA
	#define SPMODE4_PIN		GPIO_PIN_3
	
	#define NearLight_PORT	GPIOC
	#define NearLight_PIN	GPIO_PIN_7
	#define TurnRight_PORT	GPIOC
	#define TurnRight_PIN	GPIO_PIN_3
	#define TurnLeft_PORT	GPIOC
	#define TurnLeft_PIN	GPIO_PIN_5
//	#define CRZLight_PORT	GPIOA
//	#define CRZLight_PIN	GPIO_PIN_3
#elif ( PCB_VER == 0041 )
	#define SPEEDV_ADC_CH	ADC1_CHANNEL_5
	#define SPEEDV_ADC_SCH	ADC1_SCHMITTTRIG_CHANNEL5

	#define SPMODE1_PORT	GPIOD
	#define SPMODE1_PIN		GPIO_PIN_4
	#define SPMODE2_PORT	GPIOA
	#define SPMODE2_PIN		GPIO_PIN_1
	#define SPMODE3_PORT	GPIOA
	#define SPMODE3_PIN		GPIO_PIN_2
	#define SPMODE4_PORT	GPIOA
	#define SPMODE4_PIN		GPIO_PIN_3
	
	#define NearLight_PORT	GPIOD
	#define NearLight_PIN	GPIO_PIN_3
	#define TurnRight_PORT	GPIOC
	#define TurnRight_PIN	GPIO_PIN_3
	#define TurnLeft_PORT	GPIOD
	#define TurnLeft_PIN	GPIO_PIN_2
//	#define CRZLight_PORT	GPIOA
//	#define CRZLight_PIN	GPIO_PIN_3
	
	#define NearLightOut_PORT	GPIOC
	#define NearLightOut_PIN	GPIO_PIN_7
	#define TurnRightOut_PORT	GPIOC
	#define TurnRightOut_PIN	GPIO_PIN_5
	#define TurnLeftOut_PORT	GPIOD
	#define TurnLeftOut_PIN		GPIO_PIN_1	
#endif

/******************************************************************************/

#endif
