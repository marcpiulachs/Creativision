/****************************************************************************
 *                                   \ /
 *                        C R E A T I V I S I O N
 *                            E M U L A T O R
 *                                  0.2.8
 *
 *       This emulator was written using Marat Fayzullin's Emulib
 *
 *       *YOU MAY NOT DISTRIBUTE THIS FILE COMMERCIALLY. PERIOD!*
 *---------------------------------------------------------------------------
 * libPNG support
 *
 * This file takes care of saving png images
 ****************************************************************************/
#include "crvision.h"

/**
 * WritePNG
 *
 * Output current screen to a PNG file.
 */
void WritePNG( void )
{
    char pngname[MAX_WINPATH];
    char t[MAX_WINPATH];
    struct stat s;
    char *d;
    int x,y,*p,px;
    FILE *png;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep rows;
    png_text text_ptr[1];

    strcpy(pngname, rname);
    d = strrchr(pngname, '.');
    if (d) *d = 0;

    for(x=1; x<1000; x++) {
        sprintf(t,"%s-%03d.png",pngname,x);
        if (stat(t,&s) == -1) break;
    }

    png=fopen(t,"wb");
    if (!png) {
        fprintf(stderr,"Cannot create PNG file\n");
        return;
    }

    /* Create PNG structure */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
    if (!png_ptr) {
        fprintf(stderr,"Cannot create PNG structure\n");
        return;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if(!info_ptr) {
        fprintf(stderr,"Cannot create PNG info structure\n");
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr,"Error PNG init_io\n");
        return;
    }

    png_init_io(png_ptr, png);

    text_ptr[0].key="Created by";
    sprintf(t,"%s : %s",APP_TITLE, APP_VERSION);
    text_ptr[0].text=t;
    text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
    png_set_text(png_ptr, info_ptr, text_ptr,1);

    png_set_IHDR(png_ptr, info_ptr, VDP_WIDTH, VDP_HEIGHT, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr,"Error during PNG write bytes\n");
        return;
    }

    png_set_bgr(png_ptr);

    rows = malloc(VDP_WIDTH * 4);

    p = (int *)VRAM;
    for(y=0; y<VDP_HEIGHT; y++) {
        for(x=0; x<VDP_WIDTH; x++) {
            px = p[(y*VDP_WIDTH)+x];
            px |= 0xff000000;
            memcpy(rows+(x*4),&px,4);
        }
        png_write_row(png_ptr, rows);
    }

    png_write_end(png_ptr, NULL);

    free(rows);

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr,"Error during PNG end\n");
        return;
    }

    png_write_end(png_ptr,NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(png);
}
