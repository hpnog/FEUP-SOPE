#include <stdio.h>
#include <stdlib.h>

//------------------Problemas 5 d----------------------


int main(int argc, char *argv[]) {
	char* user = getenv("USER_NAME");
	printf("Hello %s\n", user);
	return 0;
}

