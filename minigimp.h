/* minigimp.h -- extremely basic image manipulation routines
 * by pts@fazekas.hu at Sun Sep 29 22:33:48 CEST 2002
 */

#ifndef MINIGIMP_H
#define MINIGIMP_H

#if defined(__GNUC__) && defined(__cplusplus)
#pragma interface
#endif

#include "miniglib.h"

/** The MiniGIMP image model:
 * -- An image is a container of layers and channels. There are three image
 *    types: GIMP_RGB, GIMP_GRAY and GIMP_INDEXED. channels are of drawable
 *    type GIMP_CHANNEL, and they contain intensity (not color). layers 
 *    contain color and opacity information; the layer type of the layers is
 *    limited by the image type. For the image type GIMP_INDEXED, all layers
 *    share the same cmap (color map, palette, lookup-table, LUT). An image
 *    never contains pixel data
 *    directly, individual pixels are stored in layers and channels.
 *    Valid combinations for layers and images are:
 *
 *      image type    layer type           image cmap         image ncolors
 *      ~~~~~~~~~~    ~~~~~~~~~~           ~~~~~~~~~~         ~~~~~~~~~~~~~
 *      GIMP_RGB      GIMP_RGB_IMAGE       NULL               -2
 *      GIMP_GRAY     GIMP_GRAY_IMAGE      NULL               -1
 *      GIMP_INDEXED  GIMP_INDEXED_IMAGE   guchar[3*ncolors]  ncolors
 *      GIMP_RGB      GIMP_RGBA_IMAGE      NULL               -2
 *      GIMP_GRAY     GIMP_GRAYA_IMAGE     NULL               -1
 *      GIMP_INDEXED  GIMP_INDEXEDA_IMAGE  guchar[3*ncolors]  ncolors
 *
 * -- Channels are not considered when displaying the image. Layers are
 *    considered in order: the bottommost layer is displayed first, and other
 *    layers are drawn over it with their respective opacity. Note most image
 *    file export routines in the current implementation
 *    completely ignore channels, and respect only the
 *    first layer (i.e the layer created first by gimp_layer_new()) when
 *    saving images.
 *
 * -- A drawable is a rectangular array of pixels with positive integer
 *    width and height. The drawable is identified by its handle (a positive
 *    integer). After a drawable is created, it should be quickly added to
 *    an image.
 *
 * -- GimpDrawable is a C struct that contains meta-information of a
 *    drawable, such as width, height, bpp (bytes per pixel). A GimpDrawable
 *    never contains pixel data. gimp_drawable_get() can be used to get
 *    the GimpDrawable of a hdrawable, and gimp_pixel_rgn_init() can be used
 *    to get access to the image pixel data in memory.
 *
 * -- A layer is a special drawable that contains color and opacity
 *    information. For drawable type GIMP_RGB{,A}_IMAGE each color is from
 *    the 8-bit RGB color space. For drawable type GIMP_GRAY{,A}_IMAGE each
 *    color is from the 8-bit grayscale color space (0 is black). For
 *    drawable type GIMP_INDEXED{,A}_IMAGE each color is an 8-bit index to
 *    the 8-bit RGB palette colormap (cmap) of the corresponding image.
 *    Opacity information (if present) is stored as an 8-bit alpha value: 255
 *    is fully opaque, 0 is transparent, middle values represent fuzzy
 *    transparency (useful for antialiasing). RGB colors are documented as
 *    hex triplets of the form #RRGGBB (i.e #7f0000 is midlle-red), RGBA pixel
 *    values are documented as hex tuples of the form #RRGGBB#AA.
 *
 * -- Opacity is stored in memory
 *    as so-called ``unassociated alpha'' data, i.e #00007f#ff is
 *    opaque dark-blue, #RRGGBB#00 is a transparent pixel of whatever color,
 *    #00007f#7f is half-opaque dark-blue. In the contrary, in _associated_
 *    alpha, #00007f#7f means half-opaque full-blue. The PNG file format
 *    stores alpha this way (unassociated), TIFF allows both associated and
 *    unassocitated, but most TIFF viewers support only associated, so TIFF
 *    export routines should call gimpts_drawable_assoc() first.
 *
 * -- A channel is a special drawable that contains arbitrary intensity
 *    information, except for color and opacity. The term ``alpha channel''
 *    is misleading, because alpha (opacity) information is part of layers,
 *    and has nothing to do with channels. Examples for a channel: floating
 *    selection; selection mask of the face on the Mona Lisa picture. A
 *    maximal (white) value represents fully selected, a zero (balck)
 *    represents unselected, and mid-gray values represent fuzzy membership.
 */

