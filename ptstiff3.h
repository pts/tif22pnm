/* ptstiff3.h -- reads (smartly) and writes TIFF files using libtiff
 * by pts@fazekas.hu at Sun Sep 29 22:43:55 CEST 2002
 */

#ifndef PTSTIFF3_H
#define PTSTIFF3_H

#ifdef __GNUC__
#pragma interface
#endif

#include "miniglib.h"
/* #include "minigimp.h" */ /* not required */

#if PTS_USE_STATIC
#  define SAVE_ARGS
#else
#  define SAVE_ARGS , gushort compression, gchar *image_comment
#endif

/** @return image descriptor */
EXTERN_C gint32 ptstiff3_load_image  (gchar const*filename, gbool headerdump_p);
/**
 * @param filename !=NULL
 * @return 0 on error (printed with g_message(), g_logv(...)), 1 on success
 */
EXTERN_C gint   ptstiff3_save_image  (gchar     const*filename,
				      gint32     image,
				      gint32     drawable_layer,
				      gint32     orig_image SAVE_ARGS);

#endif /* end of ptstiff3.h */
