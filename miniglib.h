/* miniglib.h -- minimalistic glib-style misc accessory #defines etc.,
 *   instead of config2.h
 * by pts@fazekas.hu at Sun Sep 29 22:05:54 CEST 2002
 */

#ifndef MINIGLIB_H
#define MINIGLIB_H

#if defined(__GNUC__) && defined(__cplusplus)
#pragma interface
#endif

#include "config.h"

#if NDEBUG /* NDEBUG==1 means: no assertions */
#  define ASSERT_SIDE(x) x /* assert with side effect */
#  define ASSERT_SIDE2(x,y) x /* assert with side effect */
#  define assert0(x) abort()
#  define assert(x)
#else
#  include <assert.h>
#  define ASSERT_SIDE(x) assert(x)
#  define ASSERT_SIDE2(x,y) assert(x y)
#  define assert0(x) assert(0 && (x))
#endif
#define param_assert assert
#define assert_slow(x)

#if (defined(__STDC__) && !defined(NO_PROTO)) || defined(__cplusplus)
# define _(args) args
# define OF(args) args
# define ___(arg2s,arg1s,argafter) arg2s
#else
# define _(args) ()
# define OF(args) ()
# define ___(arg2s,arg1s,argafter) arg1s argafter
#endif

#define NULLP ((void*)0)

#if SIZEOF_BOOL!=1
#define bool PTS_bool
#define true 1
#define false 0
typedef unsigned char bool;
#endif
#undef  TRUE 
#define TRUE 1
#undef  FALSE
#define FALSE 0

#ifdef const
#  undef const
#  define PTS_const
#  undef HAVE_CONST
#else
#  define PTS_const const
#  define HAVE_CONST 1
#endif

#if SIZEOF_INT>=4
  typedef unsigned slen_t;
  typedef   signed slendiff_t;
  typedef unsigned guint32;
  typedef   signed gint32;
# define SIZEOF_SLEN_T SIZEOF_INT
# define SLEN_P ""
#else
  typedef unsigned long slen_t;
  typedef   signed long slendiff_t;
  typedef unsigned long guint32;
  typedef   signed long gint32;
# define SIZEOF_SLEN_T SIZEOF_LONG
# define SLEN_P "l"
#endif

#ifndef __cplusplus
#  define EXTERN_C extern
#else
#  define EXTERN_C extern "C"
#endif

typedef double gfloat;
typedef double gdouble;
typedef int gint;
typedef unsigned int guint;
/** sizeof(char)==1 usually(?), sizeof(long)>=4, sizeof(long)>=sizeof(int)>=
 * sizeof(short)>=2 is required by ANSI C, so the smallest type intagral type
 * of at least 16 bits is always the `short'.
 */
typedef   signed short gint16;
typedef unsigned short guint16;
typedef bool gbool;
typedef unsigned char guchar;
typedef slen_t gsize_t;
typedef char gchar;
typedef unsigned short gushort;
typedef unsigned long gulong;
typedef long glong;
typedef short gshort;
typedef void *gpointer;
typedef unsigned int gdimen_t;

#define MIN(a,b) ((a)<(b)?(a):(b))
#define ABS(a) ((a)<0?-(a):(a))
#define G_N_ELEMENTS(var) (sizeof(var)/sizeof(var[0]))

EXTERN_C char *g_strdup(char const*str);
/** May return NULL. */
EXTERN_C void *g_malloc0(gsize_t len);
EXTERN_C void *g_malloc(gsize_t len);
EXTERN_C void g_free(void /*const*/ const *p);
#define g_new(type,nelems) ((type*)g_malloc((nelems)*sizeof(type))) /* Imp: check retval */
#undef  _
#define _(str) str /* cheap gettext LC_MESSAGE replacement :-) */
/* #define g_free(ptr) free(ptr) */
#define g_assert_not_reached() assert0("notreached");
/** printf-clone */
int g_message(const char *format, ...);
/** printf-clone */
int g_print(const char *format, ...);
#define G_LOG_DOMAIN 1 /* :g_logv(gg_log_domain) */
#define G_LOG_LEVEL_MESSAGE 1 /* :g_logv(gg_log_level) */
#include <stdarg.h> /* va_list */
EXTERN_C void g_logv(unsigned gg_log_domain, unsigned gg_log_level, const char *fmt, va_list ap);

#if MINIGLIB_INCLUDE_TIFFIO_H
#  ifdef VMS
#    ifdef SYSV
#      undef SYSV
#    endif
#    include <tiffioP.h>
#  endif
#  include <tiffio.h>
#endif

#undef PTS_USE_STATIC

#endif /* end of miniglib.h */
