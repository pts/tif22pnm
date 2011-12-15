/* minigimp -- extremely basic image manipulation routines
 * by pts@fazekas.hu at Sun Sep 29 22:13:46 CEST 2002
 * Tue Oct  1 00:59:37 CEST 2002
 * gimpts_drawable_convert() at Thu Oct  3 02:19:52 CEST 2002
 * multi-drawable gimpts_drawable_convert() at Sat Oct  5 13:58:54 CEST 2002
 * WORKS at Sat Oct  5 16:24:52 CEST 2002
 * *_get_rect_bpc at Sun Oct  6 15:32:44 CEST 2002
 *
 * bad: dhead->next. good: dhead->u.drawable.dnext
 * bad: dhead->prev. good: dhead->u.drawable.dprev
 */

#if defined(__GNUC__) && defined(__cplusplus)
#pragma implementation
#endif

#include "minigimp.h"
#include <string.h> /* strlen() */
#include <stdio.h> /* magic... */
#include <stdlib.h> /* exit() */

/* --- Generic resource handling (images and drawables) */

/** GimpImageType: */
#define T_FREE 0
#define T_HEAD 1
#define T_NONE 2
#define T_DRAWABLE_HEAD 3
#define T_PARASITE_HEAD 4
#define T_NORMAL_MIN 5
typedef struct MyRes {
  gint32 handle;
  /* 0 for free, 1 for head, 2..9 for images, 10..99 for layers */
  GimpImageType type;
  struct MyRes *prev, *next;
  union {
    struct {
      gdimen_t wd, ht;
      GimpParasite *parasite_head;
      /** May be NULL, memory-managed by MyRes */
      gchar *filename;
      gdouble xres, yres;
      GimpUnit unit;
      /** -2 for GIMP_RGB, -1 for GIMP_GRAY, 0..256 for indexed */
      gint ncolors;
      /** NULL for non-indexed, malloc(<=3*256) for indexed */
      guchar *cmap;
      struct MyRes *dhead;
      slen_t indexedc;
      /** Is the last color in cmap transparent? */
      gbool lasttransp;
    } image;
    struct {
      /** u.drawable.gd.type==type */
      GimpDrawable gd;
      struct MyRes *image;
      guchar *data;
      /** 0.0 .. 100.0 */
      gdouble opacity;
      /** Linked list of the drawables belonging to `image' */
      struct MyRes *dprev, *dnext;
    } drawable;
    struct {
      /** List of persistent parasites */
      GimpParasite *pers;
    } head;
  } u;
} MyRes;
/** vvv 6..8 */
#define IS_IMAGE(resp)    ((resp)!=NULLP && (resp)->type-6U<=2U)
#define IS_LAYER(resp)    ((resp)!=NULLP && (resp)->type-11U<=88U)
#define IS_CHANNEL(resp)  ((resp)!=NULLP && (resp)->type==GIMP_CHANNEL)
/** A layer or a channel */
#define IS_DRAWABLE(resp) ((resp)!=NULLP && (resp)->type-10U<=89U)

#define MYRES_CACHE_SIZE 16
/** Thread-unsafe! */
static MyRes myres_cache[MYRES_CACHE_SIZE];

static void resInit(void) {
  unsigned i=MYRES_CACHE_SIZE;
  while (i--!=0) myres_cache[i].type=T_FREE;
  myres_cache[0].handle=T_HEAD;
  myres_cache[0].type=0; /* head */
  myres_cache[0].next=myres_cache[0].prev=myres_cache+0;
  myres_cache[0].u.head.pers=gimp_parasite_new("/hd",0/*GIMP_PARASITE_PERSISTENT*/,0,"");
}

/** @return NULL or... */
static MyRes* resFind(gint32 handle) {
  static int dummy=0; /* initializer element must be a constant */
  MyRes *p;
  if (dummy==0) { resInit(); dummy=1; } /* call once in a program */
  if (handle<=0) {
    return NULL;
  } else if (handle<MYRES_CACHE_SIZE) {
    return myres_cache[handle].type<T_NORMAL_MIN ? NULL : myres_cache+handle;
  } else { /* do linear search */
    for (p=myres_cache->next; p!=myres_cache; p=p->next)
      if (p->handle==handle) return p;
    return NULL;
  }
}

static MyRes* resNew(void) {
  MyRes *p;
  gint32 handle=1;
  resFind(0); /* call resInit() */
  for (p=myres_cache+1;p!=myres_cache+MYRES_CACHE_SIZE;p++)
    if (p->type==T_FREE) { handle=p-myres_cache; goto found; }
  /* For simplictity, we allocate a handle larger than any previous one */
  for (p=myres_cache->next; p!=myres_cache; p=p->next)
    if (p->type!=T_FREE && handle<=p->handle) handle=p->handle+1;
  p=g_new(MyRes,1);
 found:
  p->handle=handle;
  p->type=T_NONE; /* just a placeholder */
  return p->prev=p->next=p;
}

static void resDelete(MyRes *p) {
  resFind(0); /* call resInit() */
  assert(p!=NULLP);
  assert(p->handle>0); /* must not delete T_HEAD */
  p->prev->next=p->next;  p->next->prev=p->prev;
  if (p->handle<MYRES_CACHE_SIZE) {
    assert(p->handle==p-myres_cache);
    p->type=T_FREE; /* back to the cache */
  } else g_free(p);
}

/* --- Hash46: helper for converting images to indexed */

/** Maximum size. Should be >=2*256 */
#define HASH46_M 1409U
/** Number of _value_ data bytes (they are not hashed) */
#define HASH46_D 1
/** Size of each tuple in the array `t' */
#define HASH46_HD (4+HASH46_D)
/** A tuple is considered free iff its first byte equals HASH46_FREE */
#define HASH46_FREE 255
typedef slen_t hash46_u32_t;

/**
 * M=1409
 * h(K)=K%1409
 * h(i,K)=-i*(1+(K%1408)) (i in 0..1408)
 * K=[a,r,g,b]=(a<<24)+(r<<16)+(g<<8)+b (4 bytes)
 * h(K)=(253*a+722*r+(g<<8)+b)%1409
 * h(i,K)=-i*(1+(896*a+768*r+(g<<8)+b)%1408)
 *
 * -- No dynamic growing or re-hashing. Useful for hashing colormap palettes
 *    with a maximum size of 256.
 * -- Deleting not supported.
 *
 * Implementation: Use 32 bit integers for calculation.
 * Derived from class Hash46 in sam2p 0.39.
 */
typedef struct Hash46 {
  /** Number of occupied elements. */
  /* unsigned size; */
  /** Number of non-free tuples in the hash. */
  unsigned char t[HASH46_M*HASH46_HD];
} Hash46;

static void hash46_init(Hash46 *self) {
  /* self->size=0; */
  memset(self->t, HASH46_FREE, sizeof(self->t));
}

#if 0 /* not needed, left from sam2p:image.cpp */
inline unsigned char* hash46_lookup(unsigned char k[4]) {
  unsigned char *ret=walk(k);
  return ret==NULLP || *ret==HASH46_FREE ? (unsigned char*)NULLP : ret;
}
#endif

/** Can be called only if size!=HASH46_M
 * @return NULL if isFull() and not found; otherwise: pointer to the tuple
 *         found or the place to which the insert can take place.
 */
static unsigned char *hash46_walk(Hash46 *self, unsigned char k[4]) {
  unsigned i=HASH46_M;
  register unsigned char *p;
  hash46_u32_t hk, hik;
  hk=HASH46_HD*(((((hash46_u32_t)1<<24)%HASH46_M)*k[0]+(((hash46_u32_t)1<<16)%HASH46_M)*k[1]+
          (((hash46_u32_t)1<< 8)%HASH46_M)*k[2]+k[3])%HASH46_M);
  hik=HASH46_HD*(1+((((hash46_u32_t)1<<24)%(HASH46_M-1))*k[0]+(((hash46_u32_t)1<<16)%(HASH46_M-1))*k[1]+
             (((hash46_u32_t)1<< 8)%(HASH46_M-1))*k[2]+k[3])%(HASH46_M-1));
  p=self->t+hk;
  do {
    if (*p==HASH46_FREE || (k[0]==p[0] && k[1]==p[1] && k[2]==p[2] && k[3]==p[3])) return p;
    if (hk>=hik) { hk-=hik; p-=hik; }
            else { hk+=HASH46_M*HASH46_HD-hik; p+=HASH46_M*HASH46_HD-hik; }
  } while (--i!=0);
  return (unsigned char*)NULLP;
}

/* --- functions of minigimp.h */

