/****************************************************************************
 *                                   \ /
 *                        C R E A T I V I S I O N
 *                            E M U L A T O R
 *                                15.12.28
 *
 *       This emulator was written using Marat Fayzullin's Emulib
 *
 *       *YOU MAY NOT DISTRIBUTE THIS FILE COMMERCIALLY. PERIOD!*
 *---------------------------------------------------------------------------
 * netplay.c
 *
 * SDL_Net game play
 ****************************************************************************/
#include "crvision.h"
#include "netplay.h"

/**
 * Globals
 */
TCPsocket server, client;
IPaddress *remoteip;
unsigned char keydown = 0;
unsigned char keyup = 0;

/**
 * NP_Init
 */
int NP_Init( void )
{
    IPaddress ip;

    if (np_player == 1)
    {
        if (SDLNet_ResolveHost(&ip, NULL, NP_PORT) == -1 ) return 0;
        server = SDLNet_TCP_Open(&ip);
        if (!server) return 0;
    }
    else
    {
        if (SDLNet_ResolveHost(&ip, np_address, NP_PORT) == -1) return 0;
        client = SDLNet_TCP_Open(&ip);
        if (!client) return 0;
    }

    return 1;
}

/**
 * NP_ShareMemory
 */
int NP_ShareMemory( void )
{
    int ret = 0;
    int ticks = 0;
    char spinner[] = "|/-\\";

    if (np_player == 1) // Server mode
    {
        client = 0;
        while (!client) {
            fprintf(stdout,"%c Waiting for client to connect ... %c\r", 254, spinner[ticks & 3]);
            client = SDLNet_TCP_Accept(server);
            if (!client) {
                SDL_Delay(100);
                if (ticks > 200) {
                    fprintf(stdout,"%c Waiting for client to connect ... TIMEOUT\n", 254);
                    return 0;
                }
            } else {
                remoteip = SDLNet_TCP_GetPeerAddress(client);
                fprintf(stdout,"%c Waiting for client to connect ... OK\n", 254);
            }
            ticks++;
        }

        if (client)
        {
            /* Send loaded machine to client */
            ret = SDLNet_TCP_Send(client, RAM, CPU_RAM_SIZE);
            if (ret != CPU_RAM_SIZE) return 0;
            ret = SDLNet_TCP_Send(client, &ROM_BANK[0], sizeof(int)<<4);
            if (ret != (sizeof(int)<<4)) return 0;
            ret = SDLNet_TCP_Send(client, &ROM_MASK[0], sizeof(int)<<4);
            if (ret != (sizeof(int)<<4)) return 0;
        }
    }
    else    // Client mode
    {
       ret = SDLNet_TCP_Recv(client, RAM, CPU_RAM_SIZE);
       if (ret != CPU_RAM_SIZE) return 0;
       ret = SDLNet_TCP_Recv(client, &ROM_BANK[0], sizeof(int)<<4);
       if (ret != (sizeof(int)<<4)) return 0;
       ret = SDLNet_TCP_Recv(client, &ROM_MASK[0], sizeof(int)<<4);
       if (ret != (sizeof(int)<<4)) return 0;
    }

    return 1;
}

/**
 * NP_check_input
 */
