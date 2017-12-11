#include "stdlib.h"
#include "bike.h"
#include "bl55072.h"
#include "TM16XX.H"

unsigned char flashflag = 0;

const unsigned char SegData[10] = {0x3F,0x30,0x6D,0x79,0x72,0x5B,0x5F,0x31,0x7F,0x7B,};

void MenuUpdate(BIKE_STATUS* bike)
{
	unsigned char i = 0;
  
	flashflag++;
	flashflag %= 10;
	
	for(i=0;i<18;i++)	TM16XX[i] = 0x00;
	
    if ( bike->bLFlashType ){
		if ( bike->bLeftFlash 	)	TM16XX[ 2] |= 0x80;	//S1
    } else {
     	if ( bike->bLeftFlash 	&& flashflag >= 5 )	TM16XX[2] |= 0x80;	//S1
    }
    if ( bike->bRFlashType ){
		if ( bike->bRightFlash	)	TM16XX[ 3]|= 0x80;	//S3
    } else {
     	if ( bike->bRightFlash && flashflag >= 5 )	TM16XX[ 3]|= 0x80;	//S3
    }
	
	if( bike->bNearLight 	) TM16XX[ 1] |= 0x80;	//S2
	if( bike->bHallERR 		) TM16XX[13] |= 0x10;	//S4	电机故障
	if( bike->bWheelERR 	) TM16XX[13] |= 0x20;	//S5	转把故障
	if( bike->bECUERR 		) TM16XX[13] |= 0x40;	//S6 	电机控制器故障
	if( bike->bBraked  		) TM16XX[13] |= 0x80;	//S7 	刹车

  /***************************Battery Area Display**********************************/
	TM16XX[12] |= 0x7F;	//T5-T11
	switch ( bike->ucBatStatus ){
    case 0:
		if ( flashflag < 5 ) 
			TM16XX[12]&=~0x7F;break;	//T5-T11
    case 1: TM16XX[11] = 0x01;break;
    case 2: TM16XX[11] = 0x03;break;
    case 3: TM16XX[11] = 0x07;break;
    case 4: TM16XX[11] = 0x0F;break;
    case 5: TM16XX[11] = 0x1F;break;
    case 6: TM16XX[11] = 0x3F;break;      
    case 7: TM16XX[11] = 0x7F;break;
    case 8: TM16XX[11] = 0xFF;break;          
    default:break; 
	}

	/***************************Temp Area Display**********************************/
	TM16XX[ 0] |= (SegData[abs(bike->siTemperature/10	)%10]);
	TM16XX[ 1] |= (SegData[abs(bike->siTemperature/100	)%10]);       
	TM16XX[ 0] |= 0x80;	//T1
	//if (bike->siTemperature < 0)
	//	TM16XX[4] |= 0x01;       
		
	/*************************** Voltage Display**********************************/
	//TM16XX[2] |= (SegData[ bike->uiVoltage/10 %10]) | 0x8;
	//TM16XX[1] |= (SegData[(bike->uiVoltage/100)%10]);

	/*************************** Mile Display**********************************/  
	TM16XX[10] |= (SegData [ bike->ulMile	   	 %10]);
	TM16XX[ 9] |= (SegData [(bike->ulMile/10   )%10]);
	TM16XX[ 8] |= (SegData [(bike->ulMile/100  )%10]); 
	TM16XX[ 7] |= (SegData [(bike->ulMile/1000 )%10]); 
	TM16XX[ 6] |= (SegData [(bike->ulMile/10000)%10]); 
	TM16XX[ 6] |= 0x80;	//T4
	if ( (bike->bMileFlash && flashflag < 5) ) {
		TM16XX[ 8] &= ~0x80;
		TM16XX[ 7] &= ~0x80;
		TM16XX[ 6] &= ~0x80; 
		TM16XX[ 1] &= ~0x80; 
		TM16XX[ 0] &= ~0x80;
	}		

	/*************************** Speed Display**********************************/
	TM16XX[ 2] |= (SegData [ bike->ucSpeed	 	%10]);
	TM16XX[ 3]  = TM16XX[ 2];
	TM16XX[ 4] |= (SegData [(bike->ucSpeed/10)	%10]); 
	TM16XX[ 4] |= 0x80;
	TM16XX[ 5]  = TM16XX[ 4];	//T2,T3
	if ( bike->bSpeedFlash ){
		if ( flashflag < 5  ) {
			TM16XX[ 2] &= ~0x80;
			TM16XX[ 3] &= ~0x80;
			TM16XX[ 4] &= ~0x80;
			TM16XX[ 5] &= ~0x80;
		}
	}

	/*************************** Mode Display**********************************/ 
	switch (bike->ucSpeedMode){
    case 1: TM16XX[13] |= 0x01;break;	//S8	P
    case 2: TM16XX[13] |= 0x02;break;	//S9	R
    case 3: TM16XX[13] |= 0x04;break;	//S10	N
	case 4: TM16XX[13] |= 0x08;break;	//S11	D
    default:
			TM16XX[13] &= ~0x0F;
			break;
	}
  
	TM16XX_Write_Data(TM16XX,16);
}
