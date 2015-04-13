#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define CORRECT_EXIT 0
#define ERROR_EXIT 1
#define READING_PIPE 0
#define WRITING_PIPE 1
#define MAX_SIZE_OF_LINE 50
#define TRUE 1





int main(int argc, char * argv[])
{	
	int stat;

	int cscPipe[2];
	if (pipe (cscPipe) < 0) {
		fprintf(stderr, "\nError creating Pipe\n");
		exit(ERROR_EXIT);
	}

	char * pathToDir = argv[1];
	char * pathToIndex = malloc(strlen(pathToDir) * sizeof(char));
	strncpy(pathToIndex, pathToDir, strlen(pathToDir));
	strcat(pathToIndex, "/index.txt");

	printf("\nPath to index: %s\n", pathToIndex);

	//------------------PUTS CONTENT ON PIPE AND REMOVES FILE---------------------------
	char nameOfFile[20] = "";
	int fileCounter = 1;
	while (TRUE)
	{
		sprintf(nameOfFile, "%d", fileCounter);
		strcat(nameOfFile, "_temp.txt");
		if (access(nameOfFile, F_OK) != 0)
			break;

		pid_t pid = fork();
		if (pid == 0) {
			close(cscPipe[READING_PIPE]);		
			dup2(cscPipe[WRITING_PIPE], STDOUT_FILENO);
			execlp("cat", "cat", nameOfFile, NULL);
		}
		else if (pid < 0)
		{
			fprintf(stderr, "\nError in Fork in csc\n");
			exit(ERROR_EXIT);
		}
		else
			wait(&stat);
		fileCounter++;

		//------------REMOVES FILE----------------------
		pid_t pid2 = fork();
		if (pid2 == 0)
			execlp("rm", "rm", nameOfFile, NULL);
		else if (pid2 < 0)
		{
			fprintf(stderr, "\nError in Fork in csc\n");
			exit(ERROR_EXIT);
		}
		else
			wait(&stat);
		//----------------------------------------------
	}
	//----------------------------------------------------------------------------------

	close(cscPipe[WRITING_PIPE]);
	FILE *readStream;
	char line[MAX_SIZE_OF_LINE];
	readStream = fdopen(cscPipe[READING_PIPE], "r");

	//---------------------------WRITES EVERYTHING ON FILE------------------------------
	FILE * outFile = NULL;
	outFile = fopen(pathToIndex, "w");
	if (outFile == NULL)
		fclose (outFile);

	while (fgets(line, MAX_SIZE_OF_LINE, readStream) != NULL)
		fputs(line, outFile);

	fclose(outFile);
	//----------------------------------------------------------------------------------
	int sortPipe[2];
	if (pipe (sortPipe) < 0) {
		fprintf(stderr, "\nError creating Pipe\n");
		exit(ERROR_EXIT);
	}
	//----------------------------SORT OF THE FILE INTO THE PIPE------------------------
	char pathToIndexTemp[strlen(pathToIndex) + 4];
	strncpy(pathToIndexTemp, pathToIndex, strlen(pathToIndex) - 4);
	strcat(pathToIndexTemp, "temp.txt");
	pid_t pid = fork();
	if (pid == 0) {
		execlp("sort", "sort", "-V", "-f", pathToIndex, "-o", pathToIndexTemp, NULL);
	}
	else if (pid < 0)
	{
		fprintf(stderr, "\nError in Fork in csc\n");
		exit(ERROR_EXIT);
	}
	else {
		wait(&stat);
	}
	//----------------------------------------------------------------------------------
	//---------------------------WRITES EVERYTHING ON FILE------------------------------

	printf("\n\n%s\n%s\n\n", pathToIndexTemp, pathToIndex);

	readStream = NULL;
	outFile = NULL;

	readStream = fopen(pathToIndexTemp, "r");
	outFile = fopen(pathToIndex, "w");
		
	if (outFile == NULL)
		fclose (outFile);

	if (readStream == NULL)
		fclose(readStream);

	char word[1000] = "";
	while (fgets(line, MAX_SIZE_OF_LINE, readStream) != NULL)
	{
		int i;
		char wordTemp[MAX_SIZE_OF_LINE] = "";
		for (i = 0; line[i] != ' ' && i < (MAX_SIZE_OF_LINE - 1); i++)
			wordTemp[i] = line[i];
		wordTemp[i] = '\0';

		if (strncmp(word, wordTemp, strlen(wordTemp)) != 0)
		{
			strcat(word, "\n");
			fputs(word, outFile);
			word[0] = '\0';
			strncpy(word, wordTemp, strlen(wordTemp));
			strcat(word, " : ");
			char a[10] = "";
			strncpy(a, line + strlen(wordTemp) + 3, strlen(line) - 3 - strlen(wordTemp));
			strcat(word, a);
		}
		else
		{
			char a[10] = "";
			strncpy(a, line + strlen(wordTemp) + 3, strlen(line) - 3 - strlen(wordTemp));
			strcat(word, " , ");
			strcat(word, a);
		}
	}
	fputs(word, outFile);

	fclose(outFile);
	fclose(readStream);
	
	//----------------------------------------------------------------------------------

	printf("\nCSC called\n%s\n", pathToIndex);

	exit(CORRECT_EXIT);
}