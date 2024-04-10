#!/bin/sh
#
# This script uses asciidoctor to build the html documents for www.fvwm.org.
# This is done by first building the pages in --embedded (-e) mode with no
# headers and footers, then placing a simple header for Jekyll on top.
#
# Use: Run utils/fvwm_doc2web.sh from the top level of the fvwm3 source.
# The output will be located in doc/fvwmorg.github.io/
CMD="asciidoctor -e -b html5 -a toc -a webfonts!"
OUT_DIR="fvwmorg.github.io"
FVWM_VER="1.1.0"
FVWM_DOCS="fvwm3 fvwm3all fvwm3commands fvwm3menus fvwm3styles FvwmAnimate \
	FvwmAuto FvwmBacker FvwmButtons FvwmCommand FvwmConsole FvwmEvent \
	FvwmForm FvwmIconMan FvwmIdent FvwmMFL FvwmPager FvwmPerl FvwmPrompt \
	FvwmRearrange FvwmScript fvwm-menu-desktop fvwm-menu-directory \
	fvwm-menu-xlock fvwm-perllib fvwm-root fvwm-convert-2.6"

if [ ! -d "fvwm" ] || [ ! -f "fvwm/fvwm3.c" ] || [ ! -d "doc" ]
then
	echo Run from root level of fvwm3 source using utils/fvwm_man2doc.sh
	exit 1
fi

cd doc/
if [ ! -d "${OUT_DIR}" ]
then
	mkdir ${OUT_DIR}
fi

# Build pages. Place them in their own directory like rest of site.
for doc in ${FVWM_DOCS}
do
	FILE=${OUT_DIR}/${doc}.html
	HEADER="---\ntitle: ${doc} manual page\nshowtitle: 1\n"
	HEADER="${HEADER}permalink: /Man/${doc}/index.html\n---"
	echo "Building HTML: ${doc}.html"
	echo ${HEADER} > ${FILE}
	${CMD} -a ${doc} ${doc}.adoc -o - >> ${FILE}
	sed -i 's/literalblock/literalblock highlight/' ${FILE}
	sed -i '/toctitle/d' ${FILE}
	sed -i 's/id="toc" class="toc"/id="markdown-toc"/' ${FILE}
done

# Build index page.
# A build will remove one of FvwmConsole or FvwmPrompt from the index.
# That is not desirable here, so use the .in file instead.
FILE=${OUT_DIR}/index.html
HEADER="---\ntitle: Fvwm3 Manual Pages\nshowtitle: 1\n---\n"
HEADER="${HEADER}The following are the manual pages for "
HEADER="${HEADER}fvwm3 version <strong>${FVWM_VER}</strong>."
echo "Building HTML: index.html"
echo ${HEADER} > ${FILE}
${CMD} -a index index.adoc.in -o - >> ${FILE}
sed -i 's!.html"!/"!' ${FILE}
sed -i '/toctitle/d' ${FILE}
sed -i 's/id="toc" class="toc"/id="markdown-toc"/' ${FILE}
sed -i 's/ class="sect1"//' ${FILE}
# This should be one line.
sed -i 's!<p>!!g' ${FILE}
sed -i 's!</p>!!g' ${FILE}