#undef  PTS_USE_PDB /* use GIMP Procedural DataBase (PDB) interface? */
#undef  PTS_USE_GUI /* use GUI elements (dialog boxes, progress indicators)? -- yes doesn't work */
#undef  PTS_USE_GIMPRGB
#undef  PTS_USE_PROGRESS

typedef unsigned char GimpImageType; /* vvv */
#define GIMP_RGB 6 /* :image_type */
#define GIMP_GRAY 7 /* :image_type */
#define GIMP_INDEXED 8 /* :image_type */
/** PtsGIMP extension */
#define GIMP_CHANNEL 10 /* :channel_type (must be even) */
#define GIMP_RGB_IMAGE 12 /* :layer_type */
#define GIMP_RGBA_IMAGE 13 /* :layer_type */
#define GIMP_GRAY_IMAGE 14 /* :layer_type */
#define GIMP_GRAYA_IMAGE 15 /* :layer_type */
/** Even */
#define GIMP_INDEXED_IMAGE 16 /* :layer_type */
/** Odd */
#define GIMP_INDEXEDA_IMAGE 17 /* :layer_type */
#define IS_DRAWABLE_ALPHA(type) ((type&1)!=0)

typedef struct GimpDrawable {
  gint32 handle;
  guint32 width, height;
  /** 0.0 .. 100.0 */
  gdouble opacity;
  /** Bytes per pixel. */
  unsigned char bpp;
  /** GIMP_{RGB,GRAY,INDEXED}{,A}_IMAGE */
  char const *name;
  GimpImageType type;
  /** Bits per component. PtsGIMP extension. For normal drawables,
   * bpc==8 is true, but for outdatas, it isn't!
   */
  unsigned char bpc;
  /** Always GIMP_NORMAL_MODE */
  unsigned char mode;
} GimpDrawable;

/** Dat: cannot hide this struct from interface, because ptstiff3.c declares
 * variables as struct `GimpPixelRgn pixel_rgn;'
 */
typedef struct GimpPixelRgn {
  /** Memory index of top-left pixel */
  guchar *topleft;
  /** GIMP_{RGB,GRAY,INDEXED}{,A}_IMAGE */
  GimpImageType type;
  /** Bytes per pixel. (`type' -> `bpp') */
  unsigned char bpp;
  /** Bytes per scanline. */
  gsize_t bpl;
  /** Bytes to be skipped after each scanline */
  /* gsize_t skip; */
  guint32 width, height;
  /** Minimum coordinates into which drawing is possible. */
  guint32 minx, miny;
} GimpPixelRgn;

typedef struct GimpParasite {
  /* gchar *name; */ /* points to (this)+1 */
  gpointer data;
  gchar *name;
  gsize_t data_size;
  slen_t refc;
  /** Curcularly linked list */
  struct GimpParasite *prev, *next;
} GimpParasite;

/** Holds meta-information about an image or drawable */
typedef struct GimptsMeta {
  /** 0 for opaque drawable, 1|2|4|8 otherwise */
  guchar minAlphaBpc;
  /** 1|2|4|8 otherwise */
  guchar minRGBBpc;
  /** false iff RGB is required */
  gbool canGray;
  /** Number of unique (different) colors in the non-transparent part,
   * only about the last drawable. 257 for too many.
   */
  guint nncols;
  /** About the last drawable. */
  guint32 width, height;
} GimptsMeta;

typedef unsigned char GimptsFileFormat;
#define GIMPTS_FF_UNKNOWN 0 /* may be a valid image, but we don't know */
#define GIMPTS_FF_INVALID 1 /* cannot be an image file */
#define GIMPTS_FF_NONE 2
#define GIMPTS_FF_TIFF 3
#define GIMPTS_FF_PCX 4
#define GIMPTS_FF_JPEG 5
#define GIMPTS_FF_GIF 6
#define GIMPTS_FF_XPM 7
#define GIMPTS_FF_TGA 8
#define GIMPTS_FF_PNG 9
#define GIMPTS_FF_LBM 10
#define GIMPTS_FF_BMP 11
#define GIMPTS_FF_eps 12
#define GIMPTS_FF_ps 13
#define GIMPTS_FF_pbm 14
#define GIMPTS_FF_pgm 15
#define GIMPTS_FF_ppm 16
#define GIMPTS_FF_PNM 17

