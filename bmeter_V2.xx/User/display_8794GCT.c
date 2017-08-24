#include "stdlib.h"
#include "display.h"
#include "bl55072.h"
#include "stm8s.h"

unsigned char BL_Data[19];

const unsigned char SegDataTime[10] 	= {0xFA,0x60,0xD6,0xF4,0x6C,0xBC,0xBE,0xE0,0xFE,0xFC};
const unsigned char SegDataVoltage[10]	= {0x5F,0x50,0x3D,0x79,0x72,0x6B,0x6F,0x51,0x7F,0x7B};
const unsigned char SegDataMile[10] 	= {0xAF,0xA0,0x6D,0xE9,0xE2,0xCB,0xCF,0xA1,0xEF,0xEB};
const unsigned char SegDataMile2[10] 	= {0x5F,0x50,0x3D,0x79,0x72,0x6B,0x6F,0x51,0x7F,0x7B};
const unsigned char SegDataSpeed[10] 	= {0x5F,0x50,0x3D,0x79,0x72,0x6B,0x6F,0x51,0x7F,0x7B};
const unsigned char SegDataTemp[10] 	= {0xFA,0x60,0xD6,0xF4,0x6C,0xBC,0xBE,0xE0,0xFE,0xFC};
const unsigned char SegDataEnergy[10] 	= {0x5F,0x50,0x3D,0x79,0x72,0x6B,0x6F,0x51,0x7F,0x7B};

