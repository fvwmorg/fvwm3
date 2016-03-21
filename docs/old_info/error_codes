/**********************************************************************
 * 
 * This file contains the codes needed to descipher an Fvwm Internal
 * Error. This list is compiled from pieces of the X include files,
 * but is not actually used by the fvwm code. It is included for
 * debugging purposes.
 *********************************************************************/

/*************************************************************************
 * Request codes 
 * From Xproto.h
 *************************************************************************/

#define X_CreateWindow                  1              
#define X_ChangeWindowAttributes        2        
#define X_GetWindowAttributes           3     
#define X_DestroyWindow                 4
#define X_DestroySubwindows             5   
#define X_ChangeSaveSet                 6
#define X_ReparentWindow                7
#define X_MapWindow                     8
#define X_MapSubwindows                 9
#define X_UnmapWindow                  10
#define X_UnmapSubwindows              11  
#define X_ConfigureWindow              12  
#define X_CirculateWindow              13  
#define X_GetGeometry                  14
#define X_QueryTree                    15
#define X_InternAtom                   16
#define X_GetAtomName                  17
#define X_ChangeProperty               18 
#define X_DeleteProperty               19 
#define X_GetProperty                  20
#define X_ListProperties               21 
#define X_SetSelectionOwner            22    
#define X_GetSelectionOwner            23    
#define X_ConvertSelection             24   
#define X_SendEvent                    25
#define X_GrabPointer                  26
#define X_UngrabPointer                27
#define X_GrabButton                   28
#define X_UngrabButton                 29
#define X_ChangeActivePointerGrab      30          
#define X_GrabKeyboard                 31
#define X_UngrabKeyboard               32 
#define X_GrabKey                      33
#define X_UngrabKey                    34
#define X_AllowEvents                  35       
#define X_GrabServer                   36      
#define X_UngrabServer                 37        
#define X_QueryPointer                 38        
#define X_GetMotionEvents              39           
#define X_TranslateCoords              40                
#define X_WarpPointer                  41       
#define X_SetInputFocus                42         
#define X_GetInputFocus                43         
#define X_QueryKeymap                  44       
#define X_OpenFont                     45    
#define X_CloseFont                    46     
#define X_QueryFont                    47
#define X_QueryTextExtents             48     
#define X_ListFonts                    49  
#define X_ListFontsWithInfo    	       50 
#define X_SetFontPath                  51 
#define X_GetFontPath                  52 
#define X_CreatePixmap                 53        
#define X_FreePixmap                   54      
#define X_CreateGC                     55    
#define X_ChangeGC                     56    
#define X_CopyGC                       57  
#define X_SetDashes                    58     
#define X_SetClipRectangles            59             
#define X_FreeGC                       60  
#define X_ClearArea                    61             
#define X_CopyArea                     62    
#define X_CopyPlane                    63     
#define X_PolyPoint                    64     
#define X_PolyLine                     65    
#define X_PolySegment                  66       
#define X_PolyRectangle                67         
#define X_PolyArc                      68   
#define X_FillPoly                     69    
#define X_PolyFillRectangle            70             
#define X_PolyFillArc                  71       
#define X_PutImage                     72    
#define X_GetImage                     73 
#define X_PolyText8                    74     
#define X_PolyText16                   75      
#define X_ImageText8                   76      
#define X_ImageText16                  77       
#define X_CreateColormap               78          
#define X_FreeColormap                 79        
#define X_CopyColormapAndFree          80               
#define X_InstallColormap              81           
#define X_UninstallColormap            82             
#define X_ListInstalledColormaps       83                  
#define X_AllocColor                   84      
#define X_AllocNamedColor              85           
#define X_AllocColorCells              86           
#define X_AllocColorPlanes             87            
#define X_FreeColors                   88      
#define X_StoreColors                  89       
#define X_StoreNamedColor              90           
#define X_QueryColors                  91       
#define X_LookupColor                  92       
#define X_CreateCursor                 93        
#define X_CreateGlyphCursor            94             
#define X_FreeCursor                   95      
#define X_RecolorCursor                96         
#define X_QueryBestSize                97         
#define X_QueryExtension               98          
#define X_ListExtensions               99          
#define X_ChangeKeyboardMapping        100
#define X_GetKeyboardMapping           101
#define X_ChangeKeyboardControl        102                
#define X_GetKeyboardControl           103             
#define X_Bell                         104
#define X_ChangePointerControl         105
#define X_GetPointerControl            106
#define X_SetScreenSaver               107          
#define X_GetScreenSaver               108          
#define X_ChangeHosts                  109       
#define X_ListHosts                    110     
#define X_SetAccessControl             111               
#define X_SetCloseDownMode             112
#define X_KillClient                   113 
#define X_RotateProperties	       114
#define X_ForceScreenSaver	       115
#define X_SetPointerMapping            116
#define X_GetPointerMapping            117
#define X_SetModifierMapping	       118
#define X_GetModifierMapping	       119
#define X_NoOperation                  127

/*****************************************************************
 * ERROR CODES 
 * from X.h
 *****************************************************************/

#define Success		   0	/* everything's okay */
#define BadRequest	   1	/* bad request code */
#define BadValue	   2	/* int parameter out of range */
#define BadWindow	   3	/* parameter not a Window */
#define BadPixmap	   4	/* parameter not a Pixmap */
#define BadAtom		   5	/* parameter not an Atom */
#define BadCursor	   6	/* parameter not a Cursor */
#define BadFont		   7	/* parameter not a Font */
#define BadMatch	   8	/* parameter mismatch */
#define BadDrawable	   9	/* parameter not a Pixmap or Window */
#define BadAccess	  10	/* depending on context:
				 - key/button already grabbed
				 - attempt to free an illegal 
				   cmap entry 
				- attempt to store into a read-only 
				   color map entry.
 				- attempt to modify the access control
				   list from other than the local host.
				*/
#define BadAlloc	  11	/* insufficient resources */
#define BadColor	  12	/* no such colormap */
#define BadGC		  13	/* parameter not a GC */
#define BadIDChoice	  14	/* choice not in range or already used */
#define BadName		  15	/* font or color name doesn't exist */
#define BadLength	  16	/* Request length incorrect */
#define BadImplementation 17	/* server is defective */

#define FirstExtension Error	128
#define LastExtensionError	255


/*************************************************************************
 * Event Types
 * From X.h
 *************************************************************************/
#define KeyPress		2
#define KeyRelease		3
#define ButtonPress		4
#define ButtonRelease		5
#define MotionNotify		6
#define EnterNotify		7
#define LeaveNotify		8
#define FocusIn			9
#define FocusOut		10
#define KeymapNotify		11
#define Expose			12
#define GraphicsExpose		13
#define NoExpose		14
#define VisibilityNotify	15
#define CreateNotify		16
#define DestroyNotify		17
#define UnmapNotify		18
#define MapNotify		19
#define MapRequest		20
#define ReparentNotify		21
#define ConfigureNotify		22
#define ConfigureRequest	23
#define GravityNotify		24
#define ResizeRequest		25
#define CirculateNotify		26
#define CirculateRequest	27
#define PropertyNotify		28
#define SelectionClear		29
#define SelectionRequest	30
#define SelectionNotify		31
#define ColormapNotify		32
#define ClientMessage		33
#define MappingNotify		34
#define LASTEvent		35	/* must be bigger than any event # */

