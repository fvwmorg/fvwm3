BaseDirFix = <Base dir path>              ! Ex Device:[Path]
BaseDir = <Base dir path, pseudo device>  ! Ex Device:[Path.]
XpmInclude = <XPM include files path>     ! If needed
XpmLib = <XPM object library path>        ! If needed
CCCommand = Cc/Pref=All/Nested_Include_Directory=Primary_File/Standard=Relaxed_Ansi89

.First
  @ If F$Search("[.Libs]Libs.Olb") .Eqs. "" Then Write Sys$Output "Creating library [.Libs]Libs.Olb..."
  @ If F$Search("[.Libs]Libs.Olb") .Eqs. "" Then Lib/Create/Obj [.Libs]Libs.Olb
  @ If F$Search("[.Fvwm]Fvwm.Olb") .Eqs. "" Then Write Sys$Output "Creating library [.Fvwm]Fvwm.Olb..."
  @ If F$Search("[.Fvwm]Fvwm.Olb") .Eqs. "" Then Lib/Create/Obj [.Fvwm]Fvwm.Olb
  @ If F$Search("[.Modules]Bin.Dir") .Eqs. "" Then Write Sys$Output "Creating directory [.Modules]Bin.Dir..."
  @ If F$Search("[.Modules]Bin.Dir") .Eqs. "" Then Cre/Dir [.Modules.Bin]
  @ Write Sys$Output "Redefining logical X11..."
  @ Assign/Job DECW$INCLUDE,$(XpmInclude) X11
  @ Assign/Job $(BaseDir)[Libs] Libs
  @ Assign/Job $(BaseDir)[Fvwm] Fvwm

Fvwm : [.Fvwm]Fvwm.Exe, -
       [.Modules.Bin]FvwmWinList.Exe, -
       [.Modules.Bin]FvwmAuto.Exe, -
       [.Modules.Bin]FvwmPager.Exe, -
       [.Modules.Bin]FvwmIdent.Exe, -
       [.Modules.Bin]FvwmForm.Exe, -
       [.Modules.Bin]FvwmIconMan.Exe, -
       [.Modules.Bin]FvwmButtons.Exe, -
       [.Modules.Bin]FvwmTalk.Exe, -
       [.Modules.Bin]FvwmEvent.Exe, -
       [.Modules.Bin]FvwmRearrange.Exe, -
       [.Modules.Bin]FvwmAnimate.Exe
  @ Continue

[.Fvwm]Fvwm.Exe : [.Libs]Libs.Olb, [.Fvwm]Fvwm.Olb
  @ Write Sys$Output "Creating image $(MMS$TargetName)..."
  @ Link/Exe=[.Fvwm]Fvwm.Exe $(BaseDir)[Fvwm]Fvwm.Olb/Lib/Inc=Fvwm, -
                             $(BaseDir)[Libs]Libs.Olb/Lib, -
                             $(XpmLib)/Lib, -
                             $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Libs]Libs.Olb : [.Libs]Vms.Obj, [.Libs]CLIENTMSG.Obj, [.Libs]COLORUTILS.Obj, -
                  [.Libs]DEBUG.Obj, [.Libs]ENVVAR.Obj, [.Libs]GETFONT.Obj, [.Libs]GETHOSTNAME.Obj, [.Libs]GRAB.Obj, -
                  [.Libs]MODPARSE.Obj, [.Libs]MODULE.Obj, [.Libs]PARSE.Obj, [.Libs]PICTURE.Obj, [.Libs]SAFEMALLOC.Obj, -
                  [.Libs]STRINGS.Obj, [.Libs]SYSTEM.Obj, [.Libs]WILD.Obj, [.Libs]XRESOURCE.Obj, [.Libs]Graphics.Obj

  @ Continue

!-----------------------------------------------------------------------------------------------------------------------------------
!    Directory [.Libs]
!-----------------------------------------------------------------------------------------------------------------------------------

[.Libs]Vms.Obj : [.Libs]Vms.C, Config.h, [.Libs]Vms.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000], $(BaseDir)[fvwm], $(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]CLIENTMSG.Obj : [.Libs]CLIENTMSG.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]COLORUTILS.Obj : [.Libs]COLORUTILS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]DEBUG.Obj : [.Libs]DEBUG.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]ENVVAR.Obj : [.Libs]ENVVAR.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]GETFONT.Obj : [.Libs]GETFONT.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]GETHOSTNAME.Obj : [.Libs]GETHOSTNAME.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]GRAB.Obj : [.Libs]GRAB.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]MODPARSE.Obj : [.Libs]MODPARSE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]MODULE.Obj : [.Libs]MODULE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]PARSE.Obj : [.Libs]PARSE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]PICTURE.Obj : [.Libs]PICTURE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]SAFEMALLOC.Obj : [.Libs]SAFEMALLOC.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]STRINGS.Obj : [.Libs]STRINGS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]SYSTEM.Obj : [.Libs]SYSTEM.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]WILD.Obj : [.Libs]WILD.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]XRESOURCE.Obj : [.Libs]XRESOURCE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

