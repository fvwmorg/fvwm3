#!/bin/sh
#
# This script uses asciidoctor to build the html documents for the fvwm3
# manual pages from the fvwm3 source. The script takes a single input,
# the full path to the fvwm3 source.
#
#     ./_build_fvwm3_man.sh /path/to/fvwm3_source

# Check for asciidoctor and input is root of fvwm3 source.
if [ -z "$1" ]
then
	echo "No input provided."
	echo "Usage: ./_build_fvwm3_man.sh /path/to/fvwm3_source"
	exit 1
fi
if [ ! -f "$1/doc/fvwm3_manpage_source.adoc" ]
then
	echo "Fvwm3 source not found at: $1"
	exit 1
fi
if ! command -v asciidoctor >/dev/null 2>&1
then
	echo "Asciidoctor not found."
	exit 1
fi

FVWM_DOC_SRC="$1/doc"

FVWM_VER="$(cd $1 && ./utils/fvwm-version-str.sh)"
[ -z "$FVWM_VER" ] && {
	echo "Couldn't ascertain FVWM version">&2
	exit 1
}

CMD="asciidoctor -e -b html5 -a toc -a webfonts!"
FVWM_DOCS_AD="commands menus styles"
FVWM_DOCS="fvwm3 fvwm3all fvwm3commands fvwm3menus fvwm3styles FvwmAnimate \
	FvwmAuto FvwmBacker FvwmButtons FvwmCommand FvwmEvent \
	FvwmForm FvwmIconMan FvwmIdent FvwmMFL FvwmPager FvwmPerl FvwmPrompt \
	FvwmRearrange FvwmScript fvwm-menu-desktop fvwm-menu-directory \
	fvwm-menu-xlock fvwm-perllib fvwm-root fvwm-convert-2.6"

# Generate .ad files by greping the appropriate section from the source.
for section in ${FVWM_DOCS_AD}
do
	IN_FILE=${FVWM_DOC_SRC}/fvwm3_manpage_source.adoc
	OUT_FILE=${FVWM_DOC_SRC}/fvwm3_${section}.ad
	echo "Generating: fvwm3${section}.ad"
	cat ${IN_FILE} | grep -A 10000000 -- "^// BEGIN '${section}'" | \
		grep -B 10000000 -- "^// END '${section}'" | \
		grep -v "^// .* '${section}'" > ${OUT_FILE}
done

# Build fvwm manual pages and modify the output from asciidoctor.
for doc in ${FVWM_DOCS}
do
	IN_FILE=${FVWM_DOC_SRC}/${doc}.adoc
	OUT_FILE=Man/${doc}.html
	HEADER="---\ntitle: ${doc} manual page\nshowtitle: 1\n"
	HEADER="${HEADER}permalink: /Man/${doc}/index.html\n---"
	echo "Building: ${OUT_FILE}"
	echo ${HEADER} > ${OUT_FILE}
	${CMD} -a ${doc} ${IN_FILE} -o - >> ${OUT_FILE}
	sed -i 's/literalblock/literalblock highlight/' ${OUT_FILE}
	sed -i '/toctitle/d' ${OUT_FILE}
	sed -i 's/id="toc" class="toc"/id="markdown-toc"/' ${OUT_FILE}
done

# Build index page.
IN_FILE=${FVWM_DOC_SRC}/index.adoc.in
OUT_FILE=Man/index.html
HEADER="---\ntitle: Fvwm3 Manual Pages\nshowtitle: 1\n---\n"
HEADER="${HEADER}The following are the manual pages for "
HEADER="${HEADER}fvwm3 version <strong>${FVWM_VER}</strong>."
echo "Building: ${OUT_FILE}"
echo ${HEADER} > ${OUT_FILE}
${CMD} -a index ${IN_FILE} -o - >> ${OUT_FILE}
sed -i 's!.html"!/"!' ${OUT_FILE}
sed -i '/toctitle/d' ${OUT_FILE}
sed -i 's/id="toc" class="toc"/id="markdown-toc"/' ${OUT_FILE}
sed -i 's/ class="sect1"//' ${OUT_FILE}
sed -i 's!<p>!!g' ${OUT_FILE}
sed -i 's!</p>!!g' ${OUT_FILE}

# Cleanup .ad files.
for section in ${FVWM_DOCS_AD}
do
	OUT_FILE=${FVWM_DOC_SRC}/fvwm3_${section}.ad
	echo "Deleting: ${OUT_FILE}"
	rm ${OUT_FILE}
done
