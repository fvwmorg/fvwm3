\documentstyle[11pt]{article}    % Specifies the document style.
\setlength{\textwidth}{6in}
\setlength{\oddsidemargin}{.25in}
\setlength{\topmargin}{0in}
\setlength{\textheight}{8in}
\setcounter{secnumdepth}{2}

\begin{document}           % End of preamble and beginning of text.

\title {The Fvwm Module Interface}
\date{\today}
\author	{Robert J. Nation}
\maketitle

\section{Important Changes Since Fvwm-1.xx}
The module interface remained fairly stable when going from Fvwm-1.xx
to Fvwm-2.xx. The change most likely to affect modules is the addition
of an extra value to the header. This value is a timestamp. Most
modules contained the line:
\begin{verbatim}
	unsigned long header[3];
\end{verbatim}
This should be changed to:
\begin{verbatim}
	unsigned long header[HEADER_SIZE];
\end{verbatim}
The rest of the changes are handled in the library function
ReadFvwmPacket.

Another notable change is that modules do not have to read the .fvwmrc
file themselves. They can request the appropriate lines from fvwm:
\begin{verbatim}
	char *tline;

	GetConfigLine(fd, &tline);
	while(tline != NULL)
	  {
	    /* Parse the line as needed */

	    /* Get a new line */
           GetConfigLine(fd, &tline);	
	  }
\end{verbatim}
This will be the preferred method of getting module-specific
configuration lines in the future.

In fvwm-2.0 pl 1, I also had fvwm parse out the command line arguments
for the modules, so if the user said ``FvwmPager 2 4'', the module
will see argv[6] = 2 and argv[7] = 4, instead of argv[6] = ``2 4''.

Some new packet types were defined, and a few values were added to
some packet types.

\section{Concept}
The module interface had several design goals:
\begin{itemize}
\item{Modules (user programs) should be able to provide the window
manager with a limited amount of instruction regarding instructions to
execute.}
\item{Modules should not be able to corrupt the internal data bases
maintained by Fvwm, nor should unauthorized modules be able to
interface to Fvwm.}
\item{Modules should be able to extract all or nearly all information
held by the window manager, in a simple manner, to provide users with
feedback not available from built in services.}
\item{Modules should gracefully terminate when the window manager
dies, quits, or is re-started.}
\item{It should be possible for programmer-users to add modules
without understanding the internals of Fvwm. Ideally, modules could be
written in some scripting language such as Tcl/Tk.}
\end{itemize}
These goals have not entirely been met at this point, as the module
interface is a work in progress. 

\section{Implementation Overview}
Although the module interface is not complete, some details of its
implementation are available.
\subsection{Security}
Limited effort has been placed on security issues. In short, modules
can not communicate with Fvwm unless they are launched by Fvwm, which
means that they must be listed in the user's .fvwmrc file.
Modules can only issue commands to Fvwm that could be issued from a
menu or key binding. These measures do not keep a poorly written
module from destroying windows or terminating an X session, but they
do keep users from maliciously connecting to another users window
manager, and it should keep modules from corrupting the Fvwm internal 
databases.
\subsection{Interface and Initialization Mechanism}
Modules MUST be launched by Fvwm. Fvwm will first open a pair of pipes
for communication with the module. One pipe is for messages to Fvwm
from the module, and the other is for messages to the module from
Fvwm. Each module has its own pair of pipes. After the pipes are open,
Fvwm will fork and spawn the module. Modules must be located in the
ModulePath, as specified in the user's .fvwmrc file. Modules can be
initiated as fvwm starts up, or can be launched part way through an X
session.

The pipes already will be open when the module starts execution. The
integer file descriptor are passed to the module as the first and
second command line arguments. The third command line argument is the
full path name of the last configuration file that fvwm read in.
The next command line argument is the application
window in whose context the module was launched. This will be 0 if the
module is launched without an application window context. The next
argument is the context of the window decoration in which the module
was launched. Contexts are listed below:
\begin{verbatim}
#define C_NO_CONTEXT 	 0 - launched during initialization
#define C_WINDOW	 1 - launched from an application window
#define C_TITLE		 2 - launched from a title bar
#define C_ICON		 4 - launched from an icon window
#define C_ROOT		 8 - launched from the root window
#define C_FRAME		16 - launched from a corner piece
#define C_SIDEBAR       32 - launched from a side-bar
#define C_L1            64 - launched from left button #1
#define C_L2           128 - launched from left button #2
#define C_L3           256 - launched from left button #3
#define C_L4           512 - launched from left button #4
#define C_L5          1024 - launched from left button #5
#define C_R1          2048 - launched from right button #1
#define C_R2          4096 - launched from right button #2
#define C_R3          8192 - launched from right button #3
#define C_R4         16384 - launched from right button #4
#define C_R5         32768 - launched from right button #5
\end{verbatim}
A number of user specified command line arguments may be present
(optional). Up to 35 arguments may be passed. For example, in:
\begin{verbatim}
	Module	"FvwmIdentify"	FvwmIdentify Hello rob! ``-fg purple''
