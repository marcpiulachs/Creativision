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
 * Motorola M6821 PIA Interface
 *
 * The following is a mix of Mess,FunnyMu and my own thoughts!
 * I'll probably forget this in a couple of months, so here's my take on how
 * the PIA works.
 *
 * There are two PIA ports in the creatiVision console, addressed as
 *
 *  $1000    PIA 0 DDR/PDR (DATA PORT)
 *  $1001    PIA 0 CR
 *
 *  $1002    PIA 1 DDR/PDR (DATA PORT)
 *  $1003    PIA 1 CR
 *
 * Both work in a similar way, so this breakdown has been applied to both.
 *
 *   Control Register
 *  ------------------
 *
 *     MSB                                              LSB
 *  +------+------+------+------+------+------+------+------+
 *  | IRQ1 | IRQ2 |  C2  | C2   | C2   | DDR  | C1   | C1   |
 *  | FLAG | FLAG |  DIR | ST1  | ST2  | PDR  | IRQ1 | IRQ2 |
 *  +------+------+------+------+------+------+------+------+
 *  |  RO  |  RO  |  RW  |  RW  |  RW  |  RW  |  RW  |  RW  |
 *  +------+------+------+------+------+------+------+------+
 *
 * Handling IRQ1/2 is relatively straight forward. Take the mask from C1
 * ($3) and transpose to IRQ1/2 flags ($C0). The MC6821 documents states
 * that a signalled IRQ is cleared by any subsequent data port read.
 *
 * The only other interesting BIT is $4 (DDR/PDR) which selects reading
 * or writing to/from the Data Direction Register(DDR) or
 * Peripheral Data Register (PDR).
 *
 *  Data Direction Register
 * -------------------------
 * Sets which pins (BITs) are used for output (1) or input (0).
 *
 *  Peripheral Data Register
 * --------------------------
 * The actual data in or out.
 *
 * Cassette Data is read / written on port $1000
 *
 * There are $70 pulses of $80/$00 for bit on, and $40 pulses for off.
 ****************************************************************************/
#include "crvision.h"

/* PIA 0 and 1 */
M6821 pia0 = {0};
M6821 pia1 = {0};

int EmuQuit = 0;
int have_io = 0;
unsigned char KEYBD[8] = { 255, 255, 255, 255, 255, 255, 255, 255 };
unsigned char JOYSM[2] = { 255, 255 };

int io_timeout = 0;

/**
 * PIA_Write
 *
 * Called from cpu on access to memory in the range $1000 - $1FFF
 */
void PIA_Write(word addr, byte data)
{

    switch (addr & 3) {
    case 0: /* PIA 0 DATA PORT */
        if ( pia0.CR & PIA_PDR) {
            if ( pia0.DDR == PIA_OUTALL ) {
                /* This should only happen when the cassette is writing */
                CSAVE(data);
                pia0.PDR = data;
                pia0.CR = (pia0.CR & 0x3f) | ((pia0.CR & 3) << 6);
            } else {
                /* Input */
                pia0.PDR = data;
            }
        } else
            pia0.DDR = data;
        break;

    case 1: /* PIA 0 CONTROL */
        pia0.CR = (data & 0x3f);
        break;

    case 2: /* PIA 1 DATA PORT */
        if (pia1.CR & PIA_PDR) {
            if (pia1.DDR == PIA_OUTALL) {
                /* Store for retrieval */
                pia1.PDR = data;

                /* Output to SN76489 */
                SN76496SPWrite(0,data);
                pia1.prev_cycles = total_cycles;

                /* Set IRQ as complete */
                pia1.CR = (pia1.CR & 0x3f) | ( ( pia1.CR & 3) << 6);
            }
            else
                pia1.PDR = data;
        } else {
            pia1.DDR = data;
        }
        break;

    case 3: /* PIA 1 CONTROL */
        pia1.CR = data & 0x3f;
        break;
    }
}

/**
 * PIA_Read
 *
 * Updated to reflect the disassembly of the PIA diagnostic cart
 */
unsigned char PIA_Read(word addr)
{
    switch( addr & 3 )
    {
    case 0: /* PIA-0 Data Port */

        /* Is the PDR selected */
        if (pia0.CR & PIA_PDR)
        {
            /* Which device was requested? */
            if (pia0.DDR == PIA_CLOAD)
                pia0.PDR = CLOAD();

            if (!( pia0.DDR & ~pia0.PDR ))/* Is it not an input? */
                return 0xff;

            return pia0.PDR;

        }
        /* Request for DDR */
        return pia0.DDR;


    case 1: /* PIA-0 Control Port */
        pia0.CR &= 0x3f;
        return pia0.CR;

    case 2: /* PIA-1 Data Port */

        /* Clear interrupts */
        if (pia1.CR & PIA_PDR)
        {
            /* Fortunately, the mask set in pia0.PDR tells us which device */
            switch(pia0.PDR)
            {
            case 0xf7:      /* Keyboard Mux 8 */
                pia1.PDR = KEYBD[4];
                break;
            case 0xfb:      /* Keyboard Mux 4 */
                pia1.PDR = KEYBD[3];
                break;
            case 0xfd:      /* Keyboard Mux 2 */
                pia1.PDR = KEYBD[2];
                break;
            case 0xfe:      /* Keyboard Mux 1 */
                pia1.PDR = KEYBD[1];
                break;
            default:
                if (pia1.DDR == PIA_OUTALL) {
                    pia1.CR &= 0x3f;
                    return pia1.PDR;
                }

                if (!(pia1.PDR & ~pia1.DDR)) return 0xff;

                return pia1.PDR;
            }

            return pia1.PDR;
        }

        return pia1.DDR;

    case 3: /* PIA-1 Control Port */
        if ( (total_cycles - pia1.prev_cycles) < 36)
            return pia1.CR;

        pia1.CR &= 0x3f;
        return pia1.CR;
    }

    return 0;   /* Will never get here */
}

