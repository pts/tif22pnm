dnl
dnl aclocal.m4, read by configure
dnl by pts@fazekas.hu at Thu Nov  1 23:07:16 CET 2001
dnl advanced integer checks Sat Apr  6 12:32:02 CEST 2002
dnl
dnl Imp: less autoconf warnings
dnl Dat: m4 SUXX, no normal error message for missing ')'
dnl Dat: FILE *f=fopen("conftestval", "w"); is autoconf trad

AC_DEFUN([AC_PTS_CHECK_INTEGRALS],[
dnl Checks for integral sizes.
AC_REQUIRE([AC_HEADER_STDC])
AC_PTS_C_CHAR_UNSIGNED
dnl ^^^ eliminates autoconf warning `warning: AC_TRY_RUN called without default to allow cross compiling'

dnl by pts@fazekas.hu at Sat Apr  6 12:43:48 CEST 2002
dnl Run some tests in a single burst.
if test x"$ac_cv_sizeof_char" = x; then
AC_MSG_CHECKING(size of char)
AC_CACHE_VAL(ac_cv_sizeof_char,
[ac_cv_sizeof_char_p=-1;ac_cv_sizeof_short=-1;ac_cv_sizeof_int=-1
ac_cv_sizeof_long=-1
AC_TRY_RUN([
#undef const
#undef volatile
#undef inline
#include <stdio.h>
main(){FILE *f=fopen("conftestval", "w");
  if (!f) exit(1);
  fprintf(f, "ac_cv_sizeof_char=%d\n", sizeof(char));
  fprintf(f, "ac_cv_sizeof_char_p=%d\n", sizeof(char*));
  fprintf(f, "ac_cv_sizeof_short=%d\n", sizeof(short));
  fprintf(f, "ac_cv_sizeof_int=%d\n", sizeof(int));
  fprintf(f, "ac_cv_sizeof_long=%d\n", sizeof(long));
  exit(0);
}], eval "`cat conftestval`", , ac_cv_sizeof_char=-1)])
AC_MSG_RESULT($ac_cv_sizeof_char)
AC_DEFINE_UNQUOTED(SIZEOF_CHAR, $ac_cv_sizeof_char)
AC_DEFINE_UNQUOTED(SIZEOF_CHAR_P, $ac_cv_sizeof_char_p)
AC_DEFINE_UNQUOTED(SIZEOF_SHORT, $ac_cv_sizeof_short)
AC_DEFINE_UNQUOTED(SIZEOF_INT, $ac_cv_sizeof_int)
AC_DEFINE_UNQUOTED(SIZEOF_LONG, $ac_cv_sizeof_long)
fi
dnl AC_CHECK_SIZEOF(char, -1)

if test $ac_cv_sizeof_char = -1; then
  AC_MSG_ERROR(cross compiling not supported by .._PTS_CHECK_INTEGRALS)
fi
AC_CHECK_SIZEOF(short, -1)
AC_CHECK_SIZEOF(int, -1)
AC_CHECK_SIZEOF(long, -1)
AC_CHECK_SIZEOF(long long, -1)
if test $ac_cv_sizeof_long_long = 8
then ac_cv_sizeof___int64=0; ac_cv_sizeof_very_long=0; fi
AC_CHECK_SIZEOF(very long, -1)
if test $ac_cv_sizeof_very_long = 8; then ac_cv_sizeof___int64=0; fi
AC_CHECK_SIZEOF(__int64, -1)
dnl ^^^ very long type doesn't exit in any C standard.

dnl Imp: make these cached
# echo $ac_cv_sizeof_long_long
if test $ac_cv_sizeof_char != 1; then
  AC_MSG_ERROR(sizeof(char)!=1)
fi
if test $ac_cv_sizeof_short = 2; then
  ac_cv_pts_int16_t=short
elif test $ac_cv_sizeof_int = 2; then
  ac_cv_pts_int16_t=int
else
  AC_MSG_ERROR(cannot find inttype: sizeof(inttype)==2)
fi
AC_DEFINE_UNQUOTED(PTS_INT16_T, $ac_cv_pts_int16_t)
if test $ac_cv_sizeof_int = 4; then
  ac_cv_pts_int32_t=int
elif test $ac_cv_sizeof_long = 4; then
  ac_cv_pts_int32_t=long
elif test $ac_cv_sizeof_long -lt 4; then
  AC_MSG_ERROR(sizeof(long)<4)
else
  AC_MSG_ERROR(cannot find inttype: sizeof(inttype)==4)
fi
AC_DEFINE_UNQUOTED(PTS_INT32_T, $ac_cv_pts_int32_t)
if test $ac_cv_sizeof_long = 8; then
  ac_cv_pts_int64_t=long
elif test $ac_cv_sizeof_long_long = 8; then
  ac_cv_pts_int64_t="long long"
elif test $ac_cv_sizeof_very_long = 8; then
  ac_cv_pts_int64_t="very long"
elif test $ac_cv_sizeof___int64 = 8; then
  ac_cv_pts_int64_t="__int64"
else
  dnl AC_DEFINE(PTS_INT64_T, 0) -- skip AC_DEFINE(); #undef is OK in aclocal.h
  dnl Imp: put a real #undef into config.h (configure comments it out :-< )
  ac_cv_pts_int64_t=0
  AC_MSG_WARN(cannot find inttype: sizeof(inttype)==8)
fi
AC_DEFINE_UNQUOTED(PTS_INT64_T, $ac_cv_pts_int64_t)
if test $ac_cv_sizeof_long = 16; then
  ac_cv_pts_int128_t=long
elif test $ac_cv_sizeof_long_long = 16; then
  ac_cv_pts_int128_t="long long"
elif test $ac_cv_sizeof_very_long = 16; then
  ac_cv_pts_int128_t="very long"
else
  dnl AC_DEFINE(PTS_INT128_T, 0) -- skip AC_DEFINE(); #undef is OK in aclocal.h
  dnl Imp: put a real #undef into config.h (configure comments it out :-< )
  ac_cv_pts_int128_t=0
  AC_MSG_WARN(cannot find inttype: sizeof(inttype)==16)
fi
AC_DEFINE_UNQUOTED(PTS_INT128_T, $ac_cv_pts_int128_t)
])


AC_DEFUN([AC_PTS_CHECK_POINTERS],[
AC_REQUIRE([AC_PTS_CHECK_INTEGRALS])
AC_CHECK_SIZEOF(char *, -1)
AC_CHECK_SIZEOF(void *, -1)
dnl no need for checking for -1, AC_PTS_CHECK_INTEGRALS already did it
AC_MSG_CHECKING(for an integral type to hold a ptr)
AC_CACHE_VAL(ac_cv_pts_intp_t, [
if test $ac_cv_sizeof_char_p != $ac_cv_sizeof_void_p; then
  AC_MSG_ERROR($ac_cv_sizeof_char_p==sizeof(char*)!=sizeof(void*))
fi
if test $ac_cv_sizeof_char_p -le 2; then
  ac_cv_pts_intp_t=$ac_cv_pts_int16_t
elif test $ac_cv_sizeof_char_p -le 4; then
  ac_cv_pts_intp_t=$ac_cv_pts_int32_t
elif test $ac_cv_sizeof_char_p -le 8; then
  ac_cv_pts_intp_t=$ac_cv_pts_int64_t
elif test $ac_cv_sizeof_char_p -le 16; then
  ac_cv_pts_intp_t=$ac_cv_pts_int128_t
else
  # :; fi; if true; then
  AC_MSG_RESULT(not found!)
  AC_MSG_ERROR(no integral type for sizeof(char*)==$ac_cv_sizeof_char_p)
fi
])dnl AC_CACHE_VAL
AC_DEFINE_UNQUOTED(PTS_INTP_T, $ac_cv_pts_intp_t)
AC_MSG_RESULT($ac_cv_pts_intp_t)
AC_MSG_CHECKING(for ptr <-> integral conversion)
AC_CACHE_VAL(ac_cv_pts_intp_ok, [
  AC_TRY_RUN([
/* #include "confdefs.h" -- automatic */
int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
/* #ifndef __cplusplus; #define const; #endif -- automatic config.h */
   {char const*p="The quick brown fox."+5;
    PTS_INTP_T i;
    while (*p!='r') p++; /* prevent compiler optimisations */
    i=(PTS_INTP_T)p;
    while (*p!='w') p++; /* prevent compiler optimisations */
    return p!=(char*)(i+2); }
  ], ac_cv_pts_intp_ok=yes, ac_cv_pts_intp_ok=no,
  [AC_MSG_ERROR(cross compiling not supported by .._PTS_CHECK_POINTERS)]
)])dnl AC_CACHE_VAL
if test x"$ac_cv_pts_intp_ok" = xyes; then
  AC_DEFINE(PTS_INTP_OK)
fi
AC_MSG_RESULT($ac_cv_pts_intp_ok)
])

dnl Ballast code elmiminated at Sat Apr  6 11:47:08 CEST 2002
dnl  [AC_MSG_RESULT(not found); AC_MSG_ERROR(This is fatal.)],
dnl  [AC_MSG_ERROR(cross compiling not supported by .._PTS_CHECK_INTEGRAL_TYPE)]
dnl)
dnlAC_MSG_RESULT($ac_cv_integral_type_$1)
dnl])

AC_DEFUN([AC_PTS_CHECK_INTEGRAL_TYPEX],[
dnl @param $1: name of type
dnl @param $2: substitution type
dnl @param $3: stored name for the variable
dnl errs out if specified type doesn't exist
dnl please invoke AC_CHECK_HEADERS(unistd.h) 1st
dnl Imp: make it work even if specified type doesn't exist
dnl AC_TYPE_SIZE_T
AC_REQUIRE([AC_HEADER_STDC])
AC_MSG_CHECKING(for integral type $3)
AC_CACHE_VAL(ac_cv_integral_type_$3, [
AC_TRY_RUN([
#include <stdio.h>
/* #include "confdefs.h" -- automatic */
#include <sys/types.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
#include <stdio.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#define MY_TYPE $1
int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
{
  FILE *f=fopen("conftestval", "w");
  if (!f) return 1;
  fprintf(f, 0<(MY_TYPE)-1 ? "unsigned " : "signed ");
       if (sizeof(MY_TYPE)==1) fprintf(f, "char");
  else if (sizeof(MY_TYPE)==sizeof(char))  fprintf(f, "char");
  else if (sizeof(MY_TYPE)==sizeof(short))  fprintf(f, "short");
  else if (sizeof(MY_TYPE)==sizeof(int))  fprintf(f, "int");
  else if (sizeof(MY_TYPE)==sizeof(long)) fprintf(f, "long");
  else if (sizeof(MY_TYPE)==sizeof(PTS_INT16_T))  fprintf(f, "PTS_INT16_T");
  else if (sizeof(MY_TYPE)==sizeof(PTS_INT32_T))  fprintf(f, "PTS_INT32_T");
  else if (sizeof(MY_TYPE)==sizeof(PTS_INT64_T))  fprintf(f, "PTS_INT64_T");
  else if (sizeof(MY_TYPE)==sizeof(PTS_INT128_T)) fprintf(f, "PTS_INT128_T");
  else return 3; /* fprintf(f, "???"); */
  if (ferror(f)) return 2; /* printf("\n"); */
  return 0;
}],
  [ac_cv_integral_type_$3=`cat conftestval`;
      ],
  dnl vvv Imp: make this a macro
  [if test x"$2" = x; then AC_MSG_RESULT(not found); AC_MSG_ERROR(This is fatal.); fi
   ac_cv_integral_type_$3="$2"
   echo -n "substituting "
   dnl ^^^ Imp: echo -n...
   dnl AC_MSG_RESULT(substituting $2) # doesn't work because of AC_CACHE_VAL
  ],
  [AC_MSG_ERROR(cross compiling not supported by .._PTS_CHECK_INTEGRAL_TYPEX)]
)
])dnl AC_CACHE_VAL
AC_DEFINE_UNQUOTED(PTS_$3, $ac_cv_integral_type_$3)
AC_MSG_RESULT($ac_cv_integral_type_$3)
])

AC_DEFUN([AC_PTS_CHECK_INTEGRAL_TYPE],[
  AC_PTS_CHECK_INTEGRAL_TYPEX($1,$2,$1)
])

AC_DEFUN([AC_PTS_TYPE_GETGROUPS], [
  AC_REQUIRE([AC_TYPE_GETGROUPS])
  dnl echo ,$ac_cv_type_getgroups,
  AC_PTS_CHECK_INTEGRAL_TYPEX($ac_cv_type_getgroups, [], getgroups_t)
])


dnl ripped from ruby-1.6.2
AC_DEFUN([AC_PTS_HAVE_PROTOTYPES], [
  AC_CACHE_CHECK(whether cc supports prototypes, ac_cv_pts_have_prototypes, [
    AC_TRY_COMPILE(
      [int foo(int x) { return 0; }], [return foo(10);],
      ac_cv_pts_have_prototypes=yes,
      ac_cv_pts_have_prototypes=no
    )
  ])
  if test x"$ac_cv_pts_have_prototypes" = xyes; then
    AC_DEFINE(HAVE_PROTOTYPES)
  fi
])

dnl by pts@fazekas.hu at Fri Nov  2 13:15:27 CET 2001
AC_DEFUN([AC_PTS_HAVE_STDC], [
dnl Check for working standard C (also called as ANSI C)
dnl implies check for AC_PTS_HAVE_PROTOTYPES, AC_HEADER_STDC
  AC_REQUIRE([AC_HEADER_STDC])
  AC_REQUIRE([AC_PTS_HAVE_PROTOTYPES])
  AC_CACHE_CHECK(whether cc compiles standard C, ac_cv_pts_have_stdc, [
    AC_EGREP_CPP(no, [
#if defined(__STDC__) && __STDC__
  /* note: the starting spaces are deliberate in the next line */
  #if 0
   no
  #endif
#else
  no
#endif
    ], ac_cv_pts_have_stdc=no, [
      if test x"$ac_cv_pts_have_prototypes" = xyes; then
        ac_cv_pts_have_stdc=yes
      else
        ac_cv_pts_have_stdc=no
      fi
    ])
  ])
  if test x"$ac_cv_pts_have_stdc" = xyes; then
    AC_DEFINE(HAVE_PTS_STDC)
  fi
])

dnl by pts@fazekas.hu at Fri Nov  2 13:46:39 CET 2001
AC_DEFUN([AC_PTS_C_VOLATILE], [
  AC_CACHE_CHECK(for C keyword volatile, ac_cv_pts_have_volatile, [
    AC_TRY_COMPILE(
      [volatile int i;], [i=5; i++; return i-6;],
      ac_cv_pts_have_volatile=yes,
      ac_cv_pts_have_volatile=no
    )
  ])
  if test x"$ac_cv_pts_have_volatile" = xno; then
    AC_DEFINE(volatile, )
  fi
])

AC_DEFUN([AC_PTS_CHECK_HEADER], [
dnl @param $1 file to be #included
dnl @param $2 function to be tested
dnl @param $3 cache name, such as `strcpy_in_string'
dnl @param $4 test code (main() function)
dnl better than AC_CHECK_HEADERS because it undefines const and volatile
dnl before including
  AC_CACHE_CHECK(for working $2 in $1, ac_cv_pts_$3, [
    AC_TRY_RUN([
/* #include "confdefs.h" -- automatic */
#undef const
#undef volatile
#undef inline
#include <$1>
#include "confdefs.h"
int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
{
  $4
  return 1;
}],
      ac_cv_pts_$3=yes,
      ac_cv_pts_$3=no,
      [AC_MSG_ERROR(cross compiling not supported by .._PTS_CHECK_HEADER)]
    )
  ])
  if test x"$ac_cv_pts_$3" = xyes; then
    AC_DEFINE_UNQUOTED(HAVE_$3)
  fi
])

