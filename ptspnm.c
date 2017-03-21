/* ptspnm.c -- reads and writes PNM files, without external libraries
 * by pts@fazekas.hu at Sun Oct  6 19:27:23 CEST 2002
 */

#if defined(__GNUC__) && defined(__cplusplus)
#pragma implementation
#endif

#include "ptspnm.h"
#include "minigimp.h"
#include <stdio.h>

static void save_pbm(FILE *f, GimpDrawable *gd, GimpPixelRgn *rr, gbool rawbits, char const *comment) {
  guchar *buf, *pend; gsize_t buflen;
  register guchar *p, *t;
  register unsigned smallen;
  guchar tmp[71];
  p=buf=g_new(guchar, buflen=((gd->width+7)>>3)*gd->height);
  pend=p+buflen;
  gimp_pixel_rgn_get_rect_bpc(rr, buf, 0, 0, gd->width, gd->height, 1, FALSE);
  fprintf(f, "P%c%s%"SLEN_P"u %"SLEN_P"u\n", (rawbits?'4':'1'), comment, (slen_t)gd->width, (slen_t)gd->height);
  if (rawbits) { 
    while (p!=pend) *p++^=255; /* PBM: 1 is black */
    fwrite(buf, buflen, 1, f);
  } else { /* PBM ASCII */
    t=(guchar*)NULLP;
    while (buflen>=70) {
      smallen=70; t=tmp; while (smallen--!=0) *t++=(*p++==0)?'1':'0';
      /* ^^^ note the swapping of '0' and '1' above */
      *t++='\n'; fwrite(tmp, 71, 1, f);
      buflen-=70;
    }
    while (buflen--!=0) *t++=(*p++==0)?'0':'1';
    /* Dat: xv requires a whitespace just before EOF */
    *t++='\n'; fwrite(tmp, t-tmp, 1, f);
  }
  g_free(buf);
}


static void save_pgpm(FILE *f, GimpDrawable *gd, GimpPixelRgn *rr, gbool rawbits, char const *comment, gbool ppm) {
  gsize_t buflen;
  register guchar *p, *t;
  register unsigned smallen, i;
  guchar tmp[71];

  p=gimpts_pixel_rgn_get_topleft(rr);
  buflen=(gsize_t)(ppm?3:1)*gd->width*gd->height;
  fprintf(f, "P%c%s%"SLEN_P"u %"SLEN_P"u 255\n", (rawbits?'5':'2')+(ppm?1:0), comment, (slen_t)gd->width, (slen_t)gd->height);
  if (rawbits) {
    fwrite(p, buflen, 1, f);
  } else { /* PGM ASCII */
    while (buflen!=0) {
      if (buflen>17) { smallen=17; buflen-=17; } else { smallen=buflen; buflen=0; }
      t=tmp; while (smallen--!=0) {
        if ((i=*(unsigned char*)p++)<10) *t++=i+'0';
        else if (i<100) {           *t++=i/10+'0'; *t++=i%10+'0'; }
        else if (i<200) { *t++='1'; *t++=(i-100)/10+'0'; *t++=i%10+'0'; }
                   else { *t++='2'; *t++=(i-200)/10+'0'; *t++=i%10+'0'; }
        *t++=' ';
      }
      /* Dat: xv requires a whitespace just before EOF */
      t[-1]='\n'; fwrite(tmp, t-tmp/*-(buflen==0)*/, 1, f);
    }
  }

}

