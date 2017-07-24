#define DUMMY \
  set -ex; \
  gcc -O2 -ansi -pedantic -W -Wall -Wshadow -Winline -Wstrict-prototypes \
  png22pnm.c -o png22pnm -lpng; \
  exit 0
/*
** png22pnm.c
** edited by pts@fazekas.hu at Tue Dec 10 16:33:53 CET 2002
**
** png22pnm.c has been tested with libpng 1.2 and 1.5. It should work with
** libpng 1.2 or later. Compatiblity with libpng versions earlier than 1.2 is
** not a goal.
**
** based on pngtopnm.c -
** read a Portable Network Graphics file and produce a portable anymap
**
** Copyright (C) 1995,1998 by Alexander Lehmann <alex@hal.rhein-main.de>
**                        and Willem van Schaik <willem@schaik.com>
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** modeled after giftopnm by David Koblas and
** with lots of bits pasted from libpng.txt by Guy Eric Schalnat
*/

/* 
   BJH 20000408:  rename PPM_MAXMAXVAL to PPM_OVERALLMAXVAL
   BJH 20000303:  fix include statement so dependencies work out right.
*/
/* GRR 19991203:  moved VERSION to new version.h header file */

/* GRR 19990713:  fixed redundant freeing of png_ptr and info_ptr in setjmp()
 *  blocks and added "pm_close(ifp)" in each.  */

/* GRR 19990317:  declared "clobberable" automatic variables in convertpng()
 *  static to fix Solaris/gcc stack-corruption bug.  Also installed custom
 *  error-handler to avoid jmp_buf size-related problems (i.e., jmp_buf
 *  compiled with one size in libpng and another size here).  */

#include <math.h>
#include <png.h>	/* includes zlib.h and setjmp.h */
#if 0
#define VERSION "p2.37.4 (5 December 1999) +netpbm"
#else
#define VERSION "0.11"
#endif

/**** pts ****/
#define PBMPLUS_RAWBITS 1
#define PGM_OVERALLMAXVAL 65535
#define PPM_OVERALLMAXVAL PGM_OVERALLMAXVAL

/* #include "pnm.h" */

typedef struct _jmpbuf_wrapper {
  jmp_buf jmpbuf;
} jmpbuf_wrapper;

/* GRR 19991205:  this is used as a test for pre-1999 versions of netpbm and
 *   pbmplus vs. 1999 or later (in which pm_close was split into two)
 */
#ifdef PBMPLUS_RAWBITS
#  define pm_closer pm_close
#  define pm_closew pm_close
#endif

#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif
#ifndef NONE
#  define NONE 0
#endif

/**** pts ****/
#include <stdio.h> /* fprintf() */
#include <string.h> /* strcmp() */
#include <stdlib.h> /* exit() */
#include <assert.h>
/* #include "pnm.h" */
#if (defined(__STDC__) && !defined(NO_PROTO)) || defined(__cplusplus)
# define _(args) args
# define OF(args) args
# define ___(arg2s,arg1s,argafter) arg2s
#else
# define _(args) ()
# define OF(args) ()
# define ___(arg2s,arg1s,argafter) arg1s argafter
#endif
#define u_char unsigned char
typedef unsigned dimen_t;
/* typedef unsigned char xel[4]; */ /* "RGBA" */
typedef unsigned pixval;
#define PBM_TYPE 1
#define PGM_TYPE 2
#define PPM_TYPE 3
#define pnm_init(_a,_b)
#define PMM(immed_str) stderr, "notice: " immed_str "\n"
#define pm_message printf
#define PME(immed_str) stderr, "error: " immed_str "\n"
#define pm_error printf /* !! exit */
#define pm_errexit() exit(3)
#define pm_close(file) fflush(file)
#define pm_keymatch(stra, strb, _x) (0==strcmp((stra),(strb)))
#define PPM_MAXMAXVAL 1023
typedef unsigned long pixel;
#define PPM_GETR(p) (((p) & 0x3ff00000) >> 20)
#define PPM_GETG(p) (((p) & 0xffc00) >> 10)
#define PPM_GETB(p) ((p) & 0x3ff)
/************* added definitions *****************/
#define PPM_PUTR(p, red) ((p) |= (((red) & 0x3ff) << 20))
#define PPM_PUTG(p, grn) ((p) |= (((grn) & 0x3ff) << 10))
#define PPM_PUTB(p, blu) ((p) |= ( (blu) & 0x3ff))
/**************************************************/
#define PPM_ASSIGN(p,red,grn,blu) (p) = ((pixel) (red) << 20) | ((pixel) (grn) << 10) | (pixel) (blu)
#define PPM_EQUAL(p,q) ((p) == (q))

static void* pm_allocrow (int const cols, int const size);
#define pnm_allocrow( cols ) ((xel*) pm_allocrow( cols, sizeof(xel) ))
#define PNM_ASSIGN1(x,v) PPM_ASSIGN(x,0,0,v)
#define PNM_GET1(x) PPM_GETB(x)
typedef pixel xel;
typedef pixval xelval;
static pixel ppm_parsecolor _(( char* colorname, pixval maxval ));
/* static void pnm_writepnminit _(( FILE* file, int cols, int rows, xelval maxval, int format, int forceplain )); */
#define pnm_freerow( xelrow ) pm_freerow( (char*) xelrow )
#define pm_freerow(x) free(x)
/* static void pnm_writepnmrow _(( FILE* file, xel* xelrow, int cols, xelval maxval, int format, int forceplain )); */
static FILE* pm_openr (const char* const name);
static FILE* pm_openw (const char* const name);

static void pm_usage (const char usage[], FILE *f) {
  fprintf(f, /*"png22pnm "VERSION"\n"*/"usage: %s\n", usage);
  exit(1);
}

