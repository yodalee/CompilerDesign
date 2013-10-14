#!/usr/bin/env bash

BIN=$(cd $(dirname $0); pwd)/../bin

if [ $# -lt 1 ]; then
	echo "Usage: $0 [testnum]"
	echo "Example: $0 1"
	exit 1
fi
if [ ! -e $1 ]; then
	echo "$1 doesn't exist"
	exit 1
fi
echo "test $1"
$BIN/AcDc $1 ${1%.*}.out

