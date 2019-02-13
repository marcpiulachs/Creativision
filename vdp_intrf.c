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
 * VDP Interface
 *
 * This file contains the required code to interface with the TMS9918 driver
 * EmuLIB.
 ****************************************************************************/
#include "crvision.h"

SDL_Window *screen = NULL;
SDL_Renderer *render = NULL;
SDL_Texture *texture = NULL;
int vdp_halt = 0;

#ifdef WIN32
HANDLE hWindow;
#endif // WIN32
/**
 * InitSDL
 */
int InitSDL( int vflags )
{
    int window_width=0;
    int window_height=0;
    char tbuf[128];

    if (magnify == 0 || magnify > 4) {
        window_width = vdp_width * 2;
        window_height = vdp_height * 2;
    } else {
        window_width = vdp_width * magnify;
        window_height = vdp_height * magnify;
    }

    /* Try to initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ) != 0 ) {
        fprintf(stderr,"SDL Failed to initialize!\nSDL: %s\n",
                SDL_GetError());
        return FAIL;
    }

    /* Set exit handler */
    atexit(SDL_Quit);

    /* How many joysticks to do */
    joysticks = SDL_NumJoysticks();
    if (joysticks) InitJoysticks(joysticks);

    strcpy(tbuf,version);
    if (np_player == 1) strcat(tbuf, " - SERVER");
    if (np_player == 2) strcat(tbuf, " - CLIENT");

    /* Create a window */
    screen = SDL_CreateWindow(tbuf,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              window_width,
                              window_height, vflags);


    if ( screen == NULL ) {
        fprintf(stderr, "Cannot create SDL window!\nSDL: %s\n",
                SDL_GetError());
        return FAIL;
    }

#ifdef WIN32
    if (vflags != SDL_WINDOW_FULLSCREEN_DESKTOP) {
        hWindow = FindWindow(NULL,version);
        if ( hWindow != INVALID_HANDLE_VALUE ) {
            HICON hIcon = LoadIcon(GetModuleHandle(0),"myicon");
            SendMessage(hWindow, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
    }
#endif // WIN32

    /* Create renderer */
    render = SDL_CreateRenderer(screen, -1, 0);
    if (render == NULL ) {
        fprintf(stderr,"Cannot create SDL renderer\nSDL: %s\n",
                SDL_GetError());
        return FAIL;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(render, vdp_width,vdp_height);

    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                vdp_width, vdp_height);
    if (texture == NULL) {
        fprintf(stderr,"Cannot create SDL texture\nSDL: %s\n",
                SDL_GetError());
        return FAIL;
    }

    return SUCCESS;
}

/**
 * RefreshScreen
 *
 * Called by TMS9918 core
 */
void RefreshScreen(void *scrbuffer, int width, int height)
{
    SDL_UpdateTexture(texture, NULL, scrbuffer, width * sizeof(int));
    SDL_RenderClear(render);
    SDL_RenderCopy(render, texture,NULL,NULL);
    SDL_RenderPresent(render);
}

/**
 * VDP_ShowRegInfo
 */
void VDP_ShowRegInfo( void )
{
    int t;

    t = (VDP.R[0] << 8) | VDP.R[1];
    printf ("MODE         : ");
    switch ( t & 0x218 ) {
    case VDP_MODE0:
        printf("40 x 32 Text Mode\n");
        break;
    case VDP_MODE1:
        printf("Graphics Mode 1\n");
        break;
    case VDP_MODE2:
        printf("Graphics Mode 2 (Bitmap)\n");
        break;
    case VDP_MODE3:
        printf("Multicolour Mode\n");
        break;
    default:
        printf("Unknown\n");
        break;
    }

    printf("EXTERNAL     : %s\n", VDP.R[0] & 1 ? "ENABLED" : "DISABLED");
    printf("VDP RAM      : %dk\n", VDP.R[1] & 0x80 ? 16 : 4);
    printf("BLANK        : %s\n", VDP.R[1] & 0x40 ? "ENABLED" : "DISABLED");
    printf("SPRITE SIZE  : %s\n", VDP.R[1] & 2 ? "16x16" : "8x8");
    printf("MAGNIFIED    : %dx\n", VDP.R[1] & 1 ? 2 : 1);
    printf("NAME TABLE   : $%04X\n", VDP.R[2] * 0x400);
    printf("COLOUR TABLE : $%04X\n", VDP.R[3] * 0x40);
    printf("PATTERN TABLE: $%04X\n", VDP.R[4] * 0x800);
    printf("SPRITE ATTR. : $%04X\n", VDP.R[5] * 0x80);
    printf("SPRITE PATT. : $%04X\n", VDP.R[6] * 0x800);
}

/**
 * VDP_ShowRegs
 *
 * Function to show some VDP information in the host console
 */
void VDP_ShowRegs( int which )
{
    SDL_Event event;
    int i;
    int quit = 1;

    if (vdp_halt) {
        SDL_PauseAudio(1);

        /* Show VDP Registers */
        for (i = 0; i < 8; i++ )
            fprintf(stdout, "R%d:%02X ", i, VDP.R[i]);

        fprintf(stdout,"SR:%02X <-- VDP ", VDP.Status);

        switch (which) {
        case 0:
            fprintf(stdout, "READ DATA");
            break;
        case 1:
            fprintf(stdout, "READ CONTROL");
            break;
        case 2:
            fprintf(stdout, "WRITE DATA");
            break;
        case 3:
            fprintf(stdout, "WRITE CONTROL");
            break;
        }

        fprintf(stdout,"\n");
        if (vdp_debug > 1) VDP_ShowRegInfo();

        /* Wait for instructions */
        while (quit) {
            if (SDL_PollEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                    case SDLK_F12:
                        quit = 0;
                        break;

                    case SDLK_F11:
                        quit = 0;
                        vdp_halt = 0;
                        break;
                    }
                }
            }
        }

        SDL_PauseAudio(demo_record);
    }
}

