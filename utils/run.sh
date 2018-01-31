#!/bin/bash

tripletrun(){
    lastop=${!#}
    echo $lastop

    opstring=""

    NUMBER=$#
    ((NUMBER--))
    i=1
    while [ $i -le $NUMBER ]; do
	opstring=${opstring}" "$1
	((i++))
	shift
    done
    echo $opstring

    ./triplet $opstring > $lastop 2>&1
}


OPTIONSFILE=$1

if [ -f $OPTIONSFILE ]; then
    OLD_IFS="$IFS"
    IFS='
'
 #   cat $OPTIONSFILE | while read LINE
    for LINE in `cat $OPTIONSFILE`
    do
	IFS="$OLD_IFS"
	echo $LINE
	tripletrun $LINE
	IFS='
'
    done #< $OPTIONSFILE
    IFS="$OLD_IFS"
else
    echo "$OPTIONSFILE is not a normal file or doesn't exist."
    exit 1
fi