[.Libs]Graphics.Obj : [.Libs]Graphics.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ $(CCCommand) /Obj=[.Libs] /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) $(MMS$Source)
  @ Lib/Rep/Obj [.Libs]Libs.Olb $(MMS$Target)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Directory [.Fvwm]
!-----------------------------------------------------------------------------------------------------------------------------------

[.Fvwm]Fvwm.Olb : [.Fvwm]FVWM.Obj, [.Fvwm]ADD_WINDOW.Obj, [.Fvwm]BINDINGS.Obj, [.Fvwm]BORDERS.Obj, [.Fvwm]BUILTINS.Obj, -
                  [.Fvwm]COLORMAPS.Obj, [.Fvwm]COLORS.Obj, [.Fvwm]DECORATIONS.Obj, [.Fvwm]EVENTS.Obj, -
                  [.Fvwm]FOCUS.Obj, [.Fvwm]FUNCTIONS.Obj, [.Fvwm]ICONS.Obj, [.Fvwm]MENUS.Obj, [.Fvwm]MISC.Obj, -
                  [.Fvwm]MODCONF.Obj, [.Fvwm]MODULE.Obj, [.Fvwm]MOVE.Obj, [.Fvwm]PLACEMENT.Obj, [.Fvwm]READ.Obj, -
                  [.Fvwm]RESIZE.Obj, [.Fvwm]STYLE.Obj, [.Fvwm]VIRTUAL.Obj, [.Fvwm]WINDOWS.Obj, [.Fvwm]IcccM2.Obj
  @ Continue

[.Fvwm]FVWM.Obj : [.Fvwm]FVWM.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FVWM.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]ADD_WINDOW.Obj : [.Fvwm]ADD_WINDOW.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) ADD_WINDOW.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]BINDINGS.Obj : [.Fvwm]BINDINGS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) BINDINGS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]BORDERS.Obj : [.Fvwm]BORDERS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) BORDERS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]BUILTINS.Obj : [.Fvwm]BUILTINS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) BUILTINS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]COLORMAPS.Obj : [.Fvwm]COLORMAPS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) COLORMAPS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]COLORS.Obj : [.Fvwm]COLORS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) COLORS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]DECORATIONS.Obj : [.Fvwm]DECORATIONS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) DECORATIONS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]EVENTS.Obj : [.Fvwm]EVENTS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) EVENTS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]FOCUS.Obj : [.Fvwm]FOCUS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FOCUS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]FUNCTIONS.Obj : [.Fvwm]FUNCTIONS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FUNCTIONS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]ICONS.Obj : [.Fvwm]ICONS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) ICONS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]MENUS.Obj : [.Fvwm]MENUS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) MENUS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]MISC.Obj : [.Fvwm]MISC.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) MISC.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]MODCONF.Obj : [.Fvwm]MODCONF.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) MODCONF.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]MODULE.Obj : [.Fvwm]MODULE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) MODULE.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]MOVE.Obj : [.Fvwm]MOVE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) MOVE.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]PLACEMENT.Obj : [.Fvwm]PLACEMENT.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) PLACEMENT.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]READ.Obj : [.Fvwm]READ.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) READ.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]RESIZE.Obj : [.Fvwm]RESIZE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) RESIZE.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]STYLE.Obj : [.Fvwm]STYLE.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) STYLE.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]VIRTUAL.Obj : [.Fvwm]VIRTUAL.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) VIRTUAL.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]WINDOWS.Obj : [.Fvwm]WINDOWS.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) WINDOWS.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

