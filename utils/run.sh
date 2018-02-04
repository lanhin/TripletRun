#!/bin/bash
#
# @2018-01  by lanhin
#

tripletrun(){
    lastop=${!#}
    #echo $lastop

    opstring=""

    NUMBER=$#
    ((NUMBER--))
    i=1
    while [ $i -le $NUMBER ]; do
	opstring=${opstring}" "$1
	((i++))
	shift
    done
    #echo $opstring

    ./triplet $opstring > $lastop 2>&1
}

if [ $# -lt 1 -o $# -gt 2 ]; then
    echo "Usage: ./run.sh  <run option file>  [#threads]"
    exit 1
fi

tempfifo=$$.fifo
OPTIONSFILE=$1

if [ $# -eq 2 ]; then
    n=$2
else
    n=1
fi

echo "n="$n

trap "exec 1000>&-;exec 1000<&-;exit 0" 2
mkfifo $tempfifo
exec 1000<>$tempfifo
rm -rf $tempfifo

for ((i=1; i<=$n; i++))
do
    echo >&1000
done

if [ -f $OPTIONSFILE ]; then
    OLD_IFS="$IFS"
    IFS='
'
 #   cat $OPTIONSFILE | while read LINE
    for LINE in `cat $OPTIONSFILE`
    do
	IFS="$OLD_IFS"
	read -u1000
	{
	    echo $LINE
	    tripletrun $LINE
	    echo >&1000
	} &
	IFS='
'
    done #< $OPTIONSFILE
    IFS="$OLD_IFS"
else
    echo "$OPTIONSFILE is not a normal file or doesn't exist."
    exit 1
fi
