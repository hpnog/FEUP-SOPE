#include <stdio.h>
#include <stdlib.h>

//------------------Problemas 5 b----------------------


int main(int argc, char *argv[]) {
	char* user = getenv("USER");
	printf("Hello %s\n", user);
	return 0;
}

