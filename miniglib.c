/* miniglib.c -- minimalistic glib-style misc accessory functions
 * by pts@fazekas.hu at Sun Sep 29 22:13:46 CEST 2002
 */

#ifdef __GNUC__
#pragma implementation
#endif

#include "miniglib.h"
#include <stdio.h> /* vprintf() */
#include <string.h> /* strlen() */
#include <stdlib.h> /* malloc() */

int g_message(const char *fmt, ...) {
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret=vfprintf(stderr, fmt, ap);
  va_end(ap);
  fflush(stderr);
  return ret;
}

int g_print(const char *fmt, ...) {
  /* Dat: currently same as g_message() */
  va_list ap;
  int ret;
  va_start(ap, fmt);
  ret=vfprintf(stdout, fmt, ap);
  va_end(ap);
  return ret;
}

void g_logv(unsigned gg_log_domain, unsigned gg_log_level, const char *fmt, va_list ap) {
  fprintf(stderr, "g_logv domain=%u level=%u: \n", gg_log_domain, gg_log_level);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, ".\n");
}
char *g_strdup(char const*str) {
  char *t=(char*)NULLP;
  if (str!=NULLP) strcpy(t=(char*)g_malloc(strlen(str)+1),str);
  return t;
}
void *g_malloc(gsize_t len) {
  void *p=malloc(len);
  if (!p) { fputs("Out of memory!\n", stderr); fflush(stderr); abort(); }
  return p;
}
void *g_malloc0(gsize_t len) {
  return malloc(len);
}
void g_free(void *p) { if (p!=NULLP) free(p); }

/* __END__ */
