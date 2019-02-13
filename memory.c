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
 * Memory Interface
 *
 * This file takes care of loading cartridges and BIOS.
 ****************************************************************************/
#include "crvision.h"

/**
 * Globals
 */
int ROMSize = 0;
int ROM_BANK[16];
int ROM_MASK[16];

/**
 * LoadBIOS
 *
 * Load BIOS ROM
 *
 * This should be 2KB, accept nothing else!
 */
int LoadBIOS( char *fname, unsigned char *buffer )
{
    FILE *inf;
    struct stat s;
    int r;
    int bsize;
    char bname[MAX_WINPATH];

    if (cslCart || salora) bsize = CSL_BIOS_SIZE;
    else bsize = BIOS_SIZE;

    if ( stat(fname, &s) == -1) {
        /* Try with BIOS path */
        strcpy(bname,"bios/");
        strcat(bname,fname);
        if (stat(bname, &s) == -1) {
            fprintf(stderr, "Invalid BIOS file name : %s\n", fname);
            return FAIL;
        } else
            strcpy(fname, bname);
    }

    if (s.st_size != bsize) {
        fprintf(stderr,"Invalid BIOS size : %d\n", (int)s.st_size);
        return FAIL;
    }

    /* Try to load the BIOS */
    inf = fopen(fname, "rb");
    if (!inf) {
        fprintf(stderr,"Unable to open %s\n", fname);
        return FAIL;
    }

    /* Load to memory buffer */
    r = fread(buffer, 1, bsize, inf);

    fclose(inf);

    if ( r != bsize ) {
        fprintf(stderr,"Cannot read BIOS - wanted %d, got %d\n",
                bsize, r);
        return FAIL;
    }

    /* Salora / Laser patches */
    if (salora)
    {
        char old_cload[]   = {0x20, 0xC3, 0xEE, 0x20, 0x46, 0xEE, 0x20, 0x69, 0xF0};
        char patch_cload[] = {0x20, 0x85, 0xEE, 0xA9, 0x5A, 0x8D, 0x5A, 0x1A, 0x60};
        char old_bload[]   = {0x20, 0xC3, 0xEE, 0x20, 0x46, 0xEE, 0x20, 0x72, 0xF0};
        char patch_bload[] = {0x20, 0x85, 0xEE, 0x20, 0x72, 0xf0, 0xA9, 0xA5, 0x8D, 0x5A, 0x1A, 0x60};
        char old_recall[]  = {0x20, 0xC3, 0xEE, 0x20, 0x46, 0xEE, 0x20, 0x57, 0xF0};
        char patch_recall[]= {0xA9, 0x55, 0x8D, 0x5A, 0x1A, 0xEA, 0xEA, 0xEA, 0xEA};

        if (!(memcmp(RAM + 0xf188, old_cload, 9)))
            memcpy(RAM + 0xf188, patch_cload, 9);

        if (!(memcmp(RAM + 0xf048, old_bload, 9)))
            memcpy(RAM + 0xf048, patch_bload, 12);

        if (!(memcmp(RAM + 0xcc8b, old_recall, 9)))
            memcpy(RAM + 0xcc8b, patch_recall, 9);
    }

    return SUCCESS;
}

/**
 * Read Page
 *
 * Support function
 */
int ROMReadPages(FILE *f, int page, int len, unsigned char *buf)
{
    int offset = page << 12;
    int r;

    r = fread(buf + offset, 1, len, f);
    if ( r != len ) {
        fprintf(stderr,"Error reading ROM file\n");
        return FAIL;
    }

    ROM_BANK[page] = 1;

    return SUCCESS;
}

/**
 * LoadROM
 *
 * Function to load a ROM cartridge
 *
 * creatiVision ROM cartridges are loaded dependent upon size.
 */
