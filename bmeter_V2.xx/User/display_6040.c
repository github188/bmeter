#include "stdlib.h"
#include "bike.h"
#include "bl55072.h"

unsigned char flashflag = 0;
unsigned char BL_Data[19];

const unsigned char SegDataMile[10] 	= {0xF5,0x60,0xD3,0xF2,0x66,0xB6,0xB7,0xE0,0xF7,0xF6};
const unsigned char SegDataSpeed[10]  = {0x5F,0x06,0x6B,0x2F,0x36,0x3D,0x7D,0x07,0x7F,0x3F};

void MenuUpdate(BIKE_STATUS* bike)
{
	unsigned char i = 0;

	flashflag ++;
	flashflag %= 10;

	for(i=0;i<18;i++)
		BL_Data[i] = 0x00;
   
	if( bike->TurnLeft  && (flashflag%10) < 5 ) BL_Data[ 7] |= 0x20;	//S1
	if( bike->TurnRight && (flashflag%10) < 5 ) BL_Data[ 3] |= 0x01;	//S7
	//if( bike->CRZLight) BL_Data[ 5] |= 0x02;	//S?
	if( bike->NearLight ) BL_Data[ 7] |= 0x10;	//S2
	if( bike->HallERR 	) BL_Data[ 7] |= 0x40;	//S3	电机霍尔故障
	if( bike->WheelERR 	) BL_Data[ 7] |= 0x80;	//S5	手把故障
	if( bike->ECUERR 	) BL_Data[ 1] |= 0x80;	//S6 	电机控制器故障
//	if( bike->PhaseERR  ) BL_Data[ 0] |= 0x80;	//S4 	电机缺相故障
	if( bike->Braked  	) BL_Data[ 0] |= 0x80;	//S4 	电机缺相故障

  /***************************Battery Area Display**********************************/
	BL_Data[ 4] |= 0x80;  //S19
	BL_Data[ 4] |= 0x01;  //S14
	switch ( bike->BatStatus ){
    case 0:
		if ( flashflag > 5 ) BL_Data[ 4] &= ~0x01;   //S14
    break;
    case 1: BL_Data[ 4] |= 0x01;break; //S14
    case 2: BL_Data[ 4] |= 0x03;break; //S15
    case 3: BL_Data[ 4] |= 0x07;break; //S16
    case 4: BL_Data[ 4] |= 0x0F;break; //S17
    case 5: BL_Data[ 4] |= 0x4F;break; //S18
    default:break; 
	}
	
	/*************************** Voltage Display**********************************/
	//BL_Data[2] |= (SegDataVoltage[ bike->Voltage		 %10]) | 0x8;
	//BL_Data[1] |= (SegDataVoltage[(bike->Voltage/10	)%10]) | 0x8;
	//BL_Data[0] |= (SegDataVoltage[(bike->Voltage/100)%10]); 

	/*************************** Mile Display**********************************/  
	BL_Data[ 5] |= (SegDataMile [ bike->Mile			 %10]);
	BL_Data[ 6] |= (SegDataMile [(bike->Mile/10   )%10]);
	if ( bike->Mile >= 100 )  BL_Data[ 6] |= 0x08;  //S20
	BL_Data[ 4] |= 0x10;  //S13

	/*************************** Speed Display**********************************/
	BL_Data[ 1] |= (SegDataSpeed[ bike->Speed		 %10]);
	BL_Data[ 0] |= (SegDataSpeed[(bike->Speed/10)%10]); 

	/*************************** Mode Display**********************************/ 
	switch (bike->SpeedMode){
	case 1: BL_Data[ 3] |= 0x04;break;  //S8
	case 2: BL_Data[ 3] |= 0x02;break;  //S9
	case 3: BL_Data[ 3] |= 0x08;break;  //S10
	case 4: BL_Data[ 5] |= 0x08;break;  //S11
	default:
	BL_Data[ 3] &= ~0x0E;
	BL_Data[ 5] &= ~0x08;
	break;
	}

	BL_Write_Data(0,18,BL_Data);
}


void Delay(unsigned long nCount)
{
	for(; nCount != 0; nCount--);
}

