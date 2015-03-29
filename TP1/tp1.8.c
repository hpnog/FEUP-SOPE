// PROGRAMA p7g.c

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	
	if (argc != 3) {
		printf("Wrong number of arguments\n");
		exit(1);
	}

	int num1 = atoi(argv[1]);
	int num2 = atoi(argv[2]);	

	if (num2 >= num1) {
		printf("Wrong arguments\n");
		exit(1);
	}

	int guess = -1;
	int i = 1;
	srand(time(NULL));
	while (guess != num2) {
		guess = rand() % num1;
		printf("%d: %d\n", i, guess); 
		i++;
	}

	exit(0);		//zero e geralmente indicativo de "sucesso"
}
