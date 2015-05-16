// PROGRAMA p07_server.c 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>

int readline(int fd, char *str);

int main(void) 
{ 
	setbuf(stdout, NULL);
	//Recebe os dados iniciais
	int   fifo_req; 
	char  strLine1[100]; 
	char  strLine2[100];
	mkfifo("/tmp/fifo_req",0660); 
	fifo_req=open("/tmp/fifo_req",O_RDONLY);

	//Retorna os resultados
	int   fifo_ans; 
	char  str_2[100]; 
	mkfifo("/tmp/fifo_ans",0660); 
	fifo_ans=open("/tmp/fifo_ans",O_WRONLY);

	while (readline(fifo_req, strLine1))
	{
		readline(fifo_req, strLine2);

		int number1, number2;
		number1 = atoi(strLine1);
		number2 = atoi(strLine2);
		if (number1 == 0 && number2 == 0)
			break;

		char message[100];
		int sum = number1 + number2;
		sprintf(message,"Sum: %d\n",sum);
		printf("Message\n%s", message);
		int messagelen = strlen(message+1); 
		write(fifo_ans,message,messagelen); 
		//write(fifo_ans, "asdf", 5);
		/*sprintf(message,"Diff: %d",number1 - number2);
		messagelen = strlen(message+1); 
		write(fifo_ans,message,messagelen); 
		sprintf(message,"Product: %d",number1 * number2);
		messagelen = strlen(message+1); 
		write(fifo_ans,message,messagelen);
		sprintf(message,"Quotient: %d",number1 / number2);
		messagelen = strlen(message+1); 
		write(fifo_ans,message,messagelen); */ 

		printf("\nSent data do FIFO");
	}
	sleep(3);

	close(fifo_ans);
	close(fifo_req); 
	return 0; 
} 

int readline(int fd, char *str) 
{ 
	int n; 
	do { 
		n = read(fd,str,1); 
	} while (n>0 && *str++ != '\0'); 
	return (n>0); 
}