void gimp_pixel_rgn_init (GimpPixelRgn *r, GimpDrawable *drawable,
 guint32 x, guint32 y, guint32 width, guint32 height,
 gbool writeAllowed /* ?? */, gbool alwaysFALSE) {
  MyRes *dra=resFind(drawable->handle);
  (void)writeAllowed;
  (void)alwaysFALSE;
  param_assert(IS_DRAWABLE(dra));
  assert(drawable==&dra->u.drawable.gd);
  /* g_message("pri %u %u %u %u\n", x, y, width, height); */
  if (x>drawable->width) x=width=0;
  else if (x+width>drawable->width) width=drawable->width-x;
  if (y>drawable->height) y=height=0;
  else if (y+height>drawable->height) height=drawable->height-y;
  r->width=(r->minx=x)+width;
  r->height=(r->miny=y)+height;
  r->bpp=drawable->bpp;
  r->bpl=drawable->bpp*drawable->width;
  r->topleft=dra->u.drawable.data;
  r->type=drawable->type;
}
void gimp_pixel_rgn_init_rel (GimpPixelRgn *r, GimpDrawable *drawable,
 guint32 x, guint32 y, guint32 width, guint32 height,
 gbool writeAllowed /* ?? */, gbool alwaysFALSE) {
  MyRes *dra=resFind(drawable->handle);
  (void)writeAllowed;
  (void)alwaysFALSE;
  param_assert(IS_DRAWABLE(dra));
  assert(drawable==&dra->u.drawable.gd);
  /* g_message("pri %u %u %u %u\n", x, y, width, height); */
  if (x>drawable->width) x=width=0;
  else if (x+width>drawable->width) width=drawable->width-x;
  if (y>drawable->height) y=height=0;
  else if (y+height>drawable->height) height=drawable->height-y;
  r->minx=r->miny=0;
  r->width=width;
  r->height=height;
  r->bpp=drawable->bpp;
  r->bpl=drawable->bpp*drawable->width;
  r->topleft=dra->u.drawable.data+r->bpl*y+r->bpp*x;
}
void gimp_pixel_rgn_set_rect(GimpPixelRgn *r, guchar const*src,
 guint32 x, guint32 y, guint32 width, guint32 height) {
  guchar *p;
  gsize_t wskip, bpl;
  /* g_message("SET %u %u %u %u rh=%u\n", x, y, width, height, r->height); */
  /* g_message("set w=%u h=%u\n", width, height); */
  wskip=r->bpp*width;
  if (y<r->miny) { src+=wskip *(r->miny-y); y=r->miny; }
  if (x<r->minx) { src+=r->bpp*(r->minx-x); x=r->minx; }
  if (x>=r->width || y>=r->height) return;
  width=r->bpp*((x+width>r->width)?r->width-x:width);
  /* ^^^ Imp: is sizeof(width) enough? */
  if (y+height>r->height) height=r->height-y;
  p=r->topleft+(bpl=r->bpl)*y+r->bpp*x;
  while (height--!=0) {
    memcpy(p, src, width);
    p+=bpl;
    src+=wskip;
  }
}
void gimp_pixel_rgn_get_rect(GimpPixelRgn *r, guchar *dst,
 guint32 x, guint32 y, guint32 width, guint32 height) {
  /* Sat Oct  5 14:13:51 CEST 2002 */
  guchar *p;
  gsize_t wskip, bpl;
  wskip=r->bpp*width;
  if (y<r->miny) { dst+=wskip *(r->miny-y); y=r->miny; }
  if (x<r->minx) { dst+=r->bpp*(r->minx-x); x=r->minx; }
  if (x>=r->width || y>=r->height) return;
  width=r->bpp*((x+width>r->width)?r->width-x:width);
  /* ^^^ Imp: is sizeof(width) enough? */
  if (y+height>r->height) height=r->height-y;
  p=r->topleft+(bpl=r->bpl)*y+r->bpp*x;
  while (height--!=0) {
    memcpy(dst, p, width);
    p+=bpl;
    dst+=wskip;
  }
}

/** Select the low bits of each sample */
#define BITS_LOW 0
/** Select the high bits of each sample */
#define BITS_HIGH 16
/** Select the low bits of even samples, high bits of odd samples */
#define BITS_MIX 32
void gimp_pixel_rgn_get_rect_bpc(GimpPixelRgn *r, guchar *dst,
 guint32 x, guint32 y, guint32 width, guint32 height, guchar dstbpc,
 gbool yflip) {
  /* Sun Oct  6 15:31:35 CEST 2002 */
  slendiff_t wskip;
  gsize_t bpl, wcopy;
  register guchar c, *p;
  register gsize_t wleft;
  unsigned wcopylow;
  /* register unsigned biteq; */
  param_assert(dstbpc==1 || dstbpc==2 || dstbpc==4 || dstbpc==8);
  wskip=(dstbpc==8) ? r->bpp*width : (r->bpp*dstbpc*width+7)>>3;
  if (y<r->miny) { dst+=wskip *(r->miny-y); y=r->miny; }
  if (x<r->minx) { dst+=r->bpp*(r->minx-x); x=r->minx; }
  if (x>=r->width || y>=r->height || width==0 || height==0) return;
  wcopy=r->bpp*((x+width>r->width)?r->width-x:width);
  assert(wcopy>0);
  if (y+height>r->height) height=r->height-y;
  p=r->topleft+(bpl=r->bpl)*y+r->bpp*x;
  if (yflip) { dst+=(height-1)*wskip; wskip=-wskip; }
  if (dstbpc==8) {
    while (height--!=0) {
      memcpy(dst, p, wcopy);
      p+=bpl;
      dst+=wskip;
    }
    return;
  }
  bpl-=wcopy;
  switch (dstbpc+(r->type==GIMP_INDEXEDA_IMAGE ? BITS_MIX :
   r->type==GIMP_INDEXED_IMAGE ? BITS_LOW : BITS_HIGH) ) {
   case 1+BITS_LOW:
    #if 0
      /* 0 1 2 3 4 5 6 7 8 9 10 11 12 */
      /*         q . . . . . .  .     */
      biteq=wcopy&7;
      while (height--!=0) {
        wleft=wcopy; q=dst; c=0;
        do {
          c=(c<<1)|(*p++&1);
          if ((--wleft&7)==biteq) { *q++=c; c=0; }
        } while (wleft!=0);
        if (biteq!=0) *q++=c<<(8-biteq);
        memcpy(dst, p, wcopy);
        p+=bpl;
        dst+=wskip;
      }
    #else
      wcopylow=wcopy&7; wskip-=wcopy>>=3;
      while (height--!=0) {
        wleft=wcopy;
        while (wleft--!=0) {
          *dst++=((p[0]&1)<<7)|((p[1]&1)<<6)|((p[2]&1)<<5)|((p[3]&1)<<4)
                |((p[4]&1)<<3)|((p[5]&1)<<2)|((p[6]&1)<<1)| (p[7]&1);
          p+=8;
        }
        if (0!=wcopylow) {
          wleft=wcopylow; c=0;
          while (wleft--!=0) c=(c<<1)|(*p++&1);
          *dst=c<<1*(8-wcopylow);
        }
        p+=bpl; dst+=wskip;
      }
    #endif
    break;
   case 2+BITS_LOW:
    wcopylow=wcopy&3; wskip-=wcopy>>=2;
    while (height--!=0) {
      wleft=wcopy;
      while (wleft--!=0) {
        *dst++=((p[0]&3)<<6)|((p[1]&3)<<4)|((p[2]&3)<<2)|(p[3]&3);
        p+=4;
      }
      if (0!=wcopylow) {
        wleft=wcopylow; c=0;
        while (wleft--!=0) c=(c<<2)|(*p++&3);
        *dst=c<<2*(4-wcopylow);
      }
      p+=bpl; dst+=wskip;
    }
    break;
   case 4+BITS_LOW:
    wcopylow=wcopy&1; wskip-=wcopy>>=1;
    while (height--!=0) {
      wleft=wcopy;
      while (wleft--!=0) {
        *dst++=((p[1]&15)<<4)|(p[2]&15);
        p+=2;
      }
      if (0!=wcopylow) *dst=(*p++&15)<<4;
      p+=bpl; dst+=wskip;
    }
    break;
   case 1+BITS_HIGH:
    wcopylow=wcopy&7; wskip-=wcopy>>=3;
    while (height--!=0) {
      wleft=wcopy;
      while (wleft--!=0) {
        *dst++= (p[0]&128)    |((p[1]&128)>>1)|((p[2]&128)>>2)|((p[3]&128)>>3)
              |((p[4]&128)>>4)|((p[5]&128)>>5)|((p[6]&128)>>6)|((p[7]&128)>>7);
        p+=8;
      }
      if (0!=wcopylow) {
        wleft=wcopylow; c=0;
        while (wleft--!=0) c=(c<<1)|((*p++&128)>>7);
        *dst=c<<1*(8-wcopylow);
      }
      p+=bpl; dst+=wskip;
    }
    break;
   case 2+BITS_HIGH:
    wcopylow=wcopy&3; wskip-=wcopy>>=2;
    while (height--!=0) {
      wleft=wcopy;
      while (wleft--!=0) {
        *dst++=(p[0]&192)|((p[1]&192)>>2)|((p[2]&192)>>4)|((p[3]&192)>>6);
        p+=4;
      }
      if (0!=wcopylow) {
        wleft=wcopylow; c=0;
        while (wleft--!=0) c=(c<<1)|((*p++&192)>>6);
        *dst=c<<2*(4-wcopylow);
      }
      p+=bpl; dst+=wskip;
    }
    break;
   case 4+BITS_HIGH:
    wcopylow=wcopy&1; wskip-=wcopy>>=1;
    while (height--!=0) {
      wleft=wcopy;
      while (wleft--!=0) {
        *dst++=(p[0]&240)|((p[1]&240)>>4);
        p+=2;
      }
      if (0!=wcopylow) *dst=(*p++&240);
      p+=bpl; dst+=wskip;
    }
    break;
   case 1+BITS_MIX:
    wcopylow=wcopy&7; wskip-=wcopy>>=3;
    while (height--!=0) {
      wleft=wcopy;
      while (wleft--!=0) {
        *dst++=((p[0]&  1)<<7)|((p[1]&128)>>1)|((p[2]&  1)<<5)|((p[3]&128)>>3)
              |((p[4]&  1)<<3)|((p[5]&128)>>5)|((p[6]&  1)<<1)|((p[7]&128)>>7);
        p+=8;
      }
      if (0!=wcopylow) {
        c=0;
        for (wleft=0; wleft<wcopylow; wleft++)
          c|=((wleft&1)==0) ? (*p++&1)<<(7-wleft) : (*p++&128)>>wleft;
        *dst=c<<1*(8-wcopylow);
      }
      p+=bpl; dst+=wskip;
    }
    break;
   case 2+BITS_MIX:
    wcopylow=wcopy&3; wskip-=wcopy>>=2;
    while (height--!=0) {
      wleft=wcopy;
      while (wleft--!=0) {
        *dst++=((p[0]&3)<<6)|((p[1]&192)>>2)|((p[2]&3)<<2)|((p[3]&192)>>6);
        p+=4;
      }
      if (0!=wcopylow) {
        *dst=(*p++&3)<<6;
        if (wcopylow>=2) {
          *dst|=(*p++&192)>>2;
          if (wcopylow>=3) *dst|=(*p++&3)<<2;
        }
      }
      p+=bpl; dst+=wskip;
    }
    break;
   case 4+BITS_MIX:
    wcopylow=wcopy&1; wskip-=wcopy>>=1;
    while (height--!=0) {
      wleft=wcopy;
      while (wleft--!=0) {
        *dst++=((p[0]&15)<<4)|((p[1]&240)>>4);
        p+=2;
      }
      if (0!=wcopylow) *dst=(p[0]&15)<<4;
      p+=bpl; dst+=wskip;
    }
    break;
   default: assert(0);
  } /* SWITCH */
}

