/* tif22pnm.c -- converts TIFF (Tag Image File format) to PNM + Alpha PGM/PPM
 * by pts@fazekas.hu at Sat Apr 13 17:27:44 CEST 2002
 * Sat Apr 13 18:25:26 CEST 2002: PGM, PPM with maxval==255 work
 * Sat Apr 13 20:23:52 CEST 2002: even alpha works; TIFF more compatible
 * 0.04 Tue Jun 11 18:08:12 CEST 2002: auto out file format selection
 * 0.05: better alpha dump, 21400 bytes
 * 0.06 Sat Oct  5 14:24:03 CEST 2002 integrate TIFF loader of GIMP 1.3
 * 0.07 Sun Oct 13 19:44:46 CEST 2002:
 *   -rgbu et al.
 *   -text implemented
 *   BUGFIXes
 *
 * v0.05 derived from: tifftopnm.c - converts a Tagged Image File to a portable anymap
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Original tifftopnm doesn't handle grayscale images with transparency. I do.
 * Original tifftopnm doesn't handle CMYK images. I do (in TIFFRGB*).
 *
 * !! Imp: verify alpha-assoc-idempotence
 */

/**** pts ****/
#include "ptspnm.h"
#include "ptstiff3.h"
#include "minigimp.h"
#include <stdio.h>
#include <string.h> /* strcmp() */
#ifdef __MINGW32__
#undef __STRICT_ANSI__
#include <fcntl.h> /* setmode() */
#endif

#define PRODUCT "tif22pnm"
#define VERSION "0.08"

/* --- string manipulation */

#define USGE(a,b) ((unsigned char)(a))>=((unsigned char)(b))

static gbool nocase_strbegins(register char const*a, register char const *with) {
  while (*with!='\0')
    if ((USGE('Z'-'A',*a-'A') ? *a+++'a'-'A' : *a++)
      !=(USGE('Z'-'A',*with-'A') ? *with+++'a'-'A' : *with++)
       ) return FALSE;
  return TRUE;
}

static gbool nocase_streq(register char const*a, register char const *with) {
  while (*with!='\0' && *a!='\0')
    if ((USGE('Z'-'A',*a-'A') ? *a+++'a'-'A' : *a++)
      !=(USGE('Z'-'A',*with-'A') ? *with+++'a'-'A' : *with++)
       ) return FALSE;
  return *with==*a; /* BUGFIX at Sun Dec  8 19:38:05 CET 2002 */
}

/* --- main */

static GimptsFileFormat parse_selector(char const *arg) {
       if (nocase_strbegins(arg, "PSL2:")
        || nocase_strbegins(arg, "EPS2:")
        || nocase_strbegins(arg, "EPS:")
        || nocase_strbegins(arg, "PS2:")
        || nocase_strbegins(arg, "PS:")
        || nocase_strbegins(arg, "PSL1:")
        || nocase_strbegins(arg, "PSLC:")
        || nocase_strbegins(arg, "PSL3:")) return GIMPTS_FF_eps;
  else if (nocase_strbegins(arg, "PNG:")) return GIMPTS_FF_PNG;
  else if (nocase_strbegins(arg, "TIFF:")
        || nocase_strbegins(arg, "TIF:")) return GIMPTS_FF_TIFF;
  else if (nocase_strbegins(arg, "GIF89a:")
        || nocase_strbegins(arg, "GIF:")) return GIMPTS_FF_GIF;
  else if (nocase_strbegins(arg, "PNM:")) return GIMPTS_FF_PNM;
  else if (nocase_strbegins(arg, "PBM:")) return GIMPTS_FF_pbm;
  else if (nocase_strbegins(arg, "PGM:")) return GIMPTS_FF_pgm;
  else if (nocase_strbegins(arg, "PPM:")) return GIMPTS_FF_ppm;
  while (arg[0]!=':' && arg[0]!='\0') arg++;
  return (arg[0]==':') ? GIMPTS_FF_INVALID : GIMPTS_FF_UNKNOWN;
}

