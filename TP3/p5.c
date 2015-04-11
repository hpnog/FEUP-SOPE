#include <stdio.h>
#include <sys/types.h>

int main(void)
{
	pid_t pid;
	pid_t pid2;
	pid = fork();
	switch (pid)
	{
		case 0: //Processo filho escreve hello
			printf("\nHello ");
			break;
		default: //Processo pai escreve World!
			pid2 = fork();
			switch (pid2)
			{
				case 0:
					sleep(1);
					printf("my");
					break;
				default:
					sleep(2);
					printf(" friends!\n");
					break;
			}
	}
	return 0;
} 