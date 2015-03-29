#include <stdio.h>

//------------------Problemas 1 - 4--------------------


int main(int argc, char *argv[]) {
	
	unsigned int numero = atoi(argv[1]);
	unsigned int counter = 0;
	while (counter < numero) {
		printf("Hello ");
		unsigned int i = 2;
		 while (i < argc) {
			printf("%s ", argv[i]);
			i++;
		}
		printf("! \n");
		counter++;
	}

	return 0;
}