int NP_check_input()
{
    int ret = 0;

    SDL_Event event;

    keydown = keyup = 0;

    if (SDL_PollEvent((&event))) {

        switch (event.type) {

        case SDL_KEYDOWN:

            switch( event.key.keysym.sym ) {

                case SDLK_ESCAPE:
                    EmuQuit = 1;
                    break;

                case SDLK_F3:
                    if (np_player == 1) {
                        ret = 1;
                        keydown |= RESET;
                    }
                    break;

                case SDLK_z:
                    if (np_player == 1) {
                        KEYBD[PA1] &= 0xf5;
                        keydown |= SELECT;
                    }
                    break;

                case SDLK_UP:
                    if (np_player == 1) KEYBD[PA0] &= 0xf7;
                    else KEYBD[PA2] &= 0xf7;

                    keydown |= ARROW_UP;
                    break;

                case SDLK_DOWN:
                    if (np_player == 1) KEYBD[PA0] &= 0xfd;
                    else KEYBD[PA2] &= 0xfd;

                    keydown |= ARROW_DOWN;
                    break;

                case SDLK_LEFT:
                    if (np_player == 1) KEYBD[PA0] &= 0xdf;
                    else KEYBD[PA2] &= 0xdf;

                    keydown |= ARROW_LEFT;
                    break;

                case SDLK_RIGHT:
                    if (np_player == 1) KEYBD[PA0] &= 0xfb;
                    else KEYBD[PA2] &= 0xfb;

                    keydown |= ARROW_RIGHT;
                    break;

                case SDLK_LCTRL:
                    if (np_player == 1) KEYBD[PA0] &= 0x7f;
                    else KEYBD[PA3] &= 0x7f;

                    keydown |= FIRE1;
                    break;

                case SDLK_LSHIFT:
                    if (np_player == 1) KEYBD[PA1] &= 0x7f;
                    else KEYBD[PA2] &= 0x7f;

                    keydown |= FIRE2;
                    break;
            }
            break;

        case SDL_KEYUP:

            switch( event.key.keysym.sym ) {

                case SDLK_z:
                    if (np_player == 1) {
                        KEYBD[PA1] |= 0x0a;
                        keyup |= SELECT;
                    }
                    break;

                case SDLK_UP:
                    if (np_player == 1) KEYBD[PA0] |= 0x08;
                    else KEYBD[PA2] |= 0x08;

                    keyup |= ARROW_UP;
                    break;

                case SDLK_DOWN:
                    if (np_player == 1) KEYBD[PA0] |= 0x02;
                    else KEYBD[PA2] |= 0x02;

                    keyup |= ARROW_DOWN;
                    break;

                case SDLK_LEFT:
                    if (np_player == 1) KEYBD[PA0] |= 0x20;
                    else KEYBD[PA2] |= 0x20;

                    keyup |= ARROW_LEFT;
                    break;

                case SDLK_RIGHT:
                    if (np_player == 1) KEYBD[PA0] |= 0x04;
                    else KEYBD[PA2] |= 0x04;

                    keyup |= ARROW_RIGHT;
                    break;

                case SDLK_LCTRL:
                    if (np_player == 1) KEYBD[PA0] |= 0x80;
                    else KEYBD[PA3] |= 0x80;

                    keyup |= FIRE1;
                    break;

                case SDLK_LSHIFT:
                    if (np_player == 1) KEYBD[PA1] |= 0x80;
                    else KEYBD[PA2] |= 0x80;

                    keyup |= FIRE2;
                    break;
            }
            break;
        }
    }

    return ret;
}

/**
 * RemoteKeys
 */
int NP_RemoteKeys()
{
    int ret = 0;
    NP_DATA np_data;

    np_data.keydown = keydown;
    np_data.keyup = keyup;
    np_data.ticks = total_cycles;

    ret = SDLNet_TCP_Send(client, &np_data, sizeof(np_data));
    ret = SDLNet_TCP_Recv(client, &np_data, sizeof(np_data));

    if (ret == sizeof(np_data))
    {
        keydown = np_data.keydown;
        keyup = np_data.keyup;

        if (total_cycles != np_data.ticks)
            printf("Out of sync!");

        if (np_player == 1)
        {
            /* Keydown */
            switch (keydown)
            {
                case ARROW_UP: KEYBD[PA2] &= 0xf7; break;
                case ARROW_DOWN: KEYBD[PA2] &= 0xfd; break;
                case ARROW_LEFT: KEYBD[PA2] &= 0xdf; break;
                case ARROW_RIGHT: KEYBD[PA2] &= 0xfb; break;
                case FIRE1: KEYBD[PA3] &= 0x7f; break;
                case FIRE2: KEYBD[PA2] &= 0x7f; break;
            }

            switch (keyup)
            {
                case ARROW_UP: KEYBD[PA2] |= 0x08; break;
                case ARROW_DOWN: KEYBD[PA2] |= 0x02; break;
                case ARROW_LEFT: KEYBD[PA2] |= 0x20; break;
                case ARROW_RIGHT: KEYBD[PA2] |= 0x04; break;
                case FIRE1: KEYBD[PA3] |= 0x80; break;
                case FIRE2: KEYBD[PA2] |= 0x80; break;
            }

        }
        else
        {
            switch (keydown)
            {
                case RESET: return 999;
                case SELECT: KEYBD[PA1] &= 0xf5; break;
                case ARROW_UP: KEYBD[PA0] &= 0xf7; break;
                case ARROW_DOWN: KEYBD[PA0] &= 0xfd; break;
                case ARROW_LEFT: KEYBD[PA0] &= 0xdf; break;
                case ARROW_RIGHT: KEYBD[PA0] &= 0xfb; break;
                case FIRE1: KEYBD[PA0] &= 0x7f; break;
                case FIRE2: KEYBD[PA1] &= 0x7f; break;
            }

            switch(keyup)
            {
                case SELECT: KEYBD[PA1] |= 0x0a; break;
                case ARROW_UP: KEYBD[PA0] |= 0x08; break;
                case ARROW_DOWN: KEYBD[PA0] |= 0x02; break;
                case ARROW_LEFT: KEYBD[PA0] |= 0x20; break;
                case ARROW_RIGHT: KEYBD[PA0] |= 0x04; break;
                case FIRE1: KEYBD[PA0] |= 0x80; break;
                case FIRE2: KEYBD[PA1] |= 0x80; break;
            }
        }
    } else {
        if (ret == -1) EmuQuit = 1;
    }

    return ret;
}
