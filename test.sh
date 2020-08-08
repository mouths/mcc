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

try "int main() return 2;" 2
try "int main() return 1+2+3;" 6
try "int main() return 3-2-1;" 0
try "int main() return 2*3;" 6
try "int main() return 5/2;" 2
try "int main() return 5%2;" 1
try "int main() return 2*3-5%2;" 5
try "
int main()
return
( 2 * 3 - 5 )% 2
;
" "1"
try "int main() return -2+3;" 1
try "int main() return 1<2;" 1
try "int main() return 2>=1-5;" 1
try "int main() return 2== 3;" 0
try "int main() return 35 -5 != 20;" 1
try "int main() return 341&682;" 0
try "int main() return 85^171;" 254
try "int main() return 85|171;" 255
try "int main() {{return 10;}{
10 +2;
return 15;
return 20;
}}" 10
try "int main() {
a = 5;
a = a + a;;;;;;
return a + 2;
}" 12
try "int main(){
	hoge = 8;
	huga = 2;
	hoge = hoge + huga;
	return hoge * 2;
}
" 20
