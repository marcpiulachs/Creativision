/****************************************************************************
 *                                   \ /
 *                        C R E A T I V I S I O N
 *                            E M U L A T O R
 *                                  0.2.9
 *
 *       This emulator was written using Marat Fayzullin's Emulib
 *
 *       *YOU MAY NOT DISTRIBUTE THIS FILE COMMERCIALLY. PERIOD!*
 *---------------------------------------------------------------------------
 * Sound Interface
 *
 * This file takes care of SN76498 and SDL.
 *
 * Function names mirror those of the original Mame driver for compatibility.
 *
 * Converted from
 *
 *  SonicPlayer
 *  www.paulsprojects.net
 *
 *  Copyright (c) 2008, Paul Baker
 *  All rights reserved.
 *
 ****************************************************************************/
#include "crvision.h"

/*
 * When in CSL mode we were missing the "click" of the keys.
 */
static void SN76496SPWriteReal(int chip, int data);
#define MAX_EVENTS 4096
static int evcounter = 0;
static uint8_t eventdata[MAX_EVENTS];
static uint64_t eventclock[MAX_EVENTS];
static uint64_t lastclock;
static int updating = 0;

static int16_t volumeTable[] = {
    32767, 26028, 20675, 16422, 13045, 10362, 8231, 6538,
    5193, 4125, 3277, 2603, 2067, 1642, 1304, 0
};

static int16_t volumeTones[16];
static int16_t volumeNoise[16];
SN76496SP psg;

/****************************************************************************
 * Local support functions
 ****************************************************************************/
static uint16_t Parity(uint16_t val)
{
    val ^= (uint16_t)(val >> 8);
    val ^= (uint16_t)(val >> 4);
    val ^= (uint16_t)(val >> 2);
    val ^= (uint16_t)(val >> 1);

    return (uint16_t)(val & 1);
}

static void CalculateToneSamples(int32_t channel, int32_t numSamples,
                                 int16_t *samples)
{
    int i;
    int16_t sample;

    for (i = 0; i < numSamples; ++i)
    {
        if (psg.toneRegisters[channel] < 2)
        {
            psg.tonePhaseCounters[channel] = 0.0f;
            sample=(int16_t)(volumeTones[psg.volumeRegisters[channel]]);
        }
        else
        {
            psg.tonePhaseCounters[channel] -= psg.tonePhase;
            while(psg.tonePhaseCounters[channel] < 0.0f )
            {
                psg.tonePhaseCounters[channel] += (double)psg.toneRegisters[channel];
                psg.toneOutputs[channel] = (uint8_t)(1 - psg.toneOutputs[channel]);
            }

            sample = (int16_t)((2 * psg.toneOutputs[channel] - 1) *
                               volumeTones[psg.volumeRegisters[channel]]);
        }

        samples[i] += sample;

    }
}

static void CalculateNoiseSamples(int32_t numSamples, int16_t *samples)
{
    int i;
    int16_t sample;
    uint16_t noiseLFSRInput;

    for (i = 0; i < numSamples; ++i)
    {
        psg.noisePhaseCounter -= psg.tonePhase;

        while(psg.noisePhaseCounter < 0.0f)
        {
            switch(psg.noiseRegister & 3)
            {
            case 0:
                psg.noisePhaseCounter += 16.0f;
                break;
            case 1:
                psg.noisePhaseCounter += 32.0f;
                break;
            case 2:
                psg.noisePhaseCounter += 64.0f;
                break;
            case 3:
                psg.noisePhaseCounter +=
                    (psg.toneRegisters[2] != 0) ? (double)psg.toneRegisters[2] : 1.0f;
                break;
            }

            psg.noiseOutput = (uint8_t)(1 - psg.noiseOutput);

            if (psg.noiseOutput == 1)
            {
                psg.noiseLFSROutput = (uint8_t)(psg.noiseLFSR & 1);

                if (psg.noiseRegister & 4)
                {
                    noiseLFSRInput = Parity((uint16_t)(psg.noiseLFSR & psg.whiteNoiseFeedback));
                }
                else
                {
                    noiseLFSRInput = (uint16_t)(psg.noiseLFSR & 1);
                }

                psg.noiseLFSR >>= 1;
                psg.noiseLFSR |= (uint16_t)(noiseLFSRInput << (psg.shiftRegisterWidth -1));
            }
        }

        sample = (int16_t)(psg.noiseLFSROutput * volumeNoise[psg.volumeRegisters[3]]);
        samples[i] += sample;
    }
}