static void* pm_allocrow(int const cols, int const size) {
    register char* itrow;

    itrow = (char*) malloc( cols * size );
    if ( itrow == (char*) 0 ) {
        fprintf(stderr, "out of memory allocating a row\n" );
        exit(1);
    }
    return itrow;
}

static FILE*
pm_openr(const char * const name) {
    FILE* f;

    if (strcmp(name, "-") == 0)
        f = stdin;
    else {
#ifndef VMS
        f = fopen(name, "rb");
#else
        f = fopen (name, "r", "ctx=stm");
#endif
        if (f == NULL) {
            fprintf(stderr,"not found: %s\n", name);
            exit(1);
        }
    }
    return f;
}

static FILE*
pm_openw(const char * const name) {
    FILE* f;

    if (strcmp(name, "-") == 0)
        f = stdout;
    else {
#ifndef VMS
        f = fopen(name, "wb");
#else
        f = fopen (name, "w", "mbc=32", "mbf=2");  /* set buffer factors */
#endif
        if (f == NULL) {
            fprintf(stderr,"not found: %s\n", name);
            exit(1);
        }
    }
    return f;
}

static long
rgbnorm(const long rgb, const long lmaxval, const int n, 
        const char * const colorname) {
/*----------------------------------------------------------------------------
   Normalize the color (r, g, or b) value 'rgb', which was specified with
   'n' digits, to a maxval of 'lmaxval'.  If the number of digits isn't
   valid, issue an error message and identify the complete color 
   color specification in error as 'colorname'.

   For example, if the user says 0ff/000/000 and the maxval is 100,
   then rgb is 0xff, n is 3, and our result is 
   0xff / (16**3-1) * 100 = 6.

-----------------------------------------------------------------------------*/

    switch (n) {
    case 1:
        return (long)((double) rgb * lmaxval / 15 + 0.5);
    case 2:
        return (long) ((double) rgb * lmaxval / 255 + 0.5);
    case 3:
        return (long) ((double) rgb * lmaxval / 4095 + 0.5);
    case 4:
        return (long) ((double) rgb * lmaxval / 65535L + 0.5);
    default:
        { pm_error( "invalid color specifier - \"%s\"", colorname ); exit(1); }
    }
    return 0;
}

static pixel
ppm_parsecolor( char* colorname, pixval maxval )
    {
    int hexit[256], i;
    pixel p;
    long lmaxval, r=0, g=0, b=0;
    char* inval = "invalid color specifier - \"%s\"";

    for ( i = 0; i < 256; ++i )
        hexit[i] = 1234567890;
    hexit['0'] = 0;
    hexit['1'] = 1;
    hexit['2'] = 2;
    hexit['3'] = 3;
    hexit['4'] = 4;
    hexit['5'] = 5;
    hexit['6'] = 6;
    hexit['7'] = 7;
    hexit['8'] = 8;
    hexit['9'] = 9;
    hexit['a'] = hexit['A'] = 10;
    hexit['b'] = hexit['B'] = 11;
    hexit['c'] = hexit['C'] = 12;
    hexit['d'] = hexit['D'] = 13;
    hexit['e'] = hexit['E'] = 14;
    hexit['f'] = hexit['F'] = 15;

    lmaxval = maxval;
    if ( strncmp( colorname, "rgb:", 4 ) == 0 )
        {
        /* It's a new-X11-style hexadecimal rgb specifier. */
        char* cp;

        cp = colorname + 4;
        r = g = b = 0;
        for ( i = 0; *cp != '/'; ++i, ++cp )
            r = r * 16 + hexit[(int)*cp];
        r = rgbnorm( r, lmaxval, i, colorname );
        for ( i = 0, ++cp; *cp != '/'; ++i, ++cp )
            g = g * 16 + hexit[(int)*cp];
        g = rgbnorm( g, lmaxval, i, colorname );
        for ( i = 0, ++cp; *cp != '\0'; ++i, ++cp )
            b = b * 16 + hexit[(int)*cp];
        b = rgbnorm( b, lmaxval, i, colorname );
        if ( r < 0 || r > lmaxval || g < 0 || g > lmaxval 
             || b < 0 || b > lmaxval )
            { pm_error( inval, colorname );  exit(1); }
        }
    else if ( strncmp( colorname, "rgbi:", 5 ) == 0 )
        {
        /* It's a new-X11-style decimal/float rgb specifier. */
        float fr, fg, fb;

        if ( sscanf( colorname, "rgbi:%f/%f/%f", &fr, &fg, &fb ) != 3 )
            { pm_error( inval, colorname ); exit(1); }
        if ( fr < 0.0 || fr > 1.0 || fg < 0.0 || fg > 1.0 
             || fb < 0.0 || fb > 1.0 )
            { pm_error( "invalid color specifier - \"%s\" - "
                      "values must be between 0.0 and 1.0", colorname ); exit(1); }
        r = fr * lmaxval;
        g = fg * lmaxval;
        b = fb * lmaxval;
        }
    else if ( colorname[0] == '#' )
        {
        /* It's an old-X11-style hexadecimal rgb specifier. */
        switch ( strlen( colorname ) - 1 /* (Number of hex digits) */ )
            {
            case 3:
            r = hexit[(int)colorname[1]];
            g = hexit[(int)colorname[2]];
            b = hexit[(int)colorname[3]];
            r = rgbnorm( r, lmaxval, 1, colorname );
            g = rgbnorm( g, lmaxval, 1, colorname );
            b = rgbnorm( b, lmaxval, 1, colorname );
            break;

            case 6:
            r = ( hexit[(int)colorname[1]] << 4 ) + hexit[(int)colorname[2]];
            g = ( hexit[(int)colorname[3]] << 4 ) + hexit[(int)colorname[4]];
            b = ( hexit[(int)colorname[5]] << 4 ) + hexit[(int)colorname[6]];
            r = rgbnorm( r, lmaxval, 2, colorname );
            g = rgbnorm( g, lmaxval, 2, colorname );
            b = rgbnorm( b, lmaxval, 2, colorname );
            break;

            case 9:
            r = ( hexit[(int)colorname[1]] << 8 ) +
              ( hexit[(int)colorname[2]] << 4 ) +
                hexit[(int)colorname[3]];
            g = ( hexit[(int)colorname[4]] << 8 ) + 
              ( hexit[(int)colorname[5]] << 4 ) +
                hexit[(int)colorname[6]];
            b = ( hexit[(int)colorname[7]] << 8 ) + 
              ( hexit[(int)colorname[8]] << 4 ) +
                hexit[(int)colorname[9]];
            r = rgbnorm( r, lmaxval, 3, colorname );
            g = rgbnorm( g, lmaxval, 3, colorname );
            b = rgbnorm( b, lmaxval, 3, colorname );
            break;

            case 12:
            r = ( hexit[(int)colorname[1]] << 12 ) + 
              ( hexit[(int)colorname[2]] << 8 ) +
                ( hexit[(int)colorname[3]] << 4 ) + hexit[(int)colorname[4]];
            g = ( hexit[(int)colorname[5]] << 12 ) + 
              ( hexit[(int)colorname[6]] << 8 ) +
                ( hexit[(int)colorname[7]] << 4 ) + hexit[(int)colorname[8]];
            b = ( hexit[(int)colorname[9]] << 12 ) + 
              ( hexit[(int)colorname[10]] << 8 ) +
                ( hexit[(int)colorname[11]] << 4 ) + hexit[(int)colorname[12]];
            r = rgbnorm( r, lmaxval, 4, colorname );
            g = rgbnorm( g, lmaxval, 4, colorname );
            b = rgbnorm( b, lmaxval, 4, colorname );
            break;

            default:
            { pm_error( inval, colorname ); exit(1); }
            }
        if ( r < 0 || r > lmaxval || g < 0 || g > lmaxval 
             || b < 0 || b > lmaxval )
            { pm_error( inval, colorname ); exit(1); }
        }
    else if ( ( colorname[0] >= '0' && colorname[0] <= '9' ) ||
              colorname[0] == '.' )
        {
        /* It's an old-style decimal/float rgb specifier. */
        float fr, fg, fb;

        if ( sscanf( colorname, "%f,%f,%f", &fr, &fg, &fb ) != 3 )
            { pm_error( inval, colorname ); exit(1); }
        if ( fr < 0.0 || fr > 1.0 || fg < 0.0 || fg > 1.0 
             || fb < 0.0 || fb > 1.0 )
            { pm_error( "invalid color specifier - \"%s\" - "
                      "values must be between 0.0 and 1.0", colorname ); exit(1); }
        r = fr * lmaxval;
        g = fg * lmaxval;
        b = fb * lmaxval;
        }
    else
        {
        /* Must be a name from the X-style rgb file. */
        { pm_error( "unknown color - \"%s\"", colorname ); exit(1); }
        }
    
    PPM_ASSIGN( p, r, g, b );
    return p;
    }


