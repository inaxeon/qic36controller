#ifndef __PROJECT_H__
#define __PROJECT_H__

#define _USART1_

#define _XTAL_FREQ 49152000 // Flogging it a bit. Limit is 40MHz
#define UART_BAUD 9600

void drive_reset(void);
bool drive_select(bool selected);
void drive_select_track(uint8_t track);
bool drive_go(bool go, bool reverse);

#include <xc.h>

#endif /* __PROJECT_H__ */