int ptspnm_save_image(gchar const*filename0, gint32 himage, gint32 hdrawable,
 gint sphoto, gint salpha, gbool rawbits) {
  /** Must be >=20 bytes long to pacify xv (filesize>=30 bytes) */
  /* Imp: respect param rawbits */
  static char const* comment="\n# PNM exported by PtsGIMP\n";
  char const*filename=filename0;
  FILE *f;
  GimpPixelRgn r;
  GimpDrawable *gd;
  gint32 hsaved;
  GimpImageType type;
  
#if 0
  filename=*(char**)&filename0; /* pacify gcc const_cast warning */
#endif
  f=(filename!=NULLP)?fopen(filename,"wb"):stdout;
  if (!f) { writerr:
    g_message ("Can't write PNM image to `%s'\n", filename);
    return 0;
  }
  param_assert(himage!=0 || hdrawable!=0);
  if (hdrawable==0) {
    /* img=resFind(himage); assert(IS_IMAGE(img)); */
    hdrawable=gimpts_image_first_drawable(himage);
    param_assert(hdrawable!=0);
  }
  hsaved=hdrawable;
  type=gimp_drawable_type(hdrawable);
  /* g_message("save PNM sphoto=%u salpha=%u rawbits=%u\n", sphoto, salpha, rawbits); */
  
  if (sphoto!=GIMPTS_FF_NONE) {
    if (sphoto==GIMPTS_FF_PNM) { /* we choose */
      GimptsMeta meta;
      gimpts_meta_init(&meta);
      /* vvv !! strip fully transparent pixels before going on */
      gimpts_meta_update(&meta, hdrawable);
      sphoto=(!meta.canGray)     ? GIMPTS_FF_ppm :
             (meta.minRGBBpc!=1) ? GIMPTS_FF_pgm : GIMPTS_FF_pbm;
      if (salpha==GIMPTS_FF_PNM) /* avoid calling gimpts_meta_update() again */
        salpha=(meta.minAlphaBpc!=1) ? GIMPTS_FF_pgm : GIMPTS_FF_pbm;
    }
    if (sphoto==GIMPTS_FF_ppm) {
      if (type!=GIMP_RGB_IMAGE)  { hsaved=gimpts_drawable_dup(hdrawable, GIMP_RGB_IMAGE );}
    } else {
      if (type!=GIMP_GRAY_IMAGE) { hsaved=gimpts_drawable_dup(hdrawable, GIMP_GRAY_IMAGE); /* extract red only */ }
    }
    gd=gimp_drawable_get(hsaved);
    gimp_pixel_rgn_init(&r, gd, 0, 0, gd->width, gd->height, FALSE, FALSE);
    switch (sphoto) {
     case GIMPTS_FF_pbm:
      save_pbm(f, gd, &r, rawbits, comment);
      break;
     case GIMPTS_FF_pgm:
      save_pgpm(f, gd, &r, rawbits, comment, FALSE);
      break;
     case GIMPTS_FF_ppm:
      save_pgpm(f, gd, &r, rawbits, comment, TRUE);
      break;
     default: assert(0);
    }
    gimp_drawable_detach(gd);
    if (hsaved!=hdrawable) gimp_drawable_free(hsaved);
    comment=" ";
  }

  if (salpha!=GIMPTS_FF_NONE && (sphoto==GIMPTS_FF_NONE || IS_DRAWABLE_ALPHA(type))) {
    /* printf("saving alpha\n"); */
    fflush(f);
    hsaved=gimpts_drawable_extract_alpha(hdrawable);
    gd=gimp_drawable_get(hsaved);
    if (salpha==GIMPTS_FF_PNM) { /* we choose */
      GimptsMeta meta;
      gimpts_meta_init(&meta);
      gimpts_meta_update(&meta, hsaved);
      salpha=(meta.minAlphaBpc!=1) ? GIMPTS_FF_pgm : GIMPTS_FF_pbm;
    }
    gimp_pixel_rgn_init(&r, gd, 0, 0, gd->width, gd->height, FALSE, FALSE);

    switch (salpha) {
     case GIMPTS_FF_pbm:
      save_pbm(f, gd, &r, rawbits, comment);
      break;
     case GIMPTS_FF_pgm:
      save_pgpm(f, gd, &r, rawbits, comment, FALSE);
      break;
     default: assert(0);
    }
    gimp_drawable_detach(gd);
    gimp_drawable_free(hsaved);
  }
  
  if (ferror(f)) { fclose(f); goto writerr; }
  if (filename!=NULLP) fclose(f);
  return 1;
}

/* --- loading */

#define ULE(a,b) (((a)+0U)<=((b)+0U))
#define ISWSPACE(i,j) ((i j)==32 || ULE(i-9,13-9) || i==0)
#if ';'!=59 || 'a'!=97
#  error ASCII system is required to compile this program
#endif

