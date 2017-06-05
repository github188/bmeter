#ifndef __YXT_H__
#define __YXT_H__

#include "bike.h"

extern unsigned char YXT_Buf[];
extern unsigned char YXT_Status[];
extern unsigned char YXT_RxBit,YXT_RxData;
extern unsigned char YXT_Update;


void YXT_Init(void);
void YXT_Task(BIKE_STATUS *bike);
void YXT_Tim_Receive(unsigned short duty);

#endif