typedef unsigned char GimptsTransferEncoding;
#define GIMPTS_TE_UNKNOWN 0
#define GIMPTS_TE_BINARY 1
#define GIMPTS_TE_HEX 2
#define GIMPTS_TE_A85 3
#define GIMPTS_TE_ASCII 4
#define GIMPTS_TE_MSBFIRST 5
#define GIMPTS_TE_LSBFIRST 6

#if PTS_USE_GIMPRGB
typedef struct GimpRGB {
  unsigned char rrr, ggg, bbb;
} GimpRGB;
EXTERN_C void gimp_rgb_set(GimpRGB *color, gfloat r, gfloat g, gfloat b);
#else
typedef struct GimpRGB GimpRGB;
#endif

/** Creates a new channel (not belonging to any image) from the alpha values
 * of the specified drawable. The returned channel can be freed later by
 * calling gimp_drawable_free(). Creates the channel width all-255 values
 * even if !IS_DRAWABLE_ALPHA(gimp_drawable_type(hdrawable)).
 */
EXTERN_C /*hchannel*/gint32 gimpts_drawable_extract_alpha(gint32 hdrawable);
EXTERN_C void gimpts_meta_init(GimptsMeta *meta);
/** The caller should get rid of the alpha channel to make .minRGBBpc reflect
 * only the status of the fully opaque pixels. Sets canGray to false if the
 * cmap for indexed images contains a non-gray color; you have to call
 * _packpal() first to get rid of unused colors. Incorporates unused colors
 * into .nncols.
 *  @param meta the .minAlphaBPC, .minRGBBpc and .canGray fields are made
 *    stricter. .nncols, .width and .height are set.
 */
EXTERN_C void gimpts_meta_update(GimptsMeta *meta, gint32 hdrawable);
/** refc++ */
EXTERN_C GimpParasite* gimp_image_parasite_find (gint32 orig_image, char const* /*"gimp-comment"*/);
/** refc--, destructor */
EXTERN_C void gimp_parasite_free(GimpParasite*);
/** inline gsize_t gimp_parasite_data_size(GimpParasite *parasite); */
#define gimp_parasite_data_size(parasite) ((parasite)->data_size)
/** inline guchar *gimp_parasite_data(GimpParasite *parasite); */
#define gimp_parasite_data(parasite) ((parasite)->data)
/** by pts */
#define gimp_parasite_name(parasite) ((gchar*)(void*)(parasite+1))
#define GIMP_PARASITE_PERSISTENT 1 /* :gimp_parasite_new(opts) */
EXTERN_C GimpParasite *gimp_parasite_new(char const*name, unsigned opts, gsize_t size, gpointer data);
/** The attach function automatically
 * detaches a previous incarnation of the parasite.
 */
EXTERN_C void gimp_image_parasite_attach(gint32 image, GimpParasite *parasite);

EXTERN_C char *gimp_get_default_comment(void);
/** needed by PDB only, but we include here nevertheless */
EXTERN_C void gimp_image_delete (gint32 image);
EXTERN_C GimpImageType gimp_drawable_type(gint32 hdrawable);
/** PtsGIMP extension */
EXTERN_C GimpImageType gimp_image_type(gint32 himage);

EXTERN_C void gimp_quit (void);
EXTERN_C gint32 gimp_image_new(gint cols, gint rows, gint image_type);
EXTERN_C void gimp_image_set_filename(gint32 image, char const* filename);
typedef unsigned GimpUnit; /* vvv */
#define GIMP_UNIT_INCH 1
#define GIMP_UNIT_MM 2
#define GIMP_UNIT_PIXEL 3
#define GIMP_UNIT__MAX 3
EXTERN_C void gimp_image_set_resolution (gint32 image, gdouble xres, gdouble yres);
EXTERN_C void gimp_image_set_unit (gint32 image, GimpUnit unit);
/** @param cmap cmap[0] is red, cmap[1] is green, cmap[2] is blue
 * @param ncolors must be <=0x300
 */
EXTERN_C void gimp_image_set_cmap (gint32 image, guchar *cmap, guint ncolors);
/** @param ncolors out, may be NULLP
 * @return NULLP if not GIMP_INDEXED, or there is no colormap yet
 */
