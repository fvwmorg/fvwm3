#!/bin/sh
FVWMPOT=fvwm3.pot
MSGINIT=msginit
MSGFMT=msgfmt
MSGUNIQ=msguniq
MSGMERGE=msgmerge
XGETTEXT=xgettext

ALL_LINGUAS="ar da de es fr ru sv_SE zh_CN zh_TW"

FVWM_FILES="../fvwm/fvwm3.c \
	../fvwm/expand.c \
	../fvwm/windowlist.c \
	../fvwm/virtual.c \
	../fvwm/menus.c \
	../modules/FvwmPager/FvwmPager.c \
	../modules/FvwmPager/x_pager.c"
FVWM_RCFILES="default-config/config \
	bin/fvwm-menu-desktop-config.fpl \
	modules/FvwmForm/FvwmForm-XDGMenuHelp \
	modules/FvwmForm/FvwmForm-XDGOptionsHelp"
FVWMSCRIPT_FILES="default-config/FvwmScript-ConfirmQuit \
	default-config/FvwmScript-ConfirmCopyConfig \
	modules/FvwmScript/Scripts/FvwmScript-BellSetup \
	modules/FvwmScript/Scripts/FvwmScript-KeyboardSetup \
	modules/FvwmScript/Scripts/FvwmScript-PointerSetup \
	modules/FvwmScript/Scripts/FvwmScript-ScreenSetup \
	modules/FvwmScript/Scripts/FvwmScript-FileBrowser"

# Function Definitions
update_po() {
	POFILE=$1.po
	if [ ! -e "${POFILE}" ]; then
		echo "${POFILE} doesn't exist. Aborting!"
		exit 1
	fi
	echo "Updating ${POFILE}."
	mv ${POFILE} temp.po
	${MSGMERGE} temp.po ${FVWMPOT} -o ${POFILE}
	rm -f temp.po
}

build_po() {
	POFILE=$1.po
	GMOFILE=fvwm3.$1.gmo
	if [ ! -e "${POFILE}" ]; then
		echo "${POFILE} doesn't exist. Aborting!"
		exit 1
	fi
	echo "Building ${GMOFILE}."
	rm -f ${GMOFILE}
	${MSGFMT} -c -f --statistics -o ${GMOFILE} ${POFILE}
}

CMD=$1
LL_CC=$2
case "$CMD" in
	"init")
		POFILE=${LL_CC}.po
		if [ -e "$POFILE" ]; then
			echo "${POFILE} exists. Aborting!"
			exit 1
		fi
		echo "Initializing ${POFILE}."
		${MSGINIT} -i ${FVWMPOT} -l ${LL_CC} -o ${POFILE}
		;;
	"update-pot")
		${XGETTEXT} --add-comments=TRANSLATORS: --keyword=_ -o temp.pot \
			--copyright-holder='fvwm workers' ${FVWM_FILES}
		echo >> temp.pot
		sed -i 's/..\/fvwm/fvwm/g' temp.pot
		for file in ${FVWM_RCFILES}; do \
			perl -ne 's/\[gt\.((\\.|.)+?)\]/ print "\#: $ARGV:$.\n"."msgid \"$1\"\n"."msgstr \"\"\n\n"/ge' \
			../$file >> temp.pot
		done
		for file in ${FVWMSCRIPT_FILES}; do \
			perl -ne 's/LocaleTitle\s+\{((\\.|.)+?)\}/ print "\#: $ARGV:$.\n"."msgid \"$1\"\n"."msgstr \"\"\n\n"/ge' \
			../$file >> temp.pot
		done
		sed -i 's/^#: ..\//#: /' temp.pot
		${MSGUNIQ} temp.pot > ${FVWMPOT}
		rm -f temp.pot
		;;
	"update")
		update_po $LL_CC
		;;
	"update-all")
		for ll_cc in ${ALL_LINGUAS}; do \
			update_po $ll_cc
		done
		;;
	"build")
		build_po $LL_CC
		;;
	"build-all")
		for ll_cc in ${ALL_LINGUAS}; do \
			build_po $ll_cc
		done
		;;
esac
