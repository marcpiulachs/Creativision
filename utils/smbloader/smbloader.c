/****************************************************************************
 * SMBLOAD Utility
 *
 * This is a utility to convert a binary creatiVision homebrew cartridge to
 * a WAV file suitable for CRUN or BLOAD on the Salora Manager.
 *
 * LICENCE
 *
 * This source code is placed in the public domain. You are free to copy,
 * distribute, modify and add to your projects without any restrictions.
 *
 * WARRANTY
 *
 * There is no warranty. You compile / run this entirely at your own risk.
 *
 * For as long as SourceForge is there - you can grab it from there as part
 * of the creatiVision Emulator project.
 *
 * Enjoy and have fun! ... and if you have time ...
 *
 * Visit http://8bit-homecomputermuseum.at and learn about the Salora
 * Manager, Creativision and rare home computers and consoles.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Microsoft Standard Wave Header
 */
typedef struct {
    int     ChunkID;            /* Always RIFF */
    int     ChunkSize;          /* Length of file, less 8 */
    int     format;             /* WAVE */
    int     subChunk1ID;        /* 'fmt ' */
    int     subChunk1Size;      /* Size of subChunk1Header, 16 */
    short   audio_format;       /* Use 1 for PCM */
    short   numchannels;        /* 1 = Mono, 2 = Stereo */
    int     samplerate;         /* 44100 etc */
    int     byterate;           /* numchannels * samplerate * bitspersample / 8 */
    short   blockalign;         /* numchannels * bitspersample / 8 */
    short   bitspersample;      /* 8, 16, 32 etc */
    int     subChunk2ID;        /* data */
    int     subChunk2Size;      /* Here to eof - 8 */
} __attribute__((__packed__)) WAVEHEADER;

/**
 * Salora Manager Sonic Invader CRUN loader
 */
const int crun_size = 224;
unsigned char crun_bin[224] = {
    0x01, 0x50, 0xDD, 0x00, 0xDE, 0x50, 0x07, 0x50, 0x0A, 0x00, 0x97, 0x00, 0x1B, 0x50, 0x0C, 0x00,
    0xBA, 0x3A, 0xBA, 0x3A, 0xBA, 0x3A, 0xBA, 0x3A, 0xBA, 0x3A, 0xBA, 0x3A, 0xBA, 0x3A, 0xBA, 0x00,
    0x37, 0x50, 0x0F, 0x00, 0xBA, 0x22, 0x4C, 0x4F, 0x41, 0x44, 0x49, 0x4E, 0x47, 0x20, 0x47, 0x41,
    0x4D, 0x45, 0x20, 0x50, 0x52, 0x4F, 0x47, 0x52, 0x41, 0x4D, 0x22, 0x00, 0x57, 0x50, 0x14, 0x00,
    0xBA, 0x22, 0x44, 0x4F, 0x20, 0x4E, 0x4F, 0x54, 0x20, 0x53, 0x54, 0x4F, 0x50, 0x20, 0x54, 0x48,
    0x45, 0x20, 0x43, 0x41, 0x53, 0x53, 0x45, 0x54, 0x54, 0x45, 0x22, 0x00, 0x7C, 0x50, 0x19, 0x00,
    0xBA, 0x22, 0x44, 0x4F, 0x20, 0x4E, 0x4F, 0x54, 0x20, 0x50, 0x52, 0x45, 0x53, 0x53, 0x20, 0x54,
    0x48, 0x45, 0x20, 0x52, 0x45, 0x53, 0x45, 0x54, 0x20, 0x53, 0x57, 0x49, 0x54, 0x43, 0x48, 0x22,
    0x00, 0x9F, 0x50, 0x1B, 0x00, 0xBA, 0x22, 0x54, 0x48, 0x49, 0x53, 0x20, 0x47, 0x41, 0x4D, 0x45,
    0x20, 0x4D, 0x55, 0x53, 0x54, 0x20, 0x55, 0x53, 0x45, 0x20, 0x4A, 0x4F, 0x59, 0x53, 0x54, 0x49,
    0x43, 0x4B, 0x22, 0x00, 0xAA, 0x50, 0x1E, 0x00, 0x92, 0x24, 0x42, 0x30, 0x30, 0x30, 0x00, 0xB5,
    0x50, 0x28, 0x00, 0x81, 0x58, 0xD0, 0x30, 0xC1, 0x33, 0x00, 0xCC, 0x50, 0x32, 0x00, 0xB9, 0x35,
    0x31, 0x32, 0xC8, 0x58, 0x2C, 0xE2, 0x28, 0xC9, 0x31, 0x36, 0x33, 0x38, 0x38, 0xC8, 0x58, 0x29,
    0x00, 0xD2, 0x50, 0x3C, 0x00, 0x82, 0x00, 0xDC, 0x50, 0x46, 0x00, 0x8C, 0xC9, 0x31, 0x33, 0x38
};

/**
 * Definitions
 */
#define ZERO_SAMPLES 26
#define ONE_SAMPLES 52
#define MAX_CYCLES 128

/**
 * Globals
 */