int LoadROM(char *fname, unsigned char *buffer)
{
    FILE *inf;
    struct stat s;
    int r;
    char bname[MAX_WINPATH];

    /* Clear pages and clear masks*/
    for (r = 0; r < 16; r++ ) {
        ROM_BANK[r] = 0;
        ROM_MASK[r] = 0xffff;
    }

    /* CSL Cart does not currently load roms */
    if (cslCart || salora ) {
        return SUCCESS;
    }

    if (stat(fname,&s)==-1) {
        strcpy(bname,"roms/");
        strcat(bname,fname);
        if (stat(bname,&s)==-1) {
            fprintf(stderr,"Invalid ROM file : %s\n", fname);
            return FAIL;
        } else
            strcpy(fname, bname);
    }

    ROMSize = (int)s.st_size;

    inf = fopen(fname, "rb");
    if (!inf) {
        fprintf(stderr,"Cannot open ROM : %s\n",fname);
        return FAIL;
    }

    r = FAIL;

    if (devrom)
    {
        int load_start = 0xC000 - ROMSize;
        int page_length = ROMSize % BANK_SIZE;

        while ( load_start < 0xC000 )
        {
            if (page_length)
            {
                r = fread(buffer+load_start, 1, page_length, inf);
                ROM_BANK[load_start>>12]=1;
                load_start += page_length;
                page_length = 0;
            }
            else
            {
                r = ROMReadPages(inf, load_start >> 12, BANK_SIZE, buffer);
                load_start += BANK_SIZE;
            }
        }
    }
    else
    {
        switch(ROMSize) {
        case 0x1000: /* 4K ROM */
        case 0x1800: /* 6K */
            r = ROMReadPages(inf, 0xb, BANK_SIZE, buffer);
            if ( (r == SUCCESS) && (ROMSize == 0x1800) ) {
                r = ROMReadPages(inf, 0xa, PAGE_SIZE, buffer);
                ROM_MASK[0xa]=0xf7ff;
            }
            break;

        case 0x2000: /* 8K */
        case 0x2800: /* 10K */
        case 0x3000: /* 12K */
            r = ROMReadPages(inf, 0xa, BANK_SIZE, buffer);
            if ( r == SUCCESS ) r = ROMReadPages(inf, 0xb, BANK_SIZE, buffer);
            if ( r == SUCCESS ) {
                if (ROMSize == 0x2800 ) {
                    r = ROMReadPages(inf, 0x4, PAGE_SIZE, buffer);
                    if ( r == SUCCESS ) {
                        ROM_BANK[4] = ROM_BANK[7] = 1;
                        ROM_MASK[4] = ROM_MASK[7] = 0x47ff;
                    }
                } else {
                    if (ROMSize == 0x3000)
                        r = ROMReadPages(inf, 0x4, BANK_SIZE, buffer);

                    if (r == SUCCESS) {
                        /* This is only ever needed by BASIC82 for 256 bytes! */
                        ROM_BANK[7] = 1;
                        ROM_MASK[4] = ROM_MASK[7] = 0x4fff;
                    }
                }
            }
            break;

        case 0x4000: /* 16K */
        case 0x4800: /* 18K */
            r = ROMReadPages(inf, 0xa, BANK_SIZE, buffer);
            if ( r == SUCCESS ) r = ROMReadPages(inf, 0xb, BANK_SIZE, buffer);
            if ( r == SUCCESS ) r = ROMReadPages(inf, 0x8, BANK_SIZE, buffer);
            if ( r == SUCCESS ) r = ROMReadPages(inf, 0x9, BANK_SIZE, buffer);
            if ( r == SUCCESS && ROMSize == 0x4800 ) {
                r = ROMReadPages(inf, 0x7, PAGE_SIZE, buffer);
                if ( r == SUCCESS ) ROM_MASK[7] = 0x77FF;
            }
            break;
        }
    }
    fclose(inf);
    return r;
}

