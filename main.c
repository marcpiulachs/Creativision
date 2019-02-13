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
 * main.c
 *
 * ... better known as the SuperGlue module!
 ****************************************************************************/
#include "crvision.h"
#include "platform.h"
#include "netplay.h"

/**
 * Global Machine
 */
M6502 CPU;          /* M6502 CPU Core */
TMS9918 VDP;        /* TMS9918 VDP */
unsigned long long int total_cycles = 0;

/* Memory RAM, VRAM etc */
unsigned char *RAM = NULL;
unsigned char *VRAM = NULL;
unsigned char *scrbuffer = NULL;
char rname[MAX_WINPATH];
char dname[MAX_WINPATH];

/* VDP */
int vdp_width  = VDP_WIDTH;
int vdp_height = VDP_HEIGHT;

/* Options */
int vdp_debug = 0;
int magnify = 0;
int cloadfast = 0;
int sound44k = 1;
int joysticks = 0;
int demo_record = 0;
int debug_trace = 0;
int devrom = 0;
int cslCart = 0;
int kvpoke = 0;
int salora = 0;
int np_player = 0;
char np_address[128] = {0};

/* VPOKE Patch */
const uint8_t vpokebin[] = {
    0x20, 0x74, 0xD8, 0x8A, 0x48, 0xA5, 0x51, 0x09,
    0xC0, 0xAA, 0xA5, 0x50, 0x20, 0x6F, 0xE9, 0x68,
    0x20, 0x63, 0xE9, 0x60
};

const char *version = APP_TITLE " " APP_VERSION;
const char *builton = "Built on " __DATE__ " " __TIME__;

/**
 * showinfo
 *
 * Let user know how to use it
 */
void showinfo(void)
{
    fprintf(stdout,"%c Available switches\n%c\n",254,254);
    fprintf(stdout,"%c -h     This text\n",254);
    fprintf(stdout,"%c -b     Load alternative BIOS rom. Default is BIOS/BIOSCV.ROM\n",254);
    fprintf(stdout,"%c -r     Load alternative ROM binary. Default is ROMS/CART.BIN\n",254);
    fprintf(stdout,"%c -m     Magnify. 2=544x416(Default) 1x to 4x zoom\n",254);
    fprintf(stdout,"%c -f     Fullscreen 4:3 Aspect\n",254);
    fprintf(stdout,"%c -v     VDP Debug mode enabled. Use F12 stop/step. F11 continue\n",254);
    fprintf(stdout,"%c -t     Trace debugger enabled.\n",254);
#ifndef RPI
    fprintf(stdout,"%c -s     Use Low Quality sound\n",254);
#endif
    fprintf(stdout,"%c -l     Load CAS files at double speed. CLOAD / CRUN\n", 254);
    fprintf(stdout,"%c -c     Set CLOAD/CSAVE filename. Default CSAVE.CAS\n",254);
    fprintf(stdout,"%c -p     Set LLIST/LPRINT filename. Default PRINTER.TXT\n",254);
#ifndef RPI
    fprintf(stdout,"%c -d     Demo recording\n",254);
#endif
    fprintf(stdout,"%c -g     Developer debug rom\n",254);
    fprintf(stdout,"%c -k     Patch VPOKE in CSL mode\n",254);
    fprintf(stdout,"%c -2     CSL Cartridge emulation\n",254);
    fprintf(stdout,"%c -3     Salora Manager / Laser 2000 emulation\n\n",254);

    fprintf(stdout,"%c -n     Network Player number\n",254);
    fprintf(stdout,"%c -i     Remote IP address\n\n",254);

    fprintf(stdout,"%c -e     Floppy image name\n\n",254);

    fprintf(stdout,"%c Emulator Keys\n%c\n",254,254);
    fprintf(stdout,"%c ESC    Terminate emulation\n",254);
    fprintf(stdout,"%c F2     Pause game play\n",254);
    fprintf(stdout,"%c F3     Reset\n",254);
    fprintf(stdout,"%c F4     Rewind cassette\n",254);
    fprintf(stdout,"%c F5     Take PNG picture\n",254);
    fprintf(stdout,"%c F7     Take snapshot\n",254);
    fprintf(stdout,"%c F8     Load snapshot\n",254);
    fprintf(stdout,"%c F10    Dump RAM and VRAM\n",254);
}

/**
 * DestroyCV
 *
 * Perform any cleanup required
 */
void DestroyCV()
{
    if (demo_record) CloseAVI();
    if (cassette) WriteBIT(256);
    if (salora) write_disk_image(dname);
    if (prt) fclose(prt);
    if (RAM) free(RAM);
    if (scrbuffer) Trash9918(&VDP);
    if (joys[0]) SDL_JoystickClose(joys[0]);
    if (joys[1]) SDL_JoystickClose(joys[1]);
    if (texture) SDL_DestroyTexture(texture);
    if (render) SDL_DestroyRenderer(render);
    if (screen) SDL_DestroyWindow(screen);
}