[.Fvwm]ICCCM2.Obj : [.Fvwm]ICCCM2.C, Config.h
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Fvwm]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) ICCCM2.C
  @ Set Def $(BaseDirFix)
  @ Lib/Rep/Obj [.Fvwm]Fvwm.Olb $(MMS$Target)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmWinList
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmWinList.Exe : [.Modules.FvwmWinList]BUTTONARRAY.Obj, [.Modules.FvwmWinList]COLORS.Obj, -
                                [.Modules.FvwmWinList]FVWMWINLIST.Obj, [.Modules.FvwmWinList]LIST.Obj, -
                                [.Modules.FvwmWinList]MALLOCS.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmWinList]BUTTONARRAY.Obj, [.Modules.FvwmWinList]COLORS.Obj, -
                           [.Modules.FvwmWinList]FVWMWINLIST.Obj, [.Modules.FvwmWinList]LIST.Obj, -
                           [.Modules.FvwmWinList]MALLOCS.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(XpmLib)/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmWinList]BUTTONARRAY.Obj : [.Modules.FvwmWinList]BUTTONARRAY.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmWinList]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) BUTTONARRAY.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmWinList]COLORS.Obj : [.Modules.FvwmWinList]COLORS.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmWinList]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) COLORS.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmWinList]FVWMWINLIST.Obj : [.Modules.FvwmWinList]FVWMWINLIST.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmWinList]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FVWMWINLIST.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmWinList]LIST.Obj : [.Modules.FvwmWinList]LIST.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmWinList]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) LIST.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmWinList]MALLOCS.Obj : [.Modules.FvwmWinList]MALLOCS.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmWinList]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) MALLOCS.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmAuto
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmAuto.Exe : [.Modules.FvwmAuto]FvwmAuto.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmAuto]FvwmAuto.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmAuto]FvwmAuto.Obj : [.Modules.FvwmAuto]FvwmAuto.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmAuto]
  @ $(CCCommand) /Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FvwmAuto.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmPager
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmPager.Exe : [.Modules.FvwmPager]FvwmPager.Obj, [.Modules.FvwmPager]X_Pager.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmPager]FvwmPager.Obj, [.Modules.FvwmPager]X_Pager.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(XpmLib)/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmPager]FvwmPager.Obj : [.Modules.FvwmPager]FvwmPager.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmPager]
  @ $(CCCommand) /Obj=FvwmPager.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FvwmPager.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmPager]X_Pager.Obj : [.Modules.FvwmPager]X_Pager.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmPager]
  @ $(CCCommand) /Obj=X_Pager.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) X_Pager.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmIdent
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmIdent.Exe : [.Modules.FvwmIdent]FvwmIdent.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmIdent]FvwmIdent.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmIdent]FvwmIdent.Obj : [.Modules.FvwmIdent]FvwmIdent.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIdent]
  @ $(CCCommand) /Obj=FvwmIdent.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FvwmIdent.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmForm
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmForm.Exe : [.Modules.FvwmForm]FvwmForm.Obj, [.Modules.FvwmForm]DefineMe.Obj, -
                             [.Modules.FvwmForm]ParseCommand.Obj, [.Modules.FvwmForm]ReadXServer.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmForm]FvwmForm.Obj, -
                           [.Modules.FvwmForm]DefineMe.Obj, -
                           [.Modules.FvwmForm]ParseCommand.Obj, -
                           [.Modules.FvwmForm]ReadXServer.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(XpmLib)/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmForm]FvwmForm.Obj : [.Modules.FvwmForm]FvwmForm.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmForm]
  @ $(CCCommand) /Obj=FvwmForm.Obj /inc=([],$(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FvwmForm.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmForm]DefineMe.Obj : [.Modules.FvwmForm]DefineMe.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmForm]
  @ $(CCCommand) /Obj=DefineMe.Obj /inc=([],$(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) DefineMe.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmForm]ParseCommand.Obj : [.Modules.FvwmForm]ParseCommand.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmForm]
  @ $(CCCommand) /Obj=ParseCommand.Obj /inc=([],$(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) ParseCommand.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmForm]ReadXServer.Obj : [.Modules.FvwmForm]ReadXServer.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmForm]
  @ $(CCCommand) /Obj=ReadXServer.Obj /inc=([],$(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) ReadXServer.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmIconMan
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmIconMan.Exe : [.Modules.FvwmIconMan]FvwmIconMan.Obj, [.Modules.FvwmIconMan]Debug.Obj, -
                                [.Modules.FvwmIconMan]Functions.Obj, [.Modules.FvwmIconMan]Fvwm.Obj, -
                                [.Modules.FvwmIconMan]Globals.Obj, [.Modules.FvwmIconMan]ReadConfig.Obj, -
                                [.Modules.FvwmIconMan]WinList.Obj, [.Modules.FvwmIconMan]X.Obj, -
                                [.Modules.FvwmIconMan]XManager.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmIconMan]FvwmIconMan.Obj, [.Modules.FvwmIconMan]Debug.Obj, -
                           [.Modules.FvwmIconMan]Functions.Obj, [.Modules.FvwmIconMan]Fvwm.Obj, -
                           [.Modules.FvwmIconMan]Globals.Obj, [.Modules.FvwmIconMan]ReadConfig.Obj, -
                           [.Modules.FvwmIconMan]WinList.Obj, [.Modules.FvwmIconMan]X.Obj, -
                           [.Modules.FvwmIconMan]XManager.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(XpmLib)/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmIconMan]FvwmIconMan.Obj : [.Modules.FvwmIconMan]FvwmIconMan.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=FvwmIconMan.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 FvwmIconMan.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconMan]Debug.Obj : [.Modules.FvwmIconMan]Debug.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=Debug.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 Debug.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconMan]Functions.Obj : [.Modules.FvwmIconMan]Functions.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=Functions.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 Functions.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconMan]Fvwm.Obj : [.Modules.FvwmIconMan]Fvwm.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=Fvwm.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 Fvwm.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconMan]Globals.Obj : [.Modules.FvwmIconMan]Globals.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=Globals.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 Globals.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconMan]ReadConfig.Obj : [.Modules.FvwmIconMan]ReadConfig.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=ReadConfig.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 ReadConfig.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconMan]WinList.Obj : [.Modules.FvwmIconMan]WinList.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=WinList.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 WinList.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconMan]X.Obj : [.Modules.FvwmIconMan]X.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=X.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 X.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconMan]XManager.Obj : [.Modules.FvwmIconMan]XManager.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconMan]
  @ $(CCCommand) /Obj=XManager.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[Modules.FvwmIconMan]) -
                 XManager.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmButtons
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmButtons.Exe : [.Modules.FvwmButtons]FvwmButtons.Obj, [.Modules.FvwmButtons]BUTTON.Obj, -
                                [.Modules.FvwmButtons]DRAW.Obj, [.Modules.FvwmButtons]ICONS.Obj, -
                                [.Modules.FvwmButtons]MISC.Obj, [.Modules.FvwmButtons]OUTPUT.Obj, -
                                [.Modules.FvwmButtons]PARSE.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmButtons]FvwmButtons.Obj, [.Modules.FvwmButtons]BUTTON.Obj, -
                           [.Modules.FvwmButtons]DRAW.Obj, [.Modules.FvwmButtons]ICONS.Obj, -
                           [.Modules.FvwmButtons]MISC.Obj, [.Modules.FvwmButtons]OUTPUT.Obj, -
                           [.Modules.FvwmButtons]PARSE.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(XpmLib)/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmButtons]FvwmButtons.Obj : [.Modules.FvwmButtons]FvwmButtons.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmButtons]
  @ $(CCCommand) /Obj=FvwmButtons.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) -
                 FvwmButtons.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmButtons]Button.Obj : [.Modules.FvwmButtons]Button.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmButtons]
  @ $(CCCommand) /Obj=Button.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) Button.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmButtons]Draw.Obj : [.Modules.FvwmButtons]Draw.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmButtons]
  @ $(CCCommand) /Obj=Draw.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) Draw.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmButtons]Icons.Obj : [.Modules.FvwmButtons]Icons.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmButtons]
  @ $(CCCommand) /Obj=Icons.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) Icons.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmButtons]Misc.Obj : [.Modules.FvwmButtons]Misc.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmButtons]
  @ $(CCCommand) /Obj=Misc.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) Misc.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmButtons]Output.Obj : [.Modules.FvwmButtons]Output.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmButtons]
  @ $(CCCommand) /Obj=Output.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) Output.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmButtons]Parse.Obj : [.Modules.FvwmButtons]Parse.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmButtons]
  @ $(CCCommand) /Obj=Parse.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) Parse.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmTalk
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmTalk.Exe : [.Modules.FvwmTalk]FvwmTalk.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmTalk]FvwmTalk.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmTalk]FvwmTalk.Obj : [.Modules.FvwmTalk]FvwmTalk.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmTalk]
  @ $(CCCommand) /Obj=FvwmTalk.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FvwmTalk.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmEvent
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmEvent.Exe : [.Modules.FvwmEvent]FvwmEvent.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmEvent]FvwmEvent.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmEvent]FvwmEvent.Obj : [.Modules.FvwmEvent]FvwmEvent.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmEvent]
  @ $(CCCommand) /Obj=FvwmEvent.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FvwmEvent.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmRearrange
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmRearrange.Exe : [.Modules.FvwmRearrange]FvwmRearrange.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmRearrange]FvwmRearrange.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmRearrange]FvwmRearrange.Obj : [.Modules.FvwmRearrange]FvwmRearrange.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmRearrange]
  @ $(CCCommand) /Obj=FvwmRearrange.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) -
                 FvwmRearrange.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmAnimate
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmAnimate.Exe : [.Modules.FvwmAnimate]FvwmAnimate.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmAnimate]FvwmAnimate.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmAnimate]FvwmAnimate.Obj : [.Modules.FvwmAnimate]FvwmAnimate.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmAnimate]
  @ $(CCCommand) /Obj=FvwmAnimate.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) -
                 FvwmAnimate.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmIconBox
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmIconBox.Exe : [.Modules.FvwmIconBox]FvwmIconBox.Obj, [.Modules.FvwmIconBox]Icons.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmIconBox]FvwmIconBox.Obj, -
                           [.Modules.FvwmIconBox]Icons.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmIconBox]FvwmIconBox.Obj : [.Modules.FvwmIconBox]FvwmIconBox.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconBox]
  @ $(CCCommand) /Obj=FvwmIconBox.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[modules.FvwmIconBox]) -
                 FvwmIconBox.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmIconBox]Icons.Obj : [.Modules.FvwmIconBox]Icons.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmIconBox]
  @ $(CCCommand) /Obj=Icons.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[modules.FvwmIconBox]) -
                 Icons.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmBacker
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmBacker.Exe : [.Modules.FvwmBacker]FvwmBacker.Obj, -
                               [.Modules.FvwmBacker]Mallocs.Obj, -
                               [.Modules.FvwmBacker]RootBits.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmBacker]FvwmBacker.Obj, -
                           [.Modules.FvwmBacker]Mallocs.Obj, -
                           [.Modules.FvwmBacker]RootBits.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmBacker]FvwmBacker.Obj : [.Modules.FvwmBacker]FvwmBacker.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmBacker]
  @ $(CCCommand) /Obj=FvwmBacker.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs], $(BaseDir)[.Modules.FvwmBacker]) -
                 FvwmBacker.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmBacker]Mallocs.Obj : [.Modules.FvwmBacker]Mallocs.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmBacker]
  @ $(CCCommand) /Obj=Mallocs.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs], $(BaseDir)[.Modules.FvwmBacker]) -
                 Mallocs.C
  @ Set Def $(BaseDirFix)