short int one_cycles[MAX_CYCLES];
short int zero_cycles[MAX_CYCLES];
char input[560] = {0};
char output[560] = {0};
int load_address = 0x10000; /* Impossible address for 64k machine */
float amplitude = 0.50f;    /* Amplitude defaults to 50% */
int bloader = 0;            /* Np CRUN loader default */
int loadraw = 0;            /* Jusr encode a raw file */
int emulator_file=0;        /* Create an emulator file */
int on_samples = ONE_SAMPLES;
int off_samples = ZERO_SAMPLES;

struct stat fs;

/**
 * Well hello there!
 */
void announce( void )
{
    fprintf(stdout,"Salora Manager BLOAD Utility v0.9 (February 2014)\n");
    fprintf(stdout,"Outputs 44.1Khz 16bit Mono PCM LE\n\n");
}

/**
 * Syntax
 */
void syntax( void )
{
    fprintf(stdout,"Usage  : smbloader -i input.bin -l load_address [-a amplitude] [-b]\n");
    fprintf(stdout,"Where  : -i        Input file name.\n");
    fprintf(stdout,"         -l        Load address in C hex notation.\n");
    fprintf(stdout,"         -a        Optional amplitude percentage. Defaults to 50%%.\n");
    fprintf(stdout,"         -c        Optional add CRUN loader to CALL -138.\n");
    fprintf(stdout,"         -r        Optional encode raw file\n");
    fprintf(stdout,"         -e        Optional Create emulator file\n");
    fprintf(stdout,"         -p        Optional Pulse width\n\n");
    fprintf(stdout,"Example: smbloader -i cart.bin -l 0xb000 -a 60\n\n");
    fprintf(stdout,"Create cart.wav with 60%% amplitude to load at address $B000\n");
}

/**
 * WriteByte
 */
void WriteByte(FILE *out, unsigned char b)
{
    int v;
    int mask = 0x80;

    if (!emulator_file) {
        for( v = 0; v < 8; v++ ) {
            if (b & mask)
                fwrite(&one_cycles, 1, 2 * on_samples, out);
            else
                fwrite(&zero_cycles, 1, 2 * off_samples, out);
            mask >>= 1;
        }
    } else {
        fwrite(&b, 1, 1, out);
    }
}

/**
 * WriteWord
 */
int WriteWord(FILE *out, short int w)
{
    int s = 0;
    WriteByte(out, w & 0xff);
    s += (w & 0xff);
    WriteByte(out, (w & 0xff00)>>8);
    s += ((w & 0xff00)>>8);

    return s;
}

/**
 * WriteSilence
 */
void WriteSilence( FILE *out, int seconds )
{
    int j = (44100 * 2 * seconds) - 1;

    if (emulator_file) j=(seconds * 2048)-1;

    fseek(out, j, SEEK_CUR);

    j = 0;

    fwrite(&j, 1, 1, out);
}

/**
 * WriteSignature
 */
void WriteSignature( FILE *out )
{
    WriteByte(out,0x01);
    WriteByte(out,0xA5);
    WriteByte(out,0x99);
    WriteByte(out,0x5A);
}

/**
 * WriteLeadInOut
 */
void WriteLeadInOut( FILE *out )
{
    int i;

    for(i = 0; i < 512; i++)
        WriteByte(out,0x00);
}

/**
 * Generate cycles
 */
void GenerateCycles( void )
{
    int i;
    short int v;

    v = (short int)(32767.0f * amplitude);

    /* Do zero */
    for( i = 0; i < off_samples/2; i++ ) {
        zero_cycles[i] = -v;
        zero_cycles[i+(off_samples/2)] = v;
    }

    /* Do one */
    for( i = 0; i < on_samples/2; i++) {
        one_cycles[i] = -v;
        one_cycles[i+(on_samples/2)] = v;
    }
}

/**
 * CreateWAVE
 */