void gimp_image_add_layer(gint32 image, gint32 hlayer, unsigned dummy){
  MyRes *img=resFind(image);
  MyRes *dra=resFind(hlayer);
  (void)dummy;
  assert(IS_IMAGE(img));
  assert(IS_LAYER(dra));
  assert(dra->type==dra->u.drawable.gd.type);
  if (dra->u.drawable.image!=NULLP) gimpts_drawable_unlink(dra->handle);
  dra->u.drawable.image=img;
  dra->u.drawable.dprev=img->u.image.dhead->u.drawable.dprev;
  dra->u.drawable.dnext=img->u.image.dhead;
  img->u.image.dhead->u.drawable.dprev->u.drawable.dnext=dra;
  img->u.image.dhead->u.drawable.dprev=dra;
  if ((dra->type|1)==GIMP_INDEXEDA_IMAGE) img->u.image.indexedc++;
}
void gimp_image_add_channel(gint32 image, gint32 hchannel, unsigned dummy){
  MyRes *img=resFind(image);
  MyRes *dra=resFind(hchannel);
  (void)dummy;
  assert(IS_IMAGE(img));
  assert(IS_CHANNEL(dra));
  assert(dra->type==dra->u.drawable.gd.type);
  if (dra->u.drawable.image!=NULLP) gimpts_drawable_unlink(dra->handle);
  dra->u.drawable.image=img;
  dra->u.drawable.dprev=img->u.image.dhead->u.drawable.dprev;
  dra->u.drawable.dnext=img->u.image.dhead;
  img->u.image.dhead->u.drawable.dprev->u.drawable.dnext=dra;
  img->u.image.dhead->u.drawable.dprev=dra;
}
void gimpts_drawable_unlink(gint32 hdrawable) {
  MyRes *dra=resFind(hdrawable);
  MyRes *img;
  assert(IS_DRAWABLE(dra));
  if ((img=dra->u.drawable.image)!=NULLP) {
    if ((dra->type|1)==GIMP_INDEXEDA_IMAGE && (0==--img->u.image.indexedc)) {
      g_free(img->u.image.cmap);
      img->u.image.cmap=(guchar*)NULLP;
    }
  }
  dra->u.drawable.dnext->u.drawable.dprev=dra->u.drawable.dprev;
  dra->u.drawable.dprev->u.drawable.dnext=dra->u.drawable.dnext;
  dra->u.drawable.image=(MyRes*)NULLP;
}

void gimp_drawable_flush (GimpDrawable* drawable){
  /* A simple no-op here */
  (void)drawable;
}
void gimp_drawable_detach(GimpDrawable* drawable){
  /* A simple no-op again :-) */
  (void)drawable;
}

gint32 gimp_layer_new (gint32 image, char const*name /*_("Background")*/,
 guint32 cols, guint32 rows, gint layer_type, gdouble opacity /*100.0*/, unsigned mode) {
  MyRes *lay;
  lay=resNew();
  (void)image;
  param_assert(layer_type==GIMP_RGB_IMAGE || layer_type==GIMP_GRAY_IMAGE || layer_type==GIMP_INDEXED_IMAGE
            || layer_type==GIMP_RGBA_IMAGE || layer_type==GIMP_GRAYA_IMAGE || layer_type==GIMP_INDEXEDA_IMAGE);
  param_assert(cols>=1);
  param_assert(rows>=1);
  param_assert(0.0<=opacity && opacity<=100.0);
  param_assert(mode==GIMP_NORMAL_MODE);
  lay->type=layer_type;
  lay->u.drawable.gd.name=g_strdup(name);
  lay->u.drawable.gd.handle=lay->handle;
  lay->u.drawable.gd.width =cols;
  lay->u.drawable.gd.height=rows;
  lay->u.drawable.opacity=opacity;
  lay->u.drawable.gd.bpp=
    layer_type==GIMP_RGB_IMAGE  ? 3 : layer_type==GIMP_GRAY_IMAGE  ? 1 : layer_type==GIMP_INDEXED_IMAGE  ? 1 :
    layer_type==GIMP_RGBA_IMAGE ? 4 : layer_type==GIMP_GRAYA_IMAGE ? 2 : layer_type==GIMP_INDEXEDA_IMAGE ? 2 : 0;
  lay->u.drawable.gd.type=layer_type;
  lay->u.drawable.gd.bpc=8;
  lay->u.drawable.gd.mode=mode;
  lay->u.drawable.dprev=lay->u.drawable.dnext=lay;
  lay->u.drawable.image=(MyRes*)NULLP;
  lay->u.drawable.data=g_new(guchar, (gsize_t)lay->u.drawable.gd.bpp*cols*rows);
  return lay->handle;
}
gint32 gimp_channel_new(gint32 image, char const*name /*_("TIFF Channel")*/,
  guint32 cols, guint32 rows, gdouble opacity, GimpRGB *color){
  MyRes *cha;
  (void)color;
  (void)image;
  cha=resNew();
  param_assert(cols>=1);
  param_assert(rows>=1);
  param_assert(0.0<=opacity && opacity<=100.0);
  cha->u.drawable.gd.name=g_strdup(name);
  cha->u.drawable.gd.handle=cha->handle;
  cha->u.drawable.gd.width =cols;
  cha->u.drawable.gd.height=rows;
  cha->u.drawable.opacity=opacity;
  cha->u.drawable.gd.bpp=1;
  cha->u.drawable.gd.type=cha->type=GIMP_CHANNEL;
  cha->u.drawable.gd.bpc=8;
  cha->u.drawable.gd.mode=GIMP_NORMAL_MODE;
  cha->u.drawable.dprev=cha->u.drawable.dnext=cha;
  cha->u.drawable.image=(MyRes*)NULLP;
  cha->u.drawable.data=g_new(guchar, cols*rows);
  return cha->handle;
}

#if 0 /* implemented in minigimp.h */
guint32 gimp_tile_height(void){ return (guint32)(-1U>>1); }
#endif

gdouble gimp_unit_get_factor (GimpUnit unit){
  return unit==GIMP_UNIT_INCH  ? 1.0
       : unit==GIMP_UNIT_MM    ? 25.4
       : unit==GIMP_UNIT_PIXEL ? 72.0
       : 0.0;
}
#if PTS_USE_GIMPRGB
void gimp_rgb_set(GimpRGB *color, gfloat r, gfloat g, gfloat b){
  assert(0);
}
#endif
char *gimp_get_default_comment(void){
  return "MiniGIMP by pts@fazekas.hu";
}
void gimp_quit (void){
  exit(2);
}
void gimp_parasite_free(GimpParasite*parasite){
  if (parasite!=NULLP) {
    assert(parasite->refc>0);
    if (0==--parasite->refc) { 
      /* Imp: save persistents to T_HEAD */
      #if 0 /* by now next and prev are assumed to be invalid */
        parasite->next->prev=parasite->prev;
        parasite->prev->next=parasite->next;
      #endif
      g_free(parasite); /* also frees the two strings */
    }
  }
}
GimpParasite *gimp_parasite_new(char const*name, unsigned opts, gsize_t size, void *data){
  gsize_t namesize=strlen(name)+1;
  GimpParasite *parasite=(GimpParasite*)g_malloc(sizeof(GimpParasite)+namesize+size);
  (void)opts;
  parasite->name=gimp_parasite_name(parasite);
  memcpy(parasite->name, name, namesize);
  parasite->data=(guchar*)(parasite->name+namesize);
  memcpy(parasite->data, data, size);
  parasite->data_size=size;
  parasite->refc=1;
  return parasite->prev=parasite->next=parasite; /* no neighbours */
}

