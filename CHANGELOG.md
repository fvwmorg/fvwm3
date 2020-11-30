# Changelog

## [Unreleased](https://github.com/fvwmorg/fvwm3/tree/HEAD)

[Full Changelog](https://github.com/fvwmorg/fvwm3/compare/1.0.1...HEAD)

**Implemented enhancements:**

- Add expansion variables for a window's X/Y page [\#255](https://github.com/fvwmorg/fvwm3/issues/255)
- Fvwm should provide a Status option [\#253](https://github.com/fvwmorg/fvwm3/issues/253)
- \[feature-request\] Global desktop with predefined resolutions [\#248](https://github.com/fvwmorg/fvwm3/issues/248)
- Move to Python 3 [\#233](https://github.com/fvwmorg/fvwm3/issues/233)

**Fixed bugs:**

- FvwmPager: pin new\_desk events to monitor instance for per-monitor mode [\#296](https://github.com/fvwmorg/fvwm3/issues/296)
- FvwmPager: windows tracked incorrectly when moving between monitors [\#294](https://github.com/fvwmorg/fvwm3/issues/294)
- bson\_as\_relaxed\_extended\_json\(\) is not available [\#286](https://github.com/fvwmorg/fvwm3/issues/286)
- EdgeResistance command not working? [\#285](https://github.com/fvwmorg/fvwm3/issues/285)
- Status: not updating on browser tab switching [\#274](https://github.com/fvwmorg/fvwm3/issues/274)
- FvwmScript - Crashes on input to TextField widget [\#272](https://github.com/fvwmorg/fvwm3/issues/272)
- FvwmButtons Geometry - @g tag positions objects on active monitor instead of globally  [\#269](https://github.com/fvwmorg/fvwm3/issues/269)
- Configuration parsing does not read FvwmPager module config line if prefixed with Test condition. [\#267](https://github.com/fvwmorg/fvwm3/issues/267)
- FvwmPager and FvwmIconMan do not update after GotoDesk [\#262](https://github.com/fvwmorg/fvwm3/issues/262)
- status:  fix bson\_t detection [\#257](https://github.com/fvwmorg/fvwm3/issues/257)
- Maximize on second monitor gives wrong window size [\#250](https://github.com/fvwmorg/fvwm3/issues/250)
- Man page `fvwm3.1` not built by default [\#246](https://github.com/fvwmorg/fvwm3/issues/246)
- FvwmPager sometimes ignores styles [\#142](https://github.com/fvwmorg/fvwm3/issues/142)
- EdgeScroll needs thinking about for per-monitor setup [\#82](https://github.com/fvwmorg/fvwm3/issues/82)
- Panframes: switching between desktops not reliable / broken [\#34](https://github.com/fvwmorg/fvwm3/issues/34)
- FvwmPager: track windows assigned to pager's screen [\#295](https://github.com/fvwmorg/fvwm3/pull/295) ([ThomasAdam](https://github.com/ThomasAdam))

**Closed issues:**

- Testing notes for FVWM3-rc.X release candidate [\#65](https://github.com/fvwmorg/fvwm3/issues/65)

**Merged pull requests:**

- Remove SAFEFREE macro [\#277](https://github.com/fvwmorg/fvwm3/pull/277) ([tekknolagi](https://github.com/tekknolagi))

## [1.0.1](https://github.com/fvwmorg/fvwm3/tree/1.0.1) (2020-10-04)

[Full Changelog](https://github.com/fvwmorg/fvwm3/compare/1.0.0...1.0.1)

**Implemented enhancements:**

- fvwm3 man and default paths to config files? [\#206](https://github.com/fvwmorg/fvwm3/issues/206)

**Fixed bugs:**

- compilation fails on openbsd-current due to safemalloc.h \(va\_list\) [\#231](https://github.com/fvwmorg/fvwm3/issues/231)
- \_NET\_WM\_STATE was not updated for maximized windows [\#203](https://github.com/fvwmorg/fvwm3/issues/203)
- EwmhBaseStruts calculations don't use monitor's [\#241](https://github.com/fvwmorg/fvwm3/issues/241)
- DesktopConfiguration global inherits behaviour from per-monitor mode [\#236](https://github.com/fvwmorg/fvwm3/issues/236)
- FvwmEvent:  missing `monitor\_focus` event [\#228](https://github.com/fvwmorg/fvwm3/issues/228)
- perllib: doesn't understand MX\_MONITOR\_\* events [\#226](https://github.com/fvwmorg/fvwm3/issues/226)
- FvwmButtons subpanels not popped out on primary monitor when desk is \> 0 [\#224](https://github.com/fvwmorg/fvwm3/issues/224)
- PositionPlacement Center: fix to use current screen [\#211](https://github.com/fvwmorg/fvwm3/issues/211)
- EwmhBaseStruts missing screen info from manpage [\#208](https://github.com/fvwmorg/fvwm3/issues/208)
- "version of go" misinterpreted by configure script? [\#202](https://github.com/fvwmorg/fvwm3/issues/202)
- Unable to build 1.0, bson.h not found \[FreeBSD 12.1\] [\#200](https://github.com/fvwmorg/fvwm3/issues/200)
- EwmhBaseStrut: fix calculations for per-monitor [\#242](https://github.com/fvwmorg/fvwm3/pull/242) ([ThomasAdam](https://github.com/ThomasAdam))
- Fix window locations in Global mode [\#237](https://github.com/fvwmorg/fvwm3/pull/237) ([ThomasAdam](https://github.com/ThomasAdam))

## [1.0.0](https://github.com/fvwmorg/fvwm3/tree/1.0.0) (2020-09-03)

[Full Changelog](https://github.com/fvwmorg/fvwm3/compare/1.0.0-rc0...1.0.0)

**Fixed bugs:**

- New versioning scheme of FVWM3 RC0 break Version test condition. [\#195](https://github.com/fvwmorg/fvwm3/issues/195)

## [1.0.0-rc0](https://github.com/fvwmorg/fvwm3/tree/1.0.0-rc0) (2020-08-31)

[Full Changelog](https://github.com/fvwmorg/fvwm3/compare/289c0e27f2438c43e66b980641561062659bd14b...1.0.0-rc0)

**Implemented enhancements:**

- Function to ignore screen boundaries in multi-monitor setups \(when maximizing/full-screening\) [\#186](https://github.com/fvwmorg/fvwm3/issues/186)
- Add a dmenu/rofi keybinding for default config [\#112](https://github.com/fvwmorg/fvwm3/issues/112)
- Support lists of fonts \(for falling back on\) [\#37](https://github.com/fvwmorg/fvwm3/issues/37)
- conky can make fvwm3 go sloppy [\#32](https://github.com/fvwmorg/fvwm3/issues/32)
- msgpack as fvwm \<-\> modules \<-\> bindings communicator [\#31](https://github.com/fvwmorg/fvwm3/issues/31)
- Proposal: FvwmEvent new events in FVWM3 [\#26](https://github.com/fvwmorg/fvwm3/issues/26)
- Segmentation fault while turning off display port cable and mouse pointer is positioned on the monitor which is going off [\#15](https://github.com/fvwmorg/fvwm3/issues/15)
- Compiler warnings in ta/desktops  [\#13](https://github.com/fvwmorg/fvwm3/issues/13)
- No .desktop file generated from make install [\#12](https://github.com/fvwmorg/fvwm3/issues/12)
- making menuitem text always vertically centered. [\#181](https://github.com/fvwmorg/fvwm3/pull/181) ([mikeandmore](https://github.com/mikeandmore))
- Add icons to the WindowOpsLong menus for default-config. [\#141](https://github.com/fvwmorg/fvwm3/pull/141) ([somiaj](https://github.com/somiaj))
- key binding and menu option for dmenu\_run [\#113](https://github.com/fvwmorg/fvwm3/pull/113) ([lgsobalvarro](https://github.com/lgsobalvarro))
- FvwmEvent: listen for RandR events [\#106](https://github.com/fvwmorg/fvwm3/pull/106) ([ThomasAdam](https://github.com/ThomasAdam))
- expansion: add $\[monitor.X\] namespace [\#74](https://github.com/fvwmorg/fvwm3/pull/74) ([ThomasAdam](https://github.com/ThomasAdam))

**Fixed bugs:**

- no check for libxt-dev when fribidi is enabled [\#191](https://github.com/fvwmorg/fvwm3/issues/191)
- fvwm-menu-desktop only produces half the xdg-menu [\#177](https://github.com/fvwmorg/fvwm3/issues/177)
- ModuleMFL doesn't appear to open socket [\#172](https://github.com/fvwmorg/fvwm3/issues/172)
- Typo in configure script line 535, confusing libbson error message  [\#162](https://github.com/fvwmorg/fvwm3/issues/162)
- EdgeScroll not working properly when values change via FvwmConsole [\#144](https://github.com/fvwmorg/fvwm3/issues/144)
- FvwmIconMan won't correctly apply colorsets when swallowed [\#135](https://github.com/fvwmorg/fvwm3/issues/135)
- Steam crashes on FVWM3 [\#124](https://github.com/fvwmorg/fvwm3/issues/124)
- multiple definition error when using gcc 10 [\#119](https://github.com/fvwmorg/fvwm3/issues/119)
- More PositionPlacement weirdness [\#115](https://github.com/fvwmorg/fvwm3/issues/115)
- Reproducible Builds: remove \_\_DATE\_\_ and \_\_TIME\_\_ [\#99](https://github.com/fvwmorg/fvwm3/issues/99)
- Transient windows sometimes switch desks [\#95](https://github.com/fvwmorg/fvwm3/issues/95)
- FindScreenOfXY: couldn't find screen at 555 x 134 returning first monitor.  This is a bug. [\#93](https://github.com/fvwmorg/fvwm3/issues/93)
- RaiseLower only Lowers windows in per-desktop mode in certain conditions [\#86](https://github.com/fvwmorg/fvwm3/issues/86)
- Windows open outside of screen [\#85](https://github.com/fvwmorg/fvwm3/issues/85)
- Wrong maximizing with EwmBaseStruts [\#84](https://github.com/fvwmorg/fvwm3/issues/84)
- No default panel with latest Master.  Dual Monitor of different size. [\#78](https://github.com/fvwmorg/fvwm3/issues/78)
- Unmaximizing windows can sometimes vanish [\#68](https://github.com/fvwmorg/fvwm3/issues/68)
- "ThisWindow \(Screen XY\) Sticky" broken? in DesktopConfiguration global [\#64](https://github.com/fvwmorg/fvwm3/issues/64)
- Fix snap attraction [\#61](https://github.com/fvwmorg/fvwm3/issues/61)
- FvwmButtons fails silently; Fvwm3 [\#60](https://github.com/fvwmorg/fvwm3/issues/60)
- FvwmPager segfaults on fvwm3 ta/gh-22 [\#44](https://github.com/fvwmorg/fvwm3/issues/44)
- Do we need to check the value returned from FCreateFImage? [\#42](https://github.com/fvwmorg/fvwm3/issues/42)
- StartsOnPage/StartsOnDesk ignored [\#39](https://github.com/fvwmorg/fvwm3/issues/39)
- FvwmButtons on FVWM3 [\#28](https://github.com/fvwmorg/fvwm3/issues/28)
- In global DesktopConfiguration mode, Wait XTerm fails after changing current desk from 0 0 to 0 1 [\#24](https://github.com/fvwmorg/fvwm3/issues/24)
- Page navigation and selection is incorrect after FVWM3 restart/start while two screens are enabled [\#23](https://github.com/fvwmorg/fvwm3/issues/23)
- FvwmPager is broken with RandR [\#22](https://github.com/fvwmorg/fvwm3/issues/22)
- Position of windows on screens, desks and pages is not accurate after FVWM 3 Restart [\#20](https://github.com/fvwmorg/fvwm3/issues/20)
- DesktopConfiguration per-monitor segmentation fault in certain repeatable conditions [\#19](https://github.com/fvwmorg/fvwm3/issues/19)
- Multiple Pages \(3x3\) and RandR is confusing and broken [\#17](https://github.com/fvwmorg/fvwm3/issues/17)
- X windows started on newly defined monitor doesn't accept focus [\#16](https://github.com/fvwmorg/fvwm3/issues/16)
- Logs of starting - closing fvwm3 with DesktopConfiguration set [\#14](https://github.com/fvwmorg/fvwm3/issues/14)
- if monitor name happens to get updated in ParseOptions, the rest of FvwmPager config is skipped [\#146](https://github.com/fvwmorg/fvwm3/pull/146) ([d-e-e-p](https://github.com/d-e-e-p))
- monitor: track focus event separately [\#140](https://github.com/fvwmorg/fvwm3/pull/140) ([ThomasAdam](https://github.com/ThomasAdam))
- Fixes issue with FvwmIconMan and Colorsets. [\#136](https://github.com/fvwmorg/fvwm3/pull/136) ([ThomasAdam](https://github.com/ThomasAdam))
- session: fix version check for used\_sm [\#105](https://github.com/fvwmorg/fvwm3/pull/105) ([ThomasAdam](https://github.com/ThomasAdam))
- expand: portably tokenise string using strsep [\#102](https://github.com/fvwmorg/fvwm3/pull/102) ([ThomasAdam](https://github.com/ThomasAdam))
- EWMH: AreaIntersection: fix basestrut calculation [\#94](https://github.com/fvwmorg/fvwm3/pull/94) ([ThomasAdam](https://github.com/ThomasAdam))
- configure: resurrect VERSIONINFO [\#92](https://github.com/fvwmorg/fvwm3/pull/92) ([ThomasAdam](https://github.com/ThomasAdam))
- UPDATE\_FVWM\_SCREEN: respect StartsOnDesk style [\#91](https://github.com/fvwmorg/fvwm3/pull/91) ([ThomasAdam](https://github.com/ThomasAdam))
- configure.ac: assorted fixes [\#88](https://github.com/fvwmorg/fvwm3/pull/88) ([ThomasAdam](https://github.com/ThomasAdam))
- unmaximize: use window's current screen for positioning [\#69](https://github.com/fvwmorg/fvwm3/pull/69) ([ThomasAdam](https://github.com/ThomasAdam))
- SnapAttraction: fix coord detection [\#62](https://github.com/fvwmorg/fvwm3/pull/62) ([ThomasAdam](https://github.com/ThomasAdam))
- fix broken positions when drawing 3d borders [\#48](https://github.com/fvwmorg/fvwm3/pull/48) ([mikeandmore](https://github.com/mikeandmore))
- Do we need to check the value returned from FCreateFImage? [\#43](https://github.com/fvwmorg/fvwm3/pull/43) ([klebertarcisio](https://github.com/klebertarcisio))

**Closed issues:**

- Better logging required [\#77](https://github.com/fvwmorg/fvwm3/issues/77)

**Merged pull requests:**

- Ta/add docker [\#147](https://github.com/fvwmorg/fvwm3/pull/147) ([ThomasAdam](https://github.com/ThomasAdam))
- FvwmPager: per-monitor improvements [\#123](https://github.com/fvwmorg/fvwm3/pull/123) ([ThomasAdam](https://github.com/ThomasAdam))
- logging: add fvwm\_debug infrastructure [\#79](https://github.com/fvwmorg/fvwm3/pull/79) ([ThomasAdam](https://github.com/ThomasAdam))