dnl by pts@fazekas.hu at Fri Nov  2 15:05:19 CET 2001
AC_DEFUN([AC_PTS_CHECK_STRING], [
dnl defines ac_cv_pts_have_string
dnl defines ac_cv_pts_have_memcmpy_builtin
dnl defines ac_cv_pts_string_header
  AC_PTS_CHECK_HEADER(string.h,  strcpy, strcpy_in_string,   [char s[42]="X"; strcpy(s, "Hello, World!"); return *s!='H';] )
  ac_cv_pts_have_string=no
  if test x"$ac_cv_pts_strcpy_in_string" = xyes; then
    ac_cv_pts_have_string=yes
    ac_cv_pts_string_header=string
  else 
    AC_PTS_CHECK_HEADER(strings.h, strcpy, strcpy_in_strings,  [char s[42]="X"; strcpy(s, "Hello, World!"); return *s!='H';] )
    if test x"$ac_cv_pts_strcpy_in_strings" = xyes; then
      ac_cv_pts_have_string=yes
      ac_cv_pts_string_header=strings
    fi
  fi
  if test x"$ac_cv_pts_have_string" = xyes; then
    AC_DEFINE(HAVE_STRING)
  fi
  CCBAK="$CC"
  CC="${CC:-cc} -Werror"
  AC_PTS_CHECK_HEADER($ac_cv_pts_string_header.h, memcpy, memcpy_in_stringxs,  [char s[42]="X"; memcpy(s, "Hello, World!", 2); return *s!='H'; ])
  CC="$CCBAK"
  
  AC_CACHE_CHECK(whether memcmp and memcpy are built-in, ac_cv_pts_have_memcmpy_builtin, [
    ac_cv_pts_have_memcmpy_builtin=no
    if test x"$ac_cv_pts_have_string" = xyes; then
      dnl They are built-in for gcc 2.7.2.3 -traditional
      if test x"$ac_cv_pts_memcpy_in_stringxs" = xno; then
        AC_TRY_RUN([
int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
{
  char s[42]="X"; memcpy(s, "Hello, World!", 2); return memcmp(s, "H", 1)!=0;
}],
          [ac_cv_pts_have_memcmpy_builtin=yes],
          [AC_MSG_ERROR(missing memcmp and memcpy)],
          [AC_MSG_ERROR(cross compiling not supported by .._PTS_CHECK_STRING)]
        )
      fi
    fi
  ])
  if test x"$ac_cv_pts_have_memcmpy_builtin" = xyes; then
    AC_DEFINE(HAVE_MEMCMPY_BUILTIN)
  fi
])

