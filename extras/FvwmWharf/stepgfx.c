/*
 * Generic graphics and misc. utilities
 * 
 */

#include <X11/Xlib.h>
#include <stdlib.h>

/* Masks to apply to color components when allocating colors 
 * you may want to set them to 0xffff if your display supports 16bpp+
 */
#define RED_MASK	0xff00
#define GREEN_MASK	0xff00
#define BLUE_MASK	0xff00
/*
 *********************************************************************** 
 * Allocates colors and fill a pixel value array. Color values are 
 * truncated with RED_MASK, GREEN_MASK and BLUE_MASK
 * 
 * from,to are the primary colors to use
 * maxcols is the number of colors
 * colors is the array of pixel values
 * light,dark are the colors for the relief
 * dr,dg,db are the increment values for the colors
 * alloc_relief if colors for relief should be allocated
 * incr - 0 if gradient is light to dark, 1 if dark to light
 ***********************************************************************
 */
int MakeColors(Display *dpy, Drawable d, int from[3], int to[3], int maxcols, 
	       unsigned long *colors, unsigned long *dark,
	       unsigned long *light, int dr, int dg, int db,
	       int alloc_relief, int incr)
{
    float rv, gv, bv;
    float sr,sg,sb;
    int red, green, blue;
    XColor color;
    int i;

    sr = (float)dr / (float)maxcols;
    sg = (float)dg / (float)maxcols;
    sb = (float)db / (float)maxcols;    
    rv = (float)from[0];
    gv = (float)from[1];
    bv = (float)from[2];
    /* kludge to frce color allocation in the first iteration on any case */
    color.red = (from[0] == 0) ? 0xffff : 0;
    color.green = 0;
    color.blue = 0;
    for(i = 0; i < maxcols; i++) {
	/* color allocation saver */	
	red = ((short)rv) & RED_MASK;
	green = ((short)gv) & GREEN_MASK;
	blue = ((short)bv) & BLUE_MASK;
	if (color.red != red || color.green != green || color.blue != blue) {
	    color.red = red;
	    color.green = green;
	    color.blue = blue;
	    if (XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
			&color)==0) {
	        return 0;
	    }
	}
	colors[i] = color.pixel;
	rv += sr;
	gv += sg;
	bv += sb;
	rv = ((rv > 65535.0) || (rv < 0.0)) ? rv -= sr : rv;
	gv = ((gv > 65535.0) || (gv < 0.0)) ? gv -= sg : gv;
	bv = ((bv > 65535.0) || (bv < 0.0)) ? bv -= sb : bv;
    }
    /* allocate 2 colors for the bevel */
    if (alloc_relief) {
	if (incr) {
	    rv = (float)from[0] * 0.8;
	    gv = (float)from[1] * 0.8;
	    bv = (float)from[2] * 0.8;
	} else {
	    rv = (float)to[0] * 0.8;
	    gv = (float)to[1] * 0.8;
	    bv = (float)to[2] * 0.8;
	}
	color.red = (unsigned short)(rv<0?0:rv);
    color.green = (unsigned short)(gv<0?0:gv);
	color.blue = (unsigned short)(bv<0?0:bv);		
	if (XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),&color))
	  *dark = color.pixel;
	else
	  *dark = colors[incr ? 0 : maxcols-1];
	
	if (incr) {
	    float avg;
	    avg = (float)(to[0]+to[1]+to[2])/3;	
	    rv = avg + (float)(to[0]/2);
	    gv = avg + (float)(to[1]/2);
	    bv = avg + (float)(to[2]/2);
	} else {
            float avg; 	
	    avg = (float)(from[0]+from[1]+from[2])/3;
	    rv = avg + (float)(from[0]/2);
	    gv = avg + (float)(from[1]/2);
	    bv = avg + (float)(from[2]/2);
	}
	color.red = (unsigned short)(rv>65535.0 ? 65535.0:rv);
    color.green = (unsigned short)(gv>65535.0 ? 65535.0:gv);
	color.blue = (unsigned short)(bv>65535.0 ? 65535.0:bv);
	if (XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), 
			&color))
	  *light = color.pixel;
	else
	  *light = colors[incr ? maxcols-1 : 0];
    } else {
	*light = colors[incr ? maxcols-1 : 0];
	*dark = colors[incr ? 0 : maxcols-1];   
    }    
    return 1;
}


