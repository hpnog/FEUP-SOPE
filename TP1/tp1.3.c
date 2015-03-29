#include <stdio.h>

//------------------Problemas 5 b----------------------


int main(int argc, char *argv[], char *envp[]) {
	unsigned int counter = 0;
	while (envp[counter] != NULL) {
		if (0 == strncmp("USER=", envp[counter], 5)) {
			printf("Hello %s\n", envp[counter]+5);
			break;
			}
		counter++;
		}
	return 0;
}

