#!/usr/bin/bash
set -euo pipefail
IFS=$'\n\t'

define="#define "
defineLength=${#define}
defineLength2Less=$(($defineLength-2))

commentStarter="//"
commentStarterLength=${#commentStarter}
commentStarterSecondChar=${commentStarter:1:1}

blockCommentStarter="/*"
blockCommentStarterSecondChar=${blockCommentStarter:1:1}
blockCommentStarterFirstChar=${blockCommentStarter:0:1}

#config_h="$(<testfile.txt)
#"
if [ 1 -gt $# ] || [ ! -f $1 ]; then
    echo "Invalid use of script. Usage $0 <file name to parse>"
    exit 1
fi
config_h="$(<$1)
"
config_hLength=${#config_h}

#COMMENT, DEFINE, DEFINENAME, DEFINEVALUE, WHITESPACE, BLOCKCOMMENT
state="WHITESPACE"

comment=""
defineName=""
defineValue=""
defineCounter=0

newLineCounter=1
indentCounter=0

maybeRequired=""
required=""

printFind() {
    comment=${comment%$'\nCOMMENT: '}
    if [ "$comment" != "" ]; then
        echo "COMMENT: $comment"
    fi
    echo -n "Define: \"$defineName\""
    if [ "$defineValue" != "" ]; then
        echo " \"$defineValue\""
    else
        echo ""
    fi
    find=`find . -path "./build" -prune -o -not -name "CMakeLists.txt" -not -name "*.am" -not -name "*_config.h" -not -name "*.m4" -not -name "configFinder.sh" -type f -exec bash -c "if grep -q $defineName {}; then echo Found \"$defineName\" in \"{}\"; grep --color=always $defineName {}; echo ' '; fi" \;`
    if [ "$find" != "" ]; then
        echo ""
        echo -e "$find"
        required="$required
/*$comment
*/
#define $defineName $defineValue
"
    else
        maybeRequired="$maybeRequired
/*$comment*/
#define $defineName $defineValue
"
        echo "Not found"
    fi

    echo ""
    comment=""
    state="WHITESPACE"
}

for (( i=0; i<$config_hLength; i++ )); do
    subString=${config_h:$i:1}


    if [ "$state" = "COMMENT" ]; then
        if [ "$subString" = $'\n' ]; then
            comment="$comment
COMMENT: "
            state="WHITESPACE"
        elif [ "$subString" != "$commentStarterSecondChar" ]; then
            comment="$comment$subString"
        fi

    elif [ "$state" = "BLOCKCOMMENT" ]; then
        if [ "$subString" = "$blockCommentStarterFirstChar" ] && [ "${config_h:$(($i-1)):1}" = "$blockCommentStarterSecondChar" ]; then
            state="WHITESPACE"
        elif [ "$subString" = $'\n' ]; then
            comment="$comment
COMMENT: "
        elif [ "${config_h:$i:2}" != "$blockCommentStarterSecondChar$blockCommentStarterFirstChar" ] && [ "${config_h:$(($i-1)):2}" != "$blockCommentStarter" ]; then
            comment="$comment$subString"
        fi

    elif [ "$state" = "DEFINE" ]; then
        if (( $defineLength2Less > $defineCounter )); then
            defineCounter=$(($defineCounter+1))
        else
            state="DEFINENAME"
        fi

    elif [ "$state" = "DEFINENAME" ]; then
        if [ "$subString" = " " ]; then
            state="DEFINEVALUE"
        elif [ "$subString" = $'\n' ]; then
            printFind
        else
            defineName="$defineName$subString"
        fi

    elif [ "$state" = "DEFINEVALUE" ]; then
        if [ "$subString" = $'\n' ]; then
            printFind
        else
            defineValue="$defineValue$subString"
        fi

    elif [ "$state" = "WHITESPACE" ]; then
        nextTwoChars=${config_h:$i:$commentStarterLength}
        if [ "$nextTwoChars" = "$commentStarter" ]; then
            state="COMMENT"
        elif [ "$nextTwoChars" = "$blockCommentStarter" ]; then
            state="BLOCKCOMMENT"
        elif [ "${config_h:$i:$defineLength}" = "$define" ]; then
            defineName=""
            defineValue=""
            defineCounter=0
            state="DEFINE"
        elif [ "$subString" != $'\n' ]; then
            for (( ii=0; ii<$i; ii++ )); do
                if [ "${config_h:$ii:1}" = $'\n' ]; then
                    indentCounter=0
                    newLineCounter=$(($newLineCounter+1))
                fi
                indentCounter=$(($indentCounter+1))
            done

            echo "INVALID CHARACTER: \"$subString\" AT CHARACTER $newLineCounter:$indentCounter"
            exit 1
        fi
    fi
done

echo -e "DONE!"
echo "$maybeRequired"
echo ""
echo "Required 100%"
echo "$required"

exit 0
