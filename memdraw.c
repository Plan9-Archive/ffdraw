#include "draw.h"
#include <stdio.h>
#include "memdraw.h"

static Memimage*	memones;
static Memimage*	memzeros;
Memimage *memwhite;
Memimage *memblack;
Memimage *memtransparent;
Memimage *memopaque;

/*
 * Conversion tables.
 */
static uchar replbit[1+8][256];		/* replbit[x][y] is the replication of the x-bit quantity y to 8-bit depth */
static uchar conv18[256][8];		/* conv18[x][y] is the yth pixel in the depth-1 pixel x */
static uchar conv28[256][4];		/* ... */
static uchar conv48[256][2];

/*
 * bitmap of how to replicate n bits to fill 8, for 1 ≤ n ≤ 8.
 * the X's are where to put the bottom (ones) bit of the n-bit pattern.
 * only the top 8 bits of the result are actually used.
 * (the lower 8 bits are needed to get bits in the right place
 * when n is not a divisor of 8.)
 *
 * Should check to see if its easier to just refer to replmul than
 * use the precomputed values in replbit.  On PCs it may well
 * be; on machines with slow multiply instructions it probably isn't.
 */
#define a ((((((((((((((((0
#define X *2+1)
#define _ *2)
static int replmul[1+8] = {
	0,
	a X X X X X X X X X X X X X X X X,
	a _ X _ X _ X _ X _ X _ X _ X _ X,
	a _ _ X _ _ X _ _ X _ _ X _ _ X _,
	a _ _ _ X _ _ _ X _ _ _ X _ _ _ X,
	a _ _ _ _ X _ _ _ _ X _ _ _ _ X _,
	a _ _ _ _ _ X _ _ _ _ _ X _ _ _ _, 
	a _ _ _ _ _ _ X _ _ _ _ _ _ X _ _,
	a _ _ _ _ _ _ _ X _ _ _ _ _ _ _ X,
};
#undef a
#undef X
#undef _

static void
mktables(void)
{
	int i, j, mask, sh, small;
		
	/* bit replication up to 8 bits */
	for(i=0; i<256; i++){
		for(j=0; j<=8; j++){	/* j <= 8 [sic] */
			small = i & ((1<<j)-1);
			replbit[j][i] = (small*replmul[j])>>8;
		}
	}

	/* bit unpacking up to 8 bits, only powers of 2 */
	for(i=0; i<256; i++){
		for(j=0, sh=7, mask=1; j<8; j++, sh--)
			conv18[i][j] = replbit[1][(i>>sh)&mask];

		for(j=0, sh=6, mask=3; j<4; j++, sh-=2)
			conv28[i][j] = replbit[2][(i>>sh)&mask];

		for(j=0, sh=4, mask=15; j<2; j++, sh-=4)
			conv48[i][j] = replbit[4][(i>>sh)&mask];
	}
}

uchar*
byteaddr(Memimage *i, Point p)
{
	uchar *a;

	a = i->data->bdata+i->zero+(int)(sizeof(ulong)*p.y*i->width);
	if(i->depth < 8){
		/*
		 * We need to always round down,
		 * but C rounds toward zero.
		 */
		int np;
		np = 8/i->depth;
		if(p.x < 0)
			return a+(p.x-np+1)/np;
		else
			return a+p.x/np;
	}
	else
		return a+p.x*(i->depth/8);
}

void
memimageinit(void)
{
	static int didinit = 0;

	if(didinit)
		return;

	mktables();
	_memmkcmap();

	memones = allocmemimage(Rect(0,0,1,1), GREY1);
	memzeros = allocmemimage(Rect(0,0,1,1), GREY1);
	if(memones == nil || memzeros == nil)
		return;

	memones->flags |= Frepl;
	memones->clipr = Rect(-0x3FFFFFF, -0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF);
	*byteaddr(memones, ZP) = ~0;

	memzeros->flags |= Frepl;
	memzeros->clipr = Rect(-0x3FFFFFF, -0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF);
	*byteaddr(memzeros, ZP) = 0;

	memwhite = memones;
	memblack = memzeros;
	memopaque = memones;
	memtransparent = memzeros;

	didinit = 1;
	return;
}

int
memsetchan(Memimage *i, ulong chan)
{
	int d;
	int t, j, k;
	ulong cc;
	int bytes;

	if((d = chantodepth(chan)) == 0) {
		fprintf(stderr, "bad channel descriptor");
		return -1;
	}

	i->depth = d;
	i->chan = chan;
	i->flags &= ~(Fgrey|Falpha|Fcmap|Fbytes);
	bytes = 1;
	for(cc=chan, j=0, k=0; cc; j+=NBITS(cc), cc>>=8, k++){
		t=TYPE(cc);
		if(t < 0 || t >= NChan){
			fprintf(stderr, "bad channel string");
			return -1;
		}
		if(t == CGrey)
			i->flags |= Fgrey;
		if(t == CAlpha)
			i->flags |= Falpha;
		if(t == CMap && i->cmap == nil){
			i->cmap = memdefcmap;
			i->flags |= Fcmap;
		}

		i->shift[t] = j;
		i->mask[t] = (1<<NBITS(cc))-1;
		i->nbits[t] = NBITS(cc);
		if(NBITS(cc) != 8)
			bytes = 0;
	}
	i->nchan = k;
	if(bytes)
		i->flags |= Fbytes;
	return 0;
}

Memimage*
allocmemimaged(Rectangle r, ulong chan, Memdata *md)
{
	int d;
	ulong l;
	Memimage *i;

	if((d = chantodepth(chan)) == 0) {
		fprintf(stderr, "bad channel descriptor %.8lux", chan);
		return nil;
	}
	if(badrect(r)){
		fprintf(stderr, "bad rectangle");
		return nil;
	}

	l = wordsperline(r, d);

	i = calloc(sizeof(Memimage), 1);
	if(i == nil)
		return nil;

	i->data = md;
	i->zero = sizeof(ulong)*l*r.min.y;
	
	if(r.min.x >= 0)
		i->zero += (r.min.x*d)/8;
	else
		i->zero -= (-r.min.x*d+7)/8;
	i->zero = -i->zero;
	i->width = l;
	i->r = r;
	i->clipr = r;
	i->flags = 0;
	i->layer = nil;
	i->cmap = memdefcmap;
	if(memsetchan(i, chan) < 0){
		free(i);
		return nil;
	}
	return i;
}

Memimage*
allocmemimage(Rectangle r, ulong chan)
{
	int d;
	uchar *p;
	ulong l, nw;
	Memdata *md;
	Memimage *i;

	if((d = chantodepth(chan)) == 0) {
		fprintf(stderr, "bad channel descriptor %.8lux", chan);
		return nil;
	}
	if(badrect(r)){
		fprintf(stderr, "bad rectangle");
		return nil;
	}

	l = wordsperline(r, d);

	nw = l*Dy(r);
	md = malloc(sizeof(Memdata));
	if(md == nil)
		return nil;

	md->ref = 1;
	md->base = malloc(sizeof(Memdata*)+(1+nw)*sizeof(ulong));
	if(md->base == nil){
		free(md);
		return nil;
	}

	p = (uchar*)md->base;
	*(Memdata**)p = md;
	p += sizeof(Memdata*);

	*(ulong*)p = 0xDEADBEEF; // TODO getcallerpc(&r);
	p += sizeof(ulong);

	/* if this changes, memimagemove must change too */
	md->bdata = p;
	md->allocd = 1;

	i = allocmemimaged(r, chan, md);
	if(i == nil){
		free(md->base);
		free(md);
		return nil;
	}
	md->imref = i;
	return i;
}