gint32 gimp_image_new(gint cols, gint rows, gint image_type){
  MyRes *img, *dhead;
  GimpParasite *phead;
  param_assert(image_type==GIMP_RGB || image_type==GIMP_GRAY || image_type==GIMP_INDEXED);
  param_assert(cols>=1);
  param_assert(rows>=1);
  img=resNew();
  img->type=image_type;
  img->u.image.wd=cols;
  img->u.image.ht=rows;

  phead=img->u.image.parasite_head=g_new(GimpParasite, 1);
  /* ^^^ old: =gimp_parasite_new("/hd",0/:GIMP_PARASITE_PERSISTENT:/,0,""); */
  phead->next=phead->prev=phead;
  phead->name=0;
  phead->data=0;
  phead->data_size=0;
  phead->refc=0;

  dhead=img->u.image.dhead=g_new(MyRes, 1);
  dhead->prev=dhead->next=dhead;
  dhead->u.drawable.dnext=dhead->u.drawable.dprev=dhead;
  dhead->handle=0;
  dhead->type=T_DRAWABLE_HEAD;
  img->u.image.filename=(gchar*)NULLP;
  img->u.image.xres=1.0;
  img->u.image.yres=1.0;
  img->u.image.unit=GIMP_UNIT_PIXEL;
  img->u.image.ncolors=image_type==GIMP_INDEXED?0:image_type==GIMP_RGB?-2:-1;
  img->u.image.cmap=(guchar*)NULLP;
  img->u.image.indexedc=0;
  img->u.image.lasttransp=FALSE;
  return img->handle;
}

GimpDrawable* gimp_drawable_get(gint32 hdrawable){
  MyRes *dra=resFind(hdrawable);
  assert(IS_DRAWABLE(dra));
  return &dra->u.drawable.gd;
}
GimpImageType gimp_drawable_type(gint32 hdrawable){
  MyRes *dra=resFind(hdrawable);
  assert(IS_DRAWABLE(dra));
  return dra->type;
}

void gimp_image_set_filename(gint32 image, char const* filename){
  MyRes *img=resFind(image);
  param_assert(IS_IMAGE(img));
  if (img->u.image.filename!=NULLP) g_free(img->u.image.filename);
  img->u.image.filename=g_strdup(filename); /* works for NULLP */
}
GimpParasite* gimp_image_parasite_find (gint32 image, char const*parasite_name) {
  /* Imp: also search for parasites of T_HEAD */
  /* Imp: disallow "/hd" */
  MyRes *img=resFind(image);
  GimpParasite *p, *head;
  param_assert(IS_IMAGE(img));
  for (p=(head=img->u.image.parasite_head)->next; p!=head; p=p->next) /* linear search */
    if (0==strcmp(parasite_name, gimp_parasite_name(p))) { p->refc++; return p; }
  /* resFind() has been already called */
  for (p=(head=myres_cache[0].u.head.pers)->next; p!=head; p=p->next) /* linear search */
    if (0==strcmp(parasite_name, gimp_parasite_name(p))) { p->refc++; return p; }
  return (GimpParasite*)NULLP;
}
void gimp_image_parasite_attach(gint32 image, GimpParasite *parasite){
  /* Imp: disallow "/hd" */
  MyRes *img=resFind(image);
  GimpParasite *p, *head;
  char *parasite_name;
  param_assert(IS_IMAGE(img));
  parasite_name=gimp_parasite_name(parasite);
  for (p=(head=img->u.image.parasite_head)->next; p!=head; p=p->next) { /* linear search */
    if (0==strcmp(parasite_name, gimp_parasite_name(p))) { /* remove old */
      p->prev->next=p->next;  p->next->prev=p->prev;
      gimp_parasite_free(p);
      break;
    }
  }
  parasite->refc++;
  parasite->prev=head->prev;
  parasite->next=head;
  head->prev->next=parasite;
  head->prev=parasite;
}
static void gimp_drawable_free0(MyRes *dra) {
  assert(IS_DRAWABLE(dra)); /* not param_assert */
#if 0
  g_free(*(char**)&dra->u.drawable.gd.name); /* pacify gcc warning */
#else
  g_free(dra->u.drawable.gd.name);
#endif
  g_free(dra->u.drawable.data);
  dra->u.drawable.data=(guchar*)NULLP; /* failsafe */
  dra->u.drawable.image=(MyRes*)NULLP; /* failsafe */
  resDelete(dra);
}
void gimp_drawable_free(gint32 hdrawable) {
  MyRes *dra=resFind(hdrawable);
  assert(IS_DRAWABLE(dra));
  dra->u.drawable.dnext->u.drawable.dprev=dra->u.drawable.dprev;
  dra->u.drawable.dprev->u.drawable.dnext=dra->u.drawable.dnext;
  gimp_drawable_free0(dra);
}
void gimp_image_delete (gint32 image){
  MyRes *img=resFind(image), *rp, *dhead;
  GimpParasite *p, *phead;
  param_assert(IS_IMAGE(img));
  g_free(img->u.image.cmap);     /* may be NULLP */
  g_free(img->u.image.filename); /* may be NULLP */
  for (p=(phead=img->u.image.parasite_head)->next; p!=phead; ) { /* iteration */
    p=p->next; gimp_parasite_free(p->prev); /* trick not to use p->next after free() */
  }
  g_free(phead);
  for (rp=(dhead=img->u.image.dhead)->u.drawable.dnext; rp!=dhead; ) { /* iteration */
    rp=rp->u.drawable.dnext;
    gimp_drawable_free0(rp->u.drawable.dprev); /* trick not to use p->...dnext after free() */
  }
  g_free(dhead);
  resDelete(img);
}
void gimp_image_set_resolution (gint32 image, gdouble xres, gdouble yres){
  MyRes *img=resFind(image);
  param_assert(IS_IMAGE(img));
  img->u.image.xres=xres;
  img->u.image.yres=yres;
}
void gimp_image_set_unit (gint32 image, GimpUnit unit){
  MyRes *img=resFind(image);
  param_assert(IS_IMAGE(img));
  param_assert(0<unit && unit<=GIMP_UNIT__MAX);
  img->u.image.unit=unit;
}
void gimp_image_set_cmap(gint32 image, guchar *cmap, guint ncolors){
  MyRes *img=resFind(image);
  param_assert(img->type==GIMP_INDEXED); /* implied: param_assert(IS_IMAGE(img)); */
  param_assert(0<=(int)ncolors && ncolors<=256);
  if (img->u.image.ncolors+0U<ncolors) { /* true for the first call to thisfn */
    g_free(img->u.image.cmap); 
    img->u.image.cmap=g_new(guchar, ncolors*3);
  } /* Dat: waste some memory until image is deleted */
  img->u.image.ncolors=ncolors;
  ncolors*=3; /* guint is wide enough */
  memcpy(img->u.image.cmap, cmap, ncolors);
}
guchar* gimp_image_get_cmap(gint32 image, gint *ncolors){
  MyRes *img=resFind(image);
  param_assert(IS_IMAGE(img));
  if (ncolors!=NULLP) *ncolors=img->u.image.ncolors;
  return img->u.image.cmap;
}
void gimp_image_get_resolution (gint32 image, gdouble *xres, gdouble *yres){
  MyRes *img=resFind(image);
  param_assert(IS_IMAGE(img));
  *xres=img->u.image.xres;
  *yres=img->u.image.yres;
}
GimpUnit gimp_image_get_unit (gint32 image){
  MyRes *img=resFind(image);
  param_assert(IS_IMAGE(img));
  return img->u.image.unit;
}