/**
 * PauseCV
 *
 * Simply pauses play
 */
void PauseCV( void )
{
    SDL_Event event;
    int quit = 1;

    SDL_PauseAudio(1);

    while (quit) {
        if (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_F2)
                    quit=0;
            }
        }
    }

    SDL_PauseAudio(demo_record);
}

/**
 * check_input
 *
 * Check keyboard each scan line.
 *
 * The values are taken from FunnyMu 0.49 and are verified with Mess
 * crvision driver.
 *
 * CSL, Salora Manager and Laser 2001 were gleaned from BIOS
 *
 */
int check_input( void )
{
    int ret = 0;
    SDL_Event event;

    /* Was a key hit ?*/
    if (SDL_PollEvent((&event))) {

        switch (event.type) {

        case SDL_KEYDOWN:

            switch( event.key.keysym.sym ) {
                /* Emulator Specific */
            case SDLK_ESCAPE:
                EmuQuit = 1;
                break;

            case SDLK_F2:
                PauseCV();
                break;

            case SDLK_F3:
                ret = 1;
                break;

            case SDLK_F4:
                if (cassette)
                    fseek(cassette, 0, SEEK_SET);
                break;

            case SDLK_F5:
                SDL_PauseAudio(1);
                WritePNG();
                SDL_PauseAudio(demo_record);
                break;

                /* Save snapshot */
            case SDLK_F7:
                SDL_PauseAudio(1);
                SaveSnapShot();
                SDL_PauseAudio(demo_record);
                break;

                /* Load snapshot */
            case SDLK_F8:
                SDL_PauseAudio(1);
                LoadSnapShot();
                SDL_PauseAudio(demo_record);
                break;

            case SDLK_F10:
                SDL_PauseAudio(1);
                FILE *mem;
                mem = fopen("ram.bin","wb");
                if(mem) {
                    if (cslCart) fwrite(RAM,1,0xc000,mem);
                    else fwrite(RAM,1,1024,mem);
                }
                if(mem)fclose(mem);
                mem = fopen("vram.bin","wb");
                if(mem)fwrite(VDP.VRAM, 1, 0x4000, mem);
                if(mem)fclose(mem);
                SDL_PauseAudio(demo_record);
                break;

                /* VDP Debug */
            case SDLK_F12:
                vdp_halt = 1;
                break;

                /* PA0 */
            case SDLK_DOWN:
                KEYBD[PA0] &= 0xfd;
                break;     /* PA0-1 P1 Down */
            case SDLK_1:
                KEYBD[PA0] &= 0xf3;
                break;        /* PA0-2 "1" */
            case SDLK_RIGHT:
                if (cslCart) KEYBD[PA2] &= 0x7f;
                else KEYBD[PA0] &= 0xfb;
                break;    /* PA0-2 P1 Right */
            case SDLK_UP:
                KEYBD[PA0] &= 0xf7;
                break;       /* PA0-3 P1 Up */
            case SDLK_LEFT:
                if (cslCart) KEYBD[PA1] &= 0xf6;
                else KEYBD[PA0] &= 0xdf;
                break;     /* PA0-5 P1 Left */
            case SDLK_RSHIFT:
                KEYBD[PA1] &= 0x7f;
                break;    /* PA0-7 P1 Right Button */
            case SDLK_RCTRL:
                KEYBD[PA0] &= 0x7f;
                break;    /* PA0-7 P1 Right Button */

                /* PA1 - 0 */
            case SDLK_f:
                KEYBD[PA1] &= 0xfc;
                break;        /* PA1-0 "F" */
            case SDLK_g:
                KEYBD[PA1] &= 0xfa;
                break;        /* PA1-0 "G" */
            case SDLK_BACKSPACE:
                KEYBD[PA1] &= 0xf6;
                break;/* PA1-0 Backspace */
            case SDLK_a:
                KEYBD[PA1] &= 0xee;
                break;        /* PA1-0 "A" */
            case SDLK_s:
                KEYBD[PA1] &= 0xde;
                break;        /* PA1-0 "S" */
            case SDLK_d:
                KEYBD[PA1] &= 0xbe;
                break;        /* PA1-0 "D" */

                /* PA1 - 1 */
            case SDLK_b:
                KEYBD[PA1] &= 0xf9;
                break;        /* PA1-1 "B" */
            case SDLK_z:
                KEYBD[PA1] &= 0xf5;
                break;        /* PA1-1 "Z" */
            case SDLK_x:
                KEYBD[PA1] &= 0xed;
                break;        /* PA1-1 "X" */
            case SDLK_c:
                KEYBD[PA1] &= 0xdd;
                break;        /* PA1-1 "C" */
            case SDLK_v:
                KEYBD[PA1] &= 0xbd;
                break;        /* PA1-1 "V" */

                /* PA1 - 2 */
            case SDLK_w:
                KEYBD[PA1] &= 0xf3;
                break;        /* PA1-2 "W" */
            case SDLK_e:
                KEYBD[PA1] &= 0xeb;
                break;        /* PA1-2 "E" */
            case SDLK_r:
                KEYBD[PA1] &= 0xdb;
                break;        /* PA1-2 "R" */
            case SDLK_t:
                KEYBD[PA1] &= 0xbb;
                break;        /* PA1-2 "T" */

                /* PA1 - 3 */
            case SDLK_q:
                KEYBD[PA1] &= 0xe7;
                break;        /* PA1-3 "Q" */
            case SDLK_4:
                KEYBD[PA1] &= 0xd7;
                break;        /* PA1-3 "4" */
            case SDLK_5:
                KEYBD[PA1] &= 0xb7;
                break;        /* PA1-3 "5" */

                /* PA1 - 4 */
            case SDLK_2:
                KEYBD[PA1] &= 0xcf;
                break;        /* PA1-4 "2" */
            case SDLK_6:
                KEYBD[PA1] &= 0xaf;
                break;        /* PA1-4 "6" */

                /* PA1 - 5 */
            case SDLK_3:
                KEYBD[PA1] &= 0x9f;
                break;        /* PA1-5 "3" */

                /* PA1 - 7 */
            case SDLK_LCTRL:
                KEYBD[PA0] &= 0x7f;
                break;   /* PA1-7 P1 Left Button */
            case SDLK_LSHIFT:
                KEYBD[PA1] &= 0x7f;
                break;   /* PA1-7 P1 Left Button */

                /* PA2 */
            case SDLK_END:
                if (cslCart) KEYBD[PA0] &= 0xfd;
                else KEYBD[PA2] &= 0xfd;
                break;      /* PA2-1 P2 Down */
            case SDLK_SPACE:
                KEYBD[PA2] &= 0xf3;
                break;    /* PA2-2 SPACEBAR */
            case SDLK_PAGEDOWN:
                if (cslCart) KEYBD[PA0] &= 0xfb;
                else KEYBD[PA2] &= 0xfb;
                break; /* PA2-2 P2 Right */
            case SDLK_HOME:
                if (cslCart) KEYBD[PA0] &= 0xf7;
                else KEYBD[PA2] &= 0xf7;
                break;     /* PA2-3 P2 Up */
            case SDLK_DELETE:
                if (cslCart) KEYBD[PA0] &= 0xdf;
                else KEYBD[PA2] &= 0xdf;
                break;   /* PA2-5 P2 Left */
            case SDLK_TAB:
                KEYBD[PA2] &= 0x7f;
                break;
            case SDLK_PAGEUP:
                KEYBD[PA2] &= 0x7f;
                break;   /* PA2-7 P2 Right Button */

                /* PA3 - 0 */
            case SDLK_u:
                KEYBD[PA3] &= 0xfc;
                break;        /* PA3-0 "U" */
            case SDLK_y:
                KEYBD[PA3] &= 0xfa;
                break;        /* PA3-0 "Y" */
            case SDLK_RETURN:
                KEYBD[PA3] &= 0xf6;
                break;   /* PA3-0 RETURN */
            case SDLK_p:
                KEYBD[PA3] &= 0xee;
                break;        /* PA3-0 "P" */
            case SDLK_o:
                KEYBD[PA3] &= 0xde;
                break;        /* PA3-0 "O" */
            case SDLK_i:
                KEYBD[PA3] &= 0xbe;
                break;        /* PA3-0 "I" */

                /* PA3 - 1 */
            case SDLK_7:
                KEYBD[PA3] &= 0xf9;
                break;        /* PA3-1 "7" */
            case SDLK_MINUS:
                KEYBD[PA3] &= 0xf5;
                break;    /* PA3-1 ":" */
            case SDLK_0:
                KEYBD[PA3] &= 0xed;
                break;        /* PA3-1 "0" */
            case SDLK_9:
                KEYBD[PA3] &= 0xdd;
                break;        /* PA3-1 "9" */
            case SDLK_8:
                KEYBD[PA3] &= 0xbd;
                break;        /* PA3-1 "8" */

                /* PA3 - 2 */
            case SDLK_l:
                KEYBD[PA3] &= 0xf3;
                break;        /* PA3-2 "L" */
            case SDLK_k:
                KEYBD[PA3] &= 0xeb;
                break;        /* PA3-2 "K" */
            case SDLK_j:
                KEYBD[PA3] &= 0xdb;
                break;        /* PA3-2 "J" */
            case SDLK_h:
                KEYBD[PA3] &= 0xbb;
                break;        /* PA3-2 "H" */

                /* PA3 - 3 */
            case SDLK_SEMICOLON:
                KEYBD[PA3] &= 0xe7;
                break;        /* PA3-3 ";" */
            case SDLK_COMMA:
                KEYBD[PA3] &= 0xd7;
                break;        /* PA3-3 "," */
            case SDLK_m:
                KEYBD[PA3] &= 0xb7;
                break;        /* PA3-3 "M" */

                /* PA3 - 4 */
            case SDLK_SLASH:
                KEYBD[PA3] &= 0xcf;
                break;    /* PA3-4 "/" */
            case SDLK_n:
                KEYBD[PA3] &= 0xaf;
                break;        /* PA3-4 "N" */

                /* PA3 - 5 */
            case SDLK_PERIOD:
                KEYBD[PA3] &= 0x9f;
                break;    /* PA3-5 "." */

                /* PA3 - 7 */
            case SDLK_EQUALS:
                KEYBD[PA3] &= 0x7f;
                break;    /* PA3-7 P2 Left Button */

            case SDLK_INSERT:
                KEYBD[PA3] &= 0x7f;
                break;    /* PA3-7 P2 Left Button */
            }
            break;

        case SDL_KEYUP:
            switch(event.key.keysym.sym) {
                /* PA0 */
            case SDLK_DOWN:
                KEYBD[PA0] |= 0x02;
                break;     /* PA0-1 P1 Down */
            case SDLK_1:
                KEYBD[PA0] |= 0x0c;
                break;     /* PA0-2 "1" */
            case SDLK_RIGHT:
                if (cslCart) KEYBD[PA2] |= 0x80;
                else KEYBD[PA0] |= 0x04;
                break;    /* PA0-2 P1 Right */
            case SDLK_UP:
                KEYBD[PA0] |= 0x08;
                break;     /* PA0-3 P1 Up */
            case SDLK_LEFT:
                if (cslCart) KEYBD[PA1] |= 0x09;
                else KEYBD[PA0] |= 0x20;
                break;     /* PA0-5 P1 Left */
            case SDLK_RSHIFT:
                KEYBD[PA1] |= 0x80;
                break;    /* PA0-7 P1 Right Button */
            case SDLK_RCTRL:
                KEYBD[PA0] |= 0x80;
                break;    /* PA0-7 P1 Right Button */

                /* PA1 - 0 */
            case SDLK_f:
                KEYBD[PA1] |= 0x03;
                break;        /* PA1-0 "F" */
            case SDLK_g:
                KEYBD[PA1] |= 0x05;
                break;        /* PA1-0 "G" */
            case SDLK_BACKSPACE:
                KEYBD[PA1] |= 0x09;
                break;/* PA1-0 Backspace */
            case SDLK_a:
                KEYBD[PA1] |= 0x11;
                break;        /* PA1-0 "A" */
            case SDLK_s:
                KEYBD[PA1] |= 0x21;
                break;        /* PA1-0 "S" */
            case SDLK_d:
                KEYBD[PA1] |= 0x41;
                break;        /* PA1-0 "D" */

                /* PA1 - 1 */
            case SDLK_b:
                KEYBD[PA1] |= 0x06;
                break;        /* PA1-1 "B" */
            case SDLK_z:
                KEYBD[PA1] |= 0x0a;
                break;        /* PA1-1 "Z" */
            case SDLK_x:
                KEYBD[PA1] |= 0x12;
                break;        /* PA1-1 "X" */
            case SDLK_c:
                KEYBD[PA1] |= 0x22;
                break;        /* PA1-1 "C" */
            case SDLK_v:
                KEYBD[PA1] |= 0x42;
                break;        /* PA1-1 "V" */

                /* PA1 - 2 */
            case SDLK_w:
                KEYBD[PA1] |= 0x0c;
                break;        /* PA1-2 "W" */
            case SDLK_e:
                KEYBD[PA1] |= 0x14;
                break;        /* PA1-2 "E" */
            case SDLK_r:
                KEYBD[PA1] |= 0x24;
                break;        /* PA1-2 "R" */
            case SDLK_t:
                KEYBD[PA1] |= 0x44;
                break;        /* PA1-2 "T" */

                /* PA1 - 3 */
            case SDLK_q:
                KEYBD[PA1] |= 0x18;
                break;        /* PA1-3 "Q" */
            case SDLK_4:
                KEYBD[PA1] |= 0x28;
                break;        /* PA1-3 "4" */
            case SDLK_5:
                KEYBD[PA1] |= 0x48;
                break;        /* PA1-3 "5" */

                /* PA1 - 4 */
            case SDLK_2:
                KEYBD[PA1] |= 0x30;
                break;        /* PA1-4 "2" */
            case SDLK_6:
                KEYBD[PA1] |= 0x50;
                break;        /* PA1-4 "6" */

                /* PA1 - 5 */
            case SDLK_3:
                KEYBD[PA1] |= 0x60;
                break;        /* PA1-5 "3" */

                /* PA1 - 7 */
            case SDLK_LCTRL:
                KEYBD[PA0] |= 0x80;
                break;   /* PA1-7 P1 Left Button */
            case SDLK_LSHIFT:
                KEYBD[PA1] |= 0x80;
                break;   /* PA1-7 P1 Left Button */

                /* PA2 */
            case SDLK_END:
                if (cslCart) KEYBD[PA0] |= 0x02;
                else KEYBD[PA2] |= 0x02;
                break;      /* PA2-1 P2 Down */
            case SDLK_SPACE:
                KEYBD[PA2] |= 0x0c;
                break;    /* PA2-2 SPCAEBAR */
            case SDLK_PAGEDOWN:
                if (cslCart) KEYBD[PA0] |= 0x04;
                else KEYBD[PA2] |= 0x04;
                break; /* PA2-2 P2 Right */
            case SDLK_HOME:
                if (cslCart) KEYBD[PA0] |= 0x08;
                else KEYBD[PA2] |= 0x08;
                break;     /* PA2-3 P2 Up */
            case SDLK_DELETE:
                if (cslCart) KEYBD[PA0] |= 0x20;
                else KEYBD[PA2] |= 0x20;
                break;   /* PA2-5 P2 Left */
            case SDLK_TAB:
                KEYBD[PA2] |= 0x80;
                break;
            case SDLK_PAGEUP:
                KEYBD[PA2] |= 0x80;
                break;   /* PA2-7 P2 Right Button */

                /* PA3 - 0 */
            case SDLK_u:
                KEYBD[PA3] |= 0x03;
                break;        /* PA3-0 "U" */
            case SDLK_y:
                KEYBD[PA3] |= 0x05;
                break;        /* PA3-0 "Y" */
            case SDLK_RETURN:
                KEYBD[PA3] |= 0x09;
                break;   /* PA3-0 RETURN */
            case SDLK_p:
                KEYBD[PA3] |= 0x11;
                break;        /* PA3-0 "P" */
            case SDLK_o:
                KEYBD[PA3] |= 0x21;
                break;        /* PA3-0 "O" */
            case SDLK_i:
                KEYBD[PA3] |= 0x41;
                break;        /* PA3-0 "I" */

                /* PA3 - 1 */
            case SDLK_7:
                KEYBD[PA3] |= 0x06;
                break;        /* PA3-1 "7" */
            case SDLK_MINUS:
                KEYBD[PA3] |= 0x0a;
                break;    /* PA3-1 ":" */
            case SDLK_0:
                KEYBD[PA3] |= 0x12;
                break;        /* PA3-1 "0" */
            case SDLK_9:
                KEYBD[PA3] |= 0x22;
                break;        /* PA3-1 "9" */
            case SDLK_8:
                KEYBD[PA3] |= 0x42;
                break;        /* PA3-1 "8" */

                /* PA3 - 2 */
            case SDLK_l:
                KEYBD[PA3] |= 0x0c;
                break;        /* PA3-2 "L" */
            case SDLK_k:
                KEYBD[PA3] |= 0x14;
                break;        /* PA3-2 "K" */
            case SDLK_j:
                KEYBD[PA3] |= 0x24;
                break;        /* PA3-2 "J" */
            case SDLK_h:
                KEYBD[PA3] |= 0x44;
                break;        /* PA3-2 "H" */

                /* PA3 - 3 */
            case SDLK_SEMICOLON:
                KEYBD[PA3] |= 0x18;
                break;    /* PA3-3 ";" */
            case SDLK_COMMA:
                KEYBD[PA3] |= 0x28;
                break;        /* PA3-3 "," */
            case SDLK_m:
                KEYBD[PA3] |= 0x48;
                break;            /* PA3-3 "M" */

                /* PA3 - 4 */
            case SDLK_SLASH:
                KEYBD[PA3] |= 0x30;
                break;    /* PA3-4 "/" */
            case SDLK_n:
                KEYBD[PA3] |= 0x50;
                break;        /* PA3-4 "N" */

                /* PA3 - 5 */
            case SDLK_PERIOD:
                KEYBD[PA3] |= 0x60;
                break;    /* PA3-5 "." */

                /* PA3 - 7 */
            case SDLK_EQUALS:
                KEYBD[PA3] |= 0x80;
                break;    /* PA3-7 P2 Left Button */

            case SDLK_INSERT:
                KEYBD[PA3] |= 0x80;
                break;    /* PA3-7 P2 Left Button */
            }
            break;

        case SDL_QUIT:
            EmuQuit = 1;
            break;

        case SDL_JOYAXISMOTION:
            SetJoyXY(event);
            break;

        case SDL_JOYBUTTONDOWN:
            SetButtonDown(event);
            break;

        case SDL_JOYBUTTONUP:
            SetButtonUp(event);
            break;
        }
    }

    return ret;
}