/*
 ************************************************************************
 * 
 * Draws a texture similar to Wharf's button background
 *
 * from: r,g,b values of top left color
 * to: r,g,b values of bottom right color
 * relief: relief to add to the borders. 0=flat, >0 raised, <0 sunken
 * maxcols: # of max colors to use
 * 
 * Aborts and returns 0 on error
 ************************************************************************
 */

int DrawDegradeRelief(Display *dpy, Drawable d, int x, int y, int w, int h, 
		       int from[3], int to[3], int relief, int maxcols)
{
    int		i,j,k;
    int		dr, dg, db;  /* delta */
    int		dmax, dmax2;
    GC		gc, gc2;
    XGCValues	gcv;
    int		px, py, pd;
    unsigned long lightcolor, darkcolor;
    int 	alloc_relief;
    unsigned long  *colors;
    float	c1,s1;

    if (w <= 0 || h <= 0)
      return 0;

    gcv.foreground = 1;
    gc = XCreateGC(dpy, d, GCForeground, &gcv);
    gc2 = XCreateGC(dpy, d, GCForeground, &gcv);

    dr = to[0] - from[0];
    dg = to[1] - from[1];
    db = to[2] - from[2];
    
    dmax = abs(dr);
    dmax2 = dr;
    if (dmax < abs(dg)) {
	dmax = abs(dg);
	dmax2 = dg;
    }
    if (dmax < abs(db)) {
	dmax = abs(db);
	dmax2 = db;
    }    
    /* if colors from and to are the same, force to one color */
    if (dmax==0)  dmax=1;

    if (relief!=0) {
	if (maxcols>4) {
	    alloc_relief=1;
	    maxcols -= 2; /* reserve two colors for the relief */
	} else {
	    alloc_relief=0;
	}
    } else
      alloc_relief = 0;
    
    if (dmax < maxcols) {
	maxcols = dmax; /* we need less colors than we expected */
    }
    
    colors = malloc(sizeof(unsigned long)*maxcols);
    /* alloc colors */
    if (!MakeColors(dpy, d, from, to, maxcols, colors, &darkcolor, &lightcolor,
	       dr, dg, db, alloc_relief, dmax2>0))
      return 0;
    
    /* display it */
    s1 = (float)(maxcols)/((float)w*2.0);
    pd = relief==0 ? 0 : 1;
    for(py = pd; py < h-pd; py++) {
	c1 = ((float)maxcols/2.0)*((float)py/(float)((h-pd*2)-1));
	j = -1;
	k=pd+x;
	for(px = pd+x; px < w-pd+x; px++) {
	    i = (int)c1;
	    if (i>=maxcols) i=maxcols-1;
	    if (j != colors[i]) {
		j = colors[i];
		XSetForeground(dpy, gc, j);
		XDrawLine(dpy, d, gc, k, y+py, px, y+py);
		k=px;
	    }
	    c1+=s1;
	}
	XSetForeground(dpy, gc, colors[i]);
	XDrawLine(dpy, d, gc, k, y+py, w-pd+x, y+py);
    }
    /* draw borders */
    if (relief != 0) {
	if (relief>0) {
	    XSetForeground(dpy, gc, lightcolor);
	    XSetForeground(dpy, gc2, darkcolor);
	} else {
	    XSetForeground(dpy, gc, darkcolor);
	    XSetForeground(dpy, gc2, lightcolor);	    
	}
	XDrawLine(dpy, d, gc, x, y, x, y+h-1);
	XDrawLine(dpy, d, gc, x, y, x+w-1, y);
	XDrawLine(dpy, d, gc2, x, y+h-1, x+w-1, y+h-1);
	XDrawLine(dpy, d, gc2, x+w-1, y, x+w-1, y+h-1);
    }
    XFreeGC(dpy,gc);
    XFreeGC(dpy,gc2);
    free(colors);
    return 1;
}


