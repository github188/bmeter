#include "stdlib.h"
#include "display.h"
#include "bl55072.h"

unsigned char flashflag = 0;
unsigned char BL_Data[19];
unsigned char BL_Reg[19];

const unsigned char SegDataTime[10] 	= {0xFA,0x06,0xD6,0xF4,0x6C,0xBC,0xDF,0xE0,0xFE,0xFC};
const unsigned char SegDataVoltage[10]= {0x5F,0x11,0x3D,0x79,0x72,0x6D,0x6F,0x51,0x7F,0x7D};
const unsigned char SegDataMile[10] 	= {0xFA,0x0A,0xD6,0x9E,0x2E,0xBC,0xFC,0x1A,0xFE,0x3E};
const unsigned char SegDataSpeed[10] 	= {0xAF,0xA0,0x6D,0xE9,0xE2,0xCB,0xCF,0xA1,0xEF,0xE3};
const unsigned char SegDataTemp[10] 	= {0xAF,0x60,0x6D,0x4F,0xC6,0xCB,0xFD,0x0E,0xEF,0xCF};

void MenuUpdate(BIKE_STATUS* bike)
{
  unsigned char i = 0;
  
	flashflag ++;
	flashflag %= 10;
	
  for(i=0;i<18;i++){
    BL_Data[i] = 0x00;
  }
   
  if( bike->TurnLeft && flashflag <= 5 ) BL_Data[0x08] |= 0x01;	//S1
  if( bike->TurnRight&& flashflag <= 5 ) BL_Data[0x0F] |= 0x04;	//S12
  if( bike->CRZLight	) BL_Data[0x0B] |= 0x20;	//S4
  if( bike->NearLight ) BL_Data[0x0B] |= 0x10;	//S5
  if( bike->MotorERR 	) BL_Data[0x0B] |= 0x80;	//S2	电机故障
  if( bike->WheelERR 	) BL_Data[0x0B] |= 0x40;	//S3	手把故障
  if( bike->ECUERR 		) BL_Data[0x0F] |= 0x01;	//S10	电机控制器故障
  if( bike->YXTERR	  ) BL_Data[0x0F] |= 0x02;	//S11	一线通故障，通用故障标识
  //if( bike->YXTERR	  ) BL_Data[0x0E] |= 0x80;	//S6	ECO
  //if( bike->YXTERR	  ) BL_Data[0x0F] |= 0x80;	//S7	R

  /***************************Battery Area Display**********************************/
	BL_Data[0x06] |=  0x80;
  switch ( bike->BatStatus ){
    case 0:
			if ( flashflag > 5 ) BL_Data[0x06] &= ~0x80; 
      break;
    case 1: BL_Data[0x07] = 0x80;break;
    case 2: BL_Data[0x07] = 0xC0;break;
    case 3: BL_Data[0x07] = 0xE0;break;
    case 4: BL_Data[0x07] = 0xF0;break;
    case 5: BL_Data[0x07] = 0xF1;break;
    case 6: BL_Data[0x07] = 0xF2;break;      
    case 7: BL_Data[0x07] = 0xF7;break;
    case 8: BL_Data[0x07] = 0xFF;break;          
    default:break; 
  }

  /***************************Temp Area Display**********************************/
	BL_Data[0x10] |= (SegDataTemp[abs(bike->Temperature/10)%10]);	//D2
	BL_Data[0x11] |= (SegDataTemp[abs(bike->Temperature/10)/10]);	//D1
	BL_Data[0x10] |= 0x01;		//S9
	if (bike->Temperature < 0)
		BL_Data[0x11] |= 0x01;	//S8       
		
	/***************************Time Area Display**********************************/
	if ( bike->HasTimer ){
		if(bike->Hour > 9) BL_Data[0x0A] |= 0x01;	//S13  
		BL_Data[0x08] |= ( SegDataTime[bike->Hour%10]);			//D5
		BL_Data[0x09] |= ( SegDataTime[bike->Minute/10] );	//D4
		BL_Data[0x0A] |= ( SegDataTime[bike->Minute%10] );	//D3    
		if ( bike->time_set ){
			switch ( bike->time_pos ){
			case 0:
				if ( flashflag > 5  ) { 
					BL_Data[0x08] &= 0x01; 
					BL_Data[0x09] &= 0x01; 
					BL_Data[0x0A] &= 0x01; 
				}
				break;			
			case 1:if ( flashflag > 5  ) BL_Data[0x08] &= 0x01; break;
			case 2:if ( flashflag > 5  ) BL_Data[0x09] &= 0x01; break;
			case 3:if ( flashflag > 5  ) BL_Data[0x0A] &= 0x01; break;
			default:break;		
			}
			BL_Data[0x09] |= 0x01;	//col
		} else if ( flashflag <= 5 ) BL_Data[7] |= 0x01;	//col
	}
	
  /*************************** Voltage Display**********************************/
  BL_Data[0x05] |= (SegDataVoltage[ bike->Voltage/10	%10]);
  BL_Data[0x06] |= (SegDataVoltage[(bike->Voltage/100)%10]);
  
  /*************************** Mile Display**********************************/  
  BL_Data[0x00] |= (SegDataMile [ bike->Mile			 %10]) | 0x80;	//S17
  BL_Data[0x01] |= (SegDataMile [(bike->Mile/10   )%10]);
  BL_Data[0x02] |= (SegDataMile [(bike->Mile/100  )%10]); 
  BL_Data[0x03] |= (SegDataMile [(bike->Mile/1000 )%10]);
  BL_Data[0x04] |= (SegDataMile [(bike->Mile/10000)%10]); 
  
  /*************************** Speed Display**********************************/
  BL_Data[0x0D] |= (SegDataSpeed[ bike->Speed%10]		 )&0x0F;
  BL_Data[0x0E] |= (SegDataSpeed[ bike->Speed%10]		 )&0xF0;
  BL_Data[0x0E] |= (SegDataSpeed[(bike->Speed/10)%10])&0x0F; 
  BL_Data[0x0F] |= (SegDataSpeed[(bike->Speed/10)%10])&0xF0; 
  BL_Data[0x0F] |= 0x08;	//S16
  
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

