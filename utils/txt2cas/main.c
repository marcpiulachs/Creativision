/****************************************************************************
 * TXT2CAS
 *
 * Utility to convert a creatiVision BASIC text file to CAS format
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define APP_TITLE "creatiVision TXT2CAS Conversion Utility"
#define APP_VERSION "0.2"

const char *version = APP_TITLE " " APP_VERSION;
const char *builton = "Built on " __DATE__ " " __TIME__;

char work[2048];
char line[1024];
char packet[64];

void WriteBIT(int bit, FILE *f)
{
    static int bits = 0;
    static char out = 0;

    if ( bits == 8 )
    {
        fwrite(&out, 1, 1, f);
        bits = 0;
        out = 0;
    }

    out <<= 1;
    out |= bit;
    bits++;
}

void WritePacketHeader(FILE *f)
{
    int i, j;

    for ( j=0; j < 13; j++ )
    {
        WriteBIT(1,f);
        for( i = 0; i < 16; i++ )
            WriteBIT(0,f);
    }

    /* Trailing value */
    WriteBIT(1,f);
    WriteBIT(1,f);
    WriteBIT(1,f);
    WriteBIT(0,f);
    WriteBIT(0,f);
    WriteBIT(1,f);
    WriteBIT(1,f);
}

void WritePacket(FILE *f)
{
    unsigned char checksum = 0;
    int i,j,mask;

    for ( i = 63; i >= 0; i--)
    {
        checksum += packet[i];

        mask = 0x80;
        for(j=0; j<8; j++)
        {
            if ( packet[i] & mask )
                WriteBIT(1,f);
            else
                WriteBIT(0,f);
            mask >>= 1;
        }
    }

    mask = 0x80;
    for (j=0; j<8; j++)
    {
        if (checksum & mask) WriteBIT(1,f);
        else WriteBIT(0,f);
        mask >>= 1;
    }

    for (j=0; j<8; j++)
        WriteBIT(0,f);
}

int main( int argc, char *argv[] )
{
    FILE *inf, *outf;
    struct stat s;
    int c;
    char *p;

    printf("%c %s\n%c %s\n", 254, version, 254, builton);

    if (argc != 3) {
        printf("%c\n%c Usage: txt2cas infile.txt outfile.cas",254,254);
        return 0;
    }

    /* Try to open input */
    if ( stat(argv[1],&s) == -1 )
    {
        printf("%c Cannot find %s\n", 254, argv[1]);
        return 2;
    }

    inf = fopen(argv[1],"r");
    if (!inf) {
        printf("%c Cannot open input file %s\n", 254, argv[1]);
        return 1;
    }

    /* Read the input file, checking for unusable characters */
    while ((c=fgetc(inf)) > 0)
    {
        if ( c > 127 )
        {
            printf("%c File contains invalid BASIC characters - aborted\n", 254);
            fclose(inf);
            return 4;
        }
    }

    rewind(inf);

    memset(packet, 32, 64);

    outf = fopen(argv[2],"wb");
    if (!outf) {
        printf("%c Cannot create %s for writing\n", 254, argv[2]);
        fclose(inf);
        return 6;
    }

    /* Place header packet in file */
    fseek(outf, 1024, SEEK_SET);

    while (fgets(line, 1024, inf) > 0)
    {
        p = strtok(line,"\n");
        if (!p) continue;

        c = strlen(p);
        if (c) {
            while(*p == 32) p++;
        }

        c = strlen(p);
        if (c)
        {
            while (p[c] == 32) p[c--]=0;
        }

        if (c) p[c] = 0;
        c = strlen(p);

        if (c)
        {
            if (c > 64)
            {
                printf("%c Line exceeds maximum packet length - aborted!\n",254);
                fclose(inf);
                return 5;
            }

            /* Set packet buffer output */
            c = 0;
            memset(packet, 0, 64); /* Confirmed 0 with BASIC disassembly */
            while ( c < strlen(p))
            {
                if ( ( p[c] == 13 ) || (p[c] == 0 )) break;

                packet[c] = p[c];
                c++;
            }

            packet[c]=13;
            printf("[%s]\n", p);
            WritePacketHeader(outf);
            WritePacket(outf);
        }
    }

    /* Add line terminator */
    WritePacketHeader(outf);
    memset(packet,0,64);
    WritePacket(outf);

    WritePacketHeader(outf);
    WritePacket(outf);

    fclose(inf);
    fclose(outf);

    return 0;
}
