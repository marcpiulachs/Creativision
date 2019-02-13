/****************************************************************************
 *                                   \ /
 *                        C R E A T I V I S I O N
 *                            E M U L A T O R
 *                                  0.2.3
 *
 *       This emulator was written using Marat Fayzullin's Emulib
 *
 *       *YOU MAY NOT DISTRIBUTE THIS FILE COMMERCIALLY. PERIOD!*
 *---------------------------------------------------------------------------
 * CPU Interface
 *
 * This file contains the required code to interface with the M6502 CPU from
 * EmuLIB.
 *
 * 17 Nov 2013      Lock down RAM to 1KB
 ****************************************************************************/
#include "crvision.h"
#include "netplay.h"

static unsigned int prev_ticks = 0;
int tmsirq_pending = 0;

/**
 * Rd6502Salora
 *
 * Memory read functions for Salora / Laser
 */
byte Rd6502Salora(register word addr)
{
    switch(addr & 0xf000)
    {
    case 0x1000:
        if((addr & 0xff00) == 0x1100) return laserdos_read(addr);
        if (addr > 0x11ff) return RAM[addr];
        return SM_PIA_Read(addr);

    case 0x2000:
        if (addr == 0x2000) return RdData9918(&VDP);
        if (addr == 0x2001) return RdCtrl9918(&VDP);
        if (addr > 0x20ff) return RAM[addr];
        break;

    default:
        return RAM[addr];
    }

    if ((addr!=0x28A9) && (addr!=0x2CA9))
        fprintf(stderr,"BR: %04X [%04X]\n", addr, CPU.PC.W);

    return NORAM;

}

/**
 * Rd6502CSL
 *
 * Memory read functions for CSLCart
 */
byte Rd6502CSL(register word addr)
{
    switch(addr & 0xF000)
    {
    case 0x1000:    /* PIA */
        return PIA_Read(addr);

    case 0x2000:    /* VDP Read */
        if (addr == 0x2000) return RdData9918(&VDP);
        if (addr == 0x2001) return RdCtrl9918(&VDP);
        break;

    default:        /* RAM or ROM */
        return RAM[addr];
    }

    /* CSL uses BIT at $D010 and $D013 */
    if ((addr!=0x28A9) && (addr!=0x2CA9))
        fprintf(stderr,"BR: %04X [%04X]\n", addr, CPU.PC.W);

    return NORAM;
}

/**
 * Rd6502
 *
 * Function to read bytes for M6502 core
 */
byte Rd6502(register word addr)
{
    if (cslCart) return Rd6502CSL(addr);
    if (salora) return Rd6502Salora(addr);

    /* Is it RAM? */
    if ( addr < 0x400 )
        return RAM[addr & 0x3ff];

    /* Is it I/O */
    switch (addr & 0xf000) {
    case 0x1000:
        return PIA_Read(addr);


    case 0x2000:   /* VDP */
        if (addr == 0x2000) {
            if (vdp_debug) VDP_ShowRegs(0);
            return RdData9918(&VDP);
        } else {
            if (addr == 0x2001) {
                if (vdp_debug) VDP_ShowRegs(1);
                return RdCtrl9918(&VDP);
            }
        }
        break;

    case 0xe000:
        return ReadCentronics(addr);
    }


    /* Is it ROM? */
    if (ROM_BANK[addr >> 12]) {
        return RAM[addr & ROM_MASK[addr>>12]];
    }

    /* Is it BIOS */
    if ( addr > 0xf7ff )
        return RAM[addr];

    fprintf(stderr,"BR: %04X [%04X]\n", addr, CPU.PC.W);
    return NORAM;
}

/**
 * Wr6502Salora
 *
 * Memory write function for Salora / Laser
 */
void Wr6502Salora(register word addr, register byte data)
{
    switch(addr & 0xf000)
    {
    case 0x1000:
        if ((addr & 0xff00)==0x1100) { laserdos_write(addr, data); return; }
        SM_PIA_Write(addr, data);
        return;

    case 0x3000:
        if (addr==0x3000) {
            WrData9918(&VDP, data);
            return;
        }

        if (addr==0x3001) {
            WrCtrl9918(&VDP,data);
            return;
        }
        break;

    case 0x0000:
        RAM[addr] = RAM[addr+0x4000] = RAM[addr+0x8000] = data;
        return;
    case 0x4000:
        RAM[addr & 0xfff] = RAM[addr] = RAM[addr + 0x4000] = data;
        return;
    case 0x8000:
        RAM[addr & 0xfff] = RAM[(addr & 0xfff)+0x4000] = RAM[addr] = data;
        return;

    case 0x5000:
    case 0x6000:
    case 0x7000:
    case 0x9000:
    case 0xA000:
    case 0xB000:
        RAM[addr] = data;
        return;
    }

    if (addr != 0xC000)
        fprintf(stderr,"BW: %04X %02X [%04X]\n",addr,data,CPU.PC.W);
}