/*
 ************************************************************************
 * 
 * Draws a horizontal gradient
 *
 * from: r,g,b values of top color
 * to: r,g,b values of bottom color
 * relief: relief to add to the borders. 0=flat, >0 raised, <0 sunken
 * maxcols: max colors to use
 * type: gradient type. 0 for one-way, != 0 for cilindrical
 * 
 * aborts and returns 0 on error.
 ************************************************************************
 */
int DrawHGradient(Display *dpy, Drawable d, int x, int y, int w, int h, 
		       int from[3], int to[3], int relief, int maxcols,
		       int type)
{
    int		i,j;
    XGCValues	gcv;
    GC		gc, gc2;
    int		dr,dg,db;
    float	s,c;
    int 	dmax;
    unsigned long  *colors, lightcolor, darkcolor;
    int 	py,pd;
    int 	alloc_relief;
    
    if (w <= 1 || h <= 1)
      return 0;

    gcv.foreground = 1;
    gc = XCreateGC(dpy, d, GCForeground, &gcv);
    gc2 = XCreateGC(dpy, d, GCForeground, &gcv);
    
    dr = to[0] - from[0];
    dg = to[1] - from[1];
    db = to[2] - from[2];
    
    dmax = dr;
    if (abs(dmax) < abs(dg)) {
	dmax = dg;
    }
    if (abs(dmax) < abs(db)) {
	dmax = db;
    }
    if (relief!=0) {
	if (maxcols>4) {
	    alloc_relief=1;
	    maxcols -= 2; /* reserve two colors for the relief */
	} else {
	    alloc_relief=0;
	}
    } else
      alloc_relief=0;
    
    if (type) {
	if (h/2 < maxcols) {
	    maxcols = h/2; /* we need less colors than we expected */
	}
    } else {
	if (h < maxcols) {
	    maxcols = h; /* we need less colors than we expected */
	}
    }
    /* alloc colors */
    colors=malloc(maxcols*sizeof(unsigned long));
    if (!MakeColors(dpy, d, from, to, maxcols, colors, &darkcolor, &lightcolor,
	       dr, dg, db, alloc_relief, dmax>0))
      return 0;
    /* display it */
    if (type) {
	s = ((float)maxcols*2)/(float)h;
    } else {
	s = (float)maxcols/(float)h;	
    }    
    pd = relief==0 ? 0 : 1;
    c=0;
    j=-1;
    if (type==0) { /* one-way */
	for(py = pd; py < h-pd; py++) {	
	    i=(int)c;
	    if (i>=maxcols) i=maxcols-1;
	    if (j != i) {
		XSetForeground(dpy, gc, colors[i]);
	    }
	    XDrawLine(dpy, d, gc, x+pd, y+py, x+w-1-pd, y+py);
	    j = i;
	    c += s;
	}
    } else { /* cylindrical */
	for(py = pd; py < h-pd; py++) {	
	    i=(int)c;
	    if (i>=maxcols) i=maxcols-1;
	    if (j != i) {
		XSetForeground(dpy, gc, colors[i]);
	    }
	    XDrawLine(dpy, d, gc, x+pd, y+py, x+w-1-pd, y+py);
	    j = i;
	    if (py == h/2) s = -s;
	    c += s;
	}
    }    
    /* draw borders */
    if (relief != 0) {
	if (relief>0) {
	    XSetForeground(dpy, gc, lightcolor);
	    XSetForeground(dpy, gc2, darkcolor);
	} else {
	    XSetForeground(dpy, gc, darkcolor);
	    XSetForeground(dpy, gc2, lightcolor);	    
	}
	XDrawLine(dpy, d, gc, x, y, x, y+h-1);
	XDrawLine(dpy, d, gc, x, y, x+w-1, y);
	XDrawLine(dpy, d, gc2, x, y+h-1, x+w-1, y+h-1);
	XDrawLine(dpy, d, gc2, x+w-1, y, x+w-1, y+h-1);
    }
    XFreeGC(dpy,gc);
    XFreeGC(dpy,gc2);
    free(colors);
    return 1;
}


