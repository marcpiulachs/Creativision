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
 * joystick.c
 *
 * Handle SDL Joysticks
 ****************************************************************************/
#include "crvision.h"

SDL_Joystick *joys[2];

/**
 * InitJoyticks
 *
 * Try to open a joystick.
 */
void InitJoysticks(int num)
{
    joys[0] = SDL_JoystickOpen(0);
    if (joys[0]==NULL) {
        fprintf(stderr,"!!Failed to initialise joystick 0\n");
    }
}

/**
 * SetJoyXY
 */
void SetJoyXY(SDL_Event event)
{
    //Handle joystick 0
    if (event.jaxis.which == 0) {
        if ( event.jaxis.axis == 0) { // Left tight
            if ( (event.jaxis.value > -8000) && (event.jaxis.value < 8000) )
                KEYBD[PA0] |= 0x24;
            else {
                if ( event.jaxis.value < 0 )
                    KEYBD[PA0] &= 0xdf;    // Left
                else
                    KEYBD[PA0] &= 0xfb;    // Right
            }
        } else {
            if ( event.jaxis.axis == 1) { // Up - down
                if ((event.jaxis.value > -8000) && (event.jaxis.value < 8000) )
                    KEYBD[PA0] |= 0x0A;
                else {
                    if (event.jaxis.value < 0)
                        KEYBD[PA0] &= 0xf7;    // Down
                    else
                        KEYBD[PA0] &= 0xfd;    // Up
                }
            }
        }
    } else {
        if ( event.jaxis.which == 1) {

        }
    }
}

/**
 * SetButtonDown
 */
void SetButtonDown(SDL_Event event)
{
    if ( event.jbutton.which == 0 ) {
        if ( event.jbutton.button == 0)
            KEYBD[PA1] &= 0x7f;    // Left Button
        else
            KEYBD[PA0] &= 0x7f;
    }
}

/**
 * SetButtonUP
 */
void SetButtonUp(SDL_Event event)
{
    if (event.jbutton.which == 0) {
        if (event.jbutton.button == 0)
            KEYBD[PA1] |= 0x80;
        else
            KEYBD[PA0] |= 0x80;
    }
}
