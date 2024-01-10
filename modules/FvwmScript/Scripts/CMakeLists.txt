project(Scripts)
find_package(Perl REQUIRED)
configure_file(${PROJECT_SOURCE_DIR}/FvwmScript-ComExample.in ${PROJECT_SOURCE_DIR}/FvwmScript-ComExample @ONLY)

#rm -f FvwmScript-ComExample
#sed -e 's,xFVWM_DATADIRx,/usr/local/share/fvwm3,' -e 's,xPERLx,/usr/bin/perl,' ../../../modules/FvwmScript/Scripts/FvwmScript-ComExample.in > FvwmScript-ComExample-t
#mv FvwmScript-ComExample-t FvwmScript-ComExample
