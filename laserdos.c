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

#include "crvision.h"
#include "laserdos.h"

#define TRACE_SECTORS 0

/**
 * Globals
 */
static int write_mode = 0;          // Flag for current mode
static int motor_on = 0;            // Flag for motor enabled
static int data_latch = 0;          // Contains last latched byte
static int data_latched = 0;        // Flag for data latched
static int ctrack = 0;              // Current track
static int disk_updated = 0;        // Flag for writing out disk
static byte disk_buffer[DISK_SIZE];    // Memory block for current disk
typedef struct{
    int pos;
    unsigned char *buffer;
}TRACK_BUFFER;

TRACK_BUFFER tb={0, disk_buffer};

#if TRACE_SECTORS
    static byte currSector[14];         // Try to determine current sector ID
    static byte v,s,t;
#endif // TRACE_SECTORS

/**
 * load_disk_image
 */
int load_disk_image( char *img )
{
    struct stat s;
    int bytes;
    FILE *f;

    /* Also load the disk rom */
    f = fopen("bios/disk.rom","rb");
    if (!f) return 0;

    bytes = fread(RAM + 0x1200, 1, 0x1F00, f);
    fclose(f);

    /* Clear memory buffer */
    memset(disk_buffer, 0xff, DISK_SIZE);

    if (stat(img,&s) != -1)
    {
        f = fopen(img,"rb");
        if (f)
        {
            bytes = fread(disk_buffer, 1, DISK_SIZE, f);
            fclose(f);

            if (bytes != DISK_SIZE) return 0;
        }
    }

    return 1;

}

/**
 * write_disk_image
 */
void write_disk_image(char *fname)
{
    FILE *f;
    int bytes;

    if (!disk_updated) return;

    f=fopen(fname,"wb");
    if (!f) {
        fprintf(stderr,"*ERROR* cannot open %s for writing!\n",fname);
        return;
    }

    bytes = fwrite(disk_buffer, 1, DISK_SIZE, f);
    fclose(f);

    if (bytes != DISK_SIZE) {
        fprintf(stderr,"*ERROR* bad write to %s needed %d, got %d\n", fname, DISK_SIZE, bytes);
    }

}

/**
 * step_motor
 */
byte step_motor( int phase )
{
    static int prev_phase = 0;          // Start at phase 0
    static int direction = 0;           // Start moving 0->39

    if (!motor_on) return(0x80);        // No motor

    switch(phase)
    {
        case 0:
            if (prev_phase == 3) direction=0;
            else direction=1;
            break;

        case 1:
            if (prev_phase == 0 ) direction=0;
            else direction=1;
            break;

        case 2:
            if (prev_phase == 1)  direction=0;
            else direction=1;
            break;

        case 3:
            if (prev_phase == 2) direction=0;
            else direction=1;
    }

    if (direction) ctrack--;
    else ctrack++;

    if (ctrack > 79) ctrack=79;
    else if(ctrack<0) ctrack=0;

    prev_phase = phase;
    tb.buffer = disk_buffer + ((ctrack>>1) * TRACK_SIZE);
    tb.pos = 0;
	
	return 0;
}

/**
 * laserdos_read
 */
byte laserdos_read(word addr)
{
    switch(addr)
    {
        case 0x1128:        // Onlu called at startup
            write_mode = 0;
            break;

        case 0x1140:        // Check write protect bit - always off
            return 0x00;       // Set 0x80 to signal write protect

        case 0x1180:        // Phase 0 off
        case 0x1182:        // Phase 1 off
        case 0x1184:        // Phase 2 off
        case 0x1186:        // Phase 3 off
            break;

        case 0x1181:        // Phase 0 on
            return step_motor(0);
        case 0x1183:        // Phase 1 on
            return step_motor(1);
        case 0x1185:        // Phase 2 on
            return step_motor(2);
        case 0x1187:        // Phase 3 on
            return step_motor(3);

        case 0x1188:         // Motor off
            motor_on = write_mode = 0;
            tb.pos = 0;
            break;

        case 0x1189:        // Select drive 1
            return 0;

        case 0x118a:        // Motor on
            motor_on = 1;
            break;

        case 0x118c:        // Read or shift
            if (write_mode) {
                if(data_latched){
                    disk_updated = 1;
                    tb.buffer[tb.pos++] = data_latch;
                    data_latched = 0;
                    if (tb.pos == TRACK_SIZE) tb.pos=0;
                    return data_latch;
                }
            }
            else
            {
                if (tb.pos == TRACK_SIZE) tb.pos=0;
#if TRACE_SECTORS
                memcpy(currSector, currSector+1, 13);
                currSector[13]=tb.buffer[tb.pos];
                if ((currSector[0]==0xd5) && (currSector[1]==0xaa) && (currSector[2]==0x96))
                {
                    v = currSector[3]<<1;
                    if (currSector[3]&0x80) v |= 1;
                    v = v & currSector[4];

                    t = currSector[5]<<1;
                    if (currSector[5]&0x80) t |= 1;
                    t = t & currSector[6];

                    s = currSector[7]<<1;
                    if (currSector[7]&0x80) s |= 1;
                    s = s & currSector[8];

                    printf("%03d/%02d/%02d - %04X\n",v,t,s,CPU.PC.W);
                }
#endif // TRACE_SECTORS
                return tb.buffer[tb.pos++];
            }
            break;

        case 0x118d:        // Enable write mode
            break;

        case 0x118e:        // Read mode
            write_mode = 0;
            break;

        default:
            printf("DROM-UR: %04X\n",addr);
            break;
    }

    return 0;
}

/**
 * laserdos_write
 */
void laserdos_write(word addr, byte data)
{
    switch(addr)
    {
        case 0x118d:        // Output byte
            if(write_mode) {
                data_latch = data;
                data_latched = 1;
            }
            break;

        case 0x118f:        // Set write mode
            write_mode = 1;
            break;

        default:
            printf("DROM-UW: %04X\n",addr);
            break;
    }
}
