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
 * Sound Interface
 *
 * This file takes care of SN76496 and SDL.
 ****************************************************************************/
#include "crvision.h"

__attribute__((__aligned__(4))) int16_t playbuffer[PLAY_BUFFER];
static int snd_pos = 0;

SDL_AudioSpec sdl_audio;

/**
 * sdl_playaudio
 *
 * Callback function
 */
static void sdl_playaudio(void *userdata, unsigned char *stream, int len)
{
    int i;
    int16_t *ns = (int16_t *)stream;
    int samples = len >> 1;

    //printf("%d\n",len);

    for ( i = 0; i < samples; i++ ) {

        if ( snd_pos == samples ) {
            SN76496SPUpdate(0, playbuffer, samples);
            snd_pos = 0;
        }

        ns[i] = playbuffer[snd_pos++];
    }
}

/**
 * AudioInit
 *
 * Set audio for SDL
 */
int AudioInit(void)
{
    SDL_AudioSpec requested;

    SDL_memset(&requested, 0, sizeof(requested));
    SDL_memset(&sdl_audio, 0, sizeof(sdl_audio));

    /* Looking for 16-bit 22Khz Mono */
    requested.channels = 1;
    requested.format = AUDIO_S16;

#ifndef RPI
    if (sound44k) {
        requested.freq = 44100;
        requested.samples = 882;
    } else {
        requested.freq = 22050;
        requested.samples = 442;
    }
#else
    requested.freq = 48000;
    requested.samples =  960;
#endif

    requested.userdata = NULL;
    requested.callback = sdl_playaudio;

    /* Is audio available ?*/
    if(SDL_OpenAudio(&requested, &sdl_audio)<0)
    {
        fprintf(stderr,"Error opening audio device SDL: %s\n",
                SDL_GetError());

        return FAIL;
    }

    /* Initialise SN76496 */
    SN76496SPInit(0, PSG_CLOCK, 0, sdl_audio.freq);

    return SUCCESS;
}