static GimptsFileFormat parse_ext(char const *arg) {
       if (nocase_strbegins(arg, "eps")
        || nocase_strbegins(arg, "epsi")
        || nocase_strbegins(arg, "epsf")
        || nocase_strbegins(arg, "ps")) return GIMPTS_FF_eps;
  else if (nocase_strbegins(arg, "png")) return GIMPTS_FF_PNG;
  else if (nocase_strbegins(arg, "tiff")
        || nocase_strbegins(arg, "tif")) return GIMPTS_FF_TIFF;
  else if (nocase_strbegins(arg, "gif")) return GIMPTS_FF_GIF;
  else if (nocase_strbegins(arg, "pnm")) return GIMPTS_FF_PNM;
  else if (nocase_strbegins(arg, "pbm")) return GIMPTS_FF_pbm;
  else if (nocase_strbegins(arg, "pgm")) return GIMPTS_FF_pgm;
  else if (nocase_strbegins(arg, "ppm")) return GIMPTS_FF_ppm;
  return GIMPTS_FF_UNKNOWN;
}

int main ___((int argc, char const* const* argv), ( argc, argv ), 
 (int argc; char const* const* argv;)) {
  gbool headerdump_p=0; /* Imp: eliminate this? */
  gbool dump_rgb_p=1, rawbits_p=1, refuse_opts=FALSE;
  int dump_alpha=1;
  char const* arg;
  char const* ap;
  /* char const* ar; */
  char const* const* args;
  char const* inputfile=(char const*)NULLP;
  char const* outputfile=(char const*)NULLP;
  GimptsFileFormat out_ff=GIMPTS_FF_UNKNOWN;
  /* GimptsTransferEncoding out_te=GIMPTS_TE_UNKNOWN; */
  gbool save_ok=TRUE;

  gint32 himage, hdrawable;
  GimptsFileFormat ff=GIMPTS_FF_UNKNOWN;
  
  char const* usage =(char const*)"[options] [--] in.img [out.img]\nOptions are:\n"
    "-headerdump\n"
    "-rgb   opaque RGB, alpha associated to black background (default)\n"
    "-rgbi  RGB, ignore alpha\n"
    "-rgba  RGB with associated alpha (black background)\n"
    "-rgbu  RGB with unassociated alpha\n"
    "-onlya alpha, ignore RGB\n"
    "-text\n";

  (void)argc;
  args=argv+1;
  if (*args==NULLP) goto usage;
  while (*args!=NULLP) {
    arg=*args++;
    if (arg[0]=='\0') {
      fprintf(stderr, PRODUCT": empty arg\n");
      return 10;
    }
    if (refuse_opts || arg[0]!='-' || arg[1]=='\0') { /* set_inputfile: */
      GimptsFileFormat ff;
      if (inputfile==NULLP) { inputfile=arg; continue; }
      if (outputfile!=NULLP) goto usage;
      if ((ff=parse_selector(arg))==GIMPTS_FF_INVALID) {
        fprintf(stderr, PRODUCT": invalid selector for: %s\n", arg);
        return 8;
      }
      if (ff!=GIMPTS_FF_UNKNOWN) {
        if (out_ff!=GIMPTS_FF_UNKNOWN && out_ff!=ff) {
          fprintf(stderr, PRODUCT": selector ambiguoates: %s\n", arg);
          return 9;
        }
        out_ff=ff;
        while (arg[0]!=':') arg++;
        arg++;
      }
      if (arg[0]=='\0') continue; /* only a selector */
      outputfile=arg;
      if (out_ff==GIMPTS_FF_UNKNOWN) {
        ap=arg+strlen(arg);
        while (ap!=arg && ap[0]!='.') ap--;
        /* ^^^ Dat: extra care later for /a/b.c/d */
        /* found a possibly useful extension? */
        /* printf("ap=%s.\n", ap); */
        if (ap!=arg && (ff=parse_ext(ap+1))!=GIMPTS_FF_UNKNOWN) out_ff=ff;
      }
      continue;
    }
    if (arg[1]=='-' && arg[2]=='\0') { refuse_opts=TRUE; continue; }
    while (arg[0]=='-') arg++;
         if (nocase_streq(arg, "headerdump") ) headerdump_p = 1;
    else if (nocase_streq(arg, "rgba") ) { dump_rgb_p=1; dump_alpha=3; }
    else if (nocase_streq(arg, "onlya")) { dump_rgb_p=0; dump_alpha=2; }
    else if (nocase_streq(arg, "rgbu") ) { dump_rgb_p=1; dump_alpha=2; }
    else if (nocase_streq(arg, "rgb")  ) { dump_rgb_p=1; dump_alpha=1; } /* default */
    else if (nocase_streq(arg, "rgbi") ) { dump_rgb_p=1; dump_alpha=0; }
    else if (nocase_streq(arg, "text") ) { rawbits_p=0; }
    else { usage:
      fprintf(stderr, "This is tif22pnm v"VERSION": TIFF <-> PNM converter, by pts@fazekas.hu\nUsage: %s %s", argv[0], usage);
      return 3;
    }
  } /* WHILE */

  if (out_ff==GIMPTS_FF_UNKNOWN) {
    fprintf(stderr, PRODUCT": OutputFormat unspecified\n");
    return 11;
  }
  if (inputfile==NULLP) goto usage;
  if (outputfile==NULLP) outputfile="-";
  if (0==strcmp(inputfile,"-")) {
    fprintf(stderr, PRODUCT": inputfile cannot be `-' (stdin)\n");
    return 4;
    /* inputfile=(char const*)NULLP; */
  }

  if (0==strcmp(outputfile, "-")) {
#ifdef __MINGW32__
    setmode(1, O_BINARY);
#endif
    outputfile=(char const*)NULLP;
  }
  /* Imp: differennt format for .ppm, .pgm, .pbm and .pnm */
  /* Imp: respect output .ext, PGM: */

  ff=gimpts_magic(inputfile); /* cannot be STDIN! */
  switch (ff) {
   case GIMPTS_FF_TIFF: himage=ptstiff3_load_image(inputfile, headerdump_p); break;
   case GIMPTS_FF_PNM: himage=ptspnm_load_image(inputfile); break;
   case GIMPTS_FF_UNKNOWN: fprintf(stderr, PRODUCT": unknown image format\n"); return 5;
   case GIMPTS_FF_INVALID: fprintf(stderr, PRODUCT": invalid image format or file not found\n"); return 6;
   default: fprintf(stderr, PRODUCT": unsupported input image format\n"); return 7;
  }
  hdrawable=gimpts_image_first_drawable(himage);
  gimpts_drawable_packpal(/*hdrawable:*/0, himage);

  if (out_ff==GIMPTS_FF_PNM || out_ff==GIMPTS_FF_pbm
   || out_ff==GIMPTS_FF_pgm || out_ff==GIMPTS_FF_ppm) {
    if ((dump_alpha&1)!=0) { /* associtate alpha with black background */
      guchar black[3]={0,0,0};
      gimpts_drawable_alpha_assoc(hdrawable, black);
    }
    /* printf("dump_rgb_p=%u dump_alpha=%u out_ff=%u\n", dump_rgb_p, dump_alpha, out_ff); */
    /* Sat Oct  5 14:25:24 CEST 2002 -- Sun Oct  6 22:55:31 CEST 2002 */
    save_ok=ptspnm_save_image(
      /*outputfile:*/outputfile,
      himage,
      /*hdrawable:*/0,
      dump_rgb_p     ? out_ff : GIMPTS_FF_NONE,
      (dump_alpha>1) ? out_ff : GIMPTS_FF_NONE,
      rawbits_p);
    gimp_image_delete(himage);
  } else if (out_ff==GIMPTS_FF_TIFF) {
    gint32 hsaved=hdrawable;
    if (gimp_drawable_type(hdrawable)==GIMP_INDEXEDA_IMAGE) {
      /* ptstiff3_save_image() cannot save _INDEXEDA_ images */
      hsaved=gimpts_drawable_dup(hdrawable, GIMP_RGBA_IMAGE);
    }
    save_ok=ptstiff3_save_image(outputfile, himage, hsaved, himage, /*compression:*/0, /*image_comment:*/NULLP);
    if (hsaved!=hdrawable) gimp_drawable_free(hsaved);
  } else {
    fprintf(stderr, PRODUCT": target FileFormat unsupported\n");
    return 12;
  }
  return save_ok ? 0 : 1;
}

/* __END__ */
