#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "WinInput.h"

static char *lookup_key(XEvent *ev,int *pcount);
void addchar(char c, char *buffer, int bufsize, int &ptr, int& endptr);
#define KBUFSIZE 100 

WinInput::WinInput(WinBase *Parent, int new_w,int new_h,
		      int new_x, int new_y, char *initlabel):
     WinText(Parent,new_w, new_h, new_x,new_y, NULL)
{
  int i;

  text_offset = bw+4;
  NewLineAction = NULL;
  if(initlabel)
    {
      label_size = (int)((strlen(initlabel)+1)/100) + 100;
      label = new char[label_size];
      num_chars =strlen(initlabel);
      ptr = num_chars;
      endptr = num_chars;
      strcpy(label,initlabel);
    }
  else
    {
      num_chars = 0;
      label_size = 100;
      label = new char[label_size];
      label[0] = 0;

      ptr = 0;
      endptr = 0;
    }
}

WinInput::~WinInput()
{
  delete label;
}

void WinInput::SetNewLineAction(void (*new_NewLineAction)(int numchars, 
						      char *newdata))
{ 
  NewLineAction = new_NewLineAction;
}

char * WinInput::GetLine()
{
  return label;
}

void WinInput::SetLabel(char *text)
{
  if(!text)
    return;
  if(strlen(text) < label_size)
    {
      strcpy(label,text);
      ptr = strlen(text);
      endptr = ptr;
      num_chars = ptr;
    }
}
void WinInput::KPressCallback(XEvent *event)
{
  int count;
  char *s;
  char *newlabel;
  int i;

  s = lookup_key(event,&count);
  if(num_chars + count + 1>= label_size)
    {
      newlabel = new char [label_size + 100];
      if(newlabel == NULL)
	{
	  cerr <<"Can't allocate buffer space\n";
	  return;
	}
      strcpy(newlabel, label);
      label_size = label_size + 100;
      delete label;
      label = newlabel;
    }
  for(i=0;i<count;i++)
    addchar(s[i],label,label_size,ptr,endptr);

  if((s[0] == '\n')||(s[0] == '\r'))
    {
      if(NewLineAction)
	NewLineAction(num_chars,label);
    }
  num_chars = strlen(label);
  RedrawWindow(1);
}

void WinInput::DrawCallback(XEvent *event)
{
  int xoff,yoff,twidth,twidth2;
  GC gc1;
  GC gc2;

 WinBase::DrawCallback(event);

  if(((!event)||(event->xexpose.count == 0))&&(label != NULL))
    {
      twidth = XTextWidth(Font,label,strlen(label));
      twidth2 = XTextWidth(Font,label,ptr);
      if(text_offset + twidth2 > w - bw - 4)
	{
	  text_offset = w - bw - 4 - twidth2;
	}
      if(text_offset + twidth2 < bw+4)
	{
	  text_offset += bw +4 - (text_offset + twidth2);
	}
      yoff = h - 1 - (h - Font->ascent - Font->descent)/2 -Font->descent;
      XDrawString(dpy,twin,ForeGC, text_offset-bw,yoff-bw,label,strlen(label));

      xoff = text_offset + XTextWidth(Font,label,ptr)-1;
      XDrawLine(dpy,twin,ForeGC,xoff-bw,yoff-Font->ascent-bw,
		xoff-bw,yoff+Font->descent - bw);
    }
}

/*  Convert the keypress event into a string.
 */
