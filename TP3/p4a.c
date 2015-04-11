#include <stdio.h>
#include <sys/types.h>

int main(void)
{
	pid_t pid;
	pid = fork();
	switch (pid)
	{
		case 0: //Processo filho escreve hello
			printf("\nHello ");
			break;
		default: //Processo pai escreve World!
			sleep(1);
			printf("World!\n");
			break;
	}
	return 0;
} 