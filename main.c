/* 
 * File:   main.c
 * Author: Matt
 *
 * Created on 13 July 2020, 06:17
 */


#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"
#include "config.h"
#include "util.h"
#include "usart.h"
#include "iopins.h"
#include "timers.h"

#ifdef __18F4320
#pragma config OSC = HSPLL     // Oscillator Selection bits (HS oscillator 4x PLL)
#pragma config WDT = OFF       // Watchdog Timer Enable bit
#pragma config FSCM = OFF
#pragma config PWRT = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config CP0 = OFF       // FLASH Program Memory Code Protection bit (Code protection off)
#pragma config CP1 = OFF
#pragma config CP2 = OFF
#pragma config CP3 = OFF
#pragma config WRT0 = OFF
#pragma config WRT1 = OFF
#pragma config WRT2 = OFF
#pragma config WRT3 = OFF
#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
#pragma config EBTRB = OFF
#pragma config BOR = ON
#pragma config BORV = 42
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config CPB = OFF
#pragma config WDTPS = 16384
#pragma config CCP2MX = ON
#pragma config PBAD = DIG
#pragma config MCLRE = ON
#pragma config STVR = ON
#pragma config LVP = OFF
#pragma config DEBUG = OFF
#endif

#define TAPE_ZONE_UNKNOWN  0
#define TAPE_ZONE_BOT      1
#define TAPE_ZONE_EOT      2
#define TAPE_ZONE_EW       3
#define TAPE_ZONE_DATA     4

typedef struct {
    uint8_t tape_zone;
} sys_runstate_t;

sys_config_t _g_cfg;
sys_runstate_t _g_rs;

static void operation_exercise(sys_runstate_t *rs, sys_config_t *config);
static void operation_rewind(sys_runstate_t *rs, sys_config_t *config);
static void write_test_pattern(uint8_t tape_zone, uint8_t track);
static void io_init(void);
static void check_keys(void);
static void check_toggle(void);

static void enable_testfreq(bool enable);

void drive_reset(void);
bool drive_select(bool selected);
void drive_select_track(uint8_t track);

void high_priority interrupt interrupt_handler_high(void) 
{
    // Test tone generator. This interrupts the MCU so often that
    // __delay_mx macros become 8x longer when this is running
    INTCONbits.TMR0IF = 0;
    LATC ^= 0x03;
    TMR0L = 0xFE;
}

void low_priority interrupt interrupt_handler_low(void)
{
    if (INTCONbits.RBIF)
    {
        INTCONbits.RBIF = 0;

        if (INPUT_ASSERTED(LTH) && INPUT_ASSERTED(UTH))
            _g_rs.tape_zone = TAPE_ZONE_BOT;
        if (!INPUT_ASSERTED(UTH) && INPUT_ASSERTED(LTH))
            _g_rs.tape_zone = TAPE_ZONE_EOT;
        if (INPUT_ASSERTED(UTH) && !INPUT_ASSERTED(LTH))
            _g_rs.tape_zone = TAPE_ZONE_EW;
        if (!INPUT_ASSERTED(UTH) && !INPUT_ASSERTED(LTH))
            _g_rs.tape_zone = TAPE_ZONE_DATA;
    }
}

int main(void)
{
    sys_runstate_t *rs = &_g_rs;
    sys_config_t *config = &_g_cfg;

    usart1_open(USART_CONT_RX, (((_XTAL_FREQ / UART_BAUD) / 64) - 1));
    
    io_init();
    timer0_init();
    timer0_stop();

    load_configuration(config);
    
    configuration_bootprompt(config);
    
    // Enable interrupts
    INTCONbits.GIE_GIEH = 1;
    INTCONbits.PEIE_GIEL = 1;

    rs->tape_zone = TAPE_ZONE_UNKNOWN;

    for (;;)
    {
        switch (config->operation)
        {
            case OPERATION_EXERCISE:
            case OPERATION_WRITE_TEST:
            {
                operation_exercise(rs, config);
                break;
            }
            case OPERATION_REWIND:
            {
                operation_rewind(rs, config);
                break;
            }
            default:
            {
                printf("Invalid or no operation specified. Press Ctrl+D to reset.\r\n");
                break;
            }
        }

        check_keys();

        delay_10ms(100);
    }
}

static void operation_rewind(sys_runstate_t *rs, sys_config_t *config)
{
    printf("Rewind running...\r\n");

    printf("Resetting drive\r\n");

    drive_reset();
    delay_10ms(200);

    printf("Selecting drive\r\n");
    if (!drive_select(true))
        return;

    printf("Rewinding tape... ");

    if (!drive_go(true, true))
        return;

    while (rs->tape_zone != TAPE_ZONE_BOT)
        check_keys();

    printf("Done\r\n");
    
    reset();
}