static char *lookup_key(XEvent *ev,int *pcount)
{
  KeySym keysym;
  static XComposeStatus compose = {NULL,0};
  int count;
  static char kbuf[KBUFSIZE];
  char *s, *c;
  int meta;
  int app_cur_keys = 0;
  int app_kp_keys = 0;

  count = XLookupString(&ev->xkey,kbuf,KBUFSIZE-1,&keysym,&compose);
  kbuf[count] = (unsigned char)0;
  meta = ev->xkey.state & Mod1Mask;
  s = NULL;
  
  switch(keysym)
    {
    case XK_Up :
      strcpy(kbuf,(app_cur_keys ? "\033OA" : "\033[A"));
      count = strlen(kbuf);
      break;
    case XK_Down :
      strcpy(kbuf,app_cur_keys ? "\033OB" : "\033[B");
      count = strlen(kbuf);
      break;
    case XK_Right :
      strcpy(kbuf,app_cur_keys ? "\033OC" : "\033[C");
      count = strlen(kbuf);
      break;
    case XK_Left :
      strcpy(kbuf,app_cur_keys ? "\033OD" : "\033[D");
      count = strlen(kbuf);
      break;
    case XK_KP_F1 :
      strcpy(kbuf,"\033OP");
      count = 3;
      break;
    case XK_KP_F2 :
      strcpy(kbuf,"\033OQ");
      count = 3;
      break;
    case XK_KP_F3 :
      strcpy(kbuf,"\033OR");
      count = 3;
      break;
    case XK_KP_F4 :
      strcpy(kbuf,"\033OS");
      count = 3;
      break;
    case XK_KP_0 :  
      strcpy(kbuf,app_kp_keys ? "\033Op" : "0");
      count = strlen(kbuf);
      break;
    case XK_KP_1 :
      strcpy(kbuf,app_kp_keys ? "\033Oq" : "1");
      count = strlen(kbuf);
      break;
    case XK_KP_2 :
      strcpy(kbuf,app_kp_keys ? "\033Or" : "2");
      count = strlen(kbuf);
      break;
    case XK_KP_3 :
      strcpy(kbuf,app_kp_keys ? "\033Os" : "3");
      count = strlen(kbuf);
      break;
    case XK_KP_4 :
      strcpy(kbuf,app_kp_keys ? "\033Ot" : "4");
      count = strlen(kbuf);
      break;
    case XK_KP_5 :
      strcpy(kbuf,app_kp_keys ? "\033Ou" : "5");
      count = strlen(kbuf);
      break;
    case XK_KP_6 :
      strcpy(kbuf,app_kp_keys ? "\033Ov" : "6");
      count = strlen(kbuf);
      break;
    case XK_KP_7 :
      strcpy(kbuf,app_kp_keys ? "\033Ow" : "7");
      count = strlen(kbuf);
      break;
    case XK_KP_8 :
      strcpy(kbuf,app_kp_keys ? "\033Ox" : "8");
      count = strlen(kbuf);
      break;
    case XK_KP_9 :
      strcpy(kbuf,app_kp_keys ? "\033Oy" : "9");
      count = strlen(kbuf);
      break;
    case XK_KP_Add:
      strcpy(kbuf,app_kp_keys ? "\033Ok" : "-");
      count = strlen(kbuf);
      break;
    case XK_KP_Subtract :
      strcpy(kbuf,app_kp_keys ? "\033Om" : "-");
      count = strlen(kbuf);
      break;
    case XK_KP_Multiply :
      strcpy(kbuf,app_kp_keys ? "\033Oj" : "-");
      count = strlen(kbuf);
      break;
    case XK_KP_Divide :
      strcpy(kbuf,app_kp_keys ? "\033Oo" : "-");
      count = strlen(kbuf);
      break;
    case XK_KP_Separator :
      strcpy(kbuf,app_kp_keys ? "\033Ol" : ",");
      count = strlen(kbuf);
      break;
    case XK_KP_Decimal :
      strcpy(kbuf,app_kp_keys ? "\033On" : ".");
      count = strlen(kbuf);
      break;
    case XK_KP_Enter :
      strcpy(kbuf,app_kp_keys ? "\033OM" : "\r");
      count = strlen(kbuf);
      break;
    case XK_Home :
      strcpy(kbuf,"\033[H");
      count = 3;
      break;
    case XK_End :
      strcpy(kbuf,"\033Ow");
      count = 3;
      break;
    case XK_F1 :
      strcpy(kbuf,"\033[11~");
      count = 5;
      break;
    case XK_F2 :
      strcpy(kbuf,"\033[12~");
      count = 5;
      break;
    case XK_F3 :
      strcpy(kbuf,"\033[13~");
      count = 5;
      break;
    case XK_F4 :
      strcpy(kbuf,"\033[14~");
      count = 5;
      break;
    case XK_F5 :
      strcpy(kbuf,"\033[15~");
      count = 5;
      break;
    case XK_F6 :
      strcpy(kbuf,"\033[17~");
      count = 5;
      break;
    case XK_F7 :
      strcpy(kbuf,"\033[18~");
      count = 5;
      break;
    case XK_F8 :
      strcpy(kbuf,"\033[19~");
      count = 5;
      break;
    case XK_F9 :
      strcpy(kbuf,"\033[20~");
      count = 5;
      break;
    case XK_F10 :
      strcpy(kbuf,"\033[21~");
      count = 5;
      break;
    case XK_F11 :
      strcpy(kbuf,"\033[23~");
      count = 5;
      break;
    case XK_F12 :
      strcpy(kbuf,"\033[24~");
      count = 5;
      break;
    case XK_F13 :
      strcpy(kbuf,"\033[25~");
      count = 5;
      break;
    case XK_F14 :
      strcpy(kbuf,"\033[26~");
      count = 5;
      break;
    case XK_Help:
    case XK_F15:
      strcpy(kbuf,"\033[28~");
      count = 5;
      break;
    case XK_Menu:
    case XK_F16:
      strcpy(kbuf,"\033[29~");
      count = 5;
      break;
    case XK_F17 :
      strcpy(kbuf,"\033[31~");
      count = 5;
      break;
    case XK_F18 :
      strcpy(kbuf,"\033[32~");
      count = 5;
      break;
    case XK_F19 :
      strcpy(kbuf,"\033[33~");
      count = 5;
      break;
    case XK_F20 :
      strcpy(kbuf,"\033[34~");
      count = 5;
      break;
    case XK_Find :
      strcpy(kbuf,"\033[1~");
      count = 4;
      break;
    case XK_Insert :
      strcpy(kbuf,"\033[2~");
      count = 4;
      break;
    case XK_Execute :
      strcpy(kbuf,"\033[3~");
      count = 4;
      break;
    case XK_Select :
      strcpy(kbuf,"\033[4~");
      count = 4;
      break;
    case XK_Prior :
      strcpy(kbuf,"\033[5~");
      count = 4;
      break;
    case XK_Next:
      strcpy(kbuf,"\033[6~");
      count = 4;
      break;
#ifdef BACKSPACE_SUCKS
    case XK_BackSpace:
      strcpy(kbuf,"\177");
      count = 1;
      break;
#endif
    }
  *pcount = count;

  if(meta &&(count > 0))
    {
      for(c = kbuf; c < kbuf+count ; c++)
	{
	  *c = *c | 0x80 ;
	}
      *(kbuf+count)=0;
    }
  else
    *(kbuf+count) = 0;
  return (kbuf);
}


