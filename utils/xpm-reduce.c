/****************************************************************************
 * This module is all original code by Dan Espen
 * except for the algorithm for color closeness which 
 * comes from the xpm package.
 * Copyright 1995, Dan Espen
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/
/*
  compile with:

  gcc -I<xpm>/include xpmr.c -lXpm -lX11 -o xpmr

  Purpose: Reduce number of colors used in xpms.

  Arg 1: Input full rgb.txt file. Example /usr/openwin/lib/rgb.txt.
  Arg 2: Input subset of rgb.txt file.  Only include the colors you want in
         the xpms.
  Rest of args: xpm file to read and then re-write.  If you want to preserve
  the originals, make a copy.

  Notes:

  The color closeness algorithm seems a little weak.  With my table, I
  would want it to convert tan to wheat, but it picks gray.  Of course
  putting tan in the subset rgb.txt file  would fix that, but the idea
  is to reduce the number of colors.

  Some tuning  of the subset rgb.txt file  would be  nice.  One idea I
  haven't  persued yet is doing  the closeness calculation for all the
  colors in the subset (excluding the color itself).  Then colors that
  are too  close to each  other could be  culled out.  (I sort  of did
  this by hand.)

  The output xpms can end up with  multiple characters assigned to the
  same  color.  Then things  like xbmbrowser  report  more colors than
  there really  are.  I haven't  found any program  that has a problem
  with  this, but  it would be   cleaner  to consolidate  the multiple
  characters.

  One nice side effect of this program is you end  up with color names
  instead of numbers.  If thats  all you want to  do, you can use  the
  full rgb.txt file as the subset.

  */

#include <stdio.h>                      /* for i/o */
#include <errno.h>                      /* for i/o error reporting */
#include <stdlib.h>                     /* for exit */
#include <X11/xpm.h>                    /* XPM stuff */

struct rgb_s {
  int r;
  int g;
  int b;
  char name[30];
  int used;
};

static struct rgb_s all_colors[2000] = {0}; /* my rgb.txt had 700 lines */
static int all_colors_found = 0;        /* end of table */
static struct rgb_s colors[2000] = {0}; /* subset rgb.txt */
static int colors_found = 0;            /* end of table */

static struct hex_color {
  char rr[2];
  char gg[2];
  char bb[2];
  char null_char;
} hc;

static int best;

static void hex_color(char *in_color);
static char *hex_to_name(struct hex_color *color_name);
static int count_used();
static int hex_to_int(char hex[2]);
static void read_rgb(struct rgb_s[], char *);

int main(argc, argv)
int argc;
char *argv[]; {
  XpmImage image;
  XpmInfo  info;
  int stat, ncolors;
  XpmColor *color_table_ptr;
  char *color_name;
  int image_count, image_index;

  fflush(stdout);
  if (argc < 4) {
    fprintf(stderr,
            "Usage: xpmr full.rgb.txt file subset.rgb.txt.file xpmfiles ...");
    exit (EXIT_FAILURE);
  }
  read_rgb(all_colors, argv[1]); /* read all rgb database */
  all_colors_found = colors_found;      /* this is a little sloppy, dje */
  colors_found = 0;
  read_rgb(colors, argv[2]);            /* load rgb table */
  image_count = argc - 3;               /* number of xpm files */
  image_index = 3;                      /* offset to one to process */
  while (image_count--) {
    printf("\nFile: %s\nOriginal color       New color           Closeness\n",
           argv[image_index]);
    stat = XpmReadFileToXpmImage( argv[image_index],
                                &image, &info);
    if (stat) {                         /* read error */
      fprintf(stderr,
              "xpmr: XpmReadFileFromXpmImage status %d xpm %s\n",
              stat, argv[image_index]);
      exit (EXIT_FAILURE);
    }
    /*      printf("chars/pixel %d, ncolors %d stat %d\n",
         image.cpp,
         image.ncolors, stat); */

    ncolors = 0;
    color_table_ptr = image.colorTable;
    while (ncolors < image.ncolors) {
      /*  Not interested in masks, and gray scales...dje
      if (color_table_ptr->m_color || color_table_ptr->g4_color ||
          color_table_ptr->g_color) {
        printf("color table mono %x, gray4 %x gray %x\n",
               color_table_ptr->m_color,
               color_table_ptr->g4_color,
               color_table_ptr->g_color);
      }*/
      if (strcasecmp("none", color_table_ptr->c_color)) {
        hex_color(color_table_ptr->c_color);
        color_name = hex_to_name(&hc);      /* get color name */
        if (color_table_ptr->c_color) {
          printf("%-20.20s",
                 color_table_ptr->c_color);
        }
        printf(" %-20.20s %d\n",
               color_name,              /* write new color value */
               best);                   /* degree of match */
        color_table_ptr->c_color = color_name; /* change color */
      }                                 /* end color not "none" */
      color_table_ptr +=1;
      ncolors +=1;
    }                                   /* end while colors in xpm */

    stat = XpmWriteFileFromXpmImage(argv[image_index], &image, &info);
    if (stat) {                         /* write error */
      fprintf(stderr,
              "xpmr: XpmWriteFileFromXpmImage status %d xpm %s\n",
              stat, argv[image_index]);
    }

    ++image_index;                      /* next xpm */
  }                                     /* end while xpm files on cmd line */
  printf("\nTotal colors used in all xpms: %d\n", count_used());
  exit (0);

}

/*
  Read rgb database format into an array
  */