void gimpts_rgb2indexed(void *img0, guchar *dst, gsize_t srclen, guint *ncolors_ret, guchar cmap_ret[768], guchar *src, unsigned adst, unsigned asrc) {
  /* Thu Oct  3 00:00:20 CEST 2002 */
  /* derived from sam2p: Image::Sampled* Image::Sampled::toIndexed0() */
  MyRes *img=(MyRes*)img0;
  unsigned ncols=0;
  Hash46 h;
  unsigned char k[4], *w, *c, *cend, *srcend=src+srclen;
  k[0]=0;
  hash46_init(&h);
  if (img!=NULLP && img->u.image.cmap!=NULLP) { /* keep cmap we already have */
    cend=(c=img->u.image.cmap)+3*(0U+img->u.image.ncolors);
    while (c!=cend) {
      k[1]=c[0]; k[2]=c[1]; k[3]=c[2]; c+=3;
      w=hash46_walk(&h, k);
      assert_slow(w!=NULL); /* Hash cannot be full since h.M>=256. */
      if (*w==HASH46_FREE) {
        assert(ncols<256);
        w[0]=0;
        *cmap_ret++=w[1]=k[1];
        *cmap_ret++=w[2]=k[2];
        *cmap_ret++=w[3]=k[3];
        w[4]=ncols++;
      }
    } /* WHILE */
  } /* IF */
  
  while (src!=srcend) {
    /* Dat: asrc<=2: GIMP_GRAYA_IMAGE | GIMP_GRAY_IMAGE | GIMP_CHANNEL */
    if (asrc<=2) { k[1]=k[2]=k[3]=src[0]; }
            else { k[1]=src[0]; k[2]=src[1]; k[3]=src[2]; }
    w=hash46_walk(&h, k);
    /* Dat: fast, collision-free walk is guaranteed if asrc<=2 && only_1_drawable */
    assert_slow(w!=NULL); /* Hash cannot be full since h.M>=256. */
    if (*w==HASH46_FREE) {
      if (ncols==256) { *ncolors_ret=0; return; }
      /* ^^^ too many colors; cannot convert image to indexed */
      w[0]=0;
      *cmap_ret++=w[1]=k[1];
      *cmap_ret++=w[2]=k[2];
      *cmap_ret++=w[3]=k[3];
      *dst=w[4]=ncols++;
    } else { /* a color that we have already seen */
      *dst=w[4];
    }
    src+=asrc; dst+=adst;
  }
  *ncolors_ret=ncols;
}

void gimpts_indexed_packpal(guchar *dst, guchar *dstend, guint *ncolors_inout, guchar cmap_inout[768], gbool alpha) {
  /* Thu Oct  3 00:22:37 CEST 2002 */
  /* derived from sam2p: packPal(), but handles transparency differently */
  unsigned ncols=0;
  Hash46 h;
  unsigned char k[4], *w, used[256], old2new[256], *src, *srcend, *cmap_ret, *o, *u;
  k[0]=0;
  hash46_init(&h);
  /* Find which colors are really used in the image */
  memset(used,'\0',256);
  w=dst;
  if (alpha) { while (w!=dstend) used[*w++]=1; }
        else { while (w!=dstend) { used[*w]=1; w+=2; } }
  /* Eliminate unused and duplicate entries from the palette */
  cmap_ret=src=cmap_inout; srcend=src+3*ncolors_inout[0]; u=used; o=old2new;
  while (src!=srcend) {
    if (*u++!=0) { /* color is used */
      k[1]=src[0]; k[2]=src[1]; k[3]=src[2];
      w=hash46_walk(&h, k);
      assert_slow(w!=NULL); /* Hash cannot be full since h.M>=256. */
      if (*w==HASH46_FREE) {
        assert_slow(ncols!=256); /* cannot have that many colors in cmap */
        w[0]=0;
        *cmap_ret++=w[1]=k[1];
        *cmap_ret++=w[2]=k[2];
        *cmap_ret++=w[3]=k[3];
        *o=w[4]=ncols++;
      } else { /* a color that we have already seen */
        *o=w[4];
      }
    }
    o++; src+=3;
  }
  ncolors_inout[0]=ncols;
  /* Update the image to reflect the new palette */
  while (dst!=dstend) { *dst=old2new[*dst]; dst++; }
}

#if 0
static void gimpts_colors_used(MyRes *rp, guchar used[256]) {
  guchar *w, guchar *dstend;
  switch (rp->type) {
   case GIMP_INDEXEDA_IMAGE:
    dstend=(w=rp->u.drawable.data)+(gsize_t)dra->u.drawable.gd.height*dra->u.drawable.gd.width;
    while (w!=dstend) { used[*w]=1; w+=2; }
   case GIMP_INDEXED_IMAGE:
    dstend=(w=rp->u.drawable.data)+(gsize_t)dra->u.drawable.gd.height*dra->u.drawable.gd.width;
    while (w!=dstend) used[*w++]=1;
  }
}
#endif

void gimpts_drawable_packpal(gint32 hdrawable, gint32 himage) {
  MyRes *dra, *img, *rp=0 /*pacify g++-2.91*/, *dhead;
  /* Thu Oct  3 00:22:37 CEST 2002 -- Sat Oct  5 11:31:39 CEST 2002 */
  /* derived from gimpts_indexed_packpal() */
  gbool onep;
  unsigned ncols=0;
  Hash46 h;
  unsigned char *dstend, k[4], *w, used[256], old2new[256], *src, *srcend, *cmap_ret, *o, *u;
  memset(used,'\0',256);
  if (hdrawable==0) {
    dra=(MyRes*)NULLP;
    param_assert(himage!=0);
    img=resFind(himage);
    param_assert(IS_IMAGE(img));
    dhead=img->u.image.dhead;
    if (img->u.image.indexedc<1) return;
    onep=(img->u.image.indexedc==1);
    rp=dhead->u.drawable.dnext; /* BUGFIX at Sun Oct 13 20:36:10 CEST 2002 */
  } else { 
    dra=resFind(hdrawable);
    param_assert(IS_DRAWABLE(dra));
    if ((dra->type|1)!=GIMP_INDEXEDA_IMAGE) return;
    if ((img=dra->u.drawable.image)==NULLP) return;
    dhead=img->u.image.dhead;
    param_assert(himage==0 || img->handle==himage);
    rp=dra;
    onep=TRUE;
  }
  for (; rp!=dhead; rp=rp->u.drawable.dnext) if ((rp->type|1)==GIMP_INDEXEDA_IMAGE) { /* iteration */
    switch (rp->type) { /* gimpts_colors_used(rp, used); */
     case GIMP_INDEXEDA_IMAGE:
      dstend=(w=rp->u.drawable.data)+(gsize_t)rp->u.drawable.gd.height*rp->u.drawable.gd.width;
      while (w!=dstend) { used[*w]=1; w+=2; }
     case GIMP_INDEXED_IMAGE:
      dstend=(w=rp->u.drawable.data)+(gsize_t)rp->u.drawable.gd.height*rp->u.drawable.gd.width;
      while (w!=dstend) used[*w++]=1;
    }
    if (onep) { hdrawable=(dra=rp)->handle; break; }
  }

  assert(img->u.image.ncolors>=0);
  assert(img->u.image.cmap!=NULLP);
  k[0]=0;
  hash46_init(&h);

  /* Eliminate unused and duplicate entries from the palette */
  cmap_ret=src=img->u.image.cmap;
  srcend=src+3*img->u.image.ncolors; u=used; o=old2new;
  while (src!=srcend) {
    if (*u++!=0) { /* color is used */
      k[1]=src[0]; k[2]=src[1]; k[3]=src[2];
      w=hash46_walk(&h, k);
      assert_slow(w!=NULL); /* Hash cannot be full since h.M>=256. */
      if (*w==HASH46_FREE) {
        assert_slow(ncols!=256); /* cannot have that many colors in cmap */
        w[0]=0;
        *cmap_ret++=w[1]=k[1];
        *cmap_ret++=w[2]=k[2];
        *cmap_ret++=w[3]=k[3];
        *o=w[4]=ncols++;
      } else { /* a color that we have already seen */
        *o=w[4];
      }
    }
    o++; src+=3;
  }

  /* Update the image to reflect the new palette */
  if (ncols!=0U+img->u.image.ncolors) {
    assert(ncols<0U+img->u.image.ncolors);
    rp=(dra==NULLP)?dhead->u.drawable.dnext:dra; /* BUGFIX at Sun Oct 13 20:36:19 CEST 2002 */
    do {
      if ((rp->type|1)==GIMP_INDEXEDA_IMAGE) { /* iteration */
        dstend=(w=rp->u.drawable.data)+(gsize_t)rp->u.drawable.gd.height*rp->u.drawable.gd.width;
        while (w!=dstend) { *w=old2new[*w]; w++; }
        if (dra!=NULLP) break;
      }
    } while ((rp=rp->u.drawable.dnext)!=dhead);
  }
}