[.Modules.FvwmBacker]Root_Bits.Obj : [.Modules.FvwmBacker]Root_Bits.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmBacker]
  @ $(CCCommand) /Obj=Root_Bits.Obj -
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs], $(BaseDir)[.Modules.FvwmBacker]) -
                 Root_Bits.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    Module FvwmBanner
!-----------------------------------------------------------------------------------------------------------------------------------
[.Modules.Bin]FvwmBanner.Exe : [.Modules.FvwmBanner]FvwmBanner.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Modules.FvwmBanner]FvwmBanner.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Modules.FvwmBanner]FvwmBanner.Obj : [.Modules.FvwmBanner]FvwmBanner.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Modules.FvwmBanner]
  @ $(CCCommand) /Obj=FvwmBanner.Obj /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs]) FvwmBanner.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    extra FvwmCommand
!-----------------------------------------------------------------------------------------------------------------------------------
[.Extras.Bin]FvwmCommand.Exe : [.Extras.FvwmCommand]FvwmCommand.Obj, [.Extras.FvwmCommand]FvwmCommandS.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Extras.FvwmCommand]FvwmCommand.Obj, -
                           [.Extras.FvwmCommand]FvwmCommandS.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Extras.FvwmCommand]FvwmCommand.Obj : [.Extras.FvwmCommand]FvwmCommand.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmCommand]
  @ $(CCCommand) /Obj=FvwmCommand.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmCommand]) -
                 FvwmCommand.C
  @ Set Def $(BaseDirFix)

