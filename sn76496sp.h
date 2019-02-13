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
 ****************************************************************************/
void SN76496SPWrite(int chip, int data);
int SN76496SPInit(int chip, int clock, int gain, int sample_rate);
void SN76496SPUpdate(int chip, short *buffer, int len);