void addchar(char c, char *buffer, int bufsize, int &ptr, int& endptr)
{
  int i;

  if((c!= '\n')&&(c!='\r')&&(c!=0)&&(ptr < (bufsize-1)))
    {
      if(((c=='\b')||(c=='\177'))&&(ptr > 0))
	{
	  ptr--;
	  endptr--;
	  for(i=ptr; i<endptr;i++)
	    buffer[i] = buffer[i+1];
	}
      /* ctrl-b  - go back one space*/
      else if((c=='b'-'a'+1)&&(ptr > 0))
	{
	  ptr--;
	}
      /* ctrl-d  - delete current char, or quit if ptr = endptr=0 */
      else if(c=='d'-'a'+1)
	{
	  if((ptr==endptr)&&(endptr == 0))
	    exit(0);
	  if(endptr > 0)
	    {
	      endptr--;
	      for(i=ptr; i<endptr;i++)
		buffer[i] = buffer[i+1];
	    }
	}
      /* ctrl-k  - delete current char, to end of line */
      else if(c=='k'-'a'+1)
	{
	  buffer[ptr] = 0;
	  endptr = ptr;
	}
      /* ctrl-f  - go forward one space*/
      else if((c=='f'-'a'+1)&&(ptr < endptr))
	{
	  ptr++;
	}
      /* ctrl-a  - go to start of line */
      else if(c=='a'-'a'+1)
	{
	  ptr=0;
	}
      /* ctrl-e  - go to end of line */
      else if(c=='e'-'a'+1)
	{
	  ptr=endptr;
	}
      else if((c >= ' ')&&(c <= '~'))/* Regular text */
	{
	  endptr++;
	  for(i=endptr; i>ptr;i--)
	    {
	      buffer[i] = buffer[i-1];
	    }
	  buffer[ptr] = c;
	  ptr++;
	}
      if(ptr > endptr)endptr = ptr;
    }
  buffer[endptr] = 0;
}