static gint32 getint(FILE *f) {
  int c, sgn=1;
  gint32 v, w;
  while (1) {
    while (ISWSPACE(c,=getc(f))) ;
    if (c!='#') break;
    while ((c=getc(f))!='\n' && c!='\r') {} /* skip comment */
  }
  if (c=='-') { c=getc(f); sgn=-1; }
  if (!ULE(c-'0','9'-'0')) { g_message("PNM: integer expected\n"); gimp_quit(); }
  /* ^^^ Dat: EOF err */
  v=sgn*(c-'0');
  while (ULE((c=getc(f))-'0','9'-'0')) {
    if ((w=10*v+sgn*(c-'0'))/10!=v) { g_message("PNM: integer overflow\n"); gimp_quit(); }
    /* ^^^ Dat: overflow check is ok between -MAX-1 .. MAX */
    v=w;
  }
  while (c=='#') { /* skip comments */
    while ((c=getc(f))!='\n' && c!='\r') {} /* skip comment */
    c=getc(f);
  }
  /* ungetc(c,f); */
  /* ^^^ unget isn't necessary for PNM files, and no unget is really convenient
   *     for reading rawbits PNM files.
   */
  return v;
}

/** Loads a single PBM|PGM|PPM file (no alpha)
 * @return 0 iff premature EOF
 */
static /*hdrawable*/gint32 load_pnm_low(FILE *f, gbool only_mask) {
  gbool rawbitsp;
  guchar *topleft, *p, *pend, c;
  gsize_t rcol;
  GimpDrawable *gd;
  GimpPixelRgn r;
  gint32 hlayer=0;
  gint32 s, width, height;
  unsigned u, mul;
  assert(f!=NULLP);

  if (getc(f)!='P') goto unk;
  switch ((u=getc(f))) {
   case '1': /* PBM.text */
   case '4': /* PBM.rawbits */
    if ((width=getint(f))<1) { pbmerr: g_message("PBM: syntax error\n"); goto q; }
    if ((height=getint(f))<1) goto pbmerr;
    hlayer=gimp_layer_new(0, "PNM Background", width, height, GIMP_GRAY_IMAGE, 100.0, GIMP_NORMAL_MODE);
    gd=gimp_drawable_get(hlayer);
    gimp_pixel_rgn_init(&r, gd, 0, 0, width, height, TRUE, FALSE);
    topleft=gimpts_pixel_rgn_get_topleft(&r);
    pend=(p=topleft)+(gsize_t)height*width;
    if (u=='4') { /* rawbits, read the easy(?) way */
      gsize_t rlen;
      guchar *src=pend-(gsize_t)height*(rlen=(width+7)>>3);
      gsize_t rlen7=width&7;
      if (1!=fread(src, rlen*height, 1, f)) return 0;
      rlen=width>>3;
      while (p!=pend) { /* extract in place */
        /* fprintf(stderr, "%d\n",  pend-p); */
        rcol=rlen; while (rcol--!=0) {
          *p++=(src[0]&128)==0?255:0;
          *p++=(src[0]&64)==0?255:0;
          *p++=(src[0]&32)==0?255:0;
          *p++=(src[0]&16)==0?255:0;
          *p++=(src[0]&8)==0?255:0;
          *p++=(src[0]&4)==0?255:0;
          *p++=(src[0]&2)==0?255:0;
          *p++=(src[0]&1)==0?255:0;
          src++;
        }
        /* fprintf(stderr, "%d X\n",  pend-p); */
        if (rlen7!=0) {
          rcol=rlen7; c=*src++;
          while (rcol--!=0) { *p++=(c&128)==0?255:0; c<<=1; }
        }
      }
      assert(src==pend);
    } else { /* text, the hard way */
      /* fprintf(stderr, "pp=%u\n", pend-p); */
      while (p!=pend) {
        while (1) {
          if ((u=getc(f))+1==0) return 0;
          if (u=='1') { *p++=0; break; }
          if (u=='0') { *p++=255; break; }
        }
      }
      while (ISWSPACE(u,=getc(f))) {}
      if (u+1!=0) ungetc(u,f);
    }
    gimp_drawable_detach(gd);
    break;
   case '3': /* PPM.text */
   case '6': /* PPM.rawbits */
    if (only_mask) { g_message("PNM: PBM|PGM alpha expected\n"); goto q; }
   case '2': /* PGM.text */
   case '5': /* PGM.rawbits */
    if ((width=getint(f))<1) { pgmerr: g_message("PNM: PGM|PPM syntax error\n"); goto q; }
    if ((height=getint(f))<1) goto pgmerr;
    if ((s=getint(f))<1) goto pgmerr;
    hlayer=gimp_layer_new(0, "PNM Background", width, height, 
      (u=='6' || u=='3' ? GIMP_RGB_IMAGE : GIMP_GRAY_IMAGE),
      100.0, GIMP_NORMAL_MODE);
    gd=gimp_drawable_get(hlayer);
    gimp_pixel_rgn_init(&r, gd, 0, 0, width, height, TRUE, FALSE);
    topleft=gimpts_pixel_rgn_get_topleft(&r);
    rcol=(gsize_t)gd->bpp*width*height;
    if ((rawbitsp=u>'4') && s==255) { /* shortcut, read binary bytes */
      if (1!=fread(topleft, rcol, 1, f)) return 0;
    } else {
      p=topleft;
      mul=(s==1) ? 255 : (s==3) ? 85 : (s==15) ? 17 : (s==255) ? 1 : 0;
      if (mul!=0) {
        if (rawbitsp) {
          while (rcol--!=0) {
            if ((u=getc(f))+1==0) return 0;
            *p++=u*mul;
          }
        } else {
          /* assert(0); */
          while (rcol--!=0) {
            if ((gint32)(u=getint(f))<0 || (gint32)u>s) goto pgmerr;
            *p++=u*mul;
          }
        }
      } else { /* do complicated arithmetic (integer division :-)) here */
        if (rawbitsp) {
          while (rcol--!=0) {
            if ((u=getc(f))+1==0) return 0;
            *p++=(u*255+(s>>1))/s; /* always <=255 */
          }
        } else {
          while (rcol--!=0) {
            if ((gint32)(u=getint(f))<0 || (gint32)u>s) goto pgmerr;
            *p++=(u*255+(s>>1))/s; /* always <=255 */
          }
        }
      }
    }
    break;
   /* vvv on EOF */
   default: unk: g_message("PNM: bad image file format\n"); q: gimp_quit();
  }
  return hlayer;
}

