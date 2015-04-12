#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CORRECT_EXIT 0
#define ERROR_EXIT 1
#define READING_PIPE 0
#define WRITING_PIPE 1
#define MAX_SIZE_OF_LINE 50


/*
 * argv[1] = nome do ficheiro
 * argv[2] = lista de palavras
 */

 int main(int argc, char *argv[])
 {

 	char *pathToFile = argv[2];
 	char *pathToWords = argv[1];

 	int swPipe[2];
 	FILE *readStream;

 	//--------------------Verificacoes de erro dos pipes
 	if (pipe (swPipe) < 0) {
 		fprintf(stderr, "\nError creating Pipe\n");
 		exit(1);
 	}
 	//--------------------------------------------------

 	pid_t pid = fork();
 	if (pid < 0)
 	{
 		fprintf(stderr, "\nError in Fork in sw\n");
 		exit(ERROR_EXIT);
 	}
 	else if (pid == 0) 
 	{
 		close(swPipe[READING_PIPE]);		
 		dup2(swPipe[WRITING_PIPE], STDOUT_FILENO);
 		execlp("grep","grep","-n", "-o", "-w", "-f", pathToWords, pathToFile, NULL);
 	}
 	else
 	{
 		wait(pid);
 		close(swPipe[WRITING_PIPE]);		
 		readStream = fdopen(swPipe[READING_PIPE], "r");
 		char line[MAX_SIZE_OF_LINE];
 		//-----------------CREATE FILE AND STORE DATA IN IT--------------------------------
 		//-----CREATE FILE------
 		char * lastSlash = strrchr(pathToFile, '/');
 		if (lastSlash == NULL)
 			lastSlash = pathToFile;
 		int numberOfLastSlash = lastSlash - pathToFile;
 		char nameOfOutput[200] = "";
 		strncpy(nameOfOutput, lastSlash + 1, strlen(pathToFile) - numberOfLastSlash - 5);	//4 simboliza o numero de caracteres em: .txt
 		
 		char nameToPrint[200] = "";
 		strncpy(nameToPrint, nameOfOutput, strlen(nameOfOutput));

 		strcat(nameOfOutput, "_temp.txt");
 		
 		FILE * outFile = NULL;

 		outFile = fopen(nameOfOutput, "w");
 		 if (outFile == NULL)
  		{
    		fclose (outFile);
  		}
 		//----------------------
 			while (fgets(line, MAX_SIZE_OF_LINE, readStream) != NULL)
 			{
 			//Introduz a palavra
 			char * lineNumber;										//char * com o numero da linha
 			char destination[200];										//char * com a informacao da palavra
 			int number;												//int com a posicao na string
 			lineNumber = strchr(line, ':');							//le ate aos 2 pontos
 			number = lineNumber - line;
 			strncpy(destination, line + number + 1, strlen(line) - number);	//copia o nome da palavra

 			destination[strlen(destination) - 1] = '\0';

 			//introduz os 2 pontos
 			strcat(destination, " : ");								
 			
 			//introduz o nome do novo ficheiro
 			strcat(destination, nameToPrint);

 			//introduz o hifen
 			strcat(destination, "-");

 			//introduz o numero da linha
 			char num[200];
 			strncpy(num, line, number);
 			strcat(destination, num);

 			strcat(destination, "\n");

 			fputs(destination, outFile);
 		}
		//----------------------------------------------------------------------------------
 		//----CLOSE FILE-------
 		fclose(outFile);
 		//---------------------

 		fclose(readStream);
 		close(swPipe[READING_PIPE]);
 	}
 	exit(CORRECT_EXIT);
 }