EXTERN_C guchar* gimp_image_get_cmap(gint32 image, gint *ncolors);
EXTERN_C void gimp_image_get_resolution (gint32 orig_image, gdouble *xresolution, gdouble *yresolution);
EXTERN_C GimpUnit gimp_image_get_unit (gint32 orig_image);
/**
 * @param image ignored
 * @return number to multiply a dimen in inches to get the dimen in param
 * `units', 0 on unknown unit
 */
EXTERN_C gint32 gimp_layer_new (gint32 himage, char const*name /*_("Background")*/, guint32 cols, guint32 rows, gint layer_type, gdouble opacity /*100*/, unsigned mode);
#define GIMP_NORMAL_MODE 1 /* ^^^ */
EXTERN_C void gimp_image_add_layer(gint32 himage, gint32 hlayer, unsigned dummy);
/**
 * @param image ignored
 * @param color ignored
 */
EXTERN_C gint32 gimp_channel_new(gint32 image, char const*name /*_("TIFF Channel")*/, guint32 cols, guint32 rows, gdouble opacity, GimpRGB *color);
/** Dat: return type should be GimpDrawable const* */
EXTERN_C GimpDrawable* gimp_drawable_get(gint32 hdrawable);
EXTERN_C void gimp_image_add_channel(gint32 image, gint32 hchannel, unsigned dummy);
/** ?? synchronize data with server */
EXTERN_C void gimp_drawable_flush (GimpDrawable* drawable);
/** refc--, destructor for the GimpDrawable struct. Currently no-op because
 * the struct is used internally.
 */
EXTERN_C void gimp_drawable_detach(GimpDrawable* drawable);
/** PtsGIMP */
EXTERN_C void gimp_drawable_free(gint32 hdrawable);
/** Initializes a pixel region, so gimp_pixel_rgn_set_rect(pixel_rgn, ...,
 * startcol, startrow, cols, rows); can draw to it.
 */
EXTERN_C void gimp_pixel_rgn_init (GimpPixelRgn *pixel_rgn, GimpDrawable *drawable, guint32 startcol, guint32 startrow, guint32 cols, guint32 rows, gbool writeAllowed /* ?? */, gbool alwaysFALSE);
/** PtsGIMP. Initializes a pixel region, so gimp_pixel_rgn_set_rect(pixel_rgn, ...,
 * 0, 0, cols, rows); can draw to it.
 */
EXTERN_C void gimp_pixel_rgn_init_rel (GimpPixelRgn *pixel_rgn, GimpDrawable *drawable, guint32 startcol, guint32 startrow, guint32 cols, guint32 rows, gbool writeAllowed /* ?? */, gbool alwaysFALSE);
EXTERN_C void gimp_pixel_rgn_set_rect(GimpPixelRgn *pixel_rgn, guchar const*src, guint32 x, guint32 y, guint32 width, guint32 height);
#if PTS_USE_PROGRESS
  EXTERN_C void gimp_progress_init(char const*);
  EXTERN_C void gimp_progress_update(gdouble per1);
#else
  #define gimp_progress_init(dummy)
  #define gimp_progress_update(dummy)
#endif
EXTERN_C void gimp_pixel_rgn_get_rect(GimpPixelRgn *pixel_rgn, guchar *pixels, guint32 startcol, guint32 startrow, guint32 cols, guint32 rows);
/** PtsGIMP. Gets the pixels at the specified bits per component. Rows are
 * padded to byte boundary, unused bits are set to zero. Bytes are filled
 * bit-128-first. The first byte of a row belongs to the leftmost pixels.
 * Equivalent to gimp_pixel_rgn_get_rect() if dstbpc==8 and yflip==false. 
 * @param dstbpc bits per component (1, 2, 4 or 8) in which the image must be
 *   put into `dst'.
 * @param yflip should line 0 (top line in pixel_rgn) be the bottom line in
 *   the returned data?
 */
EXTERN_C void gimp_pixel_rgn_get_rect_bpc(GimpPixelRgn *pixel_rgn, guchar *pixels, guint32 startcol, guint32 startrow, guint32 cols, guint32 rows, guchar bpc, gbool yflip);
/** @param colors returns the number of colors in cmap.
 * @return NULLP if 
 */
EXTERN_C gdouble gimp_unit_get_factor (GimpUnit unit);
/** @return suggested value for the height of the chunk in which an image
 * should be read. It would be a bad idea to just return int.max here, because
 * most routines pre-allocate this much data, even if the image is smaller;
 * and most images are smaller than 32767 lines. So we return 256 here.
 */