[.Extras.FvwmCommand]FvwmCommands.Obj : [.Extras.FvwmCommand]FvwmCommands.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmCommand]
  @ $(CCCommand) /Obj=FvwmCommands.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmCommand]) -
                 FvwmCommands.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    extra FvwmConsole
!-----------------------------------------------------------------------------------------------------------------------------------
[.Extras.Bin]FvwmConsole.Exe : [.Extras.FvwmConsole]FvwmConsole.Obj, -
                               [.Extras.FvwmConsole]FvwmConsoleC.Obj, -
                               [.Extras.FvwmConsole]GetLine.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Extras.FvwmConsole]FvwmConsole.Obj, -
                           [.Extras.FvwmConsole]FvwmConsoleC.Obj, -
                           [.Extras.FvwmConsole]GetLine.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Extras.FvwmConsole]FvwmConsole.Obj : [.Extras.FvwmConsole]FvwmConsole.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmConsole]
  @ $(CCConsole) /Obj=FvwmConsole.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmConsole]) -
                 FvwmConsole.C
  @ Set Def $(BaseDirFix)

[.Extras.FvwmConsole]FvwmConsoleC.Obj : [.Extras.FvwmConsole]FvwmConsoleC.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmCommand]
  @ $(CCConsoleC) /Obj=FvwmConsoleC.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmConsole]) -
                 FvwmConsoleC.C
  @ Set Def $(BaseDirFix)