void MenuUpdate(BIKE_STATUS* bike)
{
	unsigned char i = 0;

	bike->FlashCount ++;
	bike->FlashCount %= 10;

	for(i=0;i<18;i++){
		BL_Data[i] = 0x00;
	}

	if( bike->TurnLeft  && bike->FlashCount < 5 ) BL_Data[0x08] |= 0x01;	//S1
	#ifdef TurnLeftOut_PIN
	GPIO_Init(TurnLeftOut_PORT, TurnLeftOut_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);
	if( bike->TurnLeft ) GPIO_WriteLow (TurnLeftOut_PORT,TurnLeftOut_PIN); else GPIO_WriteHigh (TurnLeftOut_PORT,TurnLeftOut_PIN);
	#endif
	if( bike->TurnRight && bike->FlashCount < 5 ) BL_Data[0x0F] |= 0x04;	//S12
	#ifdef TurnRightOut_PIN
	GPIO_Init(TurnRightOut_PORT, TurnRightOut_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);
	if( bike->TurnRight ) GPIO_WriteLow (TurnRightOut_PORT,TurnRightOut_PIN); else GPIO_WriteHigh (TurnRightOut_PORT,TurnRightOut_PIN);
	#endif
	if( bike->CRZLight 	) BL_Data[0x0B] |= 0x20;	//S4
	if( bike->NearLight	) BL_Data[0x0B] |= 0x10;	//S5
	#ifdef NearLightOut_PIN
	#if ( ( TurnLeftOut_PIN == GPIO_PIN_1 ) )
	CFG->GCR = CFG_GCR_SWD;
	#endif
	GPIO_Init(NearLightOut_PORT, NearLightOut_PIN, GPIO_MODE_OUT_OD_HIZ_SLOW);
	if( bike->NearLight ) GPIO_WriteLow (NearLightOut_PORT,NearLightOut_PIN); else GPIO_WriteHigh (NearLightOut_PORT,NearLightOut_PIN);
	#endif
	if( bike->HallERR 	) BL_Data[0x0B] |= 0x80;	//S2	电机霍尔故障
	if( bike->WheelERR 	) BL_Data[0x0B] |= 0x40;	//S3	手把故障
	if( bike->ECUERR 	) BL_Data[0x0F] |= 0x01;	//S7 	电机控制器故障
//	if( bike->PhaseERR  ) BL_Data[0x0F] |= 0x02;	//S11	电机缺相故障
	if( bike->Braked  	) BL_Data[0x0F] |= 0x02;	//S11	电机缺相故障
//	if( bike->YXTERR	) BL_Data[0x0E] |= 0x80;	//S6	ECO
//	if( bike->YXTERR	) BL_Data[0x0F] |= 0x80;	//S7	R

	/***************************Battery Area Display**********************************/
	BL_Data[0x06] |=  0x80; //T
	switch ( bike->BatStatus ){
	case 0:
		if ( bike->FlashCount < 5 ) 
			BL_Data[0x06] &= ~0x80; 
		break;
	case 1: BL_Data[0x07] = 0x80;break;
	case 2: BL_Data[0x07] = 0xC0;break;
	case 3: BL_Data[0x07] = 0xE0;break;
	case 4: BL_Data[0x07] = 0xF0;break;
	case 5: BL_Data[0x07] = 0xF1;break;
	case 6: BL_Data[0x07] = 0xF3;break;      
	case 7: BL_Data[0x07] = 0xF7;break;
	case 8: BL_Data[0x07] = 0xFF;break;          
	default:break; 
	}

	/***************************Temp Area Display**********************************/
	BL_Data[0x10] |= (SegDataTemp[abs(bike->Temperature/10)%10]);	//D2
	BL_Data[0x11] |= (SegDataTemp[abs(bike->Temperature/10)/10]);	//D1
	BL_Data[0x10] |= 0x01;		//S9
	if (bike->Temperature < 0)
	BL_Data[0x11] |= 0x10;	//S8       

	/***************************Time Area Display**********************************/
	if ( bike->HasTimer ){
		if(bike->Hour > 9) BL_Data[0x0A] |= 0x01;	//S13  
		BL_Data[0x08] |= ( SegDataTime[bike->Hour%10]);		//D5
		BL_Data[0x09] |= ( SegDataTime[bike->Minute/10] );	//D4
		BL_Data[0x0A] |= ( SegDataTime[bike->Minute%10] );	//D3    
		if ( bike->time_set ){
			switch ( bike->time_pos ){
			case 0:
			if ( bike->FlashCount < 5  ) { 
				BL_Data[0x08] &= 0x01; 
				BL_Data[0x09] &= 0x01; 
				BL_Data[0x0A] &= 0x01; 
			}
			break;			
			case 1:if ( bike->FlashCount < 5  ) BL_Data[0x08] &= 0x01; break;
			case 2:if ( bike->FlashCount < 5  ) BL_Data[0x09] &= 0x01; break;
			case 3:if ( bike->FlashCount < 5  ) BL_Data[0x0A] &= 0x01; break;
			default:break;		
		}
		BL_Data[0x09] |= 0x01;	//col
	} else if ( bike->FlashCount < 5 ) BL_Data[9] |= 0x01;	//col
	}

	/*************************** Voltage Display**********************************/
	BL_Data[0x05] |= (SegDataVoltage[ bike->Voltage/10  %10]) | 0x80; //V
	BL_Data[0x06] |= (SegDataVoltage[(bike->Voltage/100)%10]);

	/*************************** Voltage Energy**********************************/
	BL_Data[0x0D] |= 0x80; //S15
	BL_Data[0x0C] |= (SegDataEnergy[ bike->Energy%10]		 )&0x0F;
	BL_Data[0x0D] |= (SegDataEnergy[ bike->Energy%10]		 )&0xF0;
	BL_Data[0x0B] |= (SegDataEnergy[(bike->Energy/10)%10])&0x0F; 
	BL_Data[0x0C] |= (SegDataEnergy[(bike->Energy/10)%10])&0xF0; 
	if ( bike->Energy == 100 ) BL_Data[0x0C] |= 0x80; //S14

	/*************************** Mile Display**********************************/  
	BL_Data[0x00] |= (SegDataMile2[ bike->Mile			 %10]) | 0x80;	//S17
	BL_Data[0x01] |= (SegDataMile [(bike->Mile/10   )%10]);
	BL_Data[0x02] |= (SegDataMile [(bike->Mile/100  )%10]); 
	BL_Data[0x03] |= (SegDataMile [(bike->Mile/1000 )%10]);
	BL_Data[0x04] |= (SegDataMile [(bike->Mile/10000)%10]); 

	/*************************** Speed Display**********************************/
	BL_Data[0x0F] |= 0x08;	//S16
	BL_Data[0x0E] |= (SegDataSpeed[ bike->Speed%10]		 )&0x0F;
	BL_Data[0x0F] |= (SegDataSpeed[ bike->Speed%10]		 )&0xF0;
	BL_Data[0x0D] |= (SegDataSpeed[(bike->Speed/10)%10])&0x0F; 
	BL_Data[0x0E] |= (SegDataSpeed[(bike->Speed/10)%10])&0xF0; 

	/*************************** Mode Display**********************************/ 
	switch (bike->SpeedMode){
	case 1: BL_Data[0x01] |= 0x10;break;	//1
	case 2: BL_Data[0x02] |= 0x10;break;	//2
	case 3: BL_Data[0x03] |= 0x10;break;	//3
	case 4: BL_Data[0x04] |= 0x10;break;	//4
	default:
			BL_Data[0x01] &= ~0x10;
			BL_Data[0x02] &= ~0x10;
			BL_Data[0x03] &= ~0x10;
			BL_Data[0x04] &= ~0x10;
	break;
	}

	BL_Write_Data(0,18,BL_Data);
}


void Delay(unsigned long nCount)
{
	for(; nCount != 0; nCount--);
}