/*
 ************************************************************************
 * 
 * Draws a vertical gradient
 *
 * from: r,g,b values of left color
 * to: r,g,b values of right color
 * relief: relief to add to the borders. 0=flat, >0 raised, <0 sunken
 * maxcols: max colors to use
 * type: gradient type. 0 for one-way, != 0 for cilindrical
 * 
 * aborts and returns 0 on error.
 ************************************************************************
 */
int DrawVGradient(Display *dpy, Drawable d, int x, int y, int w, int h, 
		       int from[3], int to[3], int relief, int maxcols,
		       int type)
{
    int		i,j;
    XGCValues	gcv;
    GC		gc, gc2;
    int		dr,dg,db;
    float	s,c;
    int 	dmax;
    unsigned long  *colors, lightcolor, darkcolor;
    int 	px,pd;
    int 	alloc_relief;
    
    if (w <= 1 || h <= 1)
      return 0;

    gcv.foreground = 1;
    gc = XCreateGC(dpy, d, GCForeground, &gcv);
    gc2 = XCreateGC(dpy, d, GCForeground, &gcv);
    
    dr = to[0] - from[0];
    dg = to[1] - from[1];
    db = to[2] - from[2];
    
    dmax = dr;
    if (abs(dmax) < abs(dg)) {
	dmax = dg;
    }
    if (abs(dmax) < abs(db)) {
	dmax = db;
    }
    if (relief!=0) {
	if (maxcols>4) {
	    alloc_relief=1;
	    maxcols -= 2; /* reserve two colors for the relief */
	} else {
	    alloc_relief=0;
	}
    } else
      alloc_relief=0;
    
    if (type) {
	if (w/2 < maxcols) {
	    maxcols = w/2; /* we need less colors than we expected */
	}
    } else {
	if (w < maxcols) {
	    maxcols = w; /* we need less colors than we expected */
	}
    }
    /* alloc colors */
    colors=malloc(maxcols*sizeof(unsigned long));
    if (!MakeColors(dpy, d, from, to, maxcols, colors, &darkcolor, &lightcolor,
	       dr, dg, db, alloc_relief, dmax>0))
      return 0;
    /* display it */
    if (type) {
	s = ((float)maxcols*2)/(float)w;
    } else {
	s = (float)maxcols/(float)w;	
    }    
    pd = relief==0 ? 0 : 1;
    c=0;
    j=-1;
    if (type==0) { /* one-way */
	for(px = pd; px < w-pd; px++) {	
	    i=(int)c;
	    if (i>=maxcols) i=maxcols-1;
	    if (j != i) {
		XSetForeground(dpy, gc, colors[i]);
	    }
	    XDrawLine(dpy, d, gc, x+px, y+pd, x+px, y+h-pd-1);
	    j = i;
	    c += s;
	}
    } else { /* cylindrical */
	for(px = pd; px < w-pd; px++) {	
	    i=(int)c;
	    if (i>=maxcols) i=maxcols-1;
	    if (j != i) {
		XSetForeground(dpy, gc, colors[i]);
	    }
	    XDrawLine(dpy, d, gc, x+px, y+pd, x+px, y+h-pd-1);
	    j = i;
	    if (px == w/2) s = -s;
	    c += s;
	}
    }
    /* draw borders */
    if (relief != 0) {
	if (relief>0) {
	    XSetForeground(dpy, gc, lightcolor);
	    XSetForeground(dpy, gc2, darkcolor);
	} else {
	    XSetForeground(dpy, gc, darkcolor);
	    XSetForeground(dpy, gc2, lightcolor);	    
	}
	XDrawLine(dpy, d, gc, x, y, x, y+h-1);
	XDrawLine(dpy, d, gc, x, y, x+w-1, y);
	XDrawLine(dpy, d, gc2, x, y+h-1, x+w-1, y+h-1);
	XDrawLine(dpy, d, gc2, x+w-1, y, x+w-1, y+h-1);
    }
    XFreeGC(dpy,gc);
    XFreeGC(dpy,gc2);
    free(colors);
    return 1;
}