/**
 * Wr6502CSL
 *
 * Memory write functions for CSL cart
 */
void Wr6502CSL(register word addr, register byte data)
{
    switch(addr & 0xf000)
    {
    case 0x1000:    /* PIA */
        PIA_Write(addr,data);
        return;

    case 0x3000:    /* VDP */
        if (addr == 0x3000) {
            WrData9918(&VDP, data);
            return;
        }
        if (addr == 0x3001) {
            WrCtrl9918(&VDP, data);
            return;
        }
        break;

    case 0x0000:    /* RAM */
    case 0x4000:
    case 0x5000:
    case 0x6000:
    case 0x7000:
    case 0x8000:
    case 0x9000:
    case 0xA000:
    case 0xB000:
        RAM[addr]=data;
        return;
    }

    /* CSL writes to $C000 when sizing memory */
    if (addr != 0xC000)
        fprintf(stderr,"BW: %04X %02X [%04X]\n",addr,data,CPU.PC.W);
}

/**
 * Wr6502
 *
 * Function to write bytes for M6502 core
 */
void Wr6502(register word addr, register byte data)
{
    if (cslCart) {
        Wr6502CSL(addr,data);
        return;
    }

    if (salora) {
        Wr6502Salora(addr,data);
        return;
    }

    /* Is it RAM? */
    if (addr < 0x400) {
        RAM[addr & 0x3ff] = data & 0xff;
        return;
    }

    /* Is it I/O */
    switch (addr & 0xf000) {

    case 0x1000:    /* PIA */
        PIA_Write(addr, data);
        return;

    case 0x3000:    /* VDP */
        if ( addr == 0x3000 ) {
            WrData9918(&VDP, data);
            if ( vdp_debug ) VDP_ShowRegs(2);
        } else {
            if ( addr == 0x3001 ) {
                if (WrCtrl9918(&VDP, data))
                    tmsirq_pending = 1;
                if ( vdp_debug ) VDP_ShowRegs(3);
            }
        }
        return;

    case 0xe000:    /* Centronics */
        WriteCentronics(addr, data);
        return;
    }

    fprintf(stderr,"BW: %04X %02X [%04X]\n",addr,data,CPU.PC.W);
}

/**
 * Patch6502
 *
 * Called when an illegal opcode is hit by M6502 core
 */
byte Patch6502(register byte opcode, register M6502 *R)
{
    /* For now - just run for the hills!!!! */
    fprintf(stderr, "Illegal Opcode : %02X @ %04X\n", opcode, R->PC.W);
    exit(-1);
}

/**
 * Loop6502
 *
 * Called to determine if an interrupt is pending
 */
byte Loop6502(register M6502 *R)
{
    if (EmuQuit==2) {
        sleep(5);
        return INT_QUIT;
    }

    unsigned int curr_ticks;
    int irq=INT_NONE;

    if (Loop9918(&VDP)) irq = INT_IRQ;

    if (tmsirq_pending) {
        irq = INT_IRQ;
        tmsirq_pending = 0;
    }

    if (demo_record) {
        if (VDP.Line == 156) {
#ifndef RPI
            SN76496SPUpdate(0,&playbuffer[0], 220);
#endif
        }
    }

    /* At end of frame ... */
    if (VDP.Line != TMS9918_END_LINE) return irq;

    /* Check keyboard */
    if (salora)
    {
        if (check_input_salora()) return INT_NMI;
    }
    else
    {
        if (np_player) {
            if (NP_check_input()) {NP_RemoteKeys(); return INT_NMI;}
            if (NP_RemoteKeys() == 999) return INT_NMI;
        } else {
            if (check_input()) return INT_NMI;
        }
    }

    if (EmuQuit) return INT_QUIT;

    /* 1 full frame is done ... so show it */
    do {
        curr_ticks = SDL_GetTicks() - prev_ticks;
    } while ( curr_ticks < 20 );
    prev_ticks = SDL_GetTicks();

    /* Demo recording */
    if (demo_record && avi) {
        RecordFrame();
#ifndef RPI
        SN76496SPUpdate(0,&playbuffer[220], 221);
#endif
        CHUNK c;
        if (avi) {
            c.id = 0x62773130;
            c.length = 882;
            if (fwrite(&c, 1, sizeof(CHUNK), avi) != sizeof(CHUNK)) {
                fprintf(stderr,"!!! Error writing AVI data\n");
                fclose(avi);
                avi = NULL;
            } else {
                if (fwrite(&playbuffer[0], 1, c.length, avi) != c.length ) {
                    fprintf(stderr,"!!! Error writing AVI data\n");
                    fclose(avi);
                    avi = NULL;
                }
            }
        }
    }

    return INT_IRQ; /* One VBlank per full frame */
}