#define gimp_tile_height() ((guint32)256)
/* EXTERN_C guint32 gimp_tile_height(void); */
#if 0
EXTERN_C void gimpts_rgb2indexed(guchar *dst, guchar *dstend, guint *ncolors_ret, guchar cmap_ret[768], guchar *src, gbool srcalpha);
EXTERN_C void gimpts_rgba2indexed(guchar *dst, guchar *dstend, guint *ncolors_ret, guchar cmap_ret[768], guchar *src);
#endif
/** Converts RGB image data to exact indexed.
 * @param ncolors_ret number of colors required, or 0 if impossible to convert
 * @param cmap_ret 768 bytes preallocated
 * @param dst dstend-dst bytes preallocated (or NULLP)
 * @param src (dstend-dst)*3 bytes preallocated
 * @param adst 1 or 2(alpha)
 * @param asrc 3 or 4(alpha)
 * @param img0 NULL
 */
EXTERN_C void gimpts_rgb2indexed(void *img0, guchar *dst, gsize_t srclen, guint *ncolors_ret, guchar cmap_ret[768], guchar *src, unsigned adst, unsigned asrc);
/** Eliminates unused and duplicate colors from the cmap.
 * @param dst GIMP_INDEXED_IMAGE or GIMP_INDEXEDA_IMAGE in dst ... dstend
 * @param ncolors_inout number of colors in cmap (before and after)
 * @param cmap_inout colormap palette, modified in place
 * @param alpha alpha true if dst:GIMP_INDEXEDA_IMAGE, false if dst:GIMP_INDEXED_IMAGE
 */
EXTERN_C void gimpts_indexed_packpal(guchar *dst, guchar *dstend, guint *ncolors_inout, guchar cmap_inout[768], gbool alpha);
/* EXTERN_C void gimpts_indexed_packpal_incr(Hash46 *h46, guchar *dst, guchar *dstend, guint *ncolors_inout, guchar cmap_inout[768], gbool alpha); */
/** Eliminates unused and duplicate colors from the cmap of the image.
 * Modifies all drawables that hdrawable shares its cmap with.
 * @param hdrawable the drawable to consider first (or 0)
 * @param himage all indexed drawables of the image should be considered (or 0)
 */
EXTERN_C void gimpts_drawable_packpal(gint32 hdrawable, gint32 himage);
/** May fail when converting to GIMP_INDEXED*_IMAGE (failure means:
 * post(ncolors==0)). Doesn't call
 * gimpts_drawable_packpal() automatically. May extend the image cmap.
 */
EXTERN_C void gimpts_drawable_convert(gint32 hdrawable, GimpImageType dt);
/** Duplicates the specified drawable into a different drawable_type.
 * May fail when converting to GIMP_INDEXED*_IMAGE (failure means:
 * post(ncolors==0)). Doesn't call
 * gimpts_drawable_packpal() automatically. May extend the image cmap.
 */
EXTERN_C /*hdrawable*/gint32 gimpts_drawable_dup(gint32 hdrawable, GimpImageType dt);
/** Adds nonassociated alpha (specified in `halpha') to `hdrawable);
 * @param hdrawable GIMP_CHANNEL or GIMP_GRAY_IMAGE or GIMP_RGB_IMAGE or
 *        GIMP_INDEXED image
 * @param halpha GIMP_GRAY_IMAGE or GIMP_CHANNEL
 */
EXTERN_C void gimpts_drawable_noassalpha(gint32 hdrawable, gint32 halpha);
/** Takes a background image (with solid background color specified in param
 * `bg', and dimensions equal to those of hdrawable), and draws `hdrawable'
 * onto it. Overwrites `hdrawable' with the resulting drawable. Ingores
 * bg[1] and bg[2] if hdrawable was _GRAYA_. Does nothing (1) if hdrawable
 * was _INDEXEDA_. Note that (1) is possibly incorrect behaviour; you should
 * call gimpts_drawable_convert() first to convert from _INDEXEDA_ to
 * _RGBA_. The alpha channel of the resulting hdrawable will be
 * meaningless, so you should call gimpts_drawable_convert() to ignore it.
 * @param hdrawable modifies GIMP_{RGB,INDEXED,GRAY}A_IMAGE, other drawable
 *        types are left unchanged
 * @param bg a RGB background color, bg[0] is red, bg[2] is blue
 */
