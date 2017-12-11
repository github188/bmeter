#ifndef __TM16XX_H__
#define __TM16XX_H__

extern unsigned char TM16XX[16];

void TM16XX_Init(unsigned char st);
void TM16XX_Write_Data(unsigned char* buf);

#endif