/****************************************************************************
 * vdp_errmsg
 *
 * Function to show fatal messages, using the display and CV font
 ****************************************************************************/
void vdp_errmsg(const char *msg[], int lines)
{
    int maxwidth = 0;
    int i;
    int x,y,yy;
    int top,left;
    int nwidth, nheight;
    int colour = 0;
    int offset = 0;
    int rchar,c,b,mask;

    for(i=0; i<lines; i++)
    {
        if (strlen(msg[i]) > maxwidth ) maxwidth = strlen(msg[i]);
    }

    nwidth = maxwidth << 3;
    nwidth += 16;

    nheight = lines << 3;
    nheight += 4;

    left = (VDP_WIDTH - nwidth) >> 1;
    top = (VDP_HEIGHT - nheight) >> 1;

    /* Draw box to display the message */
    for(y = 0; y < nheight; y++)
    {
        for( x = 0; x < nwidth; x++)
        {
            colour = 0x800000;

            if ( (x == 0) || ( x == (nwidth -1)) )
            {
                colour = 0xfefefe;
            }

            if ( ( y == 0) || ( y == (nheight -1)) )
            {
                colour = 0xfefefe;
            }

            offset = (top+y) * (VDP_WIDTH * 4);
            offset += ((left + x) * 4);
            memcpy(VRAM + offset, &colour, 4);
        }
    }

    /* Draw the text on the screen */
    top += 2;

    for(y = 0; y < lines; y++)
    {
        if (strlen(msg[y]))
        {
            /* Get top left of first character */
            offset = (top * VDP_WIDTH * 4);
            i = (VDP_WIDTH - (strlen(msg[y])*8)) >> 1;
            offset += (i*4);

            for(i = 0; i < strlen(msg[y]); i++)
            {
                c = msg[y][i];
                rchar = (c << 3) + 0xf700;
                for(yy=0; yy<8; yy++)
                {
                    c = RAM[rchar++];

                    for (x=0; x<8; x++)
                    {
                        mask = 0x80;
                        for (b = 0; b < 8; b++)
                        {
                            if (c & mask)
                                memcpy(VRAM + offset + (b*4) + (yy*VDP_WIDTH*4), &colour, 4);

                            mask >>= 1;
                        }
                    }
                }
                offset += 32;
            }
        }

        top += 8;

    }

    RefreshScreen(VDP.XBuf, VDP_WIDTH, VDP_HEIGHT);

    EmuQuit = 2;
}
