#!/bin/bash

try(){
	echo -e $1 | ./mcc > ./out.S
	clang -g3 -o ./out.out ./out.S
	./out.out
	RES=`echo $?`
	if [ $RES == $2 ]; then
		echo "pass:$1 -> $RES"
	else
		echo "expected:$1 -> $2"
		echo "but result: $RES"
		exit 1
	fi
}

try 2 2
try 1+2+3 6
try 3-2-1 0