/************************************************************************
 * 
 * Draws text with a texture
 * 
 * d - target drawable
 * font - font to draw text
 * x,y - position of text
 * gradient - texture pixmap. size must be at least as large as text
 * text - text to draw
 * chars - chars in text
 ************************************************************************/
void DrawTexturedText(Display *dpy, Drawable d, XFontStruct *font,
		      int x, int y, Pixmap gradient, char *text, int chars)
		      
{
    Pixmap mask;
    int w,h;
    GC gc;
    XGCValues gcv;
    
    /* make the mask pixmap */
    w = XTextWidth(font,text,chars);
    h = font->ascent+font->descent;
    mask=XCreatePixmap(dpy,DefaultRootWindow(dpy),w+1,h+1,1);
	gcv.foreground = 0;
	gcv.function = GXcopy;
    gcv.font = font->fid;	
	gc = XCreateGC(dpy,mask,GCFunction|GCForeground|GCFont,&gcv);
    XFillRectangle(dpy,mask,gc,0,0,w,h);
	XSetForeground(dpy,gc,1);
    XDrawString(dpy,mask,gc,0,font->ascent,text,chars);
	XFreeGC(dpy,gc);
	/* draw the texture */
	gcv.function=GXcopy;
	gc = XCreateGC(dpy,d,GCFunction,&gcv);
    XSetClipOrigin(dpy,gc,x,y);
    XSetClipMask(dpy,gc,mask);
    XCopyArea(dpy,gradient,d,gc,0,0,w,h,x,y);
    XFreeGC(dpy,gc);
    XFreePixmap(dpy,mask);
}




int MakeShadowColors(Display *dpy, int from[3], int to[3],
		     unsigned long *dark, unsigned long *light)
{
    float rv, gv, bv;
    XColor color;
    int i;
	int incr;
	int dr,dg,db,dmax;

    dr = to[0] - from[0];
    dg = to[1] - from[1];
    db = to[2] - from[2];
    
    dmax = dr;
    if (abs(dmax) < abs(dg)) {
		dmax = dg;
    }
    if (abs(dmax) < abs(db)) {
		dmax = db;
    }
	incr=(dmax>0);
    /* allocate 2 colors for the bevel */    
	if (incr) {
	    rv = (float)from[0] * 0.8;
	    gv = (float)from[1] * 0.8;
	    bv = (float)from[2] * 0.8;
	} else {
	    rv = (float)to[0] * 0.8;
	    gv = (float)to[1] * 0.8;
	    bv = (float)to[2] * 0.8;
	}
	color.red = (short)(rv<0?0:rv);
    color.green = (short)(gv<0?0:gv);
	color.blue = (short)(bv<0?0:bv);
	if (XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),&color))
	  *dark = color.pixel;
	else
	  return 0;
	
	if (incr) {
	    float avg;
	    avg = (float)(to[0]+to[1]+to[2])/3;	
	    rv = avg + (float)(to[0]/2);
	    gv = avg + (float)(to[1]/2);
	    bv = avg + (float)(to[2]/2);
	} else {
        float avg; 	
	    avg = (float)(from[0]+from[1]+from[2])/3;
	    rv = avg + (float)(from[0]/2);
	    gv = avg + (float)(from[1]/2);
	    bv = avg + (float)(from[2]/2);
	}
	color.red = (unsigned short)(rv>65535.0 ? 65535.0:rv);
    color.green = (unsigned short)(gv>65535.0 ? 65535.0:gv);
	color.blue = (unsigned short)(bv>65535.0 ? 65535.0:bv);	
	if (XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
					&color))
	  *light = color.pixel;
	else
	  return 0;
    return 1;
}
