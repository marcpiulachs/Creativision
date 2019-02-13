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
 * Snapshot.c
 *
 * Save and load snapshots of a session
 ****************************************************************************/
#include "crvision.h"

extern SN76496SP psg;

/**
 * SnapErrorWriting
 */
void SnapErrorWriting(gzFile f)
{
    fprintf(stderr,"!!! Error writing snapshot file\n");
    gzclose(f);
}

/**
 * SnapErrorReading
 */
void SnapErrorReading(gzFile f)
{
    fprintf(stderr,"!!! Error reading snapshot file\n");
    gzclose(f);
}

/**
 * SaveSnapShot
 *
 * Format
 *  M6502
 *  total_cycles
 *  1K RAM
 *  VDP
 *  16K VRAM
 *  PSG
 *  BANKS
 *  BANK DATA
 *  BIOS
 *  KEYS
 */
void SaveSnapShot( void )
{
    gzFile snap;
    char sname[MAX_WINPATH];
    char *t;

    strcpy(sname, rname);
    t = strrchr(sname,'.');
    if (t) *t=0;

    strcat(sname,".snp");

    snap = gzopen(sname,"w9");
    if (!snap) {
        fprintf(stderr,"!!! Cannot create snapshot file\n");
        return;
    }

    /* M6502 */
    if (gzwrite(snap, &CPU, sizeof(CPU)) != sizeof(CPU)) {
        SnapErrorWriting(snap);
        return;
    }

    /* Total cycle count */
    if (gzwrite(snap, &total_cycles, 8)!=8) {
        SnapErrorWriting(snap);
        return;
    }

    if (cslCart || salora) {
        if ( gzwrite(snap, RAM, 0xC000) != 0xC000) {
            SnapErrorWriting(snap);
            return;
        }
    }
    else
    {
        /* RAM */
        if (gzwrite(snap, RAM, 0x400) != 0x400) {
            SnapErrorWriting(snap);
            return;
        }
    }

    /* VDP */
    if (gzwrite(snap, &VDP, sizeof(VDP)) != sizeof(VDP)) {
        SnapErrorWriting(snap);
        return;
    }

    /* VRAM */
    if (gzwrite(snap, VDP.VRAM, 0x4000) != 0x4000) {
        SnapErrorWriting(snap);
        return;
    }

    /* PSG */
    if (gzwrite(snap, &psg, sizeof(psg)) != sizeof(psg)) {
        SnapErrorWriting(snap);
        return;
    }

    /* Keys */
    if (gzwrite(snap, &KEYBD, 8) != 8) {
        SnapErrorWriting(snap);
        return;
    }

    gzclose(snap);

    fprintf(stdout,"%c Snapshot successfully saved to %s\n", 254, sname);

}

/**
 * LoadSnapShot
 *
 * Same as above - in reverse :)
 */
void LoadSnapShot(void)
{
    gzFile snap;
    byte *v1,*v2;
    int offsets[5];
    char sname[MAX_WINPATH];
    char *t;

    strcpy(sname, rname);
    t = strrchr(sname,'.');
    if (t) *t=0;

    strcat(sname,".snp");

    snap = gzopen(sname,"r");
    if (!snap) {
        fprintf(stderr,"!!! Cannot open snapshot file\n");
        return;
    }

    /* M6502 */
    if (gzread(snap, &CPU, sizeof(CPU)) != sizeof(CPU)) {
        SnapErrorReading(snap);
        return;
    }

    /* Total cycle count */
    if (gzread(snap,&total_cycles, 8)!=8) {
        SnapErrorReading(snap);
        return;
    }

    /* RAM */
    if (cslCart || salora)
    {
        if (gzread(snap, RAM, 0xc000) != 0xc000) {
            SnapErrorReading(snap);
            return;
        }
    }
    else
    {
        if (gzread(snap, RAM, 0x400) != 0x400) {
            SnapErrorReading(snap);
            return;
        }
    }

    v1 = VDP.VRAM;
    v2 = VDP.XBuf;

    /* VDP */
    if (gzread(snap, &VDP, sizeof(VDP)) != sizeof(VDP)) {
        SnapErrorReading(snap);
        return;
    }

    /* Calc old offsets */
    offsets[0] = VDP.ChrTab - VDP.VRAM;
    offsets[1] = VDP.ChrGen - VDP.VRAM;
    offsets[2] = VDP.SprTab - VDP.VRAM;
    offsets[3] = VDP.SprGen - VDP.VRAM;
    offsets[4] = VDP.ColTab - VDP.VRAM;

    /* Set for new */
    VDP.VRAM = v1;
    VDP.XBuf = v2;

    VDP.ChrTab = VDP.VRAM + offsets[0];
    VDP.ChrGen = VDP.VRAM + offsets[1];
    VDP.SprTab = VDP.VRAM + offsets[2];
    VDP.SprGen = VDP.VRAM + offsets[3];
    VDP.ColTab = VDP.VRAM + offsets[4];

    /* VRAM */
    if (gzread(snap, VDP.VRAM, 0x4000) != 0x4000) {
        SnapErrorReading(snap);
        return;
    }

    /* PSG */
    if (gzread(snap, &psg, sizeof(psg)) != sizeof(psg)) {
        SnapErrorReading(snap);
        return;
    }

    /* Keys */
    if (gzread(snap, &KEYBD, 8) != 8) {
        SnapErrorReading(snap);
        return;
    }

    gzclose(snap);

    fprintf(stdout,"%c Snapshot successfully loaded from %s\n", 254, sname);

}