\end{verbatim}
we would get argv[6] = ``Hello'', argv[7] = ``rob!'', argv[8] = ``-fg purple''.

The following mechanism is recommended to acquire the pipe
descriptors:
\begin{verbatim}
int fd[2];
void main(int argc, char **argv)
{
  if((argc < 6))
    {	
      fprintf(stderr,"Modules should only be executed by fvwm!\n");
      exit(1);
    }
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

\end{verbatim}

The descriptor fd[0] is available for the module to send messages to
fvwm, while the descriptor fd[1] is available to read messages from
fvwm.

Special attention is paid to the status of the pipe. If Fvwm gets a
read error on a module-to-fvwm  pipe, then it assumes that the module 
is terminating,
and all communication with the module is terminated. Similarly, if a
module gets a read error on an fvwm-to-module pipe, then it should
assume that fvwm is terminating, and it should gracefully shut down.
All modules should also allow themselves to be shut down via the
Delete Window protocol for X-11.

In previous implementations, the modules were expected to read and
parse the .fvwmrc file by themselves. This caused some difficulty if a
pre-processor was used on the .fvwmrc file. In fvwm-2.0 and later,
fvwm retains the module command lines (those which start with a
``*''), and will pass them to any module on request. Modules can
request the command list by sending a ``Send\_ConfigInfo'' command to
fvwm.

\section{Module-to-Fvwm Communication}
Modules communicate with fvwm via a simple protocol. In essence, a
textual command line, similar to a command line which could be bound
to a mouse, or key-stroke in the .fvwmrc, is transmitted to fvwm.

First, the module should send the ID of the window which should be
manipulated. A window ID of ``None'' may be used, in which case Fvwm
will prompt the user to select  a window if needed. Next, length of
the the command line is send as an integer. After that, the command
line itself is sent. Finally, an integer 1 is sent if the module plans
to continue operating, or 0 if the module is finished. The following
subroutine is provided as an example of a suitable method of sending
messages to fvwm:
\begin{verbatim}
void SendInfo(int *fd,Window win, char *message)
{
  int w;

  if((message != NULL)&&(strlen(message) >0))
    {	
      /* if win == None, then Fvwm will prompt the user for a window
       * on which to operate, if needed. Some commands, like Exec,
       * don't operate on a window, so None is appropriate. */
       write(fd[0],&win,sizeof(Window));
      
      /* calculate the length of the message */
      w=strlen(message);
      /* send the message length */
      write(fd[0],&w,sizeof(int));
      /* send the message itself
      write(fd[0],message,w);

      /* send a 1, indicating that this module will keep going */
      /* a 0 would mean that this module is done */
      w=1;
      write(fd[0],&w,sizeof(int));
    }
}
\end{verbatim}
This routine is available as a library function in libfvwmlib.

There are special built-in functions, Send\_WindowList and
Send\_ConfigInfo.
Send\_WindowList causes fvwm to transmit everything that it is
currently thinking about to the module which requests the information.
This information contains the paging status (enabled/disabled),
current desktop number, position on the desktop, current focus and,
for each window, the window configuration, window, icon, and class
names, and, if the window is iconified, the icon location and size.
Send\_ConfigInfo causes fvwm to send the module a list of all
commands it has received which start with a ``*'', as will as the
pixmap-path and icon-paths that fvwm is using.

An additional special function has been added to fvwm, which is used
to control exactly what information fvwm passes to each module. The
command Set\_Mask, followed by a number which is the bitwise OR of the
packet-type values which the module wishes to receive. If the module
never sends the Set\_Mask command, then all message types will be
sent. A library function, SetMessageMask, exists to make it easy to
set this mask. This mask should be used to reduce the amount of
communication between fvwm and its modules to a minimum.

\section{Fvwm-to-Module Communication}
Fvwm
can send messages to the modules in either a broadcast mode, or a
module specific mode. Certain messages regarding important window or
desktop manipulations will be broadcast to all modules, whether they 
want it or not. Modules are able to request information about current windows
from fvwm, via the Send\_WindowList built-in. When invoked this way,
only requesting module will receive the data.

Packets from fvwm to modules conform to a standard format, so modules
which are not interested in broadcast messages can easily ignore them.
A header consisting of 4 unsigned long integers, followed by a body of
a variable length make up a packet. The header always begins with
0xffffffff. This is provided to help modules re-synchronize to the
data stream if necessary. The next entry describes the packet type.
Existing packet types are listing in the file module.h:
\begin{verbatim}
#define M_NEW_PAGE           (1)
#define M_NEW_DESK           (1<<1)
#define M_ADD_WINDOW         (1<<2)
#define M_RAISE_WINDOW       (1<<3)
#define M_LOWER_WINDOW       (1<<4)
#define M_CONFIGURE_WINDOW   (1<<5)
#define M_FOCUS_CHANGE       (1<<6)
#define M_DESTROY_WINDOW     (1<<7)
#define M_ICONIFY            (1<<8)
#define M_DEICONIFY          (1<<9)
#define M_WINDOW_NAME        (1<<10)
#define M_ICON_NAME          (1<<11)
#define M_RES_CLASS          (1<<12)
#define M_RES_NAME           (1<<13)
#define M_END_WINDOWLIST     (1<<14)
#define M_ICON_LOCATION      (1<<15)
#define M_MAP                (1<<16)
#define M_ERROR              (1<<17)
#define M_CONFIG_INFO        (1<<18)
#define M_END_CONFIG_INFO    (1<<19)
\end{verbatim}

Additional packet types will be defined in the future. The third entry
in the header tells the total length of the packet, in unsigned longs,
including the header. The fourth entry is the last time stamp received
from the X server, which is expressed in milliseconds.

The body information is packet specific, as described below.
\subsection{M\_NEW\_PAGE}
These packets contain 5 integers. The first two are the x and y
coordinates of the upper left corner of the current viewport on the
virtual desktop. The third value is the number of the current desktop.
The fourth and fifth values are the maximum allowed values of the
coordinates of the upper-left hand corner of the viewport.

\subsection{M\_NEW\_DESK}
The body of this packet consists of a single long integer, whose value
is the number of the currently active desktop. This packet is
transmitted whenever the desktop number is changed.

\subsection{M\_ADD\_WINDOW, and M\_CONFIGURE\_WINDOW}
These packets contain 24 values. The first 3 identify the window, and
the next twelve identify the location and size, as described in the
table below. Configure packets will be generated when the
viewport on the current desktop changes, or when the size or location
of the window is changed. The flags field is an bitwise OR of the
flags defined in fvwm.h.


\begin{table}
\begin{center}
\begin{tabular}[h]{|l|l|} \hline
\multicolumn{2}{|c|}{Format for Add and Configure Window Packets} \\ \hline
Byte &Significance \\\hline
0    & 0xffffffff - Start of packet \\
1    & packet type \\
2    & length of packet, including header, expressed in long integers
\\ \hline
3    & ID of the application's top level window \\
4    & ID of the Fvwm frame window \\
5    & Pointer to the Fvwm database entry \\
6    & X location of the window's frame \\
7    & Y location of the window's frame\\
8    & Width of the window's frame (pixels) \\
9    & Height of the window's frame (pixels) \\
10   & Desktop number\\ 
11   & Windows flags field\\
12   & Window Title Height (pixels) \\
13   & Window Border Width (pixels) \\
14   & Window Base Width (pixels) \\  
15   & Window Base Height (pixels) \\  
16   & Window Resize Width Increment(pixels) \\  
17   & Window Resize Height Increment (pixels) \\  
18   & Window Minimum Width (pixels) \\  
19   & Window Minimum Height (pixels) \\  
20   & Window Maximum Width Increment(pixels) \\  
21   & Window Maximum Height Increment (pixels) \\
22   & Icon Label Window ID, or 0\\
23   & Icon Pixmap Window ID, or 0\\ 
24   & Window Gravity\\  \
25   & Pixel value of the text color \\
26   & Pixel value of the window border color \\ \hline
\end{tabular}
\end{center}
\end{table}


\subsection{M\_LOWER\_WINDOW, M\_RAISE\_WINDOW, and M\_DESTROY}
These packets contain 3 values, all of the same size as an unsigned
long. The first value is the ID of the affected application's top level
window, the next is the ID of the Fvwm frame window, and the final
value is the pointer to Fvwm's internal database entry for that
window. Although the pointer itself is of no use to a module, it can
be used as a reference number when referring to the window.

\begin{table}
\begin{center}
\begin{tabular}[h]{|l|l|} \hline
\multicolumn{2}{|c|}{Format for Lower, Raise, and Focus Change Packets} \\ \hline
Byte &Significance \\\hline
0    & 0xffffffff - Start of packet \\
1    & packet type \\
2    & length of packet, including header, expressed in long integers
\\ \hline
3    & ID of the application's top level window \\
4    & ID of the Fvwm frame window \\
5    & Pointer to the Fvwm database entry \\ \hline
\end{tabular}
\end{center}
\end{table}


\subsection{M\_FOCUS\_CHANGE}
These packets contain 5 values, all of the same size as an unsigned
long. The first value is the ID of the affected application's (the
application which now has the input focus) top level
window, the next is the ID of the Fvwm frame window, and the final
value is the pointer to Fvwm's internal database entry for that
window. Although the pointer itself is of no use to a module, it can
be used as a reference number when referring to the window. The fourth
and fifth values are the text focus color's pixel value and the window
border's focus color's pixel value. In the
event that the window which now has the focus is not a window which
fvwm recognizes, only the ID of the affected application's top level
window is passed. Zeros are passed for the other values.

