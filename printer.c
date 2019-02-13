/****************************************************************************
 *                                   \ /
 *                        C R E A T I V I S I O N
 *                            E M U L A T O R
 *                                  0.2.2
 *
 *       This emulator was written using Marat Fayzullin's Emulib
 *
 *       *YOU MAY NOT DISTRIBUTE THIS FILE COMMERCIALLY. PERIOD!*
 *---------------------------------------------------------------------------
 * printer.h
 *
 * Somewhat grandiose title - really it just chucks bytes to a file.
 * Really only suitable for dumping LLIST from BASIC
 ****************************************************************************/
#include "crvision.h"

static M6821 printer;
FILE *prt = NULL;
unsigned long long int prt_last = 0;
char pfname[MAX_WINPATH];

/**
 * WriteCentronics
 *
 * Output a character to a printer
 */
void WriteCentronics( int addr, unsigned char data )
{
    switch (addr & 1) {
    case 0: /* E800 */
        if ( printer.CR & PIA_PDR )
            printer.PDR = data;
        else
            printer.DDR = data;
        break;

    case 1: /* E801 */
        printer.CR = data;
        if ( printer.CR & 0x08 ) {  /* Strobe */
            if (!prt) prt = fopen(pfname, "a");
            switch(printer.PDR) {
            case 0:     /* Does nothing */
            case 13:    /* Confuses windows */
            case 127:   /* Sent at start of page */
                break;
            default:
                if (prt) {
                    fputc(printer.PDR, prt);
                }
                break;
            }
        }
        break;
    }
}

/**
 * ReadCentronics
 *
 * Just clock the data out
 */
unsigned char ReadCentronics( int addr )
{
    switch ( addr & 1 ) {
    case 0: /* E800 */
        if ( printer.CR & PIA_PDR )
            return printer.PDR | 0x80;
        else
            return printer.DDR | 0x80;
        break;

    case 1: /* E801 */
        return printer.CR | 0x80;   /* Always signal success */
        break;

    default:
        return 0xff;
    }
}
