#include <stdio.h>

int a = 0;
int b;

int foo(){
	a = 0;
	for(int i=0; i<b; ++i){
		a += rand();
	}
	return a;
}


int main(){

	printf("%d\n", foo());

	return 0;
}