\subsection{M\_ICONIFY and M\_ICON\_LOCATION}
These packets contain 7 values. The first 3 are the usual identifiers,
and the next four describe the location and size of the icon window,
as described in the table. Note that M\_ICONIFY packets will be sent
whenever a window is first iconified, or when the icon window is changed
via the XA\_WM\_HINTS in a property notify event. An M\_ICON\_LOCATION
packet will be sent when the icon is moved.
In addition, if a window which has transients is
iconified, then an M\_ICONIFY packet is sent for each transient
window, with the x, y, width, and height fields set to 0. This packet
will be sent even if the transients were already iconified. Note that
no icons are actually generated for the transients in this case.


\begin{table}
\begin{center}
\begin{tabular}[h]{|l|l|} \hline
\multicolumn{2}{|c|}{Format for Iconify and Icon Location Packets} \\ \hline
Byte &Significance \\\hline
0    & 0xffffffff - Start of packet \\
1    & packet type \\
2    & length of packet, including header, expressed in long integers
\\ \hline
3    & ID of the application's top level window \\
4    & ID of the Fvwm frame window \\
5    & Pointer to the Fvwm database entry \\
6    & X location of the icon's frame \\
7    & Y location of the icon's frame\\
8    & Width of the icon's frame \\
9    & Height of the icon's frame \\ \hline
\end{tabular}
\end{center}
\end{table}

