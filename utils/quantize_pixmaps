#!/bin/sh
cd $HOME/tmp-icons

# We process all of the xpm files in ~/icons, reducing their collective
# colourmap, and producing files in ~/tmp-icons.

# Make a note of those files with transparent bits for zapping afterwards:
# this isn't perfect, but seems to work OK for the pixmaps that I use.

# Notice that the input, output, and number of colours are hardwired.

# (The ppmtogif | giftoppm is to make sure the ppm file is in raw format, because
# ppmquant appears not to work with the result of xpmtoppm!)

echo Converting files to ppm format
NONE=""
for i in ../icons/*.xpm
do
    echo -n $i " "
    sed 's/[Nn]one/black/g' $i | xpmtoppm | ppmtogif | giftoppm > `basename $i .xpm`.ppm
    if test -s `basename $i .xpm`.ppm
    then
	if grep -iq None $i
	then
	    NONE="$NONE `basename $i`"
	fi
    else
        rm -f `basename $i .xpm`.ppm
    fi
done

echo Performing quantization to 32 colours
ppmquantall 32 *.ppm

echo Converting to xpm format again
for i in *.ppm
do
    echo -n $i " "
    ppmtoxpm $i > `basename $i .ppm`.xpm
    rm -f $i
done

echo Trying to fix transparent pixels in some files
for i in $NONE
do
    echo $i
    sed 's/` c #000000/` c None/' $i > $i-tmp.xpm
    mv $i-tmp.xpm $i
done

