/*
 * File:   iopins.h
 * Author: Matt
 *
 * Created on 13 July 2020, 06:17
 */

#ifndef __IOPINS_H__
#define __IOPINS_H__

#define ASSERT(pin) do {\
    pin##lat = 0;   \
    pin##tris = 0; \
    } while (0)

#define DEASSERT(pin) do {\
    pin##tris = 1; \
    pin##lat = 1;   \
    } while (0)

#define INPUT_ASSERTED(pin) (!(pin##bit))
#define OUTPUT_ASSERTED(pin) (!(pin##lat))

#define GObit       PORTDbits.RD0
#define REVbit      PORTDbits.RD3
#define TR3bit      PORTAbits.RA5
#define TR2bit      PORTDbits.RD4
#define TR1bit      PORTDbits.RD5
#define TR0bit      PORTDbits.RD6
#define RSTbit      PORTDbits.RD7
#define TCHbit      PORTBbits.RB0
#define DS0bit      PORTBbits.RB1
#define RDLbit      PORTBbits.RB2
#define RDPbit      PORTBbits.RB3
#define UTHbit      PORTBbits.RB4
#define LTHbit      PORTBbits.RB5
#define SLDbit      PORTCbits.RC5
#define CINbit      PORTAbits.RA3
#define USFbit      PORTDbits.RD2
#define WDMbit      PORTCbits.RC0
#define WDPbit      PORTCbits.RC1
#define HSDbit      PORTAbits.RA2
#define WENbit      PORTAbits.RA1
#define EENbit      PORTAbits.RA0

#define GOlat       LATDbits.LATD0
#define REVlat      LATDbits.LATD3
#define TR3lat      LATAbits.LATA5
#define TR2lat      LATDbits.LATD4
#define TR1lat      LATDbits.LATD5
#define TR0lat      LATDbits.LATD6
#define RSTlat      LATDbits.LATD7
#define DS0lat      LATBbits.LATB1
#define RDLlat      LATBbits.LATB2
#define WDMlat      LATCbits.LATC0
#define WDPlat      LATCbits.LATC1
#define HSDlat      LATAbits.LATA2
#define WENlat      LATAbits.LATA1
#define EENlat      LATAbits.LATA0

#define GOtris      TRISDbits.TRISD0
#define REVtris     TRISDbits.TRISD3
#define TR3tris     TRISAbits.TRISA5
#define TR2tris     TRISDbits.TRISD4
#define TR1tris     TRISDbits.TRISD5
#define TR0tris     TRISDbits.TRISD6
#define RSTtris     TRISDbits.TRISD7
#define TCHtris     TRISBbits.TRISB0
#define DS0tris     TRISBbits.TRISB1
#define RDLtris     TRISBbits.TRISB2
#define RDPtris     TRISBbits.TRISB3
#define UTHtris     TRISBbits.TRISB4
#define LTHtris     TRISBbits.TRISB5
#define SLDtris     TRISCbits.TRISC5
#define CINtris     TRISAbits.TRISA3
#define USFtris     TRISDbits.TRISD2
#define WDMtris     TRISCbits.TRISC0
#define WDPtris     TRISCbits.TRISC1
#define HSDtris     TRISAbits.TRISA2
#define WENtris     TRISAbits.TRISA1
#define EENtris     TRISAbits.TRISA0

#endif /* __IOPINS_H__ */
