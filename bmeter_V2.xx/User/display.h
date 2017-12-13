#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "bike.h"

#if ( defined LED1640 ) 
#define DisplayInit(x)	TM16XX_Init(x)
#else
#define DisplayInit(x)	BL55072_Config(x)
#endif

extern void MenuUpdate(BIKE_STATUS* bike);
extern void Delay(unsigned long nCount);

#endif