#!/bin/bash

CC=gcc
try(){
	echo -e "$1" | ../mcc > ./out.S
	${CC} -g -c ./out.S
	${CC} -g -static -o ./out.out ./test.o ./out.o
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

mkdir -p ./test
cd ./test/
${CC} -c -static ./test.c

try "int main() {return 2;}" 2
try "int main() {return 1+2+3;}" 6
try "int main() {return 3-2-1;}" 0
try "int main() {return 2*3;}" 6
try "int main() {return 5/2;}" 2
try "int main() {return 5%2;}" 1
try "int main() {return 2*3-5%2;}" 5
try "
int main(){
return
( 2 * 3 - 5 )% 2
;}
" "1"
try "int main() {return -2+3;}" 1
try "int main() {return 1<2;}" 1
try "int main() {return 2>=1-5;}" 1
try "int main() {return 2== 3;}" 0
try "int main() {return 35 -5 != 20;}" 1
try "int main() {return 341&682;}" 0
try "int main() {return 85^171;}" 254
try "int main() {return 85|171;}" 255
try "int main() {{return 10;}{
10 +2;
return 15;
return 20;
}}" 10
try "int main() {
int a;
a = 5;
a = a + a;;;;;;
return a + 2;
}" 12
try "int main(){
	int hoge, huga;
	hoge = 8;
	huga = 2;
	hoge = hoge + huga;
	return hoge * 2;
}
" 20
try "int main(){
	int hoge;
	hoge = 3;
	hoge += 2;
	return hoge;
}" 5
try "int main(){
	int hoge;
	hoge = 11;
	hoge /= 3;
	return hoge;
}" 3
try "int main(){
	int hoge;
	hoge = 11;
	hoge %= 3;
	return hoge;
}" 2
try "int main(){
	int foo;
	return foo();
}" 0
try "int main(){
	int a, *b;
	a = 3;
	b = &a;
	return *b;
}" 3
try "int main(){
	int a, b, *c;
	a = 3;
	b = 5;
	c = &b + 1;
	return *c;
}" 3
try "int main(){
	int *a, get_pointer, **b;
	a = get_pointer();
	*a = 5;
	b = &a;
	return **b;
}" 5
try "int main(int a){
	a = 4;
	return a;
}" 4
try "int main(){
	int print_nums;
	print_nums(1, 2, 3, 4, 5, 6);
	return 0;
}" 0
try "int main(){
	int a, *b;
	a = 2;
	b = &a;
	return *b;
}" 2
try "int main(){
	int a;
	int b;
	return &a - &b;
}" 1
# bad example
#try "int main(){
#	int a;
#	return &a + &a;
#}" undefined
try "int main(){
	int a;
	return sizeof a;
}" 4
try "int main(){
	int a;
	return sizeof &a;
}" 8
try "int main(){
	int *a;
	return sizeof a;
}" 8
try "int main(){
	int a[5];
	a[3] = 3;
	return 3[a];
}" 3
try "int main(){
	int a[2];
	*a = 1;
	*(a + 1) = 2;
	int *p;
	p = a;
	return *p + *(p + 1);
}" 3
try "int main(){
	int a[4];
	return sizeof(a);
}" 16
try "int main(){
	int a[4];
	return sizeof(a + 1);
}" 8
try "int main(){
	int a[3];
	return sizeof(a[2]);
}" 4
try "
int a;
int main(){
	a = 3;
	return a;
}" 3
try "
int a[3];
int main(){
	*a = 1;
	*(a + 1) = 2;
	return *(a) + *(a + 1) * 2;
}" 5
try "int a[3];
int main(){
	a[0] = 3;
	a[1] = 2;
	return a[0] + 1[a];
}" 5
try "
int main(){
	char a;
	a = 1;
	return a;
}" 1
try "int main(){
int printf;
char a[5];
a[0] = 65;
a[1] = 10;
a[2] = 0;
printf(a);
return 0;
}" 0