static void drawable_convert_low(MyRes *dra, GimpImageType dt, gbool force_dup) {
  /* Thu Oct  3 01:00:25 CEST 2002 -- Sat Oct  5 13:56:03 CEST 2002 */
  /* unsigned u; */
  gsize_t wdht;
  guchar *p, *pend, *q, *data, *newdata;
  /* guchar newcmap[768]; */
  guchar *newcmap,
         *cp=(guchar*)NULLP, *cpend=(guchar*)NULLP, *cq=(guchar*)NULLP;
  unsigned char newbpp;
  MyRes *img;
  /* assert(IS_DRAWABLE(dra)); */
  if (dt==dra->type) return;
  wdht=(gsize_t)dra->u.drawable.gd.height*dra->u.drawable.gd.width;
  pend=(data=p=dra->u.drawable.data)+wdht*dra->u.drawable.gd.bpp;
  newbpp=1;
  switch (dt) {
   case GIMP_GRAYA_IMAGE: case GIMP_INDEXEDA_IMAGE: (newbpp=2); break;
   case GIMP_RGB_IMAGE: (newbpp=3); break;
   case GIMP_RGBA_IMAGE: (newbpp=4); break;
   /* case GIMP_CHANNEL: case GIMP_GRAY_IMAGE: case GIMP_INDEXED_IMAGE: break; */
  }
  /* (wdhtbpp!=wdht*dra->u.drawable.gd.bpp) */
  q=newdata=(force_dup || newbpp!=dra->u.drawable.gd.bpp)?g_new(guchar, wdht*newbpp):p;
  if (dra->u.drawable.image!=NULLP) {
    cq=cp=dra->u.drawable.image->u.image.cmap;
    cpend=cp+dra->u.drawable.image->u.image.ncolors*3;
  }
  switch (dra->type) {
   case GIMP_CHANNEL: case GIMP_GRAY_IMAGE:
    switch (dt) {
     case GIMP_CHANNEL: break;
     case GIMP_RGB_IMAGE:
      /* assert(0); */
      /* g_message("%d\n", pend-p); */
      /* memset(newdata, '\200', wdht*3); */
      while (p!=pend) { q[0]=q[1]=q[2]=*p++; q+=3; }
      break;
     case GIMP_RGBA_IMAGE:
      while (p!=pend) { q[0]=q[1]=q[2]=*p++; q+=4; }
      break;
     case GIMP_GRAYA_IMAGE:
      while (p!=pend) { q[0]=*p++; q[1]=255; q+=2; }
      break;
     case GIMP_INDEXEDA_IMAGE:
      assert(newdata!=p);
      memset(newdata, 255, wdht*2); /* fill with opaque */
      /* fall through */
     case GIMP_INDEXED_IMAGE:
      goto to_indexed;
     #if 0 /* old, doesn't respect other existing indexed drawables */
     case GIMP_INDEXEDA_IMAGE:
      while (p!=pend) { q[0]=*p++; q+=2; }
      /* fall through */
     case GIMP_INDEXED_IMAGE:
      q=newcmap=dra->u.drawable.image->u.image.cmap=g_new(guchar, 3*256);
      for (u=0;u<256;u++) { *q++=u; *q++=u; *q++=u; }
      dra->u.drawable.image->u.image.ncolors=256;
      /* ... if we already have indexed... */
      break;
     #endif
     default: assert(0);
    } /* SWITCH(destination type) */
    break;
   case GIMP_GRAYA_IMAGE:
    switch (dt) {
     case GIMP_GRAY_IMAGE: case GIMP_CHANNEL: /* ignore alpha channel */
      while (p!=pend) { *q++=p[0]; p+=2; }
      break;
     case GIMP_RGB_IMAGE: /* ignore alpha channel */
      while (p!=pend) { q[0]=q[1]=q[2]=p[0]; p+=2; q+=3; }
      break;
     case GIMP_RGBA_IMAGE:
      while (p!=pend) { q[0]=q[1]=q[2]=*p++; q[3]=*p++; q+=4; }
      break;
     case GIMP_INDEXEDA_IMAGE: /* ignore alpha channel */
      while (p!=pend) { q[0]=p[0]; p+=2; q+=2; }
      /* fall through */
     case GIMP_INDEXED_IMAGE:
      goto to_indexed;
     #if 0 /* old, doesn't respect other existing indexed drawables */
     case GIMP_INDEXED_IMAGE: /* ignore alpha channel */
      while (p!=pend) { *q++=p[0]; p+=2; }
      /* fall through */
     case GIMP_INDEXEDA_IMAGE:
      q=newcmap=dra->u.drawable.image->u.image.cmap=g_new(guchar, 3*256);
      for (u=0;u<256;u++) { *q++=u; *q++=u; *q++=u; }
      dra->u.drawable.image->u.image.ncolors=256;
      break;
     default: assert(0);
     #endif
    } /* SWITCH(destination type) */
    break;
   case GIMP_RGB_IMAGE:
    switch (dt) {
     case GIMP_GRAY_IMAGE: /* extract red component only */
      while (p!=pend) { *q++=p[0]; p+=3; }
      break;
     case GIMP_CHANNEL: /* extract average value of R,G,B */
      /* gray = 0.299*red + 0.587*green + 0.114*blue (NTSC video std)
       *      = (306.176*red + 601.088*green + 116.736*blue) >> 10
       *      = (19595.264*red + 38469.632*green + 7471.104*blue) >> 16
       *      = (19595*red + 38470*green + 7471*blue) >> 16; (19595+38470+7471)==(1<<16)
       */
      while (p!=pend) { *q++=((hash46_u32_t)19595*p[0] + (hash46_u32_t)38470*p[1] + (hash46_u32_t)7471*p[2]) >> 16; p+=3; }
      break;
     case GIMP_RGBA_IMAGE: /* add full opaque */
      while (p!=pend) { *q++=*p++; *q++=*p++; *q++=*p++; *q++=255; }
      break;
     case GIMP_GRAYA_IMAGE: /* extract red component only */
      while (p!=pend) { q[0]=*p; p+=3; q+=2; }
      break;
     case GIMP_INDEXEDA_IMAGE:
      assert(newdata!=p);
      memset(newdata, 255, wdht*2); /* fill with opaque */
      /* fall through */
     case GIMP_INDEXED_IMAGE: to_indexed:
      newcmap=(img=dra->u.drawable.image)->u.image.cmap=g_new(guchar, 3*256);
      gimpts_rgb2indexed(img, newdata, wdht*dra->u.drawable.gd.bpp,
        (guint*)&img->u.image.ncolors, newcmap, data, newbpp, dra->u.drawable.gd.bpp);
      if (img->u.image.cmap!=NULLP) g_free(img->u.image.cmap);
      img->u.image.cmap=newcmap;
      break;
     default: assert(0);
    } /* SWITCH(destination type) */
    break;
   case GIMP_RGBA_IMAGE:
    switch (dt) {
     case GIMP_GRAY_IMAGE: /* extract red component only, ignore alpha channel */
      while (p!=pend) { *q++=p[0]; p+=4; }
      break;
     case GIMP_CHANNEL: /* extract average value of R,G,B, ignore alpha channel  */
      while (p!=pend) { *q++=((hash46_u32_t)19595*p[0] + (hash46_u32_t)38470*p[1] + (hash46_u32_t)7471*p[2]) >> 16; p+=4; }
      break;
     case GIMP_RGB_IMAGE: /* add full opaque, ignore alpha channel  */
      while (p!=pend) { *q++=p[0]; *q++=p[1]; *q++=p[2]; p+=4; }
      break;
     case GIMP_GRAYA_IMAGE: /* extract red component only */
      while (p!=pend) { q[0]=p[0]; q[1]=p[3]; p+=4; q+=2; }
      break;
     case GIMP_INDEXEDA_IMAGE:
      while (p!=pend) { q[1]=p[3]; p+=4; q+=2; }
      /* fall through */
     case GIMP_INDEXED_IMAGE: /* ignore alpha channel */
      goto to_indexed;
     default: assert(0);
    } /* SWITCH(destination type) */
    break;
   case GIMP_INDEXED_IMAGE:
    switch (dt) {
     case GIMP_CHANNEL: /* extract average value of R,G,B */
      while (cp!=cpend) { *cq++=((hash46_u32_t)19595*cp[0] + (hash46_u32_t)38470*cp[1] + (hash46_u32_t)7471*cp[2]) >> 16; cp+=3; }
      cp=dra->u.drawable.image->u.image.cmap;
      while (p!=pend) *q++=cp[*p++];
      break;
     case GIMP_GRAY_IMAGE: /* extract red component only */
      while (cp!=cpend) { *cq++=cp[0]; cp+=3; }
      cp=dra->u.drawable.image->u.image.cmap;
      while (p!=pend) *q++=cp[*p++];
      break;
     case GIMP_RGB_IMAGE:
      while (p!=pend) { cq=cp+3*(unsigned)*p++; *q++=*cq++; *q++=*cq++; *q++=*cq; }
      /* while (p!=pend) { cq="x"; p++; *q++=*cq++; *q++=*cq++; *q++=*cq; } */
      break;
     case GIMP_RGBA_IMAGE:
      while (p!=pend) { cq=cp+3*(unsigned)*p++; *q++=*cq++; *q++=*cq++; *q++=*cq; *q++=255; }
      break;
     case GIMP_GRAYA_IMAGE: /* extract red component only */
      while (cp!=cpend) { *cq++=cp[0]; cp+=3; }
      cp=dra->u.drawable.image->u.image.cmap;
      while (p!=pend) { *q++=cp[*p++]; *q++=255; }
      break;
     case GIMP_INDEXEDA_IMAGE:
      while (p!=pend) { q[0]=*p++; q[1]=255; q+=2; }
      break;
     default: assert(0);
    } /* SWITCH(destination type) */
    break;
   case GIMP_INDEXEDA_IMAGE:
    switch (dt) {
     case GIMP_CHANNEL: /* extract average value of R,G,B, ignore alpha channel */
      while (cp!=cpend) { *cq++=((hash46_u32_t)19595*cp[0] + (hash46_u32_t)38470*cp[1] + (hash46_u32_t)7471*cp[2]) >> 16; cp+=3; }
      cp=dra->u.drawable.image->u.image.cmap;
      while (p!=pend) { *q++=cp[*p]; p+=2; }
      break;
     case GIMP_GRAY_IMAGE: /* extract red component only, ignore alpha channel */
      while (cp!=cpend) { *cq++=cp[0]; cp+=3; }
      cp=dra->u.drawable.image->u.image.cmap;
      while (p!=pend) { *q++=cp[*p]; p+=2; }
      break;
     case GIMP_RGB_IMAGE: /* ignore alpha channel */
      while (p!=pend) { cq=cp+3*(unsigned)*p; *q++=*cq++; *q++=*cq++; *q++=*cq; p+=2; }
      break;
     case GIMP_RGBA_IMAGE:
      while (p!=pend) { cq=cp+3*(unsigned)*p++; *q++=*cq++; *q++=*cq++; *q++=*cq; *q++=*p++; }
      break;
     case GIMP_GRAYA_IMAGE: /* extract red component only */
      while (cp!=cpend) { *cq++=cp[0]; cp+=3; }
      cp=dra->u.drawable.image->u.image.cmap;
      while (p!=pend) { *q++=cp[*p++]; *q++=*p++; }
      break;
     case GIMP_INDEXED_IMAGE: /* ignore alpha channel */
      while (p!=pend) { *q++=p[0]; p+=2; }
      break;
     default: assert(0);
    } /* SWITCH(destination type) */
    break;
   default: assert(0);
  } /* SWITCH(source type) */
  dra->u.drawable.gd.bpp=newbpp;
  dra->type=dra->u.drawable.gd.type=dt;
  if (newdata!=dra->u.drawable.data) {
    if (!force_dup) g_free(dra->u.drawable.data);
    dra->u.drawable.data=newdata;
  }
}