void CreateWAVE( void )
{
    FILE *inf, *outf;
    WAVEHEADER wave;
    char tmp[10];
    int i;
    int cksum;
    int len;
    unsigned char c;

    GenerateCycles();

    /* Open files */
    inf = fopen(input,"rb");
    outf = fopen(output,"wb");

    /* Write blank header */
    memset(&wave, 0, sizeof(WAVEHEADER));

    if (!emulator_file)
        fwrite(&wave, 1, sizeof(WAVEHEADER),outf);

    /* Output 1 second of silence */
    WriteSilence(outf, 1);

    if (loadraw) {
        while (fread(&c,1,1,inf)>0) {
            WriteByte(outf,c);
        }
        fclose(inf);
    } else {
        if (bloader) {
            WriteLeadInOut(outf);
            WriteSignature(outf);

            /* Fix load address */
            sprintf(tmp,"$%04X",load_address);
            memcpy(crun_bin + 0xa9, tmp, 5);

            cksum = 0;

            for(i = 0; i < crun_size; i++) {
                WriteByte(outf, crun_bin[i]);
                cksum += crun_bin[i];
            }

            WriteByte(outf, 0);
            WriteByte(outf, 0);
            WriteByte(outf, 0);
            WriteByte(outf, (cksum & 0xff));

            WriteLeadInOut(outf);

            WriteSilence(outf, 3);
        }

        /* Normal BLOAD */
        WriteLeadInOut(outf);
        WriteSignature(outf);
        cksum = WriteWord(outf, load_address);
        cksum += WriteWord(outf, fs.st_size);

        while ( fread(&c, 1, 1, inf) > 0 ) {
            WriteByte(outf, c);
            cksum += c;
        }

        fclose(inf);

        WriteByte(outf, cksum & 0xff);
        WriteSignature(outf);

    }

    WriteSilence(outf, 1);

    len = ftell(outf);

    /* Do wave header */
    wave.ChunkID = 0x46464952;
    wave.ChunkSize = len - 8;
    wave.format = 0x45564157;
    wave.subChunk1ID = 0x20746d66;
    wave.subChunk1Size = 16;
    wave.audio_format = 1;
    wave.numchannels = 1;
    wave.samplerate = 44100;
    wave.bitspersample = 16;
    wave.byterate = (wave.samplerate * wave.numchannels * wave.bitspersample / 8);
    wave.blockalign = (wave.numchannels * wave.bitspersample / 8);
    wave.subChunk2ID = 0x61746164;
    wave.subChunk2Size = len - sizeof(WAVEHEADER);

    if (!emulator_file) {
        fseek(outf, 0, SEEK_SET);
        fwrite(&wave, 1, sizeof(WAVEHEADER), outf);
    }

    /* End */
    fclose(outf);

    fprintf(stdout,"Written %d bytes.\n",len);
}

/**
 * Entry
 */
int main(int argc, char *argv[])
{
    int c;
    char *p;

    /* say hello */
    announce();

    /* Get command line */
    while ( (c = getopt(argc, argv, "i:l:a:crep:")) != -1 ) {
        switch(c) {
        case 'i': /* Input file */
            strcpy(input,optarg);
            break;

        case 'l': /* Load address */
            load_address = strtol(optarg, NULL, 16);
            break;

        case 'a': /* Amplitude */
            amplitude = strtol(optarg, NULL, 10) / 100.0f;
            break;

        case 'c': /* Add CRUN */
            bloader = 1;
            loadraw = 0;
            break;

        case 'r': /* Load RAW file ... from an emulator perhaps :) */
            loadraw = 1;
            bloader = 0;
            break;

        case 'e': /* Create an emulator compatible file */
            emulator_file=1;
            break;

        case 'p': /* Creativision Shorter Cycles */
            on_samples = strtol(optarg, NULL, 10);
            if (on_samples < 36) on_samples = 36;
            if (on_samples > 64) on_samples = 64;
            off_samples = on_samples >> 1;
            break;

        case '?': /* Ooops */
            if ( optopt == 'i' ) {
                fprintf(stderr,"-i requires an input filename\n");
                return 0;
            }
            if ( optopt == 'l') {
                fprintf(stderr,"-l requires a valid address in hex C notation 0x1234\n");
                return 0;
            }
            if ( optopt == 'a') {
                fprintf(stderr,"-a requires a value\n");
                return 0;
            }
            if (isprint(optopt)) {
                fprintf(stderr, "Unknown option -%c\n", optopt);
                return 3;
            }
            fprintf(stderr,"Unknown option character `\\x%x'.\n", optopt);
            return 0;
        }
    }

    /* Check sanity */
    if (strlen(input) == 0 ) {
        syntax();
        return 0;
    };
    if ((amplitude < 0.05f) || (amplitude > 0.95f)) {
        fprintf(stderr,"Amplitude must be between 5 and 95 percent\n");
        return 0;
    }

    if (((load_address < 0) || (load_address > 0xbfff)) && (!loadraw)) {
        fprintf(stderr,"Load address must be below $C000: $%08X\n",load_address);
        return 0;
    }

    /* Check file name */
    if ( stat(input,&fs) == -1 ) {
        fprintf(stderr,"Cannot find input file : %s\n",input);
        return 0;
    }

    /* Check file size + load are sensible */
    if ( ((load_address + fs.st_size) > 0xC000 ) && (!loadraw) ) {
        fprintf(stderr,"File size + load address exceeds $C000\n");
        return 0;
    }

    /* Show information */
    strcpy(output, input);
    p = strrchr(output,'.');
    if (p) *p = 0;

    if (emulator_file)
        strcat(output,".cas");
    else
        strcat(output,".wav");

    fprintf(stdout,"Input Binary  : %s\n", input);
    fprintf(stdout,"Output WAV    : %s\n", output);

    if (!loadraw ) {
        fprintf(stdout,"Load at       : $%04X\n", load_address);
        fprintf(stdout,"Length        : $%04X\n", (int)fs.st_size);
        fprintf(stdout,"Volume        : %02d%%\n", (int)(amplitude * 100.0f));
    }

    fprintf(stdout,"Raw Encoding  : %s\n", loadraw?"Y":"N");
    fprintf(stdout,"CRUN Loader   : %s\n", bloader?"Y":"N");
    fprintf(stdout,"Pulse Width   : %d\n\n", on_samples * 2);

    CreateWAVE();

    return 0;
}
