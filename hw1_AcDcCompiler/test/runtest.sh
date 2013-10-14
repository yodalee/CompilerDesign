#!/usr/bin/env bash

BIN=$(cd $(dirname $0); pwd)/../bin

if [ $# -lt 1 ]; then
	echo "Usage: $0 [testnum]"
	echo "Example: $0 1"
	exit 1
fi
if [ ! -e sample${1}.ac ]; then
	echo "sample$1.ac doesn't exist"
	exit 1
fi
echo "test sample$1.ac"
$BIN/AcDc ./sample$1.ac ./output$1.dc