void gimpts_drawable_convert(gint32 hdrawable, GimpImageType dt) {
  MyRes *dra=resFind(hdrawable);
  assert(IS_DRAWABLE(dra));
  drawable_convert_low(dra, dt, FALSE);
}

/*hdrawable*/gint32 gimpts_drawable_dup(gint32 hdrawable, GimpImageType dt) {
  /* Sun Oct  6 22:19:19 CEST 2002 */
  MyRes *draold=resFind(hdrawable), *dra;
  assert(IS_DRAWABLE(draold));
  dra=resNew();
  dra->type=draold->type;
  dra->u.drawable=draold->u.drawable; /* Copy constructors are to be called. */
  dra->u.drawable.gd.name=g_strdup(dra->u.drawable.gd.name);
  dra->u.drawable.gd.handle=dra->handle;
  /* vvv Prepend dra before draold */
  dra->u.drawable.dnext=draold;
  draold->u.drawable.dprev->u.drawable.dnext=dra;
  draold->u.drawable.dprev=dra;

  drawable_convert_low(dra, dt, TRUE);
  /* ^^^ Make dra->u.drawable.data and draold->u.drawable.data independent */
  return dra->handle;
}

void gimpts_drawable_noassalpha(gint32 hdrawable, gint32 halpha) {
  MyRes *dra=resFind(hdrawable), *draalpha=resFind(halpha);
  guchar *p, *pend, *src, bpp;
  assert(IS_DRAWABLE(dra));
  assert(IS_DRAWABLE(draalpha));
  param_assert(!IS_DRAWABLE_ALPHA(dra->type));
  param_assert(draalpha->u.drawable.gd.bpp==1);
  param_assert(draalpha->u.drawable.gd.width ==dra->u.drawable.gd.width);
  param_assert(draalpha->u.drawable.gd.height==dra->u.drawable.gd.height);
  if (dra->type==GIMP_CHANNEL) dra->type=GIMP_GRAY_IMAGE;
  drawable_convert_low(dra, dra->type|1, FALSE);
  src=draalpha->u.drawable.data;
  pend=(p=dra->u.drawable.data)+(gsize_t)(bpp=dra->u.drawable.gd.bpp)*
    dra->u.drawable.gd.width*dra->u.drawable.gd.height;
  p+=bpp-1; pend+=bpp-1;
  while (p!=pend) { *p=*src++; p+=bpp; }
}

void gimpts_drawable_alpha_assoc(gint32 hdrawable, guchar const bg[3]) {
  unsigned a, b, l;
  MyRes *dra=resFind(hdrawable);
  guchar *p, *pend;
  assert(IS_DRAWABLE(dra));
  /* Dat: we use >>8 instead of /255 which is 
   *      6.65 times faster than `return x/255;' on a Celeron 333Mhz
   */
  switch (dra->type) {
   case GIMP_RGBA_IMAGE:
    pend=(p=dra->u.drawable.data)+(gsize_t)4*dra->u.drawable.gd.width*dra->u.drawable.gd.height;
    if (bg[0]==0 && bg[1]==0 && bg[2]==0) { /* shortcut */
      while (p!=pend) {
        l=p[3];
        a=(b=p[0]*l)>>8; p[0]=a+((1+a+(b&255))>>8); /* p[0]=p[0]*p[3]/255; */
        a=(b=p[1]*l)>>8; p[1]=a+((1+a+(b&255))>>8); /* p[1]=p[1]*p[3]/255; */
        a=(b=p[2]*l)>>8; p[2]=a+((1+a+(b&255))>>8); /* p[2]=p[2]*p[3]/255; */
        p+=4;
      }
    } else {
      while (p!=pend) {
        l=p[3];
        /* p[0] := p[0]*p[3]/255 + bg[0]*(255-p[3])/255
         * p[0] := p[0]*l/255 + bg[0] - bg[0]*l/255
         * p[0] := (p[0]-bg[0])*l/255 + bg[0], but this operates with signed
         */
        a=(b=p [0]*l)>>8; p[0]=       a+((1+a+(b&255))>>8); /* p[0]=p[0]*p[3]/255; */
        a=(b=p [1]*l)>>8; p[1]=       a+((1+a+(b&255))>>8); /* p[1]=p[1]*p[3]/255; */
        a=(b=p [2]*l)>>8; p[2]=       a+((1+a+(b&255))>>8); /* p[2]=p[2]*p[3]/255; */
        a=(b=bg[0]*l)>>8; p[0]+=bg[0]-a-((1+a+(b&255))>>8);
        a=(b=bg[1]*l)>>8; p[1]+=bg[1]-a-((1+a+(b&255))>>8);
        a=(b=bg[2]*l)>>8; p[2]+=bg[1]-a-((1+a+(b&255))>>8);
        p+=4;
      }
    }
    break;
   case GIMP_GRAYA_IMAGE:
    pend=(p=dra->u.drawable.data)+(gsize_t)2*dra->u.drawable.gd.width*dra->u.drawable.gd.height;
    if (bg[0]==0 && bg[1]==0 && bg[2]==0) { /* shortcut */
      while (p!=pend) {
        a=(b=p[0]*p[1])>>8; p[0]=a+((1+a+(b&255))>>8); /* p[0]=p[0]*p[1]/255; */
        p+=2;
      }
    } else {
      while (p!=pend) {
        a=(b=p [0]*p[1])>>8; p[0]=       a+((1+a+(b&255))>>8); /* p[0]=p[0]*p[3]/255; */
        a=(b=bg[0]*p[1])>>8; p[0]+=bg[0]-a-((1+a+(b&255))>>8);
        p+=2;
      }
    }
    break;
   case GIMP_INDEXEDA_IMAGE:
    /* Imp: do a better thing than no-op here */
    break;
   /* Do noting for the remaining values of dra->type */
  }
}


GimpImageType gimp_image_type(gint32 himage){
  MyRes *img=resFind(himage);
  assert(IS_IMAGE(img));
  return img->type;
}
/*hdrawable*/gint32 gimpts_image_first_drawable(gint32 himage) {
  MyRes *img=resFind(himage), *dra;
  assert(IS_IMAGE(img));
  return (dra=img->u.image.dhead->u.drawable.dnext)==img->u.image.dhead ? 0 : dra->handle;
}
/*hdrawable*/gint32 gimpts_drawable_next(gint32 hdrawable) {
  MyRes *dra=resFind(hdrawable);
  assert(IS_DRAWABLE(dra));
  dra=dra->u.drawable.dnext;
  return dra==dra->u.drawable.image->u.image.dhead ? 0 : dra->handle;
}

