/*
 * File:   usart.c
 * Author: Matt
 *
 * Created on 13 July 2020, 06:17
 */

#include <stdint.h>
#include <stdbool.h>

#include "project.h"
#include "usart.h"

#ifdef _USART1_

#if defined (__PIC18_K40__)
#define TXSTAbits TXSTA1bits
#define RCSTAbits RCSTA1bits
#define SPBRG SP1BRG
#define TXREG TX1REG
#define RCREG RC1REG
#endif

void usart1_open(uint8_t flags, uint8_t brg)
{
    if (flags & USART_SYNC)
        TXSTAbits.SYNC = 1;

    if (flags & USART_9BIT)
    {
        TXSTAbits.TX9 = 1;
        RCSTAbits.RX9 = 1;
    }

    if (flags & USART_SYNC_MASTER)
        TXSTAbits.CSRC = 1;

    if (flags & USART_CONT_RX)
        RCSTAbits.CREN = 1;
    else
        RCSTAbits.SREN = 1;

    if (flags & USART_BRGH)
        TXSTAbits.BRGH = 1;
    else
        TXSTAbits.BRGH = 0;

#ifdef __PIC18_K40__
    if (flags & USART_IOR)
        PIE3bits.RC1IE = 1;
    else
        PIE3bits.RC1IE = 0;
    
    if (flags & USART_IOT)
        PIE3bits.TX1IE = 1;
    else
        PIE3bits.TX1IE = 0;
#else
    if (flags & USART_IOR)
        PIE1bits.RCIE = 1;
    else
        PIE1bits.RCIE = 0;

    if (flags & USART_IOT)
        PIE1bits.TXIE = 1;
    else
        PIE1bits.TXIE = 0;    
#endif

    SPBRG = brg;

    TXSTAbits.TXEN = 1;
    RCSTAbits.SPEN = 1;
    
#ifdef __PIC18_K40__
    RX1PPS = 0x17;
    TX1PPS = 0x16;
    RC6PPS = 0x09;
#endif /* __PIC18_K40__ */
    
#if defined(__18F2550) || defined(__18F26K22) || defined(__18F26K40) || defined(__18F2520) || defined(__16F876A) || defined(__16F876) || defined(__18F4320)
    TRISCbits.TRISC6 = 0; // TX
    TRISCbits.TRISC7 = 1; // RX
    if (TXSTAbits.SYNC && !TXSTAbits.CSRC)	//Synchronous slave mode
        TRISCbits.TRISC6 = 1;
#else
#error Unknown device
#endif
}

bool usart1_busy(void)
{
    if (!TXSTAbits.TRMT)
        return true;
    return false;
}

void usart1_put(char c)
{
    TXREG = c;
}

bool usart1_data_ready(void)
{
#ifdef __PIC18_K40__
    if (PIR3bits.RC1IF)
#else
    if (PIR1bits.RCIF)
#endif
        return true;
    return false;
}

char usart1_get(void)
{
    char data;
    data = RCREG;
    return data;
}

void usart1_clear_oerr(void)
{
#ifndef __PIC18_K42__
    if (RCSTAbits.OERR)
    {
        /* Hack to clear overrun errors */
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
    }
#endif
}

#endif /* _USART1_ */