/**********************************************************************
* CalculateSamples
*
* Normally, collecting the state-of-play every frame, 20ms, is ok
* However, for CSL and BASIC82/83 it means you miss individual sounds.
***********************************************************************/
static void CalculateSamples(int32_t numSamples, int16_t *sampleBuffer)
{
    double clocksPerSample;
    int64_t currentclock, firstclock;
    int32_t channel;
    int32_t i,k;

    if (evcounter)
    {
        currentclock = firstclock = eventclock[0];
        clocksPerSample = (double)(total_cycles - lastclock) / (double)numSamples;

        for( i = 0, k = 0; i < numSamples; i++)
        {
            /* Do psg events upto this clock time */
            if ( k != evcounter)
            {
                while (currentclock <= (firstclock + ((double)(i+1) * clocksPerSample)))
                {
                    if (eventclock[k] == currentclock)
                        SN76496SPWriteReal(0,eventdata[k++]);

                    currentclock++;
                }

                for (channel = 0; channel < 3; channel++)
                {
                    CalculateToneSamples(channel, 1, sampleBuffer + i);
                }
                CalculateNoiseSamples(1, sampleBuffer + i);
            }
            else
            {
                for (channel = 0; channel < 3; channel++)
                {
                    CalculateToneSamples(channel, numSamples - i, sampleBuffer + i);
                }
                CalculateNoiseSamples(numSamples - i, sampleBuffer + i);
                break;
            }
        }

        /* Clean up where events overwhelmed sample rate */
        while (k < evcounter)
        {
            SN76496SPWriteReal(0, eventdata[k++]);
        }
    }
    else
    {
        /* No events - same as last */
        for (channel = 0; channel < 3; channel++)
        {
            CalculateToneSamples(channel, numSamples, sampleBuffer);
        }
        CalculateNoiseSamples(numSamples, sampleBuffer);
    }
}

/****************************************************************************
 * SN76496SPUpdate
 ********************** *****************************************************/
void SN76496SPUpdate(int32_t chip, int16_t *buffer, int32_t len)
{
    memset(buffer, 0, len << 1);
    updating = 1;
    CalculateSamples(len, buffer);
    evcounter = updating = 0;
    lastclock = total_cycles;
}

/****************************************************************************
 * SN76496SPWrite
 ****************************************************************************/
void SN76496SPWrite(int chip, int data)
{
    /* Wait for sound update - synchronises */
    while (updating == 1) {
        SDL_Delay(1);
    }
    eventclock[evcounter] = total_cycles;
    eventdata[evcounter++] = data;
    evcounter &= (MAX_EVENTS-1);
}

/****************************************************************************
 * SN76496SPWrite
 ****************************************************************************/
static void SN76496SPWriteReal(int chip, int data)
{
    if ( data & 0x80 )
    {
        psg.latchedChannel = (uint8_t)((data & 0x60) >> 5);
        psg.volumesLatched = (data & 0x10);

        if (psg.volumesLatched)
        {
            psg.volumeRegisters[psg.latchedChannel] = (uint8_t)(data & 0x0f);
        }
        else if(psg.latchedChannel < 3)
        {
            psg.toneRegisters[psg.latchedChannel] &= 0x3f0;
            psg.toneRegisters[psg.latchedChannel] |= (uint16_t)(data & 0x0f);
        }
        else
        {
            psg.noiseRegister = (uint8_t)(data & 0x07);
            psg.noiseLFSR = 0x8000;
        }
    }
    else
    {
        if(psg.volumesLatched)
        {
            psg.volumeRegisters[psg.latchedChannel] = (uint8_t)(data & 0x0f);
        }
        else if (psg.latchedChannel < 3)
        {
            psg.toneRegisters[psg.latchedChannel] &= 0xf;
            psg.toneRegisters[psg.latchedChannel] |= (uint16_t)((data & 0x3f)<<4);
        }
        else
        {
            psg.noiseRegister = (uint8_t)(data & 0x7);
            psg.noiseLFSR = 0x8000;
        }
    }
}

/****************************************************************************
 * SN7649SPInit
 *
 * Reset or initialise the PSG
 ****************************************************************************/
int SN76496SPInit(int chip, int clock, int gain, int sample_rate)
{
    int i;

    for (i = 0; i < 15; i++)
    {
        volumeTones[i] = volumeTable[i] >> 2;
        volumeNoise[i] = volumeTones[i] * 0.7f;
    }

    /* Get external values */
    psg.psg_clock = clock;
    psg.sampleRate = sample_rate;

    /* Reset to defaults */
    psg.latchedChannel = 0;
    psg.volumesLatched = 0;
    psg.volumeRegisters[0] = 0x0f;
    psg.volumeRegisters[1] = 0x0f;
    psg.volumeRegisters[2] = 0x0f;
    psg.volumeRegisters[3] = 0x0f;
    psg.toneRegisters[0] = 0;
    psg.toneRegisters[1] = 0;
    psg.toneRegisters[2] = 0;
    psg.noiseRegister = 0;

    psg.tonePhaseCounters[0] = 0.0f;
    psg.tonePhaseCounters[1] = 0.0f;
    psg.tonePhaseCounters[2] = 0.0f;
    psg.toneOutputs[0] = 0;
    psg.toneOutputs[1] = 0;
    psg.toneOutputs[2] = 0;

    psg.noisePhaseCounter = 0.0f;
    psg.noiseOutput = 0;
    psg.whiteNoiseFeedback = 0xf35;
    psg.shiftRegisterWidth = 14;
    psg.noiseLFSR = (uint16_t)(1 << (psg.shiftRegisterWidth - 1));
    psg.noiseLFSROutput = 0;

    psg.tonePhase = (double)psg.psg_clock / (16.0f * (double)psg.sampleRate);

    return 0;
}