[.Extras.FvwmConsole]GetLine.Obj : [.Extras.FvwmConsole]GetLine.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmCommand]
  @ $(CCGetLine) /Obj=GetLine.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmConsole]) -
                 GetLine.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    extra FvwmDebug
!-----------------------------------------------------------------------------------------------------------------------------------
[.Extras.Bin]FvwmDebug.Exe : [.Extras.FvwmDebug]FvwmDebug.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Extras.FvwmDebug]FvwmDebug.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Extras.FvwmDebug]FvwmDebug.Obj : [.Extras.FvwmDebug]FvwmDebug.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmDebug]
  @ $(CCCommand) /Obj=FvwmDebug.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmDebug]) -
                 FvwmDebug.C
  @ Set Def $(BaseDirFix)

!-----------------------------------------------------------------------------------------------------------------------------------
!    extra FvwmScript
!-----------------------------------------------------------------------------------------------------------------------------------
[.Extras.Bin]FvwmScript.Exe : [.Extras.FvwmScript]FvwmScript.Obj, -
                              [.Extras.FvwmScript]Instructions.Obj, -
                              [.Extras.FvwmScript]LibYYWrap.Obj, -
                              [.Extras.FvwmScript]Scanner.Obj, -
                              [.Extras.FvwmScript]Script.Obj
  @ Write Sys$Output "Creating image $(MMS$Target)..."
  @ Set Def $(BaseDirFix)
  @ Link/Exe=$(MMS$Target) [.Extras.FvwmScript]FvwmScript.Obj, -
                              [.Extras.FvwmScript]Instructions.Obj, -
                              [.Extras.FvwmScript]LibYYWrap.Obj, -
                              [.Extras.FvwmScript]Scanner.Obj, -
                              [.Extras.FvwmScript]Script.Obj, -
                           $(BaseDir)[Libs]Libs.Olb/Lib, -
                           $(BaseDir)[000000]Vms_Shareables.Opt/Opt
  @ Write Sys$Output ""

[.Extras.FvwmScript]FvwmScript.Obj : [.Extras.FvwmScript]FvwmScript.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmScript]
  @ $(CCCommand) /Obj=FvwmScript.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmScript]) -
                 FvwmScript.C
  @ Set Def $(BaseDirFix)

[.Extras.FvwmScript]Instructions.Obj : [.Extras.FvwmScript]Instructions.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmScript]
  @ $(CCCommand) /Obj=Instructions.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmScript]) -
                 Instructions.C
  @ Set Def $(BaseDirFix)

[.Extras.FvwmScript]LibYYWrap.Obj : [.Extras.FvwmScript]LibYYWrap.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmScript]
  @ $(CCCommand) /Obj=LibYYWrap.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmScript]) -
                 LibYYWrap.C
  @ Set Def $(BaseDirFix)

[.Extras.FvwmScript]Scanner.Obj : [.Extras.FvwmScript]Scanner.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmScript]
  @ $(CCCommand) /Obj=Scanner.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmScript]) -
                 Scanner.C
  @ Set Def $(BaseDirFix)

[.Extras.FvwmScript]Script.Obj : [.Extras.FvwmScript]Script.C
  @ Write Sys$Output "Compiling $(MMS$Source)..."
  @ Set Def $(BaseDirFix)
  @ Set Def [.Extras.FvwmScript]
  @ $(CCCommand) /Obj=Script.Obj
                 /inc=($(BaseDir)[000000],$(BaseDir)[fvwm],$(BaseDir)[libs],$(BaseDir)[.Extras.FvwmScript]) -
                 Script.C
  @ Set Def $(BaseDirFix)

