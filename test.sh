#!/bin/bash

try(){
	echo -e "$1" | ./mcc > ./out.S
	clang -g -o ./out.out ./out.S
	./out.out
	RES=`echo $?`
	if [ "$RES" == "$2" ]; then
		echo "pass:\"$1\" -> $RES"
	else
		echo "expected:\"$1\" -> $2"
		echo "but result: $RES"
		exit 1
	fi
}

try "return 2;" 2
try "return 1+2+3;" 6
try "return 3-2-1;" 0
try "return 2*3;" 6
try "return 5/2;" 2
try "return 5%2;" 1
try "return 2*3-5%2;" 5
try "
return
( 2 * 3 - 5 )% 2
;
" "1"
try "return -2+3;" 1
try "return 1<2;" 1
try "return 2>=1-5;" 1
try "return 2== 3;" 0
try "return 35 -5 != 20;" 1
try "return 341&682;" 0
try "return 85^171;" 254
try "return 85|171;" 255
try "{{return 10;}{
10 +2;
return 15;
return 20;
}}" 10
try "{
a = 5;
a = a + a;;;;;;
return a + 2;
}" 12