\subsection{M\_DEICONIFY}
These packets contain 3 values, which are the usual window
identifiers. The packet is sent when a window is de-iconified.

\subsection{M\_MAP}
These packets contain 3 values, which are the usual window
identifiers. The packets are sent when a window is mapped, if it is
not being deiconified. This is useful to determine when a window is
finally mapped, after being added.

\subsection{M\_WINDOW\_NAME, M\_ICON\_NAME, M\_RES\_CLASS, M\_RES\_NAME}
These packets contain 3 values, which are the usual window
identifiers, followed by a variable length character string. The
packet size field in the header is expressed in units of unsigned
longs, and the packet is zero-padded until it is the  size of an
unsigned long. The RES\_CLASS and RES\_NAME fields are fields in the
XClass structure for the window. Icon and Window name packets will
will be sent upon window creation or whenever the name is changed. The
RES\_CLASS and RES\_NAME packets are sent on window creation. All
packets are sent in response to a Send\_WindowList request from a module.

\subsection{M\_END\_WINDOWLIST}
These packets contain no values. This packet is sent to mark the end
of transmission in response to  a Send\_WindowList request. A module
which request Send\_WindowList, and processes all packets received
between the request and the M\_END\_WINDOWLIST will have a snapshot of
the status of the desktop.

\subsection{M\_ERROR}
When fvwm has an error message to report, it is echoed to the modules
in a packet of this type. This packet has 3 values, all zero, followed
by a variable length string which contains the error message.

\subsection{M\_CONFIG\_INFO}
Fvwm records all configuration commands that it encounters which
begins with the character ``*''. When the built-in command
``Send\_ConfigInfo'' is invoked by a module, this entire list is
transmitted to the module in packets (one line per packet) of this
type. The packet consists of three zeros, followed by a variable
length character string. In addition, the PixmapPath, IconPath, and
ClickTime  are sent to the module.

\subsection{M\_END\_CONFIG\_INFO}
After fvwm sends all of its M\_CONFIG\_INFO packets to a module, it
sends a packet of this type to indicate the end of the configuration
information. This packet contains no values.

\end{document}