static void operation_exercise(sys_runstate_t *rs, sys_config_t *config)
{
    if (config->operation == OPERATION_WRITE_TEST)
        printf("Writing test tape...\r\n");
    else
        printf("Exercise running...\r\n");
    
    if (config->stopat_track > 8)
        config->stopat_track = 8;

    for (;;)
    {
        uint8_t track = 0;
        printf("Resetting drive\r\n");
        
        drive_reset();
        delay_10ms(200);
        
        printf("Selecting drive\r\n");
        if (!drive_select(true))
            return;
        
        for (;;)
        {
            if (track == 0)
                printf("Rewinding tape... ");
            else
                printf("Running tape to BOT... ");

            if (!drive_go(true, true))
                return;

            while (rs->tape_zone != TAPE_ZONE_BOT)
            {
                if (config->operation == OPERATION_WRITE_TEST)
                    write_test_pattern(rs->tape_zone, track);
                
                check_keys();
            }
            
            if (config->operation == OPERATION_WRITE_TEST)
                write_test_pattern(rs->tape_zone, track);

            if (!drive_go(false, false))
                return;

            printf("Done\r\n");
            
            if (config->operation == OPERATION_WRITE_TEST && track >= config->stopat_track)
            {
                printf("End of write\r\n");
                reset();
            }
        
            if (track)
                track++;
            
            printf("Moving to track: %d\r\n", track);
            
            drive_select_track(track);

            printf("Running tape to EOT... ");

            if (!drive_go(true, false))
                return;

            while (rs->tape_zone != TAPE_ZONE_EOT)
            {
                if (config->operation == OPERATION_WRITE_TEST)
                    write_test_pattern(rs->tape_zone, track);
                
                check_keys();
            }

            if (config->operation == OPERATION_WRITE_TEST)
                write_test_pattern(rs->tape_zone, track);

            if (!drive_go(false, false))
                return;

            printf("Done\r\n");
            
            if (config->operation == OPERATION_WRITE_TEST && track >= config->stopat_track)
            {
                printf("End of write\r\n");
                reset();
            }

            if (track == 8)
            {
                printf("End of exercise\r\n");
                track = 0;
            }
            else
            {
                track++;
                printf("Moving to track: %d\r\n", track);
                drive_select_track(track);
            }
        }
    }
}

static void write_test_pattern(uint8_t tape_zone, uint8_t track)
{
    if (tape_zone == TAPE_ZONE_DATA)
    {
        ASSERT(WEN); // This will also gate in the signal from the external function generator

        if (track == 0)
            ASSERT(EEN);
        else
            DEASSERT(EEN);
    }
    else
    {
        DEASSERT(EEN);
        DEASSERT(WEN);
    }
}

static void check_keys(void)
{
    if (usart1_data_ready())
    {
        char c = usart1_get();
        if (c == 4)
        {
            printf("\r\nCtrl+D received. Resetting...\r\n");
            reset();
        }
        if (c == 't')
        {
            if (OUTPUT_ASSERTED(TR0))
            {
                DEASSERT(TR0);
                putch('1');
                putch(' ');
            }
            else
            {
                ASSERT(TR0);
                putch('0');
                putch(' ');
            }
        }
    }
}

void drive_reset(void)
{
    ASSERT(RST);
    __delay_ms(15);
    DEASSERT(RST);
}

bool drive_select(bool selected)
{
    if (selected)
        ASSERT(DS0);
    else
    {
        DEASSERT(DS0);
        return true;
    }
    
    if (selected)
    {
        uint16_t i;
        for (i = 0; i < 5000; i++)
        {
            if (INPUT_ASSERTED(SLD))
                break;
            __delay_ms(1);
        }
    }
    
    __delay_ms(1);
    
    if (!INPUT_ASSERTED(CIN))
    {
        DEASSERT(DS0);
        printf("Error: No cartridge in drive.\r\n");
        return false;
    }
    
    if (!INPUT_ASSERTED(SLD))
    {
        printf("Error: Drive did not respond to select request.\r\n");
        return false;
    }
    
    return true;
}

bool drive_go(bool go, bool reverse)
{
    if (SLDbit)
    {
        printf("Error: failed to start drive motor. Drive not selected.\r\n");
        return false;
    }

    if (go)
        ASSERT(GO);
    else
        DEASSERT(GO);
    
    if (reverse)
        ASSERT(REV);
    else
        DEASSERT(REV);
    
    return true;
}

void drive_select_track(uint8_t track)
{
    if (track & 0x01)
        ASSERT(TR0);
    else
        DEASSERT(TR0);

    if (track & 0x02)
        ASSERT(TR1);
    else
        DEASSERT(TR1);

    if (track & 0x04)
        ASSERT(TR2);
    else
        DEASSERT(TR2);

    if (track & 0x08)
        ASSERT(TR3);
    else
        DEASSERT(TR3);
}

#if 0
static void enable_testfreq(bool enable)
{
    if (enable)
    {
        LATC = 0x01;
        WDMtris = 0;
        WDPtris = 0;
        timer0_start();
    }
    else
    {
        WDMtris = 1;
        WDPtris = 1;
        LATC = 0x03;
        timer0_stop();
    }
}
#endif

static void io_init(void)
{
    ADCON1bits.PCFG0 = 1;
    ADCON1bits.PCFG1 = 1;
    ADCON1bits.PCFG2 = 1;
    ADCON1bits.PCFG3 = 1;
    
    // Inputs
    TCHtris = 1;
    RDLtris = 1;
    RDPtris = 1;
    UTHtris = 1;
    LTHtris = 1;
    SLDtris = 1;
    CINtris = 1;
    USFtris = 1;
   
    // Outputs, but they are set to input unless pulled low
    GOtris  = 1;
    REVtris = 1;
    DS0tris = 1;
    TR3tris = 1;
    TR2tris = 1;
    TR1tris = 1;
    TR0tris = 1;
    RSTtris = 1;
    WDMtris = 1;
    WDPtris = 1;
    HSDtris = 1;
    WENtris = 1;
    EENtris = 1;

    GOlat  = 1;
    REVlat = 1;
    TR3lat = 1;
    TR2lat = 1;
    TR1lat = 1;
    TR0lat = 1;
    RSTlat = 1;
    DS0lat = 1;
    RDLlat = 1;
    WDMlat = 1;
    WDPlat = 1;
    HSDlat = 1;
    WENlat = 1;
    EENlat = 1;

    INTCONbits.GIE_GIEH = 0;
    INTCONbits.PEIE_GIEL = 0;

    INTCONbits.RBIE = 1;
    INTCON2bits.RBIP = 0;
    INTCON2bits.TMR0IP = 1;
    RCONbits.IPEN = 1;
}
