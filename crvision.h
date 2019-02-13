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
 * crvision.h
 *
 * Header file to pull together all the common includes.
 ****************************************************************************/
#ifndef __CRVISION_H__
#define __CRVISION_H__

/* MinGW includes */
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <png.h>
#include <zlib.h>

/* EmuLIB includes */
extern unsigned long long int total_cycles;
extern int vdp_debug;
extern int vdp_halt;

#include "M6502.h"
#include "TMS9918.h"
#include "sn76496sp.h"

/* SDL 2.0 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

/* General */
#define SUCCESS 1
#define FAIL    0
#define NORAM   255
#define MAX_WINPATH 256

/* CreatiVision */
#define VDP_WIDTH 278
#define VDP_HEIGHT 208
#define BIOS_SIZE 2048
#define BANK_SIZE 4096
#define PAGE_SIZE 2048
#define CPU_RAM_SIZE 0x10000
#define VDP_RAM_SIZE (VDP_WIDTH * VDP_HEIGHT * 4)
#define BIOS_ADDRESS 0xF800
#define CPU_CLOCK 2000000
#define PSG_CLOCK CPU_CLOCK
#define VDP_SCAN_CYCLES (CPU_CLOCK / 50) / TMS9929_LINES

/* CSL Cartridge */
#define CSL_BIOS_SIZE 16384
#define CSL_BIOS_ADDRESS 0xC000

/* PIA */
#define PA0 1
#define PA1 2
#define PA2 3
#define PA3 4

typedef struct {
    int PDR;  /* Peripheral Data Register */
    int DDR;  /* Data Direction Register */
    int CR;   /* Control Register */
    unsigned long long int prev_cycles;
} M6821;

typedef struct {
    uint32_t psg_clock;
    uint16_t whiteNoiseFeedback;
    uint8_t shiftRegisterWidth;
    uint8_t latchedChannel;
    uint8_t volumesLatched;
    uint8_t volumeRegisters[4];
    uint16_t toneRegisters[3];
    uint8_t noiseRegister;
    double tonePhaseCounters[3];
    uint8_t toneOutputs[3];
    double noisePhaseCounter;
    double tonePhase;
    uint8_t noiseOutput;
    uint16_t noiseLFSR;
    uint8_t noiseLFSROutput;
    int32_t sampleRate;
} SN76496SP;

#define PIA_INPUT    0
#define PIA_OUTALL 0xff
#define PIA_OUTHN  0xf0
#define PIA_OUTLN  0x0f
#define PIA_CLOAD  0x7f
#define PIA_CSAVE  0xff
#define PIA_KEY0   0xf7
#define PIA_KEY1   0xfb
#define PIA_KEY2   0xfd
#define PIA_KEY3   0xfe
#define PIA_PDR    4
#define PIA_DDR    0
#define PLAY_BUFFER 2048

unsigned char PIA_Read( word addr );
void PIA_Write(word addr, byte data);
unsigned char SM_PIA_Read( word addr );
void SM_PIA_Write( word addr, byte data);
void WriteCentronics( int addr, unsigned char data );
unsigned char ReadCentronics( int addr );
void CSAVE( int data );
void SM_CSAVE( void );

/* Memory et al*/
int LoadBIOS( char *fname, unsigned char *buffer );
int LoadROM(char *fname, unsigned char *buffer);
int AudioInit(void);
int InitSDL( int vflags );
int check_input( void );
int check_input_salora( void );
void WriteBIT(int v);
void InitJoysticks(int num);
void SetJoyXY(SDL_Event event);
void SetButtonDown(SDL_Event event);
void SetButtonUp(SDL_Event event);
unsigned char CLOAD(void);
void SM_CLOAD(void);
void SM_BLOAD(void);
void SM_RECALL(void);
void SaveSnapShot( void );
void LoadSnapShot(void);
void WriteBMP(void);
void OpenAVI( void );
void RecordFrame(void);
void CloseAVI(void);
void WritePNG( void );

/* All global values */
extern M6502 CPU;
extern TMS9918 VDP;
extern int ROMSize;
extern int ROM_BANK[16];
extern int ROM_MASK[16];
extern unsigned char *RAM;
extern unsigned char *VRAM;
extern int vdp_width;
extern int vdp_height;
extern SDL_Window *screen;
extern SDL_Renderer *render;
extern SDL_Texture *texture;
extern void vdp_errmsg(const char *msg[], int lines);
extern long long int performance;
extern int EmuQuit;
extern unsigned char KEYBD[8];
extern FILE *pfile;
extern FILE *bin;
void VDP_ShowRegs( int which );
extern int magnify;
extern unsigned long long int pfile_last;
extern unsigned long long int bin_last;
extern int have_io;
extern int cloadfast;
extern int sound44k;
extern int joysticks;
extern int devrom;
extern int cslCart;
extern int kvpoke;
extern int salora;
extern SDL_Joystick *joys[2];
extern char cfname[MAX_WINPATH];
extern char pfname[MAX_WINPATH];
extern char rname[MAX_WINPATH];
extern int16_t playbuffer[PLAY_BUFFER];
extern int demo_record;
extern FILE *avi;
extern int tmsirq_pending;
extern FILE *cassette;
extern FILE *prt;
extern int np_player;
extern char np_address[128];

/* Application */
#define APP_TITLE "creatiVision Emulator"
#define APP_VERSION "16.04.24"
extern const char *version;

#ifdef WIN32
extern HANDLE hWindow;
#endif

typedef struct {
    short int bmtype;
    int   fsize;
    short int reserved1, reserved2;
    int offset;
} __attribute__((__packed__)) BMPINFO;

typedef struct {
    unsigned int hsize;              /* Header size in bytes      */
    int width,height;                /* Width and height of image */
    unsigned short int planes;       /* Number of colour planes   */
    unsigned short int bits;         /* Bits per pixel            */
    unsigned int compression;        /* Compression type          */
    unsigned int imagesize;          /* Image size in bytes       */
    int xresolution,yresolution;     /* Pixels per meter          */
    unsigned int ncolours;           /* Number of colours         */
    unsigned int importantcolours;   /* Important colours         */
} __attribute__((__packed__)) INFOHEADER;

typedef struct {
    long    id;
    long    length;
} CHUNK;

typedef struct {
    long    id;
    long    flags;
    long    offset;
    long    length;
} IDX;

#define VDP_MODE0 16
#define VDP_MODE1 0
#define VDP_MODE2 0x200
#define VDP_MODE3 8

/**
 * Additions for Laser DOS
 */

extern byte laserdos_read(word addr);
extern void laserdos_write(word addr, byte data);
extern int load_disk_image(char *img);
extern void write_disk_image(char *fname);

#endif // __CRVISION_H__
