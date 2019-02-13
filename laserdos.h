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
 * laserdos.h
 *
 * Disk ROM interface header
 ****************************************************************************/
#ifndef _LASER_DOS_
#define _LASER_DOS_

#define NUM_TRACKS 40
#define SECTORS_PER_TRACK 14
#define TRACK_SIZE 5632
#define DISK_SIZE (TRACK_SIZE * NUM_TRACKS)

#endif // _LASER_DOS_
