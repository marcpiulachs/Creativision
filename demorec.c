/****************************************************************************
 *                                   \ /
 *                        C R E A T I V I S I O N
 *                            E M U L A T O R
 *                                  0.2.1
 *
 *       This emulator was written using Marat Fayzullin's Emulib
 *
 *       *YOU MAY NOT DISTRIBUTE THIS FILE COMMERCIALLY. PERIOD!*
 *---------------------------------------------------------------------------
 * demorec.c
 *
 * This module records AVI files,
 ****************************************************************************/
#include "crvision.h"
#include "riff.h"

FILE *avi = NULL;

/**
 * AVIError
 *
 * One place to close files etc
 */
void AVIError()
{
    fprintf(stderr,"!!! Error writing AVI\n");
    fclose(avi);
    avi = NULL;
}

/**
 * OpenAVI
 *
 * Opens the global AVI file and writes default header.
 * Header was taken from VirtualDUB output.
 */
void OpenAVI(void)
{
    if (!avi) {
        avi = fopen("demo.avi","wb+");
        if (!avi) return;
    }

    /* Put out standard header */
    if (fwrite(aviheader, 1, aviheader_size, avi) != aviheader_size ) {
        AVIError();
    }
}

/**
 * RecordFrame
 *
 * Record a single frame to the AVI.
 */
void RecordFrame(void)
{
    int y,x;
    short line[VDP_WIDTH];
    int *buf = (int *)VRAM;
    int c;
    int r,g,b;
    CHUNK id;

    if (!avi) return;

    id.id = 0x62443030;
    id.length = VDP_RAM_SIZE >> 1;


    if (fwrite(&id,1,sizeof(CHUNK),avi) != sizeof(CHUNK)) {
        AVIError();
        return;
    }

    for(y=VDP_HEIGHT-1; y >=0; y--) {
        for(x=0; x<VDP_WIDTH; x++) {
            //DIB RGB555 */
            c = buf[(y*VDP_WIDTH)+x];
            r = (c & 0xf80000) >> 9;
            g = (c & 0xf800) >> 6;
            b = (c & 0xf8) >> 3;

            line[x] = r | g | b;
        }

        if (fwrite(&line, 1, VDP_WIDTH << 1, avi) != (VDP_WIDTH << 1) ) {
            AVIError();
            break;
        }

    }
}

/**
 * CloseAVI
 *
 * Finalise the AVI file
 */
void CloseAVI(void)
{
    int movi2idx = 0;
    int curpos = 0;
    CHUNK c;
    IDX idx;
    int prev_offset = 4;
    int endidx;
    int l,totalframes = 0;

    if (!avi) return;

    fprintf(stdout,"%c Generating AVI Index ...", 254);
    fflush(stdout);

    movi2idx = ftell(avi);

    /* Output IDX header */
    c.id = 0x31786469;
    c.length = 0;
    if (fwrite(&c, 1, sizeof(CHUNK), avi) != sizeof(CHUNK)) {
        AVIError();
        return;
    }

    /* Rewind to start of machine */
    curpos = aviheader_size;

    while (curpos < movi2idx) {
        fseek(avi, curpos, SEEK_SET);
        if (fread(&c, 1, sizeof(CHUNK), avi) != sizeof(CHUNK)) {
            AVIError();
            return;
        }

        curpos = ftell(avi) + c.length;

        if (c.id == 0x62643030) totalframes++;

        /* Update Index */
        fseek(avi, 0, SEEK_END);
        idx.id = c.id;
        idx.length = c.length;
        idx.offset = prev_offset;
        idx.flags = 16;
        prev_offset += (c.length + 8);
        if (fwrite(&idx, 1, sizeof(IDX), avi) != sizeof(IDX)) {
            AVIError();
            return;
        }
    }

    endidx = ftell(avi);

    /* Update header */
    endidx -= 8;
    fseek(avi, 4, SEEK_SET);
    if (fwrite(&endidx, 1, 4, avi) != 4 ) {
        AVIError();
        return;
    }

    fseek(avi, 48, SEEK_SET);
    if (fwrite(&totalframes, 1, 4, avi) != 4 ) {
        AVIError();
        return;
    }

    fseek(avi, 140, SEEK_SET);
    if (fwrite(&totalframes, 1, 4, avi) != 4 ) {
        AVIError();
        return;
    }

    fseek(avi, aviheader_size - 8, SEEK_SET);
    l = movi2idx - aviheader_size + 4;
    if (fwrite(&l, 1, 4, avi) != 4) {
        AVIError();
        return;
    }

    movi2idx += 4;
    fseek(avi, movi2idx, SEEK_SET);
    movi2idx = (endidx - movi2idx) + 4;
    if (fwrite(&movi2idx, 1, 4, avi) != 4) {
        AVIError();
        return;
    }

    fclose(avi);

    fprintf(stdout," Done\n");
}