/**
 * InitCV
 *
 * Initialise the creatiVision machine
 */
int InitCV()
{
    int i, pal;

    /* Allocate required RAM in one chunk */
    RAM = malloc(CPU_RAM_SIZE + VDP_RAM_SIZE);
    if (!RAM) {
        fprintf(stderr,"Out of memory allocating machine!\n");
        return FAIL;
    }

    memset(RAM, NORAM, CPU_RAM_SIZE);
    VRAM = RAM + CPU_RAM_SIZE;
    memset(RAM, 0, 1024);

    /* Initialise VDP */
    memset(&VDP, 0, sizeof(TMS9918));
    scrbuffer = New9918(&VDP, VRAM, vdp_width, vdp_height);
    if (!scrbuffer) {
        fprintf(stderr,"Error allocating VDP device\n");
        return FAIL;
    }

    VDP.MaxSprites = TMS9918_MAXSPRITES;
    VDP.Scanlines = TMS9929_LINES;
    VDP.DrawFrames = 100;

    /* Initialise CPU */
    memset(&CPU, 0, sizeof(M6502));
    CPU.IPeriod = VDP_SCAN_CYCLES;
    CPU.Trace = debug_trace;

    Reset9918(&VDP, scrbuffer, vdp_width, vdp_height);
    pal = 32;
    for( i=0; i < 16; ++i, ++pal ) {
        VDP.XPal[i] = Palette9918[pal].R << 16 | Palette9918[pal].G << 8 | Palette9918[pal].B;
    }

    if (demo_record) OpenAVI();

    return AudioInit();
}

/**
 * Our journey begins ...
 */
