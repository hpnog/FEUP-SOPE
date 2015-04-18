#include <unistd.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define CORRECT_EXIT 0
#define ERROR_EXIT 1
#define TRUE 1

int main(int argc, char * argv[])
{
	int stat;
	char * pathToDir = malloc((strlen(argv[1]) + 1) * sizeof(char));								//Como argumento recebe o caminho para o diretório
	if (argv[1][strlen(pathToDir) - 1] == '/')
		strncpy(pathToDir, argv[1], strlen(argv[1]) - 1);
	else
		strncpy(pathToDir, argv[1], strlen(argv[1]));

	//------------------------CRIA CAMINHO PARA WORDS E VERIFICA SE ESTA CORRETO----------------
	char * pathToWords = malloc((strlen(pathToDir) + 11) * sizeof(char));				//Cria char[] para o caminho para words.txt
	strncpy(pathToWords, pathToDir, strlen(pathToDir));
	strcat(pathToWords, "/words.txt");	

	if (access(pathToWords, F_OK) != 0)						//Verifica se words.txt existe
		exit(ERROR_EXIT);									//Se nao existir terimna e retorna 1
	
	//-------------------------------------------------------------------------------------------
	int counterOfFiles = 1;
	do
	{
		//----------CREATES NAME TO NEXT FILE AND CHECKS IT OUT----------------------------------
		char stringNum[10];
		sprintf(stringNum, "%d", counterOfFiles);										//gets number of the file into string
		char * pathToFile = malloc((strlen(pathToDir) + 11) * sizeof(char));					//Cria char[] para o caminho para words.txt
		strcpy(pathToFile, pathToDir);
		strcat(pathToFile, "/");																//Adiciona ao caminho de dir - /words.txt
		strcat(pathToFile, stringNum);	
		strcat(pathToFile, ".txt");
		if (access(pathToFile, F_OK) != 0)					//Verifica se words.txt existe
			break;	
		//-------------------------------------------------------------------------------------------

		pid_t pid = fork();
		if (pid == 0)
			execlp("./bin/sw", "./bin/sw", pathToWords, pathToFile, NULL);
		else if (pid > 0)
		{
			counterOfFiles++;
			wait(&stat);
		}
		else
		{
			fprintf(stderr, "\nError in Fork in index\n");
			exit(ERROR_EXIT);
		}

		
	} while (TRUE);

	execlp("./bin/csc", "./bin/csc", pathToDir, NULL);

	exit(CORRECT_EXIT);
}