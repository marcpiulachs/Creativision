/****************************************************************************
 * Microsoft Wave File Header
 ****************************************************************************/
#ifndef __MSWAVE_HDR__
#define __MSWAVE_HDR__

typedef struct {
    long    ChunkID;            /* Always RIFF */
    long    ChunkSize;          /* Length of file, less 8 */
    long    format;             /* WAVE */
    long    subChunk1ID;        /* 'fmt ' */
    long    subChunk1Size;      /* Size of subChunk1Header, 16 */
    short   audio_format;       /* Use 1 for PCM */
    short   numchannels;        /* 1 = Mono, 2 = Stereo */
    long    samplerate;         /* 44100 etc */
    long    byterate;           /* numchannels * samplerate * bitspersample / 8 */
    short   blockalign;         /* numchannels * bitspersample / 8 */
    short   bitspersample;      /* 8, 16, 32 etc */
    long    subChunk2ID;        /* data */
    long    subChunk2Size;      /* Here to eof - 8 */
} WAVEHEADER;
#endif // __MSWAVE_HDR__