/* ----- */

/* function prototypes */
#ifdef __STDC__
static png_uint_16 _get_png_val (png_byte **pp, int bit_depth);
static void store_pixel (xel *pix, png_uint_16 r, png_uint_16 g, png_uint_16 b,                       png_uint_16 a);
static int iscolor (png_color c);
static void save_text (png_structp png_ptr, png_info *info_ptr, FILE *tfp);
static void show_time (png_structp png_ptr, png_info *info_ptr);
static void pngtopnm_error_handler (png_structp png_ptr, png_const_charp msg);
static void convertpng (FILE *ifp, FILE *tfp);
int main (int argc, char *argv[]);
#endif

enum alpha_handling
  {none, alpha_only, mix_only, none_and_alpha};

static png_uint_16 maxval, maxmaxval;
static png_uint_16 bgr, bgg, bgb; /* background colors */
static int verbose = FALSE;
static enum alpha_handling alpha = none;
static int do_background = -1;
static char *backstring;
static float displaygamma = -1.0; /* display gamma */
static float totalgamma = -1.0;
static int text = FALSE;
static char *text_file;
static int mtime = FALSE;
static jmpbuf_wrapper pngtopnm_jmpbuf_struct;

#define get_png_val(p) _get_png_val (&(p), bit_depth)

#ifdef __STDC__
static png_uint_16 _get_png_val (png_byte **pp, int bit_depth)
#else
static png_uint_16 _get_png_val (pp, bit_depth)
png_byte **pp;
int bit_depth;
#endif
{
  png_uint_16 c = 0;

  if (bit_depth == 16) {
    c = (*((*pp)++)) << 8;
  }
  c |= (*((*pp)++));

  if (maxval > maxmaxval)
    c /= ((maxval + 1) / (maxmaxval + 1));

  return c;
}

#ifdef __STDC__
static void store_pixel (xel *pix, png_uint_16 r, png_uint_16 g, png_uint_16 b, png_uint_16 a)
#else
static void store_pixel (pix, r, g, b, a)
xel *pix;
png_uint_16 r, g, b, a;
#endif
{
  if (alpha == alpha_only) {
    PNM_ASSIGN1 (*pix, a);
  } else {
    if (((alpha == mix_only) /*|| (alpha == mix_and_alpha)*/) && (a != maxval)) {
      r = r * (double)a / maxval + ((1.0 - (double)a / maxval) * bgr);
      g = g * (double)a / maxval + ((1.0 - (double)a / maxval) * bgg);
      b = b * (double)a / maxval + ((1.0 - (double)a / maxval) * bgb);
    }
    PPM_ASSIGN (*pix, r, g, b);
  }
}

