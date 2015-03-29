#include <stdio.h>

//------------------Problemas 5 a----------------------


int main(int argc, char *argv[], char *envp[]) {
	unsigned int counter = 0;
	while (envp[counter] != NULL) {
		printf("%s\n", envp[counter]);
		counter++;
		}
	return 0;
}