dnl by pts@fazekas.hu at Fri Nov  2 15:05:27 CET 2001
AC_DEFUN([AC_PTS_CHECK_MALLOC], [
dnl never fails
  AC_PTS_CHECK_HEADER(stdlib.h, malloc,   malloc_in_stdlib,  [char *p=malloc(42); if (p!=0) free(p); return p==0;])
  if test x"$ac_cv_pts_malloc_in_stdlib" != xyes; then
    AC_PTS_CHECK_HEADER(malloc.h, malloc,   malloc_in_malloc,  [char *p=malloc(42); if (p!=0) free(p); return p==0;])
  fi
])


dnl by pts@fazekas.hu at Fri Nov  2 16:51:13 CET 2001
AC_DEFUN([AC_PTS_HAVE_SWITCH_ENUM_BUG], [
  AC_CACHE_CHECK(for switch(enum) bug, ac_cv_pts_have_switch_enum_bug, [
    AC_TRY_RUN([
enum what { foo, bar };
int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
{
  switch ((enum what)1) {
    case foo: return 1;
    case bar: return 0;
  }
  return 1;
}],
      [ac_cv_pts_have_switch_enum_bug=no],
      [ac_cv_pts_have_switch_enum_bug=yes], 
      [AC_MSG_ERROR(cross compiling not supported by .._PTS_HAVE_SWITCH_ENUM_BUG)]
    )
  ])
  if test x"$ac_cv_pts_have_switch_enum_bug" = xyes; then
    AC_DEFINE(HAVE_SWITCH_ENUM_BUG)
  fi
])