#ifdef __STDC__
static png_uint_16 gamma_correct (png_uint_16 v, float g)
#else
static png_uint_16 gamma_correct (v, g)
png_uint_16 v;
float g;
#endif
{
  if (g != -1.0)
    return (png_uint_16) (pow ((double) v / maxval, 
			       (1.0 / g)) * maxval + 0.5);
  else
    return v;
}

#ifdef __STDC__
static int iscolor (png_color c)
#else
static int iscolor (c)
png_color c;
#endif
{
  return c.red != c.green || c.green != c.blue;
}

#ifdef __STDC__
static void save_text (png_structp png_ptr, png_info *info_ptr, FILE *tfp)
#else
static void save_text (png_ptr, info_ptr, tfp)
png_structp png_ptr;
png_info *info_ptr;
FILE *tfp;
#endif
{
  int i, j, k;

  png_textp text;
  int num_text;

  png_get_text(png_ptr, info_ptr, &text, &num_text);

  for (i = 0 ; i < num_text ; i++) {
    j = 0;
    while (text[i].key[j] != '\0' && text[i].key[j] != ' ')
      j++;    
    if (text[i].key[j] != ' ') {
      fprintf (tfp, "%s", text[i].key);
      for (j = strlen (text[i].key) ; j < 15 ; j++)
        putc (' ', tfp);
    } else {
      fprintf (tfp, "\"%s\"", text[i].key);
      for (j = strlen (text[i].key) ; j < 13 ; j++)
        putc (' ', tfp);
    }
    putc (' ', tfp); /* at least one space between key and text */
    
    for (j = 0 ; j+0U < text[i].text_length ; j++) {
      putc (text[i].text[j], tfp);
      if (text[i].text[j] == '\n')
        for (k = 0 ; k < 16 ; k++)
          putc ((int)' ', tfp);
    }
    putc ((int)'\n', tfp);
  }
}

#ifdef __STDC__
static void show_time (png_structp png_ptr, png_info *info_ptr)
#else
static void show_time (png_ptr, info_ptr)
png_structp png_ptr;
png_info *info_ptr;
#endif
{
  static char *month[] =
    {"", "January", "February", "March", "April", "May", "June",
     "July", "August", "September", "October", "November", "December"};
  png_timep mod_time;

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tIME)) {
    png_get_tIME(png_ptr, info_ptr, &mod_time);
    pm_message ("modification time: %02d %s %d %02d:%02d:%02d",
                mod_time->day, month[mod_time->month],
                mod_time->year, mod_time->hour,
                mod_time->minute, mod_time->second);
  }
}

#ifdef __STDC__
static void pngtopnm_error_handler (png_structp png_ptr, png_const_charp msg)
#else
static void pngtopnm_error_handler (png_ptr, msg)
png_structp png_ptr;
png_const_charp msg;
#endif
{
  jmpbuf_wrapper  *jmpbuf_ptr;

  /* this function, aside from the extra step of retrieving the "error
   * pointer" (below) and the fact that it exists within the application
   * rather than within libpng, is essentially identical to libpng's
   * default error handler.  The second point is critical:  since both
   * setjmp() and longjmp() are called from the same code, they are
   * guaranteed to have compatible notions of how big a jmp_buf is,
   * regardless of whether _BSD_SOURCE or anything else has (or has not)
   * been defined. */

  fprintf(stderr, "pnmtopng:  fatal libpng error: %s\n", msg);
  fflush(stderr);

  jmpbuf_ptr = png_get_error_ptr(png_ptr);
  if (jmpbuf_ptr == NULL) {         /* we are completely hosed now */
    fprintf(stderr,
      "pnmtopng:  EXTREMELY fatal error: jmpbuf unrecoverable; terminating.\n");
    fflush(stderr);
    exit(99);
  }

  longjmp(jmpbuf_ptr->jmpbuf, 1);
}

#define SIG_CHECK_SIZE 4

/* If __MINGW32__ is defined, then _WIN32 is also defined. */
#if defined(_WIN32) || defined(__CYGWIN__)
#define DO_WIN 1
#else
#undef  DO_WIN
#endif

#if DO_WIN
#undef __STRICT_ANSI__
#include <fcntl.h>
#define __STRICT_ANSI__ 1
extern int setmode(int, int);
extern FILE *fdopen(int, const char*);
#endif

