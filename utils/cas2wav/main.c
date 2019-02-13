/****************************************************************************
 * CVB2WAV
 *
 * Utility to convert a CVB file to a wave file for uploading to a real
 * creatiVision. At this point - it's completely untested!
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include "wavheader.h"

WAVEHEADER wh;

#define SHORT_LENGTH 10
#define LONG_LENGTH 19
#define SAMPLE_RATE 22050
#define SAMPLE_BITS 16

short int short_cycle[SHORT_LENGTH];
short int long_cycle[LONG_LENGTH];
int endbits = 0;

void GenTables(void)
{
    int i;
    float a;
    float PI=3.14159f;

    for(i=0; i<SHORT_LENGTH; i++)
    {
        a = (2*PI) * i / SHORT_LENGTH;
        a = sin(a);

        if ( a > 1.0f ) a = 1.0f;
        else if ( a < -1.0f ) a = -1.0f;

        a += 1.0f;
        a /= 2.0f;
        a = a * 65535.0f;
        a = a - 32768;
        a *= 0.75f;

        short_cycle[i] = (short int)a;
    }

    for(i=0; i<LONG_LENGTH; i++)
    {
        a = (2*PI) * i / LONG_LENGTH;
        a = sin(a);

        if ( a > 1.0f ) a = 1.0f;
        else if ( a < -1.0f) a = -1.0f;

        a += 1.0f;
        a /= 2.0f;
        a = a * 65535.0f;
        a = a - 32768;
        a *= 0.9f;

        long_cycle[i] = (short int)a;
    }

}

void WriteSample(unsigned char c, FILE *f)
{
    int i, mask;
    static int bits_out = 0;
    static int packet_count = 0;

    mask = 0x80;
    for ( i=0; i < 8; i++ )
    {
        if ( c & mask )
        {
            fwrite(&long_cycle, 1, LONG_LENGTH * 2, f);
        }
        else
        {
            fwrite(&short_cycle, 1, SHORT_LENGTH * 2, f);
        }

        mask >>= 1;

        bits_out++;

        if (bits_out == 8192) {
            fseek(f, 512, SEEK_CUR);
            packet_count = 0;
        }

        if (bits_out > 8192)
        {
            packet_count++;
            if (packet_count == 748)
            {
                fseek(f, 512, SEEK_CUR);
                packet_count = 0;
            }
        }

        if ( bits_out == endbits )
            fseek(f,4096,SEEK_CUR);
    }
}

int main(int argc, char *argv[])
{
    FILE *inf, *outf;
    struct stat s;
    unsigned char c;
    int fsize;

    printf("CVB2WAV Conversion Utility\n\n");
    printf("** NOTE: THIS SOFTWARE IS EXPERIMENTAL   **\n");
    printf("** IT MAY NOT PRODUCE A USABLE WAVE FILE **\n\n");

    /* Check args */
    if ( argc != 3)
    {
        puts("Usage: cas2wav infile.cas utfile.wav\n");
        return 1;
    }

    /* Does the input file exist */
    if (stat(argv[1],&s) == -1)
    {
        printf("Cannot find %s!\n", argv[1]);
        return 1;
    }

    inf = fopen(argv[1], "rb");
    if (!inf) {
        printf("Cannot open %s for reading\n",argv[1]);
        return 2;
    }

    /* Now try output file */
    outf = fopen(argv[2],"wb");
    if (!outf) {
        printf("Cannot create %s for writing\n",argv[2]);
        fclose(inf);
        return 3;
    }

    memset(&wh, 0, sizeof(wh));
    fwrite(&wh, 1, sizeof(wh), outf);
    GenTables();
    endbits = (s.st_size * 8) - 784;

    while(fread(&c, 1, 1, inf) > 0)
    {
        WriteSample(c,outf);
    }

    fsize = ftell(outf);

    /* Write wave header */
    wh.ChunkID = 0x46464952;
    wh.ChunkSize = fsize - 8;
    wh.format = 0x45564157;
    wh.subChunk1ID = 0x20746d66;
    wh.subChunk1Size = 16;
    wh.audio_format = 1;
    wh.numchannels = 1;
    wh.samplerate = SAMPLE_RATE;
    wh.bitspersample = SAMPLE_BITS;
    wh.byterate = (wh.samplerate * wh.numchannels * wh.bitspersample / 8);
    wh.blockalign = (wh.numchannels * wh.bitspersample / 8);
    wh.subChunk2ID = 0x61746164;
    wh.subChunk2Size = fsize - sizeof(WAVEHEADER);

    fseek(outf, 0, SEEK_SET);
    fwrite(&wh, 1, sizeof(WAVEHEADER),outf);

    fclose(outf);
    fclose(inf);
    return 0;
}