EXTERN_C void gimpts_drawable_alpha_assoc(gint32 hdrawable, guchar const bg[3]);
/** Unlinks the drawable from its containter image. */
EXTERN_C void gimpts_drawable_unlink(gint32 hdrawable);
/** @return a hdrawable or 0 */
EXTERN_C /*hdrawable*/gint32 gimpts_image_first_drawable(gint32 himage);
/** @return a hdrawable or 0 */
EXTERN_C /*hdrawable*/gint32 gimpts_drawable_next(gint32 hdrawable);
#define gimpts_pixel_rgn_get_topleft(r) ((r)->topleft)
EXTERN_C GimptsFileFormat gimpts_magic(gchar const* filename);

#if PTS_USE_PDB
typedef unsigned GimpPDBStatusType; /* vvv */
#define GIMP_PDB_EXECUTION_ERROR 1
#define GIMP_PDB_SUCCESS 2
#define GIMP_PDB_CANCEL 3
#define GIMP_PDB_CALLING_ERROR 4
#define GIMP_PDB_INT32 1
#define GIMP_PDB_STRING 2
#define GIMP_PDB_IMAGE 3
#define GIMP_PDB_DRAWABLE 4
#define GIMP_PDB_STATUS 5
#define GIMP_PLUGIN 0

typedef struct GimpParam {
  unsigned type;
  union {
    gint32 d_int32;
    GimpPDBStatusType d_status;
    gchar *d_string;
    gint32 d_image;
  } data;
} GimpParam;
typedef struct GimpPlugInInfo {
  void *init_proc;
  void *quit_proc;
  void (*query_proc)(void);
  void (*run_proc)    (gchar      *name,
		       gint        nparams,
		       GimpParam  *param,
		       gint       *nreturn_vals,
		       GimpParam **return_vals);
} GimpPlugInInfo;
typedef struct GimpParamDef {
  int type;
  char *name;
  char *desc;
} GimpParamDef;
EXTERN_C void  gimp_install_procedure (char* /*"file_tiff_load"*/,
                          char* /*"loads files of the tiff file format"*/,
                          char* /*"FIXME: write help for tiff_load"*/,
                          char* /*"Spencer Kimball, Peter Mattis & Nick Lamb"*/,
                          char* /*"Nick Lamb <njl195@zepler.org.uk>"*/,
                          char* /*"1995-1996,1998-2000"*/,
                          char* /*"<Load>/Tiff"*/,
			  char* /*NULL*/,
                          int /*GIMP_PLUGIN*/,
                          unsigned /*G_N_ELEMENTS (load_args)*/,
                          unsigned /*G_N_ELEMENTS (load_return_vals)*/,
                          GimpParamDef* /*load_args*/,
                          GimpParamDef* /*load_return_vals*/);
EXTERN_C void gimp_register_magic_load_handler (char* /*"file_tiff_load"*/,
			    char* /*"tif,tiff"*/,
			    char* /*""*/,
			    char* /*"0,string,II*\\0,0,string,MM\\0*"*/);
EXTERN_C void gimp_register_save_handler       (char* /*"file_tiff_save"*/,
			    char* /*"tif,tiff"*/,
			    char* /*""*/);
typedef unsigned GimpRunMode;
#define GIMP_RUN_INTERACTIVE 1
#define GIMP_RUN_WITH_LAST_VALS 2
#define GIMP_RUN_NONINTERACTIVE 3
#define GIMP_EXPORT_CANCEL 1
#define GIMP_EXPORT_EXPORT 2
typedef unsigned GimpExportReturnType; /* return from image export (save) dialog box */
EXTERN_C GimpExportReturnType gimp_export_image (gint32* /*&image*/, gint32* /*&drawable*/, char* /*"TIFF"*/, unsigned /* GIMP_EXPORT_CAN_*| */); /* ?? */
#define GIMP_EXPORT_CAN_HANDLE_RGB 1
#define GIMP_EXPORT_CAN_HANDLE_GRAY 2
#define GIMP_EXPORT_CAN_HANDLE_INDEXED 4
#define GIMP_EXPORT_CAN_HANDLE_ALPHA 8
#define INIT_I18N_UI()
#define INIT_I18N()
EXTERN_C void gimp_ui_init(char* /*"tiff"*/, gbool /*FALSE*/);
EXTERN_C void gimp_get_data (char* /*"file_tiff_save"*/, gpointer /*&tsvals*/);
EXTERN_C void gimp_set_data (char* /*"file_tiff_save"*/, gpointer /*&tsvals*/, gsize_t /*sizeof (TiffSaveVals)*/);
#endif


#endif /* end of minigimp.h */