int main(int argc, char *argv[])
{
    char bname[MAX_WINPATH];
    int sdl_vflags;
    int c;
    int bios_loaded = 0;
    int sharemem = 0;

    /* Set defaults */
    strcpy(rname,"roms/cart.bin");
    strcpy(bname,"bios/bioscv.rom");
    strcpy(cfname,"csave.cas");
    strcpy(pfname,"printer.txt");
    strcpy(dname,"floppy.bin");

    sdl_vflags = 0;

    /* Announce what we are */
    for( c = 0; c < 40; c++ )
        fprintf(stdout,"%c",254);

    fprintf(stdout,"\n%c %s\n%c %s\n", 254, version, 254, builton);
    fprintf(stdout,"%c Compiler: %s\n",254,PLATFORM);

    /* Look for command line options */
    while ( (c = getopt(argc, argv, "b:r:fhslvm:c:p:dtg2k3n:i:e:")) != -1 ) {
        switch(c) {
        case 'e':   // Floppy disk image name
            strcpy(dname, optarg);
            fprintf(stdout,"%c Floppy image %s\n",254,dname);
            break;

        case 'b':   // Use altenate bios
            strcpy(bname, optarg);
            fprintf(stdout,"%c Alternate BIOS %s\n",254,bname);
            break;

        case 'r':   // Use alternate rom
            strcpy(rname, optarg);
            fprintf(stdout,"%c Alternate ROM %s\n", 254,rname);
            break;

        case 'f':   // SDL Full Screen
            sdl_vflags = SDL_WINDOW_FULLSCREEN_DESKTOP;
            fprintf(stdout, "%c Option Fullscreen Enabled\n",254);
            break;

        case 'v':    // VDP Debug
            vdp_debug++;
            fprintf(stdout,"%c VDP Debug Enabled\n",254);
            break;

        case 'm':
            magnify = atoi(optarg);
            if (magnify > 4 || magnify < 0) magnify = 0;
            else fprintf(stdout,"%c Magnification %d enabled\n",254,magnify);
            break;

        case 'h':   // Show usage
            showinfo();
            return 0;
            break;

        case 'l':
            cloadfast = 1;
            fprintf(stdout,"%c CLOAD/CRUN Fast Loading Enabled\n",254);
            break;


#ifndef RPI
        case 's':
            sound44k = 0;
            fprintf(stdout,"%c Sound Low Quality Enabled\n",254);
            break;
#endif

        case 'c':
            strcpy(cfname, optarg);
            fprintf(stdout,"%c CSSAVE/CLOAD file %s\n",254,cfname);
            break;

        case 'p':
            strcpy(pfname, optarg);
            fprintf(stdout,"%c LLIST/LPRINT redirected to %s\n", 254, pfname);
            break;

#ifndef RPI
        case 'd':
            demo_record = 1;
            fprintf(stdout,"%c Demo recording enabled. Audio will be muted.\n",254);
            break;
#endif

        case 't':
            debug_trace = 1;
            fprintf(stdout,"%c Debugging enabled\n",254);
            break;

        case 'g':
            devrom = 1;
            fprintf(stdout,"%c Developer linear load enabled\n",254);
            break;

        case '2':
            cslCart = 1;
            strcpy(bname,"bios/cslbios.rom");
            fprintf(stdout,"%c CSL Cart emulation enabled\n", 254);
            break;

        case 'k':   // Patch vpoke
            kvpoke = 1;
            fprintf(stdout, "%c VPOKE patch enabled\n", 254);
            break;

        case '3':
            salora = 1;
            strcpy(bname,"bios/saloram.rom");
            fprintf(stdout,"%c Salora Manager / Laser 2001 Emulation\n", 254);
            break;

        case 'n':   // Network player number
            np_player = atoi(optarg);
            if (np_player == 1) {
                fprintf(stdout,"%c Player 1 - SERVER\n", 254);
            } else {
                if (np_player == 2) {
                    fprintf(stdout,"%c Player 2 - CLIENT\n", 254);
                }
                else
                    np_player = 0;
            }
            break;

        case 'i':   // Remote IP address
            strcpy(np_address, optarg);
            fprintf(stdout,"%c Remote server set to %s\n",254,np_address);
            break;

        case '?':   // Missing argument
            if (optopt == 'r') {
                fprintf(stderr, "-r requires a ROM name\n");
                return 1;
            }
            if (optopt == 'b') {
                fprintf(stderr, "-b requires a BIOS name\n");
                return 2;
            }
            if (isprint(optopt)) {
                fprintf(stderr, "Unknown option -%c\n", optopt);
                return 3;
            }
            fprintf(stderr,"Unknown option character `\\x%x'.\n", optopt);
            return 4;

        default:
            abort();
        }
    }

    if (demo_record) sound44k = 0;

    /* Ok - let's go! */
    if (InitSDL(sdl_vflags)) {

        if (InitCV()) {

            fprintf(stdout,"%c Machine Initialised OK\n",254);

            if (np_player) {

                if (SDLNet_Init() != -1 )
                {
                    // Reset any conflicting globals
                    salora = cslCart = 0;
                    debug_trace = vdp_debug = 0;
                    cloadfast = devrom = kvpoke = 0;

                    if (NP_Init())
                    {
                        fprintf(stdout,"%c Network play initialised OK\n", 254);

                        if(np_player==1) {
                            bios_loaded = LoadBIOS(bname, RAM + BIOS_ADDRESS);
                            if (bios_loaded) {
                                fprintf(stdout,"%c BIOS Loaded OK\n",254);
                                if (LoadROM(rname, RAM)) {
                                    fprintf(stdout,"%c ROM Loaded OK\n",254);
                                    sharemem = 1;
                                }
                            }
                        } else { sharemem = 1; }

                        if (sharemem) {
                            if (NP_ShareMemory())
                            {
                                fprintf(stdout,"%c Network play begins\n", 254);

                                /* Announce what we are */
                                for( c = 0; c < 40; c++ )
                                    fprintf(stdout,"%c",254);
                                fprintf(stdout,"\n");

                                Reset6502(&CPU);
                                SDL_PauseAudio(demo_record);
                                Run6502(&CPU);
                            }
                        }

                    }
                    else
                        fprintf(stdout,"%c *ERROR* Initialising sockets\n",254);
                }
                else
                {
                    fprintf(stdout,"%c *ERROR* Initialising SDL Net\n",254);
                }

            } else {

                /* Load BIOS */
                if (cslCart || salora) {
                    bios_loaded = LoadBIOS(bname, RAM + CSL_BIOS_ADDRESS);
                    if (kvpoke)
                    {
                        /* Patch memory */
                        RAM[0xC103]=0x0f;
                        RAM[0xC104]=0x40;

                        /* Download patch */
                        memcpy(RAM + 0x4010, &vpokebin, 20);
                    }

                    if (salora) load_disk_image(dname);
                }
                else
                    bios_loaded = LoadBIOS(bname, RAM + BIOS_ADDRESS);

                if (bios_loaded) {
                    fprintf(stdout,"%c BIOS Loaded OK\n",254);

                    /* Load cartridge */
                    if (LoadROM(rname, RAM)) {
                        fprintf(stdout,"%c ROM Loaded OK\n",254);

                        /* Announce what we are */
                        for( c = 0; c < 40; c++ )
                            fprintf(stdout,"%c",254);
                        fprintf(stdout,"\n");

                        /* Action! */
                        Reset6502(&CPU);
                        SDL_PauseAudio(demo_record);
                        Run6502(&CPU);
                    }
                }
            }
        }
    }

    /* Perform cleanups */
    DestroyCV();
    if (np_player) SDLNet_Quit();
    SDL_Quit();

    return 0;
}