#ifdef __STDC__
static void convertpng (FILE *ifp, FILE *tfp)
#else
static void convertpng (ifp, tfp)
FILE *ifp;
FILE *tfp;
#endif
{
  unsigned char sig_buf [SIG_CHECK_SIZE];
  png_struct *png_ptr;
  png_info *info_ptr;
  pixel *row;
  png_byte **png_image;
  png_byte *png_pixel;
  png_uint_32 width;
  png_uint_32 height;
  pixel *pnm_pixel;
  int bit_depth;
  png_byte color_type;
  png_color_16p background;
  double gamma;
  png_bytep trans_alpha;
  int num_trans;
  png_color_16p trans_color;
  png_color_8p sig_bit;
  int num_palette;
  png_colorp palette;
  png_uint_32 x_pixels_per_unit, y_pixels_per_unit;
  int phys_unit_type;
  int has_phys;
  int x, y;
  int linesize;
  png_uint_16 c, c2, c3, a;
  int pnm_type;
  int i;
  int trans_mix;
  pixel backcolor;
  char gamma_string[80];
  /* these variables are declared static because gcc wasn't kidding
   * about "variable XXX might be clobbered by `longjmp' or `vfork'"
   * (stack corruption observed on Solaris 2.6 with gcc 2.8.1, even
   * in the absence of any other error condition) */
  static char *type_string;
  static char *alpha_string;

#if DO_WIN
  FILE *so; int sofd;
  setmode(1, O_BINARY);
  if (0>(sofd=dup(1))) abort(); /* BUGFIX at Tue Mar 18 17:03:42 CET 2003 */
  setmode(sofd, O_BINARY);
  if (NULL==(so=fdopen(sofd, "wb"))) abort(); /* binary stdout */
#else
  FILE *so=stdout;
#endif


  type_string = alpha_string = "";

  if (fread (sig_buf, 1, SIG_CHECK_SIZE, ifp) != SIG_CHECK_SIZE) {
    pm_closer (ifp);
    { pm_error ("input file empty or too short"); exit(1); }
  }
  if (png_sig_cmp (sig_buf, (png_size_t) 0, (png_size_t) SIG_CHECK_SIZE) != 0) {
    pm_closer (ifp);
    { pm_error ("input file not a PNG file"); exit(1); }
  }

  png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
    &pngtopnm_jmpbuf_struct, pngtopnm_error_handler, NULL);
  if (png_ptr == NULL) {
    pm_closer (ifp);
    { pm_error ("cannot allocate LIBPNG structure"); exit(1); }
  }

  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL) {
    png_destroy_read_struct (&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    pm_closer (ifp);
    { pm_error ("cannot allocate LIBPNG structures"); exit(1); }
  }

  if (setjmp (pngtopnm_jmpbuf_struct.jmpbuf)) {
    png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
    pm_closer (ifp);
    { pm_error ("setjmp returns error condition"); exit(1); }
  }

  png_init_io (png_ptr, ifp);
  png_set_sig_bytes (png_ptr, SIG_CHECK_SIZE);
  png_read_info (png_ptr, info_ptr);

  bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_bKGD)) {
    png_get_bKGD(png_ptr, info_ptr, &background);
  } else {
    background = NULL;
  }
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA)) {
    png_get_gAMA(png_ptr, info_ptr, &gamma);
  } else {
    gamma = -1;
  }
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, &trans_color);
  } else {
    trans_alpha = NULL;
    num_trans = 0;
    trans_color = NULL;
  }
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE)) {
    png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
  } else {
    palette = NULL;
    num_palette = 0;
  }
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT)) {
    png_get_sBIT(png_ptr, info_ptr, &sig_bit);
  } else {
    sig_bit = NULL;
  }
  if (0 != (has_phys = png_get_valid(png_ptr, info_ptr, PNG_INFO_pHYs))) {
    png_get_pHYs(png_ptr, info_ptr, &x_pixels_per_unit, &y_pixels_per_unit,
                 &phys_unit_type);
  }

  if (verbose) {
    switch (color_type) {
      case PNG_COLOR_TYPE_GRAY:
        type_string = "gray";
        alpha_string = "";
        break;

      case PNG_COLOR_TYPE_GRAY_ALPHA:
        type_string = "gray";
        alpha_string = "+alpha";
        break;

      case PNG_COLOR_TYPE_PALETTE:
        type_string = "palette";
        alpha_string = "";
        break;

      case PNG_COLOR_TYPE_RGB:
        type_string = "truecolor";
        alpha_string = "";
        break;

      case PNG_COLOR_TYPE_RGB_ALPHA:
        type_string = "truecolor";
        alpha_string = "+alpha";
        break;
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
      alpha_string = "+transparency";
    }

    if (gamma >= 0) {
      sprintf (gamma_string, ", image gamma = %4.2f", gamma);
    } else {
      strcpy (gamma_string, "");
    }

    if (verbose) {
      pm_message ("reading a %ldw x %ldh image, %d bit%s %s%s%s%s",
		  width + 0L, height + 0L,
		  bit_depth, bit_depth > 1 ? "s" : "",
		  type_string, alpha_string, gamma_string,
		  png_get_interlace_type(png_ptr, info_ptr) ?
		  ", Adam7 interlaced" : "");
      if (background != NULL) {
        pm_message ("background {index, gray, red, green, blue} = "
                    "{%d, %d, %d, %d, %d}",
                    background->index,
                    background->gray,
                    background->red,
                    background->green,
                    background->blue);
      }
    }
  }

  png_image = (png_byte **)malloc (height * sizeof (png_byte*));
  if (png_image == NULL) {
    png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
    pm_closer (ifp);
    { pm_error ("couldn't allocate space for image"); exit(1); }
  }

  if (bit_depth == 16)
    linesize = 2 * width;
  else
    linesize = width;

  if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    linesize *= 2;
  else
  if (color_type == PNG_COLOR_TYPE_RGB)
    linesize *= 3;
  else
  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    linesize *= 4;

  for (y = 0 ; y+0U < height ; y++) {
    png_image[y] = malloc (linesize);
    if (png_image[y] == NULL) {
      for (x = 0 ; x < y ; x++)
        free (png_image[x]);
      free (png_image);
      png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
      pm_closer (ifp);
      { pm_error ("couldn't allocate space for image"); exit(1); }
    }
  }

  if (bit_depth < 8)
    png_set_packing (png_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    maxval = 255;
  } else {
    maxval = (1l << bit_depth) - 1;
  }

  /* gamma-correction */
  if (displaygamma != -1.0) {
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA)) {
      if (displaygamma != gamma) {
        png_set_gamma (png_ptr, displaygamma, gamma);
	totalgamma = (double) gamma * (double) displaygamma;
	/* in case of gamma-corrections, sBIT's as in the PNG-file are not valid anymore */
	sig_bit = NULL;
        if (verbose)
          pm_message ("image gamma is %4.2f, converted for display gamma of %4.2f",
                    gamma, displaygamma);
      }
    } else {
      if (displaygamma != gamma) {
	png_set_gamma (png_ptr, displaygamma, 1.0);
	totalgamma = (double) displaygamma;
	sig_bit = NULL;
	if (verbose)
	  pm_message ("image gamma assumed 1.0, converted for display gamma of %4.2f",
		      displaygamma);
      }
    }
  }

  /* sBIT handling is very tricky. If we are extracting only the image, we
     can use the sBIT info for grayscale and color images, if the three
     values agree. If we extract the transparency/alpha mask, sBIT is
     irrelevant for trans and valid for alpha. If we mix both, the
     multiplication may result in values that require the normal bit depth,
     so we will use the sBIT info only for transparency, if we know that only
     solid and fully transparent is used */

  if (sig_bit != NULL) {
    switch (alpha) {
      /*case mix_and_alpha: */
      case mix_only:
        if (color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
          break;
        if (color_type == PNG_COLOR_TYPE_PALETTE &&
            png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
          trans_mix = TRUE;
          for (i = 0 ; i < num_trans ; i++)
            if (trans_alpha[i] != 0 && trans_alpha[i] != 255) {
              trans_mix = FALSE;
              break;
            }
          if (!trans_mix)
            break;
        }

        /* else fall though to normal case */
  
      case none_and_alpha:
      case none:
        if ((color_type == PNG_COLOR_TYPE_PALETTE ||
             color_type == PNG_COLOR_TYPE_RGB ||
             color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
            (sig_bit->red != sig_bit->green ||
             sig_bit->red != sig_bit->blue) &&
            alpha == none) {
	  pm_message ("different bit depths for color channels not supported");
	  pm_message ("writing file with %d bit resolution", bit_depth);
        } else {
          if ((color_type == PNG_COLOR_TYPE_PALETTE) &&
	      (sig_bit->red < 255)) {
	    for (i = 0 ; i < num_palette ; i++) {
	      palette[i].red   >>= (8 - sig_bit->red);
	      palette[i].green >>= (8 - sig_bit->green);
	      palette[i].blue  >>= (8 - sig_bit->blue);
	    }
	    maxval = (1l << sig_bit->red) - 1;
	    if (verbose)
	      pm_message ("image has fewer significant bits, writing file with %d bits per channel", 
		sig_bit->red);
          } else
          if ((color_type == PNG_COLOR_TYPE_RGB ||
               color_type == PNG_COLOR_TYPE_RGB_ALPHA) &&
	      (sig_bit->red < bit_depth)) {
	    png_set_shift (png_ptr, sig_bit);
	    maxval = (1l << sig_bit->red) - 1;
	    if (verbose)
	      pm_message ("image has fewer significant bits, writing file with %d bits per channel", 
		sig_bit->red);
          } else 
          if ((color_type == PNG_COLOR_TYPE_GRAY ||
               color_type == PNG_COLOR_TYPE_GRAY_ALPHA) &&
	      (sig_bit->gray < bit_depth)) {
	    png_set_shift (png_ptr, sig_bit);
	    maxval = (1l << sig_bit->gray) - 1;
	    if (verbose)
	      pm_message ("image has fewer significant bits, writing file with %d bits",
		sig_bit->gray);
          }
        }
        break;

      case alpha_only:
        if ((color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
             color_type == PNG_COLOR_TYPE_GRAY_ALPHA) && 
	    (sig_bit->gray < bit_depth)) {
	  png_set_shift (png_ptr, sig_bit);
	  if (verbose)
	    pm_message ("image has fewer significant bits, writing file with %d bits", 
		sig_bit->alpha);
	  maxval = (1l << sig_bit->alpha) - 1;
        }
        break;

      }
  }

  /* didn't manage to get libpng to work (bugs?) concerning background */
  /* processing, therefore we do our own using bgr, bgg and bgb        */

  if (background != NULL)
    switch (color_type) {
      case PNG_COLOR_TYPE_GRAY:
      case PNG_COLOR_TYPE_GRAY_ALPHA:
        bgr = bgg = bgb = gamma_correct (background->gray, totalgamma);
        break;
      case PNG_COLOR_TYPE_PALETTE:
        bgr = gamma_correct (palette[background->index].red, totalgamma);
        bgg = gamma_correct (palette[background->index].green, totalgamma);
        bgb = gamma_correct (palette[background->index].blue, totalgamma);
        break;
      case PNG_COLOR_TYPE_RGB:
      case PNG_COLOR_TYPE_RGB_ALPHA:
        bgr = gamma_correct (background->red, totalgamma);
        bgg = gamma_correct (background->green, totalgamma);
        bgb = gamma_correct (background->blue, totalgamma);
        break;
    }
  else
    /* when no background given, we use white [from version 2.37] */
    bgr = bgg = bgb = maxval;

  /* but if background was specified from the command-line, we always use that	*/
  /* I chose to do no gamma-correction in this case; which is a bit arbitrary	*/
  if (do_background > -1)
  {
    backcolor = ppm_parsecolor (backstring, maxval);
    switch (color_type) {
      case PNG_COLOR_TYPE_GRAY:
      case PNG_COLOR_TYPE_GRAY_ALPHA:
        bgr = bgg = bgb = PNM_GET1 (backcolor);
        break;
      case PNG_COLOR_TYPE_PALETTE:
      case PNG_COLOR_TYPE_RGB:
      case PNG_COLOR_TYPE_RGB_ALPHA:
        bgr = PPM_GETR (backcolor);
        bgg = PPM_GETG (backcolor);
        bgb = PPM_GETB (backcolor);
        break;
    }
  }

  png_read_image (png_ptr, png_image);
  png_read_end (png_ptr, info_ptr);

  if (mtime)
    show_time (png_ptr, info_ptr);
  if (text)
    save_text (png_ptr, info_ptr, tfp);

  if (has_phys) {
    double r;
    r = (double)x_pixels_per_unit / y_pixels_per_unit;
    if (r != 1.0) {
      pm_message ("warning - non-square pixels; to fix do a 'pnmscale -%cscale %g'",
		    r < 1.0 ? 'x' : 'y',
		    r < 1.0 ? 1.0 / r : r );
    }
  }

  if ((row = pnm_allocrow (width)) == NULL) {
    for (y = 0 ; y+0U < height ; y++)
      free (png_image[y]);
    free (png_image);
    png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
    pm_closer (ifp);
    { pm_error ("couldn't allocate space for image"); exit(1); }
  }

  if (alpha == alpha_only) {
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_RGB) {
      pnm_type = PBM_TYPE;
    } else
      if (color_type == PNG_COLOR_TYPE_PALETTE) {
        pnm_type = PBM_TYPE;
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
          for (i = 0 ; i < num_trans ; i++) {
            if (trans_alpha[i] != 0 && trans_alpha[i] != maxval) {
              pnm_type = PGM_TYPE;
              break;
            }
          }
        }
      } else {
        if (maxval == 1)
          pnm_type = PBM_TYPE;
        else
          pnm_type = PGM_TYPE;
      }
  } else {
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
      if (bit_depth == 1) {
        pnm_type = PBM_TYPE;
      } else {
        pnm_type = PGM_TYPE;
      }
    } else
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
      pnm_type = PGM_TYPE;
      for (i = 0 ; i < num_palette ; i++) {
        if (iscolor (palette[i])) {
          pnm_type = PPM_TYPE;
          break;
        }
      }
    } else {
      pnm_type = PPM_TYPE;
    }
  }

  if ((pnm_type == PGM_TYPE) && (maxval >= PGM_OVERALLMAXVAL))
    maxmaxval = PGM_OVERALLMAXVAL;
  else if ((pnm_type == PPM_TYPE) && (maxval >= PPM_OVERALLMAXVAL))
    maxmaxval = PPM_OVERALLMAXVAL;
  else maxmaxval = maxval;

  if (verbose)
    pm_message ("writing a %s file (maxval=%u)",
                pnm_type == PBM_TYPE ? "PBM" :
                pnm_type == PGM_TYPE ? "PGM" :
                pnm_type == PPM_TYPE ? "PPM" :
                                       "UNKNOWN!", 
		maxmaxval);

  /* pnm_writepnminit (stdout, info_ptr->width, info_ptr->height, maxval, pnm_type, FALSE); */
  if (maxval>255) {
    fprintf(stderr, "supporting only 8-bit images\n");
    exit(1);
  }
  /* Dat: supporting only RAWBITS */
  switch (pnm_type) {
   case PBM_TYPE:
    fprintf(so,"P4 %lu %lu\n", 0LU+width, 0LU+height);
    break;
   case PGM_TYPE:
    fprintf(so,"P5 %lu %lu 255\n", 0LU+width, 0LU+height);
    break;
   case PPM_TYPE:
    fprintf(so,"P6 %lu %lu 255\n", 0LU+width, 0LU+height);
    break;
   default:
    assert(0);
  }

  for (y = 0 ; y+0U < height ; y++) {
    png_pixel = png_image[y];
    pnm_pixel = row;
    for (x = 0 ; x+0U < width ; x++) {
      c = get_png_val (png_pixel);
      switch (color_type) {
        case PNG_COLOR_TYPE_GRAY:
          store_pixel (pnm_pixel, c, c, c,
		(trans_color != NULL &&
		 (c == gamma_correct (trans_color->gray, totalgamma))) ?
			0 : maxval);
          break;

        case PNG_COLOR_TYPE_GRAY_ALPHA:
          a = get_png_val (png_pixel);
          store_pixel (pnm_pixel, c, c, c, a);
          break;

        case PNG_COLOR_TYPE_PALETTE:
          store_pixel (pnm_pixel, palette[c].red,
                       palette[c].green, palette[c].blue,
                       (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) &&
                        c < num_trans ? trans_alpha[c] : maxval);
          break;

        case PNG_COLOR_TYPE_RGB:
          c2 = get_png_val (png_pixel);
          c3 = get_png_val (png_pixel);
          store_pixel (pnm_pixel, c, c2, c3,
		(trans_color != NULL &&
		 (c  == gamma_correct (trans_color->red, totalgamma)) &&
		 (c2 == gamma_correct (trans_color->green, totalgamma)) &&
		 (c3 == gamma_correct (trans_color->blue, totalgamma))) ?
			0 : maxval);
          break;

        case PNG_COLOR_TYPE_RGB_ALPHA:
          c2 = get_png_val (png_pixel);
          c3 = get_png_val (png_pixel);
          a = get_png_val (png_pixel);
          store_pixel (pnm_pixel, c, c2, c3, a);
          break;

        default:
          pnm_freerow (row);
          for (i = 0 ; i+0U < height ; i++)
            free (png_image[i]);
          free (png_image);
          png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
          pm_closer (ifp);
          { pm_error ("unknown PNG color type"); exit(1); }
      }
      pnm_pixel++;
    }

    /* pnm_writepnmrow (stdout, row, info_ptr->width, maxval, pnm_type, FALSE); */
    {
      unsigned wd=width, len, cc=1;
      char *rowa, *p;
      register xel *xP=row;
      /* Imp: eliminate several pm_allocrow */
      switch (pnm_type) {
       case PBM_TYPE:
        p=rowa=pm_allocrow(len=(wd+7)>>3, 1);
        while (wd--!=0) {
          if ((cc&256)!=0) { *p++=cc; cc=1; }
          cc<<=1;
          cc|=PNM_GET1( *xP ) == 0;
          xP++;
        }
        if (cc!=1) {
          while ((cc&256)==0) cc<<=1;
          *p++=cc; cc=1;
        }
        assert(p==rowa+len);
        break;
       case PGM_TYPE:
        p=rowa=pm_allocrow(wd, 1); len=wd;
        while (wd--!=0) {
          *p++=PPM_GETB(*xP); xP++;
        }
        break;
       case PPM_TYPE:
        p=rowa=pm_allocrow(wd, 3); len=wd*3;
        while (wd--!=0) {
          /* printf("X %d\n", wd); fflush(stdout); */
          *p++=PPM_GETR(*xP);
          *p++=PPM_GETG(*xP);
          *p++=PPM_GETB(*xP); xP++;
        }
        break;
       default:
        assert(0); abort();
      }
      fwrite(rowa, 1, len, so);
      pm_freerow(rowa);
    }

#if 0
#if __STDC__
void
pnm_writepnmrow( FILE* file, xel* xelrow, int cols, xelval maxval, int format, int forceplain )
#else /*__STDC__*/
void
pnm_writepnmrow( file, xelrow, cols, maxval, format, forceplain )
    FILE* file;
    xel* xelrow;
    int cols, format;
    xelval maxval;
    int forceplain;
#endif /*__STDC__*/
    {
    register int col;
    register xel* xP;
    gray* grayrow;
    register gray* gP;
    bit* bitrow;
    register bit* bP;

    switch ( PNM_FORMAT_TYPE(format) )
	{
	case PPM_TYPE:
	ppm_writeppmrow( file, (pixel*) xelrow, cols, (pixval) maxval, forceplain );
	break;

	case PGM_TYPE:
	grayrow = pgm_allocrow( cols );
	for ( col = 0, gP = grayrow, xP = xelrow; col < cols; ++col, ++gP, ++xP )
	    *gP = PNM_GET1( *xP );
	pgm_writepgmrow( file, grayrow, cols, (gray) maxval, forceplain );
	pgm_freerow( grayrow );
	break;

	case PBM_TYPE:
	bitrow = pbm_allocrow( cols );
	for ( col = 0, bP = bitrow, xP = xelrow; col < cols; ++col, ++bP, ++xP )
	    *bP = PNM_GET1( *xP ) == 0 ? PBM_BLACK : PBM_WHITE;
	pbm_writepbmrow( file, bitrow, cols, forceplain );
	pbm_freerow( bitrow );
	break;

	default:
	{ pm_error( "invalid format argument received by pnm_writepnmrow(): %d\n"
              "PNM_FORMAT_TYPE(format) must be %d, %d, or %d", 
              format, PBM_TYPE, PGM_TYPE, PPM_TYPE); exit(1); }
	}
    }
#endif

  }

  fflush(so);