void gimpts_meta_init(GimptsMeta *meta) {
  meta->minAlphaBpc=0;
  meta->minRGBBpc=0;
  meta->canGray=TRUE;
}
#if 0
static void bpc_min(guchar *bpc_inout, char *p, char *pend) {
  unsigned bpc=*bpc_inout;
  if (bpc>=8) return;
}
#endif
EXTERN_C void gimpts_meta_update(GimptsMeta *meta, gint32 hdrawable) {
  /* Sun Oct  6 17:04:02 CEST 2002 */
  guint ncolors;
  guchar dummy_cmap[768], dummy_dst[1];
  gsize_t wdht;
  guchar *p, *pend, *data, *dataend;
  unsigned minbpb, bpp;
  MyRes *dra=resFind(hdrawable);
  assert(IS_DRAWABLE(dra));
  wdht=(gsize_t)dra->u.drawable.gd.height*dra->u.drawable.gd.width;
  dataend=pend=(data=p=dra->u.drawable.data)+wdht*(bpp=dra->u.drawable.gd.bpp);

  if (IS_DRAWABLE_ALPHA(dra->type) && meta->minAlphaBpc<8) {
    /* vvv 0->0 1->1 2->3 4->7 */
    minbpb=(meta->minAlphaBpc==0) ? 0: meta->minAlphaBpc*2-1;
    p+=bpp-1; pend+=bpp-1;
    for (; p!=pend; p+=bpp) {
      if ((*p&15)*17!=*p) { meta->minAlphaBpc=8; goto b1; } /* 4 bits are not enough */
      else if ((*p&3)*85!=*p) minbpb|=7; /* 2 bits are not enough */
      else if ((*p&1)*255!=*p) minbpb|=3; /* 1 bit is not enough */
      else if (*p!=255) minbpb|=1; /* 0 bits are not enough */
    }
    meta->minAlphaBpc=(minbpb+1)>>1;
    b1: ;
  }
  if (meta->minRGBBpc<8) {
    /* vvv 0->0 1->1 2->3 4->7 */
    minbpb=(meta->minRGBBpc==0) ? 0: meta->minRGBBpc-1;
    if (!IS_DRAWABLE_ALPHA(dra->type)) {
      for (p=data, pend=dataend; p!=pend; p++) {
        if ((*p&15)*17!=*p) { meta->minRGBBpc=8; goto b2; } /* 4 bits are not enough */
        else if ((*p&3)*85!=*p) minbpb|=3; /* 2 bits are not enough */
        else if ((*p&1)*255!=*p) minbpb|=1; /* 1 bit is not enough */
      }
    } else {
      gbool b4=bpp==4;
      assert(bpp==2 || bpp==4);
      for (p=data, pend=dataend; p!=pend; p+=2) {
        if ((*p&15)*17!=*p) { meta->minRGBBpc=8; goto b2; } /* 4 bits are not enough */
        else if ((*p&3)*85!=*p) minbpb|=3; /* 2 bits are not enough */
        else if ((*p&1)*255!=*p) minbpb|=1; /* 1 bit is not enough */
        if (b4) {
          p++;
          if ((*p&15)*17!=*p) { meta->minRGBBpc=8; goto b2; } /* 4 bits are not enough */
          else if ((*p&3)*85!=*p) minbpb|=3; /* 2 bits are not enough */
          else if ((*p&1)*255!=*p) minbpb|=1; /* 1 bit is not enough */
          p++;
          if ((*p&15)*17!=*p) { meta->minRGBBpc=8; goto b2; } /* 4 bits are not enough */
          else if ((*p&3)*85!=*p) minbpb|=3; /* 2 bits are not enough */
          else if ((*p&1)*255!=*p) minbpb|=1; /* 1 bit is not enough */
        }
      }
    }
    meta->minRGBBpc=minbpb+1;
    b2: ;
  } /* IF */
  
  p=data; pend=dataend;
  if (meta->canGray==TRUE) switch (dra->type) {
   case GIMP_GRAY_IMAGE:
   case GIMP_GRAYA_IMAGE:
   case GIMP_CHANNEL:
    break;
   case GIMP_INDEXED_IMAGE:
   case GIMP_INDEXEDA_IMAGE:
    assert(dra->u.drawable.image!=NULLP);
    assert(dra->u.drawable.image->u.image.ncolors>=0);
    p=dra->u.drawable.image->u.image.cmap;
    pend=p+3*(dra->u.drawable.image->u.image.ncolors);
    /* fall through */
   case GIMP_RGB_IMAGE:
    assert(dra->u.drawable.image!=NULLP);
    while (p!=pend) {
      if (p[0]!=p[1] || p[1]!=p[2]) { meta->canGray=FALSE; break; }
      p+=3;
    }
    break;
   case GIMP_RGBA_IMAGE:
    while (p!=pend) {
      if (p[0]!=p[1] || p[1]!=p[2]) { meta->canGray=FALSE; break; }
      p+=4;
    }
    break;
   default: assert(0);
  }

  if ((dra->type|1)==GIMP_INDEXEDA_IMAGE) {
    assert(dra->u.drawable.image!=NULLP);
    meta->nncols=dra->u.drawable.image->u.image.ncolors-(dra->u.drawable.image->u.image.lasttransp?1:0);
  } else {
    gimpts_rgb2indexed((void*)NULLP, dummy_dst, dataend-data, &ncolors, dummy_cmap, data, 0, bpp);
    meta->nncols=ncolors;
  }
  meta->width=dra->u.drawable.gd.width;
  meta->height=dra->u.drawable.gd.height;
}

/*hchannel*/gint32 gimpts_drawable_extract_alpha(gint32 hdrawable) {
  gint32 hchannel;
  MyRes *dra=resFind(hdrawable), *cha;
  guchar *p, *q, *qend;
  gsize_t wdht;
  assert(IS_DRAWABLE(dra));
  wdht=dra->u.drawable.gd.width*dra->u.drawable.gd.height;
  p=dra->u.drawable.data;
  hchannel=gimp_channel_new(0, "Alpha",
    dra->u.drawable.gd.width, dra->u.drawable.gd.height, 100.0, 0);
  cha=resFind(hchannel);
  assert(IS_CHANNEL(cha));
  qend=(q=cha->u.drawable.data)+wdht;
  switch (dra->type) {
   case GIMP_CHANNEL: /* copy it */
    memcpy(q, p, wdht);
    break;
   case GIMP_RGB_IMAGE:
   case GIMP_GRAY_IMAGE:
   case GIMP_INDEXED_IMAGE:
    memset(q, 255, wdht); /* fully opaque */
    break;
   case GIMP_GRAYA_IMAGE:
   case GIMP_INDEXEDA_IMAGE:
    p++; while (q!=qend) { *q++=*p; p+=2; }
    break;
   case GIMP_RGBA_IMAGE:
    p+=3; while (q!=qend) { *q++=*p; p+=4; }
    break;
   default: assert(0);
  }
  return hchannel;
}

#define MAGIC_LEN 64
#define PCX_ID  0
#define PCX_VER 1
#define PCX_ENC 2
#define PCX_BPP 3
GimptsFileFormat gimpts_magic(gchar const* filename0) {
  GimptsFileFormat ret=GIMPTS_FF_INVALID;
  char buf[MAGIC_LEN];
  FILE *f;
  char const*filename=filename0;
#if 0
  filename=*(char**)&filename0; /* pacify gcc const_cast warning */
#endif
  f=filename!=NULLP?fopen(filename,"rb"):stdin;
  if (f==NULLP) return GIMPTS_FF_INVALID;
  memset(buf, '\0', sizeof(buf));
  if (1!=fread(buf, sizeof(buf), 1, f)) ret=GIMPTS_FF_INVALID;
  /* vvv Dat: traditional C doesn't accept "\x.." */
  else if (0==memcmp(buf,"MM\000\052",4) || 0==memcmp(buf,"II\052\000",4)) ret=GIMPTS_FF_TIFF;
  else if (buf[0]=='P' && (buf[1]>='1' && buf[1]<='6') &&
           (buf[2]=='\t' || buf[2]==' ' || buf[2]=='\r' || buf[2]=='\n' || buf[2]=='#')) ret=GIMPTS_FF_PNM;
  else if (buf[PCX_ID]==0x0a && (unsigned char)buf[PCX_VER]<=5 &&
           buf[PCX_ENC]==1 && buf[PCX_BPP]<=8) ret=GIMPTS_FF_PCX;
  else if (0==memcmp(buf, "\377\330\377", 3)) ret=GIMPTS_FF_JPEG;
  else if (0==memcmp(buf,"GIF87a",6) || 0==memcmp(buf,"GIF89a",6)) ret=GIMPTS_FF_GIF;
  else if (0==memcmp(buf, "/* XPM */", 9)) ret=GIMPTS_FF_XPM;
  else if (0==memcmp(buf,"\211PNG\r\n\032\n",8)) ret=GIMPTS_FF_PNG;
  else if (0==memcmp(buf,"FORM",4) && 0==memcmp(buf+8,"ILBM",4)) ret=GIMPTS_FF_LBM;
  else if ((buf[0]=='B' && buf[1]=='M'
   && buf[6]==0 && buf[7]==0 && buf[8]==0 && buf[9]==0
   && (unsigned char)(buf[14])<=64 && buf[15]==0 && buf[16]==0 && buf[17]==0)) ret=GIMPTS_FF_BMP;
  else if ((unsigned char)buf[0]>=30 && (unsigned char)buf[0]<=50 && 
    (unsigned char)buf[1]<=11 &&
    ((unsigned char)buf[16]<=8 || (unsigned char)buf[16]==24)) ret=GIMPTS_FF_TGA;
  /* ^^^ Dat: TGA files don't have real magic numbers :-( */
  /* else if () ret=GIMPTS_FF_; */
  else ret=GIMPTS_FF_UNKNOWN;
  if (filename!=NULLP) fclose(f);
  return ret;
}

/* __END__ */
