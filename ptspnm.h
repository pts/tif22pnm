/* ptspnm.h -- reads and writes PNM files, without external libraries
 * by pts@fazekas.hu at Sun Oct  6 19:24:10 CEST 2002
 */

#ifndef PTSPNM_H
#define PTSPNM_H

#if defined(__GNUC__) && defined(__cplusplus)
#pragma interface
#endif

#include "miniglib.h"
/* #include "minigimp.h" */ /* not required */

/**
 * @param filename may be NULLP to indicate stdin
 * @return image descriptor
 */
EXTERN_C gint32 ptspnm_load_image(gchar const* filename);

#if 0 /* overridden by GIMPTS_FF_* constants */
#define PTSPNM_SAVE_NONE 0
#define PTSPNM_SAVE_PBM 1
#define PTSPNM_SAVE_PGM 2
#define PTSPNM_SAVE_PPM 3
#define PTSPNM_SAVE_PNM 4
#endif

/** This routine saves a PNM (PBM|PGM|PPM) file in RAWBITS format.
 * @param filename may be NULLP to indicate stdout
 * @param hdrawable 0 -> gimpts_image_first_drawable(gint32 himage).
 * @param sphoto GIMPTS_FF_PNM|GIMPTS_FF_p?m|GIMPTS_FF_NONE
 * @param salpha GIMPTS_FF_PNM|GIMPTS_FF_p?m|GIMPTS_FF_NONE
 * @param rawbits save a RAWBITS (binary) PNM file? (recommended: TRUE)
 *        Imp: don't ignore this param
 * @return 0 on error (printed with g_message(), g_logv(...)), 1 on success
 */
EXTERN_C int ptspnm_save_image(gchar const* filename, gint32 himage, gint32 hdrawable, gint sphoto, gint salpha, gbool rawbits);

#endif /* end of ptspnm.h */
