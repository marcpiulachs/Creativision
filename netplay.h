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
#ifndef NETPLAY_H_INCLUDED
#define NETPLAY_H_INCLUDED

/* Keyboard messages */
#define FIRE1       1
#define FIRE2       2
#define ARROW_UP    4
#define ARROW_DOWN  8
#define ARROW_LEFT  16
#define ARROW_RIGHT 32
#define RESET       64
#define SELECT      128

/* Network */
#define NP_PORT     54321

typedef struct {
    unsigned char keydown;
    unsigned char keyup;
    unsigned long long int ticks;
} __attribute__((__packed__)) NP_DATA;

/* Protos */
int NP_Init(void);
int NP_ShareMemory(void);
int NP_check_input();
int NP_RemoteKeys();

#endif // NETPLAY_H_INCLUDED