dnl by pts@fazekas.hu at Mon Mar  4 09:17:43 CET 2002
AC_DEFUN([AC_PTS_HAVE_ASCII_SYSTEM], [
  AC_CACHE_CHECK(for ASCII system, ac_cv_pts_have_ascii_system, [
    AC_EGREP_CPP(ascii_yes,[
#if 'a'!=97 || '!'!=33
#error You need an ASCII system to compile this.
#else
ascii_yes
#endif
], 
      ac_cv_pts_have_ascii_system=yes, ac_cv_pts_have_ascii_system=no
    )
  ])
  if test x"$ac_cv_pts_have_ascii_system" = xyes; then
    AC_DEFINE(HAVE_ASCII_SYSTEM)
  fi
])

AC_DEFUN([AC_PTS_ENSURE_ASCII_SYSTEM], [
  AC_PTS_HAVE_ASCII_SYSTEM
  if test x"$ac_cv_pts_have_ascii_system" != xyes; then
    AC_MSG_ERROR(you need an ASCII system)
  fi
])

dnl by pts@fazekas.hu at Fri Nov  2 16:51:13 CET 2001
AC_DEFUN([AC_PTS_CFG_P_TMPDIR], [
  AC_CACHE_CHECK(for tmpdir, ac_cv_pts_cfg_p_tmpdir, [
    AC_TRY_RUN([
#define __USE_SVID 1
#define __USE_GNU  1
#include <stdio.h>

int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
{
  FILE *f=fopen("conftestval","w");
  if (!f) return 1;
  fputs(P_tmpdir,f);
  return 0;
}],
      [ac_cv_pts_cfg_p_tmpdir=\""`cat conftestval`"\"],
      [ac_cv_pts_cfg_p_tmpdir=0], 
      [AC_MSG_ERROR(cross compiling not supported by .._PTS_CFG_P_TMPDIR)]
    )
  ])
  AC_DEFINE_UNQUOTED(PTS_CFG_P_TMPDIR, $ac_cv_pts_cfg_p_tmpdir)
])

dnl by pts@fazekas.hu at Fri Mar 22 16:40:22 CET 2002
AC_DEFUN([AC_PTS_HAVE_POPEN_B], [
  AC_CACHE_CHECK(for binary popen_b, ac_cv_pts_have_popen_b, [
    AC_TRY_RUN([
#define _XOPEN_SOURCE 1 /* popen() on Digital UNIX */
#define _POSIX_SOURCE 1 /* popen() */
#define _POSIX_C_SOURCE 2 /* popen() */
#include <stdio.h>

int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
{ FILE *p=popen("echo Ko   rt5","rb");
  int i;
  (void)argc; (void)argv;
  if (p==0) return 1;
  if (getc(p)!='K' || getc(p)!='o' || getc(p)!=' ') return 2;
  if (getc(p)!='r' || getc(p)!='t') return 3;
  if (getc(p)!='f'-'a'+'0') return 4;
  if (getc(p)!='\n') return 5;
  if (getc(p)!=-1) return 6;
  if (0!=pclose(p)) return 7;

  p=popen("cat >conftestval","wb");
  if (p==0) return 31;
  for (i=0;i<666;i++) putc(i,p);
  if (ferror(p)) return 32;
  if (0!=pclose(p)) return 33;
  p=fopen("conftestval","rb");
  if (p==0) return 34;
  for (i=0;i<666;i++) if (getc(p)!=(i&255)) return 35;
  if (fclose(p)!=0) return 36;  

  return 0;
}
],    [ac_cv_pts_have_popen_b=yes],
      [ac_cv_pts_have_popen_b=no], 
      [AC_MSG_ERROR(cross compiling not supported by .._PTS_HAVE_POPEN_B)]
    )
  ])
dnl  AC_DEFINE_UNQUOTED(HAVE_PTS_POPEN_B, $ac_cv_pts_have_popen_b)
  if test x"$ac_cv_pts_have_popen_b" = xyes; then
    AC_DEFINE(HAVE_PTS_POPEN_B)
  fi
])

dnl by pts@fazekas.hu at Fri Mar 22 16:40:22 CET 2002
AC_DEFUN([AC_PTS_HAVE_POPEN_], [
  AC_CACHE_CHECK(for binary popen_, ac_cv_pts_have_popen_, [
    AC_TRY_RUN([
#define _XOPEN_SOURCE 1 /* popen() on Digital UNIX */
#define _POSIX_SOURCE 1 /* popen() */
#define _POSIX_C_SOURCE 2 /* popen() */
#include <stdio.h>

int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
{ FILE *p=popen("echo Ko   rt5","r");
  int i;
  (void)argc; (void)argv;
  if (p==0) return 1;
  if (getc(p)!='K' || getc(p)!='o' || getc(p)!=' ') return 2;
  if (getc(p)!='r' || getc(p)!='t') return 3;
  if (getc(p)!='f'-'a'+'0') return 4;
  if (getc(p)!='\n') return 5;
  if (getc(p)!=-1) return 6;
  if (0!=pclose(p)) return 7;

  p=popen("cat >conftestval","w");
  if (p==0) return 31;
  for (i=0;i<666;i++) putc(i,p);
  if (ferror(p)) return 32;
  if (0!=pclose(p)) return 33;
  p=fopen("conftestval","rb");
  if (p==0) return 34;
  for (i=0;i<666;i++) if (getc(p)!=(i&255)) return 35;
  if (fclose(p)!=0) return 36;  

  return 0;
}
],    [ac_cv_pts_have_popen_=yes],
      [ac_cv_pts_have_popen_=no], 
      [AC_MSG_ERROR(cross compiling not supported by .._PTS_HAVE_POPEN_)]
    )
  ])
dnl  AC_DEFINE_UNQUOTED(HAVE_PTS_POPEN_, $ac_cv_pts_have_popen_)
  if test x"$ac_cv_pts_have_popen_" = xyes; then
    AC_DEFINE(HAVE_PTS_POPEN_)
  fi
])

dnl by pts@fazekas.hu at Fri Mar 22 19:36:26 CET 2002
AC_DEFUN([AC_PTS_HAVE_VSNPRINTF], [
  AC_CACHE_CHECK(for working vsnprintf, ac_cv_pts_have_vsnprintf, [
    AC_TRY_RUN([
#define _BSD_SOURCE 1 /* vsnprintf */
#define _POSIX_SOURCE 1
#define _POSIX_C_SOURCE 2
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static void myprintf(int max, char *dstr, char *fmt, ...) {
  va_list ap;
  #if defined(__STDC__)
    va_start(ap, fmt);
  #else
    va_start(ap);
  #endif
  vsnprintf(dstr, max, fmt, ap);
  va_end(ap);
}

int main
#ifdef __STDC__
(int argc, char **argv)
#else
(argc, argv) int argc; char **argv;
#endif
{ char buf[40];
  int i;
  for (i=0;i<sizeof(buf);i++) buf[i]='X';
  buf[sizeof(buf)-1]='\0';
  myprintf(10, buf, "%s-%ld", "Alma", -1234567L);
  return 0==strcmp(buf, "Alma--1234");
}
],    [ac_cv_pts_have_vsnprintf=yes],
      [ac_cv_pts_have_vsnprintf=no], 
      [AC_MSG_ERROR(cross compiling not supported by .._PTS_HAVE_VSNPRINTF)]
    )
  ])
  if test x"$ac_cv_pts_have_vsnprintf" = xyes; then
    AC_DEFINE(HAVE_PTS_VSNPRINTF)
  fi
])

dnl by pts@fazekas.hu at Sun Mar 31 17:26:01 CEST 2002
AC_DEFUN([AC_PTS_HAVE_ERRNO_DECL], [
  AC_CACHE_CHECK([for errno in errno.h],ac_cv_pts_errno_decl, [
      AC_TRY_COMPILE([#include <errno.h>],[int i = errno],
          ac_cv_pts_errno_decl=yes,ac_cv_pts_have_errno_decl=no)])
  if test x"$ac_cv_pts_errno_decl" = x"yes"; then
     AC_DEFINE(HAVE_ERRNO_DECL, 1, [ ])
  fi
])

dnl by pts@fazekas.hu at Sun Mar 31 22:19:16 CEST 2002
dnl modeled after Autoconf internal AC_EGREP_CPP
dnl AC_PTS_SED_EGREP_CPP(SED-PATTERN, EGREP-PATTERN, PROGRAM, [ACTION-IF-FOUND [,
dnl              ACTION-IF-NOT-FOUND]])
AC_DEFUN(AC_PTS_SED_EGREP_CPP,
[AC_REQUIRE_CPP()dnl
cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
[$3]
EOF
dnl eval is necessary to expand ac_cpp.
dnl Ultrix and Pyramid sh refuse to redirect output of eval, so use subshell.
if (eval "$ac_cpp conftest.$ac_ext") 2>&AC_FD_CC |
dnl Prevent m4 from eating regexp character classes:
changequote(, )dnl
  sed -n '$1' | egrep '$2' >/dev/null 2>&1; then
changequote([, ])dnl
  ifelse([$4], , :, [rm -rf conftest*
  $4])
ifelse([$5], , , [else
  rm -rf conftest*
  $5
])dnl
fi
rm -f conftest*
])

dnl by pts@fazekas.hu at Sun Mar 31 22:23:48 CEST 2002
dnl Digital UNIX OSF/1 has it in _some_ cases.
AC_DEFUN([AC_PTS_HAVE_GETSOCKNAME_SIZE_T], [
  AC_CACHE_CHECK(whether getsockname accepts size_t, ac_cv_pts_have_getsockname_size_t, [
    AC_PTS_SED_EGREP_CPP([/getsockname/,/)/p], [size_t],
      [#include <sys/socket.h>],
      [ac_cv_pts_have_getsockname_size_t=yes],
      [ac_cv_pts_have_getsockname_size_t=no]
    )
  ])
  if test x"$ac_cv_pts_have_getsockname_size_t" = xyes; then
    AC_DEFINE(HAVE_PTS_GETSOCKNAME_SIZE_T)
  fi
])

dnl by pts@fazekas.hu at Sun Mar 31 22:23:48 CEST 2002
dnl Digital UNIX OSF/1 has it.
AC_DEFUN([AC_PTS_HAVE_GETSOCKNAME_SOCKLEN_T], [
  AC_CACHE_CHECK(whether getsockname accepts socklen_t, ac_cv_pts_have_getsockname_socklen_t, [
    AC_PTS_SED_EGREP_CPP([/getsockname/,/)/p], [socklen_t],
      [#include <sys/socket.h>],
      [ac_cv_pts_have_getsockname_socklen_t=yes],
      [ac_cv_pts_have_getsockname_socklen_t=no]
    )
  ])
  if test x"$ac_cv_pts_have_getsockname_socklen_t" = xyes; then
    AC_DEFINE(HAVE_PTS_GETSOCKNAME_SOCKLEN_T)
  fi
])

dnl by pts@fazekas.hu at Sun Mar 31 23:50:05 CEST 2002
dnl Digital UNIX OSF/1 has it.
AC_DEFUN([AC_PTS_ENABLE_DEBUG], [
  AC_MSG_CHECKING([for --enable-debug])
  AC_SUBST(ENABLE_DEBUG)
  AC_ARG_ENABLE(debug,
    [  --enable-debug[=val]    val: no, assert(default), yes, checker],
    [], [])
  ENABLE_DEBUG="$enable_debug"
  if test x"$enable_debug" = xno; then
    AC_MSG_RESULT(no)
  elif test x"$enable_debug" = x || test x"$enable_debug" = xassert; then
    AC_MSG_RESULT(assert)
    ENABLE_DEBUG=assert
  elif test x"$enable_debug" = xyes; then
    AC_MSG_RESULT(yes)
  elif test x"$enable_debug" = xchecker; then
    AC_MSG_RESULT(checker)
  else
    AC_MSG_RESULT("?? $enable_debug")
    AC_MSG_ERROR(Invalid --enable--debug option specified.)
  fi
])

dnl by pts@fazekas.hu at Thu Apr  4 13:25:20 CEST 2002
dnl Keeps conftest*
AC_DEFUN(AC_PTS_TRY_COMPILE_NORM,
[cat > conftest.$ac_ext <<EOF
ifelse(AC_LANG, [FORTRAN77],
[      program main
[$2]
      end],
[dnl This sometimes fails to find confdefs.h, for some reason.
dnl [#]line __oline__ "[$]0"
[#]line __oline__ "configure"
#include "confdefs.h"
[$1]
int main() {
[$2]
; return 0; }
])EOF
if AC_TRY_EVAL(ac_compile); then
  ifelse([$3], , :, [
  $3])
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
ifelse([$4], , , [  
  $4
])dnl
fi
rm -f conftest*])

dnl by pts@fazekas.hu at Thu Apr  4 13:00:08 CEST 2002
dnl Checks whether `gcc' can link C++ programs (and thus libstdc++ is
dnl avoided).
AC_DEFUN([AC_PTS_GCC_LINKS_CXX], [
  AC_REQUIRE([AC_LANG_CPLUSPLUS])
  AC_REQUIRE([AC_PROG_CXX])
  AC_CACHE_CHECK(whether gcc can link C++ code, ac_cv_pts_gcc_links_cxx, [
    CXX_saved="$CXX"
    ac_ext_saved="$ac_ext"
    AC_PTS_TRY_COMPILE_NORM([
      #include <stdio.h>
      struct B            { virtual int f()=0; };
      struct C1: public B { virtual int f() {return 1;}};
      struct C0: public B { virtual int f() {return 2;}};
    ],[
      B *p=(ferror(stderr))?new C1():new C0(); /* Imp: argc... */
      /* if (p==0) throw long(42); */
      return p->f();
    ],[
      CXX=gcc
      ac_ext="$ac_objext"
      # bash
      if AC_TRY_EVAL(ac_link) && test -s conftest${ac_exeext}
      then ac_cv_pts_gcc_links_cxx=yes;
      else ac_cv_pts_gcc_links_cxx=no; fi
    ],[ac_cv_pts_gcc_links_cxx=compilation-failed])
    CXX="$CXX_saved"
    ac_ext="$ac_ext_saved"
  ])
  case x"$ac_cv_pts_gcc_links_cxx" in
    xyes) LDXX=gcc ;;
    xno)  LDXX="$CXX" ;;
    *)   AC_MSG_ERROR([Compilation failed, aborting.]) ;;
  esac
  AC_SUBST(LDXX)
])

dnl by pts@fazekas.hu at Sat Apr  6 10:56:48 CEST 2002
dnl based on AC_PROG_CXX_G from acspecific.m4 of autoconf
dnl Appends the specified flags to CXXFLAGS, so it will affect subsequent AC_*
dnl tests.
dnl Example: AC_PTS_PROG_CXXFLAGS(nrne, -fno-rtti -fno-exceptions)
AC_DEFUN(AC_PTS_PROG_CXXFLAGS,
[AC_REQUIRE([AC_PROG_CXX])
AC_CACHE_CHECK(whether ${CXX-g++} accepts $2, ac_cv_prog_cxx_$1,
[echo 'void f(void);void f(void){}' > conftest.cc
if test -z "`${CXX-g++} $2 -c conftest.cc 2>&1`"
then ac_cv_prog_cxx_$1=yes; 
else ac_cv_prog_cxx_$1=no; fi
rm -f conftest*
])
if test $ac_cv_prog_cxx_$1 = yes; then CXXFLAGS="$CXXFLAGS $2"; fi
])

dnl by pts@fazekas.hu at Sat Apr  6 10:56:48 CEST 2002
dnl based on AC_PROG_CXX_G from acspecific.m4 of autoconf
dnl Appends the specified flags to CXXFLAGSB, so it won't affect subsequent
dnl AC_* tests.
dnl Example: AC_PTS_PROG_CXXFLAGSB(apwaw, -ansi -pedantic -Wall -W)
AC_DEFUN(AC_PTS_PROG_CXXFLAGSB,
[AC_REQUIRE([AC_PROG_CXX])
AC_SUBST(CXXFLAGSB)
AC_CACHE_CHECK(whether ${CXX-g++} accepts $2, ac_cv_prog_cxx_$1,
[echo 'void f(void);void f(void){}' > conftest.cc
if test -z "`${CXX-g++} $2 -c conftest.cc 2>&1`"
then ac_cv_prog_cxx_$1=yes;
else ac_cv_prog_cxx_$1=no; fi
rm -f conftest*
])
if test $ac_cv_prog_cxx_$1 = yes; then CXXFLAGSB="$CXXFLAGSB $2"; fi
])

dnl by pts@fazekas.hu at Sat Apr  6 10:56:48 CEST 2002
dnl based on AC_PROG_CXX_G from acspecific.m4 of autoconf
dnl Appends the specified flags to CFLAGS, so it will affect subsequent AC_*
dnl tests.
dnl Example: AC_PTS_PROG_CFLAGS(nrne, -fno-rtti -fno-exceptions)
AC_DEFUN(AC_PTS_PROG_CFLAGS,
[AC_REQUIRE([AC_PROG_CC])
AC_CACHE_CHECK(whether ${CC-gcc} accepts $2, ac_cv_prog_cc_$1,
[echo 'void f(void);void f(void){}' > conftest.c
if test -z "`${CC-gcc} $2 -c conftest.c 2>&1`"
then ac_cv_prog_cc_$1=yes; 
else ac_cv_prog_cc_$1=no; fi
rm -f conftest*
])
if test $ac_cv_prog_cc_$1 = yes; then CFLAGS="$CFLAGS $2"; fi
])

dnl by pts@fazekas.hu at Sat Apr  6 10:56:48 CEST 2002
dnl based on AC_PROG_CXX_G from acspecific.m4 of autoconf
dnl Appends the specified flags to CFLAGSB, so it won't affect subsequent
dnl AC_* tests.
dnl Example: AC_PTS_PROG_CFLAGSB(apwaw, -ansi -pedantic -Wall -W)
AC_DEFUN(AC_PTS_PROG_CFLAGSB,
[AC_REQUIRE([AC_PROG_CC])
AC_SUBST(CFLAGSB)
AC_CACHE_CHECK(whether ${CC-gcc} accepts $2, ac_cv_prog_cc_$1,
[echo 'void f(void);void f(void){}' > conftest.c
if test -z "`${CC-gcc} $2 -c conftest.c 2>&1`"
then ac_cv_prog_cc_$1=yes;
else ac_cv_prog_cc_$1=no; fi
rm -f conftest*
])
if test $ac_cv_prog_cc_$1 = yes; then CFLAGSB="$CFLAGSB $2"; fi
])

dnl by pts@fazekas.hu at Sat Apr  6 11:49:59 CEST 2002
dnl Similar to AC_C_CHAR_UNSIGNED, but without the
dnl `warning: AC_TRY_RUN called without default to allow cross compiling'.
AC_DEFUN(AC_PTS_C_CHAR_UNSIGNED,
[AC_CACHE_CHECK(whether char is unsigned, ac_cv_c_char_unsigned,
[if test "$GCC" = yes; then
  # GCC predefines this symbol on systems where it applies.
AC_EGREP_CPP(yes,
[#ifdef __CHAR_UNSIGNED__
  yes
#endif
], ac_cv_c_char_unsigned=yes, ac_cv_c_char_unsigned=no)
else
AC_TRY_RUN(
[/* volatile prevents gcc2 from optimizing the test away on sparcs.  */
#if !defined(__STDC__) || __STDC__ != 1
#define volatile
#endif
main() {
  volatile char c = 255; exit(c < 0);
}], ac_cv_c_char_unsigned=yes, ac_cv_c_char_unsigned=no,
  [AC_MSG_ERROR(cross compiling not supported by .._PTS_C_CHAR_UNSIGNED)])
fi])
if test $ac_cv_c_char_unsigned = yes && test "$GCC" != yes; then
  AC_DEFINE(__CHAR_UNSIGNED__)
fi
])

dnl __EOF__
