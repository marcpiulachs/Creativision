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
 * Cassette Interface
 *
 * This is an extension to the PIA routines. When PIA 0 is set all BITs out,
 * or high BIT only in, it looks like it's communicating with the cassette.
 *
 * CSAVE
 *
 * Protocol.
 *
 *  Header          1024 bytes 0
 *  Packet Header   13 x 0x10000 + 0x1110110
 *  Packet          64 byte packet $26F - $230 - so yes, it's backwards!
 *  Checksum        1 byte checksum of bytes in packet + 1 byte 0
 *
 *  EOT 2x
 *  Packet Header   13 x 0x10000 + 0x1110110
 *  Data of Nulls   64 bytes zero
 *  Checksum        Total of paacket data
 *  Terminator      0 byte
 *
 * Interestingly, CLOAD does not read the final packet tail.
 ****************************************************************************/
#include "crvision.h"

FILE *cassette=NULL;
static int lastop = 0;

#define BITON_CYCLES     700
#define BITON_CYCLES_CSL 800
#define BITOFF_CYCLES    400

const char *errmsgr[] = {"","ERROR","","LOAD FILE DOES NOT EXIST","","TERMINATING",""};
const char *errmsgw[] = {"","ERROR","","CANNOT CREATE OUTPUT FILE","","TERMINATING",""};
const char *errwrite[] = {"","ERROR","","CANNOT WRITE TO OUTPUT","", "TERMINATING","" };
const char *errread[] = {"","ERROR","","BAD READ FROM INPUT FILE", "", "TERMINATING",""};
static int msgup = 0;

/**
 * CLOADBIT
 *
 * Bits are set to 0x80 for so many cycles, then 0x00.
 */
typedef struct {
    int out;
    int cycles;
    int flips;
    unsigned long long int prev_cycles;
} CLOADBIT;

CLOADBIT cbit;

char cfname[MAX_WINPATH];

/**
 * WriteBIT
 *
 * Write out a bit of data to a file
 */
void WriteBIT(int v)
{
    static unsigned char bit = 0;
    static int pos = 0;

    if ( v == 256 ) {
        if (pos) {
            if (cassette && (lastop == 2)) {
                fwrite(&bit,1,1,cassette);
                bit = pos = 0;
            }
            return;
        }
    }

    bit <<= 1;
    if (v) bit |= 1;
    pos++;
    if (pos == 8) {
        if (lastop == 1) {
            fclose(cassette);
            cassette = NULL;
        }
        if (!cassette) cassette = fopen(cfname,"ab+");
        lastop = 2;
        if (cassette)  {
            if (fwrite(&bit, 1, 1, cassette) != 1)
            {
                vdp_errmsg(errwrite,7);
                msgup++;
            }
        }
        else {
            if (msgup == 0) {
                fprintf(stderr,"%c Cannot open %s for writing\n",254,cfname);
                SaveSnapShot();
                vdp_errmsg(errmsgw,7);
                msgup++;
            }
        }
        pos = bit = 0;
    }
}

/**
 * ReadBIT
 *
 * Return next bit
 */
static void ReadBIT( void )
{
    static int mask = 0;
    static int c = 0;
    int bcycles;

    if (cslCart || salora) {
        bcycles = BITON_CYCLES_CSL;
        cloadfast = 0;
    }
    else bcycles = BITON_CYCLES;

    /* Open an input file */
    if (!cassette) {
        cassette = fopen(cfname, "rb");
        if (!cassette) {
            if (msgup==0) {
                fprintf(stderr, "%c Cannot open %s for reading\n",254,cfname);
                vdp_errmsg(errmsgr,7);
                msgup++;
            }
            return;
        } else {
            if(lastop==2) {
                rewind(cassette);
            }
            mask = c = 0;
            if (cloadfast)
                fseek(cassette,1024,SEEK_SET); /* Skip header */
        }
    }

    if (!cassette) return;

    if (mask == 0) {
        mask = 0x80;
        if (fread(&c, 1, 1, cassette) != 1) {
            fseek(cassette, 0, SEEK_SET);
            return;
        }
    }

    if (mask & c) {
        cbit.out = 0x80;
        cbit.flips = 0;
        cbit.cycles = (cloadfast)?bcycles/2:bcycles;
        cbit.prev_cycles = total_cycles;
    } else {
        cbit.out = 0x80;
        cbit.flips = 0;
        cbit.cycles = (cloadfast)?BITOFF_CYCLES/2:BITOFF_CYCLES;
        cbit.prev_cycles = total_cycles;
    }

    lastop = 1;
    mask >>= 1;
}