/*himage*/gint32 ptspnm_load_image(gchar const*filename0) {
  gint32 hlayer, halpha, himage;
  int s;
  FILE *f;
  char const*filename=filename0;
  GimpDrawable *gd;
#if 0
  filename=*(char**)&filename0; /* 2.95 SUXX pacify gcc const_cast warning */
#endif
  f=(filename!=NULLP)?fopen(filename,"rb"):stdin;
  if (!f) { readerr:
    g_message ("PNM: can't read image from `%s'\n", filename);
    q: gimp_quit();
  }
  if (0==(hlayer=load_pnm_low(f, FALSE))) { g_message("PNM: premature EOF\n"); goto q; }
  gd=gimp_drawable_get(hlayer);
  himage=gimp_image_new(gd->width, gd->height,
    (gd->type|1)==GIMP_INDEXEDA_IMAGE ? GIMP_INDEXED :
    (gd->type|1)==GIMP_RGBA_IMAGE ? GIMP_RGB : GIMP_GRAY);
  gimp_image_add_layer(himage, hlayer, 0);
  if ((s=fgetc(f))=='P') { /* transparency mask */
    ungetc(s,f);
    if (0==(halpha=load_pnm_low(f, TRUE))) { g_message("PNM: premature EOF\n"); goto q; }
    gimpts_drawable_noassalpha(hlayer, halpha);
    gimp_drawable_free(halpha);
    s=fgetc(f);
  }

  if (ferror(f)) { fclose(f); goto readerr; }
  if (-1!=s) { g_message("PNM: EOF expected\n"); goto q; }
  if (filename!=NULLP) fclose(f);
  return himage;
}

/* __END__ */