/**
 * check_input_salora
 *
 * Check keyboard each scan line.
 *
 * Salora Manager and Laser 2001 were gleaned from BIOS
 *
 */
int check_input_salora( void )
{
    int ret = 0;
    SDL_Event event;

    /* Was a key hit ?*/
    if (SDL_PollEvent((&event))) {

        switch (event.type) {

        case SDL_KEYDOWN:

            switch( event.key.keysym.sym ) {
                /* Emulator Specific */
            case SDLK_ESCAPE:
                EmuQuit = 1;
                break;

            case SDLK_F2:
                PauseCV();
                break;

            case SDLK_F3:
                ret = 1;
                break;

            case SDLK_F4:
                if (cassette)
                    fseek(cassette, 0, SEEK_SET);
                break;

            case SDLK_F5:
                SDL_PauseAudio(1);
                WritePNG();
                SDL_PauseAudio(demo_record);
                break;

                /* Save snapshot */
            case SDLK_F7:
                SDL_PauseAudio(1);
                SaveSnapShot();
                SDL_PauseAudio(demo_record);
                break;

                /* Load snapshot */
            case SDLK_F8:
                SDL_PauseAudio(1);
                LoadSnapShot();
                SDL_PauseAudio(demo_record);
                break;

            case SDLK_F10:
                SDL_PauseAudio(1);
                FILE *mem;
                mem = fopen("ram.bin","wb");
                if(mem) {
                    if (cslCart || salora) fwrite(RAM,1,0xc000,mem);
                    else fwrite(RAM,1,1024,mem);
                }
                if(mem)fclose(mem);
                mem = fopen("vram.bin","wb");
                if(mem)fwrite(VDP.VRAM, 1, 0x4000, mem);
                if(mem)fclose(mem);
                SDL_PauseAudio(demo_record);
                break;

                /* VDP Debug */
            case SDLK_F12:
                vdp_halt = 1;
                break;

                /* M0 */
            case SDLK_PERIOD:
                KEYBD[0] &= ~0x04;
                break;
            case SDLK_RETURN:
                KEYBD[0] &= ~0x08;
                break;
            case SDLK_3:
                KEYBD[0] &= ~0x10;
                break;
            case SDLK_e:
                KEYBD[0] &= ~0x20;
                break;
            case SDLK_s:
                KEYBD[0] &= ~0x40;
                break;
            case SDLK_x:
                KEYBD[0] &= ~0x80;
                break;

                /* M1 */
            case SDLK_LEFT:
                KEYBD[1] &= ~0x04;
                break;
            case SDLK_MINUS:
                KEYBD[1] &= ~0x08;
                break;
            case SDLK_2:
                KEYBD[1] &= ~0x10;
                break;
            case SDLK_w:
                KEYBD[1] &= ~0x20;
                break;
            case SDLK_a:
                KEYBD[1] &= ~0x40;
                break;
            case SDLK_z:
                KEYBD[1] &= ~0x80;
                break;

                /* M2 */
            case SDLK_SLASH:
                KEYBD[2] &= ~0x04;
                break;
            case SDLK_RIGHT:
                KEYBD[2] &= ~0x08;
                break;
            case SDLK_1:
                KEYBD[2] &= ~0x10;
                break;
            case SDLK_q:
                KEYBD[2] &= ~0x20;
                break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                KEYBD[2] &= ~0x40;
                break;
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                KEYBD[2] &= ~0x80;
                break;

                /* M3 */
            case SDLK_SEMICOLON:
                KEYBD[3] &= ~0x04;
                break;
            case SDLK_EQUALS:
                KEYBD[3] &= ~0x08;
                break;
            case SDLK_4:
                KEYBD[3] &= ~0x10;
                break;
            case SDLK_r:
                KEYBD[3] &= ~0x20;
                break;
            case SDLK_d:
                KEYBD[3] &= ~0x40;
                break;
            case SDLK_c:
                KEYBD[3] &= ~0x80;
                break;

                /* M4 */
            case SDLK_COMMA:
                KEYBD[4] &= ~0x04;
                break;
            case SDLK_p:
                KEYBD[4] &= ~0x08;
                break;
            case SDLK_5:
                KEYBD[4] &= ~0x10;
                break;
            case SDLK_t:
                KEYBD[4] &= ~0x20;
                break;
            case SDLK_f:
                KEYBD[4] &= ~0x40;
                break;
            case SDLK_v:
                KEYBD[4] &= ~0x80;
                break;

                /* M5 */
            case SDLK_l:
                KEYBD[5] &= ~0x04;
                break;
            case SDLK_0:
                KEYBD[5] &= ~0x08;
                break;
            case SDLK_6:
                KEYBD[5] &= ~0x10;
                break;
            case SDLK_y:
                KEYBD[5] &= ~0x20;
                break;
            case SDLK_g:
                KEYBD[5] &= ~0x40;
                break;
            case SDLK_b:
                KEYBD[5] &= ~0x80;
                break;

                /* M6 */
            case SDLK_k:
                KEYBD[6] &= ~0x04;
                break;
            case SDLK_9:
                KEYBD[6] &= ~0x08;
                break;
            case SDLK_7:
                KEYBD[6] &= ~0x10;
                break;
            case SDLK_u:
                KEYBD[6] &= ~0x20;
                break;
            case SDLK_h:
                KEYBD[6] &= ~0x40;
                break;
            case SDLK_n:
                KEYBD[6] &= ~0x80;
                break;

                /* M7 */
            case SDLK_SPACE:
                KEYBD[7] &= ~0x04;
                break;
            case SDLK_o:
                KEYBD[7] &= ~0x08;
                break;
            case SDLK_8:
                KEYBD[7] &= ~0x10;
                break;
            case SDLK_i:
                KEYBD[7] &= ~0x20;
                break;
            case SDLK_j:
                KEYBD[7] &= ~0x40;
                break;
            case SDLK_m:
                KEYBD[7] &= ~0x80;
                break;

                /* Joysticks */
            case SDLK_HOME:
                JOYSM[0] &= ~0x08;
                break;
            case SDLK_DELETE:
                JOYSM[0] &= ~0x20;
                break;
            case SDLK_PAGEDOWN:
                JOYSM[0] &= ~0x04;
                break;
            case SDLK_END:
                JOYSM[0] &= ~0x02;
                break;
            case SDLK_TAB:
                JOYSM[0] &= ~0x80;
                break;
            case SDLK_BACKSPACE:
                JOYSM[1] &= ~0x80;
                break;

            }
            break;

        case SDL_KEYUP:
            switch(event.key.keysym.sym) {
                /* M0 */
            case SDLK_PERIOD:
                KEYBD[0] |= 0x04;
                break;
            case SDLK_RETURN:
                KEYBD[0] |= 0x08;
                break;
            case SDLK_3:
                KEYBD[0] |= 0x10;
                break;
            case SDLK_e:
                KEYBD[0] |= 0x20;
                break;
            case SDLK_s:
                KEYBD[0] |= 0x40;
                break;
            case SDLK_x:
                KEYBD[0] |= 0x80;
                break;

                /* M1 */
            case SDLK_LEFT:
                KEYBD[1] |= 0x04;
                break;
            case SDLK_MINUS:
                KEYBD[1] |= 0x08;
                break;
            case SDLK_2:
                KEYBD[1] |= 0x10;
                break;
            case SDLK_w:
                KEYBD[1] |= 0x20;
                break;
            case SDLK_a:
                KEYBD[1] |= 0x40;
                break;
            case SDLK_z:
                KEYBD[1] |= 0x80;
                break;

                /* M2 */
            case SDLK_SLASH:
                KEYBD[2] |= 0x04;
                break;
            case SDLK_RIGHT:
                KEYBD[2] |= 0x08;
                break;
            case SDLK_1:
                KEYBD[2] |= 0x10;
                break;
            case SDLK_q:
                KEYBD[2] |= 0x20;
                break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                KEYBD[2] |= 0x40;
                break;
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                KEYBD[2] |= 0x80;
                break;

                /* M3 */
            case SDLK_SEMICOLON:
                KEYBD[3] |= 0x04;
                break;
            case SDLK_EQUALS:
                KEYBD[3] |= 0x08;
                break;
            case SDLK_4:
                KEYBD[3] |= 0x10;
                break;
            case SDLK_r:
                KEYBD[3] |= 0x20;
                break;
            case SDLK_d:
                KEYBD[3] |= 0x40;
                break;
            case SDLK_c:
                KEYBD[3] |= 0x80;
                break;

                /* M4 */
            case SDLK_COMMA:
                KEYBD[4] |= 0x04;
                break;
            case SDLK_p:
                KEYBD[4] |= 0x08;
                break;
            case SDLK_5:
                KEYBD[4] |= 0x10;
                break;
            case SDLK_t:
                KEYBD[4] |= 0x20;
                break;
            case SDLK_f:
                KEYBD[4] |= 0x40;
                break;
            case SDLK_v:
                KEYBD[4] |= 0x80;
                break;

                /* M5 */
            case SDLK_l:
                KEYBD[5] |= 0x04;
                break;
            case SDLK_0:
                KEYBD[5] |= 0x08;
                break;
            case SDLK_6:
                KEYBD[5] |= 0x10;
                break;
            case SDLK_y:
                KEYBD[5] |= 0x20;
                break;
            case SDLK_g:
                KEYBD[5] |= 0x40;
                break;
            case SDLK_b:
                KEYBD[5] |= 0x80;
                break;

                /* M6 */
            case SDLK_k:
                KEYBD[6] |= 0x04;
                break;
            case SDLK_9:
                KEYBD[6] |= 0x08;
                break;
            case SDLK_7:
                KEYBD[6] |= 0x10;
                break;
            case SDLK_u:
                KEYBD[6] |= 0x20;
                break;
            case SDLK_h:
                KEYBD[6] |= 0x40;
                break;
            case SDLK_n:
                KEYBD[6] |= 0x80;
                break;

                /* M7 */
            case SDLK_SPACE:
                KEYBD[7] |= 0x04;
                break;
            case SDLK_o:
                KEYBD[7] |= 0x08;
                break;
            case SDLK_8:
                KEYBD[7] |= 0x10;
                break;
            case SDLK_i:
                KEYBD[7] |= 0x20;
                break;
            case SDLK_j:
                KEYBD[7] |= 0x40;
                break;
            case SDLK_m:
                KEYBD[7] |= 0x80;
                break;

                /* Joysticks */
            case SDLK_HOME:
                JOYSM[0] |= 0x08;
                break;
            case SDLK_DELETE:
                JOYSM[0] |= 0x20;
                break;
            case SDLK_PAGEDOWN:
                JOYSM[0] |= 0x04;
                break;
            case SDLK_END:
                JOYSM[0] |= 0x02;
                break;
            case SDLK_TAB:
                JOYSM[0] |= 0x80;
                break;
            case SDLK_BACKSPACE:
                JOYSM[1] |= 0x80;
                break;
            }
            break;

        case SDL_QUIT:
            EmuQuit = 1;
            break;

        case SDL_JOYAXISMOTION:
            SetJoyXY(event);
            break;

        case SDL_JOYBUTTONDOWN:
            SetButtonDown(event);
            break;

        case SDL_JOYBUTTONUP:
            SetButtonUp(event);
            break;
        }
    }

    return ret;
}

