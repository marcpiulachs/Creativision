/****************************************************************************
 * CVGFX
 *
 * Inspired by the Atari VCS 2600 ShowGFX / EditGFX utilities.
 * This utility will dump a cartridge to a text file for editing.
 * You can then rebuild the cartridge from the edited file.
 * DO NOT ADD OR REMOVE LINES FROM THE OUTPUT FILE!
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define FBINARY 1
#define FTEXT   2

static char *buffer = NULL;
int flen = 0;

void cvttotext(char *f)
{
    FILE *of;
    int i;
    int b, mask;
    char obuf[24];

    of = fopen(f,"w");
    if (!f) {
        printf("Cannot open %s for writing\n",f);
        return;
    }

    for( i = 0; i < flen; i++ ) {
        mask = 0x80;
        sprintf(obuf,"%04X [        ]\n", i);

        for ( b = 0; b < 8; b++ ) {
            if ( buffer[i] & mask )
                obuf[b+6] = 'X';

            mask >>= 1;
        }

        fputs(obuf,of);
    }

    fclose(of);

    printf("Output converted to text in %s\n",f);
}

void cvttobinary(char *f)
{
    FILE *of;
    int t;
    char *p;
    int b,mask;
    int c;
    int obytes = 0;

    /* Make short strings again */
    for ( t = 0; t < flen; t++ ) {
        if (buffer[t] == 10)
            buffer[t] = 0;
    }
    buffer[t] = 0;

    of = fopen(f,"wb");
    if (!of) {
        printf("Cannot open %s for writing\n",f);
        return;
    }

    p = buffer;
    while (*p) {
        mask = 0x80;
        c = 0;
        for(b=0; b<8; b++) {
            if (p[6+b] == 'X') {
                c |= mask;
            }

            mask >>= 1;
        }

        if (fwrite(&c, 1, 1, of) != 1) {
            printf("Error writing output to %s\n",f);
            fclose(of);
            return;
        }

        obytes++;
        p += strlen(p)+1;
    }

    fclose(of);
    printf("Written %d to binary file %s\n", obytes, f);
}

int loadfile( char *f )
{
    struct stat s;
    FILE *i;
    int t;
    char vchars[] = { 'A', 'B', 'C', 'D', 'E', 'F', '0', '1',
                      '2', '3', '4', '5', '6', '7', '8', '9',
                      'X', '[', ']', 32, 13, 10, 0
                    };

    if ( stat(f, &s) != -1 ) {
        if (s.st_size) {
            flen = s.st_size;

            i = fopen(f,"rb");
            if (!i) {
                printf("Cannot open %s for reading\n",f);
                return 0;
            }

            buffer = malloc(s.st_size + 4);

            if ( fread(buffer, 1, s.st_size, i) != s.st_size ) {
                printf("Error reading %s\n",f);
                fclose(i);
                return 0;
            }

            fclose(i);

            printf("Loaded %d bytes from %s\n", flen, f);

            for(t = 0; t < s.st_size; t++) {
                if ( !strchr(vchars, buffer[t]) ) {
                    return FBINARY;
                }
            }

            return FTEXT;
        } else {
            printf("%s is a zero length file!\n",f);
            return 0;
        }
    }

    return 0;
}

int main( int argc, char *argv[] )
{
    int ft;

    if ( argc != 3 ) {
        puts("Usage: cvgfx infile outfile");
        return 0;
    }

    ft = loadfile(argv[1]);
    switch(ft) {
    case FBINARY:
        cvttotext(argv[2]);
        break;
    case FTEXT:
        cvttobinary(argv[2]);
        break;
    default:
        break;
    }

    if (buffer) free(buffer);

    return 0;
}