/**
 * CSAVE
 *
 * Really, it's the only time this will ever be called!
 * The stars will be aligned when PIA-0 is set to all bits out.
 *
 * I confess this is fully contrived. From reading the BASIC disassembly,
 * the bit on / bit off toggle is based on a delay value in register A.
 */
void CSAVE( int data )
{
    int cpua = 0x70;
    int cpub = 0x40;

    if (cslCart) {
        cpua = 0x91;
        cpub = 0x49;
    }

    /* Only interesting in high bit */
    if ( data == 0x80 ) {
        if ( CPU.A == cpua ) { /* Delay value for ON */
            WriteBIT(1);
        } else {
            if ( CPU.A == cpub ) { /* Delay value for OFF */
                WriteBIT(0);
            } else
                fprintf(stderr,"Sorry, the droids are not with us today!\n");
        }
    }
}

void SM_CSAVE( void )
{
    if ( (CPU.A == 0x80) || (CPU.A == 0x90) ) WriteBIT(1);
    else {
        if (CPU.A == 0x40 ) WriteBIT(0);
    }
}

/**
 * CLOAD
 *
 * Complimentary function to the above.
 * Simply feeds bits till it runs out of data.
 *
 */
unsigned char CLOAD(void)
{
    if (cbit.prev_cycles == 0) {
        ReadBIT();
    }

//0.2.2
//Guard wrap around
    if ( total_cycles < cbit.prev_cycles) cbit.prev_cycles = total_cycles;

    if ( (total_cycles - cbit.prev_cycles) > cbit.cycles ) {
        cbit.flips++;
        if (cbit.flips == 1) {
            cbit.out ^= 0x80;
            cbit.cycles <<= 1;
        } else {
            if (cbit.flips == 2) ReadBIT();
        }
    }

    return cbit.out;

}

/****************************************************************************
 * Salora Manager / Laser 2001 loading routines
 *
 * This simply replaces the load functions of the BIOS routines
 ****************************************************************************/
static int SM_SKIPHEADER()
{
    unsigned int id = 0x01A5995A;
    unsigned int rid = 0;
    int c;

    if (!cassette) {
        cassette = fopen(cfname,"rb");
        if (!cassette)
        {
            if (!msgup) vdp_errmsg(errmsgr,7);
            msgup++;
            return 0;
        }
    }

    /* Skip possible tail bytes from previous read */
    fseek(cassette, 10, SEEK_CUR);

    while ((c=fgetc(cassette)) >= 0)
    {
        rid <<= 8;
        rid |= (c & 0xff);

        if (rid == id)
            break;
    }

    return rid == id;
}

void SM_CLOAD()
{
    unsigned short load_address, length, lrc;

    if (SM_SKIPHEADER())
    {
        /* Header found - load file */
        if (fread(&load_address, 1, 2, cassette) != 2) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }

        if (fread(&length, 1, 2, cassette) != 2) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }

        if (fread(&lrc, 1, 2, cassette) != 2) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }

        if (fread(RAM + load_address, 1, length, cassette) != length) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }

        RAM[0x69] = RAM[0xaf] = lrc & 0xff;
        RAM[0x6a] = RAM[0xb0] = (lrc & 0xff00) >> 8;
    }
}

void SM_BLOAD(void)
{
    unsigned short load_address, length;

    if (SM_SKIPHEADER())
    {
        if (fread(&load_address, 1, 2, cassette) != 2) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }

        if (fread(&length, 1, 2, cassette) != 2) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }

        if ((RAM[0xe8]!=0) || (RAM[0xe9]|=0))
            load_address = (RAM[0xe9]<<8) | RAM[0xe8];

        if (fread(RAM + load_address, 1, length, cassette) != length) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }
    }
}

void SM_RECALL(void)
{
    unsigned short load_address, length;

    if (SM_SKIPHEADER())
    {
        if (fread(&load_address, 1, 2, cassette) != 2) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }

        if (fread(&length, 1, 2, cassette) != 2) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }

        if (fread(RAM + load_address, 1, length, cassette) != length) {
            if (!msgup) vdp_errmsg(errread,7);
            msgup++;
        }
    }
}