void SM_PIA_Write( word addr, byte data)
{
    //printf("%04XW: %02X [%04X]\n", addr, data, CPU.PC.W);

    if (addr == 0x1A5A) {
        /**
          * Salora Manager BLOAD / CLOAD / etc
          *
          * Timing is critical on the PIA IRQ line.
          * To overcome this, here the emulator takes
          * care of loading.
          *
          * A == 0x5A == CLOAD
          * A == 0xA5 == BLOAD
          * A == 0x55 == RECALL
          */

        switch (data)
        {
        case 0x5A: /* CLOAD */
            SM_CLOAD();
            return;

        case 0xA5: /* BLOAD */
            SM_BLOAD();
            return;

        case 0x55: /* RECALL */
            SM_RECALL();
            return;
        }
    }

    switch (addr & 3) {

    case 0:
        if (pia0.CR & PIA_PDR)
            pia0.PDR = data;
        else
            pia0.DDR = data;
        break;

    case 1:
        pia0.CR = data;
        if ((pia0.CR == 0x30 ) && (pia0.DDR == PIA_INPUT) && (pia0.PDR == 0))
        {
            SM_CSAVE();
        }
        else
            pia0.CR = (pia0.CR & 0x3f) | ((pia0.CR & 3) << 6);
        break;

    case 2:
        pia1.CR = (pia1.CR & 0x3f) | ((pia1.CR & 3) << 6);
        if (pia1.CR & PIA_PDR) {
            pia1.PDR = data;
            /* PSG */
            if ((pia1.DDR == PIA_OUTALL) && (pia1.CR == 0xa6)) {
                SN76496SPWrite(0,data);
                pia1.prev_cycles = total_cycles;
            }
            else
            {
                /* Printer */
                if ((pia1.DDR == PIA_OUTALL) && (pia0.CR == 0x30))
                {
                    if (data != 13)
                    {
                        FILE *p;
                        p=fopen(pfname,"a");
                        fputc(data,p);
                        fclose(p);
                    }
                }
            }
        }
        else
            pia1.DDR = data;
        break;

    case 3:
        pia1.CR = data;
        break;
    }
}

