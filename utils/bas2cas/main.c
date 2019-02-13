#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * MSBASIC Tokens
 */
struct reserved {
    char *name;
    uint8_t token;
};

#define MAX_TOKEN 94

struct reserved tokens[] = {
    {"END", 0x80},
    {"FOR", 0x81},
    {"NEXT", 0x82},
    {"DATA", 0x83},
    {"INPUT", 0x84},
    {"DISK", 0x85},
    {"DIM", 0x86},
    {"READ", 0x87},
    {"GR", 0x88},
    {"TEXT", 0x89},
    {"BRUN", 0x8a},
    {"CRUN", 0x8b},
    {"CALL", 0x8c},
    {"PLOT", 0x8d},
    {"RECT", 0x8e},
    {"CIRCLE", 0x8f},
    {"SOUND", 0x90},
    {"MUSIC", 0x91},
    {"BLOAD", 0x92},
    {"UNPLOT", 0x93},
    {"BSAVE", 0x94},
    {"SGEN", 0x95},
    {"HOME", 0x97},
    {"RECALL", 0x99},
    {"VPOKE", 0x9a},
    {"TRACE", 0x9b},
    {"NOTRACE", 0x9c},
    {"FLASH", 0x9f},
    {"COLOR=", 0xa0},
    {"ONERR", 0xa5},
    {"LPRINT", 0xa6},
    {"LLIST", 0xa7},
    {"STORE", 0xa8},
    {"LET", 0xaa},
    {"GOTO", 0xab},
    {"RUN", 0xac},
    {"IF", 0xad},
    {"RESTORE", 0xae},
    {"&", 0xaf},
    {"GOSUB", 0xb0},
    {"RETURN", 0xb1},
    {"REM", 0xb2},
    {"STOP", 0xb3},
    {"ON", 0xb4},
    {"WAIT", 0xb5},
    {"CLOAD", 0xb6},
    {"CSAVE", 0xb7},
    {"POKE", 0xb9},
    {"PRINT", 0xba},
    {"CONT", 0xbb},
    {"LIST", 0xbc},
    {"CLEAR", 0xbd},
    {"GET", 0xbe},
    {"NEW", 0xbf},
    {"TAB(", 0xc0},
    {"TO", 0xc1},
    {"SPC(", 0xc3},
    {"THEN", 0xc4},
    {"AT", 0xc5},
    {"NOT", 0xc6},
    {"STEP", 0xc7},
    {"+", 0xc8},
    {"-", 0xc9},
    {"**", 0xca},
    {"/", 0xcb},
    {"*", 0xcc},
    {"AND", 0xcd},
    {"OR", 0xce},
    {">", 0xcf},
    {"=", 0xd0},
    {"<", 0xd1},
    {"SGN", 0xd2},
    {"INT", 0xd3},
    {"ABS", 0xd4},
    {"RAM", 0xd6},
    {"VPEEK", 0xd8},
    {"POS", 0xd9},
    {"SQR", 0xda},
    {"RND", 0xdb},
    {"LOG", 0xdc},
    {"EXP", 0xdd},
    {"COS", 0xde},
    {"SIN", 0xdf},
    {"TAN", 0xe0},
    {"ATN", 0xe1},
    {"PEEK", 0xe2},
    {"LEN", 0xe3},
    {"STR$", 0xe4},
    {"VAL", 0xe5},
    {"ASC", 0xe6},
    {"CHR$", 0xe7},
    {"LEFT$", 0xe8},
    {"RIGHT$", 0xe9},
    {"MID$", 0xea}
};

int main(int argc, char *argv[])
{
    FILE *inf, *outf;
    int i,j;
    char buffer[1024], *t;
    int offset, pointer, opos;
    char *encoded, *obuf;
    int linenumber;
    int intext = 0;
    int header = 0x5a99a501;
    int counts = 0;

    puts("MSBASIC to CV Cassette Conversion Utility 0.2");
    puts("A part of the creatiVision emulator utilities pack");

    if (argc != 3) {
        puts("Usage: bas2cas msbasic.bas cvemu.cas");
        return 0;
    }

    inf = fopen(argv[1],"r");
    if (!inf) {
        printf("Cannot open %s for reading\n",argv[1]);
        return 0;
    }

    encoded = malloc(0x7000);
    memset(encoded,0,0x7000);
    obuf = malloc(0x10000);
    memset(obuf, 0, 0x10000);

    opos = 512;
    memcpy(obuf + opos, &header, 4);
    opos += 4;
    obuf[opos++] = 1;
    obuf[opos++] = 0x50;
    opos += 4;

    offset = 3;
    pointer = 1;

    while(fgets(buffer,1024,inf) > 0)
    {
        /* Strip line ending */
        t = strtok(buffer,"\n");
        if (!t) continue;

        /* Get line number */
        while (*t == 32) t++;

        linenumber = atoi(t);

        /* Pack line number */
        encoded[offset++] = linenumber & 0xff;
        encoded[offset++] = (linenumber & 0xff00) >> 8;

        /* Skip line number */
        while ((*t >= '0') && (*t <= '9')) t++;
        while (*t == 32) t++;
        i = strlen(t) - 1;
        if (i>0)
        {
            while((t[i] == 32) && (i > 0))
            {
                t[i] = 0;
                i--;
            }
        }
        /* Convert to tokens */
        intext = 0;

        while(strlen(t))
        {
            /* Is this a token */
            if (!intext) {

                for(j = 0; j < MAX_TOKEN; j++)
                {
                    if (!(memcmp(t, tokens[j].name,
                                 strlen(tokens[j].name))))
                    {
                        t+=strlen(tokens[j].name)-1;
                        encoded[offset++]=tokens[j].token;
                        if (tokens[j].token == 0xb2)
                            intext = 3;
                        counts++;
                        break;
                    }
                }

                if ((j==MAX_TOKEN) && (*t != 32))
                {
                    encoded[offset++] = t[0];
                    if ((t[0] == '"') && (intext != 3))
                        intext ^= 1;
                }
            }
            else
            {
                encoded[offset++] = t[0];
                if ((t[0] == '"') && (intext != 3))
                    intext ^= 1;
            }

            t++;
        }

        encoded[offset++]=0;
        encoded[pointer++] = (offset + 0x5000) & 0xff;
        encoded[pointer] = ((offset + 0x5000) & 0xff00) >> 8;
        pointer = offset;
        offset += 2;
    }

    fclose(inf);

    /* Update pointers */
    offset--;
    memcpy(obuf + opos, encoded + 1, offset);
    obuf[0x206] = offset & 0xff;
    obuf[0x207] = (offset & 0xff00) >> 8;
    offset += 0x5000;
    obuf[0x208] = offset & 0xff;
    obuf[0x209] = (offset & 0xff00) >> 8;
    offset -= 0x5000;

    for(i=0, intext=0; i < offset+5; i++)
    {
        intext += obuf[0x204 + i];
        intext &= 0xff;
    }

    obuf[i+0x205] = intext & 0xff;
    offset = i + 0x204 + 0x200;

    outf = fopen(argv[2],"wb+");
    fwrite(obuf, 1, offset, outf);
    fclose(outf);

    printf("Encoded %d lines\n",counts);

    return 0;
}