/****************************************************************************
 * ROM Loading Revision 2
 *
 * Loading memory map
 *
 *               +---+---+---+---+---+
 *  4K - $B000   | 1 | 0 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *
 *               +---+---+---+---+---+
 *  6K - $B000   | 1 | 0 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *       $A000   | 1 | 0 | 1 | 0 | 0 |
 *               +---+---+---+---+---+
 *       $A800 M | 1 | 0 | 1 | 0 | M | - $F7FF MASK
 *               +---+---+---+---+---+
 *
 *               +---+---+---+---+---+
 *  8K - $A000   | 1 | 0 | 1 | 0 | 0 |
 *               +---+---+---+---+---+
 *       $B000   | 1 | 0 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *
 *               +---+---+---+---+---+
 * 10K - $A000   | 1 | 0 | 1 | 0 | 0 |
 *               +---+---+---+---+---+
 *       $B000   | 1 | 0 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *       $7000   | 0 | 1 | 1 | 1 | 0 | \
 *               +---+---+---+---+---+  |
 *       $7800 M | 0 | 1 | 1 | 1 | M |  |
 *               +---+---+---+---+---+  | $4000 - LOAD
 *       $4800 M | 0 | M | 0 | 0 | M |  | $47FF - MASK
 *               +---+---+---+---+---+  |
 *       $4000 M | 0 | M | 0 | 0 | 0 | /
 *               +---+---+---+---+---+
 *
 *               +---+---+---+---+---+
 * 12K - $A000   | 1 | 0 | 1 | 0 | 0 |
 *               +---+---+---+---+---+
 *       $B000   | 1 | 0 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *       $7000   | 0 | 1 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *       $4000 M | 0 | M | 0 | 0 | 0 | $4FFF - MASK
 *               +---+---+---+---+---+
 *
 *               +---+---+---+---+---+
 * 16K - $A000   | 1 | 0 | 1 | 0 | 0 |
 *               +---+---+---+---+---+
 *       $B000   | 1 | 0 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *       $8000   | 1 | 0 | 0 | 0 | 0 |
 *               +---+---+---+---+---+
 *       $9000   | 1 | 0 | 0 | 1 | 0 |
 *               +---+---+---+---+---+
 *
 *               +---+---+---+---+---+
 * 18K - $A000   | 1 | 0 | 1 | 0 | 0 |
 *               +---+---+---+---+---+
 *       $B000   | 1 | 0 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *       $8000   | 1 | 0 | 0 | 0 | 0 |
 *               +---+---+---+---+---+
 *       $9000   | 1 | 0 | 0 | 1 | 0 |
 *               +---+---+---+---+---+
 *       $7000   | 0 | 1 | 1 | 1 | 0 |
 *               +---+---+---+---+---+
 *       $7800 M | 0 | 1 | 1 | 1 | M | $77FF - MASK
 *               +---+---+---+---+---+
 *
 *
 * From the circuit diagram
 *
 * 74LS139 - CV - Single Chip - 2 ports
 *
 * LS139 - 1
 *
 * A15--$8000---A1--L--\
 *                      | ----> Enable LS139 - 2
 * A14--$4000---A0--L--/
 *
 * A15--$8000---A1--H--\
 *                      | ----> ROM2
 * A14--$4000---A0--L--/
 *
 * A15--$8000---A1--L--\
 *                      | ----> ROM1
 * A14--$4000---A0--H--/
 *
 * A15--$8000---A1--H--\
 *                      | ----> ROM0
 * A14--$4000---A0--H--/
 *
 * LS139 - 2
 *
 * A13--$2000---A1--L--\
 *                      | ----> RAM
 * A12--$1000---A0--L--/
 *
 * A13--$2000---A1--L--\
 *                      | ----> PIA
 * A12--$1000---A0--H--/
 *
 * A13--$2000---A1--H--\
 *                      | ----> VDP READ (CSR)
 * A12--$1000---A0--L--/
 *
 * A13--$2000---A1--H--\
 *                      | ----> VDP WRITE (CSW)
 * A12--$1000---A0--H--/
 *
 * VDP MODE line is attached to A00
 *
 * A00 == 1 == MODE 1
 * A00 == 0 == MODE 0
 *
 *  VDP Op table     CSW CSR  M
 * +----------------+---+---+---+
 * | Write register | 0 | 1 | 1 | $3001
 * | Write address  | 0 | 1 | 1 | $3001
 * |                |   |   |   |
 * | Write data     | 0 | 1 | 0 | $3000
 * |                |   |   |   |
 * | Read register  | 1 | 0 | 1 | $2001
 * | Read address   | 0 | 1 | 1 | $3001
 * |                |   |   |   |
 * | Read data      | 1 | 0 | 0 | $2000
 * +----------------+---+---+---+
 *
 ****************************************************************************/