byte SM_PIA_Read( word addr )
{
    //printf("%04XR: [%04X]\n",addr, CPU.PC.W);

    switch (addr & 3) {

    case 0:

        pia0.CR &= 0x3f;

        /* Keys are in an 8x8 matrix */
        switch(pia1.PDR) {
        case 0xfe:
            return KEYBD[0];
        case 0xfd:
            return KEYBD[1];
        case 0xfb:
            return KEYBD[2];
        case 0xf7:
            return KEYBD[3];
        case 0xef:
            return KEYBD[4];
        case 0xdf:
            return KEYBD[5];
        case 0xbf:
            return KEYBD[6];
        case 0x7f:
            return KEYBD[7];
        }

        return pia0.PDR;
        break;

    case 1:
        return pia0.CR;

    case 2:
        if (pia1.CR & PIA_PDR)
        {
            switch(pia0.PDR)
            {
            case 0xfd:      /* Keyboard Mux 2 */
                pia1.PDR = JOYSM[1];
                break;
            case 0xfe:      /* Keyboard Mux 1 */
                pia1.PDR = JOYSM[0];
                break;
            }
        }

        pia1.CR = pia1.CR & 0x3f;
        return pia1.PDR;
        break;

    case 3:
        if ((total_cycles - pia1.prev_cycles) < 36)
            return pia1.CR & 0x3f;

        pia1.CR = (pia1.CR & 0x3f) | ((pia1.CR & 3)<<6);
        return pia1.CR;
        break;
    }

    return 0xff;
}