#if DO_WIN
  fclose(so);
#endif
  pnm_freerow (row);
  for (y = 0 ; y+8U < height ; y++)
    free (png_image[y]);
  free (png_image);
  png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
}

#ifdef __STDC__
int main (int argc, char *argv[])
#else
int main (argc, argv)
int argc;
char *argv[];
#endif
{
  FILE *ifp, *tfp;
  int argn;

  char *usage = "[-verbose] [-alpha | -mix | -rgba] [-background color] ...\n\
             ... [-gamma value] [-text file] [-time] [pngfile]";

  pnm_init (&argc, argv);
  argn = 1;

  /* stdout=stderr; */

  while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0') {
    if (pm_keymatch (argv[argn], "-verbose", 2)) {
      verbose = TRUE;
    } else if (pm_keymatch (argv[argn], "-alpha", 2)) {
      alpha = alpha_only;
    } else if (pm_keymatch (argv[argn], "-mix", 2)) {
      alpha = mix_only;
    } else if (pm_keymatch (argv[argn], "-rgba", 2)) {
      alpha = none_and_alpha;
    } else if (pm_keymatch (argv[argn], "-background", 2)) {
      do_background = 1;
      if (++argn < argc)
        backstring = argv[argn];
      else
        pm_usage (usage, stderr);
    } else
    if (pm_keymatch (argv[argn], "-gamma", 2)) {
      if (++argn < argc)
        sscanf (argv[argn], "%f", &displaygamma);
      else
        pm_usage (usage, stderr);
    } else
    if (pm_keymatch (argv[argn], "-text", 3)) {
      text = TRUE;
      if (++argn < argc)
        text_file = argv[argn];
      else
        pm_usage (usage, stderr);
    } else
    if (pm_keymatch (argv[argn], "-time", 3)) {
      mtime = TRUE;
    } else if (pm_keymatch (argv[argn], "-h", 3)) {
      fprintf(stdout,"png22pnm version %s, compiled with libpng version %s\n",
        VERSION, PNG_LIBPNG_VER_STRING);
      pm_usage (usage, stdout);
    } else {
      pm_usage (usage, stderr);
    }
    argn++;
  }

  if (argn != argc) {
    ifp = pm_openr (argv[argn]);
    ++argn;
  } else {
    ifp = stdin;
  }
  if (argn != argc)
    pm_usage (usage, stderr);

  if (text)
    tfp = pm_openw (text_file);
  else
    tfp = NULL;

  convertpng (ifp, tfp);
  if (alpha==none_and_alpha) {
    alpha=alpha_only;
    rewind(ifp);
    if (ferror(ifp)) { pm_error( "cannot rewind() PNG file" ); exit(1); }
    convertpng(ifp, tfp);
  }

  if (text)
    pm_closew (tfp);

  pm_closer (ifp);
  pm_closew (stdout);
  exit (0);
}