static void read_rgb(array, table_name)
struct rgb_s array[];
char *table_name; {
  FILE *rgb;
  int i;
  char two_tabs[80];                     /* for garbage */
  rgb = fopen(table_name, "r");
  if (!rgb) {
    char msg[80];
    sprintf(msg, "Error opening rgb.txt format file: <%s>", table_name);
    perror(msg);
    exit(EXIT_FAILURE);
  }

  while (EOF != (i=fscanf(rgb, "%d%d%d%[ 	]%[0-9A-Za-z ]",
                  &array[colors_found].r,
                  &array[colors_found].g,
                  &array[colors_found].b,
                  two_tabs,
                  &array[colors_found].name))) {
    ++colors_found;
  }
  fclose(rgb);
  return;
}

/*
  Given a color name or number, return the color number in uppercase hex
  */
static void hex_color(in_color)
char *in_color; {
  int i;

  if (*in_color == '#') {               /* if color number */
    if (*(in_color+4) == '\0') {        /* if 3 digit color */
      hc.rr[0] = *(in_color+1);
      hc.rr[1] = *(in_color+1);
      hc.gg[0] = *(in_color+2);
      hc.gg[1] = *(in_color+2);
      hc.bb[0] = *(in_color+3);
      hc.bb[1] = *(in_color+3);
    } else if (*(in_color+7) == '\0') { /* if 6 digit color */
      memcpy(&hc, in_color+1, 6);       /* copy rgb all at once */
    } else if (*(in_color+10) == '\0') { /* if 9 digit color */
      memcpy(hc.rr, in_color+1, 2);     /* get h/o red */
      memcpy(hc.gg, in_color+4, 2);
      memcpy(hc.bb, in_color+7, 2);
    } else {                            /* 12 digit color */
      memcpy(hc.rr, in_color+1, 2);     /* get h/o red */
      memcpy(hc.gg, in_color+5, 2);
      memcpy(hc.bb, in_color+9, 2);
    }
    {
      char *work = (char *)&hc;
      for(i=0;i<6;i++) {
        (char)*work = toupper( (int)*work);
        ++work;
      }
    }
  } else {                            /* if not number, must be a name */
    i = 0;
    while (i <= all_colors_found) {
      if (strcasecmp(in_color, all_colors[i].name) == 0) { /* if match */
        sprintf(hc.rr, "%2.2X", all_colors[i].r);
        sprintf(hc.gg, "%2.2X", all_colors[i].g);
        sprintf(hc.bb, "%2.2X", all_colors[i].b);
        i = all_colors_found + 6;          /* stop loop */
        continue;
      } else {
        ++i;                          /* next color */
      }                               /* end match/no match */
    }                                 /* end while */
    if (i != all_colors_found + 6) {      /* color not found */
      fprintf(stderr, "Ugh, color %s not found\n", in_color);
      memcpy(hc.rr, "FFFFFF", 6);
    }
  }                                   /* end number/name */
}

/* Convert 6 digit color number to closest name */

#define CLOSEN(named, want) \
if (named <= want) { diff = want - named; } else { diff = named - want; }


static char *
hex_to_name(hc)
struct hex_color *hc; {
  int red,green,blue;                   /* rgb values as ints */
  int i;                                /* worker */
  int diff;                             /* result from CLOSEN macro */
  int best_index;
  char *best_name;
  int closeness = 0;

  best = 32767;                         /* impossibly high */
  red = hex_to_int(hc->rr);             /* do conversions */
  green = hex_to_int(hc->gg);
  blue = hex_to_int(hc->bb);

  i = 0;
  while (i <= colors_found) {

    /* This algorithm from xpm lib.  The closeness is based on the
       sum of the r/g/b differences times 3 Plus the overall brightness
       difference.
       */
    CLOSEN(colors[i].r, red);
    closeness = diff;
    CLOSEN(colors[i].b, blue);
    closeness += diff;
    CLOSEN(colors[i].g, green);
    closeness += (diff);

    closeness *= 3;
    CLOSEN((red+green+blue), (colors[i].r+colors[i].b+colors[i].g));
    closeness += diff;

    if (closeness < best) {             /* if better match */
      best = closeness;                 /* save new best value */
      best_name = colors[i].name;       /* save the new best name */
      best_index = i;                   /* save location of best */
    }
    ++i;                                /* next color */
  }                                     /* end while */
  ++colors[best_index].used;            /* mark that this cell used */
  /*  printf("r %d g %d b %d closeness %d best %d\n",
         red, green, blue, closeness, best);*/
  return(best_name);                    /* return best match */
}                                       /* end function */

static int
count_used() {
  int i = 0;                            /* worker */
  int used = 0;                         /* total colors used */
  while (i <= colors_found) {
    if (colors[i].used) {               /* if this color used */ 
     ++used;                           /* count it */
    }                                   /* end this color used */
    ++i;                                /* next color */
  }                                     /* end while */
  return(used);                         /* return total colors used */
}                                       /* end function */

/* convert 2 character hex value to an int */
static int hex_to_int(hex)
char hex[2]; {
  int ret_int;

  if (hex[1] >= 'A' && hex[1] <= 'F') {
    ret_int = 10 + hex[1] - 'A';
  } else {
    ret_int = hex[1] - '0';
  }
  if (hex[0] >= 'A' && hex[0] <= 'F') {
    ret_int += (10 + hex[0] - 'A') * 16;
  } else {
    ret_int += (hex[0] - '0') * 16;
  }
  return(ret_int);
}
