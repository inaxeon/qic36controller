/*
 * File:   timers.c
 * Author: Matt
 *
 * Created on 20 June 2016, 17:11
 */

#include <stdint.h>
#include <stdbool.h>

#include "project.h"
#include "timers.h"

#define WD_INTERVALH              0x1F
#define WD_INTERVALL              0x06

//#define LED_BLINK               0xF0 /* 7.3267MHz */
#define LED_BLINK                 0xD8

void timer0_init(void)
{
    T0CONbits.T0PS0 = 0;
    T0CONbits.T0PS1 = 0;
    T0CONbits.T0PS2 = 0;
    T0CONbits.T0PS3 = 0;

    T0CONbits.T0CS = 0;
    T0CONbits.PSA = 1;
    
    T0CONbits.T08BIT = 1;

    INTCONbits.TMR0IE = 1;

    timer0_reset();
}

void timer0_start(void)
{
    T0CONbits.TMR0ON = 1;
}

void timer0_stop(void)
{
    T0CONbits.TMR0ON = 0;
}

void timer0_reset(void)
{
    TMR0H = LED_BLINK;
    TMR0L = 0x00;
}
